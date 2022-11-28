/*****************************************************************************

        AvsFilterMedian16.cpp
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
#include	"AvsFilterMedian16.h"

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



static void	sort_pair (__m256i &a1, __m256i &a2)
{
	const __m256i	tmp = _mm256_min_epi16 (a1, a2);
	a2 = _mm256_max_epi16 (a1, a2);
	a1 = tmp;
}



void	AvsFilterMedian16::process_subplane_rt0_avx2 (Slicer::TaskData &td)
{
	assert (_rt == 0);
	assert (_rx == 1 && _ry == 1 && _ql == 4 && _qh == 4);

	const TaskDataGlobal & tdg = *(td._glob_data_ptr);
	uint8_t *      dst_msb_ptr = tdg._dst_msb_ptr          + td._y_beg * tdg._stride_dst;
	uint8_t *      dst_lsb_ptr = tdg._dst_lsb_ptr          + td._y_beg * tdg._stride_dst;
	const int      stride_src  = tdg._src_arr [0]._stride;
	const uint8_t* src_msb_ptr = tdg._src_arr [0]._msb_ptr + td._y_beg * stride_src;
	const uint8_t* src_lsb_ptr = tdg._src_arr [0]._lsb_ptr + td._y_beg * stride_src;
	const int      w           = tdg._w;
	const int      h           = tdg._h;
	const int      x_last      = w - _rx;
	const int      range_x     = _rx * 2 + 1;
	const int      range_y     = _ry * 2 + 1;
	const int		area        = range_x * range_y;
	const int		mid_pos     = (area - 1) / 2;
	const int		w16         = (x_last - _rx) & -16;
	const int		x_first     = _rx + w16;

	uint16_t       data [MAX_AREA_SIZE];

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		if (y < _ry || y >= h - _ry)
		{
			memcpy (dst_msb_ptr, src_msb_ptr, w);
			memcpy (dst_lsb_ptr, src_lsb_ptr, w);
		}

		else
		{
			for (int x = 0; x < _rx; ++x)
			{
				dst_msb_ptr [x] = src_msb_ptr [x];
				dst_lsb_ptr [x] = src_lsb_ptr [x];
			}

			// Quick and dirty optimization for 3x3 median filter
			const __m256i  mask_sign = _mm256_set1_epi16 (-0x8000);
			const __m256i  mask_lsb  = _mm256_set1_epi16 (0x00FF);

			for (int x = _rx; x < x_first; x += 16)
			{
				const uint8_t* src2_msb_ptr = src_msb_ptr + x;
				const uint8_t* src2_lsb_ptr = src_lsb_ptr + x;

				// Code from AvsFilterRemoveGrain16
				const int      om = stride_src - 1;
				const int      o0 = stride_src    ;
				const int      op = stride_src + 1;
				__m256i        a1 = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr - op, src2_lsb_ptr - op), mask_sign);
				__m256i        a2 = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr - o0, src2_lsb_ptr - o0), mask_sign);
				__m256i        a3 = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr - om, src2_lsb_ptr - om), mask_sign);
				__m256i        a4 = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr - 1 , src2_lsb_ptr - 1 ), mask_sign);
				__m256i        c  = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr     , src2_lsb_ptr     ), mask_sign);
				__m256i        a5 = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr + 1 , src2_lsb_ptr + 1 ), mask_sign);
				__m256i        a6 = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr + om, src2_lsb_ptr + om), mask_sign);
				__m256i        a7 = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr + o0, src2_lsb_ptr + o0), mask_sign);
				__m256i        a8 = _mm256_xor_si256 (fstb::ToolsAvx2::load_16_16ml (src2_msb_ptr + op, src2_lsb_ptr + op), mask_sign);

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

				a5 = _mm256_max_epi16 (a1, a5);	// sort_pair (a1, a5);
				a6 = _mm256_max_epi16 (a2, a6);	// sort_pair (a2, a6);
				a3 = _mm256_min_epi16 (a3, a7);	// sort_pair (a3, a7);
				a4 = _mm256_min_epi16 (a4, a8);	// sort_pair (a4, a8);

				a5 = _mm256_max_epi16 (a3, a5);	// sort_pair (a3, a5);
				a4 = _mm256_min_epi16 (a4, a6);	// sort_pair (a4, a6);

					                              // sort_pair (a2, a3);
				sort_pair (a4, a5);
					                              // sort_pair (a6, a7);

				__m256i			res = fstb::ToolsAvx2::limit_epi16 (c, a4, a5);
				res = _mm256_xor_si256 (res, mask_sign);
				fstb::ToolsAvx2::store_16_16ml (
					dst_msb_ptr + x,
					dst_lsb_ptr + x,
					res,
					mask_lsb
				);
			}

			for (int x = x_first; x < x_last; ++x)
			{
				// Collects data
				int            pos = 0;
				const int      offset = x - stride_src * _ry - _rx;
				const uint8_t*	sm_ptr = src_msb_ptr + offset;
				const uint8_t*	sl_ptr = src_lsb_ptr + offset;
				for (int y2 = 0; y2 < range_y; ++y2)
				{
					for (int x2 = 0; x2 < range_x; ++x2)
					{
						data [pos] = (sm_ptr [x2] << 8) + sl_ptr [x2];
						++ pos;
					}
					sm_ptr += stride_src;
					sl_ptr += stride_src;
				}

				int				pix = data [mid_pos];

				// Sort
				std::sort (data, data + area);

				// Clipping
				const int		pix_l = data [_ql];
				const int		pix_h = data [_qh];
				pix = std::min (std::max (pix, pix_l), pix_h);

				dst_msb_ptr [x] = uint8_t (pix >> 8);
				dst_lsb_ptr [x] = uint8_t (pix     );
			}

			for (int x = x_last; x < w; ++x)
			{
				dst_msb_ptr [x] = src_msb_ptr [x];
				dst_lsb_ptr [x] = src_lsb_ptr [x];
			}
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		src_msb_ptr += stride_src;
		src_lsb_ptr += stride_src;
	}

	_mm256_zeroupper ();	// Back to SSE state
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
