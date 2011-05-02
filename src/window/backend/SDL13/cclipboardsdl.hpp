#ifndef EE_WINDOWCCLIPBOARDSDL13_HPP
#define EE_WINDOWCCLIPBOARDSDL13_HPP

#include "../../cbackend.hpp"
#include "base.hpp"

#ifdef EE_BACKEND_SDL_1_3

#include "../../base.hpp"
#include "../../cclipboard.hpp"

namespace EE { namespace Window { namespace Backend { namespace SDL13 {

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
};

}}}}

#endif

#endif
