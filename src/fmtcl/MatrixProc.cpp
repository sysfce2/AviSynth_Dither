/*****************************************************************************

        MatrixProc.cpp
        Author: Laurent de Soras, 2015

TO DO:
	- Make the SSE2 code use aligned read/write.
	- Make a special case for kRkGkB matrix conversions, where the luma plane
		is not used to compute the chroma planes.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include "fmtcl/MatrixProc.h"
#include "fmtcl/MatrixProc_macro.h"
#include "fmtcl/ProxyRwCpp.h"
#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fmtcl/ProxyRwSse2.h"
#endif
#include "fstb/fnc.h"

#include <algorithm>

#include <cassert>
#include <climits>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



MatrixProc::MatrixProc (bool sse_flag, bool sse2_flag, bool avx_flag, bool avx2_flag)
:	_sse_flag (sse_flag)
,	_sse2_flag (sse2_flag)
,	_avx_flag (avx_flag)
,	_avx2_flag (avx2_flag)
,	_proc_ptr (0)
,	_coef_flt_arr ()
,	_coef_int_arr ()
,	_coef_simd_arr ()
{
	// Nothing
}



// plane_out = -1: all planes are processed
MatrixProc::Err	MatrixProc::configure (const Mat4 &m, bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, int plane_out)
{
	assert (&m != 0);
	assert (src_fmt >= 0);
	assert (src_fmt < SplFmt_NBR_ELT);
	assert (src_bits >= 8);
	assert (src_bits <= 32);
	assert (dst_fmt >= 0);
	assert (dst_fmt < SplFmt_NBR_ELT);
	assert (dst_bits >= 8);
	assert (dst_bits <= 32);
	assert (plane_out <= NBR_PLANES);

	Err            ret_val = Err_OK;
	_proc_ptr = 0;

	const bool     single_plane_flag = (plane_out >= 0);

	// Integer
	if (int_proc_flag)
	{
		ret_val = set_matrix_int (m, plane_out, src_bits, dst_bits);

		if (ret_val == Err_OK)
		{
#define fmtcl_MatrixProc_CASE_INT(DF, DB, SF, SB) \
			case   (fmtcl::SplFmt_##DF << 18) + (DB << 11) \
			     + (fmtcl::SplFmt_##SF <<  8) + (SB <<  1) + 0: \
				_proc_ptr = &ThisType::process_3_int_cpp < \
					ProxyRwCpp <fmtcl::SplFmt_##DF>, DB, \
					ProxyRwCpp <fmtcl::SplFmt_##SF>, SB \
				>; \
				break; \
			case   (fmtcl::SplFmt_##DF << 18) + (DB << 11) \
			     + (fmtcl::SplFmt_##SF <<  8) + (SB <<  1) + 1: \
				_proc_ptr = &ThisType::process_1_int_cpp < \
					ProxyRwCpp <fmtcl::SplFmt_##DF>, DB, \
					ProxyRwCpp <fmtcl::SplFmt_##SF>, SB \
				>; \
				break;

			switch (
				  (dst_fmt  << 18)
				+ (dst_bits << 11)
				+ (src_fmt  <<  8)
				+ (src_bits <<  1)
				+ (single_plane_flag ? 1 : 0)
			)
			{
			fmtcl_MatrixProc_SPAN_I (fmtcl_MatrixProc_CASE_INT)
			default:
				ret_val = Err_INVALID_FORMAT_COMBINATION;
				break;
			}
#undef fmtcl_MatrixProc_CASE_INT
		}
	}

	// Float
	else
	{
		set_matrix_flt (m, plane_out);
		if (single_plane_flag)
		{
			_proc_ptr = &ThisType::process_1_flt_cpp;
		}
		else
		{
			_proc_ptr = &ThisType::process_3_flt_cpp;
		}
	}

#if (fstb_ARCHI == fstb_ARCHI_X86)
	if (ret_val == Err_OK)
	{
		if (_sse_flag)
		{
			setup_fnc_sse (
				int_proc_flag,
				src_fmt, src_bits,
				dst_fmt, dst_bits,
				single_plane_flag
			);
		}

		if (_sse2_flag)
		{
			setup_fnc_sse2 (
				int_proc_flag,
				src_fmt, src_bits,
				dst_fmt, dst_bits,
				single_plane_flag
			);
		}

		if (_avx_flag)
		{
			setup_fnc_avx (
				int_proc_flag,
				src_fmt, src_bits,
				dst_fmt, dst_bits,
				single_plane_flag
			);
		}

		if (_avx2_flag)
		{
			setup_fnc_avx2 (
				int_proc_flag,
				src_fmt, src_bits,
				dst_fmt, dst_bits,
				single_plane_flag
			);
		}
	}
#endif   // fstb_ARCHI_X86

	return (ret_val);
}



void	MatrixProc::process (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (_proc_ptr != 0);

	(this->*_proc_ptr) (
		dst_ptr_arr, dst_str_arr,
		src_ptr_arr, src_str_arr,
		w, h
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	MatrixProc::set_matrix_flt (const Mat4 &m, int plane_out)
{
	assert (&m != 0);
	assert (plane_out <= NBR_PLANES);

	const int      plane_beg = (plane_out >= 0) ? plane_out     : 0;
	const int      plane_end = (plane_out >= 0) ? plane_out + 1 : NBR_PLANES;

	_coef_flt_arr.resize (NBR_PLANES * MAT_SIZE, 0);
	for (int y = plane_beg; y < plane_end; ++y)
	{
		const int      y_dest = (plane_out >= 0) ? 0 : y;
		for (int x = 0; x < MAT_SIZE; ++x)
		{
			const float    c = float (m [y] [x]);
			_coef_flt_arr [y_dest * MAT_SIZE + x] = c;
		}
	}
}



MatrixProc::Err	MatrixProc::set_matrix_int (const Mat4 &m, int plane_out, int src_bits, int dst_bits)
{
	assert (&m != 0);
	assert (plane_out <= NBR_PLANES);
	assert (src_bits >= 8);
	assert (src_bits <= 16);
	assert (dst_bits >= 8);
	assert (dst_bits <= 16);

	Err            ret_val   = Err_OK;

	const int      plane_beg = (plane_out >= 0) ? plane_out     : 0;
	const int      plane_end = (plane_out >= 0) ? plane_out + 1 : NBR_PLANES;

	_coef_int_arr.resize (NBR_PLANES * MAT_SIZE, 0);

#if (fstb_ARCHI == fstb_ARCHI_X86)
	if (_sse2_flag || _avx2_flag)
	{
		if (_avx2_flag)
		{
			_coef_simd_arr.set_avx2_mode (true);
		}
		_coef_simd_arr.resize (NBR_PLANES * MAT_SIZE);
	}
#endif

	// Coefficient scale
	const double   cintsc    = double ((uint64_t (1)) << SHIFT_INT);

	// Rounding constant
	const int      div_shift = SHIFT_INT + src_bits - dst_bits;
	const int      rnd       = 1 << (div_shift - 1);

	for (int y = plane_beg; y < plane_end; ++y)
	{
#if (fstb_ARCHI == fstb_ARCHI_X86)
		// SSE2/AVX2 only
		// Compensates for the sign in 16 bits.
		// We need to take both source and destination into account.
		// Real formula:
		//		result = (sum of (plane_i * coef_i)) + cst
		// Executed formula:
		//		result - bias_d = (sum of ((plane_i - bias_s) * coef_i)) + biased_cst
		// therefore:
		//		biased_cst = cst - bias_d + bias_s * sum of (coef_i)
		double         bias_flt = (dst_bits == 16) ? -1 : 0;
#endif   // fstb_ARCHI_X86

		for (int x = 0; x < MAT_SIZE; ++x)
		{
			const bool     add_flag = (x == NBR_PLANES);
			const int      y_dest   = (plane_out >= 0) ? 0 : y;
			const int      index    = y_dest * MAT_SIZE + x;

			const double   c        = m [y] [x];
			double         scaled_c = c * cintsc;

			const double   chk_c    =
				scaled_c * double ((uint64_t (1)) << src_bits);
			if (   ! add_flag
			    && (chk_c < INT_MIN || chk_c > INT_MAX))
			{
				ret_val = Err_POSSIBLE_OVERFLOW;
			}

			const int      c_int = fstb::round_int (scaled_c);

			// Coefficient for the C++ version
			int            c_cpp = c_int;
			if (add_flag)
			{
				// Combines the additive and rounding constants to save one add.
				c_cpp += rnd;
			}
			_coef_int_arr [index] = c_cpp;

#if (fstb_ARCHI == fstb_ARCHI_X86)
			// Coefficient for the SSE2/AVX2 version
			if (_sse2_flag || _avx2_flag)
			{
				// Default: normal integer coefficient
				int            c_sse2 = c_int;

				// Multiplicative coefficient
				if (! add_flag)
				{
					if (src_bits == 16)
					{
						bias_flt += c;
					}

					if (c_sse2 < -0x8000 || c_sse2 > 0x7FFF)
					{
						ret_val = Err_TOO_BIG_COEF;
					}
					_coef_simd_arr.set_coef (index, c_sse2);
				}

				// Additive coefficient
				else
				{
					if (dst_bits == 16 || src_bits == 16)
					{
						const double   scale = double (
							(uint64_t (1)) << (src_bits + SHIFT_INT - 1)
						);
						const int      bias = fstb::round_int (bias_flt * scale);

						c_sse2 += bias;
					}

					// Finally, the rounding constant
					c_sse2 += rnd;

					// Stores the additive constant in 32 bits
					_coef_simd_arr.set_coef_int32 (index, c_sse2);
				}  // if add_flag
			}  // if _sse2_flag
#endif   // fstb_ARCHI_X86
		}  // for x
	}  // for y

	return (ret_val);
}



void	MatrixProc::setup_fnc_sse (bool int_proc_flag, SplFmt /*src_fmt*/, int /*src_bits*/, SplFmt /*dst_fmt*/, int /*dst_bits*/, bool single_plane_flag)
{
	if (! int_proc_flag)
	{
		if (single_plane_flag)
		{
			_proc_ptr = &ThisType::process_1_flt_sse;
		}
		else
		{
			_proc_ptr = &ThisType::process_3_flt_sse;
		}
	}
}



