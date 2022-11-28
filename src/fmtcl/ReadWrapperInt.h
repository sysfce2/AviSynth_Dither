/*****************************************************************************

        ReadWrapperInt.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ReadWrapperInt_HEADER_INCLUDED)
#define	fmtcl_ReadWrapperInt_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



template <class SRC, class S16R, bool PF>
class ReadWrapperInt
{
public:

	template <class VI>
	static fstb_FORCEINLINE VI
	               read (const typename SRC::PtrConst::Type &ptr, const VI &zero, const VI &sign_bit, int /*len*/);

};	// class ReadWrapperInt

template <class SRC, class S16R>
class ReadWrapperInt <SRC, S16R, true>
{
public:

	template <class VI>
	static fstb_FORCEINLINE VI
	               read (const typename SRC::PtrConst::Type &ptr, const VI &zero, const VI &sign_bit, int len);

};



}	// namespace fmtcl



#include "fmtcl/ReadWrapperInt.hpp"



#endif	// fmtcl_ReadWrapperInt_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
