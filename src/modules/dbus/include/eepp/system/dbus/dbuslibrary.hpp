#ifndef EE_SYSTEM_DBUS_DBUSLIBRARY_HPP
#define EE_SYSTEM_DBUS_DBUSLIBRARY_HPP

#include <eepp/config.hpp>

namespace EE::System::Dbus {

namespace Native {

// Public libdbus ABI for clients that build without its development headers.
struct DBusConnection;
struct DBusMessage;
struct DBusError {
	const char* name;
	const char* message;
	unsigned int dummy1 : 1;
	unsigned int dummy2 : 1;
	unsigned int dummy3 : 1;
	unsigned int dummy4 : 1;
	unsigned int dummy5 : 1;
	void* padding1;
};

struct DBusMessageIter {
	void* dummy1;
	void* dummy2;
	Uint32 dummy3;
	int dummy4;
	int dummy5;
	int dummy6;
	int dummy7;
	int dummy8;
	int dummy9;
	int dummy10;
	int dummy11;
	int pad1;
	void* pad2;
	void* pad3;
};

using DBusHandlerResult = int;
using DBusDispatchStatus = int;
using DBusMessageFunction = DBusHandlerResult ( * )( DBusConnection*, DBusMessage*, void* );
using DBusFreeFunction = void ( * )( void* );

constexpr DBusDispatchStatus DBusDispatchDataRemains = 0;
constexpr int DBusBusSession = 0;
constexpr int DBusTypeInvalid = 0;
constexpr int DBusTypeString = 's';
constexpr int DBusMessageTypeMethodReturn = 2;
constexpr int DBusMessageTypeError = 3;

struct DBusObjectPathVTable {
	void ( *unregisterFunction )( DBusConnection*, void* );
	DBusMessageFunction messageFunction;
	void ( *padding1 )( void* );
	void ( *padding2 )( void* );
	void ( *padding3 )( void* );
	void ( *padding4 )( void* );
};

} // namespace Native

class DbusTransportLibrary;

/** Runtime libdbus entry points for synchronous calls and object dispatch. */
class DbusLibrary {
  public:
	/** Returns the shared bindings for this module, or null if libdbus is unavailable. */
	static const DbusLibrary* load();

	Native::DBusConnection* ( *busGet )(int, Native::DBusError*){};
	Uint32 ( *busRegister )( Native::DBusConnection*, Native::DBusError* ){};
	const char* ( *busGetUniqueName )(Native::DBusConnection*){};
	void ( *busAddMatch )( Native::DBusConnection*, const char*, Native::DBusError* ){};
	void ( *busRemoveMatch )( Native::DBusConnection*, const char*, Native::DBusError* ){};
	Uint32 ( *connectionAddFilter )( Native::DBusConnection*, Native::DBusMessageFunction, void*,
									 Native::DBusFreeFunction ){};
	Native::DBusConnection* ( *connectionOpenPrivate )(const char*, Native::DBusError*){};
	void ( *connectionClose )( Native::DBusConnection* ){};
	void ( *connectionUnref )( Native::DBusConnection* ){};
	Uint32 ( *connectionReadWrite )( Native::DBusConnection*, int ){};
	Native::DBusDispatchStatus ( *connectionGetDispatchStatus )( Native::DBusConnection* ){};
	Native::DBusDispatchStatus ( *connectionDispatch )( Native::DBusConnection* ){};
	Uint32 ( *connectionGetUnixFd )( Native::DBusConnection*, int* ){};
	long ( *connectionGetOutgoingSize )( Native::DBusConnection* ){};
	Uint32 ( *connectionRegisterObjectPath )( Native::DBusConnection*, const char*,
											  const Native::DBusObjectPathVTable*, void* ){};
	Uint32 ( *connectionRegisterFallback )( Native::DBusConnection*, const char*,
											const Native::DBusObjectPathVTable*, void* ){};
	Uint32 ( *connectionSend )( Native::DBusConnection*, Native::DBusMessage*, Uint32* ){};
	Native::DBusMessage* ( *connectionSendWithReplyAndBlock )(Native::DBusConnection*,
															  Native::DBusMessage*, int,
															  Native::DBusError*){};
	Native::DBusMessage* ( *messageNewMethodCall )(const char*, const char*, const char*,
												   const char*){};
	Native::DBusMessage* ( *messageNewMethodReturn )(Native::DBusMessage*){};
	Native::DBusMessage* ( *messageNewError )(Native::DBusMessage*, const char*, const char*){};
	Native::DBusMessage* ( *messageNewSignal )(const char*, const char*, const char*){};
	const char* ( *messageGetPath )(Native::DBusMessage*){};
	const char* ( *messageGetInterface )(Native::DBusMessage*){};
	const char* ( *messageGetMember )(Native::DBusMessage*){};
	const char* ( *messageGetSender )(Native::DBusMessage*){};
	int ( *messageGetType )( Native::DBusMessage* ){};
	Uint32 ( *messageGetReplySerial )( Native::DBusMessage* ){};
	Uint32 ( *messageGetArgs )( Native::DBusMessage*, Native::DBusError*, int, ... ){};
	void ( *messageIterInitAppend )( Native::DBusMessage*, Native::DBusMessageIter* ){};
	Uint32 ( *messageIterInit )( Native::DBusMessage*, Native::DBusMessageIter* ){};
	void ( *messageIterGetBasic )( Native::DBusMessageIter*, void* ){};
	Uint32 ( *messageIterNext )( Native::DBusMessageIter* ){};
	int ( *messageIterGetArgType )( Native::DBusMessageIter* ){};
	void ( *messageIterRecurse )( Native::DBusMessageIter*, Native::DBusMessageIter* ){};
	Uint32 ( *messageIterAppendBasic )( Native::DBusMessageIter*, int, const void* ){};
	Uint32 ( *messageIterOpenContainer )( Native::DBusMessageIter*, int, const char*,
										  Native::DBusMessageIter* ){};
	Uint32 ( *messageIterCloseContainer )( Native::DBusMessageIter*, Native::DBusMessageIter* ){};
	void ( *messageUnref )( Native::DBusMessage* ){};
	void ( *errorInit )( Native::DBusError* ){};
	void ( *errorFree )( Native::DBusError* ){};
	Uint32 ( *errorIsSet )( const Native::DBusError* ){};
	Native::DBusConnection* ( *busGetPrivate )(int, Native::DBusError*){};
	void ( *connectionSetExitOnDisconnect )( Native::DBusConnection*, Uint32 ){};
	Uint32 ( *messageAppendArgs )( Native::DBusMessage*, int, ... ){};
	Uint32 ( *validateBusName )( const char*, Native::DBusError* ){};
	Uint32 ( *validatePath )( const char*, Native::DBusError* ){};
	Uint32 ( *validateUtf8 )( const char*, Native::DBusError* ){};

  private:
	friend class DbusTransportLibrary;
	DbusLibrary() = default;

	bool bind( void* handle );

	static void* loadHandle();

	static void* findSymbol( void* handle, const char* name );

	template <typename Function>
	static bool loadSymbol( void* handle, Function& function, const char* name ) {
		function = reinterpret_cast<Function>( findSymbol( handle, name ) );
		return function != nullptr;
	}
};

} // namespace EE::System::Dbus

#endif
