#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKENDATSPI_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKENDATSPI_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD

#include "accessibilitybackend.hpp"
#include <atomic>
#include <cstring>
#include <eepp/core/containers.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>

/**
 * AT-SPI (Linux/FreeBSD) accessibility backend.
 *
 * One AtSpiApplication is shared by every window of the process and owns a private connection
 * to the accessibility bus. libdbus is loaded at runtime, so eepp has no build-time D-Bus
 * dependency. The implementation is split by concern:
 *  - accessibilitybackendatspi.cpp:        connection, I/O thread, dispatch, client tracking and
 *                                          the AccessibilityBackend entry point.
 *  - accessibilitybackendatspievents.cpp:  manager events -> AT-SPI signals, text diffing.
 *  - accessibilitybackendatspitree.cpp:    object paths, roles, states and D-Bus marshalling.
 *  - accessibilitybackendatspimethods.cpp: incoming method calls, one handler per interface.
 */

namespace EE { namespace UI { namespace AtSpi {

constexpr int DBusMessageTypeMethodReturn = 2;
constexpr int DBusMessageTypeError = 3;

struct DBusConnection;
struct DBusMessage;
struct DBusError;

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

constexpr DBusDispatchStatus DBusDispatchDataRemains = 0;

struct DBusObjectPathVTable {
	void ( *unregisterFunction )( DBusConnection*, void* );
	DBusMessageFunction messageFunction;
	void ( *padding1 )( void* );
	void ( *padding2 )( void* );
	void ( *padding3 )( void* );
	void ( *padding4 )( void* );
};

class DBusLibrary {
  public:
	using BusGet = DBusConnection* ( * )( int, DBusError* );
	using BusRegister = int ( * )( DBusConnection*, DBusError* );
	using BusGetUniqueName = const char* ( * )( DBusConnection* );
	using BusAddMatch = void ( * )( DBusConnection*, const char*, DBusError* );
	using BusRemoveMatch = BusAddMatch;
	using ConnectionAddFilter = int ( * )( DBusConnection*, DBusMessageFunction, void*,
										   void ( * )( void* ) );
	using ConnectionOpenPrivate = DBusConnection* ( * )( const char*, DBusError* );
	using ConnectionClose = void ( * )( DBusConnection* );
	using ConnectionUnref = void ( * )( DBusConnection* );
	using ConnectionReadWrite = int ( * )( DBusConnection*, int );
	using ConnectionGetDispatchStatus = DBusDispatchStatus ( * )( DBusConnection* );
	using ConnectionDispatch = DBusDispatchStatus ( * )( DBusConnection* );
	using ConnectionGetUnixFd = int ( * )( DBusConnection*, int* );
	using ConnectionGetOutgoingSize = long ( * )( DBusConnection* );
	using ConnectionRegisterObjectPath = int ( * )( DBusConnection*, const char*,
													const DBusObjectPathVTable*, void* );
	using ConnectionRegisterFallback = ConnectionRegisterObjectPath;
	using ConnectionSend = int ( * )( DBusConnection*, DBusMessage*, Uint32* );
	using ConnectionSendWithReplyAndBlock = DBusMessage* ( * )( DBusConnection*, DBusMessage*, int,
																DBusError* );
	using MessageNewMethodCall = DBusMessage* ( * )( const char*, const char*, const char*,
													 const char* );
	using MessageGetArgs = int ( * )( DBusMessage*, DBusError*, int, ... );
	using MessageGetString = const char* ( * )( DBusMessage* );
	using MessageGetType = int ( * )( DBusMessage* );
	using MessageGetReplySerial = Uint32 ( * )( DBusMessage* );
	using MessageNewMethodReturn = DBusMessage* ( * )( DBusMessage* );
	using MessageNewError = DBusMessage* ( * )( DBusMessage*, const char*, const char* );
	using MessageNewSignal = DBusMessage* ( * )( const char*, const char*, const char* );
	using MessageIterInitAppend = void ( * )( DBusMessage*, DBusMessageIter* );
	using MessageIterInit = int ( * )( DBusMessage*, DBusMessageIter* );
	using MessageIterGetBasic = void ( * )( DBusMessageIter*, void* );
	using MessageIterNext = int ( * )( DBusMessageIter* );
	using MessageIterGetArgType = int ( * )( DBusMessageIter* );
	using MessageIterRecurse = void ( * )( DBusMessageIter*, DBusMessageIter* );
	using MessageIterAppendBasic = int ( * )( DBusMessageIter*, int, const void* );
	using MessageIterOpenContainer = int ( * )( DBusMessageIter*, int, const char*,
												DBusMessageIter* );
	using MessageIterCloseContainer = int ( * )( DBusMessageIter*, DBusMessageIter* );
	using MessageUnref = void ( * )( DBusMessage* );

