#ifndef EE_WINDOWCCLIPBOARDNULL_HPP
#define EE_WINDOWCCLIPBOARDNULL_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/clipboard.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API ClipboardNull : public Clipboard {
	public:
		virtual ~ClipboardNull();

		std::string getText();

		String getWideText();

		void setText( const std::string& Text );
	protected:
		friend class WindowNull;

		ClipboardNull( EE::Window::Window * window );

		void init();
};

}}}}

#endif
