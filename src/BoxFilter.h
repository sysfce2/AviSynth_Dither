/*****************************************************************************

        BoxFilter.h
        Author: Laurent de Soras, 2010

The picture is considered as mirrored outside its bounds (margins for the
filter): p [-x-1] = p [x] and p [w+x] = p [w-1-x]

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (BoxFilter_HEADER_INCLUDED)
#define	BoxFilter_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"fstb/ArrayAlign.h"

#include <cstdint>



class BoxFilter
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum {			MAX_SEG_LEN	= 2048	};	// To store temporary data on the stack. Must be a multiple of 8 (movq).
	enum {			MAX_RADIUS	= 91		};	// (91*2-1)^2 < 2^15, so the sum of the box fits in a signed 32-bit int.
	enum {			MARGIN		= MAX_SEG_LEN / 2	};	// Must also be a multiple of 8. Upper bound for the radius value.

	typedef	fstb::ArrayAlign <uint32_t, MAX_SEG_LEN + MARGIN * 2, 16>	SegBuffer;

	typedef	void (*	AddLinePtr) (SegBuffer &sum_v, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int pos, int w);
	typedef	void (*	AddSubLinePtr) (SegBuffer &sum_v, const uint8_t *add_msb_ptr, const uint8_t *add_lsb_ptr, const uint8_t *sub_msb_ptr, const uint8_t *sub_lsb_ptr, int pos, int w);

						BoxFilter (const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int h, int stride_dst, int stride_src_msb, int stride_src_lsb, int col_x, int col_w, int radius_h, int radius_v, AddLinePtr add_line_ptr, AddSubLinePtr addsub_line_ptr);
	virtual			~BoxFilter () {}

	void				init_sum_v ();
	void				filter_line ();

	void				init_sum_h ();
	inline int		filter_col ();
	inline int		get_col_w () const;

	static void		add_line_cpp (BoxFilter::SegBuffer &sum_v, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int pos, int w);
	static void		add_line_sse2 (BoxFilter::SegBuffer &sum_v, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int pos, int w);
	static void		addsub_line_cpp (BoxFilter::SegBuffer &sum_v, const uint8_t *add_msb_ptr, const uint8_t *add_lsb_ptr, const uint8_t *sub_msb_ptr, const uint8_t *sub_lsb_ptr, int pos, int w);
	static void		addsub_line_sse2 (BoxFilter::SegBuffer &sum_v, const uint8_t *add_msb_ptr, const uint8_t *add_lsb_ptr, const uint8_t *sub_msb_ptr, const uint8_t *sub_lsb_ptr, int pos, int w);

private:

	static inline int
						mirror_pos (int pos, int len);

	SegBuffer		_sum_v;
	int				_sum_h;

	const uint8_t *
						_src_msb_ptr;
	const uint8_t *
						_src_lsb_ptr;
	int				_w;
	int				_h;
	int				_stride_src_msb;
	int				_stride_src_lsb;
	int				_col_x;
	int				_col_w;
	int				_radius_h;
	int				_radius_v;

	int				_weight;
	unsigned int	_weight_mul;	// 2^32 / _weight
	int				_w_round;

	// Leftmost and rightmost positions of source data within the line buffers.
	int				_x_l;
	int				_x_r;

	bool				_mirror_l_flag;
	bool				_mirror_r_flag;

	// L & rightmost positions of the copy, relative to col_x.
	// Data between copy_r_pos and col_w + radius_h will have to be mirrored,
	// as well as data between col_x - radius_h and copy_l_pos.
	int				_copy_l_pos_src;
	int				_copy_l_pos;
	int				_copy_r_pos;
	int				_copy_len;

	// Vertical iterations
	int				_y;

	// Horizontal iterations (from 0 to _col_w - 1)
	int				_pos_pos;
	int				_pos_neg;

	AddLinePtr		_add_line_ptr;
	AddSubLinePtr	_addsub_line_ptr;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						BoxFilter ();
						BoxFilter (const BoxFilter &other);
	BoxFilter &		operator = (const BoxFilter &other);
	bool				operator == (const BoxFilter &other) const;
	bool				operator != (const BoxFilter &other) const;

};	// class BoxFilter



#include	"BoxFilter.hpp"



#endif	// BoxFilter_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