void	MatrixProc::setup_fnc_sse2 (bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool single_plane_flag)
{
	if (int_proc_flag)
	{
#define fmtcl_MatrixProc_CASE_INT(DF, DB, SF, SB) \
		case   (fmtcl::SplFmt_##DF << 18) + (DB << 11) \
		     + (fmtcl::SplFmt_##SF <<  8) + (SB <<  1) + 0: \
			_proc_ptr = &ThisType::process_n_int_sse2 < \
				ProxyRwSse2 <fmtcl::SplFmt_##DF>, DB, \
				ProxyRwSse2 <fmtcl::SplFmt_##SF>, SB, 3 \
			>; \
			break; \
		case   (fmtcl::SplFmt_##DF << 18) + (DB << 11) \
		     + (fmtcl::SplFmt_##SF <<  8) + (SB <<  1) + 1: \
			_proc_ptr = &ThisType::process_n_int_sse2 < \
				ProxyRwSse2 <fmtcl::SplFmt_##DF>, DB, \
				ProxyRwSse2 <fmtcl::SplFmt_##SF>, SB, 1 \
			>; \
			break;

		switch (
			  ((int_proc_flag     ? 1 : 0) << 21)
			+ ( dst_fmt                    << 18)
			+ ( dst_bits                   << 11)
			+ ( src_fmt                    <<  8)
			+ ( src_bits                   <<  1)
			+  (single_plane_flag ? 1 : 0)
		)
		{
		fmtcl_MatrixProc_SPAN_I (fmtcl_MatrixProc_CASE_INT)
		// No default, format combination is already checked
		// and the C++ code fills all the possibilities.
		}
#undef fmtcl_MatrixProc_CASE_INT
	}
}



template <typename DST, int DB, class SRC, int SB>
void	MatrixProc::process_3_int_cpp (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");

	typedef typename SRC::PtrConst::Type SrcPtr;
	typedef typename DST::Ptr::Type      DstPtr;

	const int      sizeof_st = int (sizeof (typename SRC::PtrConst::DataType));
	assert (src_str_arr [0] % sizeof_st == 0);
	assert (src_str_arr [1] % sizeof_st == 0);
	assert (src_str_arr [2] % sizeof_st == 0);
	const int      sizeof_dt = int (sizeof (typename DST::Ptr::DataType));
	assert (dst_str_arr [0] % sizeof_dt == 0);
	assert (dst_str_arr [1] % sizeof_dt == 0);
	assert (dst_str_arr [2] % sizeof_dt == 0);

	SrcPtr         src_0_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [0], src_str_arr [0], h);
	SrcPtr         src_1_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [1], src_str_arr [1], h);
	SrcPtr         src_2_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [2], src_str_arr [2], h);
	const int      src_0_str = src_str_arr [0] / sizeof_st - w;
	const int      src_1_str = src_str_arr [1] / sizeof_st - w;
	const int      src_2_str = src_str_arr [2] / sizeof_st - w;

	DstPtr         dst_0_ptr = DST::Ptr::make_ptr (dst_ptr_arr [0], dst_str_arr [0], h);
	DstPtr         dst_1_ptr = DST::Ptr::make_ptr (dst_ptr_arr [1], dst_str_arr [1], h);
	DstPtr         dst_2_ptr = DST::Ptr::make_ptr (dst_ptr_arr [2], dst_str_arr [2], h);
	const int      dst_0_str = dst_str_arr [0] / sizeof_dt - w;
	const int      dst_1_str = dst_str_arr [1] / sizeof_dt - w;
	const int      dst_2_str = dst_str_arr [2] / sizeof_dt - w;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      s0 = SRC::read (src_0_ptr);
			const int      s1 = SRC::read (src_1_ptr);
			const int      s2 = SRC::read (src_2_ptr);

			const int      d0 = (  s0 * _coef_int_arr [ 0]
			                     + s1 * _coef_int_arr [ 1]
			                     + s2 * _coef_int_arr [ 2]
			                     +      _coef_int_arr [ 3]) >> (SHIFT_INT + SB - DB);
			const int      d1 = (  s0 * _coef_int_arr [ 4]
			                     + s1 * _coef_int_arr [ 5]
			                     + s2 * _coef_int_arr [ 6]
			                     +      _coef_int_arr [ 7]) >> (SHIFT_INT + SB - DB);
			const int      d2 = (  s0 * _coef_int_arr [ 8]
			                     + s1 * _coef_int_arr [ 9]
			                     + s2 * _coef_int_arr [10]
			                     +      _coef_int_arr [11]) >> (SHIFT_INT + SB - DB);

			DST::template write_clip <DB> (dst_0_ptr, d0);
			DST::template write_clip <DB> (dst_1_ptr, d1);
			DST::template write_clip <DB> (dst_2_ptr, d2);

			SRC::PtrConst::jump (src_0_ptr, 1);
			SRC::PtrConst::jump (src_1_ptr, 1);
			SRC::PtrConst::jump (src_2_ptr, 1);

			DST::Ptr::jump (dst_0_ptr, 1);
			DST::Ptr::jump (dst_1_ptr, 1);
			DST::Ptr::jump (dst_2_ptr, 1);
		}

		SRC::PtrConst::jump (src_0_ptr, src_0_str);
		SRC::PtrConst::jump (src_1_ptr, src_1_str);
		SRC::PtrConst::jump (src_2_ptr, src_2_str);

		DST::Ptr::jump (dst_0_ptr, dst_0_str);
		DST::Ptr::jump (dst_1_ptr, dst_1_str);
		DST::Ptr::jump (dst_2_ptr, dst_2_str);
	}
}



