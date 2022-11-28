/*****************************************************************************

        AvsFilterRepair16.cpp
        Author: Laurent de Soras, 2012

Modes 5-10 and 15-18 adapted from TP7's RgTools.

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

#include "fstb/def.h"
#include "fstb/fnc.h"
#include "fstb/ToolsSse2.h"
#include "AvsFilterRepair16.h"

#include <algorithm>

#include <cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterRepair16::AvsFilterRepair16 (::IScriptEnvironment *env_ptr, ::PClip stack1_clip_sptr, ::PClip stack2_clip_sptr, int mode, int modeu, int modev)
:	GenericVideoFilter (stack1_clip_sptr)
,	_stack1_clip_sptr (stack1_clip_sptr)
,	_stack2_clip_sptr (stack2_clip_sptr)
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
		env_ptr->ThrowError ("Dither_Repair16: input must be planar.");
	}

	PlaneProcessor::check_same_format (env_ptr, vi, stack2_clip_sptr, "Dither_Repair16", "src2");

	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_Repair16: stacked MSB/LSB data must have\n"
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
	_plane_proc.set_clip_info (2, PlaneProcessor::ClipType_STACKED_16);

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
		case 11: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG01>::process_subplane_cpp; break;	// Same as 1
		case 12: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG12>::process_subplane_cpp; break;
		case 13: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG13>::process_subplane_cpp; break;
		case 14: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG14>::process_subplane_cpp; break;
		case 15: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG15>::process_subplane_cpp; break;
		case 16: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG16>::process_subplane_cpp; break;
		case 17: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG17>::process_subplane_cpp; break;
		case 18: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG18>::process_subplane_cpp; break;
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
			case 11: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG01>::process_subplane_sse2; break;	// Same as 1
			case 12: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG12>::process_subplane_sse2; break;
			case 13: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG13>::process_subplane_sse2; break;
			case 14: _process_subplane_ptr_arr [plane] = &PlaneProc <OpRG14>::process_subplane_sse2; break;
			default: break;
			}
		}

		if (_process_subplane_ptr_arr [plane] == 0)
		{
			if (_mode_arr [plane] > 0)
			{
				env_ptr->ThrowError (
					"Dither_Repair16: unsupported mode."
				);
			}
			else
			{
				_process_subplane_ptr_arr [plane] = &PlaneProc <OpRG00>::process_subplane_cpp;
			}
		}
	}
}



::PVideoFrame __stdcall	AvsFilterRepair16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_stack1_clip_sptr, &_stack2_clip_sptr, 0
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterRepair16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	src1_sptr = _stack1_clip_sptr->GetFrame (n, &env);
	::PVideoFrame	src2_sptr = _stack2_clip_sptr->GetFrame (n, &env);

	const int		w             = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h             = _plane_proc.get_height16 (dst_sptr, plane_id);
	uint8_t *		data_msbd_ptr = dst_sptr->GetWritePtr (plane_id);
	const int		stride_dst    = dst_sptr->GetPitch (plane_id);

	const uint8_t*	data_msb1_ptr = src1_sptr->GetReadPtr (plane_id);
	const int		stride_sr1    = src1_sptr->GetPitch (plane_id);

	const uint8_t*	data_msb2_ptr = src2_sptr->GetReadPtr (plane_id);
	const int		stride_sr2    = src2_sptr->GetPitch (plane_id);

	TaskDataGlobal		tdg;
	tdg._this_ptr    = this;
	tdg._w           = w;
	tdg._h           = h;
	tdg._plane_index = plane_index;
	tdg._dst_msb_ptr = data_msbd_ptr;
	tdg._dst_lsb_ptr = data_msbd_ptr + h * stride_dst;
	tdg._sr1_msb_ptr = data_msb1_ptr;
	tdg._sr1_lsb_ptr = data_msb1_ptr + h * stride_sr1;
	tdg._sr2_msb_ptr = data_msb2_ptr;
	tdg._sr2_lsb_ptr = data_msb2_ptr + h * stride_sr2;
	tdg._stride_dst  = stride_dst;
	tdg._stride_sr1  = stride_sr1;
	tdg._stride_sr2  = stride_sr2;

	// Main content
	Slicer			slicer;
	slicer.start (h, tdg, &AvsFilterRepair16::process_subplane);

	// First line
	memcpy (tdg._dst_msb_ptr, tdg._sr1_msb_ptr, w);
	memcpy (tdg._dst_lsb_ptr, tdg._sr1_lsb_ptr, w);

	// Last line
	const int		lp_dst = (h - 1) * stride_dst;
	const int		lp_sr1 = (h - 1) * stride_sr1;
	memcpy (tdg._dst_msb_ptr + lp_dst, tdg._sr1_msb_ptr + lp_sr1, w);
	memcpy (tdg._dst_lsb_ptr + lp_dst, tdg._sr1_lsb_ptr + lp_sr1, w);

	// Synchronisation point
	slicer.wait ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#define AvsFilterRepair16_READ_PIX    \
   const int      om = stride_sr2 - 1;     \
   const int      o0 = stride_sr2    ;     \
   const int      op = stride_sr2 + 1;     \
   __m128i        cr = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr1_msb_ptr     , sr1_lsb_ptr     ), mask_sign); \
   __m128i        a1 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr - op, sr2_lsb_ptr - op), mask_sign); \
   __m128i        a2 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr - o0, sr2_lsb_ptr - o0), mask_sign); \
   __m128i        a3 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr - om, sr2_lsb_ptr - om), mask_sign); \
   __m128i        a4 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr - 1 , sr2_lsb_ptr - 1 ), mask_sign); \
   __m128i        c  = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr     , sr2_lsb_ptr     ), mask_sign); \
   __m128i        a5 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr + 1 , sr2_lsb_ptr + 1 ), mask_sign); \
   __m128i        a6 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr + om, sr2_lsb_ptr + om), mask_sign); \
   __m128i        a7 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr + o0, sr2_lsb_ptr + o0), mask_sign); \
   __m128i        a8 = ConvSign::cv (fstb::ToolsSse2::load_8_16ml (sr2_msb_ptr + op, sr2_lsb_ptr + op), mask_sign);

#define AvsFilterRepair16_SORT_AXIS_CPP \
	const int      ma1 = std::max (a1, a8);   \
	const int      mi1 = std::min (a1, a8);   \
	const int      ma2 = std::max (a2, a7);   \
	const int      mi2 = std::min (a2, a7);   \
	const int      ma3 = std::max (a3, a6);   \
	const int      mi3 = std::min (a3, a6);   \
	const int      ma4 = std::max (a4, a5);   \
	const int      mi4 = std::min (a4, a5);

#define AvsFilterRepair16_SORT_AXIS2_CPP \
	const int      ma1 = std::max (std::max (a1, a8), c);   \
	const int      mi1 = std::min (std::min (a1, a8), c);   \
	const int      ma2 = std::max (std::max (a2, a7), c);   \
	const int      mi2 = std::min (std::min (a2, a7), c);   \
	const int      ma3 = std::max (std::max (a3, a6), c);   \
	const int      mi3 = std::min (std::min (a3, a6), c);   \
	const int      ma4 = std::max (std::max (a4, a5), c);   \
	const int      mi4 = std::min (std::min (a4, a5), c);



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 0 (bypass)

int	AvsFilterRepair16::OpRG00::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	fstb::unused (a1, a2, a3, a4, c, a5, a6, a7, a8);

	return (cr);
}

__m128i	AvsFilterRepair16::OpRG00::rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign)
{
	fstb::unused (sr2_msb_ptr, sr2_lsb_ptr, stride_sr2, mask_sign);

	return (fstb::ToolsSse2::load_8_16ml (sr1_msb_ptr, sr1_lsb_ptr));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 1

int	AvsFilterRepair16::OpRG01::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	const int		mi = std::min (std::min (
		std::min (std::min (a1, a2), std::min (a3, a4)),
		std::min (std::min (a5, a6), std::min (a7, a8))
	), c);
	const int		ma = std::max (std::max (
		std::max (std::max (a1, a2), std::max (a3, a4)),
		std::max (std::max (a5, a6), std::max (a7, a8))
	), c);

	return (fstb::limit (cr, mi, ma));
}

__m128i	AvsFilterRepair16::OpRG01::rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign)
{
	AvsFilterRepair16_READ_PIX

	const __m128i	mi = _mm_min_epi16 (_mm_min_epi16 (
		_mm_min_epi16 (_mm_min_epi16 (a1, a2), _mm_min_epi16 (a3, a4)),
		_mm_min_epi16 (_mm_min_epi16 (a5, a6), _mm_min_epi16 (a7, a8))
	), c);
	const __m128i	ma = _mm_max_epi16 (_mm_max_epi16 (
		_mm_max_epi16 (_mm_max_epi16 (a1, a2), _mm_max_epi16 (a3, a4)),
		_mm_max_epi16 (_mm_max_epi16 (a5, a6), _mm_max_epi16 (a7, a8))
	), c);

	return (fstb::ToolsSse2::limit_epi16 (cr, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 2

int	AvsFilterRepair16::OpRG02::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	int				a [9] = { a1, a2, a3, a4, c, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [8]) + 1);

	return (fstb::limit (cr, a [2-1], a [7]));
}

__m128i	AvsFilterRepair16::OpRG02::rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign)
{
	AvsFilterRepair16_READ_PIX

	sort_pair (a1, a8);

	sort_pair (a1,  c);
	sort_pair (a2, a5);
	sort_pair (a3, a6);
	sort_pair (a4, a7);
	sort_pair ( c, a8);

	sort_pair (a1, a3);
	sort_pair ( c, a6);
	sort_pair (a2, a4);
	sort_pair (a5, a7);

	sort_pair (a3, a8);

	sort_pair (a3,  c);
	sort_pair (a6, a8);
	sort_pair (a4, a5);

	a2 = _mm_max_epi16 (a1, a2);	// sort_pair (a1, a2);
	a3 = _mm_min_epi16 (a3, a4);	// sort_pair (a3, a4);
	sort_pair ( c, a5);
	a7 = _mm_max_epi16 (a6, a7);	// sort_pair (a6, a7);

	sort_pair (a2, a8);

	a2 = _mm_min_epi16 (a2,  c);	// sort_pair (a2,  c);
											// sort_pair (a4, a6);
	a8 = _mm_max_epi16 (a5, a8);	// sort_pair (a5, a8);

	a2 = _mm_min_epi16 (a2, a3);	// sort_pair (a2, a3);
											// sort_pair (a4,  c);
											// sort_pair (a5, a6);
	a7 = _mm_min_epi16 (a7, a8);	// sort_pair (a7, a8);

	return (fstb::ToolsSse2::limit_epi16 (cr, a2, a7));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 3

int	AvsFilterRepair16::OpRG03::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	int				a [9] = { a1, a2, a3, a4, c, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [8]) + 1);

	return (fstb::limit (cr, a [3-1], a [6]));
}

__m128i	AvsFilterRepair16::OpRG03::rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign)
{
	AvsFilterRepair16_READ_PIX

	sort_pair (a1, a8);

	sort_pair (a1,  c);
	sort_pair (a2, a5);
	sort_pair (a3, a6);
	sort_pair (a4, a7);
	sort_pair ( c, a8);

	sort_pair (a1, a3);
	sort_pair ( c, a6);
	sort_pair (a2, a4);
	sort_pair (a5, a7);

	sort_pair (a3, a8);

	sort_pair (a3,  c);
	sort_pair (a6, a8);
	sort_pair (a4, a5);

	a2 = _mm_max_epi16 (a1, a2);	// sort_pair (a1, a2);
	sort_pair (a3, a4);
	sort_pair ( c, a5);
	a6 = _mm_min_epi16 (a6, a7);	// sort_pair (a6, a7);

	sort_pair (a2, a8);

	a2 = _mm_min_epi16 (a2,  c);	// sort_pair (a2,  c);
	a6 = _mm_max_epi16 (a4, a6);	// sort_pair (a4, a6);
	a5 = _mm_min_epi16 (a5, a8);	// sort_pair (a5, a8);

	a3 = _mm_max_epi16 (a2, a3);	// sort_pair (a2, a3);
											// sort_pair (a4,  c);
	a6 = _mm_max_epi16 (a5, a6);	// sort_pair (a5, a6);
											// sort_pair (a7, a8);

	return (fstb::ToolsSse2::limit_epi16 (cr, a3, a6));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 4

int	AvsFilterRepair16::OpRG04::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	int				a [9] = { a1, a2, a3, a4, c, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [8]) + 1);

	return (fstb::limit (cr, a [4-1], a [5]));
}

__m128i	AvsFilterRepair16::OpRG04::rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign)
{
	// http://jgamble.ripco.net/cgi-bin/nw.cgi?inputs=9&algorithm=batcher&output=text

	AvsFilterRepair16_READ_PIX

	sort_pair (a1, a8);

	sort_pair (a1,  c);
	sort_pair (a2, a5);
	sort_pair (a3, a6);
	sort_pair (a4, a7);
	sort_pair ( c, a8);

	sort_pair (a1, a3);
	sort_pair ( c, a6);
	sort_pair (a2, a4);
	sort_pair (a5, a7);

	sort_pair (a3, a8);

	sort_pair (a3,  c);
	sort_pair (a6, a8);
	sort_pair (a4, a5);

	a2 = _mm_max_epi16 (a1, a2);	// sort_pair (a1, a2);
	a4 = _mm_max_epi16 (a3, a4);	// sort_pair (a3, a4);
	sort_pair ( c, a5);
	a6 = _mm_min_epi16 (a6, a7);	// sort_pair (a6, a7);

	sort_pair (a2, a8);

	c  = _mm_max_epi16 (a2,  c);	// sort_pair (a2,  c);
	sort_pair (a4, a6);
	a5 = _mm_min_epi16 (a5, a8);	// sort_pair (a5, a8);

											// sort_pair (a2, a3);
	a4 = _mm_min_epi16 (a4,  c);	// sort_pair (a4,  c);
	a5 = _mm_min_epi16 (a5, a6);	// sort_pair (a5, a6);
											// sort_pair (a7, a8);

	return (fstb::ToolsSse2::limit_epi16 (cr, a4, a5));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 5

int	AvsFilterRepair16::OpRG05::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	AvsFilterRepair16_SORT_AXIS2_CPP

	const int      c1 = std::min (std::abs (cr - fstb::limit (cr, mi1, ma1)), 0xFFFF);
	const int      c2 = std::min (std::abs (cr - fstb::limit (cr, mi2, ma2)), 0xFFFF);
	const int      c3 = std::min (std::abs (cr - fstb::limit (cr, mi3, ma3)), 0xFFFF);
	const int      c4 = std::min (std::abs (cr - fstb::limit (cr, mi4, ma4)), 0xFFFF);

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	if (mindiff == c4)
	{
		return (fstb::limit (cr, mi4, ma4));
	}
	else if (mindiff == c2)
	{
		return (fstb::limit (cr, mi2, ma2));
	}
	else if (mindiff == c3)
	{
		return (fstb::limit (cr, mi3, ma3));
	}
	return (fstb::limit (cr, mi1, ma1));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 6

int	AvsFilterRepair16::OpRG06::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	AvsFilterRepair16_SORT_AXIS2_CPP

	const int      d1   = ma1 - mi1;
	const int      d2   = ma2 - mi2;
	const int      d3   = ma3 - mi3;
	const int      d4   = ma4 - mi4;

	const int      c1   = fstb::limit ((std::abs (cr - fstb::limit (cr, mi1, ma1)) << 1) + d1, 0, 0xFFFF);
	const int      c2   = fstb::limit ((std::abs (cr - fstb::limit (cr, mi2, ma2)) << 1) + d2, 0, 0xFFFF);
	const int      c3   = fstb::limit ((std::abs (cr - fstb::limit (cr, mi3, ma3)) << 1) + d3, 0, 0xFFFF);
	const int      c4   = fstb::limit ((std::abs (cr - fstb::limit (cr, mi4, ma4)) << 1) + d4, 0, 0xFFFF);

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	if (mindiff == c4)
	{
		return (fstb::limit (cr, mi4, ma4));
	}
	else if (mindiff == c2)
	{
		return (fstb::limit (cr, mi2, ma2));
	}
	else if (mindiff == c3)
	{
		return (fstb::limit (cr, mi3, ma3));
	}
	return (fstb::limit (cr, mi1, ma1));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 7

int	AvsFilterRepair16::OpRG07::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	AvsFilterRepair16_SORT_AXIS2_CPP

	const int      d1   = ma1 - mi1;
	const int      d2   = ma2 - mi2;
	const int      d3   = ma3 - mi3;
	const int      d4   = ma4 - mi4;

	const int      clp1 = fstb::limit (cr, mi1, ma1);
	const int      clp2 = fstb::limit (cr, mi2, ma2);
	const int      clp3 = fstb::limit (cr, mi3, ma3);
	const int      clp4 = fstb::limit (cr, mi4, ma4);

	const int      c1   = std::abs (cr - clp1) + d1;
	const int      c2   = std::abs (cr - clp2) + d2;
	const int      c3   = std::abs (cr - clp3) + d3;
	const int      c4   = std::abs (cr - clp4) + d4;

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	if (mindiff == c4)
	{
		return (clp4);
	}
	else if (mindiff == c2)
	{
		return (clp2);
	}
	else if (mindiff == c3)
	{
		return (clp3);
	}
	return (clp1);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 8

int	AvsFilterRepair16::OpRG08::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	AvsFilterRepair16_SORT_AXIS2_CPP

	const int      d1   = ma1 - mi1;
	const int      d2   = ma2 - mi2;
	const int      d3   = ma3 - mi3;
	const int      d4   = ma4 - mi4;

	const int      clp1 = fstb::limit (cr, mi1, ma1);
	const int      clp2 = fstb::limit (cr, mi2, ma2);
	const int      clp3 = fstb::limit (cr, mi3, ma3);
	const int      clp4 = fstb::limit (cr, mi4, ma4);

	const int      c1   = fstb::limit (std::abs (cr - clp1) + (d1 << 1), 0, 0xFFFF);
	const int      c2   = fstb::limit (std::abs (cr - clp2) + (d2 << 1), 0, 0xFFFF);
	const int      c3   = fstb::limit (std::abs (cr - clp3) + (d3 << 1), 0, 0xFFFF);
	const int      c4   = fstb::limit (std::abs (cr - clp4) + (d4 << 1), 0, 0xFFFF);

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	if (mindiff == c4)
	{
		return (clp4);
	}
	else if (mindiff == c2)
	{
		return (clp2);
	}
	else if (mindiff == c3)
	{
		return (clp3);
	}
	return (clp1);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 9

int	AvsFilterRepair16::OpRG09::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	AvsFilterRepair16_SORT_AXIS2_CPP

	const int      d1   = ma1 - mi1;
	const int      d2   = ma2 - mi2;
	const int      d3   = ma3 - mi3;
	const int      d4   = ma4 - mi4;

	const int      mindiff = std::min (std::min (d1, d2), std::min (d3, d4));

	if (mindiff == d4)
	{
		return (fstb::limit (cr, mi4, ma4));
	}
	else if (mindiff == d2)
	{
		return (fstb::limit (cr, mi2, ma2));
	}
	else if (mindiff == d3)
	{
		return (fstb::limit (cr, mi3, ma3));
	}
	return (fstb::limit (cr, mi1, ma1));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 10

int	AvsFilterRepair16::OpRG10::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	const int      d1 = std::abs (cr - a1);
	const int      d2 = std::abs (cr - a2);
	const int      d3 = std::abs (cr - a3);
	const int      d4 = std::abs (cr - a4);
	const int      d5 = std::abs (cr - a5);
	const int      d6 = std::abs (cr - a6);
	const int      d7 = std::abs (cr - a7);
	const int      d8 = std::abs (cr - a8);
	const int      dc = std::abs (cr - c );

	const int      mindiff = std::min (std::min (
		std::min (std::min (d1, d2), std::min (d3, d4)),
		std::min (std::min (d5, d6), std::min (d7, d8))
	), dc);

	if (mindiff == d7) { return (a7); }
	if (mindiff == d8) { return (a8); }
	if (mindiff == d6) { return (a6); }
	if (mindiff == d2) { return (a2); }
	if (mindiff == d3) { return (a3); }
	if (mindiff == d1) { return (a1); }
	if (mindiff == d5) { return (a5); }
	if (mindiff == dc) { return (c ); }
	return (a4);
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 12

int	AvsFilterRepair16::OpRG12::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	int				a [10] = { a1, a2, a3, a4, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [0]) + 8);
	const int		mi = std::min (a [2-1], c);
	const int		ma = std::max (a [7-1], c);

	return (fstb::limit (cr, mi, ma));
}

__m128i	AvsFilterRepair16::OpRG12::rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign)
{
	AvsFilterRepair16_READ_PIX

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

	const __m128i	mi = _mm_min_epi16 (c, a2);
	const __m128i	ma = _mm_max_epi16 (c, a7);

	return (fstb::ToolsSse2::limit_epi16 (cr, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 13

int	AvsFilterRepair16::OpRG13::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	int				a [10] = { a1, a2, a3, a4, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [0]) + 8);
	const int		mi = std::min (a [3-1], c);
	const int		ma = std::max (a [6-1], c);

	return (fstb::limit (cr, mi, ma));
}

__m128i	AvsFilterRepair16::OpRG13::rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign)
{
	AvsFilterRepair16_READ_PIX

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

	const __m128i	mi = _mm_min_epi16 (c, a3);
	const __m128i	ma = _mm_max_epi16 (c, a6);

	return (fstb::ToolsSse2::limit_epi16 (cr, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 14

int	AvsFilterRepair16::OpRG14::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	int				a [10] = { a1, a2, a3, a4, a5, a6, a7, a8 };

	std::sort (&a [0], (&a [0]) + 8);
	const int		mi = std::min (a [4-1], c);
	const int		ma = std::max (a [5-1], c);

	return (fstb::limit (cr, mi, ma));
}

__m128i	AvsFilterRepair16::OpRG14::rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign)
{
	AvsFilterRepair16_READ_PIX

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

	const __m128i	mi = _mm_min_epi16 (c, a4);
	const __m128i	ma = _mm_max_epi16 (c, a5);

	return (fstb::ToolsSse2::limit_epi16 (cr, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 15

int	AvsFilterRepair16::OpRG15::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	AvsFilterRepair16_SORT_AXIS_CPP

	const int      c1 = std::abs (c - fstb::limit (c, mi1, ma1));
	const int      c2 = std::abs (c - fstb::limit (c, mi2, ma2));
	const int      c3 = std::abs (c - fstb::limit (c, mi3, ma3));
	const int      c4 = std::abs (c - fstb::limit (c, mi4, ma4));

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	int            mi;
	int            ma;
	if (mindiff == c4)
	{
		mi = mi4;
		ma = ma4;
	}
	else if (mindiff == c2)
	{
		mi = mi2;
		ma = ma2;
	}
	else if (mindiff == c3)
	{
		mi = mi3;
		ma = ma3;
	}
	else
	{
		mi = mi1;
		ma = ma1;
	}

	mi = std::min (mi, c);
	ma = std::max (ma, c);

	return (fstb::limit (cr, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 16

int	AvsFilterRepair16::OpRG16::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	AvsFilterRepair16_SORT_AXIS_CPP

	const int      d1   = ma1 - mi1;
	const int      d2   = ma2 - mi2;
	const int      d3   = ma3 - mi3;
	const int      d4   = ma4 - mi4;

	const int      c1   = fstb::limit ((std::abs (c - fstb::limit (c, mi1, ma1)) << 1) + d1, 0, 0xFFFF);
	const int      c2   = fstb::limit ((std::abs (c - fstb::limit (c, mi2, ma2)) << 1) + d2, 0, 0xFFFF);
	const int      c3   = fstb::limit ((std::abs (c - fstb::limit (c, mi3, ma3)) << 1) + d3, 0, 0xFFFF);
	const int      c4   = fstb::limit ((std::abs (c - fstb::limit (c, mi4, ma4)) << 1) + d4, 0, 0xFFFF);

	const int      mindiff = std::min (std::min (c1, c2), std::min (c3, c4));

	int            mi;
	int            ma;
	if (mindiff == c4)
	{
		mi = mi4;
		ma = ma4;
	}
	else if (mindiff == c2)
	{
		mi = mi2;
		ma = ma2;
	}
	else if (mindiff == c3)
	{
		mi = mi3;
		ma = ma3;
	}
	else
	{
		mi = mi1;
		ma = ma1;
	}

	mi = std::min (mi, c);
	ma = std::max (ma, c);

	return (fstb::limit (cr, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 17

int	AvsFilterRepair16::OpRG17::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	AvsFilterRepair16_SORT_AXIS_CPP

	const int      l = std::max (std::max (mi1, mi2), std::max (mi3, mi4));
	const int      u = std::min (std::min (ma1, ma2), std::min (ma3, ma4));

	const int      mi = std::min (std::min (l, u), c);
	const int      ma = std::max (std::max (l, u), c);

	return (fstb::limit (cr, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// Mode 18

int	AvsFilterRepair16::OpRG18::rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8)
{
	const int      d1 = std::max (std::abs (c - a1), std::abs (c - a8));
	const int      d2 = std::max (std::abs (c - a2), std::abs (c - a7));
	const int      d3 = std::max (std::abs (c - a3), std::abs (c - a6));
	const int      d4 = std::max (std::abs (c - a4), std::abs (c - a5));

	const int      mindiff = std::min (std::min (d1, d2), std::min(d3, d4));

	int            mi;
	int            ma;
	if (mindiff == d4)
	{
		mi = std::min (a4, a5);
		ma = std::max (a4, a5);
	}
	else if (mindiff == d2)
	{
		mi = std::min (a2, a7);
		ma = std::max (a2, a7);
	}
	else if (mindiff == d3)
	{
		mi = std::min (a3, a6);
		ma = std::max (a3, a6);
	}
	else
	{
		mi = std::min (a1, a8);
		ma = std::max (a1, a8);
	}

	mi = std::min (mi, c);
	ma = std::max (ma, c);

	return (fstb::limit (cr, mi, ma));
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

#undef AvsFilterRepair16_READ_PIX
#undef AvsFilterRepair16_SORT_AXIS_CPP



void	AvsFilterRepair16::process_subplane (Slicer::TaskData &td)
{
	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);

	(*(_process_subplane_ptr_arr [tdg._plane_index])) (td);
}



template <class OP>
void	AvsFilterRepair16::PlaneProc <OP>::process_subplane_cpp (Slicer::TaskData &td)
{
	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);

	const int		y_b = std::max (td._y_beg, 1);
	const int		y_e = std::min (td._y_end, tdg._h - 1);

	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + y_b * tdg._stride_dst;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + y_b * tdg._stride_dst;
	const uint8_t*	sr1_msb_ptr = tdg._sr1_msb_ptr + y_b * tdg._stride_sr1;
	const uint8_t*	sr1_lsb_ptr = tdg._sr1_lsb_ptr + y_b * tdg._stride_sr1;
	const uint8_t*	sr2_msb_ptr = tdg._sr2_msb_ptr + y_b * tdg._stride_sr2;
	const uint8_t*	sr2_lsb_ptr = tdg._sr2_lsb_ptr + y_b * tdg._stride_sr2;

	const int		x_e = tdg._w - 1;

	for (int y = y_b; y < y_e; ++y)
	{
		dst_msb_ptr [0] = sr1_msb_ptr [0];
		dst_lsb_ptr [0] = sr1_lsb_ptr [0];

		process_row_cpp (
			dst_msb_ptr, dst_lsb_ptr,
			sr1_msb_ptr, sr1_lsb_ptr,
			sr2_msb_ptr, sr2_lsb_ptr,
			tdg._stride_sr2,
			1,
			x_e
		);

		dst_msb_ptr [x_e] = sr1_msb_ptr [x_e];
		dst_lsb_ptr [x_e] = sr1_lsb_ptr [x_e];

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		sr1_msb_ptr += tdg._stride_sr1;
		sr1_lsb_ptr += tdg._stride_sr1;
		sr2_msb_ptr += tdg._stride_sr2;
		sr2_lsb_ptr += tdg._stride_sr2;
	}
}



template <class OP>
void	AvsFilterRepair16::PlaneProc <OP>::process_row_cpp (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, int x_beg, int x_end)
{
	const int      om = stride_sr2 - 1;
	const int      o0 = stride_sr2    ;
	const int      op = stride_sr2 + 1;

	sr1_msb_ptr += x_beg;
	sr1_lsb_ptr += x_beg;
	sr2_msb_ptr += x_beg;
	sr2_lsb_ptr += x_beg;

	for (int x = x_beg; x < x_end; ++x)
	{
		const int		cr = (sr1_msb_ptr [ 0 ] << 8) + sr1_lsb_ptr [ 0 ];
		const int		a1 = (sr2_msb_ptr [-op] << 8) + sr2_lsb_ptr [-op];
		const int		a2 = (sr2_msb_ptr [-o0] << 8) + sr2_lsb_ptr [-o0];
		const int		a3 = (sr2_msb_ptr [-om] << 8) + sr2_lsb_ptr [-om];
		const int		a4 = (sr2_msb_ptr [-1 ] << 8) + sr2_lsb_ptr [-1 ];
		const int		c  = (sr2_msb_ptr [ 0 ] << 8) + sr2_lsb_ptr [ 0 ];
		const int		a5 = (sr2_msb_ptr [ 1 ] << 8) + sr2_lsb_ptr [ 1 ];
		const int		a6 = (sr2_msb_ptr [ om] << 8) + sr2_lsb_ptr [ om];
		const int		a7 = (sr2_msb_ptr [ o0] << 8) + sr2_lsb_ptr [ o0];
		const int		a8 = (sr2_msb_ptr [ op] << 8) + sr2_lsb_ptr [ op];

		const int		res = OP::rg (cr, a1, a2, a3, a4, c, a5, a6, a7, a8);

		dst_msb_ptr [x] = uint8_t (res >> 8 );
		dst_lsb_ptr [x] = uint8_t (res & 255);

		++ sr1_msb_ptr;
		++ sr1_lsb_ptr;
		++ sr2_msb_ptr;
		++ sr2_lsb_ptr;
	}
}



template <class OP>
void	AvsFilterRepair16::PlaneProc <OP>::process_subplane_sse2 (Slicer::TaskData &td)
{
	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);

	const int		y_b = std::max (td._y_beg, 1);
	const int		y_e = std::min (td._y_end, tdg._h - 1);

	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + y_b * tdg._stride_dst;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + y_b * tdg._stride_dst;
	const uint8_t*	sr1_msb_ptr = tdg._sr1_msb_ptr + y_b * tdg._stride_sr1;
	const uint8_t*	sr1_lsb_ptr = tdg._sr1_lsb_ptr + y_b * tdg._stride_sr1;
	const uint8_t*	sr2_msb_ptr = tdg._sr2_msb_ptr + y_b * tdg._stride_sr2;
	const uint8_t*	sr2_lsb_ptr = tdg._sr2_lsb_ptr + y_b * tdg._stride_sr2;

	const __m128i	mask_lsb  = _mm_set1_epi16 ( 0x00FF);
	const __m128i	mask_sign = _mm_set1_epi16 (-0x8000);

	const int		x_e =   tdg._w - 1;
	const int		w8  = ((tdg._w - 2) & -8) + 1;
	const int		w7  = x_e - w8;

	for (int y = y_b; y < y_e; ++y)
	{
		dst_msb_ptr [0] = sr1_msb_ptr [0];
		dst_lsb_ptr [0] = sr1_lsb_ptr [0];

		for (int x = 1; x < w8; x += 8)
		{
			__m128i			res = OP::rg (
				sr1_msb_ptr + x, sr1_lsb_ptr + x,
				sr2_msb_ptr + x, sr2_lsb_ptr + x,
				tdg._stride_sr2,
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
			sr1_msb_ptr, sr1_lsb_ptr,
			sr2_msb_ptr, sr2_lsb_ptr,
			tdg._stride_sr2,
			w8,
			x_e
		);

		dst_msb_ptr [x_e] = sr1_msb_ptr [x_e];
		dst_lsb_ptr [x_e] = sr1_lsb_ptr [x_e];

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		sr1_msb_ptr += tdg._stride_sr1;
		sr1_lsb_ptr += tdg._stride_sr1;
		sr2_msb_ptr += tdg._stride_sr2;
		sr2_lsb_ptr += tdg._stride_sr2;
	}
}



PlaneProcMode	AvsFilterRepair16::conv_rgmode_to_proc (int rgmode)
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



void	AvsFilterRepair16::sort_pair (__m128i &a1, __m128i &a2)
{
	const __m128i	tmp = _mm_min_epi16 (a1, a2);
	a2 = _mm_max_epi16 (a1, a2);
	a1 = tmp;
}



void	AvsFilterRepair16::sort_pair (__m128i &mi, __m128i &ma, __m128i a1, __m128i a2)
{
	mi = _mm_min_epi16 (a1, a2);
	ma = _mm_max_epi16 (a1, a2);
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
