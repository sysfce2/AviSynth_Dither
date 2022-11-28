/*****************************************************************************

        AvsFilterBilateral16.cpp
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

#include "fstb/ToolsSse2.h"
#include "fstb/fnc.h"
#include "AvsFilterBilateral16.h"

#include <algorithm>

#include <cassert>
#include <climits>
#include <cstdlib>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterBilateral16::AvsFilterBilateral16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, ::PClip ref_clip_sptr, int radius, double threshold, double flat, double wmin, double subspl, int y, int u, int v)
:	GenericVideoFilter (src_clip_sptr)
,	_src_clip_sptr (src_clip_sptr)
,	_ref_clip_sptr (ref_clip_sptr)
,	_threshold (threshold)
,	_flat (flat)
,	_wmin (wmin)
,	_radius (radius)
,	_subspl (subspl)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_m (std::max (fstb::round_int (_threshold * 256), 1))
,	_wmax (std::max (fstb::round_int (_threshold * 256 * (1 - _flat)), 1))
,	_sse2_flag (false)
,	_filter_mutex ()
,	_filter_uptr_map ()
{
	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	const int		w = vi.width;
	const int		h = vi.height / 2;

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("Dither_bilateral16: input must be planar.");
	}
	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_bilateral16: stacked MSB/LSB data must have\n"
			"a height multiple of twice the maximum subsampling."
		);
	}
	if (w < 16 || h < 16)
	{
		env_ptr->ThrowError ("Dither_bilateral16: input must be 16x16 min.");
	}
	if (ref_clip_sptr)
	{
		PlaneProcessor::check_same_format (env_ptr, vi, ref_clip_sptr, "Dither_bilateral16", "ref");
	}
	if (radius < FilterBilateral::RADIUS_MIN)
	{
		env_ptr->ThrowError (
			"Dither_bilateral16: \"radius\" must be 2 or above."
		);
	}
	if (w < radius || h < radius)
	{
		env_ptr->ThrowError (
			"Dither_bilateral16: Picture size must be greater than \"radius\"."
		);
	}
	if (threshold < 0)
	{
		env_ptr->ThrowError (
			"Dither_bilateral16: \"thr\" must be positive."
		);
	}
	if (flat < 0 || flat > 1)
	{
		env_ptr->ThrowError (
			"Dither_bilateral16: \"flat\" must be in 0.0 - 1.0 range."
		);
	}
	if (wmin < 0)
	{
		env_ptr->ThrowError (
			"Dither_bilateral16: \"wmin\" must be positive."
		);
	}

	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (2, PlaneProcessor::ClipType_STACKED_16);

	init_fnc (*env_ptr);
}



::PVideoFrame __stdcall	AvsFilterBilateral16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_src_clip_sptr, &_ref_clip_sptr, 0
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterBilateral16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	src_sptr = _src_clip_sptr->GetFrame (n, &env);
	::PVideoFrame	ref_sptr;
	if (_ref_clip_sptr)
	{
		ref_sptr = _ref_clip_sptr->GetFrame (n, &env);
	}

	const uint8_t*	data_src_ptr = src_sptr->GetReadPtr (plane_id);
	uint8_t *		data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const int		w            = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h            = _plane_proc.get_height16 (dst_sptr, plane_id);
	const int		stride_dst   = dst_sptr->GetPitch (plane_id);
	const int		stride_src   = src_sptr->GetPitch (plane_id);

	const uint8_t*	data_ref_ptr = data_src_ptr;
	int				stride_ref   = stride_src;
	if (_ref_clip_sptr)
	{
		data_ref_ptr = ref_sptr->GetReadPtr (plane_id);
		stride_ref   = ref_sptr->GetPitch (plane_id);
	}

	const int		subspl_h = vi.width  / w;
	const int		subspl_v = vi.height / (h * 2);
	const int		radius_h =
		std::max (_radius / subspl_h, int (FilterBilateral::RADIUS_MIN));
	const int		radius_v =
		std::max (_radius / subspl_v, int (FilterBilateral::RADIUS_MIN));

	const int		offset_lsb_dst = h * stride_dst;
	const int		offset_lsb_src = h * stride_src;
	const int		offset_lsb_ref = h * stride_ref;

	FilterBilateral *	filter_ptr = create_or_access_plane_filter (
		plane_id,
		w, h,
		radius_h, radius_v,
		(_ref_clip_sptr != 0)
	);

	try
	{
		filter_ptr->process_plane (
			data_dst_ptr, data_dst_ptr + offset_lsb_dst,
			data_src_ptr, data_src_ptr + offset_lsb_src,
			data_ref_ptr, data_ref_ptr + offset_lsb_ref,
			stride_dst,
			stride_src,
			stride_ref
		);
	}
	catch (std::exception &e)
	{
		env.ThrowError (e.what ());
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterBilateral16::init_fnc (::IScriptEnvironment &env)
{
	const long		cpu_flags = env.GetCPUFlags ();
	if (   (cpu_flags & CPUF_SSE2) != 0
	    && (cpu_flags & CPUF_SSE ) != 0)
	{
		_sse2_flag = true;
	}
}



FilterBilateral *	AvsFilterBilateral16::create_or_access_plane_filter (int /*plane_id*/, int w, int h, int radius_h, int radius_v, bool src_flag)
{
	assert (w > 0);
	assert (h > 0);
	assert (radius_h > 0);
	assert (radius_v > 0);

	const int64_t	key = (int64_t (h) << 32) + w;

	std::lock_guard <std::mutex>  autolock (_filter_mutex);

	std::unique_ptr <FilterBilateral> &	filter_uptr = _filter_uptr_map [key];
	if (filter_uptr.get () == 0)
	{
		filter_uptr = std::unique_ptr <FilterBilateral> (new FilterBilateral (
			w, h, radius_h, radius_v,
			_threshold, _flat, _wmin, _subspl,
			src_flag, _sse2_flag
		));
	}

	return (filter_uptr.get ());
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
