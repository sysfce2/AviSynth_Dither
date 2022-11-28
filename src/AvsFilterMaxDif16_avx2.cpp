/*****************************************************************************

        AvsFilterMaxDif16.cpp
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

#include "fstb/ToolsAvx2.h"
#include "AvsFilterMaxDif16.h"

#include <cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



__m256i	AvsFilterMaxDif16::OpMax::operator () (__m256i d1as, __m256i d2as) const
{
	return (_mm256_cmpgt_epi16 (d2as, d1as));
}



__m256i	AvsFilterMaxDif16::OpMin::operator () (__m256i d1as, __m256i d2as) const
{
	return (_mm256_cmpgt_epi16 (d1as, d2as));
}



void	AvsFilterMaxDif16::configure_avx2 ()
{
	if (_min_flag)
	{
		_process_subplane_ptr = &PlaneProc <OpMin>::process_subplane_avx2;
	}
	else
	{
		_process_subplane_ptr = &PlaneProc <OpMax>::process_subplane_avx2;
	}
}



template <class OP>
void	AvsFilterMaxDif16::PlaneProc <OP>::process_subplane_avx2 (Slicer::TaskData &td)
{
	assert (&td != 0);

	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);
	uint8_t *      dst_msb_ptr = tdg._dst_msb_ptr + td._y_beg * tdg._stride_dst;
	uint8_t *      dst_lsb_ptr = tdg._dst_lsb_ptr + td._y_beg * tdg._stride_dst;
	const uint8_t* sr1_msb_ptr = tdg._sr1_msb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t* sr1_lsb_ptr = tdg._sr1_lsb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t* sr2_msb_ptr = tdg._sr2_msb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t* sr2_lsb_ptr = tdg._sr2_lsb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t* ref_msb_ptr = tdg._ref_msb_ptr + td._y_beg * tdg._stride_ref;
	const uint8_t* ref_lsb_ptr = tdg._ref_lsb_ptr + td._y_beg * tdg._stride_ref;

	const __m256i  mask_lsb    = _mm256_set1_epi16 ( 0x00FF);
	const __m256i  sign        = _mm256_set1_epi16 (-0x8000);

	const OP         op;

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		for (int x = 0; x < tdg._w; x += 16)
		{
			const __m256i  src1 =
				fstb::ToolsAvx2::load_16_16ml (&sr1_msb_ptr [x], &sr1_lsb_ptr [x]);
			const __m256i  src2 =
				fstb::ToolsAvx2::load_16_16ml (&sr2_msb_ptr [x], &sr2_lsb_ptr [x]);
			const __m256i  ref  =
				fstb::ToolsAvx2::load_16_16ml (&ref_msb_ptr [x], &ref_lsb_ptr [x]);

			const __m256i  d11  = _mm256_subs_epu16 (src1, ref);
			const __m256i  d12  = _mm256_subs_epu16 (ref, src1);
			const __m256i  d1a  = _mm256_or_si256 (d11, d12);
			const __m256i  d21  = _mm256_subs_epu16 (src2, ref);
			const __m256i  d22  = _mm256_subs_epu16 (ref, src2);
			const __m256i  d2a  = _mm256_or_si256 (d21, d22);
			const __m256i  d1as = _mm256_xor_si256 (d1a, sign);
			const __m256i  d2as = _mm256_xor_si256 (d2a, sign);
			const __m256i  mask = op (d1as, d2as);
			const __m256i  val  = _mm256_or_si256 (
				_mm256_and_si256 (   mask, src2),
				_mm256_andnot_si256 (mask, src1)
			);

	      fstb::ToolsAvx2::store_16_16ml (
				&dst_msb_ptr [x], &dst_lsb_ptr [x], val, mask_lsb
			);
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		sr1_msb_ptr += tdg._stride_sr1;
		sr1_lsb_ptr += tdg._stride_sr1;
		sr2_msb_ptr += tdg._stride_sr2;
		sr2_lsb_ptr += tdg._stride_sr2;
		ref_msb_ptr += tdg._stride_ref;
		ref_lsb_ptr += tdg._stride_ref;
	}

	_mm256_zeroupper ();	// Back to SSE state
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
