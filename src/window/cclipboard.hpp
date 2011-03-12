#ifndef EE_WINDOWCCLIPBOARD_HPP
#define EE_WINDOWCCLIPBOARD_HPP

#include "cwindow.hpp"

namespace EE { namespace Window {

class cClipboard {
	public:
		virtual ~cClipboard();

		/** @return The Clipboard Text if available */
		virtual std::string GetText() = 0;

		/** @return The Clipboard Text if available ( as String ) */
		virtual String GetWideText() = 0;

		/** Set the current clipboard text */
		virtual void SetText( const std::string& Text ) = 0;

		cWindow * GetWindow() const;
	protected:
		friend class cWindow;

		cClipboard( cWindow * window );

		/** Initialize the clipboard manager. This is needed because the backends first create the instance of the clipboard and then initialize the window context. */
		virtual void Init() = 0;

		cWindow * mWindow;
};

}}

#endif
