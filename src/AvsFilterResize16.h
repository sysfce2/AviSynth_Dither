/*****************************************************************************

        AvsFilterResize16.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (AvsFilterResize16_HEADER_INCLUDED)
#define	AvsFilterResize16_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#define	NOGDI
#include <windows.h>
#include "fmtcl/ChromaPlacement.h"
#include "fmtcl/ContFirInterface.h"
#include "fmtcl/DiscreteFirInterface.h"
#include "fmtcl/FilterResize.h"
#include "fmtcl/KernelData.h"
#include "fmtcl/ResampleSpecPlane.h"
#include "avisynth.h"
#include "PlaneProcCbInterface.h"
#include "PlaneProcessor.h"

#include <emmintrin.h>

#include <array>
#include <map>
#include <memory>
#include <mutex>

#include <cstdint>



class AvsFilterResize16
:	public GenericVideoFilter
,	public PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum OptBool
	{
		OptBool_UNDEF = -1,
		OptBool_FALSE = 0,
		OptBool_TRUE  = 1
	};

	typedef	AvsFilterResize16	ThisType;

	explicit			AvsFilterResize16 (::IScriptEnvironment *env_ptr, ::PClip src_clip_sptr, int dst_width, int dst_height, double win_x, double win_y, double win_w, double win_h, std::string kernel_fnc, double kernel_scale_h, double kernel_scale_v, int taps, bool a1_flag, double a1, bool a2_flag, double a2, bool a3_flag, double a3, int kovrspl, bool norm_flag, bool preserve_center_flag, int y, int u, int v, std::string kernel_fnc_h, std::string kernel_fnc_v, double kernel_total_h, double kernel_total_v, bool invks_flag, OptBool invks_h, OptBool invks_v, int invks_taps, std::string cplace_s, std::string cplace_d, std::string csp);
	virtual			~AvsFilterResize16 () {}

	virtual ::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			MAX_NBR_PLANES		= 3	};

	enum InterlacingType
	{
		InterlacingType_FRAME = 0,
		InterlacingType_TOP,
		InterlacingType_BOT,

		InterlacingType_NBR_ELT
	};

	class Win
	{
	public:
		double         _x;	// Data is in full coordinates whatever the plane (never subsampled)
		double         _y;
		double         _w;
		double         _h;
	};

	// Array order: [dest] [src]
	typedef std::array <fmtcl::ResampleSpecPlane, InterlacingType_NBR_ELT> SpecSrcArray;
	typedef std::array <SpecSrcArray,             InterlacingType_NBR_ELT> SpecArray;

	class PlaneData
	{
	public:
		typedef std::array <fmtcl::KernelData, fmtcl::FilterResize::Dir_NBR_ELT> KernelArray;
		Win            _win;
		SpecArray      _spec_arr;        // Contains the spec (used as a key) for each plane/interlacing combination
		KernelArray    _kernel_arr;
		double         _kernel_scale_h;  // Can be negative (forced scaling)
		double         _kernel_scale_v;  // Can be negative (forced scaling)
		double         _gain;
		double         _add_cst;
		bool           _preserve_center_flag;
	};

	typedef std::array <PlaneData, MAX_NBR_PLANES> PlaneDataArray;

	void				init_fnc (::IScriptEnvironment &env);

	fmtcl::FilterResize *
	               create_or_access_plane_filter (int plane_index, InterlacingType itl_d, InterlacingType itl_s);
	void           create_plane_specs ();

	static fmtcl::ChromaPlacement
						conv_str_to_chroma_placement (::IScriptEnvironment &env, std::string cplace);
	static int     conv_str_to_pixel_type (::IScriptEnvironment &env, std::string csp);
	static inline bool
						cumulate_flag (bool flag, OptBool flag2);
	static InterlacingType
	               get_itl_type (bool itl_flag, bool top_flag);

	::VideoInfo    _vi_src;
	::PClip			_src_clip_sptr;
	int				_src_width;
	int				_src_height;
	int				_dst_width;
	int				_dst_height;
	int				_invks_taps;
	double			_win_x;
	double			_win_y;
	double			_win_w;
	double			_win_h;
	double			_norm_val_h;
	double			_norm_val_v;
	bool				_norm_flag;
	bool				_invks_h_flag;
	bool				_invks_v_flag;
	fmtcl::ChromaPlacement
						_cplace_s;
	fmtcl::ChromaPlacement
						_cplace_d;

	std::unique_ptr <PlaneProcessor>
	               _plane_proc_uptr;
	bool				_sse2_flag;
	bool           _avx2_flag;
	std::mutex		_filter_mutex;				// To access _filter_uptr_map.
	std::map <fmtcl::ResampleSpecPlane, std::unique_ptr <fmtcl::FilterResize> >
						_filter_uptr_map;			// Created only on request. [(h<<32)+w]

	PlaneDataArray _plane_data_arr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						AvsFilterResize16 ();
						AvsFilterResize16 (const AvsFilterResize16 &other);
	AvsFilterResize16 &
						operator = (const AvsFilterResize16 &other);
	bool				operator == (const AvsFilterResize16 &other) const;
	bool				operator != (const AvsFilterResize16 &other) const;

};	// class AvsFilterResize16



//#include	"AvsFilterResize16.hpp"



#endif	// AvsFilterResize16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
