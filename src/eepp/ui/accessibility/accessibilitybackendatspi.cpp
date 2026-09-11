#include "accessibilitybackend.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <eepp/system/sys.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <memory>
#include <thread>

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD

#include <poll.h>
#include <unistd.h>

namespace EE { namespace UI {
namespace {

constexpr int DBusMessageTypeError = 3;

bool hasState( AccessibilityState states, AccessibilityState state ) {
	return static_cast<Uint64>( states ) & static_cast<Uint64>( state );
}

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
	using BusGet = DBusConnection* (*)( int, DBusError* );
	using BusRegister = int ( * )( DBusConnection*, DBusError* );
	using BusGetUniqueName = const char* (*)( DBusConnection* );
	using ConnectionOpenPrivate = DBusConnection* (*)( const char*, DBusError* );
	using ConnectionClose = void ( * )( DBusConnection* );
	using ConnectionUnref = void ( * )( DBusConnection* );
	using ConnectionReadWrite = int ( * )( DBusConnection*, int );
	using ConnectionGetDispatchStatus = DBusDispatchStatus ( * )( DBusConnection* );
	using ConnectionDispatch = DBusDispatchStatus ( * )( DBusConnection* );
	using ConnectionGetUnixFd = int ( * )( DBusConnection*, int* );
	using ConnectionRegisterObjectPath = int ( * )( DBusConnection*, const char*,
													const DBusObjectPathVTable*, void* );
	using ConnectionRegisterFallback = ConnectionRegisterObjectPath;
	using ConnectionSend = int ( * )( DBusConnection*, DBusMessage*, Uint32* );
	using ConnectionSendWithReplyAndBlock = DBusMessage* (*)( DBusConnection*, DBusMessage*, int,
															  DBusError* );
	using MessageNewMethodCall = DBusMessage* (*)( const char*, const char*, const char*,
												   const char* );
	using MessageGetArgs = int ( * )( DBusMessage*, DBusError*, int, ... );
	using MessageGetString = const char* (*)( DBusMessage* );
	using MessageGetType = int ( * )( DBusMessage* );
	using MessageNewMethodReturn = DBusMessage* (*)( DBusMessage* );
	using MessageNewError = DBusMessage* (*)( DBusMessage*, const char*, const char* );
	using MessageNewSignal = DBusMessage* (*)( const char*, const char*, const char* );
	using MessageIterInitAppend = void ( * )( DBusMessage*, DBusMessageIter* );
	using MessageIterInit = int ( * )( DBusMessage*, DBusMessageIter* );
	using MessageIterGetBasic = void ( * )( DBusMessageIter*, void* );
	using MessageIterNext = int ( * )( DBusMessageIter* );
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
			   loadSymbol( connectionOpenPrivate, "dbus_connection_open_private" ) &&
			   loadSymbol( connectionClose, "dbus_connection_close" ) &&
			   loadSymbol( connectionUnref, "dbus_connection_unref" ) &&
			   loadSymbol( connectionReadWrite, "dbus_connection_read_write" ) &&
			   loadSymbol( connectionGetDispatchStatus, "dbus_connection_get_dispatch_status" ) &&
			   loadSymbol( connectionDispatch, "dbus_connection_dispatch" ) &&
			   loadSymbol( connectionGetUnixFd, "dbus_connection_get_unix_fd" ) &&
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
			   loadSymbol( messageGetArgs, "dbus_message_get_args" ) &&
			   loadSymbol( messageIterInitAppend, "dbus_message_iter_init_append" ) &&
			   loadSymbol( messageIterInit, "dbus_message_iter_init" ) &&
			   loadSymbol( messageIterGetBasic, "dbus_message_iter_get_basic" ) &&
			   loadSymbol( messageIterNext, "dbus_message_iter_next" ) &&
			   loadSymbol( messageIterRecurse, "dbus_message_iter_recurse" ) &&
			   loadSymbol( messageIterAppendBasic, "dbus_message_iter_append_basic" ) &&
			   loadSymbol( messageIterOpenContainer, "dbus_message_iter_open_container" ) &&
			   loadSymbol( messageIterCloseContainer, "dbus_message_iter_close_container" ) &&
			   loadSymbol( messageUnref, "dbus_message_unref" );
	}

	BusGet busGet{};
	BusRegister busRegister{};
	BusGetUniqueName busGetUniqueName{};
	ConnectionOpenPrivate connectionOpenPrivate{};
	ConnectionClose connectionClose{};
	ConnectionUnref connectionUnref{};
	ConnectionReadWrite connectionReadWrite{};
	ConnectionGetDispatchStatus connectionGetDispatchStatus{};
	ConnectionDispatch connectionDispatch{};
	ConnectionGetUnixFd connectionGetUnixFd{};
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
	MessageGetType messageGetType{};
	MessageGetArgs messageGetArgs{};
	MessageIterInitAppend messageIterInitAppend{};
	MessageIterInit messageIterInit{};
	MessageIterGetBasic messageIterGetBasic{};
	MessageIterNext messageIterNext{};
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
	explicit AtSpiApplication( AccessibilityManager& manager ) :
		mManager( &manager ), mPrimaryManager( &manager ) {
		registerManager( manager );
		if ( manager.getSceneNode() && manager.getSceneNode()->getWindow() )
			mName = String::fromUtf8( manager.getSceneNode()->getWindow()->getTitle() );
		if ( !mDBus.load() )
			return;
		DBusConnection* sessionBus = mDBus.busGet( 0, nullptr );
		if ( !sessionBus )
			return;
		DBusMessage* request = mDBus.messageNewMethodCall( "org.a11y.Bus", "/org/a11y/bus",
														   "org.a11y.Bus", "GetAddress" );
		if ( !request )
			return;
		DBusMessage* reply =
			mDBus.connectionSendWithReplyAndBlock( sessionBus, request, 1000, nullptr );
		mDBus.messageUnref( request );
		if ( !reply )
			return;
		const char* address = nullptr;
		const bool gotAddress = mDBus.messageGetArgs( reply, nullptr, 's', &address, 0 ) != 0;
		if ( gotAddress && address ) {
			mConnection = mDBus.connectionOpenPrivate( address, nullptr );
			if ( mConnection && !mDBus.busRegister( mConnection, nullptr ) ) {
				mDBus.connectionClose( mConnection );
				mDBus.connectionUnref( mConnection );
				mConnection = nullptr;
			}
		}
		mDBus.messageUnref( reply );
	}

	~AtSpiApplication() {
		if ( mConnection ) {
			mRunning.store( false, std::memory_order_release );
			if ( mIOThread.joinable() ) {
				const char stop = 1;
				write( mWakePipe[1], &stop, sizeof( stop ) );
				mIOThread.join();
				close( mWakePipe[0] );
				close( mWakePipe[1] );
			}
			mDBus.connectionClose( mConnection );
			mDBus.connectionUnref( mConnection );
		}
	}

	Uint64 registerManager( AccessibilityManager& manager ) {
		for ( const auto& entry : mManagers ) {
			if ( entry.second == &manager )
				return entry.first;
		}
		const Uint64 id = mNextManagerId++;
		mManagers.emplace_back( id, &manager );
		if ( !mWakeInput.load( std::memory_order_acquire ) && manager.getSceneNode() &&
			 manager.getSceneNode()->getWindow() ) {
			mWakeInput.store( manager.getSceneNode()->getWindow()->getInput(),
							  std::memory_order_release );
		}
		if ( mConnection && hasActiveClients() )
			mPendingWindowAdds.push_back( &manager );
		return id;
	}

