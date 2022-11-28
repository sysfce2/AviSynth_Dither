/*****************************************************************************

        AvsFilterMedian16.cpp
        Author: Laurent de Soras, 2012

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

#include	"fstb/CpuId.h"
#include	"fstb/ToolsSse2.h"
#include	"AvsFilterMedian16.h"

#include	<algorithm>

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterMedian16::AvsFilterMedian16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, int rx, int ry, int rt, int ql, int qh, int y, int u, int v)
:	GenericVideoFilter (src_clip_sptr)
,	_src_clip_sptr (src_clip_sptr)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_rx (rx)
,	_ry (ry)
,	_rt (rt)
,	_ql (ql)
,	_qh (qh)
,	_sse2_flag (false)
,	_avx2_flag (false)
{
	assert (env_ptr != 0);

	const int		range_x = _rx * 2 + 1;
	const int		range_y = _ry * 2 + 1;
	const int		range_t = _rt * 2 + 1;
	const int		sz  = range_x * range_y * range_t;
	const int		mid = (sz - 1) / 2;
	if (_ql < 0)
	{
		_ql = mid;
	}
	if (_qh < 0)
	{
		_qh = mid;
	}

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("Dither_Median16: input must be planar.");
	}

	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_Median16: stacked MSB/LSB data must have\n"
			"a height multiple of twice the maximum subsampling."
		);
	}

	if (_rx < 0 || _ry < 0 || _rt < 0)
	{
		env_ptr->ThrowError ("Dither_Median16: rx, ry and rt must be >= 0.");
	}
	if (   range_x > PlaneProcessor::get_min_w (vi)
	    || range_y > PlaneProcessor::get_min_h (vi, true))
	{
		env_ptr->ThrowError ("Dither_Median16: Picture too small.");
	}

	if (_rt > MAX_RADIUS_T)
	{
		env_ptr->ThrowError ("Dither_Median16: Temporal radius too large.");
	}

	if (sz > MAX_AREA_SIZE)
	{
		env_ptr->ThrowError ("Dither_Median16: Area too large.");
	}

	if (_ql < 0 || _ql > sz - 1)
	{
		env_ptr->ThrowError ("Dither_Median16: ql out of range.");
	}
	if (_qh < 0 || _qh > sz - 1)
	{
		env_ptr->ThrowError ("Dither_Median16: qh out of range.");
	}
	if (_ql > _qh)
	{
		env_ptr->ThrowError ("Dither_Median16: ql must be less or equal to qh.");
	}

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);

	fstb::CpuId    cid;
	_sse2_flag = cid._sse2_flag;
	_avx2_flag = cid._avx2_flag;
}



::PVideoFrame __stdcall	AvsFilterMedian16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_src_clip_sptr, 0, 0
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterMedian16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
{
	const int      range_t  = _rt * 2 + 1;

	const int      w             = _plane_proc.get_width (dst_sptr, plane_id);
	const int      h             = _plane_proc.get_height16 (dst_sptr, plane_id);
	uint8_t *      data_msbd_ptr = dst_sptr->GetWritePtr (plane_id);
	const int      stride_dst    = dst_sptr->GetPitch (plane_id);

	TaskDataGlobal		tdg;

	// Keeps the source frames referenced until we don't need them anymore
	::PVideoFrame		src_ptr_arr [MAX_RANGE_T];

	tdg._this_ptr    = this;
	tdg._w           = w;
	tdg._h           = h;
	tdg._dst_msb_ptr = data_msbd_ptr;
	tdg._dst_lsb_ptr = data_msbd_ptr + h * stride_dst;
	tdg._stride_dst  = stride_dst;

	for (int t = 0; t < range_t; ++t)
	{
		int				ns = n - _rt + t;
		ns = std::max (std::min (ns, vi.num_frames - 1), 0);
		::PVideoFrame  src_sptr = _src_clip_sptr->GetFrame (ns, &env);
		src_ptr_arr [t] = src_sptr;

		const uint8_t* data_msbs_ptr = src_sptr->GetReadPtr (plane_id);
		const int      stride_src    = src_sptr->GetPitch (plane_id);

		tdg._src_arr [t]._msb_ptr = data_msbs_ptr;
		tdg._src_arr [t]._lsb_ptr = data_msbs_ptr + h * stride_src;
		tdg._src_arr [t]._stride  = stride_src;
	}

	Slicer			slicer;
	Slicer::ProcPtr	proc_ptr = &AvsFilterMedian16::process_subplane_rtn;
	if (_rt == 0)
	{
		proc_ptr = &AvsFilterMedian16::process_subplane_rt0;
		if (_avx2_flag && _rx == 1 && _ry == 1 && _ql == 4 && _qh == 4)
		{
			proc_ptr = &AvsFilterMedian16::process_subplane_rt0_avx2;
		}
	}
	slicer.start (h, tdg, proc_ptr);
	slicer.wait ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



static void	sort_pair (__m128i &a1, __m128i &a2)
{
	const __m128i	tmp = _mm_min_epi16 (a1, a2);
	a2 = _mm_max_epi16 (a1, a2);
	a1 = tmp;
}



void	AvsFilterMedian16::process_subplane_rt0 (Slicer::TaskData &td)
{
	assert (_rt == 0);

	const TaskDataGlobal & tdg = *(td._glob_data_ptr);
	uint8_t *      dst_msb_ptr = tdg._dst_msb_ptr          + td._y_beg * tdg._stride_dst;
	uint8_t *      dst_lsb_ptr = tdg._dst_lsb_ptr          + td._y_beg * tdg._stride_dst;
	const int      stride_src  = tdg._src_arr [0]._stride;
	const uint8_t* src_msb_ptr = tdg._src_arr [0]._msb_ptr + td._y_beg * stride_src;
	const uint8_t* src_lsb_ptr = tdg._src_arr [0]._lsb_ptr + td._y_beg * stride_src;
	const int      w           = tdg._w;
	const int      h           = tdg._h;
	const int      x_last      = w - _rx;
	const int      range_x     = _rx * 2 + 1;
	const int      range_y     = _ry * 2 + 1;
	const int		area        = range_x * range_y;
	const int		mid_pos     = (area - 1) / 2;

	const bool		medsse_flag =
		(_sse2_flag && _rx == 1 && _ry == 1 && _ql == 4 && _qh == 4);
	const int		w8          = (x_last - _rx) &  -8;
	const int		x_first     = (medsse_flag) ? _rx + w8 : _rx;

	uint16_t       data [MAX_AREA_SIZE];

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		if (y < _ry || y >= h - _ry)
		{
			memcpy (dst_msb_ptr, src_msb_ptr, w);
			memcpy (dst_lsb_ptr, src_lsb_ptr, w);
		}

		else
		{
			for (int x = 0; x < _rx; ++x)
			{
				dst_msb_ptr [x] = src_msb_ptr [x];
				dst_lsb_ptr [x] = src_lsb_ptr [x];
			}

			// Quick and dirty optimization for 3x3 median filter
			if (medsse_flag)
			{
				const __m128i  mask_sign = _mm_set1_epi16 (-0x8000);
				const __m128i  mask_lsb  = _mm_set1_epi16 (0x00FF);

				for (int x = _rx; x < x_first; x += 8)
				{
					const uint8_t* src2_msb_ptr = src_msb_ptr + x;
					const uint8_t* src2_lsb_ptr = src_lsb_ptr + x;

					// Code from AvsFilterRemoveGrain16
					const int      om = stride_src - 1;
					const int      o0 = stride_src    ;
					const int      op = stride_src + 1;
					__m128i        a1 = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr - op, src2_lsb_ptr - op), mask_sign);
					__m128i        a2 = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr - o0, src2_lsb_ptr - o0), mask_sign);
					__m128i        a3 = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr - om, src2_lsb_ptr - om), mask_sign);
					__m128i        a4 = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr - 1 , src2_lsb_ptr - 1 ), mask_sign);
					__m128i        c  = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr     , src2_lsb_ptr     ), mask_sign);
					__m128i        a5 = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr + 1 , src2_lsb_ptr + 1 ), mask_sign);
					__m128i        a6 = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr + om, src2_lsb_ptr + om), mask_sign);
					__m128i        a7 = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr + o0, src2_lsb_ptr + o0), mask_sign);
					__m128i        a8 = _mm_xor_si128 (fstb::ToolsSse2::load_8_16ml (src2_msb_ptr + op, src2_lsb_ptr + op), mask_sign);

					sort_pair (a1, a2);
					sort_pair (a3, a4);
					sort_pair (a5, a6);
					sort_pair (a7, a8);

					sort_pair (a1, a3);
					sort_pair (a2, a4);
					sort_pair (a5, a7);
					sort_pair (a6, a8);

					sort_pair (a2, a3);
					sort_pair (a6, a7);

					a5 = _mm_max_epi16 (a1, a5);	// sort_pair (a1, a5);
					a6 = _mm_max_epi16 (a2, a6);	// sort_pair (a2, a6);
					a3 = _mm_min_epi16 (a3, a7);	// sort_pair (a3, a7);
					a4 = _mm_min_epi16 (a4, a8);	// sort_pair (a4, a8);

					a5 = _mm_max_epi16 (a3, a5);	// sort_pair (a3, a5);
					a4 = _mm_min_epi16 (a4, a6);	// sort_pair (a4, a6);

															// sort_pair (a2, a3);
					sort_pair (a4, a5);
															// sort_pair (a6, a7);

					__m128i			res = fstb::ToolsSse2::limit_epi16 (c, a4, a5);
					res = _mm_xor_si128 (res, mask_sign);
					fstb::ToolsSse2::store_8_16ml (
						dst_msb_ptr + x,
						dst_lsb_ptr + x,
						res,
						mask_lsb
					);
				}
			}

			for (int x = x_first; x < x_last; ++x)
			{
				// Collects data
				int            pos = 0;
				const int      offset = x - stride_src * _ry - _rx;
				const uint8_t*	sm_ptr = src_msb_ptr + offset;
				const uint8_t*	sl_ptr = src_lsb_ptr + offset;
				for (int y2 = 0; y2 < range_y; ++y2)
				{
					for (int x2 = 0; x2 < range_x; ++x2)
					{
						data [pos] = (sm_ptr [x2] << 8) + sl_ptr [x2];
						++ pos;
					}
					sm_ptr += stride_src;
					sl_ptr += stride_src;
				}

				int				pix = data [mid_pos];

				// Sort
				std::sort (data, data + area);

				// Clipping
				const int		pix_l = data [_ql];
				const int		pix_h = data [_qh];
				pix = std::min (std::max (pix, pix_l), pix_h);

				dst_msb_ptr [x] = uint8_t (pix >> 8);
				dst_lsb_ptr [x] = uint8_t (pix     );
			}

			for (int x = x_last; x < w; ++x)
			{
				dst_msb_ptr [x] = src_msb_ptr [x];
				dst_lsb_ptr [x] = src_lsb_ptr [x];
			}
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		src_msb_ptr += stride_src;
		src_lsb_ptr += stride_src;
	}
}



void	AvsFilterMedian16::process_subplane_rtn (Slicer::TaskData &td)
{
	const TaskDataGlobal & tdg = *(td._glob_data_ptr);
	uint8_t *      dst_msb_ptr = tdg._dst_msb_ptr + td._y_beg * tdg._stride_dst;
	uint8_t *      dst_lsb_ptr = tdg._dst_lsb_ptr + td._y_beg * tdg._stride_dst;

	const int      w           = tdg._w;
	const int      h           = tdg._h;
	const int      range_x     = _rx * 2 + 1;
	const int      range_y     = _ry * 2 + 1;
	const int      range_t     = _rt * 2 + 1;
	const int      x_last      = w - _rx;
	const int		area        = range_x * range_y * range_t;
	const int		mid_pos     = (area - 1) / 2;

	const uint8_t* src_msb_ptr [MAX_RANGE_T];
	const uint8_t* src_lsb_ptr [MAX_RANGE_T];
	for (int t = 0; t < range_t; ++t)
	{
		src_msb_ptr [t] = tdg._src_arr [t]._msb_ptr + td._y_beg * tdg._src_arr [t]._stride;
		src_lsb_ptr [t] = tdg._src_arr [t]._lsb_ptr + td._y_beg * tdg._src_arr [t]._stride;
	}

	uint16_t       data [MAX_AREA_SIZE];

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		if (y < _ry || y >= h - _ry)
		{
			memcpy (dst_msb_ptr, src_msb_ptr [_rt], w);
			memcpy (dst_lsb_ptr, src_lsb_ptr [_rt], w);
		}

		else
		{
			for (int x = 0; x < _rx; ++x)
			{
				dst_msb_ptr [x] = src_msb_ptr [_rt] [x];
				dst_lsb_ptr [x] = src_lsb_ptr [_rt] [x];
			}

			for (int x = _rx; x < x_last; ++x)
			{
				// Collects data
				int            pos = 0;
				for (int t = 0; t < range_t; ++t)
				{
					const int		stride_src = tdg._src_arr [t]._stride;
					const int      offset = x - stride_src * _ry - _rx;
					const uint8_t*	sm_ptr = src_msb_ptr [t] + offset;
					const uint8_t*	sl_ptr = src_lsb_ptr [t] + offset;
					for (int y2 = 0; y2 < range_y; ++y2)
					{
						for (int x2 = 0; x2 < range_x; ++x2)
						{
							data [pos] = (sm_ptr [x2] << 8) + sl_ptr [x2];
							++ pos;
						}
						sm_ptr += stride_src;
						sl_ptr += stride_src;
					}
				}

				int				pix = data [mid_pos];

				// Sort
				std::sort (data, data + area);

				// Clipping
				const int		pix_l = data [_ql];
				const int		pix_h = data [_qh];
				pix = std::min (std::max (pix, pix_l), pix_h);

				dst_msb_ptr [x] = uint8_t (pix >> 8);
				dst_lsb_ptr [x] = uint8_t (pix     );
			}

			for (int x = x_last; x < w; ++x)
			{
				dst_msb_ptr [x] = src_msb_ptr [_rt] [x];
				dst_lsb_ptr [x] = src_lsb_ptr [_rt] [x];
			}
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		for (int t = 0; t < range_t; ++t)
		{
			const int		stride_src = tdg._src_arr [t]._stride;
			src_msb_ptr [t] += stride_src;
			src_lsb_ptr [t] += stride_src;
		}
	}
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
