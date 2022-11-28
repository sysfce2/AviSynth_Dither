/*****************************************************************************

        BoxFilter.hpp
        Author: Laurent de Soras, 2010

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (BoxFilter_CODEHEADER_INCLUDED)
#define	BoxFilter_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#if defined (_MSC_VER)
	#include <intrin.h>
	#pragma intrinsic (__emulu)
#endif



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	BoxFilter::filter_col ()
{
		const int		delta = _sum_v [_pos_pos] - _sum_v [_pos_neg];
		++ _pos_pos;
		++ _pos_neg;

		_sum_h += delta;

		const int		val =
#if defined (_MSC_VER)
			int (__emulu (_sum_h + _w_round, _weight_mul) >> 32);
#else
			int ((uint64_t (_sum_h + _w_round) * uint64_t (_weight_mul)) >> 32);
#endif

		return (val);
}



int	BoxFilter::get_col_w () const
{
	return (_col_w);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#endif	// BoxFilter_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
