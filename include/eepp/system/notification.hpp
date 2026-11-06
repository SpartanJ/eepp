#ifndef EE_SYSTEM_NOTIFICATION_HPP
#define EE_SYSTEM_NOTIFICATION_HPP

#include <eepp/config.hpp>
#include <mutex>
#include <string>

namespace EE::System {

/** Non-interactive desktop notifications. macOS uses its native app notification center;
 * other desktop platforms use eepp's existing portable-file-dialogs backend.
 *
 * Applications need no PFD headers or instances. Calls are serialized and synchronous: helper
 * discovery and submission can block, so call off the UI thread. Backend-owned notification
 * resources persist as required (including the Windows notification-area icon).
 * OS permissions, Do Not Disturb and system sound preferences still apply.
 */
class EE_API Notification {
  public:
	Notification() = delete;

	/** Whether the current platform has a supported backend. Does not query permissions or submit a
	 * notification. macOS requires a signed .app bundle and macOS 10.14 or later. Linux/BSD
	 * discovery checks for Zenity, MateDialog, Qarma or KDialog. */
	static bool isSupported();

	/** Submits UTF-8 plain text, with no dialog or actions. Embedded NUL bytes are rejected.
	 * If supplied, @p error is cleared on success and describes validation, availability or
	 * timeout or backend failures. A true result means submission succeeded, not that the OS
	 * displayed it: system permissions and notification preferences can still suppress it.
	 * macOS authorization can wait up to 60 seconds for the user; submission waits at most
	 * five seconds. Other desktop helpers are bounded to five seconds after discovery. */
	static bool show( const std::string& title, const std::string& message,
					  std::string* error = nullptr );

  private:
	static std::mutex sMutex;
};

} // namespace EE::System

#endif
