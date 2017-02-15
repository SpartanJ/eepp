#ifndef EE_WINDOWCCLIPBOARDSDL2_HPP
#define EE_WINDOWCCLIPBOARDSDL2_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/base.hpp>
#include <eepp/window/clipboard.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API ClipboardSDL : public Clipboard {
	public:
		virtual ~ClipboardSDL();

		std::string getText();

		String getWideText();

		void setText( const std::string& Text );
	protected:
		friend class WindowSDL;

		ClipboardSDL( EE::Window::Window * window );

		void init();
};

}}}}

#endif

#endif
