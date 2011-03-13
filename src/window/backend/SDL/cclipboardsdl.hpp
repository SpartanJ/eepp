#ifndef EE_WINDOWCCLIPBOARDSDL_HPP
#define EE_WINDOWCCLIPBOARDSDL_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

#include "../../base.hpp"
#include "../../cclipboard.hpp"
#include <SDL/SDL.h>

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
#include <SDL/SDL_syswm.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API cClipboardSDL : public cClipboard {
	public:
		virtual ~cClipboardSDL();

		std::string GetText();

		String GetWideText();

		void SetText( const std::string& Text );
	protected:
		friend class cWindowSDL;

		cClipboardSDL( cWindow * window );

		void Init();

		#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
		SDL_SysWMinfo * mInfo;
		#else
		void * mInfo;
		#endif

		eeScrapType clipboard_convert_format(int type);

		int clipboard_convert_scrap(int type, char *dst, char *src, int srclen);

		void clipboard_get_scrap(int type, int *dstlen, char **dst);
};

}}}}

#endif

#endif
