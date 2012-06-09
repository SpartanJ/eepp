#include <eepp/window/backend/allegro5/cclipboardal.hpp>
#include <eepp/window/backend/allegro5/cwindowal.hpp>
#include <eepp/window/base.hpp>
#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

cClipboardAl::cClipboardAl( cWindow * window ) :
	cClipboard( window )
{
}

cClipboardAl::~cClipboardAl() {
}

void cClipboardAl::Init() {
}

void cClipboardAl::SetText( const std::string& Text ) {
}

std::string cClipboardAl::GetText() {
	return GetWideText().ToUtf8();
}

String cClipboardAl::GetWideText() {
	#if defined( EE_X11_PLATFORM )
	/** Not sure if activate this, sometimes freeze the application because of the notification delay. */
	/**
	cWindowAl * win = reinterpret_cast<cWindowAl*> ( mWindow );
	eeWindowHandler mDisplay = win->GetWindowHandler();
	X11Window mWin = win->GetX11Window();

	Atom clipboard = XInternAtom(mDisplay, "CLIPBOARD", True);
	X11Window owner = XGetSelectionOwner(mDisplay, clipboard);

	if (owner != None) {
		// Request the selection as XA_STRING
		XConvertSelection(mDisplay, clipboard, XA_STRING, clipboard, mWin, CurrentTime);
		XFlush (mDisplay);
		XEvent e;

		// Wait until the selection owner sends us a SelectionNotify event, confirming it has sent us the selection
		while (!XCheckTypedWindowEvent(mDisplay, mWin, SelectionNotify, &e));

		if (e.xselection.property != None) {
			Atom type;
			int format;
			unsigned long dummy, bytes, length;
			unsigned char *data;

			XGetWindowProperty(	mDisplay, mWin, clipboard, 0, 0, False, AnyPropertyType,&type, &format, &length, &bytes, &data);

			if (bytes) {
				XGetWindowProperty(mDisplay, mWin, clipboard, 0, bytes, False, AnyPropertyType, &type, &format, &length, &dummy, &data);
			}

			return String::FromUtf8((const char*)data);
		}
	}
	*/
	#endif

	return String();
}

}}}}

#endif