	void unregisterManager( AccessibilityManager& manager ) {
		mPendingWindowAdds.erase(
			std::remove( mPendingWindowAdds.begin(), mPendingWindowAdds.end(), &manager ),
			mPendingWindowAdds.end() );
		for ( auto it = mManagers.begin(); it != mManagers.end(); ++it ) {
			if ( it->second != &manager )
				continue;
			const Int32 index = static_cast<Int32>( it - mManagers.begin() );
			if ( mConnection && hasActiveClients() )
				sendWindowChanged( manager, false, index );
			mManagers.erase( it );
			break;
		}
		if ( mManager == &manager )
			mManager = mManagers.empty() ? nullptr : mManagers.begin()->second;
		if ( mPrimaryManager.load( std::memory_order_acquire ) == &manager ) {
			auto* primaryManager = mManagers.empty() ? nullptr : mManagers.begin()->second;
			mPrimaryManager.store( primaryManager, std::memory_order_release );
			mWakeInput.store( primaryManager && primaryManager->getSceneNode() &&
									  primaryManager->getSceneNode()->getWindow()
								  ? primaryManager->getSceneNode()->getWindow()->getInput()
								  : nullptr,
							  std::memory_order_release );
		}
	}

	bool isAvailable() const { return mConnection != nullptr; }

	bool hasActiveClients() const { return mHasActiveClients.load( std::memory_order_acquire ); }

	void update() {
		if ( !mConnection )
			return;
		if ( !mRegistered ) {
			mRegistered = true;
			registerApplication();
			if ( mConnection && mDBus.connectionGetUnixFd( mConnection, &mConnectionFd ) &&
				 pipe( mWakePipe ) == 0 ) {
				mRunning.store( true, std::memory_order_release );
				mIOThread = std::thread( &AtSpiApplication::waitForMessages, this );
			}
			// Socket.Embed emits the registry's application-add event before replying. A client can
			// query us immediately, and the blocking Embed call may already have moved that query
			// into libdbus's internal dispatch queue before the I/O thread starts polling the
			// drained socket.
			if ( mConnection )
				mDispatchRequested.store( true, std::memory_order_release );
			return;
		}
		if ( !mPendingWindowAdds.empty() ) {
			for ( auto* manager : mPendingWindowAdds ) {
				for ( size_t i = 0; i < mManagers.size(); ++i ) {
					if ( mManagers[i].second == manager ) {
						sendWindowChanged( *manager, true, static_cast<Int32>( i ) );
						break;
					}
				}
			}
			mPendingWindowAdds.clear();
		}
		if ( !mDispatchRequested.exchange( false, std::memory_order_acq_rel ) )
			return;
		mDBus.connectionReadWrite( mConnection, 0 );
		constexpr size_t MaxMessagesPerUpdate = 256;
		for ( size_t i = 0;
			  i < MaxMessagesPerUpdate &&
			  mDBus.connectionGetDispatchStatus( mConnection ) == DBusDispatchDataRemains;
			  ++i )
			mDBus.connectionDispatch( mConnection );
		mDBus.connectionReadWrite( mConnection, 0 );
		// connectionReadWrite() can move multiple requests from the socket into libdbus. Once the
		// socket is drained, poll() will not wake us for requests that remain in libdbus's internal
		// dispatch queue. Preserve the bounded per-update work without stranding that backlog.
		if ( mDBus.connectionGetDispatchStatus( mConnection ) == DBusDispatchDataRemains )
			mDispatchRequested.store( true, std::memory_order_release );
	}

	void onEvent( AccessibilityManager& manager, const AccessibilityPendingEvent& event ) {
		if ( !mConnection )
			return;
		ScopedManager scopedManager( *this, &manager );
		if ( event.type == AccessibilityEvent::Destroyed && !event.related.isValid() )
			return;
		AccessibilityNodeInfo info;
		if ( event.type == AccessibilityEvent::FocusChanged ||
			 event.type == AccessibilityEvent::SelectionChanged ||
			 event.type == AccessibilityEvent::StateChanged ||
			 event.type == AccessibilityEvent::EnabledChanged ||
			 event.type == AccessibilityEvent::VisibilityChanged )
			info = manager.getNodeInfo( event.ref, false );
		const char* signal = "PropertyChange";
		const char* detail = "accessible-state";
		if ( event.type == AccessibilityEvent::FocusChanged ) {
			signal = "StateChanged";
			detail = "focused";
		} else if ( event.type == AccessibilityEvent::SelectionChanged ) {
			signal = "StateChanged";
			detail = "selected";
		} else if ( event.type == AccessibilityEvent::NameChanged ) {
			detail = "accessible-name";
		} else if ( event.type == AccessibilityEvent::DescriptionChanged ) {
			detail = "accessible-description";
		} else if ( event.type == AccessibilityEvent::ValueChanged ) {
			detail = "accessible-value";
		} else if ( event.type == AccessibilityEvent::StateChanged ) {
			signal = "StateChanged";
			if ( info.role == AccessibilityRole::CheckBox ||
				 info.role == AccessibilityRole::RadioButton ||
				 info.role == AccessibilityRole::CheckMenuItem ||
				 info.role == AccessibilityRole::RadioMenuItem )
				detail = "checked";
			else if ( info.role == AccessibilityRole::ComboBox )
				detail = "expanded";
			else
				return;
		} else if ( event.type == AccessibilityEvent::EnabledChanged ) {
			signal = "StateChanged";
			detail = "enabled";
		} else if ( event.type == AccessibilityEvent::VisibilityChanged ) {
			signal = "StateChanged";
			detail = "visible";
		} else if ( event.type == AccessibilityEvent::BoundsChanged ) {
			signal = "BoundsChanged";
			detail = "";
		} else if ( event.type == AccessibilityEvent::ChildrenChanged ||
					event.type == AccessibilityEvent::ModelChanged ) {
			signal = "ModelChanged";
			detail = "";
		} else if ( event.type == AccessibilityEvent::Created ||
					event.type == AccessibilityEvent::Destroyed ) {
			signal = "ChildrenChanged";
			detail = event.type == AccessibilityEvent::Destroyed ? "remove" : "add";
		}
		std::string pathStorage = pathFromRef( event.ref );
		DBusMessage* message =
			mDBus.messageNewSignal( pathStorage.c_str(), "org.a11y.atspi.Event.Object", signal );
		if ( !message )
			return;
		DBusMessageIter iter;
		DBusMessageIter variant;
		mDBus.messageIterInitAppend( message, &iter );
		appendBasic( iter, 's', &detail );
		Int32 detail1 = 0;
		if ( event.type == AccessibilityEvent::FocusChanged )
			detail1 = hasState( info.states, AccessibilityState::Focused );
		else if ( event.type == AccessibilityEvent::SelectionChanged )
			detail1 = hasState( info.states, AccessibilityState::Selected );
		else if ( event.type == AccessibilityEvent::StateChanged &&
				  std::strcmp( detail, "checked" ) == 0 )
			detail1 = hasState( info.states, AccessibilityState::Checked );
		else if ( event.type == AccessibilityEvent::StateChanged &&
				  std::strcmp( detail, "expanded" ) == 0 )
			detail1 = hasState( info.states, AccessibilityState::Expanded );
		else if ( event.type == AccessibilityEvent::EnabledChanged )
			detail1 = hasState( info.states, AccessibilityState::Enabled );
		else if ( event.type == AccessibilityEvent::VisibilityChanged )
			detail1 = hasState( info.states, AccessibilityState::Visible );
		else if ( event.type == AccessibilityEvent::Created ||
				  event.type == AccessibilityEvent::Destroyed )
			detail1 = event.index;
		Int32 detail2 = 0;
		appendBasic( iter, 'i', &detail1 );
		appendBasic( iter, 'i', &detail2 );
		bool childrenChanged = event.type == AccessibilityEvent::Created ||
							   event.type == AccessibilityEvent::Destroyed;
		if ( childrenChanged ) {
			mDBus.messageIterOpenContainer( &iter, 'v', "(so)", &variant );
			appendRef( variant, event.related );
		} else {
			const char* empty = "";
			mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
			appendBasic( variant, 's', &empty );
		}
		mDBus.messageIterCloseContainer( &iter, &variant );
		appendEventProperties( iter );
		send( message );
	}

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
	std::atomic<Window::Input*> mWakeInput{};
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
	std::thread mIOThread;
	std::string mBusName;
	std::string mRegistryBusName{ "org.a11y.atspi.Registry" };
	std::string mRegistryPath{ RootPath };
	Int32 mApplicationId{};
	bool mRegistered{};

