/*****************************************************************************

        FrequencyShifter.cpp
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
#include "hiir/PolyphaseIir2Designer.h"
#include "mfx/dsp/iir/TransSZBilin.h"
#include "mfx/dsp/mix/Align.h"
#include "mfx/pi/freqsh/FrequencyShifter.h"
#include "mfx/pi/freqsh/Param.h"
#include "mfx/piapi/EventParam.h"
#include "mfx/piapi/EventTs.h"
#include "mfx/piapi/EventType.h"

#include <cassert>



namespace mfx
{
namespace pi
{
namespace freqsh
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



FrequencyShifter::FrequencyShifter ()
:	_state (State_CREATED)
,	_desc ()
,	_state_set ()
,	_sample_freq (0)
,	_param_change_flag ()
,	_ali ()
,	_inv_fs (0)
,	_freq (0)
,	_step_angle (0)
{
	_state_set.init (piapi::ParamCateg_GLOBAL, _desc.use_desc_set ());

	_state_set.set_val (Param_FREQ, 0.5f);

	_state_set.add_observer (Param_FREQ, _param_change_flag);

	_state_set.set_ramp_time (Param_FREQ, 0.010);

	double         coef_list [_nbr_coef];
	hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw (
		coef_list, _nbr_coef, 1 / 1000.0
	);
	for (auto &chn : _ali->_chn_arr)
	{
		chn._ssb.set_coefs (coef_list);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



piapi::PluginInterface::State	FrequencyShifter::do_get_state () const
{
	return _state;
}



double	FrequencyShifter::do_get_param_val (piapi::ParamCateg categ, int index, int note_id) const
{
	assert (categ == piapi::ParamCateg_GLOBAL);

	return _state_set.use_state (index).get_val_tgt ();
}



int	FrequencyShifter::do_reset (double sample_freq, int max_buf_len, int &latency)
{
	latency = 0;
	_sample_freq = float (sample_freq);

	_state_set.set_sample_freq (sample_freq);
	_state_set.clear_buffers ();

	_inv_fs = float (1.0 / _sample_freq);
	for (auto &chn : _ali->_chn_arr)
	{
		chn._aa.clear_buffers ();
		chn._ssb.clear_buffers ();
	}
	_ali->_osc.clear_buffers ();

	for (auto &buf : _buf_arr)
	{
		buf.resize (max_buf_len);
	}

	_state = State_ACTIVE;

	return Err_OK;
}



void	FrequencyShifter::do_process_block (ProcInfo &proc)
{
	// Events
	for (int evt_cnt = 0; evt_cnt < proc._nbr_evt; ++evt_cnt)
	{
		const piapi::EventTs &  evt = *(proc._evt_arr [evt_cnt]);
		if (evt._type == piapi::EventType_PARAM)
		{
			const piapi::EventParam &  evtp = evt._evt._param;
			assert (evtp._categ == piapi::ParamCateg_GLOBAL);
			_state_set.set_val (evtp._index, evtp._val);
		}
	}

	// Parameters
	_state_set.process_block (proc._nbr_spl);

	if (_param_change_flag (true))
	{
		_freq = float (_state_set.get_val_end_nat (Param_FREQ));
		update_step ();
	}

	// Signal processing
	_ali->_osc.process_block (
		&_buf_arr [Buf_COS] [0],
		&_buf_arr [Buf_SIN] [0],
		proc._nbr_spl / dsp::osc::OscSinCosStableSimd::_nbr_units
	);

	const int      nbr_chn_i =
		proc._nbr_chn_arr [piapi::PluginInterface::Dir_IN ];
	const int      nbr_chn_o =
		proc._nbr_chn_arr [piapi::PluginInterface::Dir_OUT];
	const int      nbr_chn_p = std::min (nbr_chn_i, nbr_chn_o);
	for (int c = 0; c < nbr_chn_p; ++c)
	{
		_ali->_chn_arr [c]._aa.process_block (
			&_buf_arr [Buf_AAF] [0],
			proc._src_arr [c],
			proc._nbr_spl
		);

		_ali->_chn_arr [c]._ssb.process_block (
			&_buf_arr [Buf_PHC] [0],
			&_buf_arr [Buf_PHS] [0],
			&_buf_arr [Buf_AAF] [0],
			proc._nbr_spl
		);

		float *        dst_ptr = proc._dst_arr [c];
		for (int pos = 0; pos < proc._nbr_spl; pos += 4)
		{
			const auto     co  = fstb::ToolsSimd::load_f32 (&_buf_arr [Buf_COS] [pos]);
			const auto     si  = fstb::ToolsSimd::load_f32 (&_buf_arr [Buf_SIN] [pos]);
			const auto     x   = fstb::ToolsSimd::load_f32 (&_buf_arr [Buf_PHC] [pos]);
			const auto     y   = fstb::ToolsSimd::load_f32 (&_buf_arr [Buf_PHS] [pos]);
			const auto     val = co * x + si * y;
			fstb::ToolsSimd::store_f32 (dst_ptr + pos, val);
		}

	}
	for (int c = nbr_chn_p; c < nbr_chn_o; ++c)
	{
		dsp::mix::Align::copy_1_1 (
			proc._dst_arr [c],
			proc._dst_arr [0],
			proc._nbr_spl
		);
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	FrequencyShifter::update_step ()
{
	_step_angle = float ((fstb::PI * 2) * _freq / _sample_freq);
	_ali->_osc.set_step (_step_angle);

	const float    bs [3]  = { 0,                   0, 1 };
	const float    as [3]  = { 1, float (fstb::SQRT2), 1 };
	float          bz [3];
	float          az [3];
	const float    freq_aa = std::max (-_freq, 20.0f);
	const float    k       =
		dsp::iir::TransSZBilin::compute_k_approx (freq_aa * _inv_fs);
	dsp::iir::TransSZBilin::map_s_to_z_approx (bz, az, bs, as, k);
	for (auto &chn : _ali->_chn_arr)
	{
		chn._aa.set_z_eq (bz, az);
	}
}



}  // namespace freqsh
}  // namespace pi
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
