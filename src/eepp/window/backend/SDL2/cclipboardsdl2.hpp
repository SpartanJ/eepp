#ifndef EE_WINDOWCCLIPBOARDSDL2_HPP
#define EE_WINDOWCCLIPBOARDSDL2_HPP

#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/base.hpp>
#include <eepp/window/cclipboard.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API ClipboardSDL : public Clipboard {
	public:
		virtual ~ClipboardSDL();

		std::string GetText();

		String GetWideText();

		void SetText( const std::string& Text );
	protected:
		friend class cWindowSDL;

		ClipboardSDL( Window::cWindow * window );

		void Init();
};

}}}}

#endif

#endif