	void waitForMessages() {
		while ( mRunning.load( std::memory_order_acquire ) ) {
			pollfd descriptors[2]{ { mConnectionFd, POLLIN, 0 }, { mWakePipe[0], POLLIN, 0 } };
			if ( poll( descriptors, 2, -1 ) <= 0 || descriptors[1].revents & POLLIN ||
				 descriptors[0].revents & ( POLLERR | POLLHUP | POLLNVAL ) )
				break;
			if ( !( descriptors[0].revents & POLLIN ) ||
				 mDispatchRequested.exchange( true, std::memory_order_acq_rel ) )
				continue;
			// UIApplication waits on its first live window. The current query manager can point to
			// a secondary window, whose input wake-up would not release that wait and would
			// serialize accessibility requests behind the frame timeout.
			auto* input = mWakeInput.load( std::memory_order_acquire );
			if ( input )
				input->wakeUp();
		}
	}

	static DBusHandlerResult handleMessage( DBusConnection*, DBusMessage* message,
											void* userData ) {
		auto backend = static_cast<AtSpiApplication*>( userData );
		const char* interface = backend->mDBus.messageGetInterface( message );
		const char* member = backend->mDBus.messageGetMember( message );
		const bool registryPropertySet =
			interface && member &&
			std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
			std::strcmp( member, "Set" ) == 0;
		if ( !registryPropertySet )
			backend->activate();
		if ( backend->handleMessage( message ) != 0 ) {
			backend->sendError( message, "org.freedesktop.DBus.Error.UnknownMethod",
								"Unsupported accessibility method or property" );
		}
		return 0;
	}

	void activate() { mHasActiveClients.store( true, std::memory_order_release ); }

	AccessibilityNodeRef refFromPath( const char* path ) {
		if ( !path )
			return {};
		if ( std::strcmp( path, RootPath ) == 0 ) {
			mManager = mPrimaryManager.load( std::memory_order_acquire );
			return ApplicationRef;
		}
		if ( std::strncmp( path, NodePathPrefix, std::strlen( NodePathPrefix ) ) != 0 )
			return {};
		char* end = nullptr;
		const char* encoded = path + std::strlen( NodePathPrefix );
		Uint64 managerId = std::strtoull( encoded, &end, 10 );
		if ( !end || *end != '/' )
			return {};
		auto manager =
			std::find_if( mManagers.begin(), mManagers.end(),
						  [managerId]( const auto& entry ) { return entry.first == managerId; } );
		if ( manager == mManagers.end() )
			return {};
		char* sourceEnd = nullptr;
		Uint64 source = std::strtoull( end + 1, &sourceEnd, 10 );
		if ( !sourceEnd || *sourceEnd != '/' )
			return {};
		char* idEnd = nullptr;
		Uint64 id = std::strtoull( sourceEnd + 1, &idEnd, 10 );
		if ( !idEnd || *idEnd != '\0' )
			return {};
		mManager = manager->second;
		return { static_cast<AccessibilitySourceId>( source ), id };
	}

	Uint64 currentManagerId() const {
		for ( const auto& entry : mManagers ) {
			if ( entry.second == mManager )
				return entry.first;
		}
		return 0;
	}

	bool isSceneRoot( AccessibilityNodeRef ref ) const {
		return mManager && ref == mManager->getRoot();
	}

	bool isValid( AccessibilityNodeRef ref ) const {
		return ref == ApplicationRef || ( mManager && mManager->isValid( ref ) );
	}

	AccessibilityNodeInfo getNodeInfo( AccessibilityNodeRef ref, bool includeValue = true ) const {
		if ( ref != ApplicationRef ) {
			auto info =
				mManager ? mManager->getNodeInfo( ref, includeValue ) : AccessibilityNodeInfo{};
			if ( isSceneRoot( ref ) )
				info.role = AccessibilityRole::Window;
			return info;
		}
		AccessibilityNodeInfo info;
		info.role = AccessibilityRole::Application;
		info.states =
			AccessibilityState::Enabled | AccessibilityState::Visible | AccessibilityState::Showing;
		info.name = mName;
		return info;
	}

	AccessibilityNodeRef getParent( AccessibilityNodeRef ref ) {
		if ( ref == ApplicationRef )
			return {};
		if ( isSceneRoot( ref ) )
			return ApplicationRef;
		return mManager ? mManager->getParent( ref ) : AccessibilityNodeRef{};
	}

	size_t getChildCount( AccessibilityNodeRef ref ) {
		if ( ref == ApplicationRef )
			return mManagers.size();
		return mManager ? mManager->getChildCount( ref ) : 0;
	}

	AccessibilityNodeRef getChild( AccessibilityNodeRef ref, size_t index ) {
		return mManager && ref != ApplicationRef ? mManager->getChild( ref, index )
												 : AccessibilityNodeRef{};
	}

	std::string pathFromRef( AccessibilityNodeRef ref ) {
		if ( ref == ApplicationRef )
			return RootPath;
		const Uint64 managerId = currentManagerId();
		return std::string( NodePathPrefix ) + String::toString( managerId ) + "/" +
			   String::toString( ref.source ) + "/" + String::toString( ref.id );
	}

	Uint32 role( AccessibilityRole role ) const {
		switch ( role ) {
			case AccessibilityRole::Application:
				return 75;
			case AccessibilityRole::Window:
				return 69;
			case AccessibilityRole::Dialog:
				return 16;
			case AccessibilityRole::Button:
				return 43;
			case AccessibilityRole::CheckBox:
				return 7;
			case AccessibilityRole::RadioButton:
				return 44;
			case AccessibilityRole::Label:
			case AccessibilityRole::Text:
				return 29;
			case AccessibilityRole::TextBox:
				return 79;
			case AccessibilityRole::Image:
				return 27;
			case AccessibilityRole::ComboBox:
				return 11;
			case AccessibilityRole::Slider:
				return 51;
			case AccessibilityRole::SpinButton:
				return 52;
			case AccessibilityRole::ProgressBar:
				return 42;
			case AccessibilityRole::Tab:
				return 37;
			case AccessibilityRole::TabList:
				return 38;
			case AccessibilityRole::MenuBar:
				return 34;
			case AccessibilityRole::Menu:
				return 33;
			case AccessibilityRole::MenuItem:
				return 35;
			case AccessibilityRole::CheckMenuItem:
				return 8;
			case AccessibilityRole::RadioMenuItem:
				return 45;
			case AccessibilityRole::List:
				return 31;
			case AccessibilityRole::ListItem:
				return 32;
			case AccessibilityRole::Table:
				return 55;
			case AccessibilityRole::Row:
				return 90;
			case AccessibilityRole::Cell:
				return 56;
			case AccessibilityRole::Tree:
				return 65;
			case AccessibilityRole::TreeItem:
				return 91;
			default:
				return 67;
		}
	}

