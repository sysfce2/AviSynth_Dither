/*****************************************************************************

        BilatData.cpp
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



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"BilatData.h"

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



BilatData::BilatData (int width, int height, int margin_h, int margin_v, bool src_flag)
:	_width (width)
,	_height (height)
,	_margin_h (margin_h)
,	_margin_v (margin_v)
,	_src_flag (src_flag)
,	_cache_src ()
,	_cache_ref ()
{
	assert (width > 0);
	assert (height > 0);
	assert (margin_h >= 0);
	assert (margin_v >= 0);

	const int		pix_s = (width + ALIGN_PIX - 1) & -ALIGN_PIX;

	ArrayMargin <float> *	ref_ptr = &_cache_ref;
	if (_src_flag)
	{
		ref_ptr = 0;
	}
	_cache_ref.init (pix_s, height, _margin_h, _margin_v, 1);
	_cache_src.init (pix_s, height, _margin_h, _margin_v, 1, ref_ptr);
}



int	BilatData::get_width () const
{
	return (_width);
}



int	BilatData::get_height () const
{
	return (_height);
}



int	BilatData::get_margin_h () const
{
	return (_margin_h);
}



int	BilatData::get_margin_v () const
{
	return (_margin_v);
}



bool	BilatData::has_src () const
{
	return (_src_flag);
}



float *	BilatData::get_cache_src_ptr () const
{
	return (_cache_src._org_ptr);
}



float *	BilatData::get_cache_ref_ptr () const
{
	return (_cache_ref._org_ptr);
}



int	BilatData::get_cache_in_stride () const
{
	return (_cache_src._stride);
}



void	BilatData::mirror_pic ()
{
	_cache_ref.mirror ();
	if (has_src ())
	{
		_cache_src.mirror ();
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
void	BilatData::ArrayMargin <T>::init (int w, int h, int ml, int mt, int d, ArrayMargin <T> *other_ptr)
{
	assert (d > 0);
	assert (ml >= 0);
	assert (mt >= 0);
	assert (0 < w);
	assert (0 < h);

	_mla   = align (ml, d);			// Margin, left
	_mta   = align (mt, d);			// Margin, top
	_w     = (w + d - 1) / d;		// Width, without margin
	_h     = (h + d - 1) / d;		// Height, without margin
	_pra   = align (w, d);			// Right position, aligned
	_pba   = align (h, d);			// Bottom position, aligned
	_prad  = _pra - _w;				// Number of missing elements between _w and _pra
	_pbad  = _pba - _h;				// Number of missing elements between _h and _pba
	_pram  = align (w + ml, d);	// width + margin, aligned
	_pbam  = align (h + mt, d);	// height + margin, aligned
	_pramd = _pram - _pra;
	_pbamd = _pbam - _pba;

	const int		wa = _mla + _pram;
	const int		ha = _mta + _pbam;
	_stride = wa;
	if (other_ptr == 0)
	{
		const int		offset = _stride * _mta + _mla;
		_data_arr.resize (wa * ha);
		_org_ptr = &_data_arr [offset];
	}
	else
	{
		assert (other_ptr->_w == _w);
		assert (other_ptr->_h == _h);
		assert (other_ptr->_stride == _stride);
		_org_ptr = other_ptr->_org_ptr;
	}
}



template <class T>
void	BilatData::ArrayMargin <T>::mirror ()
{
	const int		stride = _stride;
	const int		h      = _h;

	// Horizontal mirroring
	const int		mla   = _mla;
	const int		prad  = _prad;
	const int		pramd = _pramd;
	EltType *		data_l_ptr  = _org_ptr;
	EltType *		data_r_ptr  = data_l_ptr + _w;
	EltType *		data_ra_ptr = data_l_ptr + _pra;

	for (int y = 0; y < h; ++y)
	{
		if (prad > 0)
		{
			const EltType	val = data_r_ptr [-1];
			int				x = 0;
			do
			{
				data_r_ptr [x] = val;
				++x;
			}
			while (x < prad);
		}

		for (int x = 0; x < mla; ++x)
		{
			data_l_ptr [-1 - x] = data_l_ptr [x];
		}

		for (int x = 0; x < pramd; ++x)
		{
			data_ra_ptr [x] = data_ra_ptr [-1 - x];
		}

		data_l_ptr  += stride;
		data_r_ptr  += stride;
		data_ra_ptr += stride;
	}

	// Vertical mirroring
	EltType *		data_t_ptr  = _org_ptr - _mla;
	EltType *		data_b_ptr  = data_t_ptr + stride * h;
	EltType *		data_ba_ptr = data_t_ptr + stride * _pba;
	const int		stride_byte = stride * sizeof (*data_t_ptr);

	const int		pbad = _pbad;
	for (int y = 0; y < pbad; ++y)
	{
		memcpy (
			data_b_ptr + y * stride,
			data_b_ptr -     stride,
			stride_byte
		);
	}

	const int		mta = _mta;
	for (int y = 0; y < mta; ++y)
	{
		memcpy (
			data_t_ptr - (y + 1) * stride,
			data_t_ptr +  y      * stride,
			stride_byte
		);
	}

	const int		pbamd = _pbamd;
	for (int y = 0; y < pbamd; ++y)
	{
		memcpy (
			data_ba_ptr +  y      * stride,
			data_ba_ptr - (y + 1) * stride,
			stride_byte
		);
	}
}



template <class T>
int	BilatData::ArrayMargin <T>::align (int v, int d)
{
	assert (v >= 0);
	assert (d > 0);

	return ((((v + ALIGN_PIX - 1) & -ALIGN_PIX) + d - 1) / d);
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
