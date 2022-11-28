/*****************************************************************************

        AvsFilterSmoothGrad.cpp
        Author: Laurent de Soras, 2010

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

#include	"AvsFilterSmoothGrad.h"
#include	"DiffSoftClipper.h"
#include	"fstb/ToolsSse2.h"

#if defined (_MSC_VER)
	#include <intrin.h>
	#pragma intrinsic (__emulu)
#endif

#include	<algorithm>

#include	<cassert>
#include	<cmath>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterSmoothGrad::AvsFilterSmoothGrad (::IScriptEnvironment *env_ptr, ::PClip msb_clip_sptr, ::PClip lsb_clip_sptr, int radius, double threshold, bool stacked_flag, ::PClip ref_clip_sptr, double elast, int y, int u, int v)
:	GenericVideoFilter (msb_clip_sptr)
,	_msb_clip_sptr (msb_clip_sptr)
,	_lsb_clip_sptr (lsb_clip_sptr)
,	_ref_clip_sptr (ref_clip_sptr)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_radius (radius)
,	_threshold (threshold)
,	_elast (elast)
,	_stacked_flag (stacked_flag)
,	_thr_1 (0)
,	_thr_2 (0)
,	_thr_slope (0)
,	_thr_offset (0)
,	_process_segment_ptr (0)
,	_add_line_ptr (0)
,	_addsub_line_ptr (0)
{
	assert (env_ptr != 0);

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("SmoothGrad: input must be planar.");
	}

	if (lsb_clip_sptr)
	{
		PlaneProcessor::check_same_format (env_ptr, vi, lsb_clip_sptr, "SmoothGrad", "lsb");

		vi.height *= 2;
	}
	else
	{
		if (_stacked_flag)
		{
			if (! PlaneProcessor::check_stack16_height (vi))
			{
				env_ptr->ThrowError (
					"SmoothGrad: stacked MSB/LSB data must have\n"
					"a height multiple of twice the maximum subsampling."
				);
			}
		}
		else
		{
			if ((vi.num_frames & 1) != 0)
			{
				env_ptr->ThrowError (
					"SmoothGrad: main clip used alone must have\n"
					"an even number of frames (MSB/LSB pairs)."
				);
			}

			vi.num_frames /= 2;
			vi.MulDivFPS (1, 2);
			vi.height *= 2;
		}
	}

	// Here, vi.height has already been doubled.
	if (ref_clip_sptr)
	{
		PlaneProcessor::check_same_format (env_ptr, vi, ref_clip_sptr, "SmoothGrad", "ref");
	}

	if (radius <= 0)
	{
		env_ptr->ThrowError (
			"SmoothGrad: \"radius\" must be strictly positive."
		);
	}
	else if (radius > BoxFilter::MARGIN || radius > BoxFilter::MAX_RADIUS)
	{
		env_ptr->ThrowError (
			"SmoothGrad: \"radius\" is too big."
		);
	}

	if (threshold <= 0)
	{
		env_ptr->ThrowError (
			"SmoothGrad: \"thr\" must be strictly positive."
		);
	}

	if (elast < 1 || elast > 10)
	{
		env_ptr->ThrowError (
			"SmoothGrad: \"elast\" must be in range 1 - 10."
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
	if (lsb_clip_sptr)
	{
		_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_MSB);
	}
	else if (_stacked_flag)
	{
		_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);
	}
	else
	{
		_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_INTERLEAVED_16);
	}
	_plane_proc.set_clip_info (2, PlaneProcessor::ClipType_LSB);
	_plane_proc.set_clip_info (3, PlaneProcessor::ClipType_STACKED_16);

	init_fnc (*env_ptr);
}



AvsFilterSmoothGrad::~AvsFilterSmoothGrad ()
{
	// Nothing
}



::PVideoFrame __stdcall	AvsFilterSmoothGrad::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_msb_clip_sptr, &_lsb_clip_sptr, &_ref_clip_sptr
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterSmoothGrad::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	msb_sptr;
	::PVideoFrame	lsb_sptr;
	if (_lsb_clip_sptr)
	{
		msb_sptr = _msb_clip_sptr->GetFrame (n, &env);
		lsb_sptr = _lsb_clip_sptr->GetFrame (n, &env);
	}
	else if (_stacked_flag)
	{
		msb_sptr = _msb_clip_sptr->GetFrame (n, &env);
	}
	else
	{
		msb_sptr = _msb_clip_sptr->GetFrame (n * 2    , &env);
		lsb_sptr = _msb_clip_sptr->GetFrame (n * 2 + 1, &env);
	}

	::PVideoFrame	ref_sptr;
	if (_ref_clip_sptr)
	{
		ref_sptr = _ref_clip_sptr->GetFrame (n, &env);
	}

	uint8_t *		data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const uint8_t*	data_msb_ptr = msb_sptr->GetReadPtr (plane_id);
	const int		w            = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h            = _plane_proc.get_height16 (dst_sptr, plane_id);
	const int		stride_dst   = dst_sptr->GetPitch (plane_id);
	const int		stride_msb   = msb_sptr->GetPitch (plane_id);

	const uint8_t*	data_lsb_ptr = 0;
	int				stride_lsb   = 0;
	if (! _lsb_clip_sptr && _stacked_flag)
	{
		data_lsb_ptr = data_msb_ptr + h * stride_msb;
		stride_lsb   = stride_msb;
	}
	else
	{
		data_lsb_ptr = lsb_sptr->GetReadPtr (plane_id);
		stride_lsb   = lsb_sptr->GetPitch (plane_id);
	}

	const uint8_t*	ref_msb_ptr = data_msb_ptr;
	const uint8_t*	ref_lsb_ptr = data_lsb_ptr;
	int				stride_ref_msb = stride_msb;
	int				stride_ref_lsb = stride_lsb;
	if (_ref_clip_sptr)
	{
		const int		stride_ref = ref_sptr->GetPitch (plane_id);
		ref_msb_ptr = ref_sptr->GetReadPtr (plane_id);
		ref_lsb_ptr = ref_msb_ptr + h * stride_ref;
		stride_ref_msb = stride_ref;
		stride_ref_lsb = stride_ref;
	}

	int				radius_h = _radius;
	int				radius_v = _radius;

	const int		div_h = vi.width / w;
	const int		div_v = vi.height / (h * 2);

	radius_h = std::max (radius_h / div_h, 2);
	radius_v = std::max (radius_v / div_v, 2);

	process_plane (
		data_dst_ptr, data_dst_ptr + h * stride_dst,
		data_msb_ptr, data_lsb_ptr,
		ref_msb_ptr, ref_lsb_ptr,
		w, h,
		stride_dst,
		stride_msb, stride_lsb,
		stride_ref_msb, stride_ref_lsb,
		radius_h, radius_v
	);
}




/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterSmoothGrad::init_fnc (::IScriptEnvironment &env)
{
	assert (&env != 0);

	_process_segment_ptr = &AvsFilterSmoothGrad::process_segment_cpp;
	_add_line_ptr        = &BoxFilter::add_line_cpp;
	_addsub_line_ptr     = &BoxFilter::addsub_line_cpp;

	const long		cpu_flags = env.GetCPUFlags ();
	if ((cpu_flags & CPUF_SSE2) != 0)
	{
		_process_segment_ptr = &AvsFilterSmoothGrad::process_segment_sse2;
		_add_line_ptr        = &BoxFilter::add_line_sse2;
		_addsub_line_ptr     = &BoxFilter::addsub_line_sse2;
	}
}



void	AvsFilterSmoothGrad::process_plane (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w, int h, int stride_dst, int stride_src_msb, int stride_src_lsb, int stride_ref_msb, int stride_ref_lsb, int radius_h, int radius_v) const
{
	for (int col_x = 0; col_x < w; col_x += BoxFilter::MAX_SEG_LEN)
	{
		const int		col_w = std::min (int (BoxFilter::MAX_SEG_LEN), w - col_x);

		process_column (
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			ref_msb_ptr, ref_lsb_ptr,
			w, h,
			stride_dst,
			stride_src_msb, stride_src_lsb,
			stride_ref_msb, stride_ref_lsb,
			col_x, col_w,
			radius_h, radius_v
		);
	}
}



void	AvsFilterSmoothGrad::process_column (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w, int h, int stride_dst, int stride_src_msb, int stride_src_lsb, int stride_ref_msb, int stride_ref_lsb, int col_x, int col_w, int radius_h, int radius_v) const
{
	assert (col_w <= BoxFilter::MAX_SEG_LEN);
	assert (h >= radius_v);
	assert (radius_h <= BoxFilter::MARGIN);
	assert (col_x == 0 || col_x >= radius_h);

	BoxFilter		filter (
		src_msb_ptr, src_lsb_ptr, w, h,
		stride_dst, stride_src_msb, stride_src_lsb,
		col_x, col_w, radius_h, radius_v,
		_add_line_ptr, _addsub_line_ptr
	);

	dst_msb_ptr += col_x;
	dst_lsb_ptr += col_x;
	src_msb_ptr += col_x;
	src_lsb_ptr += col_x;
	ref_msb_ptr += col_x;
	ref_lsb_ptr += col_x;

	for (int y = 0; y < h; ++y)
	{
		filter.filter_line ();

		(this->*_process_segment_ptr) (
			filter,
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			ref_msb_ptr, ref_lsb_ptr
		);

		dst_msb_ptr += stride_dst;
		dst_lsb_ptr += stride_dst;
		src_msb_ptr += stride_src_msb;
		src_lsb_ptr += stride_src_lsb;
		ref_msb_ptr += stride_ref_msb;
		ref_lsb_ptr += stride_ref_lsb;
	}
}



void	AvsFilterSmoothGrad::process_segment_cpp (BoxFilter &filter, uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr) const
{
	filter.init_sum_h ();

	const int		col_w = filter.get_col_w ();
	for (int x = 0; x < col_w; ++x)
	{
		const int		val_flt = filter.filter_col ();
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



void	AvsFilterSmoothGrad::process_segment_sse2 (BoxFilter &filter, uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr) const
{
	filter.init_sum_h ();

	const __m128i	thr_1      = _mm_set1_epi16 (int16_t (_thr_1));
	const __m128i	thr_2      = _mm_set1_epi16 (int16_t (_thr_2));
	const __m128i	thr_slope  = _mm_set1_epi16 (int16_t (_thr_slope));
	const __m128i	thr_offset = _mm_set1_epi16 (int16_t (_thr_offset));
	const __m128i	mask_lsb   = _mm_set1_epi16 (0x00FF);

	const int		col_w = filter.get_col_w ();
	for (int x = 0; x < col_w; x += 8)
	{
		union {
			__m128i			_v128;
			uint16_t			_v16 [8];
		}					in;
		in._v16 [0] = uint16_t (filter.filter_col ());
		in._v16 [1] = uint16_t (filter.filter_col ());
		in._v16 [2] = uint16_t (filter.filter_col ());
		in._v16 [3] = uint16_t (filter.filter_col ());
		in._v16 [4] = uint16_t (filter.filter_col ());
		in._v16 [5] = uint16_t (filter.filter_col ());
		in._v16 [6] = uint16_t (filter.filter_col ());
		in._v16 [7] = uint16_t (filter.filter_col ());
		const __m128i	val_flt = in._v128;

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



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
