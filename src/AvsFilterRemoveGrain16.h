/*****************************************************************************

        AvsFilterRemoveGrain16.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterRemoveGrain16_HEADER_INCLUDED)
#define	AvsFilterRemoveGrain16_HEADER_INCLUDED

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



class AvsFilterRemoveGrain16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterRemoveGrain16	ThisType;

	explicit			AvsFilterRemoveGrain16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, int mode, int modeu, int modev);
	virtual			~AvsFilterRemoveGrain16 () {}

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

	class LineProcAll
	{
	public:
		static inline bool skip_line (int /*y*/) { return (false); }
	};
	class LineProcEven
	{
	public:
		static inline bool skip_line (int y) { return ((y & 1) != 0); }
	};
	class LineProcOdd
	{
	public:
		static inline bool skip_line (int y) { return ((y & 1) == 0); }
	};

	class OpRG00
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG01
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG02
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG03
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG04
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG05
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG06
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG07
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG08
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG09
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG10
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG11
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG12
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG1314
	{
	public:
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
	};
	class OpRG13 : public OpRG1314, public LineProcEven { };
	class OpRG14 : public OpRG1314, public LineProcOdd  { };
	class OpRG1516
	{
	public:
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
	};
	class OpRG15 : public OpRG1516, public LineProcEven { };
	class OpRG16 : public OpRG1516, public LineProcOdd  { };
	class OpRG17
	:	public LineProcAll
	{
	public:
		typedef	ConvSigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG18
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG19
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG20
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	class OpRG21
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	private:
		static const __declspec (align (16)) uint16_t
							_bit0 [8];
	};
	class OpRG22
	:	public LineProcAll
	{
	public:
		typedef	ConvUnsigned	ConvSign;
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
		static __forceinline __m128i
							rg (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, __m128i mask_sign);
	};
	// Didée, 26. May 2007, 14:06, http://forum.gleitz.info/archive/index.php/t-34282.html
	// "mode=23 ist völlig unbrauchbar, imho."
	// "mode=24 ist etwas schwächer, d.h. er zerstört einfach nur etwas weniger. ;)"
	// No need to optimize these modes
	class OpRG23
	:	public LineProcAll
	{
	public:
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
	};
	class OpRG24
	:	public LineProcAll
	{
	public:
		static __forceinline int
							rg (int c, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
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
		const uint8_t*	_src_msb_ptr;
		const uint8_t*	_src_lsb_ptr;
		int				_stride_dst;
		int				_stride_src;
	};

	typedef	MTSlicer <ThisType, TaskDataGlobal>	Slicer;

	void				process_subplane (Slicer::TaskData &td);

	template <class OP>
	class PlaneProc
	{
	public:
		static void		process_subplane_cpp (Slicer::TaskData &td);
		static __forceinline void
							process_row_cpp (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_src, int x_beg, int x_end);
		static void		process_subplane_sse2 (Slicer::TaskData &td);
	};

	::PClip			_src_clip_sptr;
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

						AvsFilterRemoveGrain16 ();
						AvsFilterRemoveGrain16 (const AvsFilterRemoveGrain16 &other);
	AvsFilterRemoveGrain16 &
						operator = (const AvsFilterRemoveGrain16 &other);
	bool				operator == (const AvsFilterRemoveGrain16 &other) const;
	bool				operator != (const AvsFilterRemoveGrain16 &other) const;

};	// class AvsFilterRemoveGrain16



//#include	"AvsFilterRemoveGrain16.hpp"



#endif	// AvsFilterRemoveGrain16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
