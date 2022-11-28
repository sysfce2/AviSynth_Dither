/*****************************************************************************

        Borrower.hpp
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (Borrower_CODEHEADER_INCLUDED)
#define	Borrower_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	<stdexcept>

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
Borrower <T>::Borrower (conc::ObjPool <T> &mgr, const char *fail_msg_0)
:	_mgr (mgr)
,	_rsrc_ptr (mgr.take_obj ())
{
	assert (&mgr != 0);
	assert (fail_msg_0 != 0);

	if (_rsrc_ptr == 0)
	{
		throw std::runtime_error (fail_msg_0);
	}
}



template <class T>
Borrower <T>::~Borrower ()
{
	if (_rsrc_ptr != 0)
	{
		_mgr.return_obj (*_rsrc_ptr);
		_rsrc_ptr = 0;
	}
}



template <class T>
T &	Borrower <T>::use ()
{
	assert (_rsrc_ptr != 0);

	return (*_rsrc_ptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#endif	// Borrower_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
