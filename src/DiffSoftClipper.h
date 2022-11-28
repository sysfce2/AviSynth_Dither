/*****************************************************************************

        DiffSoftClipper.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (DiffSoftClipper_HEADER_INCLUDED)
#define	DiffSoftClipper_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	<emmintrin.h>
#include	<immintrin.h>



class DiffSoftClipper
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static inline void
						init_cst (int &thr_1, int &thr_2, int &thr_slope, int &thr_offset, double threshold, double elast);

	static __forceinline int
						clip_cpp (int val_filt, int val_orig, int val_ref, int thr_1, int thr_2, int thr_slope, int thr_offset);
	static __forceinline int
						clip_dif_cpp (int val_filt, int val_orig, int dif_abs_ref, int thr_1, int thr_2, int thr_slope, int thr_offset);

	static __forceinline __m128i
						clip_sse2 (const __m128i &val_filt, const __m128i &val_orig, const __m128i &val_ref, const __m128i &thr_1, const __m128i &thr_2, const __m128i &thr_slope, const __m128i &thr_offset);
	static __forceinline __m128i
						clip_dif_sse2 (const __m128i &val_filt, const __m128i &val_orig, const __m128i &dif_abs_ref, const __m128i &thr_1, const __m128i &thr_2, const __m128i &thr_slope, const __m128i &thr_offset);

	static __forceinline __m256i
						clip_avx2 (const __m256i &val_filt, const __m256i &val_orig, const __m256i &val_ref, const __m256i &thr_1, const __m256i &thr_2, const __m256i &thr_slope, const __m256i &thr_offset);
	static __forceinline __m256i
						clip_dif_avx2 (const __m256i &val_filt, const __m256i &val_orig, const __m256i &dif_abs_ref, const __m256i &thr_1, const __m256i &thr_2, const __m256i &thr_slope, const __m256i &thr_offset);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						DiffSoftClipper ();
						DiffSoftClipper (const DiffSoftClipper &other);
	virtual			~DiffSoftClipper () {}
	DiffSoftClipper &
						operator = (const DiffSoftClipper &other);
	bool				operator == (const DiffSoftClipper &other) const;
	bool				operator != (const DiffSoftClipper &other) const;

};	// class DiffSoftClipper



#include	"DiffSoftClipper.hpp"



#endif	// DiffSoftClipper_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