	~DBusLibrary() {
		if ( mHandle )
			System::Sys::unloadObject( mHandle );
	}

	bool load() {
		mHandle = System::Sys::loadObject( "libdbus-1.so.3" );
		if ( !mHandle )
			mHandle = System::Sys::loadObject( "libdbus-1.so" );
		return loadSymbol( busGet, "dbus_bus_get" ) &&
			   loadSymbol( busRegister, "dbus_bus_register" ) &&
			   loadSymbol( busGetUniqueName, "dbus_bus_get_unique_name" ) &&
			   loadSymbol( busAddMatch, "dbus_bus_add_match" ) &&
			   loadSymbol( busRemoveMatch, "dbus_bus_remove_match" ) &&
			   loadSymbol( connectionAddFilter, "dbus_connection_add_filter" ) &&
			   loadSymbol( connectionOpenPrivate, "dbus_connection_open_private" ) &&
			   loadSymbol( connectionClose, "dbus_connection_close" ) &&
			   loadSymbol( connectionUnref, "dbus_connection_unref" ) &&
			   loadSymbol( connectionReadWrite, "dbus_connection_read_write" ) &&
			   loadSymbol( connectionGetDispatchStatus, "dbus_connection_get_dispatch_status" ) &&
			   loadSymbol( connectionDispatch, "dbus_connection_dispatch" ) &&
			   loadSymbol( connectionGetUnixFd, "dbus_connection_get_unix_fd" ) &&
			   loadSymbol( connectionGetOutgoingSize, "dbus_connection_get_outgoing_size" ) &&
			   loadSymbol( connectionRegisterObjectPath, "dbus_connection_register_object_path" ) &&
			   loadSymbol( connectionRegisterFallback, "dbus_connection_register_fallback" ) &&
			   loadSymbol( connectionSend, "dbus_connection_send" ) &&
			   loadSymbol( connectionSendWithReplyAndBlock,
						   "dbus_connection_send_with_reply_and_block" ) &&
			   loadSymbol( messageNewMethodCall, "dbus_message_new_method_call" ) &&
			   loadSymbol( messageNewMethodReturn, "dbus_message_new_method_return" ) &&
			   loadSymbol( messageNewError, "dbus_message_new_error" ) &&
			   loadSymbol( messageNewSignal, "dbus_message_new_signal" ) &&
			   loadSymbol( messageGetPath, "dbus_message_get_path" ) &&
			   loadSymbol( messageGetInterface, "dbus_message_get_interface" ) &&
			   loadSymbol( messageGetMember, "dbus_message_get_member" ) &&
			   loadSymbol( messageGetType, "dbus_message_get_type" ) &&
			   loadSymbol( messageGetReplySerial, "dbus_message_get_reply_serial" ) &&
			   loadSymbol( messageGetSender, "dbus_message_get_sender" ) &&
			   loadSymbol( messageGetArgs, "dbus_message_get_args" ) &&
			   loadSymbol( messageIterInitAppend, "dbus_message_iter_init_append" ) &&
			   loadSymbol( messageIterInit, "dbus_message_iter_init" ) &&
			   loadSymbol( messageIterGetBasic, "dbus_message_iter_get_basic" ) &&
			   loadSymbol( messageIterNext, "dbus_message_iter_next" ) &&
			   loadSymbol( messageIterGetArgType, "dbus_message_iter_get_arg_type" ) &&
			   loadSymbol( messageIterRecurse, "dbus_message_iter_recurse" ) &&
			   loadSymbol( messageIterAppendBasic, "dbus_message_iter_append_basic" ) &&
			   loadSymbol( messageIterOpenContainer, "dbus_message_iter_open_container" ) &&
			   loadSymbol( messageIterCloseContainer, "dbus_message_iter_close_container" ) &&
			   loadSymbol( messageUnref, "dbus_message_unref" );
	}

