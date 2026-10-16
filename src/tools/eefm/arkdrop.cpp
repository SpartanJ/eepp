#include "arkdrop.hpp"

#include <eepp/system/sys.hpp>

using namespace EE::System;
using namespace EE::Window;

namespace eefm {

namespace {

constexpr const char* ServiceType = "application/x-kde-ark-dndextract-service";
constexpr const char* PathType = "application/x-kde-ark-dndextract-path";

struct DBusConnection;
struct DBusMessage;

/** libdbus' public DBusError layout. */
struct DBusError {
	const char* name;
	const char* message;
	unsigned int flags;
	void* padding;
};

constexpr int DBusBusSession = 0;
constexpr int DBusTypeInvalid = 0;
constexpr int DBusTypeString = 's';
constexpr int DBusReplyTimeoutMs = 25000;

/** The libdbus functions one method call needs, loaded at runtime (as eepp's AT-SPI backend
 * does), so eefm has no build-time D-Bus dependency. Loaded once and never unloaded. */
class DBusFunctions {
  public:
	static const DBusFunctions& get() {
		static const DBusFunctions functions;
		return functions;
	}

	bool loaded() const { return mLoaded; }

	void ( *errorInit )( DBusError* ){ nullptr };
	void ( *errorFree )( DBusError* ){ nullptr };
	unsigned int ( *errorIsSet )( const DBusError* ){ nullptr };
	DBusConnection* ( *busGetPrivate )(int, DBusError*){ nullptr };
	void ( *connectionSetExitOnDisconnect )( DBusConnection*, unsigned int ){ nullptr };
	void ( *connectionClose )( DBusConnection* ){ nullptr };
	void ( *connectionUnref )( DBusConnection* ){ nullptr };
	DBusMessage* ( *messageNewMethodCall )(const char*, const char*, const char*,
										   const char*){ nullptr };
	unsigned int ( *messageAppendArgs )( DBusMessage*, int, ... ){ nullptr };
	DBusMessage* ( *connectionSendWithReplyAndBlock )(DBusConnection*, DBusMessage*, int,
													  DBusError*){ nullptr };
	void ( *messageUnref )( DBusMessage* ){ nullptr };
	/** Validators that report instead of asserting, unlike the message constructors. */
	unsigned int ( *validateBusName )( const char*, DBusError* ){ nullptr };
	unsigned int ( *validatePath )( const char*, DBusError* ){ nullptr };
	unsigned int ( *validateUtf8 )( const char*, DBusError* ){ nullptr };

  private:
	DBusFunctions() {
		void* handle = Sys::loadObject( "libdbus-1.so.3" );
		if ( !handle )
			handle = Sys::loadObject( "libdbus-1.so" );
		mLoaded = handle && load( handle, errorInit, "dbus_error_init" ) &&
				  load( handle, errorFree, "dbus_error_free" ) &&
				  load( handle, errorIsSet, "dbus_error_is_set" ) &&
				  load( handle, busGetPrivate, "dbus_bus_get_private" ) &&
				  load( handle, connectionSetExitOnDisconnect,
						"dbus_connection_set_exit_on_disconnect" ) &&
				  load( handle, connectionClose, "dbus_connection_close" ) &&
				  load( handle, connectionUnref, "dbus_connection_unref" ) &&
				  load( handle, messageNewMethodCall, "dbus_message_new_method_call" ) &&
				  load( handle, messageAppendArgs, "dbus_message_append_args" ) &&
				  load( handle, connectionSendWithReplyAndBlock,
						"dbus_connection_send_with_reply_and_block" ) &&
				  load( handle, messageUnref, "dbus_message_unref" ) &&
				  load( handle, validateBusName, "dbus_validate_bus_name" ) &&
				  load( handle, validatePath, "dbus_validate_path" ) &&
				  load( handle, validateUtf8, "dbus_validate_utf8" );
	}

	template <typename Function>
	static bool load( void* handle, Function& function, const char* name ) {
		function = reinterpret_cast<Function>( Sys::loadFunction( handle, name ) );
		return function != nullptr;
	}

	bool mLoaded{ false };
};

} // namespace

const std::vector<std::string>& ArkDrop::mimeTypes() {
	static const std::vector<std::string> types{ ServiceType, PathType };
	return types;
}

bool ArkDrop::isArkDrop( const DataDrop& drop ) {
	return drop.find( ServiceType ) && drop.find( PathType );
}

std::string ArkDrop::extract( const DataDrop& drop, const std::string& folder ) {
	const std::string* service = drop.find( ServiceType );
	const std::string* path = drop.find( PathType );
	const auto& dbus = DBusFunctions::get();
	if ( !dbus.loaded() )
		return "D-Bus is unavailable (libdbus-1 could not be loaded)";
	DBusError error;
	dbus.errorInit( &error );
	auto takeError = [&]( const char* fallback ) {
		std::string message = dbus.errorIsSet( &error ) && error.message ? error.message : fallback;
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
		return takeError( "The drag did not name the Ark instance it came from" );
	if ( !valid( &folder, dbus.validateUtf8 ) )
		return takeError( "Ark can only extract into folders whose path is valid UTF-8" );
	// A private connection: libdbus' shared one belongs to whoever else uses it in the process.
	DBusConnection* connection = dbus.busGetPrivate( DBusBusSession, &error );
	if ( !connection )
		return takeError( "Could not connect to the D-Bus session bus" );
	dbus.connectionSetExitOnDisconnect( connection, 0 );
	std::string result;
	if ( DBusMessage* call =
			 dbus.messageNewMethodCall( service->c_str(), path->c_str(), "org.kde.ark.DndExtract",
										"extractSelectedFilesTo" ) ) {
		const char* destination = folder.c_str();
		if ( !dbus.messageAppendArgs( call, DBusTypeString, &destination, DBusTypeInvalid ) ) {
			result = "Out of memory";
		} else if ( DBusMessage* reply = dbus.connectionSendWithReplyAndBlock(
						connection, call, DBusReplyTimeoutMs, &error ) ) {
			dbus.messageUnref( reply );
		} else {
			result = takeError( "Ark did not answer" );
		}
		dbus.messageUnref( call );
	} else {
		result = "Invalid Ark D-Bus name or object path";
	}
	dbus.connectionClose( connection );
	dbus.connectionUnref( connection );
	return result;
}

} // namespace eefm
