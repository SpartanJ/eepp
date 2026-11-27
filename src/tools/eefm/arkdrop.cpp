#include "arkdrop.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
#include <eepp/system/dbus/dbuslibrary.hpp>
#endif

using namespace EE::Window;

namespace eefm {

namespace {

constexpr const char* ServiceType = "application/x-kde-ark-dndextract-service";
constexpr const char* PathType = "application/x-kde-ark-dndextract-path";

} // namespace

const std::vector<std::string>& ArkDrop::mimeTypes() {
	static const std::vector<std::string> types{ ServiceType, PathType };
	return types;
}

bool ArkDrop::isArkDrop( const DataDrop& drop ) {
	return drop.find( ServiceType ) && drop.find( PathType );
}

std::string ArkDrop::extract( const DataDrop& drop, const std::string& folder,
							  const Translator& translator ) {
	auto unavailable = [&] {
		return translator
			.getString( "eefm_dbus_unavailable",
						"D-Bus is unavailable (libdbus-1 could not be loaded)" )
			.toUtf8();
	};
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	using EE::System::Dbus::DbusLibrary;
	using namespace EE::System::Dbus::Native;
	constexpr int DBusReplyTimeoutMs = 25000;
	const std::string* service = drop.find( ServiceType );
	const std::string* path = drop.find( PathType );
	const auto* library = DbusLibrary::load();
	if ( !library )
		return unavailable();
	const auto& dbus = *library;
	DBusError error;
	dbus.errorInit( &error );
	auto takeError = [&]( std::string fallback ) {
		std::string message = dbus.errorIsSet( &error ) && error.message
								  ? std::string( error.message )
								  : std::move( fallback );
		dbus.errorFree( &error );
		return message;
	};
	// The drag's contents come from another application: libdbus aborts the process on an
	// invalid name, path or string, so everything is validated before building the call.
	auto valid = [&]( const std::string* value, auto validate ) {
		return value && value->find( '\0' ) == std::string::npos &&
			   validate( value->c_str(), &error );
	};
	if ( !valid( service, dbus.validateBusName ) || !valid( path, dbus.validatePath ) )
		return takeError( translator
							  .getString( "eefm_ark_drag_invalid",
										  "The drag did not name the Ark instance it came from" )
							  .toUtf8() );
	if ( !valid( &folder, dbus.validateUtf8 ) )
		return takeError(
			translator
				.getString( "eefm_ark_path_invalid",
							"Ark can only extract into folders whose path is valid UTF-8" )
				.toUtf8() );
	// A private connection: libdbus' shared one belongs to whoever else uses it in the process.
	DBusConnection* connection = dbus.busGetPrivate( DBusBusSession, &error );
	if ( !connection )
		return takeError( translator
							  .getString( "eefm_dbus_connection_failed",
										  "Could not connect to the D-Bus session bus" )
							  .toUtf8() );
	dbus.connectionSetExitOnDisconnect( connection, 0 );
	std::string result;
	if ( DBusMessage* call =
			 dbus.messageNewMethodCall( service->c_str(), path->c_str(), "org.kde.ark.DndExtract",
										"extractSelectedFilesTo" ) ) {
		const char* destination = folder.c_str();
		if ( !dbus.messageAppendArgs( call, DBusTypeString, &destination, DBusTypeInvalid ) ) {
			result = translator.getString( "eefm_out_of_memory", "Out of memory" ).toUtf8();
		} else if ( DBusMessage* reply = dbus.connectionSendWithReplyAndBlock(
						connection, call, DBusReplyTimeoutMs, &error ) ) {
			dbus.messageUnref( reply );
		} else {
			result = takeError(
				translator.getString( "eefm_ark_no_answer", "Ark did not answer" ).toUtf8() );
		}
		dbus.messageUnref( call );
	} else {
		result =
			translator
				.getString( "eefm_ark_instance_invalid", "Invalid Ark D-Bus name or object path" )
				.toUtf8();
	}
	dbus.connectionClose( connection );
	dbus.connectionUnref( connection );
	return result;
#else
	return unavailable();
#endif
}

} // namespace eefm
