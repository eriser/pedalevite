/*****************************************************************************

        CtrlEdit.cpp
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
#include "mfx/pi/param/Tools.h"
#include "mfx/uitk/pg/CtrlEdit.h"
#include "mfx/uitk/pg/Tools.h"
#include "mfx/uitk/NodeEvt.h"
#include "mfx/uitk/PageMgrInterface.h"
#include "mfx/uitk/PageSwitcher.h"
#include "mfx/ui/Font.h"
#include "mfx/Cst.h"
#include "mfx/LocEdit.h"
#include "mfx/Model.h"
#include "mfx/View.h"

#include <cassert>



namespace mfx
{
namespace uitk
{
namespace pg
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



CtrlEdit::CtrlEdit (PageSwitcher &page_switcher, LocEdit &loc_edit, const std::vector <CtrlSrcNamed> &csn_list)
:	_csn_list_base (csn_list)
,	_page_switcher (page_switcher)
,	_loc_edit (loc_edit)
,	_model_ptr (0)
,	_view_ptr (0)
,	_page_ptr (0)
,	_page_size ()
,	_fnt_ptr (0)
,	_src_sptr (     new NText (Entry_SRC     ))
,	_step_rel_sptr (new NText (Entry_STEP_REL))
,	_minmax ()
,	_curve_sptr (   new NText (Entry_CURVE   ))
,	_u2b_sptr (     new NText (Entry_CONV_U2B))
,	_step_index (0)
,	_val_unit_w (0)
,	_csn_list_full (_csn_list_base)
,	_cls ()
,	_ctrl_link ()
,	_ctrl_index (-1)
,	_src_unknown_flag (false)
,	_src_unknown ()
{
	assert (! csn_list.empty ());

	for (size_t mm = 0; mm < _minmax.size (); ++mm)
	{
		MinMax &       gork = _minmax [mm];

		std::string    ratio;
		for (size_t k = 0; k < _nbr_steps; ++k)
		{
			ratio += "\xE2\x9A\xAB";   // MEDIUM BLACK CIRCLE U+26AB
			gork._step_sptr_arr [k] = TxtSPtr (new NText (_id_step_arr [mm] + k));
			gork._step_sptr_arr [k]->set_text (" " + ratio + " ");
		}

		gork._label_sptr    = TxtSPtr (new NText (_id_label_arr [mm]));
		gork._val_unit_sptr = TxtSPtr (new NText (_id_val_arr [mm]));
	}
	_minmax [0]._label_sptr->set_text ("Min  : ");
	_minmax [1]._label_sptr->set_text ("Max  : ");
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	CtrlEdit::do_connect (Model &model, const View &view, PageMgrInterface &page, Vec2d page_size, void *usr_ptr, const FontSet &fnt)
{
	assert (_loc_edit._slot_id >= 0);
	assert (_loc_edit._pi_type >= 0);
	assert (_loc_edit._param_index >= 0);

	_model_ptr = &model;
	_view_ptr  = &view;
	_page_ptr  = &page;
	_page_size = page_size;
	_fnt_ptr   = &fnt._m;

	const doc::Preset &  preset  = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot    = preset.use_slot (_loc_edit._slot_id);
	const doc::PluginSettings &   settings = slot.use_settings (_loc_edit._pi_type);
	auto           it_cls = settings._map_param_ctrl.find (_loc_edit._param_index);
	if (it_cls == settings._map_param_ctrl.end ())
	{
		_cls = doc::CtrlLinkSet ();
	}
	else
	{
		_cls = it_cls->second;
	}
	_ctrl_index = _loc_edit._ctrl_index;

	_src_sptr     ->set_font (*_fnt_ptr);
	_step_rel_sptr->set_font (*_fnt_ptr);
	_curve_sptr   ->set_font (*_fnt_ptr);
	_u2b_sptr     ->set_font (*_fnt_ptr);

	const int      scr_w = _page_size [0];
	const int      h_m   = _fnt_ptr->get_char_h ();

	_src_sptr     ->set_coord (Vec2d (0, h_m * 0));
	_step_rel_sptr->set_coord (Vec2d (0, h_m * 1));
	_curve_sptr   ->set_coord (Vec2d (0, h_m * 6));
	_u2b_sptr     ->set_coord (Vec2d (0, h_m * 7));

	_page_ptr->push_back (_src_sptr     );
	_page_ptr->push_back (_step_rel_sptr);

	std::array <int, _nbr_steps>  width_arr;
	int            total_w = 0;
	for (size_t k = 0; k < _nbr_steps; ++k)
	{
		_minmax [0]._step_sptr_arr [k]->set_font (*_fnt_ptr);
		_minmax [1]._step_sptr_arr [k]->set_font (*_fnt_ptr);
		width_arr [k] =
			_minmax [0]._step_sptr_arr [k]->get_bounding_box ().get_size () [0];
		total_w += width_arr [k];
	}
	assert (total_w <= scr_w);
	static_assert (_nbr_steps > 1, "_nbr_steps");
	const int      dist   = (scr_w - total_w) / (_nbr_steps - 1);
	for (int mm = 0; mm < int (_minmax.size ()); ++mm)
	{
		const int      y      = h_m * (2 + mm * 2);
		_minmax [mm]._label_sptr->set_font (*_fnt_ptr);
		_minmax [mm]._label_sptr->set_coord (Vec2d (0, y));
		_page_ptr->push_back (_minmax [mm]._label_sptr);
		const int      lbl_w  =
			_minmax [mm]._label_sptr->get_bounding_box ().get_size () [0];
		_val_unit_w = _page_size [0] - lbl_w;
		_minmax [mm]._val_unit_sptr->set_font (*_fnt_ptr);
		_minmax [mm]._val_unit_sptr->set_coord (Vec2d (lbl_w, y));
		_page_ptr->push_back (_minmax [mm]._val_unit_sptr);
		int            x_step = 0;
		for (size_t k = 0; k < _nbr_steps; ++k)
		{
			_minmax [mm]._step_sptr_arr [k]->set_coord (Vec2d (x_step, y + h_m));
			_page_ptr->push_back (_minmax [mm]._step_sptr_arr [k]);
			x_step += dist + width_arr [k];
		}
	}

	_page_ptr->push_back (_curve_sptr);
	_page_ptr->push_back (_u2b_sptr  );

	_src_unknown_flag = false;
	if (_loc_edit._ctrl_index >= 0)
	{
		update_ctrl_link ();
		const int      csn_index =
			Tools::find_ctrl_index (_ctrl_link._source, _csn_list_full);
		_src_unknown_flag = (csn_index < 0);
		_src_unknown      = _ctrl_link._source;
	}

	update_display ();
	_page_ptr->jump_to (Entry_SRC);
}



void	CtrlEdit::do_disconnect ()
{
	// Nothing
}



MsgHandlerInterface::EvtProp	CtrlEdit::do_handle_evt (const NodeEvt &evt)
{
	EvtProp        ret_val = EvtProp_PASS;

	const int      node_id = evt.get_target ();

	if (evt.is_cursor ())
	{
		if (evt.get_cursor () == NodeEvt::Curs_ENTER)
		{
			for (size_t mm = 0; mm < _minmax.size (); ++mm)
			{
				const int      step_base = _id_step_arr [mm];
				if (   node_id >= step_base
				    && node_id <  step_base + _nbr_steps)
				{
					_step_index = node_id - step_base;
				}
			}
		}
	}

	else if (evt.is_button_ex ())
	{
		const Button   but = evt.get_button_ex ();
		switch (but)
		{
		case Button_E:
			_page_switcher.switch_to (pg::PageType_PARAM_CONTROLLERS, 0);
			ret_val = EvtProp_CATCH;
			break;
		case Button_L:
			ret_val = change_something (node_id, -1);
			break;
		case Button_R:
			ret_val = change_something (node_id, +1);
			break;
		default:
			// Nothing
			break;
		}
	}

	return ret_val;
}



void	CtrlEdit::do_activate_preset (int index)
{
	_page_switcher.switch_to (PageType_EDIT_PROG, 0);
}



void	CtrlEdit::do_remove_plugin (int slot_id)
{
	if (slot_id == _loc_edit._slot_id)
	{
		_page_switcher.switch_to (PageType_EDIT_PROG, 0);
	}
}



void	CtrlEdit::do_set_param_ctrl (int slot_id, PiType type, int index, const doc::CtrlLinkSet &cls)
{
	if (   slot_id == _loc_edit._slot_id
	    && type    == _loc_edit._pi_type
	    && index   == _loc_edit._param_index)
	{
		_cls = cls;

		bool           stay_flag = true;
		if (! _loc_edit._ctrl_abs_flag)
		{
			const int      nbr_mod = int (cls._mod_arr.size ());
			if (_loc_edit._ctrl_index >= nbr_mod)
			{
				stay_flag = false;
			}
		}

		if (stay_flag)
		{
			update_display ();
		}
		else
		{
			_page_switcher.switch_to (PageType_PARAM_CONTROLLERS, 0);
		}
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	CtrlEdit::update_display ()
{
	const int      scr_w = _page_size [0];

	const std::vector <CtrlSrcNamed> csn_ports (
		Tools::make_port_list (*_model_ptr, *_view_ptr)
	);
	_csn_list_full = _csn_list_base;
	_csn_list_full.insert (
		_csn_list_full.begin (),
		csn_ports.begin (),
		csn_ports.end ()
	);

	const bool     active_flag = (_loc_edit._ctrl_index >= 0);

	_step_rel_sptr->show (active_flag);
	_curve_sptr->show (active_flag);
	_u2b_sptr->show (active_flag);

	PageMgrInterface::NavLocList  nav_list (1);
	nav_list [0]._node_id = Entry_SRC;

	for (size_t mm = 0; mm < _minmax.size (); ++mm)
	{
		_minmax [mm]._val_unit_sptr->show (active_flag);
		for (size_t k = 0; k < _nbr_steps; ++k)
		{
			_minmax [mm]._step_sptr_arr [k]->show (active_flag);
		}
	}

	std::string    src_txt ("Src  : ");
	if (! active_flag)
	{
		src_txt += "<Empty/Delete>";
	}
	else
	{
		update_ctrl_link ();

		std::string    min_txt = "Min  : ";
		if (! _loc_edit._ctrl_abs_flag)
		{
			min_txt = "Amp  : ";
		}
		else if (_ctrl_link._source.is_bipolar ())
		{
			min_txt = "Zero : ";
		}
		_minmax [0]._label_sptr->set_text (min_txt);

		const std::string src_name_multilabel =
			Tools::find_ctrl_name (_ctrl_link._source, _csn_list_full);
		const std::string src_name = pi::param::Tools::print_name_bestfit (
			scr_w, src_name_multilabel.c_str (),
			*_src_sptr, &NText::get_char_width
		);
		src_txt += src_name;

		char           txt_0 [127+1];

		std::string    step_txt ("Step : ");
		const bool     rel_flag = _ctrl_link._source.is_relative ();
		if (rel_flag)
		{
			nav_list.resize (nav_list.size () + 1);
			nav_list.back ()._node_id = Entry_STEP_REL;

			bool              frac_flag = (_ctrl_link._step > 0);
			int               inv_s_int = 0;
			if (frac_flag)
			{
				const float       inv_s     = 1.0f / _ctrl_link._step;
				inv_s_int = fstb::round_int (inv_s);
				frac_flag = fstb::is_eq (inv_s, float (inv_s_int), inv_s * 1e-3f);
			}
			if (frac_flag)
			{
				fstb::snprintf4all (
					txt_0, sizeof (txt_0), "1/%d", inv_s_int
				);
			}
			else
			{
				fstb::snprintf4all (
					txt_0, sizeof (txt_0), "%3.3f%%", _ctrl_link._step
				);
			}
			step_txt += txt_0;
		}
		else
		{
			step_txt += "--";
		}
		_step_rel_sptr->set_text (step_txt);

		if (_loc_edit._ctrl_abs_flag)
		{
			const std::array <float, 2>   val_arr =
			{
				_ctrl_link._base, _ctrl_link._base + _ctrl_link._amp
			};
			const PiType   type    = _loc_edit._pi_type;
			const int      index   = _loc_edit._param_index;
			for (size_t mm = 0; mm < _minmax.size (); ++mm)
			{
				for (size_t k = 0; k < _nbr_steps; ++k)
				{
					_minmax [mm]._step_sptr_arr [k]->show (true);
					nav_list.resize (nav_list.size () + 1);
					nav_list.back ()._node_id = _id_step_arr [mm] + k;
				}

				const float       val = val_arr [mm];
				Tools::set_param_text (
					*_model_ptr, *_view_ptr, _val_unit_w,
					index, val, _loc_edit._slot_id, type,
					0, *(_minmax [mm]._val_unit_sptr), 0, 0, true
				);
				_minmax [mm]._val_unit_sptr->show (true);
				_minmax [mm]._label_sptr->show (true);
			}
		}
		else
		{
			for (size_t k = 0; k < _nbr_steps; ++k)
			{
				_minmax [1]._step_sptr_arr [k]->show (false);
				nav_list.resize (nav_list.size () + 1);
				nav_list.back ()._node_id = _id_step_arr [0] + k;
			}

			fstb::snprintf4all (
				txt_0, sizeof (txt_0), "%+7.2f %%", _ctrl_link._amp * 100
			);
			_minmax [0]._val_unit_sptr->set_text (txt_0);
			_minmax [1]._val_unit_sptr->show (false);
			_minmax [1]._label_sptr->show (false);
		}

		const std::string curve_name =
			ControlCurve_get_name (_ctrl_link._curve);
		_curve_sptr->set_text ("Curve: " + curve_name);
		nav_list.resize (nav_list.size () + 1);
		nav_list.back ()._node_id = Entry_CURVE;

		std::string    curve_txt ("Range: ");
		if (! _ctrl_link._source.is_bipolar () && ! _loc_edit._ctrl_abs_flag)
		{
			curve_txt += (_ctrl_link._u2b_flag ? "Bipolar" : "Unipolar");

			nav_list.resize (nav_list.size () + 1);
			nav_list.back ()._node_id = Entry_CONV_U2B;
		}
		else
		{
			curve_txt += "--";   // U+2014 EM DASH
		}
		_u2b_sptr->set_text (curve_txt);
	}
	_src_sptr->set_text (src_txt);

	_page_ptr->set_nav_layout (nav_list);

	_page_ptr->invalidate (Rect (Vec2d (0, 0), _page_size));
}



void	CtrlEdit::update_ctrl_link ()
{
	assert (_loc_edit._ctrl_index >= 0);
	if (_loc_edit._ctrl_abs_flag)
	{
		assert (_cls._bind_sptr.get () != 0);
		_ctrl_link  = *_cls._bind_sptr;
	}
	else
	{
		assert (_cls._mod_arr [_loc_edit._ctrl_index].get () != 0);
		_ctrl_link  = *(_cls._mod_arr [_loc_edit._ctrl_index]);
		_ctrl_index = _loc_edit._ctrl_index;
	}
}



doc::CtrlLink &	CtrlEdit::use_ctrl_link (doc::CtrlLinkSet &cls) const
{
	assert (_loc_edit._ctrl_index >= 0);
	if (_loc_edit._ctrl_abs_flag)
	{
		assert (cls._bind_sptr.get () != 0);
		return *cls._bind_sptr;
	}

	assert (cls._mod_arr [_loc_edit._ctrl_index].get () != 0);
	return *(cls._mod_arr [_loc_edit._ctrl_index]);
}



const doc::CtrlLink &	CtrlEdit::use_ctrl_link (const doc::CtrlLinkSet &cls) const
{
	assert (_loc_edit._ctrl_index >= 0);
	if (_loc_edit._ctrl_abs_flag)
	{
		assert (cls._bind_sptr.get () != 0);
		return *cls._bind_sptr;
	}

	assert (cls._mod_arr [_loc_edit._ctrl_index].get () != 0);
	return *(cls._mod_arr [_loc_edit._ctrl_index]);
}



MsgHandlerInterface::EvtProp	CtrlEdit::change_something (int node_id, int dir)
{
	EvtProp        ret_val = EvtProp_PASS;

	if (node_id == Entry_SRC)
	{
		change_source (dir);
		ret_val = EvtProp_CATCH;
	}
	else if (node_id == Entry_CURVE)
	{
		change_curve (dir);
		ret_val = EvtProp_CATCH;
	}
	else if (node_id == Entry_CONV_U2B)
	{
		change_u2b ();
		ret_val = EvtProp_CATCH;
	}
	else
	{
		for (int mm = 0
		;	mm < int (_minmax.size ()) && ret_val == EvtProp_PASS
		;	++mm)
		{
			const int      step_base = _id_step_arr [mm];
			if (node_id >= step_base && node_id < step_base + _nbr_steps)
			{
				change_val (mm, node_id - step_base, dir);
			}
		}
	}

	return ret_val;
}



void	CtrlEdit::change_source (int dir)
{
	doc::CtrlLinkSet    cls (_cls);
	int            csn_index = find_next_source (dir);

	if (_loc_edit._ctrl_abs_flag)
	{
		if (csn_index == -1)
		{
			cls._bind_sptr.reset ();
			_loc_edit._ctrl_index = -1;
		}
		else
		{
			if (_loc_edit._ctrl_index < 0)
			{
				cls._bind_sptr = create_controller (csn_index);
				_loc_edit._ctrl_index = 0;
			}
			else
			{
				cls._bind_sptr->_source = create_source (csn_index);
			}
		}
	}

	else
	{
		if (csn_index == -1)
		{
			if (_loc_edit._ctrl_index >= 0)
			{
				cls._mod_arr.erase (
					cls._mod_arr.begin () + _loc_edit._ctrl_index
				);
				_loc_edit._ctrl_index = -1;
			}
		}
		else
		{
			if (_loc_edit._ctrl_index < 0)
			{
				if (_ctrl_index < 0)
				{
					_ctrl_index = int (cls._mod_arr.size ());
				}
				cls._mod_arr.insert (
					cls._mod_arr.begin () + _ctrl_index,
					create_controller (csn_index)
				);
				_loc_edit._ctrl_index = _ctrl_index;
			}
			else
			{
				cls._mod_arr [_loc_edit._ctrl_index]->_source =
					create_source (csn_index);
			}
		}
	}

	const int      slot_id = _loc_edit._slot_id;
	const PiType   type    = _loc_edit._pi_type;
	const int      index   = _loc_edit._param_index;
	_model_ptr->set_param_ctrl (slot_id, type, index, cls);
}



void	CtrlEdit::change_curve (int dir)
{
	doc::CtrlLinkSet  cls (_cls);
	doc::CtrlLink &   cl (use_ctrl_link (cls));

	cl._curve = ControlCurve (
		(cl._curve + dir + ControlCurve_NBR_ELT) % ControlCurve_NBR_ELT
	);

	const int      slot_id = _loc_edit._slot_id;
	const PiType   type    = _loc_edit._pi_type;
	const int      index   = _loc_edit._param_index;
	_model_ptr->set_param_ctrl (slot_id, type, index, cls);
}



void	CtrlEdit::change_u2b ()
{
	doc::CtrlLinkSet  cls (_cls);
	doc::CtrlLink &   cl (use_ctrl_link (cls));

	cl._u2b_flag = ! cl._u2b_flag;

	const int      slot_id = _loc_edit._slot_id;
	const PiType   type    = _loc_edit._pi_type;
	const int      index   = _loc_edit._param_index;
	_model_ptr->set_param_ctrl (slot_id, type, index, cls);
}



void	CtrlEdit::change_val (int mm, int step_index, int dir)
{
	doc::CtrlLinkSet  cls (_cls);
	doc::CtrlLink &   cl (use_ctrl_link (cls));

	const int      slot_id    = _loc_edit._slot_id;
	const PiType   type       = _loc_edit._pi_type;
	const int      index      = _loc_edit._param_index;
	const float    step       =
		float (Cst::_step_param / pow (10, step_index));

	if (_loc_edit._ctrl_abs_flag)
	{
		std::array <double, 2>  val_arr =
		{
			_ctrl_link._base, _ctrl_link._base + _ctrl_link._amp
		};

		val_arr [mm] = Tools::change_param (
			val_arr [mm], *_model_ptr, *_view_ptr, slot_id, type,
			index, step, step_index, dir
		);

		cl._base = float (              val_arr [0]);
		cl._amp  = float (val_arr [1] - val_arr [0]);
	}
	else
	{
		cl._base = 0;
		cl._amp  = fstb::limit (_ctrl_link._amp + step * dir, -4.0f, 4.0f);
	}

	_model_ptr->set_param_ctrl (slot_id, type, index, cls);
}



// -2: unknown (_src_unknown)
// -1: empty
// 0 to N-1: from _csn_list_full
int	CtrlEdit::find_next_source (int dir) const
{
	const int      nbr_csn = int (_csn_list_full.size ());
	int            csn_index = -1;
	if (_loc_edit._ctrl_index >= 0)
	{
		const doc::CtrlLink  ctrl_link (use_ctrl_link (_cls));
		csn_index = Tools::find_ctrl_index (ctrl_link._source, _csn_list_full);
		if (csn_index < 0)
		{
			assert (_src_unknown_flag);
			assert (_src_unknown == ctrl_link._source);
			csn_index = -2;
		}
	}

	csn_index += dir;
	int            val_min = (_src_unknown_flag) ? -2 : -1;
	const int      length  = nbr_csn - val_min;
	csn_index -= val_min;
	csn_index += length;
	csn_index %= length;
	csn_index += val_min;

	return csn_index;
}



doc::CtrlLinkSet::LinkSPtr	CtrlEdit::create_controller (int csn_index) const
{
	assert (csn_index != -1);

	doc::CtrlLinkSet::LinkSPtr	sptr;
	if (_ctrl_index >= 0)
	{
		sptr = doc::CtrlLinkSet::LinkSPtr (
			new doc::CtrlLink (_ctrl_link)
		);
	}
	else
	{
		sptr = doc::CtrlLinkSet::LinkSPtr (
			new doc::CtrlLink
		);
	}

	sptr->_source = create_source (csn_index);

	if (! _loc_edit._ctrl_abs_flag)
	{
		sptr->_amp = 0.10f;
	}

	return sptr;
}



ControlSource	CtrlEdit::create_source (int csn_index) const
{
	assert (csn_index != -1);

	if (csn_index == -2)
	{
		return _src_unknown;
	}

	return _csn_list_full [csn_index]._src;
}



const std::array <CtrlEdit::Entry, 2>	CtrlEdit::_id_label_arr =
{{ Entry_LABEL_MIN, Entry_LABEL_MAX }};

const std::array <CtrlEdit::Entry, 2>	CtrlEdit::_id_val_arr   =
{{ Entry_VAL_MIN  , Entry_VAL_MAX   }};

const std::array <CtrlEdit::Entry, 2>	CtrlEdit::_id_step_arr  =
{{ Entry_STEP_MIN , Entry_STEP_MAX  }};



}  // namespace pg
}  // namespace uitk
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
