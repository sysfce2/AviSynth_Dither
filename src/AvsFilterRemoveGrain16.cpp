/*****************************************************************************

        AvsFilterRemoveGrain16.cpp
        Author: Laurent de Soras, 2012

Modes 5-10, 13-18 and 22-24 ported from the tp7's RgTools.

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

#include	"fstb/def.h"
#include	"fstb/fnc.h"
#include	"fstb/ToolsSse2.h"
#include	"AvsFilterRemoveGrain16.h"

#include	<algorithm>

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterRemoveGrain16::AvsFilterRemoveGrain16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, int mode, int modeu, int modev)
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
/*
,	_mode_arr ()
,	_process_subplane_ptr_arr ()
*/
{
	assert (env_ptr != 0);

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("Dither_RemoveGrain16: input must be planar.");
	}

	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_RemoveGrain16: stacked MSB/LSB data must have\n"
			"a height multiple of twice the maximum subsampling."
		);
	}

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

	const PlaneProcMode	y = conv_rgmode_to_proc (mode);
	const PlaneProcMode	u = conv_rgmode_to_proc (modeu);
	const PlaneProcMode	v = conv_rgmode_to_proc (modev);
	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);

	_mode_arr [0] = mode;
	_mode_arr [1] = modeu;
	_mode_arr [2] = modev;

	const long		cpu_flags = env_ptr->GetCPUFlags ();
	const bool		sse2_flag = ((cpu_flags & CPUF_SSE2) != 0);

	for (int plane = 0; plane < MAX_NBR_PLANES; ++ plane)
	{
		_process_subplane_ptr_arr [plane] = 0;

		switch (_mode_arr [plane])
		{
		case  0: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG00>::process_subplane_cpp; break;
		case  1: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG01>::process_subplane_cpp; break;
		case  2: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG02>::process_subplane_cpp; break;
		case  3: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG03>::process_subplane_cpp; break;
		case  4: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG04>::process_subplane_cpp; break;
		case  5: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG05>::process_subplane_cpp; break;
		case  6: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG06>::process_subplane_cpp; break;
		case  7: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG07>::process_subplane_cpp; break;
		case  8: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG08>::process_subplane_cpp; break;
		case  9: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG09>::process_subplane_cpp; break;
		case 10: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG10>::process_subplane_cpp; break;
		case 11: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG11>::process_subplane_cpp; break;
		case 12: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG12>::process_subplane_cpp; break;
		case 13: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG13>::process_subplane_cpp; break;
		case 14: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG14>::process_subplane_cpp; break;
		case 15: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG15>::process_subplane_cpp; break;
		case 16: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG16>::process_subplane_cpp; break;
		case 17: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG17>::process_subplane_cpp; break;
		case 18: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG18>::process_subplane_cpp; break;
		case 19: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG19>::process_subplane_cpp; break;
		case 20: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG20>::process_subplane_cpp; break;
		case 21: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG21>::process_subplane_cpp; break;
		case 22: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG22>::process_subplane_cpp; break;
		case 23: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG23>::process_subplane_cpp; break;
		case 24: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG24>::process_subplane_cpp; break;
		default: break;
		}

		if (sse2_flag)
		{
			switch (_mode_arr [plane])
			{
			case  0: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG00>::process_subplane_sse2; break;
			case  1: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG01>::process_subplane_sse2; break;
			case  2: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG02>::process_subplane_sse2; break;
			case  3: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG03>::process_subplane_sse2; break;
			case  4: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG04>::process_subplane_sse2; break;
			case  5: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG05>::process_subplane_sse2; break;
			case  6: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG06>::process_subplane_sse2; break;
			case  7: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG07>::process_subplane_sse2; break;
			case  8: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG08>::process_subplane_sse2; break;
			case  9: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG09>::process_subplane_sse2; break;
			case 10: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG10>::process_subplane_sse2; break;
			case 11: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG11>::process_subplane_sse2; break;
			case 12: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG12>::process_subplane_sse2; break;
			case 17: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG17>::process_subplane_sse2; break;
			case 18: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG18>::process_subplane_sse2; break;
			case 19: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG19>::process_subplane_sse2; break;
			case 20: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG20>::process_subplane_sse2; break;
			case 21: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG21>::process_subplane_sse2; break;
			case 22: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG22>::process_subplane_sse2; break;
			default: break;
			}
		}

		if (_process_subplane_ptr_arr [plane] == 0)
		{
			if (_mode_arr [plane] > 0)
			{
				env_ptr->ThrowError (
					"Dither_RemoveGrain16: unsupported mode."
				);
			}
			else
			{
				_process_subplane_ptr_arr [plane] = &PlaneProc <OpRG00>::process_subplane_cpp;
			}
		}
	}
}



::PVideoFrame __stdcall	AvsFilterRemoveGrain16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_src_clip_sptr, 0, 0
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterRemoveGrain16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	src1_sptr = _src_clip_sptr->GetFrame (n, &env);

	const int		w             = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h             = _plane_proc.get_height16 (dst_sptr, plane_id);
	uint8_t *		data_msbd_ptr = dst_sptr->GetWritePtr (plane_id);
	const int		stride_dst    = dst_sptr->GetPitch (plane_id);

	const uint8_t*	data_msb1_ptr = src1_sptr->GetReadPtr (plane_id);
	const int		stride_src    = src1_sptr->GetPitch (plane_id);

	TaskDataGlobal		tdg;
	tdg._this_ptr    = this;
	tdg._w           = w;
	tdg._h           = h;
	tdg._plane_index = plane_index;
	tdg._dst_msb_ptr = data_msbd_ptr;
	tdg._dst_lsb_ptr = data_msbd_ptr + h * stride_dst;
	tdg._src_msb_ptr = data_msb1_ptr;
	tdg._src_lsb_ptr = data_msb1_ptr + h * stride_src;
	tdg._stride_dst  = stride_dst;
	tdg._stride_src  = stride_src;

	// Main content
	Slicer			slicer;
	slicer.start (h, tdg, &AvsFilterRemoveGrain16::process_subplane);

	// First line
	memcpy (tdg._dst_msb_ptr, tdg._src_msb_ptr, w);
	memcpy (tdg._dst_lsb_ptr, tdg._src_lsb_ptr, w);

	// Last line
	const int		lp_dst = (h - 1) * stride_dst;
	const int		lp_src = (h - 1) * stride_src;
	memcpy (tdg._dst_msb_ptr + lp_dst, tdg._src_msb_ptr + lp_src, w);
	memcpy (tdg._dst_lsb_ptr + lp_dst, tdg._src_lsb_ptr + lp_src, w);

	// Synchronisation point
	slicer.wait ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#define AvsFilterRemoveGrain16_READ_PIX    \
   const int      om = stride_src - 1;     \
   const int      o0 = stride_src    ;     \
   const int      op = stride_src + 1;     \
   __m128i        a1 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr - op, src_lsb_ptr - op), mask_sign); \
   __m128i        a2 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr - o0, src_lsb_ptr - o0), mask_sign); \
   __m128i        a3 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr - om, src_lsb_ptr - om), mask_sign); \
   __m128i        a4 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr - 1 , src_lsb_ptr - 1 ), mask_sign); \
   __m128i        c  = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr     , src_lsb_ptr     ), mask_sign); \
   __m128i        a5 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr + 1 , src_lsb_ptr + 1 ), mask_sign); \
   __m128i        a6 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr + om, src_lsb_ptr + om), mask_sign); \
   __m128i        a7 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr + o0, src_lsb_ptr + o0), mask_sign); \
   __m128i        a8 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (src_msb_ptr + op, src_lsb_ptr + op), mask_sign);

