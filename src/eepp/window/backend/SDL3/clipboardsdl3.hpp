#ifndef EE_WINDOWCCLIPBOARDSDL3_HPP
#define EE_WINDOWCCLIPBOARDSDL3_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL3/base.hpp>

#ifdef EE_BACKEND_SDL3

#include <eepp/window/base.hpp>
#include <eepp/window/clipboard.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API ClipboardSDL : public Clipboard {
  public:
	virtual ~ClipboardSDL();

	std::string getText();

	String getWideText();

	void setText( const std::string& text );

	bool hasPrimarySelection() const;

	std::string getPrimarySelectionText();

	void setPrimarySelectionText( const std::string& text );

  protected:
	friend class WindowSDL;

	ClipboardSDL( EE::Window::Window* window );

	void init();
};

}}}} // namespace EE::Window::Backend::SDL3

#endif
#endif
