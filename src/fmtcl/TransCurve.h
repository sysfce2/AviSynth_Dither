/*****************************************************************************

        TransCurve.h
        Author: Laurent de Soras, 2013

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransCurve_HEADER_INCLUDED)
#define	fmtcl_TransCurve_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



enum TransCurve
{
	TransCurve_UNDEF = -1,

	// ITU-T H.265, High efficiency video coding, 2014-10, p. 348
	TransCurve_RESERVED1 = 0,
	TransCurve_709,         // ITU-R BT.709
	TransCurve_UNSPECIFIED,
	TransCurve_RESERVED2,
	TransCurve_470M,        // ITU-R BT.470-6 System M, FCC (assumed display gamma 2.2)
	TransCurve_470BG,       // ITU-R BT.470-6 System B, G (assumed display gamma 2.8)
	TransCurve_601,         // ITU-R BT.601
	TransCurve_240,         // SMPTE 240M
	TransCurve_LINEAR,
	TransCurve_LOG100,      // 100:1 range
	TransCurve_LOG316,      // 100*Sqrt(10):1 range
	TransCurve_61966_2_4,   // IEC 61966-2-4. Same as 709, but symetric and with more range
	TransCurve_1361,        // ITU-R BT.1361 extended colour gamut system
	TransCurve_SRGB,        // IEC 61966-2-1. sRGB or sYCC
	TransCurve_2020_10,     // ITU-R BT.2020 10 bits
	TransCurve_2020_12,     // ITU-R BT.2020 12 bits
	TransCurve_2084,        // SMPTE ST 2084
	TransCurve_428,         // SMPTE ST 428

	TransCurve_NBR_ELT,

	TransCurve_ISO_RANGE_LAST = 255,

	// Other values
	TransCurve_1886,        // ITU-R BT.1886
	TransCurve_1886A,       // ITU-R BT.1886 alternative approximation
	TransCurve_FILMSTREAM,  // Thomson FilmStream
	TransCurve_SLOG,        // Sony S-Log
	TransCurve_LOGC2,       // Arri Log C Alexa 2.x (800 EI), linear scene exposure
	TransCurve_LOGC3,       // Arri Log C Alexa 3.x (800 EI), linear scene exposure
	TransCurve_CANONLOG     // Canon-Log
};	// enum TransCurve



}	// namespace fmtcl



//#include "fmtcl/TransCurve.hpp"



#endif	// fmtcl_TransCurve_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
