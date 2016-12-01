/*****************************************************************************

        TransientAnalyser.cpp
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

#include "fstb/def.h"
#include "mfx/pi/dist2/TransientAnalyser.h"

#include <cassert>
#include <cfloat>
#include <cmath>



namespace mfx
{
namespace pi
{
namespace dist2
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	TransientAnalyser::reset (double sample_freq, int max_block_size)
{
	assert (sample_freq > 0);
	assert (max_block_size > 0);

	_sample_freq = float (sample_freq);
	_max_block_size = max_block_size;

	const int      mbs_alig = (_max_block_size + 3) & -4;
	_buf.resize (mbs_alig);

	// Attack, fast envelope
	_env_helper->set_atk_coef (0, compute_coef (0.0001f));
	_env_helper->set_rls_coef (0, compute_coef (0.050f));

	// Attack, slow envelope
	_env_helper->set_atk_coef (1, compute_coef (0.050f));
	_env_helper->set_rls_coef (1, compute_coef (0.050f));

	// Sustain, fast envelope
	_env_helper->set_atk_coef (2, compute_coef (0.005f));
	_env_helper->set_rls_coef (2, compute_coef (0.200f));

	// Sustain, slow envelope
	_env_helper->set_atk_coef (3, compute_coef (0.005f));
	_env_helper->set_rls_coef (3, compute_coef (0.600f));

	const double   min_freq = 50; // Hz
	const int      hold_time = fstb::round_int (sample_freq / min_freq);
	for (int e = 0; e < EnvHelper::_nbr_env; ++e)
	{
		_env_helper->set_hold_time (e, hold_time);
	}
}



void	TransientAnalyser::set_epsilon (float eps)
{
	assert (eps > sqrt (FLT_MIN));

	_eps_sq = eps * eps;
}



// Stores the result as the log2 of the ratio between:
// - the fast envelope and the slow envelope for the attack
// - the slow envelope and the fast envelope for the sustain
// Negative values are clipped to 0.
void	TransientAnalyser::process_block (float atk_ptr [], float sus_ptr [], const float * const src_ptr_arr [], int nbr_chn, int nbr_spl)
{
	assert (_sample_freq > 0);
	assert (fstb::DataAlign <true>::check_ptr (src_ptr_arr [0]));
	assert (fstb::DataAlign <true>::check_ptr (atk_ptr));
	assert (fstb::DataAlign <true>::check_ptr (sus_ptr));
	assert (nbr_chn > 0);
	assert (nbr_spl > 0);

	// Converts everything to mono (squared)
	perpare_mono_input (&_buf [0], src_ptr_arr, nbr_chn, nbr_spl);

	// Envelope detection
	_env_helper->process_block (&_buf [0], &_buf [0], nbr_spl);

	// Ratio
	const auto     eps  = fstb::ToolsSimd::set1_f32 (_eps_sq);
	const auto     zero = fstb::ToolsSimd::set_f32_zero ();
	for (int pos = 0; pos < nbr_spl; pos += 4)
	{
		// Collects the envelopes into vectors
		// Name: {e}nvelope - {a}ttack/{s}ustain - {s}low/{f}ast
		auto           eaf = fstb::ToolsSimd::load_f32 (&_buf [pos    ]);
		auto           eas = fstb::ToolsSimd::load_f32 (&_buf [pos + 1]);
		auto           esf = fstb::ToolsSimd::load_f32 (&_buf [pos + 2]);
		auto           ess = fstb::ToolsSimd::load_f32 (&_buf [pos + 3]);
		fstb::ToolsSimd::transpose_f32 (eaf, eas, esf, ess);

		// Computes the ratio for the attack
		eaf += eps;    // Prevents dividing by zero and makes a unity ratio
		eas += eps;    // when everything tends toward zero.
		auto           eas_inv = fstb::ToolsSimd::rcp_approx2 (eas);
		auto           ea_ratio = eaf * eas_inv;
		auto           ea_r_l2 = fstb::ToolsSimd::log2_approx (ea_ratio);
		ea_r_l2 = fstb::ToolsSimd::max_f32 (ea_r_l2, zero);
		fstb::ToolsSimd::store_f32 (atk_ptr + pos, ea_r_l2);

		// Ratio for the sustain
		esf += eps;
		ess += eps;
		auto           esf_inv = fstb::ToolsSimd::rcp_approx2 (esf);
		auto           es_ratio = ess * esf_inv;
		auto           es_r_l2 = fstb::ToolsSimd::log2_approx (es_ratio);
		es_r_l2 = fstb::ToolsSimd::max_f32 (es_r_l2, zero);
		fstb::ToolsSimd::store_f32 (sus_ptr + pos, es_r_l2);
	}
}



void	TransientAnalyser::clear_buffers ()
{
	_env_helper->clear_buffers ();
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



float	TransientAnalyser::compute_coef (float t) const
{
	assert (_sample_freq > 0);

	float          coef = 1;
	const float    tsf  = t * _sample_freq;
	if (tsf > 1)
	{
		coef = float (1.0f - exp (-1.0f / tsf));
	}

	return coef;
}



void	TransientAnalyser::perpare_mono_input (fstb::ToolsSimd::VectF32 buf_ptr [], const float * const src_ptr_arr [], int nbr_chn, int nbr_spl)
{
	if (nbr_chn == 1)
	{
		const float *  src_ptr = src_ptr_arr [0];
		for (int pos = 0; pos < nbr_spl; pos += 4)
		{
			auto           x = fstb::ToolsSimd::load_f32 (src_ptr + pos);
			x = fstb::ToolsSimd::abs (x);
			spread_and_store (buf_ptr + pos, x);
		}
	}

	else if (nbr_chn == 2)
	{
		const float *  src0_ptr = src_ptr_arr [0];
		const float *  src1_ptr = src_ptr_arr [1];
		const auto     gain = fstb::ToolsSimd::set1_f32 (0.5f);
		for (int pos = 0; pos < nbr_spl; pos += 4)
		{
			auto           x0 = fstb::ToolsSimd::load_f32 (src0_ptr + pos);
			auto           x1 = fstb::ToolsSimd::load_f32 (src1_ptr + pos);
			x0 = fstb::ToolsSimd::abs (x0);
			x1 = fstb::ToolsSimd::abs (x1);
			const auto     x = (x0 + x1) * gain;
			spread_and_store (buf_ptr + pos, x);
		}
	}

	else
	{
		const auto     gain = fstb::ToolsSimd::set1_f32 (1.0f / float (nbr_chn));
		for (int pos = 0; pos < nbr_spl; pos += 4)
		{
			auto           x = fstb::ToolsSimd::set_f32_zero ();
			for (int chn = 0; chn < nbr_chn; ++chn)
			{
				const float *  src_ptr = src_ptr_arr [chn];
				auto           xn = fstb::ToolsSimd::load_f32 (src_ptr + pos);
				xn = fstb::ToolsSimd::abs (xn);
				x += xn;
			}
			x *= gain;
			spread_and_store (buf_ptr + pos, x);
		}
	}
}



void	TransientAnalyser::spread_and_store (fstb::ToolsSimd::VectF32 dst_ptr [], fstb::ToolsSimd::VectF32 x)
{
	fstb::ToolsSimd::store_f32 (
		dst_ptr + 0, fstb::ToolsSimd::Shift <0>::spread (x)
	);
	fstb::ToolsSimd::store_f32 (
		dst_ptr + 1, fstb::ToolsSimd::Shift <1>::spread (x)
	);
	fstb::ToolsSimd::store_f32 (
		dst_ptr + 2, fstb::ToolsSimd::Shift <2>::spread (x)
	);
	fstb::ToolsSimd::store_f32 (
		dst_ptr + 3, fstb::ToolsSimd::Shift <3>::spread (x)
	);
}



}  // namespace dist2
}  // namespace pi
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/