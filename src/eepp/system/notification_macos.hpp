#ifndef EE_SYSTEM_NOTIFICATION_MACOS_HPP
#define EE_SYSTEM_NOTIFICATION_MACOS_HPP

#include <string>

namespace EE::System::Private {

/** Native app-identity backend: AppleScript cannot register the calling application. */
class NotificationMacOS {
  public:
	static bool isSupported();

	static bool show( const std::string& title, const std::string& message, std::string* error );
};

} // namespace EE::System::Private

#endif
