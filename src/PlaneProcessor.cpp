/*****************************************************************************

        PlaneProcessor.cpp
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

#define	NOMINMAX
#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>

#include "fstb/fnc.h"
#include	"avisynth.h"
#include	"PlaneProcCbInterface.h"
#include	"PlaneProcessor.h"

#include	<algorithm>

#include	<cassert>
#include	<cstdio>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



PlaneProcessor::PlaneProcessor (const ::VideoInfo &vi, const ::VideoInfo &vi_src, PlaneProcCbInterface &cb)
:	_vi (vi)
,	_vi_src (vi_src)
,	_nbr_planes (get_nbr_planes (vi))
,	_cb (cb)
/*,	_proc_mode_arr ();
,	_clip_type_arr ()*/
{
	assert (&vi != 0);
	assert (&vi_src != 0);
	assert (&cb != 0);

	for (int p_index = 0; p_index < _nbr_planes; ++p_index)
	{
		_proc_mode_arr [p_index] = PlaneProcMode_PROCESS;
	}

	for (int c_index = 0; c_index < MAX_NBR_CLIPS; ++c_index)
	{
		_clip_type_arr [c_index] = ClipType_UNKNOWN;
	}
}



void	PlaneProcessor::set_proc_mode (int y, int u, int v)
{
	_proc_mode_arr [0] = y;
	_proc_mode_arr [1] = u;
	_proc_mode_arr [2] = v;
}



// 0 = destination clip
void	PlaneProcessor::set_clip_info (int index, ClipType type)
{
	assert (index >= 0);
	assert (index < MAX_NBR_CLIPS);
	assert (type >= 0);
	assert (type < ClipType_NBR_ELT);

	_clip_type_arr [index] = type;
}



int	PlaneProcessor::get_nbr_planes () const
{
	return (_nbr_planes);
}



void	PlaneProcessor::process_frame (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, ::PClip *src_1_ptr, ::PClip *src_2_ptr, ::PClip *src_3_ptr, void *ctx_ptr)
{
	assert (dst_sptr != 0);
	assert (dst_sptr->IsWritable ());
	assert (n >= 0);

	const ClipType	type_dst = _clip_type_arr [0];

	for (int plane_index = 0; plane_index < _nbr_planes; ++plane_index)
	{
		const int		plane_id = _plane_id_list [plane_index];
		const int		mode     = _proc_mode_arr [plane_index];

		if (mode == PlaneProcMode_PROCESS)
		{
			_cb.process_plane (dst_sptr, n, env, plane_index, plane_id, ctx_ptr);
		}
		else if (mode >= PlaneProcMode_COPY1 && mode <= PlaneProcMode_COPY3)
		{
			static const int	burp [PlaneProcMode_NBR_ELT] =
			{
				-1, -1, 1, -1, 2, 3
			};
			const int			src_index = burp [mode];
			::PClip *			pouet [4] = { 0, src_1_ptr, src_2_ptr, src_3_ptr };
			::PClip *			src_ptr   = pouet [src_index];
			const ClipType		type_src  = _clip_type_arr [src_index];

			if (src_ptr != 0)
			{
				copy (dst_sptr, n, plane_id, type_dst, *src_ptr, type_src, env);
			}
		}
		else if (mode <= PlaneProcMode_FILL)
		{
			fill (dst_sptr, n, plane_id, type_dst, -mode);
		}
	}
}



int	PlaneProcessor::get_width (const ::PVideoFrame &frame_sptr, int plane_id, bool src_flag) const
{
	assert (&frame_sptr != 0);
	assert (frame_sptr != 0);

	int				width = 0;
	if (plane_id == PLANAR_U || plane_id == PLANAR_V)
	{
		width = frame_sptr->GetRowSize (PLANAR_Y);
		const int		subspl = (src_flag)
			? _vi_src.GetPlaneWidthSubsampling (plane_id)
			: _vi.GetPlaneWidthSubsampling (plane_id);
		width >>= subspl;
	}
	else
	{
		width = frame_sptr->GetRowSize (plane_id);
	}

	return (width);
}



