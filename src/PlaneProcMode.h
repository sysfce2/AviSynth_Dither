/*****************************************************************************

        PlaneProcMode.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (PlaneProcMode_HEADER_INCLUDED)
#define	PlaneProcMode_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



enum PlaneProcMode
{
	PlaneProcMode_FILL = 0,
	PlaneProcMode_GARBAGE,
	PlaneProcMode_COPY1,
	PlaneProcMode_PROCESS,
	PlaneProcMode_COPY2,
	PlaneProcMode_COPY3,

	PlaneProcMode_NBR_ELT

};	// class PlaneProcMode



#endif	// PlaneProcMode_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
