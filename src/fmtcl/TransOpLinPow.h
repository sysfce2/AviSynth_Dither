/*****************************************************************************

        TransOpLinPow.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpLinPow_HEADER_INCLUDED)
#define	fmtcl_TransOpLinPow_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpLinPow
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	//  beta         <= L                 : V =    alpha * pow ( L        , p1) - (alpha - 1)
	// -beta / scneg <  L <   beta        : V =            pow ( L * slope, p2)
	//                  L <= -beta / scneg: V = - (alpha * pow (-L * scneg, p1) - (alpha - 1)) / scneg
	explicit       TransOpLinPow (bool inv_flag, double alpha, double beta, double p1, double slope, double lb = 0, double ub = 1, double scneg = 1, double p2 = 1);
	virtual        ~TransOpLinPow () {}

	// TransOpInterface
	virtual double operator () (double x) const;
	virtual double get_max () const { return (_ub); }



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	const bool     _inv_flag;
	const double   _alpha;
	const double   _beta;
	const double   _p1;
	const double   _slope;
	const double   _lb;
	const double   _ub;
	const double   _scneg;
	const double   _p2;
	double         _alpha_m1;
	double         _beta_n;
	double         _beta_i;
	double         _beta_in;
	double         _lb_i;
	double         _ub_i;
	double         _p1_i;
	double         _p2_i;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpLinPow ()                               = delete;
	               TransOpLinPow (const TransOpLinPow &other)     = delete;
	TransOpLinPow& operator = (const TransOpLinPow &other)        = delete;
	bool           operator == (const TransOpLinPow &other) const = delete;
	bool           operator != (const TransOpLinPow &other) const = delete;

};	// class TransOpLinPow



}	// namespace fmtcl



//#include "fmtcl/TransOpLinPow.hpp"



#endif	// fmtcl_TransOpLinPow_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
