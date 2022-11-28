/*****************************************************************************

        AvsFilterLimitDif16.cpp
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
#include	"AvsFilterLimitDif16.h"
#include	"DiffSoftClipper.h"

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterLimitDif16::process_segment_avx2 (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *flt_msb_ptr, const uint8_t *flt_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w) const
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (flt_msb_ptr != 0);
	assert (flt_lsb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (ref_msb_ptr != 0);
	assert (ref_lsb_ptr != 0);
	assert (w > 0);

	const __m256i	thr_1      = _mm256_set1_epi16 (int16_t (_thr_1));
	const __m256i	thr_2      = _mm256_set1_epi16 (int16_t (_thr_2));
	const __m256i	thr_slope  = _mm256_set1_epi16 (int16_t (_thr_slope));
	const __m256i	thr_offset = _mm256_set1_epi16 (int16_t (_thr_offset));
	const __m256i	mask_lsb   = _mm256_set1_epi16 (0x00FF);

	if (_refabsdif_flag)
	{
		for (int x = 0; x < w; x += 16)
		{
			const __m256i	val_flt =
				fstb::ToolsAvx2::load_16_16ml (flt_msb_ptr + x, flt_lsb_ptr + x);
			const __m256i	val_src =
				fstb::ToolsAvx2::load_16_16ml (src_msb_ptr + x, src_lsb_ptr + x);
			const __m256i	dif_ref =
				fstb::ToolsAvx2::load_16_16ml (ref_msb_ptr + x, ref_lsb_ptr + x);

			const __m256i	val_dst = DiffSoftClipper::clip_dif_avx2 (
				val_flt, val_src, dif_ref,
				thr_1, thr_2, thr_slope, thr_offset
			);

			fstb::ToolsAvx2::store_16_16ml (
				dst_msb_ptr + x, dst_lsb_ptr + x,
				val_dst, mask_lsb
			);
		}
	}
	else
	{
		for (int x = 0; x < w; x += 16)
		{
			const __m256i	val_flt =
				fstb::ToolsAvx2::load_16_16ml (flt_msb_ptr + x, flt_lsb_ptr + x);
			const __m256i	val_src =
				fstb::ToolsAvx2::load_16_16ml (src_msb_ptr + x, src_lsb_ptr + x);
			const __m256i	val_ref =
				fstb::ToolsAvx2::load_16_16ml (ref_msb_ptr + x, ref_lsb_ptr + x);

			const __m256i	val_dst = DiffSoftClipper::clip_avx2 (
				val_flt, val_src, val_ref,
				thr_1, thr_2, thr_slope, thr_offset
			);

			fstb::ToolsAvx2::store_16_16ml (
				dst_msb_ptr + x, dst_lsb_ptr + x,
				val_dst, mask_lsb
			);
		}
	}

	_mm256_zeroupper ();	// Back to SSE state
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