int	PlaneProcessor::get_height (const ::PVideoFrame &frame_sptr, int plane_id, bool src_flag) const
{
	assert (&frame_sptr != 0);
	assert (frame_sptr != 0);

	int				height = 0;
	if (plane_id == PLANAR_U || plane_id == PLANAR_V)
	{
		height = frame_sptr->GetHeight (PLANAR_Y);
		const int		subspl = (src_flag)
			? _vi_src.GetPlaneHeightSubsampling (plane_id)
			: _vi.GetPlaneHeightSubsampling (plane_id);
		height >>= subspl;
	}
	else
	{
		height = frame_sptr->GetHeight (plane_id);
	}

	return (height);
}



int	PlaneProcessor::get_height16 (const ::PVideoFrame &frame_sptr, int plane_id, bool src_flag) const
{
	return (get_height (frame_sptr, plane_id, src_flag) >> 1);
}



int	PlaneProcessor::get_nbr_planes (const ::VideoInfo &vi)
{
	assert (&vi != 0);

	return ((vi.IsY8 () || ! vi.IsPlanar ()) ? 1 : 3);
}



int	PlaneProcessor::get_min_w (const ::VideoInfo &vi)
{
	assert (&vi != 0);

	int				l = vi.width;
	const int		nbr_planes = get_nbr_planes (vi);
	for (int index = 1; index < nbr_planes; ++index)
	{
		const int      lc = compute_plane_w (vi, index, vi.width);
		l = std::min (l, lc);
	}

	return (l);
}



int	PlaneProcessor::get_min_h (const ::VideoInfo &vi, bool stack16_flag)
{
	assert (&vi != 0);

	int				l = vi.height;
	const int		nbr_planes = get_nbr_planes (vi);
	for (int index = 1; index < nbr_planes; ++index)
	{
		const int      lc = compute_plane_h (vi, index, vi.height);
		l = std::min (l, lc);
	}
	if (stack16_flag)
	{
		l >>= 1;
	}

	return (l);
}



int	PlaneProcessor::compute_plane_w (const ::VideoInfo &vi, int plane_index, int w)
{
	assert (&vi != 0);
	assert (plane_index >= 0);
	assert (plane_index < MAX_NBR_PLANES);
	assert (w > 0);

	int				l = w;
	if (plane_index > 0)
	{
		const int		plane_id = _plane_id_list [plane_index];
		const int		subspl   = vi.GetPlaneWidthSubsampling (plane_id);
		l >>= subspl;
	}

	return (l);
}



int	PlaneProcessor::compute_plane_h (const ::VideoInfo &vi, int plane_index, int h)
{
	assert (&vi != 0);
	assert (plane_index >= 0);
	assert (plane_index < MAX_NBR_PLANES);
	assert (h > 0);

	int				l = h;
	if (plane_index > 0)
	{
		const int		plane_id = _plane_id_list [plane_index];
		const int		subspl   = vi.GetPlaneHeightSubsampling (plane_id);
		l >>= subspl;
	}

	return (l);
}



bool	PlaneProcessor::check_stack16_width (const ::VideoInfo &vi, int width)
{
	assert (&vi != 0);

	if (width < 0)
	{
		width = vi.width;
	}

	bool				ok_flag = true;

	const int		nbr_planes = get_nbr_planes (vi);
	for (int index = 1; index < nbr_planes && ok_flag; ++index)
	{
		const int		plane_id = _plane_id_list [index];
		const int		subspl   = vi.GetPlaneWidthSubsampling (plane_id);
		const int		mask     = (1 << subspl) - 1;
		if ((width & mask) != 0)
		{
			ok_flag = false;
		}
	}

	return (ok_flag);
}



