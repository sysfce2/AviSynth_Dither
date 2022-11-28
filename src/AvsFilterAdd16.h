/*****************************************************************************

        AvsFilterAdd16.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterAdd16_HEADER_INCLUDED)
#define	AvsFilterAdd16_HEADER_INCLUDED

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
#include	<immintrin.h>

#include <cstdint>



class AvsFilterAdd16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterAdd16	ThisType;

	explicit			AvsFilterAdd16 (::IScriptEnvironment *env_ptr, ::PClip stack1_clip_sptr, ::PClip stack2_clip_sptr, bool wrap_flag, int y, int u, int v, bool dif_flag, bool sub_flag);
	virtual			~AvsFilterAdd16 () {}

	virtual ::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			MAX_NBR_PLANES	= 3	};

	class OpAddWrap
	{
	public:
		__forceinline int
							operator () (int src1, int src2) const;
		__forceinline __m128i
							operator () (__m128i src1, __m128i src2, __m128i mask_sign) const;
		__forceinline __m256i
							operator () (__m256i src1, __m256i src2, __m256i mask_sign) const;
	};
	class OpAddSaturate
	{
	public:
		__forceinline int
							operator () (int src1, int src2) const;
		__forceinline __m128i
							operator () (__m128i src1, __m128i src2, __m128i mask_sign) const;
		__forceinline __m256i
							operator () (__m256i src1, __m256i src2, __m256i mask_sign) const;
	};
	class OpAddDifWrap
	{
	public:
		__forceinline int
							operator () (int src1, int src2) const;
		__forceinline __m128i
							operator () (__m128i src1, __m128i src2, __m128i mask_sign) const;
		__forceinline __m256i
							operator () (__m256i src1, __m256i src2, __m256i mask_sign) const;
	};
	class OpAddDifSaturate
	{
	public:
		__forceinline int
							operator () (int src1, int src2) const;
		__forceinline __m128i
							operator () (__m128i src1, __m128i src2, __m128i mask_sign) const;
		__forceinline __m256i
							operator () (__m256i src1, __m256i src2, __m256i mask_sign) const;
	};
	class OpSubWrap
	{
	public:
		__forceinline int
							operator () (int src1, int src2) const;
		__forceinline __m128i
							operator () (__m128i src1, __m128i src2, __m128i mask_sign) const;
		__forceinline __m256i
							operator () (__m256i src1, __m256i src2, __m256i mask_sign) const;
	};
	class OpSubSaturate
	{
	public:
		__forceinline int
							operator () (int src1, int src2) const;
		__forceinline __m128i
							operator () (__m128i src1, __m128i src2, __m128i mask_sign) const;
		__forceinline __m256i
							operator () (__m256i src1, __m256i src2, __m256i mask_sign) const;
	};
	class OpSubDifWrap
	{
	public:
		__forceinline int
							operator () (int src1, int src2) const;
		__forceinline __m128i
							operator () (__m128i src1, __m128i src2, __m128i mask_sign) const;
		__forceinline __m256i
							operator () (__m256i src1, __m256i src2, __m256i mask_sign) const;
	};
	class OpSubDifSaturate
	{
	public:
		__forceinline int
							operator () (int src1, int src2) const;
		__forceinline __m128i
							operator () (__m128i src1, __m128i src2, __m128i mask_sign) const;
		__forceinline __m256i
							operator () (__m256i src1, __m256i src2, __m256i mask_sign) const;
	};

	class TaskDataGlobal
	{
	public:
		ThisType *		_this_ptr;
		int				_w;
		uint8_t *		_dst_msb_ptr;
		uint8_t *		_dst_lsb_ptr;
		const uint8_t*	_sr1_msb_ptr;
		const uint8_t*	_sr1_lsb_ptr;
		const uint8_t*	_sr2_msb_ptr;
		const uint8_t*	_sr2_lsb_ptr;
		int				_stride_dst;
		int				_stride_sr1;
		int				_stride_sr2;
	};

	typedef	MTSlicer <ThisType, TaskDataGlobal>	Slicer;

	void				process_subplane (Slicer::TaskData &td);
	void				configure_avx2 ();

	template <class OP>
	class RowProc
	{
	public:
		static void		process_row_cpp (uint8_t *data_msbd_ptr, uint8_t *data_lsbd_ptr, const uint8_t * data_msb1_ptr, const uint8_t * data_lsb1_ptr, const uint8_t *data_msb2_ptr, const uint8_t *data_lsb2_ptr, int w);
		static void		process_row_sse2 (uint8_t *data_msbd_ptr, uint8_t *data_lsbd_ptr, const uint8_t * data_msb1_ptr, const uint8_t * data_lsb1_ptr, const uint8_t *data_msb2_ptr, const uint8_t *data_lsb2_ptr, int w);
		static void		process_row_avx2 (uint8_t *data_msbd_ptr, uint8_t *data_lsbd_ptr, const uint8_t * data_msb1_ptr, const uint8_t * data_lsb1_ptr, const uint8_t *data_msb2_ptr, const uint8_t *data_lsb2_ptr, int w);
	};

	::PClip			_stack1_clip_sptr;
	::PClip			_stack2_clip_sptr;
	PlaneProcessor	_plane_proc;
	bool				_wrap_flag;
	bool				_dif_flag;
	bool				_sub_flag;
	void (*			_process_row_ptr) (uint8_t *data_msbd_ptr, uint8_t *data_lsbd_ptr, const uint8_t * data_msb1_ptr, const uint8_t * data_lsb1_ptr, const uint8_t *data_msb2_ptr, const uint8_t *data_lsb2_ptr, int w);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterAdd16 ();
						AvsFilterAdd16 (const AvsFilterAdd16 &other);
	AvsFilterAdd16 &
						operator = (const AvsFilterAdd16 &other);
	bool				operator == (const AvsFilterAdd16 &other) const;
	bool				operator != (const AvsFilterAdd16 &other) const;

};	// class AvsFilterAdd16



//#include	"AvsFilterAdd16.hpp"



#endif	// AvsFilterAdd16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