template <typename DST, int DB, class SRC, int SB>
void	MatrixProc::process_1_int_cpp (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");

	typedef typename SRC::PtrConst::Type SrcPtr;
	typedef typename DST::Ptr::Type      DstPtr;

	const int      sizeof_st = int (sizeof (typename SRC::PtrConst::DataType));
	assert (src_str_arr [0] % sizeof_st == 0);
	assert (src_str_arr [1] % sizeof_st == 0);
	assert (src_str_arr [2] % sizeof_st == 0);
	const int      sizeof_dt = int (sizeof (typename DST::Ptr::DataType));
	assert (dst_str_arr [0] % sizeof_dt == 0);

	SrcPtr         src_0_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [0], src_str_arr [0], h);
	SrcPtr         src_1_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [1], src_str_arr [1], h);
	SrcPtr         src_2_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [2], src_str_arr [2], h);
	const int      src_0_str = src_str_arr [0] / sizeof_st - w;
	const int      src_1_str = src_str_arr [1] / sizeof_st - w;
	const int      src_2_str = src_str_arr [2] / sizeof_st - w;

	DstPtr         dst_0_ptr = DST::Ptr::make_ptr (dst_ptr_arr [0], dst_str_arr [0], h);
	const int      dst_0_str = dst_str_arr [0] / sizeof_dt - w;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      s0 = SRC::read (src_0_ptr);
			const int      s1 = SRC::read (src_1_ptr);
			const int      s2 = SRC::read (src_2_ptr);

			const int      d0 = (  s0 * _coef_int_arr [ 0]
			                     + s1 * _coef_int_arr [ 1]
			                     + s2 * _coef_int_arr [ 2]
			                     +      _coef_int_arr [ 3]) >> (SHIFT_INT + SB - DB);

			DST::template write_clip <DB> (dst_0_ptr, d0);

			SRC::PtrConst::jump (src_0_ptr, 1);
			SRC::PtrConst::jump (src_1_ptr, 1);
			SRC::PtrConst::jump (src_2_ptr, 1);

			DST::Ptr::jump (dst_0_ptr, 1);
		}

		SRC::PtrConst::jump (src_0_ptr, src_0_str);
		SRC::PtrConst::jump (src_1_ptr, src_1_str);
		SRC::PtrConst::jump (src_2_ptr, src_2_str);

		DST::Ptr::jump (dst_0_ptr, dst_0_str);
	}
}