#define AvsFilterRemoveGrain16_SORT_AXIS_CPP \
	const int      ma1 = std::max (a1, a8);   \
	const int      mi1 = std::min (a1, a8);   \
	const int      ma2 = std::max (a2, a7);   \
	const int      mi2 = std::min (a2, a7);   \
	const int      ma3 = std::max (a3, a6);   \
	const int      mi3 = std::min (a3, a6);   \
	const int      ma4 = std::max (a4, a5);   \
	const int      mi4 = std::min (a4, a5);

#define AvsFilterRemoveGrain16_SORT_AXIS_SSE2   \
	const __m128i  ma1 = _mm_max_epi16 (a1, a8); \
	const __m128i  mi1 = _mm_min_epi16 (a1, a8); \
	const __m128i  ma2 = _mm_max_epi16 (a2, a7); \
	const __m128i  mi2 = _mm_min_epi16 (a2, a7); \
	const __m128i  ma3 = _mm_max_epi16 (a3, a6); \
	const __m128i  mi3 = _mm_min_epi16 (a3, a6); \
	const __m128i  ma4 = _mm_max_epi16 (a4, a5); \
	const __m128i  mi4 = _mm_min_epi16 (a4, a5);



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 0 (bypass)

int	AvsFilterRemoveGrain16::OpRG00::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	fstb::unused (a1, a2, a3, a4, a5, a6, a7, a8);

	return (c);
}

__m128i	AvsFilterRemoveGrain16::OpRG00::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	fstb::unused (stride_src, mask_sign);

	return (fstb::ToolsSse2::load_8_16ml (src_msb_ptr, src_lsb_ptr));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 1

int	AvsFilterRemoveGrain16::OpRG01::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	const int		mi = std::min (
		std::min (std::min (a1, a2), std::min (a3, a4)),
		std::min (std::min (a5, a6), std::min (a7, a8))
	);
	const int		ma = std::max (
		std::max (std::max (a1, a2), std::max (a3, a4)),
		std::max (std::max (a5, a6), std::max (a7, a8))
	);

	return (fstb::limit (c, mi, ma));
}

__m128i	AvsFilterRemoveGrain16::OpRG01::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

	const __m128i	mi = _mm_min_epi16 (
		_mm_min_epi16 (_mm_min_epi16 (a1, a2), _mm_min_epi16 (a3, a4)),
		_mm_min_epi16 (_mm_min_epi16 (a5, a6), _mm_min_epi16 (a7, a8))
	);
	const __m128i	ma = _mm_max_epi16 (
		_mm_max_epi16 (_mm_max_epi16 (a1, a2), _mm_max_epi16 (a3, a4)),
		_mm_max_epi16 (_mm_max_epi16 (a5, a6), _mm_max_epi16 (a7, a8))
	);

	return (fstb::ToolsSse2::limit_epi16 (c, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 2

int	AvsFilterRemoveGrain16::OpRG02::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	int				a [8] = { a1, a2, a3, a4, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [7]) + 1);

	return (fstb::limit (c, a [2-1], a [7-1]));
}

__m128i	AvsFilterRemoveGrain16::OpRG02::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

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
	sort_pair (a2, a6);
	sort_pair (a3, a7);
	a4 = _mm_min_epi16 (a4, a8);	// sort_pair (a4, a8);

	a3 = _mm_min_epi16 (a3, a5);	// sort_pair (a3, a5);
	a6 = _mm_max_epi16 (a4, a6);	// sort_pair (a4, a6);

	a2 = _mm_min_epi16 (a2, a3);	// sort_pair (a2, a3);
	a7 = _mm_max_epi16 (a6, a7);	// sort_pair (a6, a7);

	return (fstb::ToolsSse2::limit_epi16 (c, a2, a7));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 3

int	AvsFilterRemoveGrain16::OpRG03::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	int				a [8] = { a1, a2, a3, a4, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [7]) + 1);

	return (fstb::limit (c, a [3-1], a [6-1]));
}

__m128i	AvsFilterRemoveGrain16::OpRG03::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

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
	sort_pair (a2, a6);
	sort_pair (a3, a7);
	a4 = _mm_min_epi16 (a4, a8);	// sort_pair (a4, a8);

	a3 = _mm_min_epi16 (a3, a5);	// sort_pair (a3, a5);
	a6 = _mm_max_epi16 (a4, a6);	// sort_pair (a4, a6);

	a3 = _mm_max_epi16 (a2, a3);	// sort_pair (a2, a3);
	a6 = _mm_min_epi16 (a6, a7);	// sort_pair (a6, a7);

	return (fstb::ToolsSse2::limit_epi16 (c, a3, a6));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 4

int	AvsFilterRemoveGrain16::OpRG04::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	int				a [8] = { a1, a2, a3, a4, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [7]) + 1);

	return (fstb::limit (c, a [4-1], a [5-1]));
}

__m128i	AvsFilterRemoveGrain16::OpRG04::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	// http://en.wikipedia.org/wiki/Batcher_odd%E2%80%93even_mergesort

	AvsFilterRemoveGrain16_READ_PIX

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

	return (fstb::ToolsSse2::limit_epi16 (c, a4, a5));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 5

int	AvsFilterRemoveGrain16::OpRG05::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	AvsFilterRemoveGrain16_SORT_AXIS_CPP

	const int      c1 = std::abs (c - fstb::limit (c, mi1, ma1));
	const int      c2 = std::abs (c - fstb::limit (c, mi2, ma2));
	const int      c3 = std::abs (c - fstb::limit (c, mi3, ma3));
	const int      c4 = std::abs (c - fstb::limit (c, mi4, ma4));

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	if (mindiff == c4)
	{
		return (fstb::limit (c, mi4, ma4));
	}
	else if (mindiff == c2)
	{
		return (fstb::limit (c, mi2, ma2));
	}
	else if (mindiff == c3)
	{
		return (fstb::limit (c, mi3, ma3));
	}

	return (fstb::limit (c, mi1, ma1));
}

__m128i	AvsFilterRemoveGrain16::OpRG05::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX
	AvsFilterRemoveGrain16_SORT_AXIS_SSE2

	const __m128i  cli1  = fstb::ToolsSse2::limit_epi16 (c, mi1, ma1);
	const __m128i  cli2  = fstb::ToolsSse2::limit_epi16 (c, mi2, ma2);
	const __m128i  cli3  = fstb::ToolsSse2::limit_epi16 (c, mi3, ma3);
	const __m128i  cli4  = fstb::ToolsSse2::limit_epi16 (c, mi4, ma4);

	const __m128i  cli1u = _mm_xor_si128 (cli1, mask_sign);
	const __m128i  cli2u = _mm_xor_si128 (cli2, mask_sign);
	const __m128i  cli3u = _mm_xor_si128 (cli3, mask_sign);
	const __m128i  cli4u = _mm_xor_si128 (cli4, mask_sign);
	const __m128i  cu    = _mm_xor_si128 (c   , mask_sign);

	const __m128i  c1u   = fstb::ToolsSse2::abs_dif_epu16 (cu, cli1u);
	const __m128i  c2u   = fstb::ToolsSse2::abs_dif_epu16 (cu, cli2u);
	const __m128i  c3u   = fstb::ToolsSse2::abs_dif_epu16 (cu, cli3u);
	const __m128i  c4u   = fstb::ToolsSse2::abs_dif_epu16 (cu, cli4u);

	const __m128i  c1    = _mm_xor_si128 (c1u, mask_sign);
	const __m128i  c2    = _mm_xor_si128 (c2u, mask_sign);
	const __m128i  c3    = _mm_xor_si128 (c3u, mask_sign);
	const __m128i  c4    = _mm_xor_si128 (c4u, mask_sign);

	const __m128i  mindiff = _mm_min_epi16 (
		_mm_min_epi16 (c1, c2),
		_mm_min_epi16 (c3, c4)
	);

	__m128i        res = cli1;
	res = fstb::ToolsSse2::select_16_equ (mindiff, c3, cli3, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, c2, cli2, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, c4, cli4, res);

	return (res);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 6

