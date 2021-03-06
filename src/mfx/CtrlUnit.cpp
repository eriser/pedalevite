/*****************************************************************************

        CtrlUnit.cpp
        Author: Laurent de Soras, 2016

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

#include "fstb/fnc.h"
#include "mfx/doc/CtrlLink.h"
#include "mfx/CtrlUnit.h"
#include "mfx/ProcessingContext.h"

#include <complex>

#include <cassert>
#include <cmath>



namespace mfx
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



CtrlUnit::CtrlUnit (const doc::CtrlLink &other, bool abs_flag)
:	_source (other._source)
,	_step (other._step)
,	_val (0)
,	_curve (other._curve)
,	_u2b_flag (other._u2b_flag)
,	_abs_flag (abs_flag)
,	_base (other._base)
,	_amp (other._amp)
,	_notch_list (other._notch_list)
{
	// Nothing
}



void	CtrlUnit::update_internal_val (float val_nrm)
{
	assert (_abs_flag);

	float               mod_val = val_nrm - _base;
	if (_amp != 0)
	{
		mod_val /= _amp;
	}
	mod_val = fstb::limit (mod_val, 0.f, 1.f);

	// Inverse curve
	mod_val = apply_curve (mod_val, _curve, true);

	_val = mod_val;
}



void	CtrlUnit::update_abs_val (float raw_val)
{
	if (_source.is_relative ())
	{
		_val += raw_val * _step;
		_val = fstb::limit (_val, 0.f, 1.f);
	}
	else
	{
		_val = raw_val;
	}
}



// param_val is the original parameter value, or modulated with the previous
// modulations in the mod chain.
// param_val and the return value are normalized but can be out of the
// [0; 1] range, so clipping happens only on the final value.
float	CtrlUnit::evaluate (float param_val) const
{
	float          mod_val = _val;
	if (_u2b_flag)
	{
		mod_val = mod_val * 2 - 1;
	}

	mod_val  = apply_curve (mod_val, _curve, false);
	mod_val *= _amp;

	if (_abs_flag)
	{
		param_val = _base + mod_val;
	}
	else
	{
		param_val += mod_val;
	}

	if (! _notch_list.empty ())
	{
		auto           it_l = _notch_list.lower_bound (param_val);
		if (it_l == _notch_list.end ())
		{
			param_val = *_notch_list.rbegin ();
		}
		else if (it_l == _notch_list.begin ())
		{
			param_val = *it_l;
		}
		else
		{
			assert (_notch_list.size () >= 2);
			const float    v1 = *it_l;
			-- it_l;
			const float    v0 = *it_l;
			const float    d1 = fabs (param_val - v1);
			const float    d0 = fabs (param_val - v0);
			param_val = (d0 < d1) ? v0 : v1;
		}
	}

	return param_val;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Input is not range-restricted, it can be bipolar too.
float	CtrlUnit::apply_curve (float val, ControlCurve curve, bool invert_flag)
{
	static const int  inv = 1000;
	switch (curve + ((invert_flag) ? inv : 0))
	{
	case ControlCurve_LINEAR:
	case ControlCurve_LINEAR + inv:
		// Nothing
		break;

	case ControlCurve_SQ:
		val = fabs (val) * val;
		break;
	case ControlCurve_SQ + inv:
		val = std::copysign (sqrt (fabs (val)), val);
		break;

	case ControlCurve_CB:
		val = val * val * val;
		break;
	case ControlCurve_CB + inv:
		val = cbrt (val);
		break;

	case ControlCurve_SAT2:
		{
			const float    vx = 1 - fabs (val);
			val = std::copysign (1 - vx * vx, val);
		}
		break;
	case ControlCurve_SAT2 + inv:
		{
			const float    vy = fabs (val);
			val = std::copysign (1 - sqrt (1 - vy), val);
		}
		break;

	case ControlCurve_SAT3:
		{
			const float    vx = 1 - fabs (val);
			val = std::copysign (1 - vx * vx * vx, val);
		}
		break;
	case ControlCurve_SAT3 + inv:
		{
			const float    vy = fabs (val);
			val = std::copysign (1 - cbrt (1 - vy), val);
		}
		break;

	case ControlCurve_S1:
	case ControlCurve_FLAT1 + inv:
		val = std::copysign (val * val * (3 - 2 * fabs (val)), val);
		break;
	case ControlCurve_S1 + inv:
	case ControlCurve_FLAT1:
		val = float (invert_s1 (val));
		break;

	case ControlCurve_S2:
	case ControlCurve_FLAT2 + inv:
		val = apply_curve (val, ControlCurve_S1, false);
		val = apply_curve (val, ControlCurve_S1, false);
		break;
	case ControlCurve_S2 + inv:
	case ControlCurve_FLAT2:
		val = float (invert_s1 (invert_s1 (val)));
		break;

	default:
		assert (false);
		break;
	}

	return val;
}



// 3x^2 - 2x^3 - a = 0
// d = cbrt (2 * sqrt (a * (a - 1)) - 2 * a + 1)
// | x = 0.5 + 0.5 * (d + 1 / d)
// | x = -0.25 * (1 - i * sqrt (3)) * d - 0.25*(1 + i * sqrt(3)) / d + 0.5
// | x = -0.25 * (1 + i * sqrt (3)) * d - 0.25*(1 - i * sqrt(3)) / d + 0.5 <-- this one
double	CtrlUnit::invert_s1 (double val)
{
	const double                  a_r = fabs (val);
	const std::complex <double>   a (a_r, 0);
	const std::complex <double>   aam1 (a_r * (a_r - 1.));
	const std::complex <double>   d   =
		std::pow ((std::sqrt (aam1) - a) * 2. + 1., 1 / 3.0);
	const double      s3 = sqrt (3);
	const double      x  =
		(- 0.25 * std::complex <double> (1,  s3) * d
		 - 0.25 * std::complex <double> (1, -s3) / d).real ()
		+ 0.5;
	
	return std::copysign (x, val);
}



}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
