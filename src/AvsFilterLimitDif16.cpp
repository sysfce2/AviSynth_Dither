/*****************************************************************************

        AvsFilterLimitDif16.cpp
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

#include	"fstb/CpuId.h"
#include	"fstb/ToolsSse2.h"
#include	"AvsFilterLimitDif16.h"
#include	"DiffSoftClipper.h"

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterLimitDif16::AvsFilterLimitDif16 (::IScriptEnvironment *env_ptr, ::PClip flt_clip_sptr, ::PClip src_clip_sptr, ::PClip ref_clip_sptr, double threshold, double elast, int y, int u, int v, bool refabsdif_flag)
:	GenericVideoFilter (flt_clip_sptr)
,	_flt_clip_sptr (flt_clip_sptr)
,	_src_clip_sptr (src_clip_sptr)
,	_ref_clip_sptr (ref_clip_sptr)
,	_threshold (threshold)
,	_elast (elast)
,	_refabsdif_flag (refabsdif_flag)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_thr_1 (0)
,	_thr_2 (0)
,	_thr_slope (0)
,	_thr_offset (0)
,	_process_segment_ptr (0)
{
	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("Dither_limitdif16: input must be planar.");
	}
	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_limitdif16: stacked MSB/LSB data must have\n"
			"a height multiple of twice the maximum subsampling."
		);
	}
	PlaneProcessor::check_same_format (env_ptr, vi, src_clip_sptr, "Dither_limitdif16", "src");
	if (ref_clip_sptr)
	{
		PlaneProcessor::check_same_format (env_ptr, vi, ref_clip_sptr, "Dither_limitdif16", "ref");
	}
	if (threshold <= 0)
	{
		env_ptr->ThrowError (
			"Dither_limitdif16: \"thr\" must be strictly positive."
		);
	}
	if (elast < 1 || elast > 10)
	{
		env_ptr->ThrowError (
			"Dither_limitdif16: \"elast\" must be in range 1 - 10."
		);
	}

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Misc. initialisations

	DiffSoftClipper::init_cst (
		_thr_1, _thr_2, _thr_slope, _thr_offset,
		_threshold, _elast
	);

	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (2, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (3, PlaneProcessor::ClipType_STACKED_16);

	init_fnc ();
}



::PVideoFrame __stdcall	AvsFilterLimitDif16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);
	::PVideoFrame	flt_sptr = _flt_clip_sptr->GetFrame (n, env_ptr);
	::PVideoFrame	src_sptr = _src_clip_sptr->GetFrame (n, env_ptr);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_flt_clip_sptr, &_src_clip_sptr, &_ref_clip_sptr
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterLimitDif16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	flt_sptr = _flt_clip_sptr->GetFrame (n, &env);
	::PVideoFrame	src_sptr = _src_clip_sptr->GetFrame (n, &env);

	::PVideoFrame	ref_sptr;
	if (_ref_clip_sptr)
	{
		ref_sptr = _ref_clip_sptr->GetFrame (n, &env);
	}

	const uint8_t*	data_flt_ptr = flt_sptr->GetReadPtr (plane_id);
	const uint8_t*	data_src_ptr = src_sptr->GetReadPtr (plane_id);
	uint8_t *		data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const int		w            = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h            = _plane_proc.get_height16 (dst_sptr, plane_id);
	const int		stride_dst   = dst_sptr->GetPitch (plane_id);
	const int		stride_src   = src_sptr->GetPitch (plane_id);
	const int		stride_flt   = flt_sptr->GetPitch (plane_id);

	const uint8_t*	data_ref_ptr = data_src_ptr;
	int				stride_ref   = stride_src;
	if (_ref_clip_sptr)
	{
		data_ref_ptr = ref_sptr->GetReadPtr (plane_id);
		stride_ref   = ref_sptr->GetPitch (plane_id);
	}

	const int		offset_lsb_dst = h * stride_dst;
	const int		offset_lsb_flt = h * stride_flt;
	const int		offset_lsb_src = h * stride_src;
	const int		offset_lsb_ref = h * stride_ref;

	TaskDataGlobal		tdg;
	tdg._this_ptr    = this;
	tdg._w           = w;
	tdg._dst_msb_ptr = data_dst_ptr;
	tdg._dst_lsb_ptr = data_dst_ptr + offset_lsb_dst;
	tdg._flt_msb_ptr = data_flt_ptr;
	tdg._flt_lsb_ptr = data_flt_ptr + offset_lsb_flt;
	tdg._src_msb_ptr = data_src_ptr;
	tdg._src_lsb_ptr = data_src_ptr + offset_lsb_src;
	tdg._ref_msb_ptr = data_ref_ptr;
	tdg._ref_lsb_ptr = data_ref_ptr + offset_lsb_ref;
	tdg._stride_dst  = stride_dst;
	tdg._stride_flt  = stride_flt;
	tdg._stride_src  = stride_src;
	tdg._stride_ref  = stride_ref;

	Slicer			slicer;
	slicer.start (h, tdg, &AvsFilterLimitDif16::process_subplane);
	slicer.wait ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterLimitDif16::init_fnc ()
{
	_process_segment_ptr = &AvsFilterLimitDif16::process_segment_cpp;

	fstb::CpuId    cid;
	if (cid._sse2_flag)
	{
		_process_segment_ptr = &AvsFilterLimitDif16::process_segment_sse2;
	}
	if (cid._avx2_flag)
	{
		_process_segment_ptr = &AvsFilterLimitDif16::process_segment_avx2;
	}
}



void	AvsFilterLimitDif16::process_subplane (Slicer::TaskData &td)
{
	assert (&td != 0);

	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);
	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + tdg._stride_dst * td._y_beg;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + tdg._stride_dst * td._y_beg;
	const uint8_t*	flt_msb_ptr = tdg._flt_msb_ptr + tdg._stride_flt * td._y_beg;
	const uint8_t*	flt_lsb_ptr = tdg._flt_lsb_ptr + tdg._stride_flt * td._y_beg;
	const uint8_t*	src_msb_ptr = tdg._src_msb_ptr + tdg._stride_src * td._y_beg;
	const uint8_t*	src_lsb_ptr = tdg._src_lsb_ptr + tdg._stride_src * td._y_beg;
	const uint8_t*	ref_msb_ptr = tdg._ref_msb_ptr + tdg._stride_ref * td._y_beg;
	const uint8_t*	ref_lsb_ptr = tdg._ref_lsb_ptr + tdg._stride_ref * td._y_beg;

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		(this->*_process_segment_ptr) (
			dst_msb_ptr, dst_lsb_ptr,
			flt_msb_ptr, flt_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			ref_msb_ptr, ref_lsb_ptr,
			tdg._w
		);

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		flt_msb_ptr += tdg._stride_flt;
		flt_lsb_ptr += tdg._stride_flt;
		src_msb_ptr += tdg._stride_src;
		src_lsb_ptr += tdg._stride_src;
		ref_msb_ptr += tdg._stride_ref;
		ref_lsb_ptr += tdg._stride_ref;
	}
}



void	AvsFilterLimitDif16::process_segment_cpp (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *flt_msb_ptr, const uint8_t *flt_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w) const
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (flt_msb_ptr != 0);
	assert (flt_lsb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (ref_msb_ptr != 0);
	assert (ref_lsb_ptr != 0);
	assert (w > 0);

	if (_refabsdif_flag)
	{
		for (int x = 0; x < w; ++x)
		{
			const int		val_flt = (flt_msb_ptr [x] << 8) + flt_lsb_ptr [x];
			const int		val_src = (src_msb_ptr [x] << 8) + src_lsb_ptr [x];
			const int		dif_ref = (ref_msb_ptr [x] << 8) + ref_lsb_ptr [x];

			const int		val_dst = DiffSoftClipper::clip_dif_cpp (
				val_flt, val_src, dif_ref,
				_thr_1, _thr_2, _thr_slope, _thr_offset
			);

			dst_msb_ptr [x] = uint8_t (val_dst >> 8);
			dst_lsb_ptr [x] = uint8_t (val_dst     );
		}
	}
	else
	{
		for (int x = 0; x < w; ++x)
		{
			const int		val_flt = (flt_msb_ptr [x] << 8) + flt_lsb_ptr [x];
			const int		val_src = (src_msb_ptr [x] << 8) + src_lsb_ptr [x];
			const int		val_ref = (ref_msb_ptr [x] << 8) + ref_lsb_ptr [x];

			const int		val_dst = DiffSoftClipper::clip_cpp (
				val_flt, val_src, val_ref,
				_thr_1, _thr_2, _thr_slope, _thr_offset
			);

			dst_msb_ptr [x] = uint8_t (val_dst >> 8);
			dst_lsb_ptr [x] = uint8_t (val_dst     );
		}
	}
}



void	AvsFilterLimitDif16::process_segment_sse2 (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *flt_msb_ptr, const uint8_t *flt_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w) const
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (flt_msb_ptr != 0);
	assert (flt_lsb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (ref_msb_ptr != 0);
	assert (ref_lsb_ptr != 0);
	assert (w > 0);

	const __m128i	thr_1      = _mm_set1_epi16 (int16_t (_thr_1));
	const __m128i	thr_2      = _mm_set1_epi16 (int16_t (_thr_2));
	const __m128i	thr_slope  = _mm_set1_epi16 (int16_t (_thr_slope));
	const __m128i	thr_offset = _mm_set1_epi16 (int16_t (_thr_offset));
	const __m128i	mask_lsb   = _mm_set1_epi16 (0x00FF);

	if (_refabsdif_flag)
	{
		for (int x = 0; x < w; x += 8)
		{
			const __m128i	val_flt =
				fstb::ToolsSse2::load_8_16ml (flt_msb_ptr + x, flt_lsb_ptr + x);
			const __m128i	val_src =
				fstb::ToolsSse2::load_8_16ml (src_msb_ptr + x, src_lsb_ptr + x);
			const __m128i	dif_ref =
				fstb::ToolsSse2::load_8_16ml (ref_msb_ptr + x, ref_lsb_ptr + x);

			const __m128i	val_dst = DiffSoftClipper::clip_dif_sse2 (
				val_flt, val_src, dif_ref,
				thr_1, thr_2, thr_slope, thr_offset
			);

			fstb::ToolsSse2::store_8_16ml (
				dst_msb_ptr + x, dst_lsb_ptr + x,
				val_dst, mask_lsb
			);
		}
	}
	else
	{
		for (int x = 0; x < w; x += 8)
		{
			const __m128i	val_flt =
				fstb::ToolsSse2::load_8_16ml (flt_msb_ptr + x, flt_lsb_ptr + x);
			const __m128i	val_src =
				fstb::ToolsSse2::load_8_16ml (src_msb_ptr + x, src_lsb_ptr + x);
			const __m128i	val_ref =
				fstb::ToolsSse2::load_8_16ml (ref_msb_ptr + x, ref_lsb_ptr + x);

			const __m128i	val_dst = DiffSoftClipper::clip_sse2 (
				val_flt, val_src, val_ref,
				thr_1, thr_2, thr_slope, thr_offset
			);

			fstb::ToolsSse2::store_8_16ml (
				dst_msb_ptr + x, dst_lsb_ptr + x,
				val_dst, mask_lsb
			);
		}
	}
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