int	AvsFilterRemoveGrain16::OpRG06::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	AvsFilterRemoveGrain16_SORT_AXIS_CPP

	const int      d1   = ma1 - mi1;
	const int      d2   = ma2 - mi2;
	const int      d3   = ma3 - mi3;
	const int      d4   = ma4 - mi4;

	const int      cli1 = fstb::limit (c, mi1, ma1);
	const int      cli2 = fstb::limit (c, mi2, ma2);
	const int      cli3 = fstb::limit (c, mi3, ma3);
	const int      cli4 = fstb::limit (c, mi4, ma4);

	const int      c1   = fstb::limit ((std::abs (c - cli1) << 1) + d1, 0, 0xFFFF);
	const int      c2   = fstb::limit ((std::abs (c - cli2) << 1) + d2, 0, 0xFFFF);
	const int      c3   = fstb::limit ((std::abs (c - cli3) << 1) + d3, 0, 0xFFFF);
	const int      c4   = fstb::limit ((std::abs (c - cli4) << 1) + d4, 0, 0xFFFF);

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	if (mindiff == c4)
	{
		return (cli4);
	}
	else if (mindiff == c2)
	{
		return (cli2);
	}
	else if (mindiff == c3)
	{
		return (cli3);
	}

	return (cli1);
}

__m128i	AvsFilterRemoveGrain16::OpRG06::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX
	AvsFilterRemoveGrain16_SORT_AXIS_SSE2

	const __m128i  d1u   = _mm_sub_epi16 (ma1, mi1);
	const __m128i  d2u   = _mm_sub_epi16 (ma2, mi2);
	const __m128i  d3u   = _mm_sub_epi16 (ma3, mi3);
	const __m128i  d4u   = _mm_sub_epi16 (ma4, mi4);

	const __m128i  cli1  = fstb::ToolsSse2::limit_epi16 (c, mi1, ma1);
	const __m128i  cli2  = fstb::ToolsSse2::limit_epi16 (c, mi2, ma2);
	const __m128i  cli3  = fstb::ToolsSse2::limit_epi16 (c, mi3, ma3);
	const __m128i  cli4  = fstb::ToolsSse2::limit_epi16 (c, mi4, ma4);

	const __m128i  cli1u = _mm_xor_si128 (cli1, mask_sign);
	const __m128i  cli2u = _mm_xor_si128 (cli2, mask_sign);
	const __m128i  cli3u = _mm_xor_si128 (cli3, mask_sign);
	const __m128i  cli4u = _mm_xor_si128 (cli4, mask_sign);
	const __m128i  cu    = _mm_xor_si128 (c   , mask_sign);

	const __m128i  ad1u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli1u);
	const __m128i  ad2u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli2u);
	const __m128i  ad3u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli3u);
	const __m128i  ad4u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli4u);

	const __m128i  c1u   = _mm_adds_epu16 (_mm_adds_epu16 (d1u, ad1u), ad1u);
	const __m128i  c2u   = _mm_adds_epu16 (_mm_adds_epu16 (d2u, ad2u), ad2u);
	const __m128i  c3u   = _mm_adds_epu16 (_mm_adds_epu16 (d3u, ad3u), ad3u);
	const __m128i  c4u   = _mm_adds_epu16 (_mm_adds_epu16 (d4u, ad4u), ad4u);

	const __m128i  c1    = _mm_xor_si128 (c1u, mask_sign);
	const __m128i  c2    = _mm_xor_si128 (c2u, mask_sign);
	const __m128i  c3    = _mm_xor_si128 (c3u, mask_sign);
	const __m128i  c4    = _mm_xor_si128 (c4u, mask_sign);

	const __m128i  mindiff = _mm_min_epi16 (
		_mm_min_epi16 (c1, c2),
		_mm_min_epi16 (c3, c4)
	);

	__m128i        res = cli1;
	res = fstb::ToolsSse2::select_16_equ (mindiff, c3, cli3, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, c2, cli2, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, c4, cli4, res);

	return (res);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 7

int	AvsFilterRemoveGrain16::OpRG07::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	AvsFilterRemoveGrain16_SORT_AXIS_CPP

	const int      d1   = ma1 - mi1;
	const int      d2   = ma2 - mi2;
	const int      d3   = ma3 - mi3;
	const int      d4   = ma4 - mi4;

	const int      cli1 = fstb::limit (c, mi1, ma1);
	const int      cli2 = fstb::limit (c, mi2, ma2);
	const int      cli3 = fstb::limit (c, mi3, ma3);
	const int      cli4 = fstb::limit (c, mi4, ma4);

	const int      c1   = std::abs (c - cli1) + d1;
	const int      c2   = std::abs (c - cli2) + d2;
	const int      c3   = std::abs (c - cli3) + d3;
	const int      c4   = std::abs (c - cli4) + d4;

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	if (mindiff == c4)
	{
		return (cli4);
	}
	else if (mindiff == c2)
	{
		return (cli2);
	}
	else if (mindiff == c3)
	{
		return (cli3);
	}

	return (cli1);
}

__m128i	AvsFilterRemoveGrain16::OpRG07::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX
	AvsFilterRemoveGrain16_SORT_AXIS_SSE2

	const __m128i  d1u   = _mm_sub_epi16 (ma1, mi1);
	const __m128i  d2u   = _mm_sub_epi16 (ma2, mi2);
	const __m128i  d3u   = _mm_sub_epi16 (ma3, mi3);
	const __m128i  d4u   = _mm_sub_epi16 (ma4, mi4);

	const __m128i  cli1  = fstb::ToolsSse2::limit_epi16 (c, mi1, ma1);
	const __m128i  cli2  = fstb::ToolsSse2::limit_epi16 (c, mi2, ma2);
	const __m128i  cli3  = fstb::ToolsSse2::limit_epi16 (c, mi3, ma3);
	const __m128i  cli4  = fstb::ToolsSse2::limit_epi16 (c, mi4, ma4);

	const __m128i  cli1u = _mm_xor_si128 (cli1, mask_sign);
	const __m128i  cli2u = _mm_xor_si128 (cli2, mask_sign);
	const __m128i  cli3u = _mm_xor_si128 (cli3, mask_sign);
	const __m128i  cli4u = _mm_xor_si128 (cli4, mask_sign);
	const __m128i  cu    = _mm_xor_si128 (c   , mask_sign);

	const __m128i  ad1u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli1u);
	const __m128i  ad2u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli2u);
	const __m128i  ad3u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli3u);
	const __m128i  ad4u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli4u);

	const __m128i  c1u   = _mm_adds_epu16 (d1u, ad1u);
	const __m128i  c2u   = _mm_adds_epu16 (d2u, ad2u);
	const __m128i  c3u   = _mm_adds_epu16 (d3u, ad3u);
	const __m128i  c4u   = _mm_adds_epu16 (d4u, ad4u);

	const __m128i  c1    = _mm_xor_si128 (c1u, mask_sign);
	const __m128i  c2    = _mm_xor_si128 (c2u, mask_sign);
	const __m128i  c3    = _mm_xor_si128 (c3u, mask_sign);
	const __m128i  c4    = _mm_xor_si128 (c4u, mask_sign);

	const __m128i  mindiff = _mm_min_epi16 (
		_mm_min_epi16 (c1, c2),
		_mm_min_epi16 (c3, c4)
	);

	__m128i        res = cli1;
	res = fstb::ToolsSse2::select_16_equ (mindiff, c3, cli3, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, c2, cli2, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, c4, cli4, res);

	return (res);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 8

