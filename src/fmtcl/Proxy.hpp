/*****************************************************************************

        Proxy.hpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_Proxy_CODEHEADER_INCLUDED)
#define	fmtcl_Proxy_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/fnc.h"

#include <cstddef>
#include <cstring>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
typename Proxy::Ptr1 <T>::Type	Proxy::Ptr1 <T>::make_ptr (const uint8_t *ptr, int /*stride_bytes*/, int /*h*/)
{
	return ((T *) ptr);
}

template <class T>
void	Proxy::Ptr1 <T>::jump (Type &ptr, int stride)
{
	ptr += stride;
}

template <class T>
bool	Proxy::Ptr1 <T>::check_ptr (const Type &ptr, int align)
{
	const ptrdiff_t   x = reinterpret_cast <ptrdiff_t> (ptr);

	return (x != 0 && (x & (align - 1)) == 0);
}

template <class T>
void	Proxy::Ptr1 <T>::copy (const Type &dst_ptr, const typename Ptr1 <const T>::Type &src_ptr, size_t nbr_elt)
{
	memcpy (dst_ptr, src_ptr, nbr_elt * sizeof (DataType));
}



template <class T>
typename Proxy::Ptr2 <T>::Type	Proxy::Ptr2 <T>::make_ptr (const uint8_t *ptr, int stride_bytes, int h)
{
	return (Type (
		(T *) ptr,
		(T *) (ptr + h * stride_bytes)
	));
}

template <class T>
Proxy::Ptr2 <T>::Type::Type (T *msb_ptr, T *lsb_ptr)
:	_msb_ptr (msb_ptr)
,	_lsb_ptr (lsb_ptr)
{
	// Noting
}

template <class T>
void	Proxy::Ptr2 <T>::jump (Type &ptr, int stride)
{
	ptr._msb_ptr += stride;
	ptr._lsb_ptr += stride;
}

template <class T>
bool	Proxy::Ptr2 <T>::check_ptr (const Type &ptr, int align)
{
	const ptrdiff_t   xm = reinterpret_cast <ptrdiff_t> (ptr._msb_ptr);
	const ptrdiff_t   xl = reinterpret_cast <ptrdiff_t> (ptr._lsb_ptr);

	return (   xm != 0 && (xm & (align - 1)) == 0
	        && xl != 0 && (xl & (align - 1)) == 0);
}

template <class T>
void	Proxy::Ptr2 <T>::copy (const Type &dst_ptr, const typename Ptr2 <const T>::Type &src_ptr, size_t nbr_elt)
{
	memcpy (dst_ptr._msb_ptr, src_ptr._msb_ptr, nbr_elt * sizeof (DataType));
	memcpy (dst_ptr._lsb_ptr, src_ptr._lsb_ptr, nbr_elt * sizeof (DataType));
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_Proxy_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
