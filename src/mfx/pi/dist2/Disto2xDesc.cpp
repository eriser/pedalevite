/*****************************************************************************

        Disto2xDesc.cpp
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

#include "mfx/pi/dist2/Disto2xDesc.h"
#include "mfx/pi/dist2/DistoStage.h"
#include "mfx/pi/dist2/Param.h"
#include "mfx/pi/param/MapS.h"
#include "mfx/pi/param/MapPseudoLog.h"
#include "mfx/pi/param/TplEnum.h"
#include "mfx/pi/param/TplLin.h"
#include "mfx/pi/param/TplLog.h"
#include "mfx/pi/param/TplMapped.h"

#include <cassert>



namespace mfx
{
namespace pi
{
namespace dist2
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



const double	Disto2xDesc::_gain_min = 0.1;
const double	Disto2xDesc::_gain_max = 1000;



Disto2xDesc::Disto2xDesc ()
:	_desc_set (Param_NBR_ELT, 0)
{
	typedef param::TplMapped <param::MapS <false> > TplMix;
	typedef param::TplMapped <param::MapPseudoLog> TplPsl;

	// Crossover frequency
	param::TplLog *   log_ptr = new param::TplLog (
		5, 20480,
		"Crossover frequency\nCrossover freq\nCrossover\nXover",
		"Hz",
		param::HelperDispNum::Preset_FLOAT_STD,
		0,
		"%5.0f"
	);
	log_ptr->set_categ (piapi::ParamDescInterface::Categ_FREQ_HZ);
	_desc_set.add_glob (Param_XOVER, log_ptr);

	// Input LPF cutoff frequency
	log_ptr = new param::TplLog (
		20, 20480,
		"Input low-pass filter cutoff frequency\nInput LPF frequency\nLPF freq\nLPF",
		"Hz",
		param::HelperDispNum::Preset_FLOAT_STD,
		0,
		"%5.0f"
	);
	log_ptr->set_categ (piapi::ParamDescInterface::Categ_FREQ_HZ);
	_desc_set.add_glob (Param_PRE_LPF, log_ptr);

	// Attack gain modifier
	log_ptr = new param::TplLog (
		1.0 / 16, 16,
		"Attack gain modifier\nAttack gain mod\nAttack gain\nAtk gain\nAtk G",
		"dB",
		param::HelperDispNum::Preset_DB,
		0,
		"%+5.1f"
	);
	_desc_set.add_glob (Param_DYN_ATK, log_ptr);

	// Sustain gain modifier
	log_ptr = new param::TplLog (
		1.0 / 16, 16,
		"Sustain gain modifier\nSustain gain mod\nSustain gain\nSus gain\nSus G",
		"dB",
		param::HelperDispNum::Preset_DB,
		0,
		"%+5.1f"
	);
	_desc_set.add_glob (Param_DYN_RLS, log_ptr);

	register_stage (1, Param_S1_BASE);
	register_stage (2, Param_S2_BASE);

	// Stage 1-2 mix
	TplMix *       mix_ptr = new TplMix (
			0, 1,
			"Stage 1-2 Mix\nS12 Mix",
			"%",
			param::HelperDispNum::Preset_FLOAT_PERCENT,
			0,
			"%5.1f"
		);
	mix_ptr->use_mapper ().config (
		mix_ptr->get_nat_min (),
		mix_ptr->get_nat_max ()
	);
	_desc_set.add_glob (Param_S12_MIX, mix_ptr);

	// Low band gain
	mix_ptr = new TplMix (
		0, 1,
		"Low band mix\nLow mix\nL Mix",
		"%",
		param::HelperDispNum::Preset_FLOAT_PERCENT,
		0,
		"%5.1f"
	);
	mix_ptr->use_mapper ().config (
		mix_ptr->get_nat_min (),
		mix_ptr->get_nat_max ()
	);
	_desc_set.add_glob (Param_LB_MIX, mix_ptr);

	// Density
	param::TplLin *   lin_ptr = new param::TplLin (
		0, 1,
		"Density\nDens",
		"%",
		0,
		"%5.1f"
	);
	lin_ptr->use_disp_num ().set_preset (param::HelperDispNum::Preset_FLOAT_PERCENT);
	_desc_set.add_glob (Param_DENSITY, lin_ptr);

	// Threshold
	TplPsl *       pl_ptr = new TplPsl (
		0, 1,
		"Threshold\nThresh\nThr",
		"dB",
		param::HelperDispNum::Preset_DB,
		0,
		"%+5.1f"
	);
	_desc_set.add_glob (Param_THRESH, pl_ptr);
}



ParamDescSet &	Disto2xDesc::use_desc_set ()
{
	return _desc_set;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



std::string	Disto2xDesc::do_get_unique_id () const
{
	return "dist2";
}



std::string	Disto2xDesc::do_get_name () const
{
	return "Double distortion\nDouble dist\nDist2x";
}



void	Disto2xDesc::do_get_nbr_io (int &nbr_i, int &nbr_o, int &nbr_s) const
{
	nbr_i = 1;
	nbr_o = 1;
	nbr_s = 0;
}



bool	Disto2xDesc::do_prefer_stereo () const
{
	return false;
}



int	Disto2xDesc::do_get_nbr_param (piapi::ParamCateg categ) const
{
	return _desc_set.get_nbr_param (categ);
}



const piapi::ParamDescInterface &	Disto2xDesc::do_get_param_info (piapi::ParamCateg categ, int index) const
{
	return _desc_set.use_param (categ, index);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Disto2xDesc::register_stage (int stage, int base)
{
	// HPF Cutoff frequency
	param::TplLog *   log_ptr = new param::TplLog (
		3, 3000,
		"Stage %d input high-pass filter cutoff frequency\nStage %d input HPF frequency\nStage %d HPF cutoff frequency\nStage %d HPF frequency\nStage %d HPF freq\nStage %d HPF\nS%d HPF",
		"Hz",
		param::HelperDispNum::Preset_FLOAT_STD,
		stage,
		"%4.0f"
	);
	log_ptr->set_categ (piapi::ParamDescInterface::Categ_FREQ_HZ);
	_desc_set.add_glob (base + ParamStage_HPF_PRE, log_ptr);

	// Bias
	param::TplLin *   lin_ptr = new param::TplLin (
		-1, 1,
		"Stage %d Bias\nS%d Bias",
		"%",
		stage,
		"%+6.1f"
	);
	lin_ptr->use_disp_num ().set_preset (param::HelperDispNum::Preset_FLOAT_PERCENT);
	_desc_set.add_glob (base + ParamStage_BIAS, lin_ptr);

	// Type
	param::TplEnum *  enu_ptr = new param::TplEnum (
		"Diode\nAsym 1",
		"Stage %d distortion type\nStage %d disto type\nStage %d type\nS%d type",
		"",
		stage,
		"%s"
	);
	assert (enu_ptr->get_nat_max () == DistoStage::Type_NBR_ELT - 1);
	_desc_set.add_glob (base + ParamStage_TYPE, enu_ptr);

	// Gain
	log_ptr = new param::TplLog (
		double (_gain_min), double (_gain_max),
		"Stage %d Distortion Gain\nStage %d Gain\nS%d Gain",
		"dB",
		param::HelperDispNum::Preset_DB,
		stage,
		"%+5.1f"
	);
	_desc_set.add_glob (base + ParamStage_GAIN, log_ptr);

	// LPF Cutoff frequency
	log_ptr = new param::TplLog (
		20, 20480,
		"Stage %d output low-pass filter cutoff frequency\nStage %d output LPF frequency\nStage %d LPF cutoff frequency\nStage %d LPF frequency\nStage %d LPF freq\nStage %d LPF\nS%d LPF",
		"Hz",
		param::HelperDispNum::Preset_FLOAT_STD,
		stage,
		"%5.0f"
	);
	log_ptr->set_categ (piapi::ParamDescInterface::Categ_FREQ_HZ);
	_desc_set.add_glob (base + ParamStage_LPF_POST, log_ptr);
}



}  // namespace dist2
}  // namespace pi
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