int	AvsFilterRemoveGrain16::OpRG08::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	AvsFilterRemoveGrain16_SORT_AXIS_CPP

	const int      d1   = ma1 - mi1;
	const int      d2   = ma2 - mi2;
	const int      d3   = ma3 - mi3;
	const int      d4   = ma4 - mi4;

	const int      cli1 = fstb::limit (c, mi1, ma1);
	const int      cli2 = fstb::limit (c, mi2, ma2);
	const int      cli3 = fstb::limit (c, mi3, ma3);
	const int      cli4 = fstb::limit (c, mi4, ma4);

	const int      c1   = fstb::limit (std::abs (c - cli1) + (d1 << 1), 0, 0xFFFF);
	const int      c2   = fstb::limit (std::abs (c - cli2) + (d2 << 1), 0, 0xFFFF);
	const int      c3   = fstb::limit (std::abs (c - cli3) + (d3 << 1), 0, 0xFFFF);
	const int      c4   = fstb::limit (std::abs (c - cli4) + (d4 << 1), 0, 0xFFFF);

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	if (mindiff == c4)
	{
		return (cli4);
	}
	else if (mindiff == c2)
	{
		return (cli2);
	}
	else if (mindiff == c3)
	{
		return (cli3);
	}

	return (cli1);
}

__m128i	AvsFilterRemoveGrain16::OpRG08::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX
	AvsFilterRemoveGrain16_SORT_AXIS_SSE2

	const __m128i  d1u   = _mm_sub_epi16 (ma1, mi1);
	const __m128i  d2u   = _mm_sub_epi16 (ma2, mi2);
	const __m128i  d3u   = _mm_sub_epi16 (ma3, mi3);
	const __m128i  d4u   = _mm_sub_epi16 (ma4, mi4);

	const __m128i  cli1  = fstb::ToolsSse2::limit_epi16 (c, mi1, ma1);
	const __m128i  cli2  = fstb::ToolsSse2::limit_epi16 (c, mi2, ma2);
	const __m128i  cli3  = fstb::ToolsSse2::limit_epi16 (c, mi3, ma3);
	const __m128i  cli4  = fstb::ToolsSse2::limit_epi16 (c, mi4, ma4);

	const __m128i  cli1u = _mm_xor_si128 (cli1, mask_sign);
	const __m128i  cli2u = _mm_xor_si128 (cli2, mask_sign);
	const __m128i  cli3u = _mm_xor_si128 (cli3, mask_sign);
	const __m128i  cli4u = _mm_xor_si128 (cli4, mask_sign);
	const __m128i  cu    = _mm_xor_si128 (c   , mask_sign);

	const __m128i  ad1u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli1u);
	const __m128i  ad2u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli2u);
	const __m128i  ad3u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli3u);
	const __m128i  ad4u  = fstb::ToolsSse2::abs_dif_epu16 (cu, cli4u);

	const __m128i  c1u   = _mm_adds_epu16 (_mm_adds_epu16 (d1u, d1u), ad1u);
	const __m128i  c2u   = _mm_adds_epu16 (_mm_adds_epu16 (d2u, d2u), ad2u);
	const __m128i  c3u   = _mm_adds_epu16 (_mm_adds_epu16 (d3u, d3u), ad3u);
	const __m128i  c4u   = _mm_adds_epu16 (_mm_adds_epu16 (d4u, d4u), ad4u);

	const __m128i  c1    = _mm_xor_si128 (c1u, mask_sign);
	const __m128i  c2    = _mm_xor_si128 (c2u, mask_sign);
	const __m128i  c3    = _mm_xor_si128 (c3u, mask_sign);
	const __m128i  c4    = _mm_xor_si128 (c4u, mask_sign);

	const __m128i  mindiff = _mm_min_epi16 (
		_mm_min_epi16 (c1, c2),
		_mm_min_epi16 (c3, c4)
	);

	__m128i        res = cli1;
	res = fstb::ToolsSse2::select_16_equ (mindiff, c3, cli3, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, c2, cli2, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, c4, cli4, res);

	return (res);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 9

int	AvsFilterRemoveGrain16::OpRG09::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	AvsFilterRemoveGrain16_SORT_AXIS_CPP

	const int      d1 = ma1 - mi1;
	const int      d2 = ma2 - mi2;
	const int      d3 = ma3 - mi3;
	const int      d4 = ma4 - mi4;

	const int      mindiff = std::min (std::min (d1, d2), std::min (d3, d4));

	if (mindiff == d4)
	{
		return (fstb::limit (c, mi4, ma4));
	}
	else if (mindiff == d2)
	{
		return (fstb::limit (c, mi2, ma2));
	}
	else if (mindiff == d3)
	{
		return (fstb::limit (c, mi3, ma3));
	}

	return (fstb::limit (c, mi1, ma1));
}

__m128i	AvsFilterRemoveGrain16::OpRG09::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX
	AvsFilterRemoveGrain16_SORT_AXIS_SSE2

	const __m128i  cli1  = fstb::ToolsSse2::limit_epi16 (c, mi1, ma1);
	const __m128i  cli2  = fstb::ToolsSse2::limit_epi16 (c, mi2, ma2);
	const __m128i  cli3  = fstb::ToolsSse2::limit_epi16 (c, mi3, ma3);
	const __m128i  cli4  = fstb::ToolsSse2::limit_epi16 (c, mi4, ma4);

	const __m128i  d1u   = _mm_sub_epi16 (ma1, mi1);
	const __m128i  d2u   = _mm_sub_epi16 (ma2, mi2);
	const __m128i  d3u   = _mm_sub_epi16 (ma3, mi3);
	const __m128i  d4u   = _mm_sub_epi16 (ma4, mi4);

	const __m128i  d1    = _mm_xor_si128 (d1u, mask_sign);
	const __m128i  d2    = _mm_xor_si128 (d2u, mask_sign);
	const __m128i  d3    = _mm_xor_si128 (d3u, mask_sign);
	const __m128i  d4    = _mm_xor_si128 (d4u, mask_sign);

	const __m128i  mindiff = _mm_min_epi16 (
		_mm_min_epi16 (d1, d2),
		_mm_min_epi16 (d3, d4)
	);

	__m128i        res = cli1;
	res = fstb::ToolsSse2::select_16_equ (mindiff, d3, cli3, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d2, cli2, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d4, cli4, res);

	return (res);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 10

