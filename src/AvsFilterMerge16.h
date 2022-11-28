/*****************************************************************************

        AvsFilterMerge16.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterMerge16_HEADER_INCLUDED)
#define	AvsFilterMerge16_HEADER_INCLUDED

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

#include <cstdint>



class AvsFilterMerge16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterMerge16	ThisType;

	explicit       AvsFilterMerge16 (::IScriptEnvironment *env_ptr, ::PClip src1_clip_sptr, ::PClip src2_clip_sptr, ::PClip mask_clip_sptr, bool luma_flag, int y, int u, int v);
	virtual        ~AvsFilterMerge16 () {}

	virtual ::PVideoFrame __stdcall
	               GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void   do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

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
		const uint8_t* _msk_msb_ptr;
		const uint8_t* _msk_lsb_ptr;
		int            _stride_dst;
		int            _stride_sr1;
		int            _stride_sr2;
		int            _stride_msk;
		int            _pitch_mul;
	};

	typedef	MTSlicer <ThisType, TaskDataGlobal>	Slicer;
	typedef	void (ThisType::*PlaneProcPtr) (Slicer::TaskData &td);

	template <class T>
	void           process_subplane_cpp (Slicer::TaskData &td);
	template <class T>
	void           process_subplane_sse2 (Slicer::TaskData &td);

	__forceinline static __m128i
	               process_vect_sse2 (const __m128i &src_1, const __m128i &src_2, const __m128i &mask, const __m128i &zero, const __m128i &ffff, const __m128i &mask_sign, const __m128i &offset_s);

	class Mask444
	{
	public:
	__forceinline static int
						get_mask_cpp (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk);
	__forceinline static __m128i
						get_mask_sse2 (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk);
	};
	class Mask420
	{
	public:
	__forceinline static int
						get_mask_cpp (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk);
	__forceinline static __m128i
						get_mask_sse2 (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk);
	};
	class Mask422
	{
	public:
	__forceinline static int
						get_mask_cpp (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk);
	__forceinline static __m128i
						get_mask_sse2 (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk);
	};
	class Mask411
	{
	public:
	__forceinline static int
						get_mask_cpp (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk);
	__forceinline static __m128i
						get_mask_sse2 (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk);
	};

	::PClip        _src1_clip_sptr;
	::PClip        _src2_clip_sptr;
	::PClip        _mask_clip_sptr;
	PlaneProcessor _plane_proc;
	bool           _luma_flag;
	int            _luma_type;    // CS_YV24, CS_YV16, CS_YV411 or CS_YV12
	int            _pitch_mul;
	PlaneProcPtr   _process_default_subplane_ptr;
	PlaneProcPtr   _process_chroma_subplane_ptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterMerge16 ();
						AvsFilterMerge16 (const AvsFilterMerge16 &other);
	AvsFilterMerge16 &
						operator = (const AvsFilterMerge16 &other);
	bool				operator == (const AvsFilterMerge16 &other) const;
	bool				operator != (const AvsFilterMerge16 &other) const;

};	// class AvsFilterMerge16



//#include	"AvsFilterMerge16.hpp"



#endif	// AvsFilterMerge16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
