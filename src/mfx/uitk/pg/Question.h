/*****************************************************************************

        Question.h
        Author: Laurent de Soras, 2016

Switch with call_page()
Pass a QArg structure as usr_ptr
Process the reply on the next connect() in the caller page.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (mfx_uitk_pg_Question_HEADER_INCLUDED)
#define mfx_uitk_pg_Question_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "mfx/uitk/NText.h"
#include "mfx/uitk/NWindow.h"
#include "mfx/uitk/PageInterface.h"

#include <memory>
#include <string>
#include <vector>



namespace mfx
{
namespace uitk
{

class PageSwitcher;

namespace pg
{



class Question
:	public PageInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	class QArg
	{
	public:
		std::string    _title;
		std::vector <std::string>
		               _choice_arr;
		int            _selection = 0; // Input/output
		std::set <int> _check_set;     // Input. Indexes showing check marks
		bool           _ok_flag;       // Output: choice validated or cancelled
	};

	explicit       Question (PageSwitcher &page_switcher);
	virtual        ~Question () = default;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// mfx::uitk::PageInterface
	virtual void   do_connect (Model &model, const View &view, PageMgrInterface &page, Vec2d page_size, void *usr_ptr, const FontSet &fnt);
	virtual void   do_disconnect ();

	// mfx::uitk::MsgHandlerInterface via mfx::uitk::PageInterface
	virtual EvtProp
	               do_handle_evt (const NodeEvt &evt);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum Entry
	{
		Entry_WINDOW = 1000000,
		Entry_TITLE
	};

	typedef std::shared_ptr <NText> TxtSPtr;
	typedef std::vector <TxtSPtr> TxtArray;
	typedef std::shared_ptr <NWindow> WinSPtr;

	PageSwitcher & _page_switcher;
	const View *   _view_ptr;     // 0 = not connected
	PageMgrInterface *            // 0 = not connected
	               _page_ptr;
	Vec2d          _page_size;

	WinSPtr        _menu_sptr;
	TxtSPtr        _title_sptr;
	TxtArray       _choice_list;

	QArg *         _arg_ptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Question ()                               = delete;
	               Question (const Question &other)          = delete;
	Question &     operator = (const Question &other)        = delete;
	bool           operator == (const Question &other) const = delete;
	bool           operator != (const Question &other) const = delete;

}; // class Question



}  // namespace pg
}  // namespace uitk
}  // namespace mfx



//#include "mfx/uitk/pg/Question.hpp"



#endif   // mfx_uitk_pg_Question_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
