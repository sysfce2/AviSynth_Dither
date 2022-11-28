/*****************************************************************************

        AvsFilterOut.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (AvsFilterOut_HEADER_INCLUDED)
#define	AvsFilterOut_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#define	NOGDI
#include <windows.h>
#include	"avisynth.h"
#include	"PlaneProcCbInterface.h"
#include	"PlaneProcessor.h"


class AvsFilterOut
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterOut	ThisType;

	explicit       AvsFilterOut (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, bool big_endian_flag);
	virtual        ~AvsFilterOut () {}

	virtual ::PVideoFrame __stdcall
	               GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static bool    check_sse2 (::IScriptEnvironment &env);

	::PClip        _src_clip_sptr;
	::VideoInfo    _vi_src;
	bool           _big_endian_flag;
	bool           _sse2_flag;
	PlaneProcessor _plane_proc;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               AvsFilterOut ();
	               AvsFilterOut (const AvsFilterOut &other);
	AvsFilterOut & operator = (const AvsFilterOut &other);
	bool           operator == (const AvsFilterOut &other) const;
	bool           operator != (const AvsFilterOut &other) const;

};	// class AvsFilterOut



//#include	"AvsFilterOut.hpp"



#endif	// AvsFilterOut_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