	BusGet busGet{};
	BusRegister busRegister{};
	BusGetUniqueName busGetUniqueName{};
	BusAddMatch busAddMatch{};
	BusRemoveMatch busRemoveMatch{};
	ConnectionAddFilter connectionAddFilter{};
	ConnectionOpenPrivate connectionOpenPrivate{};
	ConnectionClose connectionClose{};
	ConnectionUnref connectionUnref{};
	ConnectionReadWrite connectionReadWrite{};
	ConnectionGetDispatchStatus connectionGetDispatchStatus{};
	ConnectionDispatch connectionDispatch{};
	ConnectionGetUnixFd connectionGetUnixFd{};
	ConnectionGetOutgoingSize connectionGetOutgoingSize{};
	ConnectionRegisterObjectPath connectionRegisterObjectPath{};
	ConnectionRegisterFallback connectionRegisterFallback{};
	ConnectionSend connectionSend{};
	ConnectionSendWithReplyAndBlock connectionSendWithReplyAndBlock{};
	MessageNewMethodCall messageNewMethodCall{};
	MessageNewMethodReturn messageNewMethodReturn{};
	MessageNewError messageNewError{};
	MessageNewSignal messageNewSignal{};
	MessageGetString messageGetPath{};
	MessageGetString messageGetInterface{};
	MessageGetString messageGetMember{};
	MessageGetString messageGetSender{};
	MessageGetType messageGetType{};
	MessageGetReplySerial messageGetReplySerial{};
	MessageGetArgs messageGetArgs{};
	MessageIterInitAppend messageIterInitAppend{};
	MessageIterInit messageIterInit{};
	MessageIterGetBasic messageIterGetBasic{};
	MessageIterNext messageIterNext{};
	MessageIterGetArgType messageIterGetArgType{};
	MessageIterRecurse messageIterRecurse{};
	MessageIterAppendBasic messageIterAppendBasic{};
	MessageIterOpenContainer messageIterOpenContainer{};
	MessageIterCloseContainer messageIterCloseContainer{};
	MessageUnref messageUnref{};

  private:
	void* mHandle{};

	template <typename T> bool loadSymbol( T& symbol, const char* name ) {
		if ( !mHandle )
			return false;
		symbol = reinterpret_cast<T>( System::Sys::loadFunction( mHandle, name ) );
		return symbol != nullptr;
	}
};

class AtSpiApplication final : public std::enable_shared_from_this<AtSpiApplication> {
  public:
	explicit AtSpiApplication( AccessibilityManager& manager );

	void initialize();

	~AtSpiApplication();

	Uint64 registerManager( AccessibilityManager& manager );

	void unregisterManager( AccessibilityManager& manager );

	bool isAvailable() const {
		return mInitialized.load( std::memory_order_acquire ) && mConnection != nullptr;
	}

	bool isInitialized() const { return mInitialized.load( std::memory_order_acquire ); }

	bool hasActiveClients() const { return mHasActiveClients.load( std::memory_order_acquire ); }

	void update();

	void onEvent( AccessibilityManager& manager, const AccessibilityPendingEvent& event );

	bool onTextChanged( AccessibilityManager& manager, AccessibilityNodeRef ref,
						const AccessibilityTextChange& change );

	void announce( AccessibilityManager& manager, const String& message,
				   AccessibilityLive priority );