void	MatrixProc::process_3_flt_cpp (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");
	const int      sizeof_xt = int (sizeof (float));
	assert (src_str_arr [0] % sizeof_xt == 0);
	assert (src_str_arr [1] % sizeof_xt == 0);
	assert (src_str_arr [2] % sizeof_xt == 0);
	assert (dst_str_arr [0] % sizeof_xt == 0);
	assert (dst_str_arr [1] % sizeof_xt == 0);
	assert (dst_str_arr [2] % sizeof_xt == 0);

	const float *  src_0_ptr = reinterpret_cast <const float *> (src_ptr_arr [0]);
	const float *  src_1_ptr = reinterpret_cast <const float *> (src_ptr_arr [1]);
	const float *  src_2_ptr = reinterpret_cast <const float *> (src_ptr_arr [2]);
	const int      src_0_str = src_str_arr [0] / sizeof_xt;
	const int      src_1_str = src_str_arr [1] / sizeof_xt;
	const int      src_2_str = src_str_arr [2] / sizeof_xt;

	float *        dst_0_ptr = reinterpret_cast <      float *> (dst_ptr_arr [0]);
	float *        dst_1_ptr = reinterpret_cast <      float *> (dst_ptr_arr [1]);
	float *        dst_2_ptr = reinterpret_cast <      float *> (dst_ptr_arr [2]);
	const int      dst_0_str = dst_str_arr [0] / sizeof_xt;
	const int      dst_1_str = dst_str_arr [1] / sizeof_xt;
	const int      dst_2_str = dst_str_arr [2] / sizeof_xt;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const float    s0 = src_0_ptr [x];
			const float    s1 = src_1_ptr [x];
			const float    s2 = src_2_ptr [x];

			const float    d0 =   s0 * _coef_flt_arr [ 0]
			                    + s1 * _coef_flt_arr [ 1]
			                    + s2 * _coef_flt_arr [ 2]
			                    +      _coef_flt_arr [ 3];
			const float    d1 =   s0 * _coef_flt_arr [ 4]
			                    + s1 * _coef_flt_arr [ 5]
			                    + s2 * _coef_flt_arr [ 6]
			                    +      _coef_flt_arr [ 7];
			const float    d2 =   s0 * _coef_flt_arr [ 8]
			                    + s1 * _coef_flt_arr [ 9]
			                    + s2 * _coef_flt_arr [10]
			                    +      _coef_flt_arr [11];

			dst_0_ptr [x] = d0;
			dst_1_ptr [x] = d1;
			dst_2_ptr [x] = d2;
		}

		src_0_ptr += src_0_str;
		src_1_ptr += src_1_str;
		src_2_ptr += src_2_str;

		dst_0_ptr += dst_0_str;
		dst_1_ptr += dst_1_str;
		dst_2_ptr += dst_2_str;
	}
}



