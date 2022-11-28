/*****************************************************************************

        AvsFilterAdd16.cpp
        Author: Laurent de Soras, 2015

To be compiled with /arch:AVX2 in order to avoid SSE/AVX state switch
slowdown.

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

#include	"fstb/ToolsAvx2.h"
#include	"AvsFilterAdd16.h"

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterAdd16::configure_avx2 ()
{
	const int		flags =
		  ((_wrap_flag    ) ? 4 : 0)
		+ ((_dif_flag     ) ? 2 : 0)
		+ ((_sub_flag     ) ? 1 : 0);

	switch (flags)
	{
	case	 0x0:	_process_row_ptr = &RowProc <OpAddSaturate   >::process_row_avx2;	break;
	case	 0x1:	_process_row_ptr = &RowProc <OpSubSaturate   >::process_row_avx2;	break;
	case	 0x2:	_process_row_ptr = &RowProc <OpAddDifSaturate>::process_row_avx2;	break;
	case	 0x3:	_process_row_ptr = &RowProc <OpSubDifSaturate>::process_row_avx2;	break;
	case	 0x4:	_process_row_ptr = &RowProc <OpAddWrap       >::process_row_avx2;	break;
	case	 0x5:	_process_row_ptr = &RowProc <OpSubWrap       >::process_row_avx2;	break;
	case	 0x6:	_process_row_ptr = &RowProc <OpAddDifWrap    >::process_row_avx2;	break;
	case	 0x7:	_process_row_ptr = &RowProc <OpSubDifWrap    >::process_row_avx2;	break;
	default:
		assert (false);
		break;
	}

}



__m256i	AvsFilterAdd16::OpAddWrap::operator () (__m256i src1, __m256i src2, __m256i /*mask_sign*/) const
{
	return (_mm256_add_epi16 (src1, src2));
}



__m256i	AvsFilterAdd16::OpAddSaturate::operator () (__m256i src1, __m256i src2, __m256i /*mask_sign*/) const
{
	return (_mm256_adds_epu16 (src1, src2));
}



__m256i	AvsFilterAdd16::OpAddDifWrap::operator () (__m256i src1, __m256i src2, __m256i mask_sign) const
{
	return (_mm256_xor_si256 (_mm256_add_epi16 (src1, src2), mask_sign));
}



__m256i	AvsFilterAdd16::OpAddDifSaturate::operator () (__m256i src1, __m256i src2, __m256i mask_sign) const
{
	return (_mm256_xor_si256 (
		_mm256_adds_epi16 (
			_mm256_xor_si256 (src1, mask_sign),
			_mm256_xor_si256 (src2, mask_sign)
		),
		mask_sign
	));
}



__m256i	AvsFilterAdd16::OpSubWrap::operator () (__m256i src1, __m256i src2, __m256i /*mask_sign*/) const
{
	return (_mm256_sub_epi16 (src1, src2));
}



__m256i	AvsFilterAdd16::OpSubSaturate::operator () (__m256i src1, __m256i src2, __m256i /*mask_sign*/) const
{
	return (_mm256_subs_epu16 (src1, src2));
}



__m256i	AvsFilterAdd16::OpSubDifWrap::operator () (__m256i src1, __m256i src2, __m256i mask_sign) const
{
	return (_mm256_xor_si256 (_mm256_sub_epi16 (src1, src2), mask_sign));
}



__m256i	AvsFilterAdd16::OpSubDifSaturate::operator () (__m256i src1, __m256i src2, __m256i mask_sign) const
{
	return (_mm256_xor_si256 (
		_mm256_subs_epi16 (
			_mm256_xor_si256 (src1, mask_sign),
			_mm256_xor_si256 (src2, mask_sign)
		),
		mask_sign
	));
}




template <class OP>
void	AvsFilterAdd16::RowProc <OP>::process_row_avx2 (uint8_t *data_msbd_ptr, uint8_t *data_lsbd_ptr, const uint8_t * data_msb1_ptr, const uint8_t * data_lsb1_ptr, const uint8_t *data_msb2_ptr, const uint8_t *data_lsb2_ptr, int w)
{
	OP             op;

	const __m256i  mask_sign = _mm256_set1_epi16 (-0x8000);
	const __m256i  mask_lsb  = _mm256_set1_epi16 ( 0x00FF);

	const int      w16 = w & -16;
	const int      w15 = w - w16;

	for (int x = 0; x < w16; x += 16)
	{
		const __m256i	src1 =
			fstb::ToolsAvx2::load_16_16ml (data_msb1_ptr + x, data_lsb1_ptr + x);
		const __m256i	src2 =
			fstb::ToolsAvx2::load_16_16ml (data_msb2_ptr + x, data_lsb2_ptr + x);

		const __m256i	res = op (src1, src2, mask_sign);

		fstb::ToolsAvx2::store_16_16ml (
			data_msbd_ptr + x,
			data_lsbd_ptr + x,
			res,
			mask_lsb
		);
	}

	_mm256_zeroupper ();	// Back to SSE state

	if (w15 > 0)
	{
		process_row_cpp (
			data_msbd_ptr + w16, data_lsbd_ptr + w16,
			data_msb1_ptr + w16, data_lsb1_ptr + w16,
			data_msb2_ptr + w16, data_lsb2_ptr + w16,
			w15
		);
	}
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
