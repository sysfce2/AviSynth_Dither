/*****************************************************************************

        FilterBilateral.cpp
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



// Disabled because not fast enough. Increasing the subsampling to get
// the same speed is not on par quality-wise.
#undef FilterBilateral_SUBSPL_SSE2_ACCURATE



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"fstb/def.h"
#include	"fstb/fnc.h"
#include	"fstb/ToolsSse2.h"
#include "fmtcl/VoidAndCluster.h"
#include	"Borrower.h"
#include	"FilterBilateral.h"
#include	"RndGen.h"

#include <functional>
#include <random>

#include	<cassert>
#include	<cmath>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



FilterBilateral::FilterBilateral (int width, int height, int radius_h, int radius_v, double threshold, double flat, double wmin, double subspl, bool src_flag, bool sse2_flag)
:	_avstp (AvstpWrapper::use_instance ())
,	_width (width)
,	_height (height)
,	_radius_h (radius_h)
,	_radius_v (radius_v)
,	_threshold (threshold)
,	_flat (flat)
,	_wmin (wmin)
,	_subspl (subspl)
,	_src_flag (src_flag)
,	_sse2_flag (sse2_flag)
,	_subspl_flag (false)
,	_m (std::max (fstb::round_int (_threshold * 256), 1))
,	_wmax (std::max (fstb::round_int (_threshold * 256 * (1 - _flat)), 1))
,	_base_area ((radius_h * 2 - 1) * (radius_v * 2 - 1))
,	_sum_w_min (compute_sum_w_min (_base_area))
,	_bd_pool ()
,	_bd_factory (width, height, radius_h, radius_v, src_flag)
,	_subspl_point_list ()
,	_nbr_points (0)
,	_process_subplane_ptr (&ThisType::process_subplane_cpp)
{
	assert (width > 0);
	assert (height > 0);
	assert (radius_h >= RADIUS_MIN);
	assert (radius_v >= RADIUS_MIN);
	assert (threshold >= 0);
	assert (flat >= 0);
	assert (flat <= 1);
	assert (wmin >= 0);

	if (_subspl >= SUBSPL_MIN)
	{
		_subspl_flag = true;
	}
	else if (fabs (_subspl - SUBSPL_AUTO) < 1e-3)
	{
		_subspl_flag = true;
		_subspl      = radius_h + radius_v;
	}

	if (_subspl_flag)
	{
		init_subspl_data ();
	}

	if (_sse2_flag)
	{
		init_sse2_data ();
	}
}



void	FilterBilateral::process_plane (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, const uint8_t *ref_msb_ptr, const uint8_t *ref_lsb_ptr, int stride_dst, int stride_src, int stride_ref)
{
	BilatData *		bd_ptr = 0;
	if (_sse2_flag)
	{
		bd_ptr = _bd_pool.take_obj ();
		if (bd_ptr == 0)
		{
			throw std::runtime_error (
				"Dither_bilateral16: Cannot allocate buffer memory."
			);
		}
	}

	TaskFilterGlobal	tdg;
	tdg._this_ptr    = this;
	tdg._bd_ptr      = bd_ptr;
	tdg._dst_msb_ptr = dst_msb_ptr;
	tdg._dst_lsb_ptr = dst_lsb_ptr;
	tdg._src_msb_ptr = src_msb_ptr;
	tdg._src_lsb_ptr = src_lsb_ptr;
	tdg._ref_msb_ptr = ref_msb_ptr;
	tdg._ref_lsb_ptr = ref_lsb_ptr;
	tdg._stride_dst  = stride_dst;
	tdg._stride_src  = stride_src;
	tdg._stride_ref  = stride_ref;

	TaskFilter			td_arr [MAX_THREADS];

	int				nbr_threads = _avstp.get_nbr_threads ();
	nbr_threads = std::min (nbr_threads, int (MAX_THREADS));
	nbr_threads = std::min (nbr_threads, _height);

	int				y_beg = 0;
	for (int t_cnt = 0; t_cnt < nbr_threads; ++t_cnt)
	{
		const int		y_end = (t_cnt + 1) * _height / nbr_threads;
		TaskFilter &		task_data = td_arr [t_cnt];
		task_data._glob_data_ptr = &tdg;
		task_data._y_beg         = y_beg;
		task_data._y_end         = y_end;
		y_beg = y_end;
	}
	assert (y_beg == _height);

	avstp_TaskDispatcher *	task_dispatcher_ptr = _avstp.create_dispatcher ();

	// Prepare data
	if (bd_ptr != 0)
	{
		for (int t_cnt = 0; t_cnt < nbr_threads; ++t_cnt)
		{
			_avstp.enqueue_task (
				task_dispatcher_ptr,
				&redirect_task_prepare,
				&td_arr [t_cnt]
			);
		}
		_avstp.wait_completion (task_dispatcher_ptr);

		bd_ptr->mirror_pic ();
	}

	// Filter
	for (int t_cnt = 0; t_cnt < nbr_threads; ++t_cnt)
	{
		_avstp.enqueue_task (
			task_dispatcher_ptr,
			&redirect_task_filter,
			&td_arr [t_cnt]
		);
	}
	_avstp.wait_completion (task_dispatcher_ptr);

	// Done
	_avstp.destroy_dispatcher (task_dispatcher_ptr);
	task_dispatcher_ptr = 0;

	if (bd_ptr != 0)
	{
		_bd_pool.return_obj (*bd_ptr);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	FilterBilateral::init_subspl_data ()
{
	RndGen			rnd_gen;

	_nbr_points = fstb::limit (
		fstb::round_int (_base_area / _subspl),
		3,
		int (MAX_SUBSPL_POINTS)
	);

	_subspl_point_list.resize (_nbr_points * NBR_POINT_LISTS);

	const bool     spiral_flag = (_nbr_points < 32);
	const int      max_h       = _radius_h * 2 - 1;
	const int      max_v       = _radius_v * 2 - 1;

	const int      vnc_size = fstb::limit (
		std::max (max_h, max_v) * 3 / 2, 16, 32
	);
	const int     vnc_area = vnc_size * vnc_size;
	assert (vnc_area <= 65536);
	fmtcl::MatrixWrap <uint16_t>	vnc (vnc_size, vnc_size);
	if (! spiral_flag)
	{
		fmtcl::VoidAndCluster vnc_gen;
		vnc_gen.create_matrix (vnc);
	}

	std::default_random_engine rand_gen;
	std::uniform_int_distribution <int> dist_x (0, max_h - 1);
	std::uniform_int_distribution <int> dist_y (0, max_v - 1);
	std::uniform_int_distribution <int> dist_a (0, NBR_POINT_LISTS - 1);
	auto           dice_x = std::bind (dist_x, rand_gen);
	auto           dice_y = std::bind (dist_y, rand_gen);
	auto           dice_a = std::bind (dist_a, rand_gen);

	for (int list_cnt = 0; list_cnt < NBR_POINT_LISTS; ++list_cnt)
	{
		const int		ofs = list_cnt * _nbr_points;

		std::vector <bool>	done_arr (max_h * max_v, false);

		_subspl_point_list [ofs]._x = 0;
		_subspl_point_list [ofs]._y = 0;
		int				point_cnt = 1;
		done_arr [_radius_h - 1 + (_radius_v - 1) * max_h] = true;

		if (spiral_flag)
		{
			// 4-pointed spiral
			const double   angle_base = dice_a () * (fstb::PI * 0.5 / NBR_POINT_LISTS);
			const int      arm_dir    = 1 - (list_cnt & 2);
			const int		nbr_arms   = 4;
			const int		nbr_points_per_arm = (_nbr_points - point_cnt) / nbr_arms;
			const double	amul = 2 * fstb::PI / nbr_arms * arm_dir;
			for (int p_cnt = 0; p_cnt < nbr_points_per_arm; ++p_cnt)
			{
				double			pos = double (p_cnt) / nbr_points_per_arm;
				pos = pow (pos, 3.0 / 5);
				for (int arm_cnt = 0; arm_cnt < nbr_arms; ++arm_cnt)
				{
					const double	angle = angle_base + (pos * 2 + arm_cnt) * amul;
					const int		x =
						fstb::round_int (cos (angle) * pos * (_radius_h - 1));
					const int		y =
						fstb::round_int (sin (angle) * pos * (_radius_v - 1));
					add_subspl_point (x, y, ofs, done_arr, point_cnt);
				}
			}

			// Complete with random points
			while (point_cnt < _nbr_points)
			{
				const int		x = ((rnd_gen.gen_val () >> 8) % max_h) - _radius_h + 1;
				const int		y = ((rnd_gen.gen_val () >> 8) % max_v) - _radius_v + 1;
				add_subspl_point (x, y, ofs, done_arr, point_cnt);
			}
		}

		else
		{
			const int		win_w   = _radius_h * 2 - 1;
			const int		win_h   = _radius_v * 2 - 1;
			const int      ofs_x   = dice_x ();
			const int      ofs_y   = dice_y ();
			int            cur_lvl = 0;
			int            trg_lvl = fstb::floor_int (vnc_area / _subspl);
			while (point_cnt < _nbr_points)
			{
				for (int y = 0; y < win_w && point_cnt < _nbr_points; ++y)
				{
					for (int x = 0; x < win_h && point_cnt < _nbr_points; ++x)
					{
						const int v = vnc (x + ofs_x, y + ofs_y);
						if (v >= cur_lvl && v < trg_lvl)
						{
							add_subspl_point (
								x - (_radius_h - 1),
								y - (_radius_v - 1),
								ofs,
								done_arr,
								point_cnt
							);
						}
					}
				}

				cur_lvl = trg_lvl;
				++ trg_lvl;
			}
		}
	}

	_sum_w_min = compute_sum_w_min (_nbr_points);
}



void	FilterBilateral::add_subspl_point (int x, int y, int ofs, std::vector <bool> &done_arr, int &point_cnt)
{
	assert (x >= 1 - _radius_h);
	assert (x <= _radius_h - 1);
	assert (y >= 1 - _radius_v);
	assert (y <= _radius_v - 1);
	assert (point_cnt < _nbr_points);

	const int		da_coord =
		  (x + _radius_h - 1)
		+ (y + _radius_v - 1) * (_radius_h * 2 - 1);

	if (! done_arr [da_coord])
	{
		_subspl_point_list [ofs + point_cnt]._x = x;
		_subspl_point_list [ofs + point_cnt]._y = y;

		done_arr [da_coord] = true;
		++ point_cnt;
	}
}



void	FilterBilateral::init_sse2_data ()
{
	_bd_pool.set_factory (_bd_factory);

	if (_subspl_flag)
	{
		_process_subplane_ptr = &ThisType::process_subplane_sse_subspl;
	}
	else
	{
		_process_subplane_ptr = &ThisType::process_subplane_sse;
	}
}



// Set stride_ref to 0 to prevent ofs_ref_arr initialisation.
void	FilterBilateral::init_offsets (OffsetArray &ofs_src_arr, OffsetArray &ofs_ref_arr, int stride_src, int stride_ref) const
{
	assert (_subspl_flag);
	assert (&ofs_src_arr != 0);
	assert (&ofs_ref_arr != 0);

	const int		total_nbr_points = _nbr_points * NBR_POINT_LISTS;
	if (stride_ref == 0)
	{
		for (int i = 0; i < total_nbr_points; ++i)
		{
			const Coord &	p = _subspl_point_list [i];
			ofs_src_arr [i] = p._y * stride_src + p._x;
		}
	}
	else
	{
		for (int i = 0; i < total_nbr_points; ++i)
		{
			const Coord &	p = _subspl_point_list [i];
			ofs_src_arr [i] = p._y * stride_src + p._x;
			ofs_ref_arr [i] = p._y * stride_ref + p._x;
		}
	}
}



double	FilterBilateral::compute_sum_w_min (int area) const
{
	assert (area > 0);

	const double	sum = std::max (double (_wmin * _wmax * area), 1.0);

	return (sum);
}



// SSE2
// All data stored in the float and min/max buffers are signed conversions
// of the original data.
void	FilterBilateral::prepare_subplane (TaskFilter &td)
{
	assert (&td != 0);

#if (fstb_ARCHI == fstb_ARCHI_X86)
 #if ! defined (_WIN64) && ! defined (__64BIT__) && ! defined (__amd64__) && ! defined (__x86_64__)
	// We can arrive here from any thread and we don't know the state of the
	// FP/MMX registers (it could have been changed by the previous filter that
	// used avstp) so we explicitely switch to the FP registers as we're going
	// to use them.
	_mm_empty ();
 #endif
#endif

	const int		y_beg = td._y_beg;
	const int		y_end = td._y_end;
	const TaskFilterGlobal &	tdg = *(td._glob_data_ptr);
	assert (this == tdg._this_ptr);

	BilatData &		bd          = *tdg._bd_ptr;
	const int		stride_ref  = tdg._stride_ref;
	const uint8_t*	ref_msb_ptr = tdg._ref_msb_ptr + stride_ref * y_beg;
	const uint8_t*	ref_lsb_ptr = tdg._ref_lsb_ptr + stride_ref * y_beg;

	assert (ref_msb_ptr != 0);
	assert (ref_lsb_ptr != 0);
	assert (stride_ref > 0);
	assert (y_beg >= 0);
	assert (y_beg < y_end);
	assert (y_end <= _height);

	float *			ref_ptr = bd.get_cache_ref_ptr ();
	const int		stride_in = bd.get_cache_in_stride ();
	ref_ptr += stride_in * y_beg;
	const int		w = bd.get_width ();

	const __m128i	sign_32  = _mm_set1_epi32 (+0x8000);
	const __m128i	zero = _mm_setzero_si128 ();

	for (int y = y_beg; y < y_end; ++y)
	{
		for (int x = 0; x < w; x += 8)
		{
			const __m128i	ref = fstb::ToolsSse2::load_8_16ml (
				ref_msb_ptr + x,
				ref_lsb_ptr + x
			);
			__m128i			ref_03   = _mm_unpacklo_epi16 (ref, zero);
			__m128i			ref_47   = _mm_unpackhi_epi16 (ref, zero);
			ref_03 = _mm_sub_epi32 (ref_03, sign_32);
			ref_47 = _mm_sub_epi32 (ref_47, sign_32);
			const __m128	ref_f_03 = _mm_cvtepi32_ps (ref_03);
			const __m128	ref_f_47 = _mm_cvtepi32_ps (ref_47);
			_mm_store_ps (ref_ptr + x    , ref_f_03);
			_mm_store_ps (ref_ptr + x + 4, ref_f_47);
		}

		ref_msb_ptr += stride_ref;
		ref_lsb_ptr += stride_ref;
		ref_ptr     += stride_in;
	}

	// If we have two different clips for the reference and the source.
	if (bd.has_src ())
	{
		const int		stride_src  = tdg._stride_src;
		const uint8_t*	src_msb_ptr = tdg._src_msb_ptr + stride_src * y_beg;
		const uint8_t*	src_lsb_ptr = tdg._src_lsb_ptr + stride_src * y_beg;

		assert (stride_src > 0);
		assert (src_msb_ptr != 0);
		assert (src_lsb_ptr != 0);

		float *			src_ptr = bd.get_cache_src_ptr ();
		src_ptr += stride_in * y_beg;

		for (int y = y_beg; y < y_end; ++y)
		{
			for (int x = 0; x < w; x += 8)
			{
				const __m128i	src = fstb::ToolsSse2::load_8_16ml (
					src_msb_ptr + x,
					src_lsb_ptr + x
				);
				__m128i			src_03   = _mm_unpacklo_epi16 (src, zero);
				__m128i			src_47   = _mm_unpackhi_epi16 (src, zero);
				src_03 = _mm_sub_epi32 (src_03, sign_32);
				src_47 = _mm_sub_epi32 (src_47, sign_32);
				const __m128	src_f_03 = _mm_cvtepi32_ps (src_03);
				const __m128	src_f_47 = _mm_cvtepi32_ps (src_47);
				_mm_store_ps (src_ptr + x    , src_f_03);
				_mm_store_ps (src_ptr + x + 4, src_f_47);
			}

			src_msb_ptr += stride_src;
			src_lsb_ptr += stride_src;
			src_ptr     += stride_in;
		}
	}
}



// Very slow. Kept mainly for testing and reference.
void	FilterBilateral::process_subplane_cpp (TaskFilter &td)
{
	assert (&td != 0);

#if (fstb_ARCHI == fstb_ARCHI_X86)
 #if ! defined (_WIN64) && ! defined (__64BIT__) && ! defined (__amd64__) && ! defined (__x86_64__)
	_mm_empty ();
 #endif
#endif

	const int		y_beg = td._y_beg;
	const int		y_end = td._y_end;
	const TaskFilterGlobal &	tdg = *(td._glob_data_ptr);
	assert (this == tdg._this_ptr);

	const int		stride_dst  = tdg._stride_dst;
	const int		stride_src  = tdg._stride_src;
	const int		stride_ref  = tdg._stride_ref;
	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + stride_dst * y_beg;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + stride_dst * y_beg;
	const uint8_t*	src_msb_ptr = tdg._src_msb_ptr;
	const uint8_t*	src_lsb_ptr = tdg._src_lsb_ptr;
	const uint8_t*	ref_msb_ptr = tdg._ref_msb_ptr;
	const uint8_t*	ref_lsb_ptr = tdg._ref_lsb_ptr;

	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (ref_msb_ptr != 0);
	assert (ref_lsb_ptr != 0);
	assert (stride_dst > 0);
	assert (stride_src > 0);
	assert (stride_ref > 0);
	assert (y_beg >= 0);
	assert (y_beg < y_end);
	assert (y_end <= _height);

	const int		m         = _m;
	const int		wmax      = _wmax;
	const int		sum_w_min = fstb::round_int (_sum_w_min);

	int				subspl_list_index = 0;
	const int		subspl_list_stop = _nbr_points * NBR_POINT_LISTS;
	OffsetArray		ofs_src_arr;
	OffsetArray		ofs_ref_arr;
	RndGen			rnd_gen;
	if (_subspl_flag)
	{
		init_offsets (ofs_src_arr, ofs_ref_arr, stride_src, stride_ref);
	}

	for (int y = y_beg; y < y_end; ++y)
	{
		const int		yt = std::max (y - _radius_v + 1, 0);
		const int		yb = std::min (y + _radius_v, _height - 1);

		const int		offset_c_ref = y * stride_ref;
		const uint8_t*	c_rm_ptr = ref_msb_ptr + offset_c_ref;
		const uint8_t*	c_rl_ptr = ref_lsb_ptr + offset_c_ref;

		const int		offset_c_src = y * stride_src;
		const uint8_t*	c_sm_ptr = src_msb_ptr + offset_c_src;
		const uint8_t*	c_sl_ptr = src_lsb_ptr + offset_c_src;

		const int		offset_s_src = yt * stride_src;
		const int		offset_s_ref = yt * stride_ref;

		if (_subspl_flag)
		{
			subspl_list_index =
				_nbr_points * ((rnd_gen.gen_val () >> 8) % NBR_POINT_LISTS);
		}

		for (int x = 0; x < _width; ++x)
		{
			const int		cen_src = (c_sm_ptr [x] << 8) | c_sl_ptr [x];
			const int		cen_ref = (c_rm_ptr [x] << 8) | c_rl_ptr [x];
			int64_t			sum_v = 0;	// Stores values relative to cen_src.
			int				sum_w = 0;

			if (_subspl_flag)
			{
				const uint8_t*	s_sm_ptr = c_sm_ptr + x;
				const uint8_t*	s_sl_ptr = c_sl_ptr + x;
				
				const uint8_t*	s_rm_ptr = c_rm_ptr + x;
				const uint8_t*	s_rl_ptr = c_rl_ptr + x;

				const int *		loc_ofs_src_arr = ofs_src_arr + subspl_list_index;
				const int *		loc_ofs_ref_arr = ofs_ref_arr + subspl_list_index;

				const int		min_ofs_src = -offset_c_src - x;
				const int		max_ofs_src = min_ofs_src + _height * stride_src;
				const int		block_size = 64;
				for (int block = 0; block < _nbr_points; block += block_size)
				{
					int				sum_h = 0;

					const int		block_end = std::min (block + block_size, _nbr_points);
					for (int i = block; i < block_end; ++i)
					{
						const int		o_s = loc_ofs_src_arr [i];
						const int		o_r = loc_ofs_ref_arr [i];
						if (o_s >= min_ofs_src && o_s < max_ofs_src)
						{
							const int		src = (s_sm_ptr [o_s] << 8) | s_sl_ptr [o_s];
							const int		ref = (s_rm_ptr [o_r] << 8) | s_rl_ptr [o_r];
							const int		dist = abs (ref - cen_ref);
							const int		weight = fstb::limit (m - dist, 0, wmax);

							sum_w += weight;
							sum_h += weight * (src - cen_src);
						}
					}

					sum_v += sum_h;
				}

				subspl_list_index += _nbr_points;
				if (subspl_list_index >= subspl_list_stop)
				{
					subspl_list_index = 0;
				}
			}

			else
			{
				const int		xl = std::max (x - _radius_h + 1, 0);
				const int		xr = std::min (x + _radius_h, _width - 1);

				const uint8_t*	s_sm_ptr = src_msb_ptr + offset_s_src;
				const uint8_t*	s_sl_ptr = src_lsb_ptr + offset_s_src;

				const uint8_t*	s_rm_ptr = ref_msb_ptr + offset_s_ref;
				const uint8_t*	s_rl_ptr = ref_lsb_ptr + offset_s_ref;

				for (int ys = yt; ys < yb; ++ys)
				{
					int				sum_h = 0;

					for (int xs = xl; xs < xr; ++xs)
					{
						const int		src = (s_sm_ptr [xs] << 8) | s_sl_ptr [xs];
						const int		ref = (s_rm_ptr [xs] << 8) | s_rl_ptr [xs];
						const int		dist = abs (ref - cen_ref);
						const int		weight = fstb::limit (m - dist, 0, wmax);

						sum_w += weight;
						sum_h += weight * (src - cen_src);
					}

					sum_v += sum_h;

					s_sm_ptr += stride_src;
					s_sl_ptr += stride_src;
					s_rm_ptr += stride_ref;
					s_rl_ptr += stride_ref;
				}
			}

			sum_w = std::max (sum_w, sum_w_min);
			const int		delta = int (sum_v / sum_w);
			const int		p = cen_src + delta;

			dst_msb_ptr [x] = uint8_t (p >> 8);
			dst_lsb_ptr [x] = uint8_t (p     );
		}

		dst_msb_ptr += stride_dst;
		dst_lsb_ptr += stride_dst;
	}
}



void	FilterBilateral::process_subplane_sse (TaskFilter &td)
{
	assert (&td != 0);

#if (fstb_ARCHI == fstb_ARCHI_X86)
 #if ! defined (_WIN64) && ! defined (__64BIT__) && ! defined (__amd64__) && ! defined (__x86_64__)
	_mm_empty ();
 #endif
#endif


	const int		y_beg = td._y_beg;
	const int		y_end = td._y_end;
	const TaskFilterGlobal &	tdg = *(td._glob_data_ptr);
	assert (this == tdg._this_ptr);

	BilatData &		bd          = *tdg._bd_ptr;
	const int		stride_dst  = tdg._stride_dst;
	const int		stride_src  = tdg._stride_src;
	const int		stride_ref  = tdg._stride_ref;
	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + stride_dst * y_beg;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + stride_dst * y_beg;

	assert (&bd != 0);
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (stride_dst > 0);
	assert (stride_src > 0);
	assert (stride_ref > 0);
	assert (y_beg >= 0);
	assert (y_beg < y_end);
	assert (y_end <= _height);

	// Could be replaced by fstb::ArrayAlign for portability
	__declspec (align (16)) float	out_buf [MAX_SEG_SIZE];

	const float *	ref_line_ptr = bd.get_cache_ref_ptr ();
	const float *	src_line_ptr = bd.get_cache_src_ptr ();
	const int		stride_in    = bd.get_cache_in_stride ();
	ref_line_ptr += stride_in * y_beg;
	src_line_ptr += stride_in * y_beg;
	const int		offset_y  = stride_in * (1 - _radius_v);
	const int		win_h     = _radius_v * 2 - 1;

	const __m128	mask_abs  = _mm_castsi128_ps (_mm_set1_epi32 (0x7FFFFFFF));
	const __m128	m         = _mm_set1_ps (float (_m));
	const __m128	wmax      = _mm_set1_ps (float (_wmax));
	const __m128	sum_w_min = _mm_set1_ps (float (_sum_w_min));
	const __m128	zero      = _mm_setzero_ps ();

	for (int dest_y = y_beg; dest_y < y_end; ++dest_y)
	{
		int				seg_x = 0;
		do
		{
			const int		seg_end  = std::min (seg_x + MAX_SEG_SIZE, _width);
			float * const	dest_ptr = out_buf - seg_x;

			for (int dest_x = seg_x; dest_x < seg_end; dest_x += 4)
			{
				const float *	cen_src_ptr  = src_line_ptr + dest_x;
				const float *	cen_ref_ptr  = ref_line_ptr + dest_x;
				const __m128	cen_src      = _mm_loadu_ps (cen_src_ptr);
				const __m128	cen_ref      = _mm_loadu_ps (cen_ref_ptr);

				__m128			sum          = zero;	// Stores values relative to cen_src.
				__m128			sum_w        = zero;

				const float *	s_s_ptr      = src_line_ptr + offset_y;
				const float *	s_r_ptr      = ref_line_ptr + offset_y;
				const int		dx_end = dest_x + _radius_h;
				for (int ys = 1 - _radius_v; ys < _radius_v; ++ys)
				{
					for (int xs = dest_x + 1 - _radius_h; xs < dx_end; ++xs)
					{
						process_vect_sse (
							sum, sum_w, cen_ref, cen_src, mask_abs, wmax, m, zero,
							s_s_ptr + xs, s_r_ptr + xs
						);
					}

					s_s_ptr += stride_in;
					s_r_ptr += stride_in;
				}

				sum_w = _mm_max_ps (sum_w, sum_w_min);
				const __m128	delta = _mm_div_ps (sum, sum_w);
				const __m128	p     = _mm_add_ps (cen_src, delta);
				_mm_store_ps (dest_ptr + dest_x, p);

			}	// for dest_x

			conv_output_segment (
				dst_msb_ptr + seg_x,
				dst_lsb_ptr + seg_x,
				out_buf,
				seg_end - seg_x
			);

			seg_x = seg_end;
		}
		while (seg_x < _width);

		dst_msb_ptr += stride_dst;
		dst_lsb_ptr += stride_dst;

		ref_line_ptr += stride_in;
		src_line_ptr += stride_in;

	}	// for dest_y
}



void	FilterBilateral::process_subplane_sse_subspl (TaskFilter &td)
{
	assert (&td != 0);

#if (fstb_ARCHI == fstb_ARCHI_X86)
 #if ! defined (_WIN64) && ! defined (__64BIT__) && ! defined (__amd64__) && ! defined (__x86_64__)
	_mm_empty ();
 #endif
#endif


	const int		y_beg = td._y_beg;
	const int		y_end = td._y_end;
	const TaskFilterGlobal &	tdg = *(td._glob_data_ptr);
	assert (this == tdg._this_ptr);

	BilatData &		bd          = *tdg._bd_ptr;
	const int		stride_dst  = tdg._stride_dst;
	const int		stride_src  = tdg._stride_src;
	const int		stride_ref  = tdg._stride_ref;
	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + stride_dst * y_beg;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + stride_dst * y_beg;

	assert (_subspl_flag);
	assert (&bd != 0);
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (stride_dst > 0);
	assert (stride_src > 0);
	assert (stride_ref > 0);
	assert (y_beg >= 0);
	assert (y_beg < y_end);
	assert (y_end <= _height);

	// Could be replaced by fstb::ArrayAlign for portability
	__declspec (align (16)) float	out_buf [MAX_SEG_SIZE];
#if defined (FilterBilateral_SUBSPL_SSE2_ACCURATE)
	__declspec (align (16)) float	src_buf [MAX_SUBSPL_POINTS] [4];	// Actual size is _nbr_points
	__declspec (align (16)) float	ref_buf [MAX_SUBSPL_POINTS] [4];	// Actual size is _nbr_points
	__declspec (align (16)) float	cen_src_buf [4];
	__declspec (align (16)) float	cen_ref_buf [4];
#endif

	const float *	ref_line_ptr = bd.get_cache_ref_ptr ();
	const float *	src_line_ptr = bd.get_cache_src_ptr ();
	const int		stride_in    = bd.get_cache_in_stride ();
	ref_line_ptr += stride_in * y_beg;
	src_line_ptr += stride_in * y_beg;
	const int		win_h = _radius_v * 2 - 1;

	int				subspl_list_index = 0;
	const int		subspl_list_stop = _nbr_points * NBR_POINT_LISTS;
	OffsetArray		ofs_src_arr;
	RndGen			rnd_gen;
	init_offsets (ofs_src_arr, ofs_src_arr, stride_in, 0);

	const __m128	mask_abs  = _mm_castsi128_ps (_mm_set1_epi32 (0x7FFFFFFF));
	const __m128	m         = _mm_set1_ps (float (_m));
	const __m128	wmax      = _mm_set1_ps (float (_wmax));
	const __m128	sum_w_min = _mm_set1_ps (float (_sum_w_min));
	const __m128	zero      = _mm_setzero_ps ();

	for (int dest_y = y_beg; dest_y < y_end; ++dest_y)
	{
		subspl_list_index =
			_nbr_points * ((rnd_gen.gen_val () >> 8) % NBR_POINT_LISTS);

		int				seg_x = 0;
		do
		{
			const int		seg_end  = std::min (seg_x + MAX_SEG_SIZE, _width);
			float * const	dest_ptr = out_buf - seg_x;

			for (int dest_x = seg_x; dest_x < seg_end; dest_x += 4)
			{
				__m128			sum   = zero;	// Stores values relative to cen_src.
				__m128			sum_w = zero;

#if defined (FilterBilateral_SUBSPL_SSE2_ACCURATE)

				for (int vect_x = 0; vect_x < 4; ++vect_x)
				{
					const int *		loc_ofs_src_ptr = ofs_src_arr + subspl_list_index;
					const float *	cen_ref_ptr     = ref_line_ptr + dest_x + vect_x;
					const float *	cen_src_ptr     = src_line_ptr + dest_x + vect_x;
					cen_ref_buf [vect_x] = *cen_ref_ptr;
					cen_src_buf [vect_x] = *cen_src_ptr;
					for (int point_cnt = 0; point_cnt < _nbr_points; ++point_cnt)
					{
						const int		offset = loc_ofs_src_ptr [point_cnt];
						src_buf [point_cnt] [vect_x] = cen_src_ptr [offset];
						ref_buf [point_cnt] [vect_x] = cen_ref_ptr [offset];
					}

					subspl_list_index += _nbr_points;
					if (subspl_list_index >= subspl_list_stop)
					{
						subspl_list_index = 0;
					}
				}

				const __m128	cen_ref = _mm_loadu_ps (cen_ref_buf);
				const __m128	cen_src = _mm_loadu_ps (cen_src_buf);

				for (int point_cnt = 0; point_cnt < _nbr_points; ++point_cnt)
				{
					process_vect_sse (
						sum, sum_w, cen_ref, cen_src, mask_abs, wmax, m, zero, 
						src_buf [point_cnt], ref_buf [point_cnt]
					);
				}

#else // FilterBilateral_SUBSPL_SSE2_ACCURATE

				const int *		loc_ofs_src_ptr = ofs_src_arr + subspl_list_index;
				const float *	cen_ref_ptr     = ref_line_ptr + dest_x;
				const float *	cen_src_ptr     = src_line_ptr + dest_x;
				const __m128	cen_ref         = _mm_loadu_ps (cen_ref_ptr);
				const __m128	cen_src         = _mm_loadu_ps (cen_src_ptr);

				for (int point_cnt = 0; point_cnt < _nbr_points; ++point_cnt)
				{
					const int		offset = loc_ofs_src_ptr [point_cnt];
					process_vect_sse (
						sum, sum_w, cen_ref, cen_src, mask_abs, wmax, m, zero, 
						cen_src_ptr + offset, cen_ref_ptr + offset
					);
				}

#endif   // FilterBilateral_SUBSPL_SSE2_ACCURATE

				sum_w = _mm_max_ps (sum_w, sum_w_min);
				const __m128	delta = _mm_div_ps (sum, sum_w);
				const __m128	p     = _mm_add_ps (cen_src, delta);
				_mm_store_ps (dest_ptr + dest_x, p);

				subspl_list_index += _nbr_points;
				if (subspl_list_index >= subspl_list_stop)
				{
					subspl_list_index = 0;
				}

			}	// for dest_x

			conv_output_segment (
				dst_msb_ptr + seg_x,
				dst_lsb_ptr + seg_x,
				out_buf,
				seg_end - seg_x
			);

			seg_x = seg_end;
		}
		while (seg_x < _width);

		dst_msb_ptr += stride_dst;
		dst_lsb_ptr += stride_dst;

		ref_line_ptr += stride_in;
		src_line_ptr += stride_in;

	}	// for dest_y
}



void	FilterBilateral::process_vect_sse (__m128 &sum, __m128 &sum_w, const __m128 &cen_ref, const __m128 &cen_src, const __m128 &mask_abs, const __m128 &wmax, const __m128 &m, const __m128 &zero, const float *s_s_ptr, const float *s_r_ptr)
{
	const __m128	src      = _mm_loadu_ps (s_s_ptr);
	const __m128	ref      = _mm_loadu_ps (s_r_ptr);
	__m128			dist_ref = _mm_sub_ps (ref, cen_ref);
	dist_ref                = _mm_and_ps (dist_ref, mask_abs);
	__m128			weight   = _mm_sub_ps (m, dist_ref);
	weight                  = _mm_max_ps (_mm_min_ps (weight, wmax), zero);
	const __m128	diff_src = _mm_sub_ps (src, cen_src);
	const __m128	diff_wei = _mm_mul_ps (diff_src, weight);

	sum_w = _mm_add_ps (sum_w, weight);
	sum   = _mm_add_ps (sum, diff_wei);
}



void	FilterBilateral::conv_output_segment (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const float *src_ptr, int w)
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (w > 0);

	const __m128i	mask_lsb = _mm_set1_epi16 (0x00FF);
	const __m128i	sign_bit = _mm_set1_epi16 (-0x8000);

	for (int x = 0; x < w; x += 8)
	{
		const __m128	val_03_f = _mm_load_ps (src_ptr + x    );
		const __m128	val_47_f = _mm_load_ps (src_ptr + x + 4);

		const __m128i	val_03 = _mm_cvtps_epi32 (val_03_f);
		const __m128i	val_47 = _mm_cvtps_epi32 (val_47_f);

		__m128i			val = _mm_packs_epi32 (val_03, val_47);
		val = _mm_xor_si128 (val, sign_bit);

		fstb::ToolsSse2::store_8_16ml (
			dst_msb_ptr + x,
			dst_lsb_ptr + x,
			val,
			mask_lsb
		);
	}
}



void	FilterBilateral::redirect_task_prepare (avstp_TaskDispatcher * /*dispatcher_ptr*/, void *data_ptr)
{
	TaskFilter *		td_ptr = reinterpret_cast <TaskFilter *> (data_ptr);
	FilterBilateral *	this_ptr = td_ptr->_glob_data_ptr->_this_ptr;

	this_ptr->prepare_subplane (*td_ptr);
}



void	FilterBilateral::redirect_task_filter (avstp_TaskDispatcher * /*dispatcher_ptr*/, void *data_ptr)
{
	TaskFilter *		td_ptr = reinterpret_cast <TaskFilter *> (data_ptr);
	FilterBilateral *	this_ptr = td_ptr->_glob_data_ptr->_this_ptr;

	((*this_ptr).*(this_ptr->_process_subplane_ptr)) (*td_ptr);
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
