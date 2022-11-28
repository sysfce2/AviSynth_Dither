/*****************************************************************************

        AvsFilterMaxDif16.cpp
        Author: Laurent de Soras, 2011

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

#include "fstb/CpuId.h"
#include "fstb/ToolsSse2.h"
#include "AvsFilterMaxDif16.h"

#include <cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterMaxDif16::AvsFilterMaxDif16 (::IScriptEnvironment *env_ptr, ::PClip src1_clip_sptr, ::PClip src2_clip_sptr, ::PClip ref_clip_sptr, int y, int u, int v, bool min_flag)
:	GenericVideoFilter (src1_clip_sptr)
,	_src1_clip_sptr (src1_clip_sptr)
,	_src2_clip_sptr (src2_clip_sptr)
,	_ref_clip_sptr (ref_clip_sptr)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_min_flag (min_flag)
,	_sse2_flag (false)
,	_avx2_flag (false)
,	_process_subplane_ptr (0)
{
	const char * const   fncname_0 =
		(_min_flag) ? "Dither_mindif16" : "Dither_maxdif16";

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError (
			  (_min_flag)
			? "Dither_mindif16: input must be planar."
			: "Dither_maxdif16: input must be planar."
		);
	}

	PlaneProcessor::check_same_format (env_ptr, vi, src2_clip_sptr, fncname_0, "src2");
	PlaneProcessor::check_same_format (env_ptr, vi, ref_clip_sptr , fncname_0, "ref" );

	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			  (_min_flag)
			? "Dither_mindif16: stacked MSB/LSB data must have\n"
			  "a height multiple of twice the maximum subsampling."
			: "Dither_maxdif16: stacked MSB/LSB data must have\n"
			  "a height multiple of twice the maximum subsampling."
		);
	}

	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (2, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (3, PlaneProcessor::ClipType_STACKED_16);

	fstb::CpuId    cid;
	_sse2_flag = cid._sse2_flag;
	_avx2_flag = cid._avx2_flag;

	if (_avx2_flag)
	{
		configure_avx2 ();
	}
	else if (_sse2_flag)
	{
		if (_min_flag)
		{
			_process_subplane_ptr = &PlaneProc <OpMin>::process_subplane_sse2;
		}
		else
		{
			_process_subplane_ptr = &PlaneProc <OpMax>::process_subplane_sse2;
		}
	}
	else
	{
		if (_min_flag)
		{
			_process_subplane_ptr = &PlaneProc <OpMin>::process_subplane_cpp;
		}
		else
		{
			_process_subplane_ptr = &PlaneProc <OpMax>::process_subplane_cpp;
		}
	}
}



::PVideoFrame __stdcall	AvsFilterMaxDif16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_src1_clip_sptr, &_src2_clip_sptr, &_ref_clip_sptr
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterMaxDif16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	src1_sptr = _src1_clip_sptr->GetFrame (n, &env);
	::PVideoFrame	src2_sptr = _src2_clip_sptr->GetFrame (n, &env);
	::PVideoFrame	ref_sptr  = _ref_clip_sptr->GetFrame (n, &env);

	const int		w            = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h            = _plane_proc.get_height16 (dst_sptr, plane_id);
	uint8_t *		dst_msb_ptr  = dst_sptr->GetWritePtr (plane_id);
	const int		stride_dst   = dst_sptr->GetPitch (plane_id);

	const uint8_t*	src1_msb_ptr = src1_sptr->GetReadPtr (plane_id);
	const int		stride_src1  = src1_sptr->GetPitch (plane_id);
	const uint8_t*	src2_msb_ptr = src2_sptr->GetReadPtr (plane_id);
	const int		stride_src2  = src2_sptr->GetPitch (plane_id);
	const uint8_t*	ref_msb_ptr  = ref_sptr->GetReadPtr (plane_id);
	const int		stride_ref   = ref_sptr->GetPitch (plane_id);

	TaskDataGlobal		tdg;
	tdg._this_ptr    = this;
	tdg._w           = w;
	tdg._dst_msb_ptr = dst_msb_ptr;
	tdg._dst_lsb_ptr = dst_msb_ptr  + h * stride_dst;
	tdg._sr1_msb_ptr = src1_msb_ptr;
	tdg._sr1_lsb_ptr = src1_msb_ptr + h * stride_src1;
	tdg._sr2_msb_ptr = src2_msb_ptr;
	tdg._sr2_lsb_ptr = src2_msb_ptr + h * stride_src2;
	tdg._ref_msb_ptr = ref_msb_ptr;
	tdg._ref_lsb_ptr = ref_msb_ptr  + h * stride_ref;
	tdg._stride_dst  = stride_dst;
	tdg._stride_sr1  = stride_src1;
	tdg._stride_sr2  = stride_src2;
	tdg._stride_ref  = stride_ref;

	Slicer			slicer;
	slicer.start (h, tdg, &AvsFilterMaxDif16::process_subplane);
	slicer.wait ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



bool	AvsFilterMaxDif16::OpMax::operator () (int d1as, int d2as) const
{
	return (d1as < d2as);
}

__m128i	AvsFilterMaxDif16::OpMax::operator () (__m128i d1as, __m128i d2as) const
{
	return (_mm_cmpgt_epi16 (d2as, d1as));
}



bool	AvsFilterMaxDif16::OpMin::operator () (int d1as, int d2as) const
{
	return (d2as < d1as);
}

__m128i	AvsFilterMaxDif16::OpMin::operator () (__m128i d1as, __m128i d2as) const
{
	return (_mm_cmpgt_epi16 (d1as, d2as));
}



void	AvsFilterMaxDif16::process_subplane (Slicer::TaskData &td)
{
	return ((*_process_subplane_ptr) (td));
}



template <class OP>
void	AvsFilterMaxDif16::PlaneProc <OP>::process_subplane_cpp (Slicer::TaskData &td)
{
	assert (&td != 0);

	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);
	uint8_t *      dst_msb_ptr = tdg._dst_msb_ptr + td._y_beg * tdg._stride_dst;
	uint8_t *      dst_lsb_ptr = tdg._dst_lsb_ptr + td._y_beg * tdg._stride_dst;
	const uint8_t* sr1_msb_ptr = tdg._sr1_msb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t* sr1_lsb_ptr = tdg._sr1_lsb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t* sr2_msb_ptr = tdg._sr2_msb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t* sr2_lsb_ptr = tdg._sr2_lsb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t* ref_msb_ptr = tdg._ref_msb_ptr + td._y_beg * tdg._stride_ref;
	const uint8_t* ref_lsb_ptr = tdg._ref_lsb_ptr + td._y_beg * tdg._stride_ref;

	const OP         op;

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		for (int x = 0; x < tdg._w; ++x)
		{
			const int      src1 = (sr1_msb_ptr [x] << 8) | sr1_lsb_ptr [x];
			const int      src2 = (sr2_msb_ptr [x] << 8) | sr2_lsb_ptr [x];
			const int      ref  = (ref_msb_ptr [x] << 8) | ref_lsb_ptr [x];

			int				val = src1;
			if (op (abs (src1 - ref), abs (src2 - ref)))
			{
				val = src2;
			}

			dst_msb_ptr [x] = uint8_t (val >> 8 );
			dst_lsb_ptr [x] = uint8_t (val & 255);
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		sr1_msb_ptr += tdg._stride_sr1;
		sr1_lsb_ptr += tdg._stride_sr1;
		sr2_msb_ptr += tdg._stride_sr2;
		sr2_lsb_ptr += tdg._stride_sr2;
		ref_msb_ptr += tdg._stride_ref;
		ref_lsb_ptr += tdg._stride_ref;
	}
}



template <class OP>
void	AvsFilterMaxDif16::PlaneProc <OP>::process_subplane_sse2 (Slicer::TaskData &td)
{
	assert (&td != 0);

	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);
	uint8_t *      dst_msb_ptr = tdg._dst_msb_ptr + td._y_beg * tdg._stride_dst;
	uint8_t *      dst_lsb_ptr = tdg._dst_lsb_ptr + td._y_beg * tdg._stride_dst;
	const uint8_t* sr1_msb_ptr = tdg._sr1_msb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t* sr1_lsb_ptr = tdg._sr1_lsb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t* sr2_msb_ptr = tdg._sr2_msb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t* sr2_lsb_ptr = tdg._sr2_lsb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t* ref_msb_ptr = tdg._ref_msb_ptr + td._y_beg * tdg._stride_ref;
	const uint8_t* ref_lsb_ptr = tdg._ref_lsb_ptr + td._y_beg * tdg._stride_ref;

	const __m128i  mask_lsb    = _mm_set1_epi16 ( 0x00FF);
	const __m128i  sign        = _mm_set1_epi16 (-0x8000);

	const OP         op;

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		for (int x = 0; x < tdg._w; x += 8)
		{
			const __m128i  src1 =
				fstb::ToolsSse2::load_8_16ml (&sr1_msb_ptr [x], &sr1_lsb_ptr [x]);
			const __m128i  src2 =
				fstb::ToolsSse2::load_8_16ml (&sr2_msb_ptr [x], &sr2_lsb_ptr [x]);
			const __m128i  ref  =
				fstb::ToolsSse2::load_8_16ml (&ref_msb_ptr [x], &ref_lsb_ptr [x]);

			const __m128i  d11  = _mm_subs_epu16 (src1, ref);
			const __m128i  d12  = _mm_subs_epu16 (ref, src1);
			const __m128i  d1a  = _mm_or_si128 (d11, d12);
			const __m128i  d21  = _mm_subs_epu16 (src2, ref);
			const __m128i  d22  = _mm_subs_epu16 (ref, src2);
			const __m128i  d2a  = _mm_or_si128 (d21, d22);
			const __m128i  d1as = _mm_xor_si128 (d1a, sign);
			const __m128i  d2as = _mm_xor_si128 (d2a, sign);
			const __m128i  mask = op (d1as, d2as);
			const __m128i  val  = _mm_or_si128 (
				_mm_and_si128 (   mask, src2),
				_mm_andnot_si128 (mask, src1)
			);

	      fstb::ToolsSse2::store_8_16ml (
				&dst_msb_ptr [x], &dst_lsb_ptr [x], val, mask_lsb
			);
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		sr1_msb_ptr += tdg._stride_sr1;
		sr1_lsb_ptr += tdg._stride_sr1;
		sr2_msb_ptr += tdg._stride_sr2;
		sr2_lsb_ptr += tdg._stride_sr2;
		ref_msb_ptr += tdg._stride_ref;
		ref_lsb_ptr += tdg._stride_ref;
	}
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
