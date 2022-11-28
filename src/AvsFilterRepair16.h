/*****************************************************************************

        AvsFilterRepair16.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterRepair16_HEADER_INCLUDED)
#define	AvsFilterRepair16_HEADER_INCLUDED

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



class AvsFilterRepair16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterRepair16	ThisType;

	explicit			AvsFilterRepair16 (::IScriptEnvironment *env_ptr, ::PClip stack1_clip_sptr, ::PClip stack2_clip_sptr, int mode, int modeu, int modev);
	virtual			~AvsFilterRepair16 () {}

	virtual ::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			MAX_NBR_PLANES	= 3	};

	class ConvSigned
	{
	public:
		static __forceinline __m128i cv (__m128i a, __m128i m)
		{
			return (_mm_xor_si128 (a, m));
		}
	};
	class ConvUnsigned
	{
	public:
		static __forceinline __m128i cv (__m128i a, __m128i /*m*/)
		{
			return (a);
		}
	};

	class OpRG00
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign);
	};
	class OpRG01
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign);
	};
	class OpRG02
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign);
	};
	class OpRG03
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign);
	};
	class OpRG04
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign);
	};
	class OpRG05
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG06
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG07
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG08
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG09
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG10
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG12
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign);
	};
	class OpRG13
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign);
	};
	class OpRG14
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, __m128i mask_sign);
	};
	class OpRG15
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG16
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG17
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};
	class OpRG18
	{
	public:
		static __forceinline int
							rg (int cr, int a1, int a2, int a3, int a4, int c, int a5, int a6, int a7, int a8);
	};

	class TaskDataGlobal
	{
	public:
		ThisType *		_this_ptr;
		int				_w;
		int				_h;
		int				_plane_index;
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

	template <class OP>
	class PlaneProc
	{
	public:
		static void		process_subplane_cpp (Slicer::TaskData &td);
		static __forceinline void
							process_row_cpp (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *sr1_msb_ptr, const uint8_t *sr1_lsb_ptr, const uint8_t *sr2_msb_ptr, const uint8_t *sr2_lsb_ptr, int stride_sr2, int x_beg, int x_end);
		static void		process_subplane_sse2 (Slicer::TaskData &td);
	};

	::PClip			_stack1_clip_sptr;
	::PClip			_stack2_clip_sptr;
	PlaneProcessor	_plane_proc;
	int				_mode_arr [MAX_NBR_PLANES];
	void (*			_process_subplane_ptr_arr [MAX_NBR_PLANES]) (Slicer::TaskData &td);

	static inline PlaneProcMode
						conv_rgmode_to_proc (int rgmode);

	static __forceinline void
						sort_pair (__m128i &a1, __m128i &a2);
	static __forceinline void
						sort_pair (__m128i &mi, __m128i &ma, __m128i a1, __m128i a2);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterRepair16 ();
						AvsFilterRepair16 (const AvsFilterRepair16 &other);
	AvsFilterRepair16 &
						operator = (const AvsFilterRepair16 &other);
	bool				operator == (const AvsFilterRepair16 &other) const;
	bool				operator != (const AvsFilterRepair16 &other) const;

};	// class AvsFilterRepair16



//#include	"AvsFilterRepair16.hpp"



#endif	// AvsFilterRepair16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
