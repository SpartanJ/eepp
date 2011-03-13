#include "cclipboardsdl.hpp"
#include "cwindowsdl.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace SDL {

#define T(A, B, C, D)	(int)((A<<24)|(B<<16)|(C<<8)|(D<<0))
#define FORMAT_PREFIX	"EE_scrap_0x"

#if defined( EE_X11_PLATFORM )

static void * CurrentHandler = NULL;

static int clipboard_filter( const SDL_Event *event ) {
	/* Post all non-window manager specific events */
	if ( event->type != SDL_SYSWMEVENT ) {
		return 1;
	}

	Display * curDisplay = (Display *)CurrentHandler;

	/* Handle window-manager specific clipboard events */
	switch ( event->syswm.msg->event.xevent.type ) {
		/* Copy the selection from XA_CUT_BUFFER0 to the requested property */
		case SelectionRequest:
		{
			XSelectionRequestEvent *req;
			XEvent sevent;
			int seln_format;
			unsigned long nbytes;
			unsigned long overflow;
			unsigned char *seln_data;

			req = &event->syswm.msg->event.xevent.xselectionrequest;
			sevent.xselection.type = SelectionNotify;
			sevent.xselection.display = req->display;
			sevent.xselection.selection = req->selection;
			sevent.xselection.target = None;
			sevent.xselection.property = None;
			sevent.xselection.requestor = req->requestor;
			sevent.xselection.time = req->time;

			if ( XGetWindowProperty(curDisplay, DefaultRootWindow(curDisplay), XA_CUT_BUFFER0, 0, INT_MAX/4, False, req->target, &sevent.xselection.target, &seln_format, &nbytes, &overflow, &seln_data) == Success ) {
				if ( sevent.xselection.target == req->target ) {
					if ( sevent.xselection.target == XA_STRING ) {
						if ( seln_data[nbytes-1] == '\0' )
							--nbytes;
					}

					XChangeProperty(curDisplay, req->requestor, req->property, sevent.xselection.target, seln_format, PropModeReplace, seln_data, nbytes);
					sevent.xselection.property = req->property;
				}

				XFree(seln_data);
			}

			XSendEvent(curDisplay,req->requestor, False, 0, &sevent);
			XSync(curDisplay, False);
		}

		break;
	}

	/* Post the event for X11 clipboard reading above */
	return 1;
}
#endif

cClipboardSDL::cClipboardSDL( cWindow * window ) :
	cClipboard( window ),
	mInfo( NULL )
{
}

cClipboardSDL::~cClipboardSDL() {
}

void cClipboardSDL::Init() {
	#if defined( EE_X11_PLATFORM )
	/// Enable the special window hook events
	SDL_EventState( SDL_SYSWMEVENT, SDL_ENABLE );
	SDL_SetEventFilter( clipboard_filter );

	CurrentHandler = (void*)mWindow->GetWindowHandler();
	#endif

	#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
	mInfo = &( reinterpret_cast<cWindowSDL*> ( mWindow )->mWMinfo );
	#endif
}

#if defined( EE_X11_PLATFORM )
#ifdef X_HAVE_UTF8_STRING
#define TEXT_FORMAT XInternAtom(display, "UTF8_STRING", False)
#else
#define TEXT_FORMAT XA_STRING
#endif
#endif

void cClipboardSDL::SetText( const std::string& Text ) {
	#if defined( EE_X11_PLATFORM )
	eeWindowHandler display	= mInfo->info.x11.display;
	X11Window window		= mInfo->info.x11.wmwindow;
	Atom format				= TEXT_FORMAT;

	XChangeProperty( display, DefaultRootWindow( display ), XA_CUT_BUFFER0, format, 8, PropModeReplace, (const unsigned char *)Text.c_str(), Text.size() );

	if ( XGetSelectionOwner( display, XA_PRIMARY ) != window ) {
		XSetSelectionOwner( display, XA_PRIMARY, window, CurrentTime );
	}
	#elif EE_PLATFORM == EE_PLATFORM_WIN

	#endif
}

int cClipboardSDL::clipboard_convert_scrap( int type, char *dst, char *src, int srclen ) {
	int dstlen;

	dstlen = 0;
	switch (type) {
		case T('T', 'E', 'X', 'T'):
			if ( srclen == 0 )
				srclen = (int)strlen(src);

			if ( dst ) {
				while ( --srclen >= 0 ) {
					#if EE_PLATFORM == EE_PLATFORM_WIN
					if ( *src == '\r' )
					/* drop extraneous '\r' */;
					else
					#endif
					if ( *src == '\n' ) {
						*dst++ = '\r';
						++dstlen;
					} else {
						*dst++ = *src;
						++dstlen;
					}
					++src;
				}
				*dst = '\0';
				++dstlen;
			} else {
				while ( --srclen >= 0 ) {
					#if EE_PLATFORM == EE_PLATFORM_WIN
					if ( *src == '\r' )
					/* drop unspected '\r' */;
					else
					#endif
					++dstlen;
					++src;
				}
				++dstlen;
			}
			break;
		default:
			dstlen = *(int *)src;
			if ( dst ) {
				if ( srclen == 0 )
					memcpy(dst, src+sizeof(int), dstlen);
				else
					memcpy(dst, src+sizeof(int), srclen-sizeof(int));
			}
			break;
	}
	return dstlen;
}

