/*****************************************************************************

        AvsFilterDitherPost.h
        Author: Laurent de Soras, 2010

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterDitherPost_HEADER_INCLUDED)
#define	AvsFilterDitherPost_HEADER_INCLUDED
#pragma once

#if defined (_MSC_VER)
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#define	NOGDI
#include <windows.h>
#include "conc/ObjPool.h"
#include "fmtcl/ErrDifBufFactory.h"
#include "fstb/ArrayAlign.h"
#include "avisynth.h"
#include "MTSlicer.h"
#include "PlaneProcCbInterface.h"
#include "PlaneProcessor.h"

#include	<emmintrin.h>

#include <array>

#include <cstdint>



namespace fmtcl
{
	class ErrDifBuf;
}



class AvsFilterDitherPost
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	AvsFilterDitherPost	ThisType;

	enum Mode
	{
		Mode_ROUND = -1,
		Mode_ORDERED = 0,
		Mode_B1,
		Mode_B2L,
		Mode_B2M,
		Mode_B2S,
		Mode_B2SS,
		Mode_FLOYD,
		Mode_STUCKI,
		Mode_ATKINSON,

		Mode_NBR_ELT
	};

	enum Pattern
	{
		Pattern_REGULAR = 0,
		Pattern_ALT_V,
		Pattern_ALT_H,

		Pattern_NBR_ELT
	};

	explicit			AvsFilterDitherPost (::IScriptEnvironment *env_ptr, ::PClip msb_clip_sptr, ::PClip lsb_clip_sptr, int mode, double ampo, double ampn, int pat, bool dyn_flag, bool prot_flag, ::PClip mask_clip_sptr, double thr, bool stacked_flag, bool interlaced_flag, int y, int u, int v, bool staticnoise_flag, bool slice_flag);
	virtual			~AvsFilterDitherPost ();

	virtual ::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum Type
	{
		Type_ORDERED	= 0,
		Type_2B,
		Type_ERRDIF,

		Type_NBR_ELT
	};

	enum {			MAX_NBR_PLANES	= 3		};
	enum {			PAT_WIDTH		= 8		};	// Must be a power of 2 (recursive pattern generation) and multiple of 8 (movq).
	enum {			PAT_PERIOD		= 4		};	// Must be a power of 2 (I don't remember why)
	enum {			PAT_LEVELS		= 3		};	// For 2-bit dithering, different patterns for ranges 32-95, 96-159 and 160-223.
	enum {			MAX_SEG_LEN		= 2048	};	// To store temporary data on the stack. Must be a multiple of PAT_WIDTH and multiple of 8 (movq).
	enum {			AMP_BITS			= 5		};	// Bit depth of the amplitude fractionnal part. The whole thing is 7 bits, and we need a few bits for the integer part.

	typedef	int16_t	PatData [PAT_WIDTH] [PAT_LEVELS] [PAT_WIDTH];	// [y] [level] [x]
	typedef	fstb::ArrayAlign <PatData, PAT_PERIOD, 16>	PatDataArray;

	class TaskDataGlobal
	{
	public:
		ThisType *		_this_ptr = nullptr;
		::IScriptEnvironment *
							_env_ptr;
		int				_w;
		int				_h;
		uint8_t *		_dst_ptr;
		const uint8_t*	_msb_ptr;
		const uint8_t*	_lsb_ptr;
		const uint8_t*	_msk_ptr;
		int				_stride_dst;
		int				_stride_msb;
		int				_stride_lsb;
		int				_stride_msk;
		const PatData*	_pattern_ptr;
		uint32_t			_rnd_state;
	};

	typedef	MTSlicer <ThisType, TaskDataGlobal>	Slicer;

	class PlaneContext
	{
	public:
		Slicer         _slicer;
		TaskDataGlobal _global_data;
	};

	class FrameContext
	{
	public:
		std::mutex     _mutex;
		volatile bool  _filled_flag = false;
		::PVideoFrame  _msb_sptr;  // We need to keep these pointers until the
		::PVideoFrame  _lsb_sptr;  // we really finished reading the source data
		::PVideoFrame  _msk_sptr;  // (when all wait() exited).
		std::array <std::array <PlaneContext, 2>, PlaneProcessor::MAX_NBR_PLANES>
		               _pc_arr;
	};

	void				build_dither_pat ();
	void				build_dither_pat_round ();
	void				build_dither_pat_ordered ();
	void				build_dither_pat_b1 ();
	void				build_dither_pat_b2 (long p_0, long p_1, long p_2, int map_alt);
	void				build_next_dither_pat ();
	void				copy_dither_level (PatData &pat);
	void				copy_dither_pat_rotate (PatData &dst, const PatData &src, int angle);
	void				init_fnc (::IScriptEnvironment &env);

	void				process_plane (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const uint8_t *msk_ptr, int w, int h, int stride_dst, int stride_msb, int stride_lsb, int stride_msk, int frame_index, int plane_index, ::IScriptEnvironment &env, PlaneContext &plane_context);
	void				process_subplane (Slicer::TaskData &td);

	void				protect_segment_cpp (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag) const;
	void				protect_segment_sse2 (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag) const;
	void				protect_segment_ref (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag) const;

	void				process_segment_ord_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t &rnd_state) const;
	void				process_segment_ord_sse2 (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t &rnd_state) const;
	void				process_segment_ord_sse2_simple (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t &rnd_state) const;

	void				process_segment_2b_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t &rnd_state) const;
	void				process_segment_2b_sse2 (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t &rnd_state) const;

	void				process_segment_floydsteinberg_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, fmtcl::ErrDifBuf &ed_buf, int w, int buf_ofs, int y, uint32_t &rnd_state) const;
	void				process_segment_stucki_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr,fmtcl:: ErrDifBuf &ed_buf, int w, int buf_ofs, int y, uint32_t &rnd_state) const;
	void				process_segment_atkinson_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, fmtcl::ErrDifBuf &ed_buf, int w, int buf_ofs, int y, uint32_t &rnd_state) const;

	void				apply_mask_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *msk_ptr, int w) const;
	void				apply_mask_sse2 (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *msk_ptr, int w) const;

	static inline void
						protect_pix_generic (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int pos, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag);
	static inline void
						update_min_max (int &mi, int &ma, int val);
	static inline void
						update_min_max (__m128i &mi_0, __m128i &mi_1, __m128i &ma_0, __m128i &ma_1, const uint8_t *val_ptr);
	static inline void
						protect_pix_decide (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, int msb, const uint8_t *src_lsb_ptr, int pos, int mi, int ma);
	static inline void
						generate_rnd (uint32_t &state);
	static inline void
						generate_rnd_eol (uint32_t &state);
	static inline void
						quantize_pix (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, int x, int &err, uint32_t &rnd_state, int ampe_i, int ampn_i);
	static inline void
						diffuse_floydsteinberg (int err, int &e1, int &e3, int &e5, int &e7);
	static inline void
						diffuse_stucki (int err, int &e1, int &e2, int &e4, int &e8);
	static inline void
						diffuse_atkinson (int err, int &e1);

	::PClip        _msb_clip_sptr;
	::PClip        _lsb_clip_sptr;
	::PClip        _mask_clip_sptr;
	const ::VideoInfo
	               _vi_src;
	PlaneProcessor _plane_proc;
	int            _mode;
	double         _ampo;
	double         _ampn;
	int            _pat;
	bool           _dyn_flag;
	bool           _prot_flag;
	double         _thr;
	bool           _stacked_flag;
	bool           _interlaced_flag;
	bool           _staticnoise_flag;

	PatDataArray   _dither_pat_arr;	// For ordered dithering, it contains levels, and for fixed dithering, it contains MSB deltas.
	int            _ampo_i;				// [0 ; 127], 1.0 = 1<<AMP_BITS
	int            _ampn_i;				// [0 ; 127], 1.0 = 1<<AMP_BITS
	int            _ampe_i;				// [0 ; 2047], 1.0 = 256
	int            _thr_a;				// [-100 ; -8]
	int            _thr_b;				// [0<<8 ; 50<<8]
	bool           _threshold_flag;	// Indicates that threshold calculations are enabled
	bool           _slice_flag;      // Frame slicing is enabled (for multi-threaded error diffusion algorithms).

	Type           _dith_type;

	conc::ObjPool <fmtcl::ErrDifBuf> // For error diffusion algorithms
	               _edb_pool;
	fmtcl::ErrDifBufFactory
	               _edb_factory;

	void (ThisType::*
	               _protect_segment_ptr) (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag) const;
	void (ThisType::*
	               _process_segment_ptr) (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t &rnd_state) const;
	void (ThisType::*
	               _process_segment_errdif_ptr) (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, fmtcl::ErrDifBuf &ed_buf, int w, int buf_ofs, int y, uint32_t &rnd_state) const;
	void (ThisType::*
	               _apply_mask_ptr) (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *msk_ptr, int w) const;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterDitherPost ();
						AvsFilterDitherPost (const AvsFilterDitherPost &other);
	AvsFilterDitherPost &
						operator = (const AvsFilterDitherPost &other);
	bool				operator == (const AvsFilterDitherPost &other) const;
	bool				operator != (const AvsFilterDitherPost &other) const;

};	// class AvsFilterDitherPost



//#include	"AvsFilterDitherPost.hpp"



#endif	// AvsFilterDitherPost_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