void	MatrixProc::process_1_flt_cpp (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");
	const int      sizeof_xt = int (sizeof (float));
	assert (src_str_arr [0] % sizeof_xt == 0);
	assert (src_str_arr [1] % sizeof_xt == 0);
	assert (src_str_arr [2] % sizeof_xt == 0);
	assert (dst_str_arr [0] % sizeof_xt == 0);

	const float *  src_0_ptr = reinterpret_cast <const float *> (src_ptr_arr [0]);
	const float *  src_1_ptr = reinterpret_cast <const float *> (src_ptr_arr [1]);
	const float *  src_2_ptr = reinterpret_cast <const float *> (src_ptr_arr [2]);
	const int      src_0_str = src_str_arr [0] / sizeof_xt;
	const int      src_1_str = src_str_arr [1] / sizeof_xt;
	const int      src_2_str = src_str_arr [2] / sizeof_xt;

	float *        dst_0_ptr = reinterpret_cast <      float *> (dst_ptr_arr [0]);
	const int      dst_0_str = dst_str_arr [0] / sizeof_xt;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const float    s0 = src_0_ptr [x];
			const float    s1 = src_1_ptr [x];
			const float    s2 = src_2_ptr [x];

			const float    d0 =   s0 * _coef_flt_arr [ 0]
			                    + s1 * _coef_flt_arr [ 1]
			                    + s2 * _coef_flt_arr [ 2]
			                    +      _coef_flt_arr [ 3];

			dst_0_ptr [x] = d0;
		}

		src_0_ptr += src_0_str;
		src_1_ptr += src_1_str;
		src_2_ptr += src_2_str;

		dst_0_ptr += dst_0_str;
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