int	AvsFilterRemoveGrain16::OpRG10::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	const int      d1 = std::abs (c - a1);
	const int      d2 = std::abs (c - a2);
	const int      d3 = std::abs (c - a3);
	const int      d4 = std::abs (c - a4);
	const int      d5 = std::abs (c - a5);
	const int      d6 = std::abs (c - a6);
	const int      d7 = std::abs (c - a7);
	const int      d8 = std::abs (c - a8);

	const int      mindiff = std::min (
		std::min (std::min (d1, d2), std::min (d3, d4)),
		std::min (std::min (d5, d6), std::min (d7, d8))
	);

	if (mindiff == d7) { return (a7); }
	if (mindiff == d8) { return (a8); }
	if (mindiff == d6) { return (a6); }
	if (mindiff == d2) { return (a2); }
	if (mindiff == d3) { return (a3); }
	if (mindiff == d1) { return (a1); }
	if (mindiff == d5) { return (a5); }

	return (a4);
}

__m128i	AvsFilterRemoveGrain16::OpRG10::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

	const __m128i  d1u = fstb::ToolsSse2::abs_dif_epu16 (c, a1);
	const __m128i  d2u = fstb::ToolsSse2::abs_dif_epu16 (c, a2);
	const __m128i  d3u = fstb::ToolsSse2::abs_dif_epu16 (c, a3);
	const __m128i  d4u = fstb::ToolsSse2::abs_dif_epu16 (c, a4);
	const __m128i  d5u = fstb::ToolsSse2::abs_dif_epu16 (c, a5);
	const __m128i  d6u = fstb::ToolsSse2::abs_dif_epu16 (c, a6);
	const __m128i  d7u = fstb::ToolsSse2::abs_dif_epu16 (c, a7);
	const __m128i  d8u = fstb::ToolsSse2::abs_dif_epu16 (c, a8);

	const __m128i  d1  = _mm_xor_si128 (d1u, mask_sign);
	const __m128i  d2  = _mm_xor_si128 (d2u, mask_sign);
	const __m128i  d3  = _mm_xor_si128 (d3u, mask_sign);
	const __m128i  d4  = _mm_xor_si128 (d4u, mask_sign);
	const __m128i  d5  = _mm_xor_si128 (d5u, mask_sign);
	const __m128i  d6  = _mm_xor_si128 (d6u, mask_sign);
	const __m128i  d7  = _mm_xor_si128 (d7u, mask_sign);
	const __m128i  d8  = _mm_xor_si128 (d8u, mask_sign);

	const __m128i  mindiff = _mm_min_epi16 (
		_mm_min_epi16 (_mm_min_epi16 (d1, d2), _mm_min_epi16 (d3, d4)),
		_mm_min_epi16 (_mm_min_epi16 (d5, d6), _mm_min_epi16 (d7, d8))
	);

	__m128i        res = a4;
	res = fstb::ToolsSse2::select_16_equ (mindiff, d5, a5, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d1, a1, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d3, a3, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d2, a2, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d6, a6, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d8, a8, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d7, a7, res);

	return (res);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 11

int	AvsFilterRemoveGrain16::OpRG11::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	const int		sum = 4 * c + 2 * (a2 + a4 + a5 + a7) + a1 + a3 + a6 + a8;
	const int		val = (sum + 8) >> 4;

	return (val);
}

__m128i	AvsFilterRemoveGrain16::OpRG11::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	// Rounding does not match the original, but this is acceptable
	return (OpRG12::rg (src_msb_ptr, src_lsb_ptr, stride_src, mask_sign));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 12

int	AvsFilterRemoveGrain16::OpRG12::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	// Rounding does not match the original, but this is acceptable
	return (OpRG11::rg (c, a1, a2, a3, a4, a5, a6, a7, a8));
}

__m128i	AvsFilterRemoveGrain16::OpRG12::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

	const __m128i	bias = _mm_set1_epi16 (1);

	const __m128i	a13  = _mm_avg_epu16 (a1, a3);
	const __m128i	a123 = _mm_avg_epu16 (a2, a13);

	const __m128i	a68  = _mm_avg_epu16 (a6, a8);
	const __m128i	a678 = _mm_avg_epu16 (a7, a68);

	const __m128i	a45  = _mm_avg_epu16 (a4, a5);
	const __m128i	a4c5 = _mm_avg_epu16 (c, a45);

	const __m128i	a123678  = _mm_avg_epu16 (a123, a678);
	const __m128i	a123678b = _mm_subs_epu16 (a123678, bias);
	const __m128i	val      = _mm_avg_epu16 (a4c5, a123678b);

	return (val);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 13-14

int	AvsFilterRemoveGrain16::OpRG1314::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	fstb::unused (c, a4, a5);

	const int      d1 = std::abs (a1 - a8);
	const int      d2 = std::abs (a2 - a7);
	const int      d3 = std::abs (a3 - a6);

	const int      mindiff = std::min (std::min (d1, d2), d3);

	if (mindiff == d2)
	{
		return ((a2 + a7 + 1) >> 1);
	}
	if (mindiff == d3)
	{
		return ((a3 + a6 + 1) >> 1);
	}

	return ((a1 + a8 + 1) >> 1);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 15-16
// Rounding does not match the original, but this is acceptable

int	AvsFilterRemoveGrain16::OpRG1516::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	fstb::unused (c, a4, a5);

	const int      d1 = std::abs (a1 - a8);
	const int      d2 = std::abs (a2 - a7);
	const int      d3 = std::abs (a3 - a6);

	const int      mindiff = std::min (std::min (d1, d2), d3);
	const int      average = (2 * (a2 + a7) + a1 + a3 + a6 + a8 + 4) >> 3;

	if (mindiff == d2)
	{
		return (fstb::limit (average, std::min (a2, a7), std::max (a2, a7)));
	}
	if (mindiff == d3)
	{
		return (fstb::limit (average, std::min (a3, a6), std::max (a3, a6)));
	}

	return (fstb::limit (average, std::min (a1, a8), std::max (a1, a8)));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 17

int	AvsFilterRemoveGrain16::OpRG17::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	AvsFilterRemoveGrain16_SORT_AXIS_CPP

	const int      l = std::max (std::max (mi1, mi2), std::max (mi3, mi4));
	const int      u = std::min (std::min (ma1, ma2), std::min (ma3, ma4));

	return (fstb::limit (c, std::min (l, u), std::max (l, u)));
}

