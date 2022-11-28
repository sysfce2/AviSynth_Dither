/*****************************************************************************

        fnc.cpp
        Author: Laurent de Soras, 2010

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

#if defined (_MSC_VER)
	#include "fstb/txt/Conv.h"
	#include "fstb/Err.h"
#endif
#include "fstb/fnc.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>



namespace fstb
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Only for ANSI strings.
void	conv_to_lower_case (std::string &str)
{
	for (std::string::size_type p = 0; p < str.length (); ++p)
	{
		str [p] = char (tolower (str [p]));
	}
}



int	snprintf4all (char *out_0, size_t size, const char *format_0, ...)
{
	va_list        ap;
	va_start (ap, format_0);
	int            cnt = -1;

#if defined (_MSC_VER) && (_MSC_VER < 1900)

	if (size != 0)
	{
		cnt = _vsnprintf_s (out_0, size, _TRUNCATE, format_0, ap);
	}
	if (cnt == -1)
	{
		cnt = _vscprintf (format_0, ap);
	}

#else

	cnt = vsnprintf (out_0, size, format_0, ap);

#endif

	va_end (ap);

	return cnt;
}



FILE *	fopen_utf8 (const char *filename_0, const char *mode_0)
{
	FILE *         f_ptr = nullptr;

#if defined (_MSC_VER)

	std::basic_string <wchar_t>   filename_utf16;
	std::basic_string <wchar_t>   mode_utf16;
	using fstb::txt::Conv;
	if (   Conv::utf8_to_utf16 (filename_utf16, filename_0) == fstb::Err_OK
	    && Conv::utf8_to_utf16 (mode_utf16    , mode_0    ) == fstb::Err_OK)
	{
		if (_wfopen_s (&f_ptr, filename_utf16.c_str (), mode_utf16.c_str ()) != 0)
		{
			f_ptr = nullptr;
		}
	}

#else

 #if fstb_COMPILER == fstb_COMPILER_GCC && defined (__USE_FILE_OFFSET64)
	// fopen64 needed with the GNU/LibC to overcome the 2GB limit
	// https://www.gnu.org/software/libc/manual/html_node/Opening-Streams.html#index-fopen64
	f_ptr = fopen64 (filename_0, mode_0);
 #else
	f_ptr = fopen (filename_0, mode_0);
 #endif

#endif

	return f_ptr;
}



}	// namespace fstb



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