bool	PlaneProcessor::check_stack16_height (const ::VideoInfo &vi, int height)
{
	assert (&vi != 0);

	if (height < 0)
	{
		height = vi.height;
	}

	bool				ok_flag = ((height & 1) == 0);

	const int		nbr_planes = get_nbr_planes (vi);
	for (int index = 1; index < nbr_planes && ok_flag; ++index)
	{
		const int		plane_id = _plane_id_list [index];
		const int		subspl   = vi.GetPlaneHeightSubsampling (plane_id);
		const int		mask     = (2 << subspl) - 1;
		if ((height & mask) != 0)
		{
			ok_flag = false;
		}
	}

	return (ok_flag);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	PlaneProcessor::fill (::PVideoFrame &dst_sptr, int n, int plane_id, ClipType type, int val)
{
	assert (dst_sptr != 0);
	assert (n >= 0);
	assert (type != ClipType_UNKNOWN);

	if (type == ClipType_STACKED_16)
	{
		fill_frame_part (dst_sptr, n, plane_id, uint8_t (val >> 8 ), true, 0);
		fill_frame_part (dst_sptr, n, plane_id, uint8_t (val & 255), true, 1);
	}
	else
	{
		if (type == ClipType_INTERLEAVED_16)
		{
			val >>= (1 - (n & 1)) * 8;
		}
		fill_frame_part (dst_sptr, n, plane_id, uint8_t (val & 255), false, 0);
	}
}



void	PlaneProcessor::fill_frame_part (::PVideoFrame &dst_sptr, int /*n*/, int plane_id, uint8_t val, bool stacked_flag, int part)
{
	const int      stride   = dst_sptr->GetPitch (plane_id);
	const int      width    = get_width (dst_sptr, plane_id, false);
	int            height   = get_height (dst_sptr, plane_id, false);
	uint8_t *      data_ptr = dst_sptr->GetWritePtr (plane_id);

	if (stacked_flag && part != 0)
	{
		height >>= 1;
		data_ptr += stride * height;
	}

	if (height > 0)
	{
		memset (data_ptr, val, stride * (height - 1) + width);
	}
}



void	PlaneProcessor::copy (::PVideoFrame &dst_sptr, int n, int plane_id, ClipType type_dst, ::PClip &src_clip, ClipType type_src, ::IScriptEnvironment &env)
{
	assert (dst_sptr != 0);
	assert (n >= 0);
	assert (type_dst >= 0);
	assert (type_dst != ClipType_UNKNOWN);
	assert (type_dst < ClipType_NBR_ELT);
	assert (&src_clip != 0);
	assert (src_clip != 0);
	assert (type_src >= 0);
	assert (type_src != ClipType_UNKNOWN);
	assert (type_src < ClipType_NBR_ELT);

	if (src_clip != 0)
	{
		if (have_same_height (type_dst, type_src))
		{
			if (   type_dst == ClipType_INTERLEAVED_16
			    && type_src != ClipType_INTERLEAVED_16)
			{
				n >>= 1;
			}
			else if (   type_dst != ClipType_INTERLEAVED_16
			         && type_src == ClipType_INTERLEAVED_16)
			{
				n <<= 1;
				if (type_dst == ClipType_LSB)
				{
					++ n;
				}
			}

			copy_n_to_n (dst_sptr, src_clip, n, plane_id, env);
		}

		else if (is_stacked (type_src) && ! is_stacked (type_dst))
		{
			bool				lsb_flag = (type_dst == ClipType_LSB);

			if (type_dst == ClipType_INTERLEAVED_16)
			{
				lsb_flag = ((n & 1) != 0);
				n >>= 1;
			}

			copy_16_to_8 (dst_sptr, src_clip, n, plane_id, env, (lsb_flag) ? 1 : 0);
		}

		else	// ! is_stacked (type_src) && is_stacked (type_dst)
		{
			if (type_src == ClipType_INTERLEAVED_16)
			{
				n <<= 1;
				copy_8_to_16 (dst_sptr, src_clip, n,     plane_id, env, 0);
				copy_8_to_16 (dst_sptr, src_clip, n + 1, plane_id, env, 1);
			}

			else
			{
				copy_8_to_16 (dst_sptr, src_clip, n,     plane_id, env, 0);
				fill_frame_part (dst_sptr, n, plane_id, 0, true, 1);
			}
		}
	}
}



void	PlaneProcessor::copy_n_to_n (::PVideoFrame &dst_sptr, ::PClip &src_clip, int n, int plane_id, ::IScriptEnvironment &env)
{
	const int      dst_stride   = dst_sptr->GetPitch (plane_id);
	const int      dst_width    = get_width (dst_sptr, plane_id);
	const int      dst_height   = get_height (dst_sptr, plane_id);
	uint8_t *      dst_data_ptr = dst_sptr->GetWritePtr (plane_id);

	n = std::min (n, src_clip->GetVideoInfo ().num_frames - 1);

	::PVideoFrame  src_sptr     = src_clip->GetFrame (n, &env);
	const int      src_stride   = src_sptr->GetPitch (plane_id);
	const int      src_width    = get_width (src_sptr, plane_id, true);
	const int      src_height   = get_height (src_sptr, plane_id, true);
	const uint8_t* src_data_ptr = src_sptr->GetReadPtr (plane_id);

	const int      width  = std::min (src_width,  dst_width);
	const int      height = std::min (src_height, dst_height);

	env.BitBlt (
		dst_data_ptr, dst_stride,
		src_data_ptr, src_stride, width, height
	);
}



void	PlaneProcessor::copy_8_to_16 (::PVideoFrame &dst_sptr, ::PClip &src_clip, int n, int plane_id, ::IScriptEnvironment &env, int part)
{
	const int      dst_stride   = dst_sptr->GetPitch (plane_id);
	const int      dst_width    = get_width (dst_sptr, plane_id);
	const int      dst_height   = get_height16 (dst_sptr, plane_id);
	uint8_t *      dst_data_ptr = dst_sptr->GetWritePtr (plane_id);

	n = std::min (n, src_clip->GetVideoInfo ().num_frames - 1);

	::PVideoFrame  src_sptr     = src_clip->GetFrame (n, &env);
	const int      src_stride   = src_sptr->GetPitch (plane_id);
	const int      src_width    = get_width (src_sptr, plane_id, true);
	const int      src_height   = get_height (src_sptr, plane_id, true);
	const uint8_t* src_data_ptr = src_sptr->GetReadPtr (plane_id);

	const int      width        = std::min (src_width,  dst_width);
	const int      height       = std::min (src_height, dst_height);

	env.BitBlt (
		dst_data_ptr + part * dst_height * dst_stride, dst_stride,
		src_data_ptr, src_stride, width, height
	);
}



void	PlaneProcessor::copy_16_to_8 (::PVideoFrame &dst_sptr, ::PClip &src_clip, int n, int plane_id, ::IScriptEnvironment &env, int part)
{
	const int      dst_stride   = dst_sptr->GetPitch (plane_id);
	const int      dst_width    = get_width (dst_sptr, plane_id);
	const int      dst_height   = get_height (dst_sptr, plane_id);
	uint8_t *      dst_data_ptr = dst_sptr->GetWritePtr (plane_id);

	n = std::min (n, src_clip->GetVideoInfo ().num_frames - 1);

	::PVideoFrame  src_sptr     = src_clip->GetFrame (n, &env);
	const int      src_stride   = src_sptr->GetPitch (plane_id);
	const int      src_width    = get_width (src_sptr, plane_id, true);
	const int      src_height   = get_height16 (src_sptr, plane_id, true);
	const uint8_t* src_data_ptr = src_sptr->GetReadPtr (plane_id);

	const int      width        = std::min (src_width,  dst_width);
	const int      height       = std::min (src_height, dst_height);

	env.BitBlt (
		dst_data_ptr, dst_stride,
		src_data_ptr + part * src_height * src_stride, src_stride,
		width, height
	);
}



void	PlaneProcessor::check_same_format (::IScriptEnvironment *env_ptr, const ::VideoInfo &vi, const ::PClip tst_sptr, const char *fnc_name_0, const char *arg_name_0, int flags)
{
	assert (env_ptr != 0);
	assert (&vi != 0);
	assert (tst_sptr != 0);
	assert (fnc_name_0 != 0);
	assert (arg_name_0 != 0);

	const ::VideoInfo &  vi_tst = tst_sptr->GetVideoInfo ();

	static char    txt_0 [4095+1];

	if (   vi_tst.height != vi.height
	    && (flags & FmtChkFlag_H) != 0)
	{
		fstb::snprintf4all (
			txt_0, sizeof (txt_0),
			"%s: clip \"%s\" has a height of %d instead of %d.",
			fnc_name_0, arg_name_0, vi_tst.height, vi.height
		);
		env_ptr->ThrowError (txt_0);
	}

	if (   (flags & FmtChkFlag_W) != 0
	    && vi_tst.width != vi.width)
	{
		fstb::snprintf4all (
			txt_0, sizeof (txt_0),
			"%s: clip \"%s\" has a width of %d instead of %d.",
			fnc_name_0, arg_name_0, vi_tst.width, vi.width
		);
		env_ptr->ThrowError (txt_0);
	}

	if (   (flags & FmtChkFlag_NBR_FRAMES) != 0
	    && vi_tst.num_frames != vi.num_frames)
	{
		fstb::snprintf4all (
			txt_0, sizeof (txt_0),
			"%s: clip \"%s\" has %d frames instead of %d.",
			fnc_name_0, arg_name_0, vi_tst.num_frames, vi.num_frames
		);
		env_ptr->ThrowError (txt_0);
	}

	// Tests absolute colorspace equality
	if ((flags & FmtChkFlag_CS_ALL) == FmtChkFlag_CS_ALL)
	{
		if (! vi_tst.IsSameColorspace (vi))
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has not the same colorspace (code %X instead of %X).",
				fnc_name_0, arg_name_0, vi_tst.pixel_type, vi.pixel_type
			);
			env_ptr->ThrowError (txt_0);
		}
	}

	// Tests only specific colorspace features
	else
	{
		if (   (flags & FmtChkFlag_CS_TYPE) != 0
		    && (   vi_tst.IsRGB () != vi.IsRGB ()
		        || (vi.IsRGB () && vi_tst.IsRGB24 () != vi.IsRGB24 ())))
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong pixel type.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
		if (   (flags & FmtChkFlag_CS_LAYOUT) != 0
		    && vi_tst.IsPlanar () != vi.IsPlanar ())
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong planarity.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
		// Hahaha! The following test is unreadable. Basically, it makes
		// Y8 compatible with any other YUV colorspace (planar or not).
		if (   (flags & FmtChkFlag_CS_SUBSPL) != 0
		    && (   vi_tst.IsYUV () != vi.IsYUV ()
		        || (   (vi.IsYUV () && ! vi.IsY8 () && ! vi_tst.IsY8 ())
		            && (   vi_tst.GetPlaneWidthSubsampling  (PLANAR_U) != vi.GetPlaneWidthSubsampling (PLANAR_U)
		                || vi_tst.GetPlaneHeightSubsampling (PLANAR_U) != vi.GetPlaneHeightSubsampling (PLANAR_U)
		               ) 
	              )
		       )
		   )
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong chroma subsampling.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
		if (   (flags & FmtChkFlag_CS_FORMAT) != 0
		    &&    (vi_tst.pixel_type & ::VideoInfo::CS_Sample_Bits_Mask)
		       != (    vi.pixel_type & ::VideoInfo::CS_Sample_Bits_Mask))
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong data format.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
		if (   (flags & FmtChkFlag_CS_NBRCOMP) != 0
		    && (  (vi_tst.pixel_type == ::VideoInfo::CS_Y8)
		        ^ (    vi.pixel_type == ::VideoInfo::CS_Y8)) != 0)
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong number of components.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
	}
}



bool	PlaneProcessor::have_same_height (ClipType t1, ClipType t2)
{
	assert (t1 >= 0);
	assert (t1 != ClipType_UNKNOWN);
	assert (t1 < ClipType_NBR_ELT);
	assert (t2 >= 0);
	assert (t2 != ClipType_UNKNOWN);
	assert (t2 < ClipType_NBR_ELT);

	return (! (is_stacked (t1) ^ is_stacked (t2)));
}



bool	PlaneProcessor::is_stacked (ClipType type)
{
	assert (type >= 0);
	assert (type != ClipType_UNKNOWN);
	assert (type < ClipType_NBR_ELT);

	return (type == ClipType_STACKED_16);
}



const int	PlaneProcessor::_plane_id_list [MAX_NBR_PLANES] =
{
   PLANAR_Y,
	PLANAR_U,
	PLANAR_V
};



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
