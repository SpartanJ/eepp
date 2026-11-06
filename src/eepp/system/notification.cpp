#include <eepp/system/notification.hpp>

#include <eepp/system/clock.hpp>
#if EE_PLATFORM == EE_PLATFORM_MACOS
#include "notification_macos.hpp"
#else
#include <portable-file-dialogs/portable-file-dialogs.h>
#endif

namespace EE::System {

std::mutex Notification::sMutex;

bool Notification::isSupported() {
	std::lock_guard<std::mutex> lock( sMutex );
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::NotificationMacOS::isSupported();
#elif EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_LINUX || \
	EE_PLATFORM == EE_PLATFORM_BSD
	return pfd::settings::available();
#else
	return false;
#endif
}

bool Notification::show( const std::string& title, const std::string& message,
						 std::string* error ) {
	if ( error )
		error->clear();
	const auto fail = [error]( const char* reason ) {
		if ( error )
			*error = reason;
		return false;
	};
	if ( title.find( '\0' ) != std::string::npos || message.find( '\0' ) != std::string::npos )
		return fail( "Notification text contains a NUL byte" );
#if EE_PLATFORM == EE_PLATFORM_MACOS
	std::lock_guard<std::mutex> lock( sMutex );
	return Private::NotificationMacOS::show( title, message, error );
#else
	if ( !isSupported() )
		return fail( "Desktop notification backend is unavailable" );

	// PFD owns process-wide settings and the Windows notification-area icon. Serializing calls
	// prevents concurrent notifications from replacing that icon while it is being initialized.
	std::lock_guard<std::mutex> lock( sMutex );
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	// Desktop helpers can interpret markup in their combined title/body text. PFD still handles
	// all platform argument quoting; this only preserves our plain-text API contract.
	const auto escape = []( const std::string& text ) {
		std::string escaped;
		escaped.reserve( text.size() );
		for ( char c : text ) {
			if ( c == '&' )
				escaped += "&amp;";
			else if ( c == '<' )
				escaped += "&lt;";
			else if ( c == '>' )
				escaped += "&gt;";
			else
				escaped += c;
		}
		return escaped;
	};
	pfd::notify notice( escape( title ), escape( message ) );
#else
	pfd::notify notice( title, message );
#endif
	// Some desktop helpers remain alive with a tray icon. Don't let PFD's destructor wait
	// indefinitely for that process; it owns cancellation and reaping as well as delivery.
	Clock timeout;
	while ( !notice.ready( 100 ) ) {
		if ( timeout.getElapsedTime() >= Seconds( 5 ) ) {
			notice.kill();
			return fail( "Desktop notification helper timed out" );
		}
	}
	const int result = notice.result();
	if ( result != 0 ) {
		if ( error )
			*error =
				"Desktop notification backend failed (exit code " + std::to_string( result ) + ")";
		return false;
	}
	return true;
#endif
}

} // namespace EE::System
