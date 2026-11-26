#ifndef EE_WINDOWCCLIPBOARD_HPP
#define EE_WINDOWCCLIPBOARD_HPP

#include <eepp/window/window.hpp>

namespace EE::UI {

class PrimarySelection;

} // namespace EE::UI

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

	/** @return True if the primary selection currently contains text. */
	virtual bool hasPrimarySelection() const;

	/** @return Native or emulated primary selection text. On emulated backends, fall back to
	 * clipboard text when the emulated selection is empty. */
	virtual std::string getPrimarySelectionText();

	/** Set the current clipboard primary selection text */
	virtual void setPrimarySelectionText( const std::string& text );

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

	/**
	 * Offers fixed data in several MIME types, in order of preference: a convenience over
	 * setData() for content that is already known. The data is copied and kept until the offer is
	 * replaced or cleared. To keep the content pasteable as text, also offer it under
	 * getTextMimeTypes(). Only implemented by the SDL3 backend.
	 * @return False if the backend does not support clipboard data.
	 */
	bool setMimeData( std::vector<std::pair<std::string, std::string>> dataByMimeType );

	/** @return getData() as a string (empty if the MIME type is not available). */
	std::string getDataString( const std::string& mimeType );

	/** @return The MIME types that text is requested as across platforms (UTF-8 text first),
	 * for offers made with setData() / setMimeData() that must also be readable as text. */
	static const std::vector<std::string>& getTextMimeTypes();

	/** @return True if the video backend supports primary selection. */
	virtual bool supportsPrimarySelection() const { return false; }

	/** Set primary selection text and associate an emulated selection with its source owner. */
	void publishPrimarySelection( const std::string& text, const void* owner );

	/** Clear the emulated selection if it belongs to owner. */
	static void clearEmulatedPrimarySelection( const void* owner );

	/** Complete any pending publication before reading text for a primary selection paste. */
	std::string getPrimarySelectionPasteText();

  protected:
	friend class Window;

	Clipboard( EE::Window::Window* window );

	/** Initialize the clipboard manager. This is needed because the backends first create the
	 * instance of the clipboard and then initialize the window context. */
	virtual void init() = 0;

	/** @return False when EEPP_DISABLE_NATIVE_PRIMARY_SELECTION is "1". This is a test-only
	 * override: the unit tests set it so that selections use the emulated primary selection and
	 * never replace the desktop's PRIMARY. Applications must not rely on it. */
	static bool nativePrimarySelectionEnabled();

	EE::Window::Window* mWindow;

  private:
	friend class EE::UI::PrimarySelection;

	/** Register the UI thread callback that completes a deferred primary selection publication. */
	static void setPrimarySelectionFlushCallback( void ( *callback )() );

	static void setEmulatedPrimarySelection( const std::string& text, const void* owner );

	static std::string sEmulatedPrimarySelection;
	static const void* sEmulatedPrimarySelectionOwner;
	static void ( *sPrimarySelectionFlushCallback )();
};

}} // namespace EE::Window

#endif
