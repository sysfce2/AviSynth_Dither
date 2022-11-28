/*****************************************************************************

        AvsFilterAdd16.cpp
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
#include	"fstb/fnc.h"
#include	"fstb/ToolsSse2.h"
#include	"AvsFilterAdd16.h"

#include	<algorithm>

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterAdd16::AvsFilterAdd16 (::IScriptEnvironment *env_ptr, ::PClip stack1_clip_sptr, ::PClip stack2_clip_sptr, bool wrap_flag, int y, int u, int v, bool dif_flag, bool sub_flag)
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
,	_wrap_flag (wrap_flag)
,	_dif_flag (dif_flag)
,	_sub_flag (sub_flag)
,	_process_row_ptr (0)
{
	assert (env_ptr != 0);

	const char *	name_0 = (sub_flag) ? "Dither_sub16" : "Dither_add16";

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("%s: input must be planar.", name_0);
	}

	PlaneProcessor::check_same_format (env_ptr, vi, stack2_clip_sptr, name_0, "src2");

	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"%s: stacked MSB/LSB data must have\n"
			"a height multiple of twice the maximum subsampling.",
			name_0
		);
	}

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (2, PlaneProcessor::ClipType_STACKED_16);

	fstb::CpuId		cid;
	const int		flags =
		  ((_wrap_flag    ) ? 8 : 0)
		+ ((_dif_flag     ) ? 4 : 0)
		+ ((_sub_flag     ) ? 2 : 0)
		+ ((cid._sse2_flag) ? 1 : 0);

	_process_row_ptr = &RowProc <OpAddWrap>::process_row_cpp;
	switch (flags)
	{
	case	 0x0:	_process_row_ptr = &RowProc <OpAddSaturate   >::process_row_cpp;	break;
	case	 0x1:	_process_row_ptr = &RowProc <OpAddSaturate   >::process_row_sse2;	break;
	case	 0x2:	_process_row_ptr = &RowProc <OpSubSaturate   >::process_row_cpp;	break;
	case	 0x3:	_process_row_ptr = &RowProc <OpSubSaturate   >::process_row_sse2;	break;

	case	 0x4:	_process_row_ptr = &RowProc <OpAddDifSaturate>::process_row_cpp;	break;
	case	 0x5:	_process_row_ptr = &RowProc <OpAddDifSaturate>::process_row_sse2;	break;
	case	 0x6:	_process_row_ptr = &RowProc <OpSubDifSaturate>::process_row_cpp;	break;
	case	 0x7:	_process_row_ptr = &RowProc <OpSubDifSaturate>::process_row_sse2;	break;

	case	 0x8:	_process_row_ptr = &RowProc <OpAddWrap       >::process_row_cpp;	break;
	case	 0x9:	_process_row_ptr = &RowProc <OpAddWrap       >::process_row_sse2;	break;
	case	 0xA:	_process_row_ptr = &RowProc <OpSubWrap       >::process_row_cpp;	break;
	case	 0xB:	_process_row_ptr = &RowProc <OpSubWrap       >::process_row_sse2;	break;

	case	 0xC:	_process_row_ptr = &RowProc <OpAddDifWrap    >::process_row_cpp;	break;
	case	 0xD:	_process_row_ptr = &RowProc <OpAddDifWrap    >::process_row_sse2;	break;
	case	 0xE:	_process_row_ptr = &RowProc <OpSubDifWrap    >::process_row_cpp;	break;
	case	 0xF:	_process_row_ptr = &RowProc <OpSubDifWrap    >::process_row_sse2;	break;

	default:
		assert (false);
		break;
	}

	if (cid._avx2_flag)
	{
		configure_avx2 ();
	}

	assert (_process_row_ptr != 0);
}



::PVideoFrame __stdcall	AvsFilterAdd16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_stack1_clip_sptr, &_stack2_clip_sptr, 0
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterAdd16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
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
	tdg._dst_msb_ptr = data_msbd_ptr;
	tdg._dst_lsb_ptr = data_msbd_ptr + h * stride_dst;
	tdg._sr1_msb_ptr = data_msb1_ptr;
	tdg._sr1_lsb_ptr = data_msb1_ptr + h * stride_sr1;
	tdg._sr2_msb_ptr = data_msb2_ptr;
	tdg._sr2_lsb_ptr = data_msb2_ptr + h * stride_sr2;
	tdg._stride_dst  = stride_dst;
	tdg._stride_sr1  = stride_sr1;
	tdg._stride_sr2  = stride_sr2;

	Slicer			slicer;
	slicer.start (h, tdg, &AvsFilterAdd16::process_subplane);
	slicer.wait ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	AvsFilterAdd16::OpAddWrap::operator () (int src1, int src2) const
{
	return (src1 + src2);
}

__m128i	AvsFilterAdd16::OpAddWrap::operator () (__m128i src1, __m128i src2, __m128i /*mask_sign*/) const
{
	return (_mm_add_epi16 (src1, src2));
}



int	AvsFilterAdd16::OpAddSaturate::operator () (int src1, int src2) const
{
	return (std::min (src1 + src2, 65535));
}

__m128i	AvsFilterAdd16::OpAddSaturate::operator () (__m128i src1, __m128i src2, __m128i /*mask_sign*/) const
{
	return (_mm_adds_epu16 (src1, src2));
}



int	AvsFilterAdd16::OpAddDifWrap::operator () (int src1, int src2) const
{
	return (src1 + src2 - 32768);
}

__m128i	AvsFilterAdd16::OpAddDifWrap::operator () (__m128i src1, __m128i src2, __m128i mask_sign) const
{
	return (_mm_xor_si128 (_mm_add_epi16 (src1, src2), mask_sign));
}



