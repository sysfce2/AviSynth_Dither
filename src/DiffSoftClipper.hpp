/*****************************************************************************

        DiffSoftClipper.hpp
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (DiffSoftClipper_CODEHEADER_INCLUDED)
#define	DiffSoftClipper_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"fstb/fnc.h"

#include	<algorithm>

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	DiffSoftClipper::init_cst (int &thr_1, int &thr_2, int &thr_slope, int &thr_offset, double threshold, double elast)
{
	const double	th2 = fstb::limit (threshold, 0.1, 10.0);
	const double	el2 = fstb::limit (elast, 1.01, 10.0);
	thr_1 = int (th2       * 128.0 + 0.5);
	thr_2 = int (th2 * el2 * 128.0 + 0.5);

	// Problems could happen with these combinations:
	// - low threshold and low elast
	// - high threshold and high elast
	// We need to keep thr_1 and thr_2 distinct, and keep them in the positive
	// range of signed int16.
	const int		min_thr_dif = 2;
	const int		thr_dif     = std::max (thr_2 - thr_1, min_thr_dif);
	thr_1 = fstb::limit (thr_1, 0, 32767 - min_thr_dif);
	thr_2 = std::min (thr_1 + thr_dif, 32767);

	thr_slope = 32768 / (thr_2 - thr_1);
	thr_offset = thr_1 * thr_slope + 32768;	// Can be truncated to 16 bits
}



int	DiffSoftClipper::clip_cpp (int val_filt, int val_orig, int val_ref, int thr_1, int thr_2, int thr_slope, int thr_offset)
{
	// Previous formula: modif = dif * (1 - min (abs (dif) / threshold, 1) ^ 4)

	const int		dif = (val_filt - val_orig) >> 1;		// 9.7 bits, signed
	const int		dif_ref = (val_filt - val_ref) >> 1;	// 9.7 bits, signed
	const int		dif_abs = (dif_ref < 0) ? -dif_ref : dif_ref;	// 8.7 bits, unsigned
	int				val_final;
	if (dif_abs < thr_1)
	{
		val_final = val_filt;
	}
	else if (dif_abs > thr_2)
	{
		val_final = val_orig;
	}
	else
	{
		const int		mult_neg = dif_abs * thr_slope - thr_offset;	// 1.15 bits, negative
		const int		modif_neg = (dif * mult_neg) >> 14;
		val_final = val_orig - modif_neg;
	}

	return (val_final);
}



int	DiffSoftClipper::clip_dif_cpp (int val_filt, int val_orig, int dif_abs_ref, int thr_1, int thr_2, int thr_slope, int thr_offset)
{
	const int		dif_abs = dif_abs_ref >> 1;	// 8.7 bits, unsigned
	const int		dif = (val_filt - val_orig) >> 1;		// 9.7 bits, signed
	int				val_final;
	if (dif_abs < thr_1)
	{
		val_final = val_filt;
	}
	else if (dif_abs > thr_2)
	{
		val_final = val_orig;
	}
	else
	{
		const int		mult_neg = dif_abs * thr_slope - thr_offset;	// 1.15 bits, negative
		const int		modif_neg = (dif * mult_neg) >> 14;
		val_final = val_orig - modif_neg;
	}

	return (val_final);
}



__m128i	DiffSoftClipper::clip_sse2 (const __m128i &val_filt, const __m128i &val_orig, const __m128i &val_ref, const __m128i &thr_1, const __m128i &thr_2, const __m128i &thr_slope, const __m128i &thr_offset)
{
	// 8.7 bits, unsigned
	const __m128i	val_filt_sr1 = _mm_srli_epi16 (val_filt, 1);
	const __m128i	val_orig_sr1 = _mm_srli_epi16 (val_orig, 1);
	const __m128i	val_ref_sr1  = _mm_srli_epi16 (val_ref,  1);

	// 9.7 bits, signed
	const __m128i	dif     = _mm_sub_epi16 (val_filt_sr1, val_orig_sr1);

	// dif_abs: 8.7 bits, unsigned
	const __m128i	dif_min = _mm_min_epi16 (val_filt_sr1, val_ref_sr1);
	const __m128i	dif_max = _mm_max_epi16 (val_filt_sr1, val_ref_sr1);
	const __m128i	dif_abs = _mm_sub_epi16 (dif_max, dif_min);

	// Threshold masks
	const __m128i	mask_thr_1  = _mm_cmplt_epi16 (dif_abs, thr_1);
	const __m128i	mask_thr_2  = _mm_cmplt_epi16 (thr_2, dif_abs);
	const __m128i	maskn_slope = _mm_or_si128 (mask_thr_1, mask_thr_2);	// Use with andnot

	// Slope calculation
	__m128i			mult_neg = _mm_mullo_epi16 (dif_abs, thr_slope);
	mult_neg = _mm_sub_epi16 (mult_neg, thr_offset);	// 1.15 bits, negative
	__m128i			modif_neg = _mm_mulhi_epi16 (dif, mult_neg);
	modif_neg = _mm_slli_epi16 (modif_neg, 2);
	__m128i			val_final = _mm_sub_epi16 (val_orig, modif_neg);

	// Final masking
	val_final = _mm_andnot_si128 (maskn_slope, val_final);
	const __m128i	val_filt_masked = _mm_and_si128 (mask_thr_1, val_filt);
	val_final = _mm_or_si128 (val_final, val_filt_masked);
	const __m128i	val_orig_masked = _mm_and_si128 (mask_thr_2, val_orig);
	val_final = _mm_or_si128 (val_final, val_orig_masked);

	return (val_final);
}



__m128i	DiffSoftClipper::clip_dif_sse2 (const __m128i &val_filt, const __m128i &val_orig, const __m128i &dif_abs_ref, const __m128i &thr_1, const __m128i &thr_2, const __m128i &thr_slope, const __m128i &thr_offset)
{
	// 8.7 bits, unsigned
	const __m128i	val_filt_sr1 = _mm_srli_epi16 (val_filt, 1);
	const __m128i	val_orig_sr1 = _mm_srli_epi16 (val_orig, 1);

	// 9.7 bits, signed
	const __m128i	dif     = _mm_sub_epi16 (val_filt_sr1, val_orig_sr1);

	// dif_abs: 8.7 bits, unsigned
	const __m128i	dif_abs = _mm_srai_epi16 (dif_abs_ref,  1);

	// Threshold masks
	const __m128i	mask_thr_1  = _mm_cmplt_epi16 (dif_abs, thr_1);
	const __m128i	mask_thr_2  = _mm_cmplt_epi16 (thr_2, dif_abs);
	const __m128i	maskn_slope = _mm_or_si128 (mask_thr_1, mask_thr_2);	// Use with andnot

	// Slope calculation
	__m128i			mult_neg = _mm_mullo_epi16 (dif_abs, thr_slope);
	mult_neg = _mm_sub_epi16 (mult_neg, thr_offset);	// 1.15 bits, negative
	__m128i			modif_neg = _mm_mulhi_epi16 (dif, mult_neg);
	modif_neg = _mm_slli_epi16 (modif_neg, 2);
	__m128i			val_final = _mm_sub_epi16 (val_orig, modif_neg);

	// Final masking
	val_final = _mm_andnot_si128 (maskn_slope, val_final);
	const __m128i	val_filt_masked = _mm_and_si128 (mask_thr_1, val_filt);
	val_final = _mm_or_si128 (val_final, val_filt_masked);
	const __m128i	val_orig_masked = _mm_and_si128 (mask_thr_2, val_orig);
	val_final = _mm_or_si128 (val_final, val_orig_masked);

	return (val_final);
}



__m256i	DiffSoftClipper::clip_avx2 (const __m256i &val_filt, const __m256i &val_orig, const __m256i &val_ref, const __m256i &thr_1, const __m256i &thr_2, const __m256i &thr_slope, const __m256i &thr_offset)
{
	// 8.7 bits, unsigned
	const __m256i	val_filt_sr1 = _mm256_srli_epi16 (val_filt, 1);
	const __m256i	val_orig_sr1 = _mm256_srli_epi16 (val_orig, 1);
	const __m256i	val_ref_sr1  = _mm256_srli_epi16 (val_ref,  1);

	// 9.7 bits, signed
	const __m256i	dif     = _mm256_sub_epi16 (val_filt_sr1, val_orig_sr1);

	// dif_abs: 8.7 bits, unsigned
	const __m256i	dif_min = _mm256_min_epi16 (val_filt_sr1, val_ref_sr1);
	const __m256i	dif_max = _mm256_max_epi16 (val_filt_sr1, val_ref_sr1);
	const __m256i	dif_abs = _mm256_sub_epi16 (dif_max, dif_min);

	// Threshold masks
	const __m256i	mask_thr_1  = _mm256_cmpgt_epi16 (thr_1, dif_abs);
	const __m256i	mask_thr_2  = _mm256_cmpgt_epi16 (dif_abs, thr_2);
	const __m256i	maskn_slope = _mm256_or_si256 (mask_thr_1, mask_thr_2);	// Use with andnot

	// Slope calculation
	__m256i			mult_neg = _mm256_mullo_epi16 (dif_abs, thr_slope);
	mult_neg = _mm256_sub_epi16 (mult_neg, thr_offset);	// 1.15 bits, negative
	__m256i			modif_neg = _mm256_mulhi_epi16 (dif, mult_neg);
	modif_neg = _mm256_slli_epi16 (modif_neg, 2);
	__m256i			val_final = _mm256_sub_epi16 (val_orig, modif_neg);

	// Final masking
	val_final = _mm256_andnot_si256 (maskn_slope, val_final);
	const __m256i	val_filt_masked = _mm256_and_si256 (mask_thr_1, val_filt);
	val_final = _mm256_or_si256 (val_final, val_filt_masked);
	const __m256i	val_orig_masked = _mm256_and_si256 (mask_thr_2, val_orig);
	val_final = _mm256_or_si256 (val_final, val_orig_masked);

	return (val_final);
}



__m256i	DiffSoftClipper::clip_dif_avx2 (const __m256i &val_filt, const __m256i &val_orig, const __m256i &dif_abs_ref, const __m256i &thr_1, const __m256i &thr_2, const __m256i &thr_slope, const __m256i &thr_offset)
{
	// 8.7 bits, unsigned
	const __m256i	val_filt_sr1 = _mm256_srli_epi16 (val_filt, 1);
	const __m256i	val_orig_sr1 = _mm256_srli_epi16 (val_orig, 1);

	// 9.7 bits, signed
	const __m256i	dif     = _mm256_sub_epi16 (val_filt_sr1, val_orig_sr1);

	// dif_abs: 8.7 bits, unsigned
	const __m256i	dif_abs = _mm256_srai_epi16 (dif_abs_ref,  1);

	// Threshold masks
	const __m256i	mask_thr_1  = _mm256_cmpgt_epi16 (thr_1, dif_abs);
	const __m256i	mask_thr_2  = _mm256_cmpgt_epi16 (dif_abs, thr_2);
	const __m256i	maskn_slope = _mm256_or_si256 (mask_thr_1, mask_thr_2);	// Use with andnot

	// Slope calculation
	__m256i			mult_neg = _mm256_mullo_epi16 (dif_abs, thr_slope);
	mult_neg = _mm256_sub_epi16 (mult_neg, thr_offset);	// 1.15 bits, negative
	__m256i			modif_neg = _mm256_mulhi_epi16 (dif, mult_neg);
	modif_neg = _mm256_slli_epi16 (modif_neg, 2);
	__m256i			val_final = _mm256_sub_epi16 (val_orig, modif_neg);

	// Final masking
	val_final = _mm256_andnot_si256 (maskn_slope, val_final);
	const __m256i	val_filt_masked = _mm256_and_si256 (mask_thr_1, val_filt);
	val_final = _mm256_or_si256 (val_final, val_filt_masked);
	const __m256i	val_orig_masked = _mm256_and_si256 (mask_thr_2, val_orig);
	val_final = _mm256_or_si256 (val_final, val_orig_masked);

	return (val_final);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#endif	// DiffSoftClipper_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
