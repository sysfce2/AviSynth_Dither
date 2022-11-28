/*****************************************************************************

        AvsFilterSmoothGrad.h
        Author: Laurent de Soras, 2010

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterSmoothGrad_HEADER_INCLUDED)
#define	AvsFilterSmoothGrad_HEADER_INCLUDED
#pragma once

#if defined (_MSC_VER)
	#pragma warning (4 : 4250) // "Inherits via dominance."
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



class AvsFilterSmoothGrad
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterSmoothGrad	ThisType;

	explicit			AvsFilterSmoothGrad (::IScriptEnvironment *env_ptr, ::PClip msb_clip_sptr, ::PClip lsb_clip_sptr, int radius, double threshold, bool stacked_flag, ::PClip ref_clip_sptr, double elast, int y, int u, int v);
	virtual			~AvsFilterSmoothGrad ();

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

	void				process_plane (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w, int h, int stride_dst, int stride_src_msb, int stride_src_lsb, int stride_ref_msb, int stride_ref_lsb, int radius_h, int radius_v) const;
	void				process_column (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w, int h, int stride_dst, int stride_src_msb, int stride_src_lsb, int stride_ref_msb, int stride_ref_lsb, int col_x, int col_w, int radius_h, int radius_v) const;
	void				process_segment_cpp (BoxFilter &filter, uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr) const;
	void				process_segment_sse2 (BoxFilter &filter, uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr) const;

	::PClip			_msb_clip_sptr;
	::PClip			_lsb_clip_sptr;
	::PClip			_ref_clip_sptr;
	PlaneProcessor	_plane_proc;
	int				_radius;
	double			_threshold;
	double			_elast;
	bool				_stacked_flag;		// In case of a single input clip, indicates that MSB and LSB parts are stacked on the same frame instead of being in interleaved frames.

	int				_thr_1;				//
	int				_thr_2;				//
	int				_thr_slope;			//
	int				_thr_offset;		//

	void (ThisType::*
						_process_segment_ptr) (BoxFilter &filter, uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr) const;

	BoxFilter::AddLinePtr
						_add_line_ptr;
	BoxFilter::AddSubLinePtr
						_addsub_line_ptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterSmoothGrad ();
						AvsFilterSmoothGrad (const AvsFilterSmoothGrad &other);
	AvsFilterSmoothGrad &
						operator = (const AvsFilterSmoothGrad &other);
	bool				operator == (const AvsFilterSmoothGrad &other) const;
	bool				operator != (const AvsFilterSmoothGrad &other) const;

};	// class AvsFilterSmoothGrad



//#include	"AvsFilterSmoothGrad.hpp"



#endif	// AvsFilterSmoothGrad_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