	const char* roleName( AccessibilityRole role ) const {
		switch ( role ) {
			case AccessibilityRole::Application:
				return "application";
			case AccessibilityRole::Window:
				return "window";
			case AccessibilityRole::Dialog:
				return "dialog";
			case AccessibilityRole::Button:
				return "push button";
			case AccessibilityRole::CheckBox:
				return "check box";
			case AccessibilityRole::RadioButton:
				return "radio button";
			case AccessibilityRole::Label:
				return "label";
			case AccessibilityRole::Text:
				return "text";
			case AccessibilityRole::TextBox:
				return "entry";
			case AccessibilityRole::Image:
				return "image";
			case AccessibilityRole::ComboBox:
				return "combo box";
			case AccessibilityRole::Slider:
				return "slider";
			case AccessibilityRole::SpinButton:
				return "spin button";
			case AccessibilityRole::ProgressBar:
				return "progress bar";
			case AccessibilityRole::Tab:
				return "page tab";
			case AccessibilityRole::TabList:
				return "page tab list";
			case AccessibilityRole::MenuBar:
				return "menu bar";
			case AccessibilityRole::Menu:
				return "menu";
			case AccessibilityRole::MenuItem:
				return "menu item";
			case AccessibilityRole::CheckMenuItem:
				return "check menu item";
			case AccessibilityRole::RadioMenuItem:
				return "radio menu item";
			case AccessibilityRole::List:
				return "list";
			case AccessibilityRole::ListItem:
				return "list item";
			case AccessibilityRole::Table:
				return "table";
			case AccessibilityRole::Row:
				return "table row";
			case AccessibilityRole::Cell:
				return "table cell";
			case AccessibilityRole::Tree:
				return "tree";
			case AccessibilityRole::TreeItem:
				return "tree item";
			default:
				return "unknown";
		}
	}

	AccessibilityAction actionAt( AccessibilityActions actions, Int32 index ) const {
		for ( Uint32 action = 0; action <= static_cast<Uint32>( AccessibilityAction::ScrollTo );
			  ++action ) {
			if ( actions & ( 1u << action ) ) {
				if ( index-- == 0 )
					return static_cast<AccessibilityAction>( action );
			}
		}
		return AccessibilityAction::Focus;
	}

	AccessibilityActions nativeActions( AccessibilityActions actions ) const {
		return actions & ~( accessibilityActionMask( AccessibilityAction::SetValue ) |
							accessibilityActionMask( AccessibilityAction::SetText ) );
	}

	const char* actionName( AccessibilityAction action ) const {
		switch ( action ) {
			case AccessibilityAction::Focus:
				return "focus";
			case AccessibilityAction::Press:
				return "click";
			case AccessibilityAction::Toggle:
				return "toggle";
			case AccessibilityAction::Select:
				return "select";
			case AccessibilityAction::Increment:
				return "increment";
			case AccessibilityAction::Decrement:
				return "decrement";
			case AccessibilityAction::SetValue:
				return "set value";
			case AccessibilityAction::SetText:
				return "set text";
			case AccessibilityAction::Expand:
				return "expand";
			case AccessibilityAction::Collapse:
				return "collapse";
			case AccessibilityAction::ScrollTo:
				return "scroll to";
		}
		return "";
	}

	void appendBasic( DBusMessageIter& iter, int type, const void* value ) {
		mDBus.messageIterAppendBasic( &iter, type, value );
	}

	void appendRef( DBusMessageIter& iter, AccessibilityNodeRef ref ) {
		DBusMessageIter structure;
		mDBus.messageIterOpenContainer( &iter, 'r', nullptr, &structure );
		const bool valid = ref == ApplicationRef || ref.isValid();
		const char* bus = valid ? mBusName.c_str() : "";
		std::string pathStorage = valid ? pathFromRef( ref ) : "/org/a11y/atspi/null";
		const char* path = pathStorage.c_str();
		appendBasic( structure, 's', &bus );
		appendBasic( structure, 'o', &path );
		mDBus.messageIterCloseContainer( &iter, &structure );
	}

	void appendApplicationRef( DBusMessageIter& iter ) { appendRef( iter, ApplicationRef ); }

	void appendEventProperties( DBusMessageIter& iter ) {
		DBusMessageIter array;
		mDBus.messageIterOpenContainer( &iter, 'a', "{sv}", &array );
		mDBus.messageIterCloseContainer( &iter, &array );
	}

	void appendDesktopRef( DBusMessageIter& iter ) {
		DBusMessageIter structure;
		mDBus.messageIterOpenContainer( &iter, 'r', nullptr, &structure );
		const char* bus = mRegistryBusName.c_str();
		const char* path = mRegistryPath.c_str();
		appendBasic( structure, 's', &bus );
		appendBasic( structure, 'o', &path );
		mDBus.messageIterCloseContainer( &iter, &structure );
	}

	void appendInterfaces( DBusMessageIter& iter, AccessibilityNodeRef ref,
						   const AccessibilityNodeInfo& info ) {
		DBusMessageIter array;
		mDBus.messageIterOpenContainer( &iter, 'a', "s", &array );
		const char* accessible = "org.a11y.atspi.Accessible";
		const char* component = "org.a11y.atspi.Component";
		appendBasic( array, 's', &accessible );
		appendBasic( array, 's', &component );
		if ( nativeActions( info.actions ) ) {
			const char* action = "org.a11y.atspi.Action";
			appendBasic( array, 's', &action );
		}
		if ( info.range.valid ) {
			const char* value = "org.a11y.atspi.Value";
			appendBasic( array, 's', &value );
		}
		if ( info.text.valid ) {
			const char* text = "org.a11y.atspi.Text";
			appendBasic( array, 's', &text );
			if ( hasState( info.states, AccessibilityState::Editable ) ) {
				const char* editableText = "org.a11y.atspi.EditableText";
				appendBasic( array, 's', &editableText );
			}
		}
		if ( ref == ApplicationRef ) {
			const char* application = "org.a11y.atspi.Application";
			appendBasic( array, 's', &application );
		}
		mDBus.messageIterCloseContainer( &iter, &array );
	}

