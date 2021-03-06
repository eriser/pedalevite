/*****************************************************************************

        Notch.h
        Author: Laurent de Soras, 2017

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (mfx_pi_nzbl_Notch_HEADER_INCLUDED)
#define mfx_pi_nzbl_Notch_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "mfx/dsp/iir/Biquad.h"
#include "mfx/dsp/dyn/EnvFollowerRms.h"



namespace mfx
{
namespace pi
{
namespace nzbl
{



class Notch
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               Notch ()  = default;
	virtual        ~Notch () = default;

	void           reset (double sample_freq, int max_buf_len, float buf_0_ptr [], float buf_1_ptr []);
	void           set_freq (float freq);
	void           set_q (float q);
	void           set_lvl (float lvl);
	void           process_block (float dst_ptr [], const float src_ptr [], int nbr_spl);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	void           update_filter ();

	float          _sample_freq  = 0;     // Hz, > 0. 0 = not initialized
	float          _inv_fs       = 0;     // s, > 0. 0 = not initialized
	int            _max_buf_size = 0;     // Samples, > 0. 0 = not initialized

	float          _freq         = 5000;  // Hz, > 0
	float          _q            = 0.25f; // > 0
	float          _lvl          = 1;     // >= 0. 0 = off
	float          _rel_thr      = 20;    // Threshold (relatvie to _lvl) above which the notch has no effect

	bool           _flt_dirty_flag = true;
	dsp::iir::Biquad
	               _bpf;
	dsp::dyn::EnvFollowerRms
	               _env;

	float *        _buf_env_ptr  = 0;     // Aligned to 16 bytes. 0 = not set
	float *        _buf_bpf_ptr  = 0;     // Aligned to 16 bytes. 0 = not set



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Notch (const Notch &other)             = delete;
	Notch &        operator = (const Notch &other)        = delete;
	bool           operator == (const Notch &other) const = delete;
	bool           operator != (const Notch &other) const = delete;

}; // class Notch



}  // namespace nzbl
}  // namespace pi
}  // namespace mfx



//#include "mfx/pi/nzbl/Notch.hpp"



#endif   // mfx_pi_nzbl_Notch_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
