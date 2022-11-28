/*****************************************************************************

        AvsFilterOut.cpp
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

#include	"fstb/ToolsSse2.h"
#include	"AvsFilterOut.h"

#include	<emmintrin.h>

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterOut::AvsFilterOut (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, bool big_endian_flag)
:	GenericVideoFilter (src_clip_sptr)
,	_vi_src (vi)
,	_src_clip_sptr (src_clip_sptr)
,	_big_endian_flag (big_endian_flag)
,	_sse2_flag (check_sse2 (*env_ptr))
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
{
	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("Dither_out: input must be planar.");
	}
	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_out: stacked MSB/LSB data must have\n"
			"a height multiple of twice the maximum subsampling."
		);
	}

	_plane_proc.set_proc_mode (
		PlaneProcMode_PROCESS,
		PlaneProcMode_PROCESS,
		PlaneProcMode_PROCESS
	);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_NORMAL_8);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);

	vi.width  *= 2;
	vi.height /= 2;
}



::PVideoFrame __stdcall	AvsFilterOut::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (dst_sptr, n, *env_ptr, &_src_clip_sptr, 0, 0);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterOut::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	src_sptr = _src_clip_sptr->GetFrame (n, &env);

	const uint8_t* data_src_ptr = src_sptr->GetReadPtr (plane_id);
	uint8_t *      data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const int      stride_dst   = dst_sptr->GetPitch (plane_id);
	const int      stride_src   = src_sptr->GetPitch (plane_id);
	const int      w            = _plane_proc.get_width (src_sptr, plane_id);
	const int      h            = _plane_proc.get_height16 (src_sptr, plane_id);

	int            offset_lsb   = h * stride_src;
	const uint8_t* src_msb_ptr  = data_src_ptr;
	const uint8_t* src_lsb_ptr  = data_src_ptr;
	if (_big_endian_flag)
	{
		src_msb_ptr += offset_lsb;
	}
	else
	{
		src_lsb_ptr += offset_lsb;
	}

	for (int y = 0; y < h; ++y)
	{
		if (_sse2_flag)
		{
			for (int x = 0; x < w; x += 8)
			{
			   const __m128i  val =
					fstb::ToolsSse2::load_8_16ml (src_msb_ptr + x, src_lsb_ptr + x);
				_mm_store_si128 (
					reinterpret_cast <__m128i *> (data_dst_ptr + x * 2),
					val
				);
			}
		}

		else
		{
			for (int x = 0; x < w; ++x)
			{
				const int      msb = src_msb_ptr [x];
				const int      lsb = src_lsb_ptr [x];
				const uint16_t val = uint16_t ((msb << 8) + lsb);
				reinterpret_cast <uint16_t *> (data_dst_ptr) [x] = val;
			}
		}

		src_msb_ptr  += stride_src;
		src_lsb_ptr  += stride_src;
		data_dst_ptr += stride_dst;
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



bool	AvsFilterOut::check_sse2 (::IScriptEnvironment &env)
{
	bool           sse2_flag = false;

	const long     cpu_flags = env.GetCPUFlags ();
	if (   (cpu_flags & CPUF_SSE2) != 0
	    && (cpu_flags & CPUF_SSE ) != 0)
	{
		sse2_flag = true;
	}

	return (sse2_flag);
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
