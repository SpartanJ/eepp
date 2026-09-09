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

	bool setData( DataCallback callback, CleanupCallback cleanup,
				  const std::vector<std::string>& mimeTypes );

	bool clearData();

	Data getData( const std::string& mimeType );

	std::vector<std::string> getMimeTypes();

	bool hasData( const std::string& mimeType ) const;

  protected:
	friend class WindowSDL;

	ClipboardSDL( EE::Window::Window* window );

	void init();
};

}}}} // namespace EE::Window::Backend::SDL3

#endif
#endif
