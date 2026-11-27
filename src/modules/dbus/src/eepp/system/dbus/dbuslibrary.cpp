#include <eepp/system/dbus/dbuslibrary.hpp>

#include <dlfcn.h>
#include <initializer_list>

namespace EE::System::Dbus {

const DbusLibrary* DbusLibrary::load() {
	static const DbusLibrary library = [] {
		DbusLibrary bindings;
		void* handle = loadHandle();
		if ( handle && bindings.bind( handle ) )
			return bindings;
		return DbusLibrary{};
	}();
	return library.busGet ? &library : nullptr;
}

void* DbusLibrary::loadHandle() {
	// Keep the native library loaded for the lifetime of its clients.
	static void* const library = []() -> void* {
		for ( const char* name : { "libdbus-1.so.3", "libdbus-1.so" } ) {
			void* handle = dlopen( name, RTLD_NOW | RTLD_LOCAL );
			if ( !handle )
				continue;
			Uint32 ( *initThreads )();
			if ( loadSymbol( handle, initThreads, "dbus_threads_init_default" ) && initThreads() )
				return handle;
			dlclose( handle );
		}
		return nullptr;
	}();
	return library;
}

void* DbusLibrary::findSymbol( void* handle, const char* name ) {
	return dlsym( handle, name );
}

bool DbusLibrary::bind( void* handle ) {
	return loadSymbol( handle, busGet, "dbus_bus_get" ) &&
		   loadSymbol( handle, busRegister, "dbus_bus_register" ) &&
		   loadSymbol( handle, busGetUniqueName, "dbus_bus_get_unique_name" ) &&
		   loadSymbol( handle, busAddMatch, "dbus_bus_add_match" ) &&
		   loadSymbol( handle, busRemoveMatch, "dbus_bus_remove_match" ) &&
		   loadSymbol( handle, connectionAddFilter, "dbus_connection_add_filter" ) &&
		   loadSymbol( handle, connectionOpenPrivate, "dbus_connection_open_private" ) &&
		   loadSymbol( handle, connectionClose, "dbus_connection_close" ) &&
		   loadSymbol( handle, connectionUnref, "dbus_connection_unref" ) &&
		   loadSymbol( handle, connectionReadWrite, "dbus_connection_read_write" ) &&
		   loadSymbol( handle, connectionGetDispatchStatus,
					   "dbus_connection_get_dispatch_status" ) &&
		   loadSymbol( handle, connectionDispatch, "dbus_connection_dispatch" ) &&
		   loadSymbol( handle, connectionGetUnixFd, "dbus_connection_get_unix_fd" ) &&
		   loadSymbol( handle, connectionGetOutgoingSize, "dbus_connection_get_outgoing_size" ) &&
		   loadSymbol( handle, connectionRegisterObjectPath,
					   "dbus_connection_register_object_path" ) &&
		   loadSymbol( handle, connectionRegisterFallback, "dbus_connection_register_fallback" ) &&
		   loadSymbol( handle, connectionSend, "dbus_connection_send" ) &&
		   loadSymbol( handle, connectionSendWithReplyAndBlock,
					   "dbus_connection_send_with_reply_and_block" ) &&
		   loadSymbol( handle, messageNewMethodCall, "dbus_message_new_method_call" ) &&
		   loadSymbol( handle, messageNewMethodReturn, "dbus_message_new_method_return" ) &&
		   loadSymbol( handle, messageNewError, "dbus_message_new_error" ) &&
		   loadSymbol( handle, messageNewSignal, "dbus_message_new_signal" ) &&
		   loadSymbol( handle, messageGetPath, "dbus_message_get_path" ) &&
		   loadSymbol( handle, messageGetInterface, "dbus_message_get_interface" ) &&
		   loadSymbol( handle, messageGetMember, "dbus_message_get_member" ) &&
		   loadSymbol( handle, messageGetType, "dbus_message_get_type" ) &&
		   loadSymbol( handle, messageGetReplySerial, "dbus_message_get_reply_serial" ) &&
		   loadSymbol( handle, messageGetSender, "dbus_message_get_sender" ) &&
		   loadSymbol( handle, messageGetArgs, "dbus_message_get_args" ) &&
		   loadSymbol( handle, messageIterInitAppend, "dbus_message_iter_init_append" ) &&
		   loadSymbol( handle, messageIterInit, "dbus_message_iter_init" ) &&
		   loadSymbol( handle, messageIterGetBasic, "dbus_message_iter_get_basic" ) &&
		   loadSymbol( handle, messageIterNext, "dbus_message_iter_next" ) &&
		   loadSymbol( handle, messageIterGetArgType, "dbus_message_iter_get_arg_type" ) &&
		   loadSymbol( handle, messageIterRecurse, "dbus_message_iter_recurse" ) &&
		   loadSymbol( handle, messageIterAppendBasic, "dbus_message_iter_append_basic" ) &&
		   loadSymbol( handle, messageIterOpenContainer, "dbus_message_iter_open_container" ) &&
		   loadSymbol( handle, messageIterCloseContainer, "dbus_message_iter_close_container" ) &&
		   loadSymbol( handle, messageUnref, "dbus_message_unref" ) &&
		   loadSymbol( handle, errorInit, "dbus_error_init" ) &&
		   loadSymbol( handle, errorFree, "dbus_error_free" ) &&
		   loadSymbol( handle, errorIsSet, "dbus_error_is_set" ) &&
		   loadSymbol( handle, busGetPrivate, "dbus_bus_get_private" ) &&
		   loadSymbol( handle, connectionSetExitOnDisconnect,
					   "dbus_connection_set_exit_on_disconnect" ) &&
		   loadSymbol( handle, messageAppendArgs, "dbus_message_append_args" ) &&
		   loadSymbol( handle, validateBusName, "dbus_validate_bus_name" ) &&
		   loadSymbol( handle, validatePath, "dbus_validate_path" ) &&
		   loadSymbol( handle, validateUtf8, "dbus_validate_utf8" );
}

} // namespace EE::System::Dbus