// DST and SRC are ProxyRwSse2 classes
template <class DST, int DB, class SRC, int SB, int NP>
void	MatrixProc::process_n_int_sse2 (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");

	enum { BPS_SRC = (SB + 7) >> 3 };
	enum { BPS_DST = (DB + 7) >> 3 };

	typedef typename SRC::PtrConst::Type SrcPtr;
	typedef typename DST::Ptr::Type      DstPtr;

	const int      packsize  = 8;
	const int      sizeof_st = int (sizeof (typename SRC::PtrConst::DataType));
	assert (src_str_arr [0] % sizeof_st == 0);
	assert (src_str_arr [1] % sizeof_st == 0);
	assert (src_str_arr [2] % sizeof_st == 0);

	const __m128i  zero     = _mm_setzero_si128 ();
	const __m128i  mask_lsb = _mm_set1_epi16 (0x00FF);
	const __m128i  sign_bit = _mm_set1_epi16 (-0x8000);
	const __m128i  ma       = _mm_set1_epi16 (int16_t (uint16_t ((1 << DB) - 1)));

	const __m128i* coef_ptr = reinterpret_cast <const __m128i *> (
		_coef_simd_arr.use_vect_sse2 (0)
	);

	// Looping over lines then over planes helps keeping input data
	// in the cache.
	const int      wp = (w + (packsize - 1)) & -packsize;
	SrcPtr         src_0_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [0], src_str_arr [0], h);
	SrcPtr         src_1_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [1], src_str_arr [1], h);
	SrcPtr         src_2_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [2], src_str_arr [2], h);
	const int      src_0_str = src_str_arr [0] / sizeof_st;
	const int      src_1_str = src_str_arr [1] / sizeof_st;
	const int      src_2_str = src_str_arr [2] / sizeof_st;

	for (int y = 0; y < h; ++y)
	{
		for (int plane_index = 0; plane_index < NP; ++ plane_index)
		{
			DstPtr         dst_ptr (DST::Ptr::make_ptr (
				dst_ptr_arr [plane_index] + y * dst_str_arr [plane_index],
				dst_str_arr [plane_index],
				h
			));
			const int      cind    = plane_index * MAT_SIZE;

			for (int x = 0; x < w; x += packsize)
			{
				typedef typename SRC::template S16 <false     , (SB == 16)> SrcS16R;
				typedef typename DST::template S16 <(DB != 16), (DB == 16)> DstS16W;

				const __m128i  s0 = SrcS16R::read (src_0_ptr, zero, sign_bit);
				const __m128i  s1 = SrcS16R::read (src_1_ptr, zero, sign_bit);
				const __m128i  s2 = SrcS16R::read (src_2_ptr, zero, sign_bit);

				__m128i        d0 = _mm_load_si128 (coef_ptr + cind + NBR_PLANES);
				__m128i        d1 = d0;

				// src is variable, up to 16-bit signed (full range, +1 = 32767+1)
				// coef is 13-bit signed (+1 = 4096)
				// dst1 and dst2 are 28-bit signed (+1 = 2 ^ 27) packed on 32-bit int.
				// Maximum headroom: *16 (4 bits)
				fstb::ToolsSse2::mac_s16_s16_s32 (
					d0, d1, s0, _mm_load_si128 (coef_ptr + cind + 0));
				fstb::ToolsSse2::mac_s16_s16_s32 (
					d0, d1, s1, _mm_load_si128 (coef_ptr + cind + 1));
				fstb::ToolsSse2::mac_s16_s16_s32 (
					d0, d1, s2, _mm_load_si128 (coef_ptr + cind + 2));

				d0 = _mm_srai_epi32 (d0, SHIFT_INT + SB - DB);
				d1 = _mm_srai_epi32 (d1, SHIFT_INT + SB - DB);

				__m128i			val = _mm_packs_epi32 (d0, d1);

				DstS16W::write_clip (dst_ptr, val, mask_lsb, zero, ma, sign_bit);

				SRC::PtrConst::jump (src_0_ptr, packsize);
				SRC::PtrConst::jump (src_1_ptr, packsize);
				SRC::PtrConst::jump (src_2_ptr, packsize);

				DST::Ptr::jump (dst_ptr, packsize);
			}

			SRC::PtrConst::jump (src_0_ptr, -wp);
			SRC::PtrConst::jump (src_1_ptr, -wp);
			SRC::PtrConst::jump (src_2_ptr, -wp);
		}

		SRC::PtrConst::jump (src_0_ptr, src_0_str);
		SRC::PtrConst::jump (src_1_ptr, src_1_str);
		SRC::PtrConst::jump (src_2_ptr, src_2_str);
	}
}



