/*****************************************************************************

        AvsFilterBoxFilter16.cpp
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

#include	"AvsFilterBoxFilter16.h"

#include	<algorithm>

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterBoxFilter16::AvsFilterBoxFilter16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, int radius, int y, int u, int v)
:	GenericVideoFilter (src_clip_sptr)
,	_src_clip_sptr (src_clip_sptr)
,	_radius (radius)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_add_line_ptr (0)
,	_addsub_line_ptr (0)
{
	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("Dither_boxfilter16: input must be planar.");
	}

	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_boxfilter16: stacked MSB/LSB data must have\n"
			"a height multiple of twice the maximum subsampling."
		);
	}

	if (radius <= 0)
	{
		env_ptr->ThrowError (
			"Dither_boxfilter16: \"radius\" must be strictly positive."
		);
	}
	else if (radius > BoxFilter::MARGIN || radius > BoxFilter::MAX_RADIUS)
	{
		env_ptr->ThrowError (
			"Dither_boxfilter16: \"radius\" is too big."
		);
	}

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Misc. initialisations

	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);

	init_fnc (*env_ptr);
}



::PVideoFrame __stdcall	AvsFilterBoxFilter16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (dst_sptr, n, *env_ptr, &_src_clip_sptr, 0, 0);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterBoxFilter16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	src_sptr = _src_clip_sptr->GetFrame (n, &env);

	const uint8_t*	data_src_ptr = src_sptr->GetReadPtr (plane_id);
	uint8_t *		data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const int		w            = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h            = _plane_proc.get_height16 (dst_sptr, plane_id);
	const int		stride_dst   = dst_sptr->GetPitch (plane_id);
	const int		stride_src   = src_sptr->GetPitch (plane_id);

	const int		subspl_h = vi.width  / w;
	const int		subspl_v = vi.height / (h * 2);
	const int		radius_h = std::max (_radius / subspl_h, 2);
	const int		radius_v = std::max (_radius / subspl_v, 2);

	const int		offset_lsb_dst = h * stride_dst;
	const int		offset_lsb_src = h * stride_src;

	process_plane (
		data_dst_ptr, data_dst_ptr + offset_lsb_dst,
		data_src_ptr, data_src_ptr + offset_lsb_src,
		w, h,
		stride_dst,
		stride_src,
		radius_h, radius_v
	);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterBoxFilter16::init_fnc (::IScriptEnvironment &env)
{
	_add_line_ptr        = &BoxFilter::add_line_cpp;
	_addsub_line_ptr     = &BoxFilter::addsub_line_cpp;

	const long		cpu_flags = env.GetCPUFlags ();
	if ((cpu_flags & CPUF_SSE2) != 0)
	{
		_add_line_ptr        = &BoxFilter::add_line_sse2;
		_addsub_line_ptr     = &BoxFilter::addsub_line_sse2;
	}
}



void	AvsFilterBoxFilter16::process_plane (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int h, int stride_dst, int stride_src, int radius_h, int radius_v) const
{
	for (int col_x = 0; col_x < w; col_x += BoxFilter::MAX_SEG_LEN)
	{
		const int		col_w = std::min (int (BoxFilter::MAX_SEG_LEN), w - col_x);

		process_column (
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			w, h,
			stride_dst,
			stride_src,
			col_x, col_w,
			radius_h, radius_v
		);
	}
}



void	AvsFilterBoxFilter16::process_column (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int h, int stride_dst, int stride_src, int col_x, int col_w, int radius_h, int radius_v) const
{
	assert (col_w <= BoxFilter::MAX_SEG_LEN);
	assert (h >= radius_v);
	assert (radius_h <= BoxFilter::MARGIN);
	assert (col_x == 0 || col_x >= radius_h);

	BoxFilter		filter (
		src_msb_ptr, src_lsb_ptr, w, h,
		stride_dst, stride_src, stride_src,
		col_x, col_w, radius_h, radius_v,
		_add_line_ptr, _addsub_line_ptr
	);

	dst_msb_ptr += col_x;
	dst_lsb_ptr += col_x;
	src_msb_ptr += col_x;
	src_lsb_ptr += col_x;

	for (int y = 0; y < h; ++y)
	{
		filter.filter_line ();

		process_segment (
			filter,
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr
		);

		dst_msb_ptr += stride_dst;
		dst_lsb_ptr += stride_dst;
		src_msb_ptr += stride_src;
		src_lsb_ptr += stride_src;
	}
}



void	AvsFilterBoxFilter16::process_segment (BoxFilter &filter, uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t * /*src_msb_ptr*/, const uint8_t * /*src_lsb_ptr*/) const
{
	filter.init_sum_h ();

	const int		col_w = filter.get_col_w ();
	for (int x = 0; x < col_w; ++x)
	{
		const int		val_flt = filter.filter_col ();

		dst_msb_ptr [x] = uint8_t (val_flt >> 8);
		dst_lsb_ptr [x] = uint8_t (val_flt     );
	}
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
