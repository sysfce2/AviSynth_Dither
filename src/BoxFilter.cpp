/*****************************************************************************

        BoxFilter.cpp
        Author: Laurent de Soras, 2010

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



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fstb/ToolsSse2.h"
#include "BoxFilter.h"

#include <algorithm>

#include <cassert>
#include <cstring>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



BoxFilter::BoxFilter (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int h, int stride_dst, int stride_src_msb, int stride_src_lsb, int col_x, int col_w, int radius_h, int radius_v, AddLinePtr add_line_ptr, AddSubLinePtr addsub_line_ptr)
:	_sum_v ()
,	_sum_h (0)
,	_src_msb_ptr (src_msb_ptr)
,	_src_lsb_ptr (src_lsb_ptr)
,	_w (w)
,	_h (h)
,	_stride_src_msb (stride_src_msb)
,	_stride_src_lsb (stride_src_lsb)
,	_col_x (col_x)
,	_col_w (col_w)
,	_radius_h (radius_h)
,	_radius_v (radius_v)
,	_weight ((radius_h * 2 - 1) * (radius_v * 2 - 1))
,	_weight_mul ((unsigned int) ((int64_t (1) << 32) / _weight))
,	_w_round (_weight >> 1)
,	_x_l (MARGIN - radius_h)
,	_x_r (MARGIN + col_w + radius_h)
,	_mirror_l_flag (col_x == 0)
,	_mirror_r_flag (col_x + col_w + radius_h >= w)
,	_copy_l_pos_src (std::max (col_x - radius_h, 0))
,	_copy_l_pos (_copy_l_pos_src - col_x)
,	_copy_r_pos (std::min (w - col_x, col_w + radius_h))
,	_copy_len (_copy_r_pos - _copy_l_pos)
,	_addsub_line_ptr (addsub_line_ptr)
,	_add_line_ptr (add_line_ptr)
,	_y (0)
,	_pos_pos (0)
,	_pos_neg (0)
{
	fstb::unused (stride_dst);

	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (stride_dst != 0);
	assert (w > 0);
	assert (h > 0);
	assert (col_x >= 0);
	assert (col_w > 0);
	assert (col_x + col_w <= w);
	assert (radius_h <= MAX_RADIUS);
	assert (radius_v <= MAX_RADIUS);
	assert (add_line_ptr != 0);
	assert (addsub_line_ptr != 0);

	init_sum_v ();
}



void	BoxFilter::init_sum_v ()
{
	memset (&_sum_v [_x_l], 0, (_x_r - _x_l) * sizeof (_sum_v [0]));

	// Starts with lines [-radius_v ; radius_v-2] in the buffer (y = -1)
	for (int y = 0; y < _radius_v; ++y)
	{
		if (y == _radius_v - 1 && y > 0)
		{
			for (int x = _copy_l_pos; x < _copy_r_pos; ++x)
			{
				_sum_v [x + MARGIN] <<= 1;
			}
		}

		const int		ofs_msb = y * _stride_src_msb + _col_x;
		const int		ofs_lsb = y * _stride_src_lsb + _col_x;
		(*_add_line_ptr) (
			_sum_v,
			_src_msb_ptr + ofs_msb,
			_src_lsb_ptr + ofs_lsb,
			_copy_l_pos,
			_copy_len
		);
	}

	_y = 0;
}



void	BoxFilter::filter_line ()
{
	assert (_y >= 0);
	assert (_y < _h);

	// Add the bottom line to the running sum and subtract the top line.
	{
		const int		add_y = mirror_pos (_y + _radius_v - 1, _h);
		const int		ofs_add_msb = add_y * _stride_src_msb + _col_x;
		const int		ofs_add_lsb = add_y * _stride_src_lsb + _col_x;

		const int		sub_y = mirror_pos (_y - _radius_v, _h);
		const int		ofs_sub_msb = sub_y * _stride_src_msb + _col_x;
		const int		ofs_sub_lsb = sub_y * _stride_src_lsb + _col_x;

		(*_addsub_line_ptr) (
			_sum_v,
			_src_msb_ptr + ofs_add_msb,
			_src_lsb_ptr + ofs_add_lsb,
			_src_msb_ptr + ofs_sub_msb,
			_src_lsb_ptr + ofs_sub_lsb,
			_copy_l_pos,
			_copy_len
		);
	}

	// Mirrors data
	if (_mirror_l_flag)
	{
		const int		mir_len = _radius_h + _copy_l_pos;
		for (int x = 0; x < mir_len; ++x)
		{
			_sum_v [MARGIN - 1 + _copy_l_pos - x] =
				_sum_v [MARGIN + _copy_l_pos + x];
		}
	}
	if (_mirror_r_flag)
	{
		const int		mir_len = _col_w + _radius_h - _copy_r_pos;
		for (int x = 0; x < mir_len; ++x)
		{
			_sum_v [MARGIN + _copy_r_pos + x] =
				_sum_v [MARGIN - 1 + _copy_r_pos - x];
		}
	}

	++ _y;
}



void	BoxFilter::init_sum_h ()
{
	_sum_h = 0;
	for (int x = -_radius_h; x < _radius_h - 1; ++x)
	{
		_sum_h += _sum_v [MARGIN + x];
	}

	_pos_neg = MARGIN - _radius_h;		// MARGIN + x - _radius_h;
	_pos_pos = MARGIN + _radius_h - 1;	// MARGIN + x + _radius_h - 1;
}



void	BoxFilter::add_line_cpp (SegBuffer &sum_v, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int pos, int w)
{
	const int		beg = pos;
	const int		end = beg + w;
	assert (beg + MARGIN >= 0);
	assert (end + MARGIN <= sum_v.size ());

	for (int x = beg; x < end; ++x)
	{
		const int		delta = (src_msb_ptr [x] << 8) + src_lsb_ptr [x];
		sum_v [x + MARGIN] += delta;
	}
}



void	BoxFilter::add_line_sse2 (SegBuffer &sum_v, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int pos, int w)
{
	int				beg = pos;
	const int		end = beg + w;
	assert (beg + MARGIN >= 0);
	assert (end + MARGIN <= sum_v.size ());

	// Make sure that sum_v [x + BoxFilter::MARGIN] is aligned on a 16-byte boundary.
	beg &= -4;

	const __m128i	zero = _mm_setzero_si128 ();

	for (int x = beg; x < end; x += 8)
	{
		const __m128i  delta    =
			fstb::ToolsSse2::load_8_16ml (src_msb_ptr + x, src_lsb_ptr + x);
		const __m128i  delta_03 = _mm_unpacklo_epi16 (delta, zero);
		const __m128i  delta_47 = _mm_unpackhi_epi16 (delta, zero);

		__m128i        val_03   = _mm_load_si128 (
			reinterpret_cast <const __m128i *> (&sum_v [x + MARGIN    ])
		);
		__m128i        val_47   = _mm_load_si128 (
			reinterpret_cast <const __m128i *> (&sum_v [x + MARGIN + 4])
		);
		val_03 = _mm_add_epi32 (val_03, delta_03);
		val_47 = _mm_add_epi32 (val_47, delta_47);
		_mm_store_si128 (
			reinterpret_cast <__m128i *> (&sum_v [x + MARGIN    ]),
			val_03
		);
		_mm_store_si128 (
			reinterpret_cast <__m128i *> (&sum_v [x + MARGIN + 4]),
			val_47
		);
	}
}



void	BoxFilter::addsub_line_cpp (SegBuffer &sum_v, const uint8_t *add_msb_ptr, const uint8_t *add_lsb_ptr, const uint8_t *sub_msb_ptr, const uint8_t *sub_lsb_ptr, int pos, int w)
{
	const int		beg = pos;
	const int		end = beg + w;
	assert (beg + MARGIN >= 0);
	assert (end + MARGIN <= sum_v.size ());

	for (int x = beg; x < end; ++x)
	{
		const int		vpos = (add_msb_ptr [x] << 8) + add_lsb_ptr [x];
		const int		vneg = (sub_msb_ptr [x] << 8) + sub_lsb_ptr [x];
		const int		delta = vpos - vneg;
		sum_v [x + MARGIN] += delta;
	}
}



void	BoxFilter::addsub_line_sse2 (SegBuffer &sum_v, const uint8_t *add_msb_ptr, const uint8_t *add_lsb_ptr, const uint8_t *sub_msb_ptr, const uint8_t *sub_lsb_ptr, int pos, int w)
{
	int				beg = pos;
	const int		end = beg + w;
	assert (beg + MARGIN >= 0);
	assert (end + MARGIN <= sum_v.size ());

	// Make sure that sum_v [x + BoxFilter::MARGIN] is aligned on a 16-byte boundary.
	beg &= -4;

	const __m128i	zero = _mm_setzero_si128 ();

	for (int x = beg; x < end; x += 8)
	{
		const __m128i  val_pos =
			fstb::ToolsSse2::load_8_16ml (add_msb_ptr + x, add_lsb_ptr + x);
		const __m128i	val_pos_03 = _mm_unpacklo_epi16 (val_pos, zero);
		const __m128i	val_pos_47 = _mm_unpackhi_epi16 (val_pos, zero);

		const __m128i  val_neg =
			fstb::ToolsSse2::load_8_16ml (sub_msb_ptr + x, sub_lsb_ptr + x);
		const __m128i	val_neg_03 = _mm_unpacklo_epi16 (val_neg, zero);
		const __m128i	val_neg_47 = _mm_unpackhi_epi16 (val_neg, zero);

		__m128i			val_03 = _mm_load_si128 (
			reinterpret_cast <const __m128i *> (&sum_v [x + MARGIN    ])
		);
		__m128i			val_47 = _mm_load_si128 (
			reinterpret_cast <const __m128i *> (&sum_v [x + MARGIN + 4])
		);
		val_03 = _mm_add_epi32 (val_03, val_pos_03);
		val_03 = _mm_sub_epi32 (val_03, val_neg_03);
		val_47 = _mm_add_epi32 (val_47, val_pos_47);
		val_47 = _mm_sub_epi32 (val_47, val_neg_47);
		_mm_store_si128 (
			reinterpret_cast <__m128i *> (&sum_v [x + MARGIN    ]),
			val_03
		);
		_mm_store_si128 (
			reinterpret_cast <__m128i *> (&sum_v [x + MARGIN + 4]),
			val_47
		);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	BoxFilter::mirror_pos (int pos, int len)
{
	if (pos < 0)
	{
		pos = -pos - 1;
	}
	else if (pos >= len)
	{
		pos = 2 * len - pos - 1;
	}

	return (pos);
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