void	MatrixProc::process_3_flt_sse (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");
	const int      sizeof_xt = int (sizeof (float));
	assert (src_str_arr [0] % sizeof_xt == 0);
	assert (src_str_arr [1] % sizeof_xt == 0);
	assert (src_str_arr [2] % sizeof_xt == 0);
	assert (dst_str_arr [0] % sizeof_xt == 0);
	assert (dst_str_arr [1] % sizeof_xt == 0);
	assert (dst_str_arr [2] % sizeof_xt == 0);

	const float *  src_0_ptr = reinterpret_cast <const float *> (src_ptr_arr [0]);
	const float *  src_1_ptr = reinterpret_cast <const float *> (src_ptr_arr [1]);
	const float *  src_2_ptr = reinterpret_cast <const float *> (src_ptr_arr [2]);
	const int      src_0_str = src_str_arr [0] / sizeof_xt;
	const int      src_1_str = src_str_arr [1] / sizeof_xt;
	const int      src_2_str = src_str_arr [2] / sizeof_xt;

	float *        dst_0_ptr = reinterpret_cast <      float *> (dst_ptr_arr [0]);
	float *        dst_1_ptr = reinterpret_cast <      float *> (dst_ptr_arr [1]);
	float *        dst_2_ptr = reinterpret_cast <      float *> (dst_ptr_arr [2]);
	const int      dst_0_str = dst_str_arr [0] / sizeof_xt;
	const int      dst_1_str = dst_str_arr [1] / sizeof_xt;
	const int      dst_2_str = dst_str_arr [2] / sizeof_xt;

	const __m128   c00 = _mm_set1_ps (_coef_flt_arr [ 0]);
	const __m128   c01 = _mm_set1_ps (_coef_flt_arr [ 1]);
	const __m128   c02 = _mm_set1_ps (_coef_flt_arr [ 2]);
	const __m128   c03 = _mm_set1_ps (_coef_flt_arr [ 3]);
	const __m128   c04 = _mm_set1_ps (_coef_flt_arr [ 4]);
	const __m128   c05 = _mm_set1_ps (_coef_flt_arr [ 5]);
	const __m128   c06 = _mm_set1_ps (_coef_flt_arr [ 6]);
	const __m128   c07 = _mm_set1_ps (_coef_flt_arr [ 7]);
	const __m128   c08 = _mm_set1_ps (_coef_flt_arr [ 8]);
	const __m128   c09 = _mm_set1_ps (_coef_flt_arr [ 9]);
	const __m128   c10 = _mm_set1_ps (_coef_flt_arr [10]);
	const __m128   c11 = _mm_set1_ps (_coef_flt_arr [11]);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; x += 4)
		{
			const __m128   s0 = _mm_load_ps (src_0_ptr + x);
			const __m128   s1 = _mm_load_ps (src_1_ptr + x);
			const __m128   s2 = _mm_load_ps (src_2_ptr + x);

			const __m128   d0 = _mm_add_ps (_mm_add_ps (_mm_add_ps (
				_mm_mul_ps (s0, c00),
				_mm_mul_ps (s1, c01)),
				_mm_mul_ps (s2, c02)),
				                c03);
			const __m128   d1 = _mm_add_ps (_mm_add_ps (_mm_add_ps (
				_mm_mul_ps (s0, c04),
				_mm_mul_ps (s1, c05)),
				_mm_mul_ps (s2, c06)),
				                c07);
			const __m128   d2 = _mm_add_ps (_mm_add_ps (_mm_add_ps (
				_mm_mul_ps (s0, c08),
				_mm_mul_ps (s1, c09)),
				_mm_mul_ps (s2, c10)),
				                c11);

			_mm_store_ps (dst_0_ptr + x, d0);
			_mm_store_ps (dst_1_ptr + x, d1);
			_mm_store_ps (dst_2_ptr + x, d2);
		}

		src_0_ptr += src_0_str;
		src_1_ptr += src_1_str;
		src_2_ptr += src_2_str;

		dst_0_ptr += dst_0_str;
		dst_1_ptr += dst_1_str;
		dst_2_ptr += dst_2_str;
	}
}