__m128i	AvsFilterRemoveGrain16::OpRG17::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX
	AvsFilterRemoveGrain16_SORT_AXIS_SSE2

	const __m128i  l = _mm_max_epi16 (
		_mm_max_epi16 (mi1, mi2),
		_mm_max_epi16 (mi3, mi4)
	);
	const __m128i  u = _mm_min_epi16 (
		_mm_min_epi16 (ma1, ma2),
		_mm_min_epi16 (ma3, ma4)
	);
	const __m128i  mi = _mm_min_epi16 (l, u);
	const __m128i  ma = _mm_max_epi16 (l, u);

	return (fstb::ToolsSse2::limit_epi16 (c, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 18

int	AvsFilterRemoveGrain16::OpRG18::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	const int      d1 = std::max (std::abs (c - a1), std::abs (c - a8));
	const int      d2 = std::max (std::abs (c - a2), std::abs (c - a7));
	const int      d3 = std::max (std::abs (c - a3), std::abs (c - a6));
	const int      d4 = std::max (std::abs (c - a4), std::abs (c - a5));

	const int      mindiff = std::min (std::min (d1, d2), std::min(d3, d4));

	if (mindiff == d4)
	{
		return (fstb::limit (c, std::min (a4, a5), std::max (a4, a5)));
	}
	if (mindiff == d2)
	{
		return (fstb::limit (c, std::min (a2, a7), std::max (a2, a7)));
	}
	if (mindiff == d3)
	{
		return (fstb::limit (c, std::min (a3, a6), std::max (a3, a6)));
	}

	return (fstb::limit (c, std::min (a1, a8), std::max (a1, a8)));
}

__m128i	AvsFilterRemoveGrain16::OpRG18::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

	const __m128i  absdiff1u = fstb::ToolsSse2::abs_dif_epu16 (c, a1);
	const __m128i  absdiff2u = fstb::ToolsSse2::abs_dif_epu16 (c, a2);
	const __m128i  absdiff3u = fstb::ToolsSse2::abs_dif_epu16 (c, a3);
	const __m128i  absdiff4u = fstb::ToolsSse2::abs_dif_epu16 (c, a4);
	const __m128i  absdiff5u = fstb::ToolsSse2::abs_dif_epu16 (c, a5);
	const __m128i  absdiff6u = fstb::ToolsSse2::abs_dif_epu16 (c, a6);
	const __m128i  absdiff7u = fstb::ToolsSse2::abs_dif_epu16 (c, a7);
	const __m128i  absdiff8u = fstb::ToolsSse2::abs_dif_epu16 (c, a8);

	const __m128i  absdiff1  = _mm_xor_si128 (absdiff1u, mask_sign);
	const __m128i  absdiff2  = _mm_xor_si128 (absdiff2u, mask_sign);
	const __m128i  absdiff3  = _mm_xor_si128 (absdiff3u, mask_sign);
	const __m128i  absdiff4  = _mm_xor_si128 (absdiff4u, mask_sign);
	const __m128i  absdiff5  = _mm_xor_si128 (absdiff5u, mask_sign);
	const __m128i  absdiff6  = _mm_xor_si128 (absdiff6u, mask_sign);
	const __m128i  absdiff7  = _mm_xor_si128 (absdiff7u, mask_sign);
	const __m128i  absdiff8  = _mm_xor_si128 (absdiff8u, mask_sign);

	const __m128i  d1 = _mm_max_epi16 (absdiff1, absdiff8);
	const __m128i  d2 = _mm_max_epi16 (absdiff2, absdiff7);
	const __m128i  d3 = _mm_max_epi16 (absdiff3, absdiff6);
	const __m128i  d4 = _mm_max_epi16 (absdiff4, absdiff5);

	const __m128i  mindiff = _mm_min_epi16 (
		_mm_min_epi16 (d1, d2),
		_mm_min_epi16 (d3, d4)
	);

	const __m128i  a1s  = _mm_xor_si128 (a1, mask_sign);
	const __m128i  a2s  = _mm_xor_si128 (a2, mask_sign);
	const __m128i  a3s  = _mm_xor_si128 (a3, mask_sign);
	const __m128i  a4s  = _mm_xor_si128 (a4, mask_sign);
	const __m128i  a5s  = _mm_xor_si128 (a5, mask_sign);
	const __m128i  a6s  = _mm_xor_si128 (a6, mask_sign);
	const __m128i  a7s  = _mm_xor_si128 (a7, mask_sign);
	const __m128i  a8s  = _mm_xor_si128 (a8, mask_sign);
	const __m128i  cs   = _mm_xor_si128 (c , mask_sign);

	const __m128i  ma1  = _mm_max_epi16 (a1s, a8s);
	const __m128i  mi1  = _mm_min_epi16 (a1s, a8s);
	const __m128i  ma2  = _mm_max_epi16 (a2s, a7s);
	const __m128i  mi2  = _mm_min_epi16 (a2s, a7s);
	const __m128i  ma3  = _mm_max_epi16 (a3s, a6s);
	const __m128i  mi3  = _mm_min_epi16 (a3s, a6s);
	const __m128i  ma4  = _mm_max_epi16 (a4s, a5s);
	const __m128i  mi4  = _mm_min_epi16 (a4s, a5s);

	const __m128i  cli1 = fstb::ToolsSse2::limit_epi16 (cs, mi1, ma1);
	const __m128i  cli2 = fstb::ToolsSse2::limit_epi16 (cs, mi2, ma2);
	const __m128i  cli3 = fstb::ToolsSse2::limit_epi16 (cs, mi3, ma3);
	const __m128i  cli4 = fstb::ToolsSse2::limit_epi16 (cs, mi4, ma4);

	__m128i        res = cli1;
	res = fstb::ToolsSse2::select_16_equ (mindiff, d3, cli3, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d2, cli2, res);
	res = fstb::ToolsSse2::select_16_equ (mindiff, d4, cli4, res);

	return (_mm_xor_si128 (res, mask_sign));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 19

int	AvsFilterRemoveGrain16::OpRG19::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	fstb::unused (c);

	const int		sum = a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
	const int		val = (sum + 4) >> 3;

//	const int		val = (  (((((a1 + a3 + 1) >> 1) + ((a6 + a8 + 1) >> 1) + 1) >> 1) - 1)
//						       +  ((((a2 + a5 + 1) >> 1) + ((a4 + a7 + 1) >> 1) + 1) >> 1)
//						       + 1) >> 1;

	return (val);
}

__m128i	AvsFilterRemoveGrain16::OpRG19::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX


	const __m128i	bias   = _mm_set1_epi16 (1);

	const __m128i	a13    = _mm_avg_epu16 (a1, a3);
	const __m128i	a68    = _mm_avg_epu16 (a6, a8);
	const __m128i	a1368  = _mm_avg_epu16 (a13, a68);
	const __m128i	a1368b = _mm_subs_epu16 (a1368, bias);
	const __m128i	a25    = _mm_avg_epu16 (a2, a5);
	const __m128i	a47    = _mm_avg_epu16 (a4, a7);
	const __m128i	a2457  = _mm_avg_epu16 (a25, a47);
	const __m128i	val    = _mm_avg_epu16 (a1368b, a2457);

	return (val);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 20

int	AvsFilterRemoveGrain16::OpRG20::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	const int		sum = a1 + a2 + a3 + a4 + c + a5 + a6 + a7 + a8;
	const int		val = (sum + 4) / 9;

	return (val);
}

__m128i	AvsFilterRemoveGrain16::OpRG20::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

	const __m128i  zero  = _mm_setzero_si128 ();

	__m128i        sum_0 = _mm_set1_epi32 (-0x8000 * 9 + 4);
	__m128i        sum_1 = sum_0;

	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, c , zero);
	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, a1, zero);
	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, a2, zero);
	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, a3, zero);
	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, a4, zero);
	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, a5, zero);
	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, a6, zero);
	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, a7, zero);
	fstb::ToolsSse2::add_x16_s32 (sum_0, sum_1, a8, zero);

	const __m128i  fix_0 = _mm_srai_epi32 (sum_0, 15);
	const __m128i  fix_1 = _mm_srai_epi32 (sum_1, 15);
	sum_0 = _mm_sub_epi32 (sum_0, fix_0);
	sum_1 = _mm_sub_epi32 (sum_1, fix_1);

	const __m128i  mult = _mm_set1_epi16 (((1 << 16) + 4) / 9);
	const __m128i  val = fstb::ToolsSse2::mul_s32_s15_s16 (sum_0, sum_1, mult);

	return (_mm_xor_si128 (val, mask_sign));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 21
