#ifndef EE_WINDOWCCLIPBOARDNULL_HPP
#define EE_WINDOWCCLIPBOARDNULL_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/cclipboard.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API ClipboardNull : public Clipboard {
	public:
		virtual ~ClipboardNull();

		std::string GetText();

		String GetWideText();

		void SetText( const std::string& Text );
	protected:
		friend class cWindowNull;

		ClipboardNull( Window::cWindow * window );

		void Init();
};

}}}}

#endif
