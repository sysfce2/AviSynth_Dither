/*****************************************************************************

        FilterBilateral.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (FilterBilateral_HEADER_INCLUDED)
#define	FilterBilateral_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"conc/ObjPool.h"
#include	"avstp.h"
#include	"AvstpWrapper.h"
#include	"BilatData.h"
#include	"BilatDataFactory.h"

#include	<emmintrin.h>

#include	<vector>

#include <cstdint>



class FilterBilateral
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	FilterBilateral	ThisType;

	enum {         RADIUS_MIN        = 2    };
	enum {         SUBSPL_MIN        = 4    };
	enum {         SUBSPL_AUTO       = 0    };
	enum {         MAX_SUBSPL_POINTS = 4096 };

	explicit			FilterBilateral (int width, int height, int radius_h, int radius_v, double threshold, double flat, double wmin, double subspl, bool src_flag, bool sse2_flag);
	virtual			~FilterBilateral () {}

	void				process_plane (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int stride_dst, int stride_src, int stride_ref);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			MAX_SEG_SIZE		= 128	};	// Must be a multiple of 32 (2 packed vectors) and fit in the stack
	enum {			NBR_POINT_LISTS	= 23	};
	enum {			MAX_THREADS			= 64	};

#if 0	// Not used
	template <class T>
	class ObjFactoryDef
	:	public conc::ObjFactoryInterface <T>
	{
	protected:
		// conc::ObjFactoryInterface
		virtual T *		do_create () { return new T; }
	};
#endif

	class MaskInfo
	{
	public:
		float *			_mask_ptr;	// Aligned
		int				_scan_x;
		int				_scan_w;
		bool				_long_flag;
	};

	class Coord
	{
	public:
		int				_x;
		int				_y;
	};

	class TaskFilterGlobal
	{
	public:
		FilterBilateral *
							_this_ptr;
		BilatData *		_bd_ptr;
		uint8_t *		_dst_msb_ptr;
		uint8_t *		_dst_lsb_ptr;
		const uint8_t*	_src_msb_ptr;
		const uint8_t*	_src_lsb_ptr;
		const uint8_t*	_ref_msb_ptr;
		const uint8_t*	_ref_lsb_ptr;
		int				_stride_dst;
		int				_stride_src;
		int				_stride_ref;
	};

	class TaskFilter
	{
	public:
		const TaskFilterGlobal *
							_glob_data_ptr;
		int				_y_beg;
		int				_y_end;
	};

	typedef	int OffsetArray [MAX_SUBSPL_POINTS * NBR_POINT_LISTS];		// We need two of them on the stack

	void				init_subspl_data ();
	void				add_subspl_point (int x, int y, int ofs, std::vector <bool> &done_arr, int &point_cnt);
	void				init_sse2_data ();
	void				init_offsets (OffsetArray &ofs_src_arr, OffsetArray &ofs_ref_arr, int stride_src, int stride_ref) const;
	double			compute_sum_w_min (int area) const;

	void				prepare_subplane (TaskFilter &td);

	void				process_subplane_cpp (TaskFilter &td);
	void				process_subplane_sse (TaskFilter &td);
	void				process_subplane_sse_subspl (TaskFilter &td);
	__forceinline static void
						process_vect_sse (__m128 &sum, __m128 &sum_w, const __m128 &cen_ref, const __m128 &cen_src, const __m128 &mask_abs, const __m128 &wmax, const __m128 &m, const __m128 &zero, const float *s_s_ptr, const float *s_r_ptr);

	static void		conv_output_segment (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const float *src_ptr, int w);
	static void		redirect_task_prepare (avstp_TaskDispatcher *dispatcher_ptr, void *data_ptr);
	static void		redirect_task_filter (avstp_TaskDispatcher *dispatcher_ptr, void *data_ptr);

	AvstpWrapper &	_avstp;
	int				_width;
	int				_height;
	int				_radius_h;
	int				_radius_v;
	double			_threshold;
	double			_flat;
	double			_wmin;
	double			_subspl;
	bool				_src_flag;
	bool				_sse2_flag;
	bool				_subspl_flag;
	int				_m;
	int				_wmax;
	int				_base_area;
	double			_sum_w_min;

	conc::ObjPool <BilatData>
						_bd_pool;
	BilatDataFactory
						_bd_factory;
	std::vector <Coord>
						_subspl_point_list;
	int				_nbr_points;

	void (ThisType::*
						_process_subplane_ptr) (TaskFilter &td);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						FilterBilateral ();
						FilterBilateral (const FilterBilateral &other);
	FilterBilateral &
						operator = (const FilterBilateral &other);
	bool				operator == (const FilterBilateral &other) const;
	bool				operator != (const FilterBilateral &other) const;

};	// class FilterBilateral



//#include	"FilterBilateral.hpp"



#endif	// FilterBilateral_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