	void appendStates( DBusMessageIter& iter, AccessibilityState states ) {
		Uint32 words[2]{};
		auto addState = [&words]( bool enabled, Uint32 state ) {
			if ( enabled )
				words[state / 32] |= 1u << ( state % 32 );
		};
		Uint64 stateBits = static_cast<Uint64>( states );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Active ), 1 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Checked ), 4 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Editable ), 7 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Enabled ), 8 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Expanded ), 9 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Focusable ), 11 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Focused ), 12 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Selected ), 23 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Showing ), 25 );
		addState( stateBits & static_cast<Uint64>( AccessibilityState::Visible ), 30 );
		DBusMessageIter array;
		mDBus.messageIterOpenContainer( &iter, 'a', "u", &array );
		appendBasic( array, 'u', &words[0] );
		appendBasic( array, 'u', &words[1] );
		mDBus.messageIterCloseContainer( &iter, &array );
	}

	Int32 indexInParent( AccessibilityNodeRef ref ) {
		if ( isSceneRoot( ref ) ) {
			for ( size_t i = 0; i < mManagers.size(); ++i ) {
				if ( mManagers[i].second == mManager )
					return static_cast<Int32>( i );
			}
			return -1;
		}
		auto parent = getParent( ref );
		if ( !parent.isValid() || !mManager )
			return -1;
		auto children = mManager->getChildren( parent );
		for ( size_t i = 0; i < children.size(); ++i ) {
			if ( children[i] == ref )
				return static_cast<Int32>( i );
		}
		return -1;
	}

	void sendWindowChanged( AccessibilityManager& manager, bool added, Int32 index ) {
		ScopedManager scopedManager( *this, &manager );
		auto root = manager.getRoot();
		DBusMessage* message =
			mDBus.messageNewSignal( RootPath, "org.a11y.atspi.Event.Object", "ChildrenChanged" );
		if ( !message )
			return;
		DBusMessageIter iter;
		DBusMessageIter variant;
		mDBus.messageIterInitAppend( message, &iter );
		const char* detail = added ? "add" : "remove";
		appendBasic( iter, 's', &detail );
		Int32 detail2 = 0;
		appendBasic( iter, 'i', &index );
		appendBasic( iter, 'i', &detail2 );
		mDBus.messageIterOpenContainer( &iter, 'v', "(so)", &variant );
		appendRef( variant, root );
		mDBus.messageIterCloseContainer( &iter, &variant );
		appendEventProperties( iter );
		send( message );
	}

	void send( DBusMessage* reply ) {
		if ( !reply )
			return;
		mDBus.connectionSend( mConnection, reply, nullptr );
		mDBus.messageUnref( reply );
	}

	void sendBasic( DBusMessage* request, int type, const void* value ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		appendBasic( iter, type, value );
		send( reply );
	}

	void sendError( DBusMessage* request, const char* name, const char* message ) {
		send( mDBus.messageNewError( request, name, message ) );
	}

	Math::Rectf boundsForCoordinateType( AccessibilityNodeRef ref,
										 const AccessibilityNodeInfo& info,
										 Uint32 coordinateType ) {
		auto bounds = info.bounds;
		if ( coordinateType == 0 ) {
			auto scene = mManager ? mManager->getSceneNode() : nullptr;
			if ( scene && scene->getWindow() ) {
				auto position = scene->getWindow()->getPosition();
				bounds.move( Math::Vector2f( position.x, position.y ) );
			}
		} else if ( coordinateType == 2 ) {
			auto parent = getParent( ref );
			if ( parent.isValid() ) {
				auto parentBounds = getNodeInfo( parent ).bounds;
				bounds.move( Math::Vector2f( -parentBounds.Left, -parentBounds.Top ) );
			}
		}
		return bounds;
	}

	void registerApplication() {
		mBusName = mDBus.busGetUniqueName( mConnection );
		static const DBusObjectPathVTable vtable{
			nullptr, &AtSpiApplication::handleMessage, nullptr, nullptr, nullptr, nullptr };
		if ( mBusName.empty() ||
			 !mDBus.connectionRegisterObjectPath( mConnection, RootPath, &vtable, this ) ||
			 !mDBus.connectionRegisterObjectPath( mConnection, CachePath, &vtable, this ) ||
			 !mDBus.connectionRegisterFallback( mConnection, "/org/eepp/a11y", &vtable, this ) ) {
			mDBus.connectionClose( mConnection );
			mDBus.connectionUnref( mConnection );
			mConnection = nullptr;
			return;
		}

		DBusMessage* embed = mDBus.messageNewMethodCall( "org.a11y.atspi.Registry",
														 "/org/a11y/atspi/accessible/root",
														 "org.a11y.atspi.Socket", "Embed" );
		if ( !embed )
			return;
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( embed, &iter );
		appendApplicationRef( iter );
		DBusMessage* reply =
			mDBus.connectionSendWithReplyAndBlock( mConnection, embed, 1000, nullptr );
		mDBus.messageUnref( embed );
		if ( reply && mDBus.messageGetType( reply ) != DBusMessageTypeError ) {
			DBusMessageIter iter;
			DBusMessageIter structure;
			if ( mDBus.messageIterInit( reply, &iter ) ) {
				mDBus.messageIterRecurse( &iter, &structure );
				const char* bus = nullptr;
				const char* path = nullptr;
				mDBus.messageIterGetBasic( &structure, &bus );
				if ( mDBus.messageIterNext( &structure ) )
					mDBus.messageIterGetBasic( &structure, &path );
				if ( bus && path ) {
					mRegistryBusName = bus;
					mRegistryPath = path;
				}
			}
			mDBus.messageUnref( reply );
			return;
		}
		if ( reply )
			mDBus.messageUnref( reply );
		mDBus.connectionClose( mConnection );
		mDBus.connectionUnref( mConnection );
		mConnection = nullptr;
	}

	DBusHandlerResult handleMessage( DBusMessage* request ) {
		const char* path = mDBus.messageGetPath( request );
		const char* interface = mDBus.messageGetInterface( request );
		const char* member = mDBus.messageGetMember( request );
		if ( path && interface && member && std::strcmp( path, CachePath ) == 0 &&
			 std::strcmp( interface, "org.a11y.atspi.Cache" ) == 0 &&
			 std::strcmp( member, "GetItems" ) == 0 ) {
			DBusMessage* reply = mDBus.messageNewMethodReturn( request );
			DBusMessageIter iter;
			DBusMessageIter array;
			mDBus.messageIterInitAppend( reply, &iter );
			mDBus.messageIterOpenContainer( &iter, 'a', "((so)(so)a(so)assusau)", &array );
			mDBus.messageIterCloseContainer( &iter, &array );
			send( reply );
			return 0;
		}
		if ( path && interface && member && std::strcmp( path, CachePath ) == 0 &&
			 std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
			 std::strcmp( member, "Get" ) == 0 ) {
			const char* requestedInterface = nullptr;
			const char* property = nullptr;
			if ( !mDBus.messageGetArgs( request, nullptr, 's', &requestedInterface, 's', &property,
										0 ) ||
				 !requestedInterface || !property ||
				 std::strcmp( requestedInterface, "org.a11y.atspi.Cache" ) != 0 ||
				 std::strcmp( property, "version" ) != 0 )
				return 1;
			DBusMessage* reply = mDBus.messageNewMethodReturn( request );
			DBusMessageIter iter;
			DBusMessageIter variant;
			mDBus.messageIterInitAppend( reply, &iter );
			mDBus.messageIterOpenContainer( &iter, 'v', "u", &variant );
			Uint32 version = 1;
			appendBasic( variant, 'u', &version );
			mDBus.messageIterCloseContainer( &iter, &variant );
			send( reply );
			return 0;
		}
		auto ref = refFromPath( path );
		if ( !interface || !member || !isValid( ref ) )
			return 1;

		if ( std::strcmp( interface, "org.a11y.atspi.Accessible" ) == 0 ) {
			if ( std::strcmp( member, "GetRole" ) == 0 ) {
				auto info = getNodeInfo( ref, false );
				Uint32 value = role( info.role );
				sendBasic( request, 'u', &value );
			} else if ( std::strcmp( member, "GetRoleName" ) == 0 ||
						std::strcmp( member, "GetLocalizedRoleName" ) == 0 ) {
				auto info = getNodeInfo( ref, false );
				const char* value = roleName( info.role );
				sendBasic( request, 's', &value );
			} else if ( std::strcmp( member, "GetChildAtIndex" ) == 0 ) {
				Int32 index = -1;
				mDBus.messageGetArgs( request, nullptr, 'i', &index, 0 );
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				if ( ref == ApplicationRef && index >= 0 &&
					 static_cast<size_t>( index ) < mManagers.size() ) {
					auto manager = std::next( mManagers.begin(), index )->second;
					ScopedManager scopedManager( *this, manager );
					appendRef( iter, manager->getRoot() );
				} else {
					appendRef( iter, index >= 0 ? getChild( ref, index ) : AccessibilityNodeRef{} );
				}
				send( reply );
			} else if ( std::strcmp( member, "GetChildren" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter array;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'a', "(so)", &array );
				if ( ref == ApplicationRef ) {
					for ( const auto& entry : mManagers ) {
						ScopedManager scopedManager( *this, entry.second );
						appendRef( array, entry.second->getRoot() );
					}
				} else {
					for ( const auto& child : mManager->getChildren( ref ) )
						appendRef( array, child );
				}
				mDBus.messageIterCloseContainer( &iter, &array );
				send( reply );
			} else if ( std::strcmp( member, "GetIndexInParent" ) == 0 ) {
				Int32 index = indexInParent( ref );
				sendBasic( request, 'i', &index );
			} else if ( std::strcmp( member, "GetApplication" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendApplicationRef( iter );
				send( reply );
			} else if ( std::strcmp( member, "GetInterfaces" ) == 0 ) {
				auto info = getNodeInfo( ref, false );
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendInterfaces( iter, ref, info );
				send( reply );
			} else if ( std::strcmp( member, "GetState" ) == 0 ) {
				auto info = getNodeInfo( ref, false );
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendStates( iter, info.states );
				send( reply );
			} else if ( std::strcmp( member, "GetAttributes" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter attributes;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'a', "{ss}", &attributes );
				mDBus.messageIterCloseContainer( &iter, &attributes );
				send( reply );
			} else if ( std::strcmp( member, "GetRelationSet" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter relations;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'a', "(ua(so))", &relations );
				mDBus.messageIterCloseContainer( &iter, &relations );
				send( reply );
			} else if ( std::strcmp( member, "GetAccessibleId" ) == 0 ) {
				std::string valueStorage = pathFromRef( ref );
				const char* value = valueStorage.c_str();
				sendBasic( request, 's', &value );
			} else if ( std::strcmp( member, "GetLocale" ) == 0 ) {
				const char* locale = "C";
				sendBasic( request, 's', &locale );
			} else {
				return 1;
			}
			return 0;
		}

		if ( std::strcmp( interface, "org.a11y.atspi.Application" ) == 0 ) {
			if ( std::strcmp( member, "GetLocale" ) == 0 ) {
				const char* locale = "C";
				sendBasic( request, 's', &locale );
			} else if ( std::strcmp( member, "GetApplicationBusAddress" ) == 0 ) {
				const char* address = "";
				sendBasic( request, 's', &address );
			} else {
				return 1;
			}
			return 0;
		}

		if ( std::strcmp( interface, "org.a11y.atspi.Component" ) == 0 ) {
			if ( std::strcmp( member, "GetExtents" ) == 0 ) {
				auto info = getNodeInfo( ref, false );
				Uint32 coordinateType = 0;
				mDBus.messageGetArgs( request, nullptr, 'u', &coordinateType, 0 );
				auto bounds = boundsForCoordinateType( ref, info, coordinateType );
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter structure;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'r', nullptr, &structure );
				Int32 values[] = { static_cast<Int32>( bounds.Left ),
								   static_cast<Int32>( bounds.Top ),
								   static_cast<Int32>( bounds.getWidth() ),
								   static_cast<Int32>( bounds.getHeight() ) };
				for ( auto value : values )
					appendBasic( structure, 'i', &value );
				mDBus.messageIterCloseContainer( &iter, &structure );
				send( reply );
			} else if ( std::strcmp( member, "Contains" ) == 0 ) {
				auto info = getNodeInfo( ref, false );
				Int32 x = 0;
				Int32 y = 0;
				Uint32 coordinateType = 0;
				mDBus.messageGetArgs( request, nullptr, 'i', &x, 'i', &y, 'u', &coordinateType, 0 );
				int contains = boundsForCoordinateType( ref, info, coordinateType )
								   .contains( Math::Vector2f( x, y ) );
				sendBasic( request, 'b', &contains );
			} else if ( std::strcmp( member, "GetAccessibleAtPoint" ) == 0 ) {
				Int32 x = 0;
				Int32 y = 0;
				Uint32 coordinateType = 0;
				mDBus.messageGetArgs( request, nullptr, 'i', &x, 'i', &y, 'u', &coordinateType, 0 );
				if ( coordinateType == 0 ) {
					auto scene = mManager ? mManager->getSceneNode() : nullptr;
					if ( scene && scene->getWindow() ) {
						auto position = scene->getWindow()->getPosition();
						x -= position.x;
						y -= position.y;
					}
				}
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendRef( iter, mManager ? mManager->hitTest( Math::Vector2f( x, y ) )
										  : AccessibilityNodeRef{} );
				send( reply );
			} else if ( std::strcmp( member, "GrabFocus" ) == 0 ) {
				int success =
					mManager && mManager->performAction( ref, { AccessibilityAction::Focus, {} } );
				sendBasic( request, 'b', &success );
			} else if ( std::strcmp( member, "GetPosition" ) == 0 ) {
				auto info = getNodeInfo( ref, false );
				Uint32 coordinateType = 0;
				mDBus.messageGetArgs( request, nullptr, 'u', &coordinateType, 0 );
				auto bounds = boundsForCoordinateType( ref, info, coordinateType );
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				Int32 x = static_cast<Int32>( bounds.Left );
				Int32 y = static_cast<Int32>( bounds.Top );
				appendBasic( iter, 'i', &x );
				appendBasic( iter, 'i', &y );
				send( reply );
			} else if ( std::strcmp( member, "GetSize" ) == 0 ) {
				auto info = getNodeInfo( ref, false );
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				Int32 width = static_cast<Int32>( info.bounds.getWidth() );
				Int32 height = static_cast<Int32>( info.bounds.getHeight() );
				appendBasic( iter, 'i', &width );
				appendBasic( iter, 'i', &height );
				send( reply );
			} else if ( std::strcmp( member, "GetLayer" ) == 0 ) {
				Uint32 layer = 1;
				sendBasic( request, 'u', &layer );
			} else if ( std::strcmp( member, "GetMDIZOrder" ) == 0 ) {
				Int16 order = 0;
				sendBasic( request, 'n', &order );
			} else if ( std::strcmp( member, "GetAlpha" ) == 0 ) {
				double alpha = 1;
				sendBasic( request, 'd', &alpha );
			} else {
				return 1;
			}
			return 0;
		}

		if ( std::strcmp( interface, "org.a11y.atspi.Action" ) == 0 ) {
			auto info = getNodeInfo( ref, false );
			const auto actions = nativeActions( info.actions );
			Int32 count = static_cast<Int32>( __builtin_popcount( actions ) );
			if ( std::strcmp( member, "GetNActions" ) == 0 ) {
				sendBasic( request, 'i', &count );
			} else if ( std::strcmp( member, "GetActions" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter array;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'a', "(sss)", &array );
				for ( Int32 index = 0; index < count; ++index ) {
					DBusMessageIter structure;
					auto action = actionAt( actions, index );
					const char* name = actionName( action );
					const char* empty = "";
					mDBus.messageIterOpenContainer( &array, 'r', nullptr, &structure );
					appendBasic( structure, 's', &name );
					appendBasic( structure, 's', &empty );
					appendBasic( structure, 's', &empty );
					mDBus.messageIterCloseContainer( &array, &structure );
				}
				mDBus.messageIterCloseContainer( &iter, &array );
				send( reply );
			} else {
				Int32 index = -1;
				mDBus.messageGetArgs( request, nullptr, 'i', &index, 0 );
				if ( index < 0 || index >= count )
					return 1;
				auto action = actionAt( actions, index );
				if ( std::strcmp( member, "DoAction" ) == 0 ) {
					int success = mManager && mManager->performAction( ref, { action, {} } );
					sendBasic( request, 'b', &success );
				} else if ( std::strcmp( member, "GetName" ) == 0 ||
							std::strcmp( member, "GetLocalizedName" ) == 0 ) {
					const char* name = actionName( action );
					sendBasic( request, 's', &name );
				} else if ( std::strcmp( member, "GetDescription" ) == 0 ||
							std::strcmp( member, "GetKeyBinding" ) == 0 ) {
					const char* empty = "";
					sendBasic( request, 's', &empty );
				} else {
					return 1;
				}
			}
			return 0;
		}

		if ( std::strcmp( interface, "org.a11y.atspi.Text" ) == 0 ) {
			auto info = getNodeInfo( ref, true );
			if ( !info.text.valid )
				return 1;
			const Int32 characterCount = static_cast<Int32>( info.value.size() );
			if ( std::strcmp( member, "GetText" ) == 0 ) {
				Int32 start = 0;
				Int32 end = -1;
				mDBus.messageGetArgs( request, nullptr, 'i', &start, 'i', &end, 0 );
				start = std::max( 0, std::min( start, characterCount ) );
				end = end < 0 ? characterCount : std::max( start, std::min( end, characterCount ) );
				std::string textStorage = info.value.substr( start, end - start ).toUtf8();
				const char* text = textStorage.c_str();
				sendBasic( request, 's', &text );
			} else if ( std::strcmp( member, "GetStringAtOffset" ) == 0 ) {
				Int32 offset = 0;
				Uint32 granularity = 0;
				mDBus.messageGetArgs( request, nullptr, 'i', &offset, 'u', &granularity, 0 );
				Int32 start = std::max( 0, std::min( offset, characterCount ) );
				Int32 end = start;
				if ( granularity == 0 && start < characterCount ) {
					end = start + 1;
				} else if ( granularity == 1 ) {
					auto isSpace = [&info]( Int32 index ) {
						auto character = info.value[index];
						return character == ' ' || character == '\t' || character == '\n' ||
							   character == '\r';
					};
					while ( start > 0 && !isSpace( start - 1 ) )
						--start;
					end = std::max( 0, std::min( offset, characterCount ) );
					while ( end < characterCount && !isSpace( end ) )
						++end;
				} else {
					start = 0;
					end = characterCount;
				}
				std::string textStorage = info.value.substr( start, end - start ).toUtf8();
				const char* text = textStorage.c_str();
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendBasic( iter, 's', &text );
				appendBasic( iter, 'i', &start );
				appendBasic( iter, 'i', &end );
				send( reply );
			} else if ( std::strcmp( member, "GetCharacterAtOffset" ) == 0 ) {
				Int32 offset = 0;
				mDBus.messageGetArgs( request, nullptr, 'i', &offset, 0 );
				Int32 character = offset >= 0 && offset < characterCount ? info.value[offset] : 0;
				sendBasic( request, 'i', &character );
			} else if ( std::strcmp( member, "GetAttributes" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter attributes;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'a', "{ss}", &attributes );
				mDBus.messageIterCloseContainer( &iter, &attributes );
				Int32 start = 0;
				appendBasic( iter, 'i', &start );
				appendBasic( iter, 'i', &characterCount );
				send( reply );
			} else if ( std::strcmp( member, "GetDefaultAttributes" ) == 0 ||
						std::strcmp( member, "GetDefaultAttributeSet" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter attributes;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'a', "{ss}", &attributes );
				mDBus.messageIterCloseContainer( &iter, &attributes );
				send( reply );
			} else if ( std::strcmp( member, "GetNSelections" ) == 0 ) {
				Int32 count = info.text.selectionStart != info.text.selectionEnd ? 1 : 0;
				sendBasic( request, 'i', &count );
			} else if ( std::strcmp( member, "GetSelection" ) == 0 ) {
				Int32 selection = -1;
				mDBus.messageGetArgs( request, nullptr, 'i', &selection, 0 );
				if ( selection != 0 || info.text.selectionStart == info.text.selectionEnd )
					return 1;
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendBasic( iter, 'i', &info.text.selectionStart );
				appendBasic( iter, 'i', &info.text.selectionEnd );
				send( reply );
			} else if ( std::strcmp( member, "SetCaretOffset" ) == 0 ) {
				int success = false;
				sendBasic( request, 'b', &success );
			} else {
				return 1;
			}
			return 0;
		}

		if ( std::strcmp( interface, "org.a11y.atspi.EditableText" ) == 0 ) {
			auto info = getNodeInfo( ref, true );
			if ( !info.text.valid || !hasState( info.states, AccessibilityState::Editable ) ||
				 std::strcmp( member, "SetTextContents" ) != 0 )
				return 1;
			const char* contents = nullptr;
			if ( !mDBus.messageGetArgs( request, nullptr, 's', &contents, 0 ) )
				return 1;
			int success =
				mManager &&
				mManager->performAction(
					ref, { AccessibilityAction::SetText,
						   String::fromUtf8( std::string_view( contents ? contents : "" ) ) } );
			sendBasic( request, 'b', &success );
			return 0;
		}

		if ( std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
			 std::strcmp( member, "Get" ) == 0 ) {
			const char* requestedInterface = nullptr;
			const char* property = nullptr;
			if ( !mDBus.messageGetArgs( request, nullptr, 's', &requestedInterface, 's', &property,
										0 ) ||
				 !requestedInterface || !property )
				return 1;
			const bool includeValue =
				( std::strcmp( requestedInterface, "org.a11y.atspi.Text" ) == 0 &&
				  std::strcmp( property, "CharacterCount" ) == 0 ) ||
				( std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) == 0 &&
				  std::strcmp( property, "CurrentValue" ) == 0 );
			const bool needsInfo =
				std::strcmp( requestedInterface, "org.a11y.atspi.Action" ) == 0 ||
				std::strcmp( requestedInterface, "org.a11y.atspi.Text" ) == 0 ||
				std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) == 0 ||
				std::strcmp( property, "Name" ) == 0 || std::strcmp( property, "Description" ) == 0;
			AccessibilityNodeInfo info;
			if ( needsInfo )
				info = getNodeInfo( ref, includeValue );
			DBusMessage* reply = mDBus.messageNewMethodReturn( request );
			DBusMessageIter iter;
			DBusMessageIter variant;
			mDBus.messageIterInitAppend( reply, &iter );
			if ( std::strcmp( requestedInterface, "org.a11y.atspi.Action" ) == 0 &&
				 std::strcmp( property, "NActions" ) == 0 ) {
				Int32 value =
					static_cast<Int32>( __builtin_popcount( nativeActions( info.actions ) ) );
				mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
				appendBasic( variant, 'i', &value );
			} else if ( std::strcmp( requestedInterface, "org.a11y.atspi.Text" ) == 0 &&
						info.text.valid &&
						( std::strcmp( property, "CharacterCount" ) == 0 ||
						  std::strcmp( property, "CaretOffset" ) == 0 ) ) {
				Int32 value = std::strcmp( property, "CharacterCount" ) == 0
								  ? static_cast<Int32>( info.value.size() )
								  : info.text.caretOffset;
				mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
				appendBasic( variant, 'i', &value );
			} else if ( std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) == 0 &&
						info.range.valid ) {
				double value = 0;
				if ( std::strcmp( property, "CurrentValue" ) == 0 )
					String::fromString( value, info.value.toUtf8() );
				else if ( std::strcmp( property, "MaximumValue" ) == 0 )
					value = info.range.maximum;
				else if ( std::strcmp( property, "MinimumValue" ) == 0 )
					value = info.range.minimum;
				else if ( std::strcmp( property, "MinimumIncrement" ) == 0 )
					value = info.range.smallChange;
				else {
					mDBus.messageUnref( reply );
					return 1;
				}
				mDBus.messageIterOpenContainer( &iter, 'v', "d", &variant );
				appendBasic( variant, 'd', &value );
			} else if ( std::strcmp( property, "Name" ) == 0 ||
						std::strcmp( property, "Description" ) == 0 ) {
				std::string text =
					( std::strcmp( property, "Name" ) == 0 ? info.name : info.description )
						.toUtf8();
				const char* value = text.c_str();
				mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
				appendBasic( variant, 's', &value );
			} else if ( std::strcmp( property, "HelpText" ) == 0 ) {
				const char* value = "";
				mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
				appendBasic( variant, 's', &value );
			} else if ( std::strcmp( property, "Locale" ) == 0 ) {
				const char* value = "C";
				mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
				appendBasic( variant, 's', &value );
			} else if ( std::strcmp( property, "AccessibleId" ) == 0 ) {
				std::string valueStorage = pathFromRef( ref );
				const char* value = valueStorage.c_str();
				mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
				appendBasic( variant, 's', &value );
			} else if ( std::strcmp( property, "ChildCount" ) == 0 ) {
				Int32 value = static_cast<Int32>( getChildCount( ref ) );
				mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
				appendBasic( variant, 'i', &value );
			} else if ( std::strcmp( property, "Parent" ) == 0 ) {
				mDBus.messageIterOpenContainer( &iter, 'v', "(so)", &variant );
				if ( ref == ApplicationRef )
					appendDesktopRef( variant );
				else
					appendRef( variant, getParent( ref ) );
			} else if ( std::strcmp( property, "ToolkitName" ) == 0 ||
						std::strcmp( property, "Version" ) == 0 ||
						std::strcmp( property, "AtspiVersion" ) == 0 ) {
				const char* value = std::strcmp( property, "ToolkitName" ) == 0 ? "eepp" : "0.1";
				mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
				appendBasic( variant, 's', &value );
			} else if ( std::strcmp( property, "ProcessId" ) == 0 ) {
				Int32 value = static_cast<Int32>( getpid() );
				mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
				appendBasic( variant, 'i', &value );
			} else if ( std::strcmp( property, "Id" ) == 0 ) {
				mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
				appendBasic( variant, 'i', &mApplicationId );
			} else {
				mDBus.messageUnref( reply );
				return 1;
			}
			mDBus.messageIterCloseContainer( &iter, &variant );
			send( reply );
			return 0;
		}
		if ( std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
			 std::strcmp( member, "Set" ) == 0 && ref == ApplicationRef ) {
			DBusMessageIter iter;
			DBusMessageIter variant;
			const char* requestedInterface = nullptr;
			const char* property = nullptr;
			if ( !mDBus.messageIterInit( request, &iter ) )
				return 1;
			mDBus.messageIterGetBasic( &iter, &requestedInterface );
			if ( !mDBus.messageIterNext( &iter ) )
				return 1;
			mDBus.messageIterGetBasic( &iter, &property );
			if ( !mDBus.messageIterNext( &iter ) || !requestedInterface || !property ||
				 std::strcmp( requestedInterface, "org.a11y.atspi.Application" ) != 0 ||
				 std::strcmp( property, "Id" ) != 0 )
				return 1;
			mDBus.messageIterRecurse( &iter, &variant );
			mDBus.messageIterGetBasic( &variant, &mApplicationId );
			send( mDBus.messageNewMethodReturn( request ) );
			return 0;
		}
		if ( std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
			 std::strcmp( member, "Set" ) == 0 ) {
			DBusMessageIter iter;
			DBusMessageIter variant;
			const char* requestedInterface = nullptr;
			const char* property = nullptr;
			double value = 0;
			if ( !mDBus.messageIterInit( request, &iter ) )
				return 1;
			mDBus.messageIterGetBasic( &iter, &requestedInterface );
			if ( !mDBus.messageIterNext( &iter ) )
				return 1;
			mDBus.messageIterGetBasic( &iter, &property );
			if ( !mDBus.messageIterNext( &iter ) )
				return 1;
			mDBus.messageIterRecurse( &iter, &variant );
			mDBus.messageIterGetBasic( &variant, &value );
			if ( !requestedInterface || !property ||
				 std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) != 0 ||
				 std::strcmp( property, "CurrentValue" ) != 0 )
				return 1;
			auto info = getNodeInfo( ref, false );
			if ( !info.range.valid ||
				 !( info.actions & accessibilityActionMask( AccessibilityAction::SetValue ) ) )
				return 1;
			if ( !mManager ||
				 !mManager->performAction(
					 ref, { AccessibilityAction::SetValue, String( String::toString( value ) ) } ) )
				return 1;
			send( mDBus.messageNewMethodReturn( request ) );
			return 0;
		}
		return 1;
	}
};