// Rounding does not match the original

int	AvsFilterRemoveGrain16::OpRG21::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	const int      l1l = (a1 + a8    ) >> 1;
	const int      l2l = (a2 + a7    ) >> 1;
	const int      l3l = (a3 + a6    ) >> 1;
	const int      l4l = (a4 + a5    ) >> 1;

	const int      l1h = (a1 + a8 + 1) >> 1;
	const int      l2h = (a2 + a7 + 1) >> 1;
	const int      l3h = (a3 + a6 + 1) >> 1;
	const int      l4h = (a4 + a5 + 1) >> 1;

	const int      mi = std::min (std::min (l1l, l2l), std::min (l3l, l4l));
	const int      ma = std::max (std::max (l1h, l2h), std::max (l3h, l4h));

	return (fstb::limit (c, mi, ma));
}

__m128i	AvsFilterRemoveGrain16::OpRG21::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

	const __m128i  bit0 =
		_mm_load_si128 (reinterpret_cast <const __m128i *> (_bit0));

	const __m128i  odd1 = _mm_and_si128 (_mm_xor_si128 (a1, a8), bit0);
	const __m128i  odd2 = _mm_and_si128 (_mm_xor_si128 (a2, a7), bit0);
	const __m128i  odd3 = _mm_and_si128 (_mm_xor_si128 (a3, a6), bit0);
	const __m128i  odd4 = _mm_and_si128 (_mm_xor_si128 (a4, a5), bit0);

	const __m128i  l1hu = _mm_avg_epu16 (a1, a8);
	const __m128i  l2hu = _mm_avg_epu16 (a2, a7);
	const __m128i  l3hu = _mm_avg_epu16 (a3, a6);
	const __m128i  l4hu = _mm_avg_epu16 (a4, a5);

	const __m128i  l1h  = _mm_xor_si128 (l1hu, mask_sign);
	const __m128i  l2h  = _mm_xor_si128 (l2hu, mask_sign);
	const __m128i  l3h  = _mm_xor_si128 (l3hu, mask_sign);
	const __m128i  l4h  = _mm_xor_si128 (l4hu, mask_sign);

	const __m128i  l1l  = _mm_subs_epi16 (l1h, odd1);
	const __m128i  l2l  = _mm_subs_epi16 (l2h, odd2);
	const __m128i  l3l  = _mm_subs_epi16 (l3h, odd3);
	const __m128i  l4l  = _mm_subs_epi16 (l4h, odd4);

	const __m128i  mi = _mm_min_epi16 (
		_mm_min_epi16 (l1l, l2l),
		_mm_min_epi16 (l3l, l4l)
	);
	const __m128i  ma = _mm_max_epi16 (
		_mm_max_epi16 (l1h, l2h),
		_mm_max_epi16 (l3h, l4h)
	);

	const __m128i  cs  = _mm_xor_si128 (c, mask_sign);
	const __m128i  res = fstb::ToolsSse2::limit_epi16 (cs, mi, ma);

	return (_mm_xor_si128 (res, mask_sign));
}

const __declspec (align (16)) uint16_t	AvsFilterRemoveGrain16::OpRG21::_bit0 [8] =
{ 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 };



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 22

int	AvsFilterRemoveGrain16::OpRG22::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	const int      l1 = (a1 + a8 + 1) >> 1;
	const int      l2 = (a2 + a7 + 1) >> 1;
	const int      l3 = (a3 + a6 + 1) >> 1;
	const int      l4 = (a4 + a5 + 1) >> 1;

	const int      mi = std::min (std::min (l1, l2), std::min (l3, l4));
	const int      ma = std::max (std::max (l1, l2), std::max (l3, l4));

	return (fstb::limit (c, mi, ma));
}

__m128i	AvsFilterRemoveGrain16::OpRG22::rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign)
{
	AvsFilterRemoveGrain16_READ_PIX

	const __m128i  l1u = _mm_avg_epu16 (a1, a8);
	const __m128i  l2u = _mm_avg_epu16 (a2, a7);
	const __m128i  l3u = _mm_avg_epu16 (a3, a6);
	const __m128i  l4u = _mm_avg_epu16 (a4, a5);

	const __m128i  l1  = _mm_xor_si128 (l1u, mask_sign);
	const __m128i  l2  = _mm_xor_si128 (l2u, mask_sign);
	const __m128i  l3  = _mm_xor_si128 (l3u, mask_sign);
	const __m128i  l4  = _mm_xor_si128 (l4u, mask_sign);

	const __m128i  mi = _mm_min_epi16 (
		_mm_min_epi16 (l1, l2),
		_mm_min_epi16 (l3, l4)
	);
	const __m128i  ma = _mm_max_epi16 (
		_mm_max_epi16 (l1, l2),
		_mm_max_epi16 (l3, l4)
	);

	const __m128i  cs  = _mm_xor_si128 (c, mask_sign);
	const __m128i  res = fstb::ToolsSse2::limit_epi16 (cs, mi, ma);

	return (_mm_xor_si128 (res, mask_sign));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 23

int	AvsFilterRemoveGrain16::OpRG23::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	AvsFilterRemoveGrain16_SORT_AXIS_CPP

	const int      linediff1 = ma1 - mi1;
	const int      linediff2 = ma2 - mi2;
	const int      linediff3 = ma3 - mi3;
	const int      linediff4 = ma4 - mi4;

	const int      u1 = std::min (c - ma1, linediff1);
	const int      u2 = std::min (c - ma2, linediff2);
	const int      u3 = std::min (c - ma3, linediff3);
	const int      u4 = std::min (c - ma4, linediff4);
	const int      u  = std::max (
		std::max (std::max (u1, u2), std::max (u3, u4)),
		0
	);

	const int      d1 = std::min (mi1 - c, linediff1);
	const int      d2 = std::min (mi2 - c, linediff2);
	const int      d3 = std::min (mi3 - c, linediff3);
	const int      d4 = std::min (mi4 - c, linediff4);
	const int      d  = std::max (
		std::max (std::max (d1, d2), std::max (d3, d4)),
		0
	);

	return (c - u + d);  // This probably will never overflow.
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 24

int	AvsFilterRemoveGrain16::OpRG24::rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	AvsFilterRemoveGrain16_SORT_AXIS_CPP

	const int      linediff1 = ma1 - mi1;
	const int      linediff2 = ma2 - mi2;
	const int      linediff3 = ma3 - mi3;
	const int      linediff4 = ma4 - mi4;

	const int      tu1 = c - ma1;
	const int      tu2 = c - ma2;
	const int      tu3 = c - ma3;
	const int      tu4 = c - ma4;

	const int      u1 = std::min (tu1, linediff1 - tu1);
	const int      u2 = std::min (tu2, linediff2 - tu2);
	const int      u3 = std::min (tu3, linediff3 - tu3);
	const int      u4 = std::min (tu4, linediff4 - tu4);
	const int      u  = std::max (
		std::max (std::max (u1, u2), std::max (u3, u4)),
		0
	);

	const int      td1 = mi1 - c;
	const int      td2 = mi2 - c;
	const int      td3 = mi3 - c;
	const int      td4 = mi4 - c;

	const int      d1 = std::min (td1, linediff1 - td1);
	const int      d2 = std::min (td2, linediff2 - td2);
	const int      d3 = std::min (td3, linediff3 - td3);
	const int      d4 = std::min (td4, linediff4 - td4);
	const int      d  = std::max (
		std::max (std::max (d1, d2), std::max (d3, d4)),
		0
	);

	return (c - u + d);  // This probably will never overflow.
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

#undef AvsFilterRemoveGrain16_READ_PIX
#undef AvsFilterRemoveGrain16_SORT_AXIS_CPP



void	AvsFilterRemoveGrain16::process_subplane (Slicer::TaskData &td)
{
	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);

	(*(_process_subplane_ptr_arr [tdg._plane_index])) (td);
}



