/*****************************************************************************

        AvsFilterLimitDif16.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterLimitDif16_HEADER_INCLUDED)
#define	AvsFilterLimitDif16_HEADER_INCLUDED

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
#include	"MTSlicer.h"
#include	"PlaneProcCbInterface.h"
#include	"PlaneProcessor.h"

#include <cstdint>



class AvsFilterLimitDif16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterLimitDif16	ThisType;

	explicit			AvsFilterLimitDif16 (::IScriptEnvironment *env_ptr, ::PClip flt_clip_sptr, ::PClip src_clip_sptr, ::PClip ref_clip_sptr, double threshold, double elast, int y, int u, int v, bool refabsdif_flag);
	virtual			~AvsFilterLimitDif16 () {}

	virtual ::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			MAX_NBR_PLANES	= 3	};

	class TaskDataGlobal
	{
	public:
		ThisType *		_this_ptr;
		int				_w;
		uint8_t *		_dst_msb_ptr;
		uint8_t *		_dst_lsb_ptr;
		const uint8_t*	_flt_msb_ptr;
		const uint8_t*	_flt_lsb_ptr;
		const uint8_t*	_src_msb_ptr;
		const uint8_t*	_src_lsb_ptr;
		const uint8_t*	_ref_msb_ptr;
		const uint8_t*	_ref_lsb_ptr;
		int				_stride_dst;
		int				_stride_flt;
		int				_stride_src;
		int				_stride_ref;
	};

	typedef	MTSlicer <ThisType, TaskDataGlobal>	Slicer;

	typedef	void (ThisType::*
						ProcSegPtr) (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *flt_msb_ptr, const uint8_t *flt_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w) const;

	void				init_fnc ();

	void				process_subplane (Slicer::TaskData &td);

	void				process_segment_cpp (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *flt_msb_ptr, const uint8_t *flt_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w) const;
	void				process_segment_sse2 (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *flt_msb_ptr, const uint8_t *flt_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w) const;
	void				process_segment_avx2 (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *flt_msb_ptr, const uint8_t *flt_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int w) const;

	::PClip			_flt_clip_sptr;
	::PClip			_src_clip_sptr;
	::PClip			_ref_clip_sptr;
	double			_threshold;
	double			_elast;
	bool				_refabsdif_flag;
	PlaneProcessor	_plane_proc;

	int				_thr_1;				//
	int				_thr_2;				//
	int				_thr_slope;			//
	int				_thr_offset;		//

	ProcSegPtr		_process_segment_ptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterLimitDif16 ();
						AvsFilterLimitDif16 (const AvsFilterLimitDif16 &other);
	AvsFilterLimitDif16 &
						operator = (const AvsFilterLimitDif16 &other);
	bool				operator == (const AvsFilterLimitDif16 &other) const;
	bool				operator != (const AvsFilterLimitDif16 &other) const;

};	// class AvsFilterLimitDif16



//#include	"AvsFilterLimitDif16.hpp"



#endif	// AvsFilterLimitDif16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
