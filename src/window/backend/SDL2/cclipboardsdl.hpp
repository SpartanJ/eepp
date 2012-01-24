#ifndef EE_WINDOWCCLIPBOARDSDL2_HPP
#define EE_WINDOWCCLIPBOARDSDL2_HPP

#include "../../cbackend.hpp"
#include "base.hpp"

#ifdef EE_BACKEND_SDL2

#include "../../base.hpp"
#include "../../cclipboard.hpp"

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API cClipboardSDL : public cClipboard {
	public:
		virtual ~cClipboardSDL();

		std::string GetText();

		String GetWideText();

		void SetText( const std::string& Text );
	protected:
		friend class cWindowSDL;

		cClipboardSDL( Window::cWindow * window );

		void Init();
};

}}}}

#endif

#endif