void	MatrixProc::process_1_flt_sse (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");
	const int      sizeof_xt = int (sizeof (float));
	assert (src_str_arr [0] % sizeof_xt == 0);
	assert (src_str_arr [1] % sizeof_xt == 0);
	assert (src_str_arr [2] % sizeof_xt == 0);
	assert (dst_str_arr [0] % sizeof_xt == 0);

	const float *  src_0_ptr = reinterpret_cast <const float *> (src_ptr_arr [0]);
	const float *  src_1_ptr = reinterpret_cast <const float *> (src_ptr_arr [1]);
	const float *  src_2_ptr = reinterpret_cast <const float *> (src_ptr_arr [2]);
	const int      src_0_str = src_str_arr [0] / sizeof_xt;
	const int      src_1_str = src_str_arr [1] / sizeof_xt;
	const int      src_2_str = src_str_arr [2] / sizeof_xt;

	float *        dst_0_ptr = reinterpret_cast <      float *> (dst_ptr_arr [0]);
	const int      dst_0_str = dst_str_arr [0] / sizeof_xt;

	const __m128   c00 = _mm_set1_ps (_coef_flt_arr [ 0]);
	const __m128   c01 = _mm_set1_ps (_coef_flt_arr [ 1]);
	const __m128   c02 = _mm_set1_ps (_coef_flt_arr [ 2]);
	const __m128   c03 = _mm_set1_ps (_coef_flt_arr [ 3]);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; x += 4)
		{
			const __m128   s0 = _mm_load_ps (src_0_ptr + x);
			const __m128   s1 = _mm_load_ps (src_1_ptr + x);
			const __m128   s2 = _mm_load_ps (src_2_ptr + x);

			const __m128   d0 = _mm_add_ps (_mm_add_ps (_mm_add_ps (
				_mm_mul_ps (s0, c00),
				_mm_mul_ps (s1, c01)),
				_mm_mul_ps (s2, c02)),
				                c03);

			_mm_store_ps (dst_0_ptr + x, d0);
		}

		src_0_ptr += src_0_str;
		src_1_ptr += src_1_str;
		src_2_ptr += src_2_str;

		dst_0_ptr += dst_0_str;
	}
}



#endif   // fstb_ARCHI_X86



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
