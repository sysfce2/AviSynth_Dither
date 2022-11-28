/*****************************************************************************

        AvsFilterResize16.cpp
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

#include "fmtcl/FilterResize.h"
#include "fstb/CpuId.h"
#include "fstb/fnc.h"
#include "AvsFilterResize16.h"

#include <algorithm>

#include <cassert>
#include <cctype>
#include <cstring>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterResize16::AvsFilterResize16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, int dst_width, int dst_height, double win_x, double win_y, double win_w, double win_h, std::string kernel_fnc, double kernel_scale_h, double kernel_scale_v, int taps, bool a1_flag, double a1, bool a2_flag, double a2, bool a3_flag, double a3, int kovrspl, bool norm_flag, bool preserve_center_flag, int y, int u, int v, std::string kernel_fnc_h, std::string kernel_fnc_v, double kernel_total_h, double kernel_total_v, bool invks_flag, OptBool invks_h, OptBool invks_v, int invks_taps, std::string cplace_s, std::string cplace_d, std::string csp)
:	GenericVideoFilter (src_clip_sptr)
,	_vi_src (vi)
,	_src_clip_sptr (src_clip_sptr)
,	_src_width (vi.width)
,	_src_height (vi.height / 2)
,	_dst_width (dst_width)
,	_dst_height (dst_height)
,	_invks_taps (invks_taps)
,	_norm_val_h (kernel_total_h)
,	_norm_val_v (kernel_total_v)
,	_norm_flag (norm_flag)
,	_invks_h_flag (cumulate_flag (invks_flag, invks_h))
,	_invks_v_flag (cumulate_flag (invks_flag, invks_v))
,	_cplace_s (conv_str_to_chroma_placement (*env_ptr, cplace_s))
,	_cplace_d (conv_str_to_chroma_placement (*env_ptr, cplace_d))
,	_plane_proc_uptr ()
,	_sse2_flag (false)
,	_avx2_flag (false)
,	_filter_mutex ()
,	_filter_uptr_map ()
{
	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("Dither_resize16: input must be planar.");
	}
	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_resize16: stacked MSB/LSB data must have\n"
			"a height multiple of twice the maximum subsampling."
		);
	}

	if (! csp.empty ())
	{
		vi.pixel_type = conv_str_to_pixel_type (*env_ptr, csp);
		PlaneProcessor::check_same_format (
			env_ptr,
			vi,
			_src_clip_sptr,
			"Dither_resize16",
			"input",
			PlaneProcessor::FmtChkFlag_CS_NBRCOMP
		);
	}

	_plane_proc_uptr = std::unique_ptr <PlaneProcessor> (
		new PlaneProcessor (vi, _vi_src, *this)
	);

	if (dst_width <= 0)
	{
		env_ptr->ThrowError ("Dither_resize16: target_width must be > 0.");
	}
	if (dst_height <= 0)
	{
		env_ptr->ThrowError ("Dither_resize16: target_height must be > 0.");
	}
	if (kernel_scale_h == 0)
	{
		env_ptr->ThrowError ("Dither_resize16: fh must be not null.");
	}
	if (kernel_scale_v == 0)
	{
		env_ptr->ThrowError ("Dither_resize16: fv must be not null.");
	}
	if (kernel_total_h < 0)
	{
		env_ptr->ThrowError ("Dither_resize16: totalh must be positive or null.");
	}
	if (kernel_total_v < 0)
	{
		env_ptr->ThrowError ("Dither_resize16: totalv must be positive or null.");
	}
	if (taps < 1 || taps > 128)
	{
		env_ptr->ThrowError ("Dither_resize16: taps must be in the 1-128 range.");
	}
	if (_invks_taps < 1 || _invks_taps > 128)
	{
		env_ptr->ThrowError ("Dither_resize16: invkstaps must be in the 1-128 range.");
	}

	if (win_w == 0)
	{
		win_w = _src_width;
	}
	else if (win_w < 0)
	{
		win_w = _src_width + win_w - win_x;
		if (win_w <= 0)
		{
			env_ptr->ThrowError ("Dither_resize16: source width must be > 0.");
		}
	}

	if (win_h == 0)
	{
		win_h = _src_height;
	}
	if (win_h <= 0)
	{
		win_h = _src_height + win_h - win_y;
		if (win_h <= 0)
		{
			env_ptr->ThrowError ("Dither_resize16: source height must be > 0.");
		}
	}

	_plane_proc_uptr->set_proc_mode (y, u, v);
	_plane_proc_uptr->set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc_uptr->set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);

	try
	{
		// Kernel init begin
		if (kernel_fnc_h.empty ())
		{
			kernel_fnc_h = kernel_fnc;
		}
		if (kernel_fnc_v.empty ())
		{
			kernel_fnc_v = kernel_fnc;
		}

		const int      nbr_planes = PlaneProcessor::get_nbr_planes (vi);

		for (int plane_index = 0; plane_index < nbr_planes; ++plane_index)
		{
			PlaneData &                plane_data = _plane_data_arr [plane_index];
			PlaneData::KernelArray &   kernel_arr = plane_data._kernel_arr;

			plane_data._win._x = win_x;
			plane_data._win._y = win_y;
			plane_data._win._w = win_w;
			plane_data._win._h = win_h;

			std::vector <double>	coef_arr;

			coef_arr.clear ();
			kernel_arr [fmtcl::FilterResize::Dir_H].create_kernel (
				kernel_fnc_h, coef_arr, taps,
				a1_flag, a1,
				a2_flag, a2,
				a3_flag, a3,
				kovrspl,
				_invks_h_flag,
				_invks_taps
			);

			coef_arr.clear ();
			kernel_arr [fmtcl::FilterResize::Dir_V].create_kernel (
				kernel_fnc_v, coef_arr, taps,
				a1_flag, a1,
				a2_flag, a2,
				a3_flag, a3,
				kovrspl,
				_invks_v_flag,
				_invks_taps
			);

			plane_data._kernel_scale_h       = kernel_scale_h;
			plane_data._kernel_scale_v       = kernel_scale_v;
			plane_data._gain                 = 1;
			plane_data._add_cst              = 0;
			plane_data._preserve_center_flag = preserve_center_flag;
		}
		// Kernel init end
	}
	catch (std::runtime_error &e)
	{
		static char		err_msg [4095+1];
		fstb::snprintf4all (
			err_msg, sizeof (err_msg),
			"Dither_resize16: %s", e.what ()
		);
		env_ptr->ThrowError (err_msg);
	}
	catch (...)
	{
		env_ptr->ThrowError (
			"Dither_resize16: exception during kernel initialization."
		);
	}

	vi.width  = _dst_width;
	vi.height = _dst_height * 2;

	if (! PlaneProcessor::check_stack16_width (vi))
	{
		env_ptr->ThrowError (
			"Dither_resize16: \"width\" must be\n"
			"multiple of the maximum subsampling."
		);
	}
	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_resize16: \"height\" must be\n"
			"multiple of the maximum subsampling."
		);
	}

	create_plane_specs ();

	init_fnc (*env_ptr);
}



::PVideoFrame __stdcall	AvsFilterResize16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc_uptr->process_frame (dst_sptr, n, *env_ptr, &_src_clip_sptr, 0, 0);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterResize16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	src_sptr = _src_clip_sptr->GetFrame (n, &env);

	const uint8_t*	data_src_ptr = src_sptr->GetReadPtr (plane_id);
	uint8_t *		data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const int		dst_h        = _plane_proc_uptr->get_height16 (dst_sptr, plane_id);
	const int		src_h        = _plane_proc_uptr->get_height16 (src_sptr, plane_id, true);
	const int		stride_dst   = dst_sptr->GetPitch (plane_id);
	const int		stride_src   = src_sptr->GetPitch (plane_id);

	const int		offset_lsb_dst = dst_h * stride_dst;
	const int		offset_lsb_src = src_h * stride_src;

	const bool     interlaced_flag = (vi.IsFieldBased () && vi.IsParityKnown ());
	const bool     top_flag        = vi.IsTFF ();   // or _src_clip_sptr->GetParity(n)?
	const InterlacingType   itl_s = get_itl_type (interlaced_flag, top_flag);
	const InterlacingType   itl_d = itl_s;

	try
	{
		fmtcl::FilterResize *   filter_ptr =
			create_or_access_plane_filter (plane_index, itl_d, itl_s);

		const bool     chroma_flag = (plane_index > 0);

		filter_ptr->process_plane (
			data_dst_ptr, data_dst_ptr + offset_lsb_dst,
			data_src_ptr, data_src_ptr + offset_lsb_src,
			stride_dst,
			stride_src,
			chroma_flag
		);
	}

	catch (std::exception &e)
	{
		env.ThrowError (e.what ());
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterResize16::init_fnc (::IScriptEnvironment &env)
{
	const long		cpu_flags = env.GetCPUFlags ();
	if (   (cpu_flags & CPUF_SSE2) != 0
	    && (cpu_flags & CPUF_SSE ) != 0)
	{
		_sse2_flag = true;

		fstb::CpuId    cid;
		_avx2_flag = cid._avx2_flag;
	}
}



fmtcl::FilterResize *	AvsFilterResize16::create_or_access_plane_filter (int plane_index, InterlacingType itl_d, InterlacingType itl_s)
{
	assert (plane_index >= 0);
	assert (itl_d >= 0);
	assert (itl_d < InterlacingType_NBR_ELT);
	assert (itl_s >= 0);
	assert (itl_s < InterlacingType_NBR_ELT);

	const PlaneData & plane_data         = _plane_data_arr [plane_index];
	const fmtcl::ResampleSpecPlane & key = plane_data._spec_arr [itl_d] [itl_s];

	std::lock_guard <std::mutex>  autolock (_filter_mutex);

	std::unique_ptr <fmtcl::FilterResize> &   filter_uptr = _filter_uptr_map [key];
	if (filter_uptr.get () == 0)
	{
		filter_uptr = std::unique_ptr <fmtcl::FilterResize> (new fmtcl::FilterResize (
			key,
			*(plane_data._kernel_arr [fmtcl::FilterResize::Dir_H]._k_uptr),
			*(plane_data._kernel_arr [fmtcl::FilterResize::Dir_V]._k_uptr),
			_norm_flag, _norm_val_h, _norm_val_v,
			plane_data._gain,
			fmtcl::SplFmt_STACK16, 16,
			fmtcl::SplFmt_STACK16, 16,
			false, _sse2_flag, _avx2_flag
		));
	}

	return (filter_uptr.get ());
}



// Requires the kernel data to be initialized beforehand.
void	AvsFilterResize16::create_plane_specs ()
{
	fmtcl::ResampleSpecPlane   spec;

	const int      src_w = _vi_src.width;
	const int      src_h = _vi_src.height / 2;
	const int      dst_w = vi.width;
	const int      dst_h = vi.height / 2;
	const int      nbr_planes = PlaneProcessor::get_nbr_planes (vi);

	for (int plane_index = 0; plane_index < nbr_planes; ++plane_index)
	{
		PlaneData &    plane_data = _plane_data_arr [plane_index];

		spec._src_width  =
			_plane_proc_uptr->compute_plane_w (_vi_src, plane_index, src_w);
		spec._src_height =
			_plane_proc_uptr->compute_plane_h (_vi_src, plane_index, src_h);
		spec._dst_width  =
			_plane_proc_uptr->compute_plane_w ( vi    , plane_index, dst_w);
		spec._dst_height =
			_plane_proc_uptr->compute_plane_h ( vi    , plane_index, dst_h);

		const int      subspl_h = src_w / spec._src_width;
		const int      subspl_v = src_h / spec._src_height;

		const int      ss_s_h = fstb::get_prev_pow_2 (subspl_h);
		const int      ss_s_v = fstb::get_prev_pow_2 (subspl_v);
		const int      ss_d_h = fstb::get_prev_pow_2 (dst_w / spec._dst_width);
		const int      ss_d_v = fstb::get_prev_pow_2 (dst_h / spec._dst_height);

		const Win &    s = plane_data._win;
		spec._win_x = s._x / subspl_h;
		spec._win_y = s._y / subspl_v;
		spec._win_w = s._w / subspl_h;
		spec._win_h = s._h / subspl_v;

		spec._add_cst        = plane_data._add_cst;
		spec._kernel_scale_h = plane_data._kernel_scale_h;
		spec._kernel_scale_v = plane_data._kernel_scale_v;
		spec._kernel_hash_h  = plane_data._kernel_arr [fmtcl::FilterResize::Dir_H].get_hash ();
		spec._kernel_hash_v  = plane_data._kernel_arr [fmtcl::FilterResize::Dir_V].get_hash ();

		for (int itl_d = 0; itl_d < InterlacingType_NBR_ELT; ++itl_d)
		{
			for (int itl_s = 0; itl_s < InterlacingType_NBR_ELT; ++itl_s)
			{
				double         cp_s_h = 0;
				double         cp_s_v = 0;
				double         cp_d_h = 0;
				double         cp_d_v = 0;
				if (plane_data._preserve_center_flag)
				{
					ChromaPlacement_compute_cplace (
						cp_s_h, cp_s_v, _cplace_s, plane_index,
						ss_s_h, ss_s_v, false,
						(itl_s != InterlacingType_FRAME),
						(itl_s == InterlacingType_TOP)
					);
					ChromaPlacement_compute_cplace (
						cp_d_h, cp_d_v, _cplace_d, plane_index,
						ss_d_h, ss_d_v, false,
						(itl_d != InterlacingType_FRAME),
						(itl_d == InterlacingType_TOP)
					);
				}

				spec._center_pos_src_h = cp_s_h;
				spec._center_pos_src_v = cp_s_v;
				spec._center_pos_dst_h = cp_d_h;
				spec._center_pos_dst_v = cp_d_v;

				plane_data._spec_arr [itl_d] [itl_s] = spec;
			}  // for itl_s
		}  // for itl_d
	}  // for plane_index
}



fmtcl::ChromaPlacement	AvsFilterResize16::conv_str_to_chroma_placement (::IScriptEnvironment &env, std::string cplace)
{
	assert (&env != 0);
	assert (&cplace != 0);

	fmtcl::ChromaPlacement  cp_val = fmtcl::ChromaPlacement_MPEG1;

	fstb::conv_to_lower_case (cplace);
	if (strcmp (cplace.c_str (), "mpeg1") == 0)
	{
		cp_val = fmtcl::ChromaPlacement_MPEG1;
	}
	else if (strcmp (cplace.c_str (), "mpeg2") == 0)
	{
		cp_val = fmtcl::ChromaPlacement_MPEG2;
	}
	else if (strcmp (cplace.c_str (), "dv") == 0)
	{
		cp_val = fmtcl::ChromaPlacement_DV;
	}
	else
	{
		env.ThrowError ("Dither_resize16: unexpected cplace string.");
	}

	return (cp_val);
}



int	AvsFilterResize16::conv_str_to_pixel_type (::IScriptEnvironment &env, std::string csp)
{
	int            pixel_type = ::VideoInfo::CS_YV12;

	fstb::conv_to_lower_case (csp);
	const char *   csp_0 = csp.c_str ();
	if (strcmp (csp_0, "yv12") == 0)
	{
		pixel_type = ::VideoInfo::CS_YV12;
	}
	else if (strcmp (csp_0, "yv16") == 0)
	{
		pixel_type = ::VideoInfo::CS_YV16;
	}
	else if (strcmp (csp_0, "yv24") == 0)
	{
		pixel_type = ::VideoInfo::CS_YV24;
	}
	else if (strcmp (csp_0, "yv411") == 0)
	{
		pixel_type = ::VideoInfo::CS_YV411;
	}
	else if (strcmp (csp_0, "y8") == 0)
	{
		pixel_type = ::VideoInfo::CS_Y8;
	}
	else
	{
		env.ThrowError ("Dither_resize16: unexpected csp string.");
	}

	return (pixel_type);
}



bool	AvsFilterResize16::cumulate_flag (bool flag, OptBool flag2)
{
	if (flag2 == OptBool_TRUE)
	{
		flag = true;
	}
	else if (flag2 == OptBool_FALSE)
	{
		flag = false;
	}
	else
	{
		assert (flag2 == OptBool_UNDEF);
	}

	return (flag);
}



AvsFilterResize16::InterlacingType	AvsFilterResize16::get_itl_type (bool itl_flag, bool top_flag)
{
	return (
		(itl_flag) ? ((top_flag) ? InterlacingType_TOP
		                         : InterlacingType_BOT)
		           :               InterlacingType_FRAME
	);
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
