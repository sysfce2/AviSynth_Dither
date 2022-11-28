/*****************************************************************************

        AvsFilterBilateral16.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterBilateral16_HEADER_INCLUDED)
#define	AvsFilterBilateral16_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#define	NOGDI
#include <windows.h>

#include "avisynth.h"
#include "BilatDataFactory.h"
#include "FilterBilateral.h"
#include "PlaneProcCbInterface.h"
#include "PlaneProcessor.h"

#include <emmintrin.h>

#include <map>
#include <memory>
#include <mutex>

#include <cstdint>



class AvsFilterBilateral16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterBilateral16	ThisType;

	explicit			AvsFilterBilateral16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, ::PClip ref_clip_sptr, int radius, double threshold, double flat, double wmin, double subspl, int y, int u, int v);
	virtual			~AvsFilterBilateral16 () {}

	virtual ::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			MAX_NBR_PLANES		= 3	};

	bool				is_same_clip_format (const ::PClip &test_clip_sptr) const;
	void				init_fnc (::IScriptEnvironment &env);

	FilterBilateral *
						create_or_access_plane_filter (int plane_id, int w, int h, int radius_h, int radius_v, bool src_flag);

	::PClip			_flt_clip_sptr;
	::PClip			_src_clip_sptr;
	::PClip			_ref_clip_sptr;
	double			_threshold;
	double			_flat;
	double			_wmin;
	int				_radius;
	double			_subspl;
	PlaneProcessor	_plane_proc;

	int				_m;
	int				_wmax;
	bool				_sse2_flag;

	std::mutex     _filter_mutex;          // To access _filter_aptr_map.
	std::map <int64_t, std::unique_ptr <FilterBilateral> >
	               _filter_uptr_map;       // Created only on request. [(h<<32)+w]



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterBilateral16 ();
						AvsFilterBilateral16 (const AvsFilterBilateral16 &other);
	AvsFilterBilateral16 &
						operator = (const AvsFilterBilateral16 &other);
	bool				operator == (const AvsFilterBilateral16 &other) const;
	bool				operator != (const AvsFilterBilateral16 &other) const;

};	// class AvsFilterBilateral16



//#include	"AvsFilterBilateral16.hpp"



#endif	// AvsFilterBilateral16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
