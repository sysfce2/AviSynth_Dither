/*****************************************************************************

        AvsFilterMaxDif16.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterMaxDif16_HEADER_INCLUDED)
#define	AvsFilterMaxDif16_HEADER_INCLUDED

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
#include "MTSlicer.h"
#include "PlaneProcCbInterface.h"
#include "PlaneProcessor.h"

#include	<emmintrin.h>
#include	<immintrin.h>

#include <cstdint>



class AvsFilterMaxDif16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterMaxDif16	ThisType;

	explicit       AvsFilterMaxDif16 (::IScriptEnvironment *env_ptr, ::PClip src1_clip_sptr, ::PClip src2_clip_sptr, ::PClip ref_clip_sptr, int y, int u, int v, bool min_flag);
	virtual        ~AvsFilterMaxDif16 () {}

	virtual ::PVideoFrame __stdcall
	               GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void   do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	class OpMax
	{
	public:
		__forceinline bool
							operator () (int d1as, int d2as) const;
		__forceinline __m128i
							operator () (__m128i d1as, __m128i d2as) const;
		__forceinline __m256i
							operator () (__m256i d1as, __m256i d2as) const;
	};

	class OpMin
	{
	public:
		__forceinline bool
							operator () (int d1as, int d2as) const;
		__forceinline __m128i
							operator () (__m128i d1as, __m128i d2as) const;
		__forceinline __m256i
							operator () (__m256i d1as, __m256i d2as) const;
	};

	class TaskDataGlobal
	{
	public:
		ThisType *     _this_ptr;
		int            _w;
		uint8_t *      _dst_msb_ptr;
		uint8_t *      _dst_lsb_ptr;
		const uint8_t* _sr1_msb_ptr;
		const uint8_t* _sr1_lsb_ptr;
		const uint8_t* _sr2_msb_ptr;
		const uint8_t* _sr2_lsb_ptr;
		const uint8_t* _ref_msb_ptr;
		const uint8_t* _ref_lsb_ptr;
		int            _stride_dst;
		int            _stride_sr1;
		int            _stride_sr2;
		int            _stride_ref;
	};

	typedef	MTSlicer <ThisType, TaskDataGlobal>	Slicer;

	template <class OP>
	class PlaneProc
	{
	public:
		static void    process_subplane_cpp (Slicer::TaskData &td);
		static void    process_subplane_sse2 (Slicer::TaskData &td);
		static void    process_subplane_avx2 (Slicer::TaskData &td);
	};

	void           configure_avx2 ();
	void           process_subplane (Slicer::TaskData &td);

	::PClip        _src1_clip_sptr;
	::PClip        _src2_clip_sptr;
	::PClip        _ref_clip_sptr;
	PlaneProcessor _plane_proc;

	const bool     _min_flag;

	bool           _sse2_flag;
	bool           _avx2_flag;

	void (*        _process_subplane_ptr) (Slicer::TaskData &td);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterMaxDif16 ();
						AvsFilterMaxDif16 (const AvsFilterMaxDif16 &other);
	AvsFilterMaxDif16 &
						operator = (const AvsFilterMaxDif16 &other);
	bool				operator == (const AvsFilterMaxDif16 &other) const;
	bool				operator != (const AvsFilterMaxDif16 &other) const;

};	// class AvsFilterMaxDif16



//#include	"AvsFilterMaxDif16.hpp"



#endif	// AvsFilterMaxDif16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
