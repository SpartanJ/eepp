#ifndef EE_WINDOWCCLIPBOARDAl_HPP
#define EE_WINDOWCCLIPBOARDAl_HPP

#include <eepp/window/cbackend.hpp>

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include <eepp/window/base.hpp>
#include <eepp/window/cclipboard.hpp>

namespace EE { namespace Window { namespace Backend { namespace Al {

class EE_API cClipboardAl : public cClipboard {
	public:
		virtual ~cClipboardAl();

		std::string GetText();

		String GetWideText();

		void SetText( const std::string& Text );
	protected:
		friend class cWindowAl;

		cClipboardAl( Window::cWindow * window );

		void Init();
};

}}}}

#endif

#endif
