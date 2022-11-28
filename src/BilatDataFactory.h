/*****************************************************************************

        BilatDataFactory.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (BilatDataFactory_HEADER_INCLUDED)
#define	BilatDataFactory_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"conc/ObjFactoryInterface.h"
#include	"BilatData.h"



class BilatDataFactory
:	public conc::ObjFactoryInterface <BilatData>
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit			BilatDataFactory (int width, int height, int margin_h, int margin_v, bool src_flag);
	virtual			~BilatDataFactory () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// conc::ObjFactoryInterface
	virtual BilatData *
						do_create ();



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	int				_width;
	int				_height;
	int				_margin_h;
	int				_margin_v;
	bool				_src_flag;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						BilatDataFactory ();
						BilatDataFactory (const BilatDataFactory &other);
	BilatDataFactory &
						operator = (const BilatDataFactory &other);
	bool				operator == (const BilatDataFactory &other) const;
	bool				operator != (const BilatDataFactory &other) const;

};	// class BilatDataFactory



//#include	"BilatDataFactory.hpp"



#endif	// BilatDataFactory_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