  private:
	static constexpr const char* RootPath = "/org/a11y/atspi/accessible/root";
	static constexpr const char* CachePath = "/org/a11y/atspi/cache";
	static constexpr const char* NodePathPrefix = "/org/eepp/a11y/";

	static constexpr AccessibilityNodeRef ApplicationRef{ 0, 1 };

	class ScopedManager {
	  public:
		ScopedManager( AtSpiApplication& application, AccessibilityManager* manager ) :
			mApplication( application ), mPrevious( application.mManager ) {
			application.mManager = manager;
		}

		~ScopedManager() { mApplication.mManager = mPrevious; }

	  private:
		AtSpiApplication& mApplication;
		AccessibilityManager* mPrevious;
	};

	AccessibilityManager* mManager{};
	std::atomic<AccessibilityManager*> mPrimaryManager{};
	String mName;
	std::vector<std::pair<Uint64, AccessibilityManager*>> mManagers;
	std::vector<AccessibilityManager*> mPendingWindowAdds;
	Uint64 mNextManagerId{ 1 };
	DBusLibrary mDBus;
	DBusConnection* mConnection{};
	int mConnectionFd{ -1 };
	int mWakePipe[2]{ -1, -1 };
	std::atomic<bool> mRunning{};
	std::atomic<bool> mDispatchRequested{};
	std::atomic<bool> mHasActiveClients{};
	std::atomic<bool> mInitialized{};
	std::thread mIOThread;
	std::string mBusName;
	std::string mRegistryBusName{ "org.a11y.atspi.Registry" };
	std::string mRegistryPath{ RootPath };
	Int32 mApplicationId{};
	bool mOutgoingPending{};
	/** What a client last saw of a text: its caret and selection, to report their changes, and
	 * its length as of the last change event, to describe a whole-text replacement. Queries never
	 * update the length. No contents are kept: edits arrive as exact change records. */
	struct TextState {
		AccessibilityTextInfo text;
		Int32 length{ -1 };
	};
	UnorderedMap<std::string, TextState> mTextStates;
	UnorderedSet<std::string> mClients;
	/** NameHasOwner checks sent when a client was first seen, by reply serial. */
	UnorderedMap<Uint32, std::string> mClientChecks;

	struct PendingText {
		AccessibilityManager* manager;
		AccessibilityNodeRef ref;
		bool wholeText;
	};
	std::vector<PendingText> mPendingTexts;

	// Connection, dispatch and client tracking: accessibilitybackendatspi.cpp

	void initializationFinished() { mInitialized.store( true, std::memory_order_release ); }

	void waitForMessages();

	static DBusHandlerResult handleMessage( DBusConnection*, DBusMessage* message, void* userData );

	void activate( DBusMessage* message );

	/** Watches only this client's disconnection; see the definition for the ordering argument. */
	void watchClient( const char* name );

	void clientGone( const char* name );

	static DBusHandlerResult clientDisconnected( DBusConnection*, DBusMessage* message,
												 void* userData );

	void send( DBusMessage* reply );

	void sendBasic( DBusMessage* request, int type, const void* value );

	void sendError( DBusMessage* request, const char* name, const char* message ) {
		send( mDBus.messageNewError( request, name, message ) );
	}

	void registerApplication();

	// Manager events and text change notifications: accessibilitybackendatspievents.cpp

	/** Starts tracking a text the client is looking at; returns its state. */
	TextState& rememberText( AccessibilityNodeRef ref, const AccessibilityTextInfo& text );

	void queueText( AccessibilityManager& manager, AccessibilityNodeRef ref, bool wholeText );

	void flushPendingTexts();

	void sendObjectEvent( AccessibilityNodeRef ref, const char* signal, const char* detail,
						  Int32 offset, Int32 length = 0, const String& text = {} );

	void sendTextSelection( AccessibilityNodeRef ref, const AccessibilityTextInfo& text );

	/** Reports a whole-text replacement: delete everything the client knew, insert the rest. */
	void sendTextReplaced( AccessibilityNodeRef ref );

	/** Reports the replacement of `previous` by `current` as its minimal delete and insert. */
	void sendTextDiff( AccessibilityNodeRef ref, const String& previous, const String& current );