int	AvsFilterAdd16::OpAddDifSaturate::operator () (int src1, int src2) const
{
	return (fstb::limit (src1 + src2 - 32768, 0, 65535));
}

__m128i	AvsFilterAdd16::OpAddDifSaturate::operator () (__m128i src1, __m128i src2, __m128i mask_sign) const
{
	return (_mm_xor_si128 (
		_mm_adds_epi16 (
			_mm_xor_si128 (src1, mask_sign),
			_mm_xor_si128 (src2, mask_sign)
		),
		mask_sign
	));
}



int	AvsFilterAdd16::OpSubWrap::operator () (int src1, int src2) const
{
	return (src1 - src2);
}

__m128i	AvsFilterAdd16::OpSubWrap::operator () (__m128i src1, __m128i src2, __m128i /*mask_sign*/) const
{
	return (_mm_sub_epi16 (src1, src2));
}



int	AvsFilterAdd16::OpSubSaturate::operator () (int src1, int src2) const
{
	return (std::max (src1 - src2, 0));
}

__m128i	AvsFilterAdd16::OpSubSaturate::operator () (__m128i src1, __m128i src2, __m128i /*mask_sign*/) const
{
	return (_mm_subs_epu16 (src1, src2));
}



int	AvsFilterAdd16::OpSubDifWrap::operator () (int src1, int src2) const
{
	return (src1 - src2 + 32768);
}

__m128i	AvsFilterAdd16::OpSubDifWrap::operator () (__m128i src1, __m128i src2, __m128i mask_sign) const
{
	return (_mm_xor_si128 (_mm_sub_epi16 (src1, src2), mask_sign));
}



int	AvsFilterAdd16::OpSubDifSaturate::operator () (int src1, int src2) const
{
	return (fstb::limit (src1 - src2, -32768, 32767) + 32768);
}

__m128i	AvsFilterAdd16::OpSubDifSaturate::operator () (__m128i src1, __m128i src2, __m128i mask_sign) const
{
	return (_mm_xor_si128 (
		_mm_subs_epi16 (
			_mm_xor_si128 (src1, mask_sign),
			_mm_xor_si128 (src2, mask_sign)
		),
		mask_sign
	));
}



void	AvsFilterAdd16::process_subplane (Slicer::TaskData &td)
{
	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);
	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + td._y_beg * tdg._stride_dst;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + td._y_beg * tdg._stride_dst;
	const uint8_t*	sr1_msb_ptr = tdg._sr1_msb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t*	sr1_lsb_ptr = tdg._sr1_lsb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t*	sr2_msb_ptr = tdg._sr2_msb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t*	sr2_lsb_ptr = tdg._sr2_lsb_ptr + td._y_beg * tdg._stride_sr2;

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		_process_row_ptr (
			dst_msb_ptr, dst_lsb_ptr,
			sr1_msb_ptr, sr1_lsb_ptr,
			sr2_msb_ptr, sr2_lsb_ptr,
			tdg._w
		);

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		sr1_msb_ptr += tdg._stride_sr1;
		sr1_lsb_ptr += tdg._stride_sr1;
		sr2_msb_ptr += tdg._stride_sr2;
		sr2_lsb_ptr += tdg._stride_sr2;
	}
}



template <class OP>
void	AvsFilterAdd16::RowProc <OP>::process_row_cpp (uint8_t *data_msbd_ptr, uint8_t *data_lsbd_ptr, const uint8_t * data_msb1_ptr, const uint8_t * data_lsb1_ptr, const uint8_t *data_msb2_ptr, const uint8_t *data_lsb2_ptr, int w)
{
	OP					op;

	for (int x = 0; x < w; ++x)
	{
		const int		src1 = (data_msb1_ptr [x] << 8) + data_lsb1_ptr [x];
		const int		src2 = (data_msb2_ptr [x] << 8) + data_lsb2_ptr [x];

		const int		res = op (src1, src2);

		data_msbd_ptr [x] = uint8_t (res >> 8 );
		data_lsbd_ptr [x] = uint8_t (res & 255);
	}
}



template <class OP>
void	AvsFilterAdd16::RowProc <OP>::process_row_sse2 (uint8_t *data_msbd_ptr, uint8_t *data_lsbd_ptr, const uint8_t * data_msb1_ptr, const uint8_t * data_lsb1_ptr, const uint8_t *data_msb2_ptr, const uint8_t *data_lsb2_ptr, int w)
{
	OP					op;

	const __m128i	mask_sign = _mm_set1_epi16 (-0x8000);
	const __m128i	mask_lsb  = _mm_set1_epi16 ( 0x00FF);

	const int		w8 = w & -8;
	const int		w7 = w - w8;

	for (int x = 0; x < w8; x += 8)
	{
		const __m128i	src1 =
			fstb::ToolsSse2::load_8_16ml (data_msb1_ptr + x, data_lsb1_ptr + x);
		const __m128i	src2 =
			fstb::ToolsSse2::load_8_16ml (data_msb2_ptr + x, data_lsb2_ptr + x);

		const __m128i	res = op (src1, src2, mask_sign);

		fstb::ToolsSse2::store_8_16ml (
			data_msbd_ptr + x,
			data_lsbd_ptr + x,
			res,
			mask_lsb
		);
	}

	if (w7 > 0)
	{
		process_row_cpp (
			data_msbd_ptr + w8, data_lsbd_ptr + w8,
			data_msb1_ptr + w8, data_lsb1_ptr + w8,
			data_msb2_ptr + w8, data_lsb2_ptr + w8,
			w7
		);
	}
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
