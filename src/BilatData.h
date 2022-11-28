/*****************************************************************************

        BilatData.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (BilatData_HEADER_INCLUDED)
#define	BilatData_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"fstb/AllocAlign.h"

#include	<vector>



class BilatData
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit			BilatData (int width, int height, int margin_h, int margin_v, bool src_flag);
	virtual			~BilatData () {}

	int				get_width () const;
	int				get_height () const;
	int				get_margin_h () const;
	int				get_margin_v () const;
	bool				has_src () const;

	float *			get_cache_src_ptr () const;
	float *			get_cache_ref_ptr () const;
	int				get_cache_in_stride () const;

	void				mirror_pic ();



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {			ALIGN_PIX	= 4	};	// Minimum pixel alignment

	template <class T>
	class ArrayMargin
	{
	public:
		void				init (int w, int h, int ml, int mt, int d, ArrayMargin <T> *other_ptr = 0);
		void				mirror ();
		inline int		align (int v, int d);

		typedef	T	EltType;
		std::vector <EltType, fstb::AllocAlign <EltType, 16> >
							_data_arr;
		T *				_org_ptr;
		int				_stride;		// EltType count.
		int				_mla;			// Margin, left, aligned to block boundaries
		int				_mta;			// Margin, top, aligned
		int				_w;			// Width, without margin
		int				_h;			// Height, without margin
		int				_pra;			// Right position, aligned
		int				_prad;		// Number of missing elements between _w and _pra
		int				_pram;		// width + margin, aligned
		int				_pramd;		//
		int				_pba;			// Bottom position, aligned
		int				_pbad;		// Number of missing elements between _h and _pba
		int				_pbam;		// height + margin, aligned
		int				_pbamd;
	};

	int				_width;
	int				_height;
	int				_margin_h;
	int				_margin_v;
	bool				_src_flag;

	ArrayMargin <float>
						_cache_src;
	ArrayMargin <float>
						_cache_ref;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						BilatData ();
						BilatData (const BilatData &other);
	BilatData &		operator = (const BilatData &other);
	bool				operator == (const BilatData &other) const;
	bool				operator != (const BilatData &other) const;

};	// class BilatData



//#include	"BilatData.hpp"



#endif	// BilatData_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