template <class OP>
void	AvsFilterRemoveGrain16::PlaneProc <OP>::process_subplane_cpp (Slicer::TaskData &td)
{
	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);

	const int		y_b = std::max (td._y_beg, 1);
	const int		y_e = std::min (td._y_end, tdg._h - 1);

	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + y_b * tdg._stride_dst;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + y_b * tdg._stride_dst;
	const uint8_t*	src_msb_ptr = tdg._src_msb_ptr + y_b * tdg._stride_src;
	const uint8_t*	src_lsb_ptr = tdg._src_lsb_ptr + y_b * tdg._stride_src;

	const int		x_e = tdg._w - 1;

	for (int y = y_b; y < y_e; ++y)
	{
		if (OP::skip_line (y))
		{
			memcpy (dst_msb_ptr, src_msb_ptr, tdg._w);
			memcpy (dst_lsb_ptr, src_lsb_ptr, tdg._w);
		}
		else
		{
			dst_msb_ptr [0] = src_msb_ptr [0];
			dst_lsb_ptr [0] = src_lsb_ptr [0];

			process_row_cpp (
				dst_msb_ptr, dst_lsb_ptr,
				src_msb_ptr, src_lsb_ptr,
				tdg._stride_src,
				1,
				x_e
			);

			dst_msb_ptr [x_e] = src_msb_ptr [x_e];
			dst_lsb_ptr [x_e] = src_lsb_ptr [x_e];
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		src_msb_ptr += tdg._stride_src;
		src_lsb_ptr += tdg._stride_src;
	}
}



template <class OP>
void	AvsFilterRemoveGrain16::PlaneProc <OP>::process_row_cpp (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, int x_beg, int x_end)
{
	const int      om = stride_src - 1;
	const int      o0 = stride_src    ;
	const int      op = stride_src + 1;

	src_msb_ptr += x_beg;
	src_lsb_ptr += x_beg;

	for (int x = x_beg; x < x_end; ++x)
	{
		const int		a1 = (src_msb_ptr [-op] << 8) + src_lsb_ptr [-op];
		const int		a2 = (src_msb_ptr [-o0] << 8) + src_lsb_ptr [-o0];
		const int		a3 = (src_msb_ptr [-om] << 8) + src_lsb_ptr [-om];
		const int		a4 = (src_msb_ptr [-1 ] << 8) + src_lsb_ptr [-1 ];
		const int		c  = (src_msb_ptr [ 0 ] << 8) + src_lsb_ptr [ 0 ];
		const int		a5 = (src_msb_ptr [ 1 ] << 8) + src_lsb_ptr [ 1 ];
		const int		a6 = (src_msb_ptr [ om] << 8) + src_lsb_ptr [ om];
		const int		a7 = (src_msb_ptr [ o0] << 8) + src_lsb_ptr [ o0];
		const int		a8 = (src_msb_ptr [ op] << 8) + src_lsb_ptr [ op];

		const int		res = OP::rg (c, a1, a2, a3, a4, a5, a6, a7, a8);

		dst_msb_ptr [x] = uint8_t (res >> 8 );
		dst_lsb_ptr [x] = uint8_t (res & 255);

		++ src_msb_ptr;
		++ src_lsb_ptr;
	}
}



template <class OP>
void	AvsFilterRemoveGrain16::PlaneProc <OP>::process_subplane_sse2 (Slicer::TaskData &td)
{
	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);

	const int		y_b = std::max (td._y_beg, 1);
	const int		y_e = std::min (td._y_end, tdg._h - 1);

	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + y_b * tdg._stride_dst;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + y_b * tdg._stride_dst;
	const uint8_t*	src_msb_ptr = tdg._src_msb_ptr + y_b * tdg._stride_src;
	const uint8_t*	src_lsb_ptr = tdg._src_lsb_ptr + y_b * tdg._stride_src;

	const __m128i	mask_lsb  = _mm_set1_epi16 ( 0x00FF);
	const __m128i	mask_sign = _mm_set1_epi16 (-0x8000);

	const int		x_e =   tdg._w - 1;
	const int		w8  = ((tdg._w - 2) & -8) + 1;
	const int		w7  = x_e - w8;

	for (int y = y_b; y < y_e; ++y)
	{
		if (OP::skip_line (y))
		{
			memcpy (dst_msb_ptr, src_msb_ptr, tdg._w);
			memcpy (dst_lsb_ptr, src_lsb_ptr, tdg._w);
		}
		else
		{
			dst_msb_ptr [0] = src_msb_ptr [0];
			dst_lsb_ptr [0] = src_lsb_ptr [0];

			for (int x = 1; x < w8; x += 8)
			{
				__m128i			res = OP::rg (
					src_msb_ptr + x, src_lsb_ptr + x,
					tdg._stride_src,
					mask_sign
				);

				res = OP::ConvSign::cv (res, mask_sign);
				fstb::ToolsSse2::store_8_16ml (
					dst_msb_ptr + x,
					dst_lsb_ptr + x,
					res,
					mask_lsb
				);
			}

			process_row_cpp (
				dst_msb_ptr, dst_lsb_ptr,
				src_msb_ptr, src_lsb_ptr,
				tdg._stride_src,
				w8,
				x_e
			);

			dst_msb_ptr [x_e] = src_msb_ptr [x_e];
			dst_lsb_ptr [x_e] = src_lsb_ptr [x_e];
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		src_msb_ptr += tdg._stride_src;
		src_lsb_ptr += tdg._stride_src;
	}
}



PlaneProcMode	AvsFilterRemoveGrain16::conv_rgmode_to_proc (int rgmode)
{
	PlaneProcMode	ppmode = PlaneProcMode_COPY1;

	if (rgmode < 0)
	{
		ppmode = PlaneProcMode_GARBAGE;
	}
	else if (rgmode > 0)
	{
		ppmode = PlaneProcMode_PROCESS;
	}

	return (ppmode);
}



void	AvsFilterRemoveGrain16::sort_pair (__m128i &a1, __m128i &a2)
{
	const __m128i	tmp = _mm_min_epi16 (a1, a2);
	a2 = _mm_max_epi16 (a1, a2);
	a1 = tmp;
}



void	AvsFilterRemoveGrain16::sort_pair (__m128i &mi, __m128i &ma, __m128i a1, __m128i a2)
{
	mi = _mm_min_epi16 (a1, a2);
	ma = _mm_max_epi16 (a1, a2);
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