class AtSpiAccessibilityBackend final : public AccessibilityBackend {
  public:
	explicit AtSpiAccessibilityBackend( AccessibilityManager& manager ) : mManager( manager ) {
		auto scene = manager.getSceneNode();
		if ( !scene || !scene->getWindow() || scene->getParent() )
			return;
		mApplication = application().lock();
		if ( !mApplication ) {
			mApplication = std::make_shared<AtSpiApplication>( manager );
			application() = mApplication;
		} else {
			mApplication->registerManager( manager );
		}
	}

	~AtSpiAccessibilityBackend() {
		if ( mApplication )
			mApplication->unregisterManager( mManager );
	}

	bool isAvailable() const { return mApplication && mApplication->isAvailable(); }

	bool hasActiveClients() const { return mApplication && mApplication->hasActiveClients(); }

	void update() {
		if ( mApplication )
			mApplication->update();
	}

	void onEvent( const AccessibilityPendingEvent& event ) {
		if ( mApplication )
			mApplication->onEvent( mManager, event );
	}

  private:
	static std::weak_ptr<AtSpiApplication>& application() {
		static std::weak_ptr<AtSpiApplication> instance;
		return instance;
	}

	AccessibilityManager& mManager;
	std::shared_ptr<AtSpiApplication> mApplication;
};

} // namespace

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager ) {
	return std::make_unique<AtSpiAccessibilityBackend>( manager );
}

}} // namespace EE::UI

#endif
