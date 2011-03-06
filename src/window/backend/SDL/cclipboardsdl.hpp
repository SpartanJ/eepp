#ifndef EE_WINDOWCCLIPBOARDSDL_HPP
#define EE_WINDOWCCLIPBOARDSDL_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

#include "../../base.hpp"
#include "../../cclipboard.hpp"
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API cClipboardSDL : public cClipboard {
	public:
		virtual ~cClipboardSDL();

		std::string GetText();

		String GetWideText();

		void SetText( const std::string& Text );
		
		void SetText( const String& Text );
	protected:
		friend class cWindowSDL;

		cClipboardSDL( cWindow * window );

		void Init();

		SDL_SysWMinfo * mInfo;

		eeScrapType clipboard_convert_format(int type);

		int clipboard_convert_scrap(int type, char *dst, char *src, int srclen);

		void clipboard_get_scrap(int type, int *dstlen, char **dst);
};

}}}}

#endif

#endif