eeScrapType cClipboardSDL::clipboard_convert_format( int type ) {
	switch (type) {
		case T('T', 'E', 'X', 'T'):
			#if defined( EE_X11_PLATFORM )
			return XA_STRING;
			#elif EE_PLATFORM == EE_PLATFORM_WIN
			return CF_TEXT;
			#endif
		default: {
			char format[ sizeof(FORMAT_PREFIX)+8+1 ];
			StrFormat(format, sizeof(FORMAT_PREFIX)+8+1, "%s%08lx", FORMAT_PREFIX, (unsigned long)type);

			#if defined( EE_X11_PLATFORM )
			return XInternAtom( mInfo->info.x11.display, format, False );
			#elif EE_PLATFORM == EE_PLATFORM_WIN
				#ifdef UNICODE
				return RegisterClipboardFormat( reinterpret_cast<LPCWSTR>( format ) );
				#else
				return RegisterClipboardFormat( reinterpret_cast<LPCSTR>( format ) );
				#endif
			#endif
		}
	}

	return 0;
}

void cClipboardSDL::clipboard_get_scrap( int type, int *dstlen, char **dst ) {
	eeScrapType format;
	*dstlen = 0;
	format = clipboard_convert_format( type );

#if defined( EE_X11_PLATFORM )
	X11Window owner;
	Atom selection;
	Atom seln_type;
	int seln_format;
	unsigned long nbytes;
	unsigned long overflow;
	char *src;

	mInfo->info.x11.lock_func();
	owner = XGetSelectionOwner( mInfo->info.x11.display , XA_PRIMARY);
	mInfo->info.x11.unlock_func();

	if ( ( owner == None ) || ( owner == mInfo->info.x11.wmwindow ) ) {
		owner = DefaultRootWindow( mInfo->info.x11.display );
		selection = XA_CUT_BUFFER0;
	} else {
		int selection_response = 0;
		SDL_Event event;

		owner = mInfo->info.x11.wmwindow;
		mInfo->info.x11.lock_func();

		selection = XInternAtom( mInfo->info.x11.display, "SELECTION", False);
		XConvertSelection( mInfo->info.x11.display, XA_PRIMARY, format, selection, owner, CurrentTime);

		mInfo->info.x11.unlock_func();

		while ( ! selection_response ) {
			SDL_WaitEvent(&event);
			if ( event.type == SDL_SYSWMEVENT ) {
				XEvent xevent = event.syswm.msg->event.xevent;
				if ( (xevent.type == SelectionNotify) && (xevent.xselection.requestor == owner) )
					selection_response = 1;
			}
		}
	}

	mInfo->info.x11.lock_func();
	if ( XGetWindowProperty( mInfo->info.x11.display, owner, selection, 0, INT_MAX/4, False, format, &seln_type, &seln_format, &nbytes, &overflow, (unsigned char **)&src ) == Success ) {
		if ( seln_type == format ) {
			*dstlen = clipboard_convert_scrap(type, NULL, src, nbytes);
			*dst = (char *)eeMalloc( *dstlen );

			if ( *dst == NULL )
				*dstlen = 0;
			else
				clipboard_convert_scrap(type, *dst, src, nbytes);
		}
		XFree(src);
	}
	mInfo->info.x11.unlock_func();
#elif EE_PLATFORM == EE_PLATFORM_WIN
	if ( IsClipboardFormatAvailable(format) && OpenClipboard( mInfo->window ) ) {
		HANDLE hMem;
		char *src;
		hMem = GetClipboardData(format);
		if ( hMem != NULL ) {
			src = (char *)GlobalLock(hMem);
			*dstlen = clipboard_convert_scrap(type, NULL, src, 0);
			*dst = (char *)eeMalloc( *dstlen );
			if ( *dst == NULL )
			*dstlen = 0;
			else
			clipboard_convert_scrap(type, *dst, src, 0);
			GlobalUnlock(hMem);
		}
		CloseClipboard();
	}
#endif
}

std::string cClipboardSDL::GetText() {
	std::string tStr;

	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	char *scrap = NULL;
	int scraplen;

	clipboard_get_scrap( T('T','E','X','T'), &scraplen, &scrap );

	if ( scraplen != 0 && strcmp( scrap, "SDL-\r-scrap") ) {
		char *cp;
		int   i;

		for ( cp = scrap, i = 0; i < scraplen; ++cp, ++i ) {
			if ( *cp == '\r' )
				*cp = '\n';
		}

		tStr.assign( scrap, scraplen-1 );
	}

	eeSAFE_DELETE_ARRAY( scrap );
	#else
		#warning cClipboardSDL::GetClipboardText() not implemented on this platform.
	#endif

	return tStr;
}

String cClipboardSDL::GetWideText() {
	String tStr;

	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	char * scrap = NULL;
	int scraplen;

	clipboard_get_scrap( T('T','E','X','T'), &scraplen, &scrap );

	if ( scraplen != 0 && strcmp( scrap, "SDL-\r-scrap" ) ) {
		tStr.resize( scraplen-1, ' ' );

		char *cp;
		int   i;

		for ( cp = scrap, i = 0; i < scraplen; ++cp, ++i ) {
			if ( *cp == '\r' )
				*cp = '\n';

			unsigned char y = *cp; // convert the nevative values to positives
			tStr[i] = y;
		}
	}

	eeSAFE_DELETE_ARRAY( scrap );
	#else
		#warning cClipboardSDL::GetWideText() not implemented on this platform.
	#endif

	return tStr;
}

}}}}

#endif
