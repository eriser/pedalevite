/*****************************************************************************

        FileIOWindows.h
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (mfx_FileIOWindows_HEADER_INCLUDED)
#define mfx_FileIOWindows_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "mfx/FileIOInterface.h"



namespace mfx
{



namespace ui
{
	class LedInterface;
}



class FileIOWindows
:	public FileIOInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       FileIOWindows (ui::LedInterface &led);
	virtual        ~FileIOWindows () = default;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// mfx::FileIOInterface
	virtual int    do_write_txt_file (const std::string &pathname, const std::string &content);
	virtual int    do_read_txt_file (const std::string &pathname, std::string &content);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	ui::LedInterface &
	               _led;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               FileIOWindows ()                               = delete;
	               FileIOWindows (const FileIOWindows &other)     = delete;
	FileIOWindows &
	               operator = (const FileIOWindows &other)        = delete;
	bool           operator == (const FileIOWindows &other) const = delete;
	bool           operator != (const FileIOWindows &other) const = delete;

}; // class FileIOWindows



}  // namespace mfx



//#include "mfx/FileIOWindows.hpp"



#endif   // mfx_FileIOWindows_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
