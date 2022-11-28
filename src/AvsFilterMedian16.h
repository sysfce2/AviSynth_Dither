/*****************************************************************************

        AvsFilterMedian16.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterMedian16_HEADER_INCLUDED)
#define	AvsFilterMedian16_HEADER_INCLUDED

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

#include	<emmintrin.h>



class AvsFilterMedian16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterMedian16	ThisType;

	explicit			AvsFilterMedian16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, int rx, int ry, int rt, int ql, int qh, int y, int u, int v);
	virtual			~AvsFilterMedian16 () {}

	virtual ::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			MAX_NBR_PLANES = 3    };
	enum {			MAX_AREA_SIZE  = 4096 };
	enum {			MAX_RADIUS_T   = 32   };
	enum {			MAX_RANGE_T    = MAX_RADIUS_T * 2 + 1 };

	class TaskDataGlobal
	{
	public:
		class SrcFrame
		{
		public:
			const uint8_t*	_msb_ptr;
			const uint8_t*	_lsb_ptr;
			int				_stride;
		};

		ThisType *		_this_ptr;
		int				_w;
		int				_h;
		uint8_t *		_dst_msb_ptr;
		uint8_t *		_dst_lsb_ptr;
		int				_stride_dst;
		SrcFrame			_src_arr [MAX_RANGE_T];
	};

	typedef	MTSlicer <ThisType, TaskDataGlobal>	Slicer;

	void				process_subplane_rt0 (Slicer::TaskData &td);
	void				process_subplane_rt0_avx2 (Slicer::TaskData &td);
	void				process_subplane_rtn (Slicer::TaskData &td);

	::PClip			_src_clip_sptr;
	PlaneProcessor	_plane_proc;
	int				_rx;
	int				_ry;
	int				_rt;	// Currently ignored
	int				_ql;
	int				_qh;
	bool				_sse2_flag;
	bool				_avx2_flag;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterMedian16 ();
						AvsFilterMedian16 (const AvsFilterMedian16 &other);
	AvsFilterMedian16 &
						operator = (const AvsFilterMedian16 &other);
	bool				operator == (const AvsFilterMedian16 &other) const;
	bool				operator != (const AvsFilterMedian16 &other) const;

};	// class AvsFilterMedian16



//#include	"AvsFilterMedian16.hpp"



#endif	// AvsFilterMedian16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
