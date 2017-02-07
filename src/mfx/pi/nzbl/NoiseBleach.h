/*****************************************************************************

        NoiseBleach.h
        Author: Laurent de Soras, 2017

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (mfx_pi_nzbl_NoiseBleach_HEADER_INCLUDED)
#define mfx_pi_nzbl_NoiseBleach_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/util/NotificationFlag.h"
#include "fstb/util/NotificationFlagCascadeSingle.h"
#include "fstb/AllocAlign.h"
#include "mfx/pi/nzbl/Cst.h"
#include "mfx/pi/nzbl/NoiseBleachDesc.h"
#include "mfx/pi/nzbl/Notch.h"
#include "mfx/pi/ParamStateSet.h"
#include "mfx/piapi/PluginInterface.h"

#include <vector>



namespace mfx
{
namespace pi
{
namespace nzbl
{



class NoiseBleach
:	public piapi::PluginInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               NoiseBleach ();
	virtual        ~NoiseBleach () = default;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// mfx::piapi::PluginInterface
	virtual State  do_get_state () const;
	virtual double do_get_param_val (piapi::ParamCateg categ, int index, int note_id) const;
	virtual int    do_reset (double sample_freq, int max_buf_len, int &latency);
	virtual void   do_process_block (ProcInfo &proc);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	class Channel
	{
	public:
		std::array <Notch, Cst::_nbr_notches>
		               _notch_arr;
	};
	typedef std::array <Channel, _max_nbr_chn> ChannelArray;

	typedef std::vector <float, fstb::AllocAlign <float, 16> > BufAlign;

	void           update_param (bool force_flag);
	void           update_all_levels ();

	State          _state;

	NoiseBleachDesc
	               _desc;
	ParamStateSet  _state_set;
	float          _sample_freq;        // Hz, > 0. <= 0: not initialized
	float          _inv_fs;             // 1 / _sample_freq, <= 0: not initialized

	fstb::util::NotificationFlag
	               _param_change_flag;
	fstb::util::NotificationFlagCascadeSingle
	               _param_change_flag_misc;
	std::array <fstb::util::NotificationFlagCascadeSingle, Cst::_nbr_notches>
	               _param_change_flag_notch_arr;

	ChannelArray   _chn_arr;
	float          _noise_ratio;        // 0 = pure denoised output, 1 = pure noise (phase-inverted)

	float          _lvl_base;
	std::array <float, Cst::_nbr_notches>
	               _lvl_notch_arr;

	std::array <BufAlign, 2>
	               _buf_tmp_arr;




/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               NoiseBleach (const NoiseBleach &other)       = delete;
	NoiseBleach &  operator = (const NoiseBleach &other)        = delete;
	bool           operator == (const NoiseBleach &other) const = delete;
	bool           operator != (const NoiseBleach &other) const = delete;

}; // class NoiseBleach



}  // namespace nzbl
}  // namespace pi
}  // namespace mfx



//#include "mfx/pi/nzbl/NoiseBleach.hpp"



#endif   // mfx_pi_nzbl_NoiseBleach_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/