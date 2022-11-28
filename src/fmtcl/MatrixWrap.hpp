/*****************************************************************************

        MatrixWrap.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_MatrixWrap_CODEHEADER_INCLUDED)
#define	fmtcl_MatrixWrap_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
MatrixWrap <T>::MatrixWrap (int w, int h)
:	_w (w)
,	_h (h)
,	_mat (w * h, 0)
{
	assert (w > 0);
	assert (h > 0);
}



template <class T>
void	MatrixWrap <T>::clear (T fill_val)
{
	_mat.assign (_mat.size (), fill_val);
}



template <class T>
T &	MatrixWrap <T>::operator () (int x, int y)
{
	assert (x >= -MARGIN * _w);
	assert (y >= -MARGIN * _h);

	x = (x + _w * MARGIN) % _w;
	y = (y + _h * MARGIN) % _h;

	return (_mat [y * _w + x]);
}



template <class T>
const T &	MatrixWrap <T>::operator () (int x, int y) const
{
	assert (x >= -MARGIN * _w);
	assert (y >= -MARGIN * _h);

	x = (x + _w * MARGIN) % _w;
	y = (y + _h * MARGIN) % _h;

	return (_mat [y * _w + x]);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_MatrixWrap_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
