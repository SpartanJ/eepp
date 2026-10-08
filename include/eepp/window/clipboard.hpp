#ifndef EE_WINDOWCCLIPBOARD_HPP
#define EE_WINDOWCCLIPBOARD_HPP

#include <eepp/window/window.hpp>

namespace EE { namespace Window {

class EE_API Clipboard {
  public:
	using Data = std::vector<Uint8>;
	using DataCallback = std::function<const void*( const char* mimeType, size_t* size )>;
	using CleanupCallback = std::function<void()>;

	virtual ~Clipboard();

	/** @return The Clipboard Text if available */
	virtual std::string getText() = 0;

	/** @return The Clipboard Text if available ( as String ) */
	virtual String getWideText() = 0;

	/** Set the current clipboard text */
	virtual void setText( const std::string& Text ) = 0;

	/** @return The parent window of the clipboard */
	EE::Window::Window* getWindow() const;

	/** @return True if primary selection is available */
	virtual bool hasPrimarySelection() const { return false; }

	/** @return The Clipboard Primary Selection Text if available */
	virtual std::string getPrimarySelectionText() { return ""; }

	/** Set the current clipboard primary selection text */
	virtual void setPrimarySelectionText( const std::string& text ) {}

	/**
	 * Offer non-text clipboard data in one or more MIME types. The data callback is invoked lazily
	 * when the operating system requests a format, and with a null MIME type when the offer is
	 * cleared or replaced. The returned data must remain valid until the cleanup callback is
	 * invoked. Only implemented by the SDL3 backend.
	 */
	virtual bool setData( DataCallback callback, CleanupCallback cleanup,
						  const std::vector<std::string>& mimeTypes ) {
		return false;
	}

	/** Clear all clipboard data. Only implemented by the SDL3 backend. */
	virtual bool clearData() { return false; }

	/** Get clipboard data for a MIME type. Only implemented by the SDL3 backend. */
	virtual Data getData( const std::string& mimeType ) { return {}; }

	/** @return The MIME types currently offered by the clipboard. SDL3 backend only. */
	virtual std::vector<std::string> getMimeTypes() { return {}; }

	/** @return Whether clipboard data is available for a MIME type. SDL3 backend only. */
	virtual bool hasData( const std::string& mimeType ) const { return false; }

  protected:
	friend class Window;

	Clipboard( EE::Window::Window* window );

	/** Initialize the clipboard manager. This is needed because the backends first create the
	 * instance of the clipboard and then initialize the window context. */
	virtual void init() = 0;

	EE::Window::Window* mWindow;
};

}} // namespace EE::Window

#endif
