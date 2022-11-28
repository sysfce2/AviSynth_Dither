/*****************************************************************************

        AvsFilterBoxFilter16.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterBoxFilter16_HEADER_INCLUDED)
#define	AvsFilterBoxFilter16_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#define	NOGDI
#include <windows.h>
#include	"avisynth.h"
#include	"BoxFilter.h"
#include	"PlaneProcCbInterface.h"
#include	"PlaneProcessor.h"

#include	<emmintrin.h>

#include <cstdint>



class AvsFilterBoxFilter16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterBoxFilter16	ThisType;

	explicit			AvsFilterBoxFilter16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, int radius, int y, int u, int v);
	virtual			~AvsFilterBoxFilter16 () {}

	virtual ::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			MAX_NBR_PLANES	= 3	};

	void				init_fnc (::IScriptEnvironment &env);

	void				process_plane (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int h, int stride_dst, int stride_src, int radius_h, int radius_v) const;
	void				process_column (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int h, int stride_dst, int stride_src, int col_x, int col_w, int radius_h, int radius_v) const;
	void				process_segment (BoxFilter &filter, uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr) const;

	::PClip			_src_clip_sptr;
	int				_radius;
	PlaneProcessor	_plane_proc;

	BoxFilter::AddLinePtr
						_add_line_ptr;
	BoxFilter::AddSubLinePtr
						_addsub_line_ptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterBoxFilter16 ();
						AvsFilterBoxFilter16 (const AvsFilterBoxFilter16 &other);
	AvsFilterBoxFilter16 &
						operator = (const AvsFilterBoxFilter16 &other);
	bool				operator == (const AvsFilterBoxFilter16 &other) const;
	bool				operator != (const AvsFilterBoxFilter16 &other) const;

};	// class AvsFilterBoxFilter16



//#include	"AvsFilterBoxFilter16.hpp"



#endif	// AvsFilterBoxFilter16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
