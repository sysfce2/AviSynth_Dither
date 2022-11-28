/*****************************************************************************

        BilatDataFactory.cpp
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

#include	"BilatData.h"
#include	"BilatDataFactory.h"

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



BilatDataFactory::BilatDataFactory (int width, int height, int margin_h, int margin_v, bool src_flag)
:	_width (width)
,	_height (height)
,	_margin_h (margin_h)
,	_margin_v (margin_v)
,	_src_flag (src_flag)
{
	assert (width > 0);
	assert (height > 0);
	assert (margin_h >= 0);
	assert (margin_v >= 0);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



BilatData *	BilatDataFactory::do_create ()
{
	BilatData *		data_ptr = 0;
	try
	{
		data_ptr = new BilatData (_width, _height, _margin_h, _margin_v, _src_flag);
	}
	catch (...)
	{
		data_ptr = 0;
	}

	return (data_ptr);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
