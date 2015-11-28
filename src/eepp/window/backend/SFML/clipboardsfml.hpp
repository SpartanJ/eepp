#ifndef EE_WINDOWCCLIPBOARDSFML_HPP
#define EE_WINDOWCCLIPBOARDSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/base.hpp>
#include <eepp/window/clipboard.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API ClipboardSFML : public Clipboard {
	public:
		virtual ~ClipboardSFML();

		std::string GetText();

		String GetWideText();

		void SetText( const std::string& Text );
	protected:
		friend class WindowSFML;

		ClipboardSFML( EE::Window::Window * window );

		void Init();
};

}}}}

#endif

#endif