	void sendWindowChanged( AccessibilityManager& manager, bool added, Int32 index );

	// Object paths, tree navigation, roles, states and marshalling:
	// accessibilitybackendatspitree.cpp

	AccessibilityNodeRef refFromPath( const char* path );

	Uint64 currentManagerId() const;

	bool isSceneRoot( AccessibilityNodeRef ref ) const {
		return mManager && ref == mManager->getRoot();
	}

	bool isValid( AccessibilityNodeRef ref ) const {
		return ref == ApplicationRef || ( mManager && mManager->isValid( ref ) );
	}

	AccessibilityNodeInfo getNodeInfo( AccessibilityNodeRef ref, bool includeValue = true ) const;

	AccessibilityNodeRef getParent( AccessibilityNodeRef ref );

	size_t getChildCount( AccessibilityNodeRef ref );

	AccessibilityNodeRef getChild( AccessibilityNodeRef ref, size_t index );

	std::string pathFromRef( AccessibilityNodeRef ref );

	Uint32 role( AccessibilityRole role ) const;

	const char* roleName( AccessibilityRole role ) const;

	AccessibilityAction actionAt( AccessibilityActions actions, Int32 index ) const;

	AccessibilityActions nativeActions( AccessibilityActions actions ) const;

	const char* actionName( AccessibilityAction action ) const;

	/** The Action key binding ("mnemonic;sequence;accelerator") of the activating action. */
	std::string actionKeyBinding( const AccessibilityNodeInfo& info,
								  AccessibilityAction action ) const;

	void appendBasic( DBusMessageIter& iter, int type, const void* value ) {
		mDBus.messageIterAppendBasic( &iter, type, value );
	}

	bool readPropertySet( DBusMessage* request, const char*& interface, const char*& property,
						  DBusMessageIter& variant );

	void appendRef( DBusMessageIter& iter, AccessibilityNodeRef ref );

	void appendApplicationRef( DBusMessageIter& iter ) { appendRef( iter, ApplicationRef ); }

	void appendEventProperties( DBusMessageIter& iter );

	void appendDesktopRef( DBusMessageIter& iter );

	void appendInterfaces( DBusMessageIter& iter, AccessibilityNodeRef ref,
						   const AccessibilityNodeInfo& info );

	void appendStates( DBusMessageIter& iter, const AccessibilityNodeInfo& info );

	Int32 indexInParent( AccessibilityNodeRef ref );

	Math::Rectf boundsForCoordinateType( AccessibilityNodeRef ref,
										 const AccessibilityNodeInfo& info, Uint32 coordinateType );

	// Incoming method calls, one handler per AT-SPI interface: accessibilitybackendatspimethods.cpp

	DBusHandlerResult handleMessage( DBusMessage* request );

	/** Serves the AT-SPI Cache object. eepp exposes no cached items; clients query lazily. */
	DBusHandlerResult handleCache( DBusMessage* request, const char* interface,
								   const char* member );

	DBusHandlerResult handleAccessible( DBusMessage* request, AccessibilityNodeRef ref,
										const char* member );

	DBusHandlerResult handleApplication( DBusMessage* request, AccessibilityNodeRef ref,
										 const char* member );

	DBusHandlerResult handleComponent( DBusMessage* request, AccessibilityNodeRef ref,
									   const char* member );

	DBusHandlerResult handleAction( DBusMessage* request, AccessibilityNodeRef ref,
									const char* member );

	DBusHandlerResult handleText( DBusMessage* request, AccessibilityNodeRef ref,
								  const char* member );

	DBusHandlerResult handleEditableText( DBusMessage* request, AccessibilityNodeRef ref,
										  const char* member );

	DBusHandlerResult handlePropertiesGet( DBusMessage* request, AccessibilityNodeRef ref );

	DBusHandlerResult handlePropertiesSet( DBusMessage* request, AccessibilityNodeRef ref );
};

}}} // namespace EE::UI::AtSpi

#endif

#endif
