#include "accessibilitybackend.hpp"
#include <cstdlib>
#include <cstring>
#include <eepp/system/sys.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD

namespace EE { namespace UI {

namespace {

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
using DBusMessageFunction = DBusHandlerResult ( * )( DBusConnection*, DBusMessage*, void* );

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
	using ConnectionReadWriteDispatch = int ( * )( DBusConnection*, int );
	using ConnectionRegisterObjectPath = int ( * )( DBusConnection*, const char*,
													const DBusObjectPathVTable*, void* );
	using ConnectionRegisterFallback = ConnectionRegisterObjectPath;
	using ConnectionSend = int ( * )( DBusConnection*, DBusMessage*, Uint32* );
	using ConnectionFlush = void ( * )( DBusConnection* );
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
			   loadSymbol( connectionReadWriteDispatch, "dbus_connection_read_write_dispatch" ) &&
			   loadSymbol( connectionRegisterObjectPath, "dbus_connection_register_object_path" ) &&
			   loadSymbol( connectionRegisterFallback, "dbus_connection_register_fallback" ) &&
			   loadSymbol( connectionSend, "dbus_connection_send" ) &&
			   loadSymbol( connectionFlush, "dbus_connection_flush" ) &&
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
	ConnectionReadWriteDispatch connectionReadWriteDispatch{};
	ConnectionRegisterObjectPath connectionRegisterObjectPath{};
	ConnectionRegisterFallback connectionRegisterFallback{};
	ConnectionSend connectionSend{};
	ConnectionFlush connectionFlush{};
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

class AtSpiAccessibilityBackend final : public AccessibilityBackend {
  public:
	explicit AtSpiAccessibilityBackend( AccessibilityManager& manager ) : mManager( manager ) {
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
		if ( mConnection )
			registerApplication();
	}

	~AtSpiAccessibilityBackend() {
		if ( mConnection ) {
			mDBus.connectionClose( mConnection );
			mDBus.connectionUnref( mConnection );
		}
	}

	bool isAvailable() const { return mConnection != nullptr; }

	bool hasActiveClients() const { return mHasActiveClients; }

	void update() {
		if ( mConnection )
			mDBus.connectionReadWriteDispatch( mConnection, 0 );
	}

	void onEvent( const AccessibilityPendingEvent& event ) {
		if ( !mConnection )
			return;
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
		} else if ( event.type == AccessibilityEvent::ValueChanged ) {
			detail = "accessible-value";
		} else if ( event.type == AccessibilityEvent::ChildrenChanged ||
					event.type == AccessibilityEvent::Created ||
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
		Int32 detail1 = event.type == AccessibilityEvent::FocusChanged ||
								event.type == AccessibilityEvent::SelectionChanged
							? 1
							: 0;
		Int32 detail2 = 0;
		appendBasic( iter, 'i', &detail1 );
		appendBasic( iter, 'i', &detail2 );
		const char* empty = "";
		mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
		appendBasic( variant, 's', &empty );
		mDBus.messageIterCloseContainer( &iter, &variant );
		appendRef( iter, mManager.getRoot() );
		send( message );
		mDBus.connectionFlush( mConnection );
	}

  private:
	static constexpr const char* RootPath = "/org/a11y/atspi/accessible/root";
	static constexpr const char* NodePathPrefix = "/org/eepp/a11y/";

	AccessibilityManager& mManager;
	DBusLibrary mDBus;
	DBusConnection* mConnection{};
	std::string mBusName;
	bool mHasActiveClients{ false };

	static DBusHandlerResult handleMessage( DBusConnection*, DBusMessage* message,
											void* userData ) {
		return static_cast<AtSpiAccessibilityBackend*>( userData )->handleMessage( message );
	}

	AccessibilityNodeRef refFromPath( const char* path ) {
		if ( !path )
			return {};
		if ( std::strcmp( path, RootPath ) == 0 )
			return mManager.getRoot();
		if ( std::strncmp( path, NodePathPrefix, std::strlen( NodePathPrefix ) ) != 0 )
			return {};
		char* end = nullptr;
		Uint64 id = std::strtoull( path + std::strlen( NodePathPrefix ), &end, 10 );
		return end && *end == '\0' ? AccessibilityNodeRef{ 1, id } : AccessibilityNodeRef{};
	}

	std::string pathFromRef( AccessibilityNodeRef ref ) {
		if ( ref == mManager.getRoot() )
			return RootPath;
		return std::string( NodePathPrefix ) + String::toString( ref.id );
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
			default:
				return "unknown";
		}
	}

	AccessibilityAction actionAt( AccessibilityActions actions, Int32 index ) const {
		for ( Uint32 action = 0; action <= static_cast<Uint32>( AccessibilityAction::SetText );
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
		}
		return "";
	}

	void appendBasic( DBusMessageIter& iter, int type, const void* value ) {
		mDBus.messageIterAppendBasic( &iter, type, value );
	}

	void appendRef( DBusMessageIter& iter, AccessibilityNodeRef ref ) {
		DBusMessageIter structure;
		mDBus.messageIterOpenContainer( &iter, 'r', nullptr, &structure );
		const char* bus = ref.isValid() ? mBusName.c_str() : "";
		std::string pathStorage = ref.isValid() ? pathFromRef( ref ) : "/org/a11y/atspi/null";
		const char* path = pathStorage.c_str();
		appendBasic( structure, 's', &bus );
		appendBasic( structure, 'o', &path );
		mDBus.messageIterCloseContainer( &iter, &structure );
	}

	void appendDesktopRef( DBusMessageIter& iter ) {
		DBusMessageIter structure;
		mDBus.messageIterOpenContainer( &iter, 'r', nullptr, &structure );
		const char* bus = "org.a11y.atspi.Registry";
		const char* path = RootPath;
		appendBasic( structure, 's', &bus );
		appendBasic( structure, 'o', &path );
		mDBus.messageIterCloseContainer( &iter, &structure );
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

	Math::Rectf boundsForCoordinateType( AccessibilityNodeRef ref,
										 const AccessibilityNodeInfo& info,
										 Uint32 coordinateType ) {
		auto bounds = info.bounds;
		if ( coordinateType == 0 ) {
			auto scene = mManager.getSceneNode();
			if ( scene && scene->getWindow() ) {
				auto position = scene->getWindow()->getPosition();
				bounds.move( Math::Vector2f( position.x, position.y ) );
			}
		} else if ( coordinateType == 2 ) {
			auto parent = mManager.getParent( ref );
			if ( parent.isValid() ) {
				auto parentBounds = mManager.getNodeInfo( parent ).bounds;
				bounds.move( Math::Vector2f( -parentBounds.Left, -parentBounds.Top ) );
			}
		}
		return bounds;
	}

	void registerApplication() {
		mBusName = mDBus.busGetUniqueName( mConnection );
		static const DBusObjectPathVTable vtable{
			nullptr, &AtSpiAccessibilityBackend::handleMessage, nullptr, nullptr, nullptr,
			nullptr };
		if ( mBusName.empty() ||
			 !mDBus.connectionRegisterObjectPath( mConnection, RootPath, &vtable, this ) ||
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
		appendRef( iter, mManager.getRoot() );
		DBusMessage* reply =
			mDBus.connectionSendWithReplyAndBlock( mConnection, embed, 1000, nullptr );
		mDBus.messageUnref( embed );
		if ( reply && mDBus.messageGetType( reply ) != DBusMessageTypeError ) {
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
		auto ref = refFromPath( path );
		if ( !interface || !member || !mManager.isValid( ref ) )
			return 1;
		mHasActiveClients = true;
		auto info = mManager.getNodeInfo( ref );

		if ( std::strcmp( interface, "org.a11y.atspi.Accessible" ) == 0 ) {
			if ( std::strcmp( member, "GetRole" ) == 0 ) {
				Uint32 value = role( info.role );
				sendBasic( request, 'u', &value );
			} else if ( std::strcmp( member, "GetRoleName" ) == 0 ||
						std::strcmp( member, "GetLocalizedRoleName" ) == 0 ) {
				const char* value = roleName( info.role );
				sendBasic( request, 's', &value );
			} else if ( std::strcmp( member, "GetChildAtIndex" ) == 0 ) {
				Int32 index = -1;
				mDBus.messageGetArgs( request, nullptr, 'i', &index, 0 );
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendRef( iter,
						   index >= 0 ? mManager.getChild( ref, index ) : AccessibilityNodeRef{} );
				send( reply );
			} else if ( std::strcmp( member, "GetChildren" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter array;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'a', "(so)", &array );
				for ( size_t i = 0; i < mManager.getChildCount( ref ); ++i )
					appendRef( array, mManager.getChild( ref, i ) );
				mDBus.messageIterCloseContainer( &iter, &array );
				send( reply );
			} else if ( std::strcmp( member, "GetIndexInParent" ) == 0 ) {
				Int32 index = -1;
				auto parent = mManager.getParent( ref );
				for ( size_t i = 0; parent.isValid() && i < mManager.getChildCount( parent );
					  ++i ) {
					if ( mManager.getChild( parent, i ) == ref ) {
						index = static_cast<Int32>( i );
						break;
					}
				}
				sendBasic( request, 'i', &index );
			} else if ( std::strcmp( member, "GetApplication" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendRef( iter, mManager.getRoot() );
				send( reply );
			} else if ( std::strcmp( member, "GetInterfaces" ) == 0 ) {
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter array;
				mDBus.messageIterInitAppend( reply, &iter );
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
				if ( ref == mManager.getRoot() ) {
					const char* application = "org.a11y.atspi.Application";
					appendBasic( array, 's', &application );
				}
				mDBus.messageIterCloseContainer( &iter, &array );
				send( reply );
			} else if ( std::strcmp( member, "GetState" ) == 0 ) {
				Uint32 words[2]{};
				auto addState = [&words]( bool enabled, Uint32 state ) {
					if ( enabled )
						words[state / 32] |= 1u << ( state % 32 );
				};
				Uint64 states = static_cast<Uint64>( info.states );
				addState( states & static_cast<Uint64>( AccessibilityState::Checked ), 4 );
				addState( states & static_cast<Uint64>( AccessibilityState::Editable ), 7 );
				addState( states & static_cast<Uint64>( AccessibilityState::Enabled ), 8 );
				addState( states & static_cast<Uint64>( AccessibilityState::Focusable ), 11 );
				addState( states & static_cast<Uint64>( AccessibilityState::Focused ), 12 );
				addState( states & static_cast<Uint64>( AccessibilityState::Selected ), 23 );
				addState( states & static_cast<Uint64>( AccessibilityState::Showing ), 25 );
				addState( states & static_cast<Uint64>( AccessibilityState::Visible ), 30 );
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				DBusMessageIter array;
				mDBus.messageIterInitAppend( reply, &iter );
				mDBus.messageIterOpenContainer( &iter, 'a', "u", &array );
				appendBasic( array, 'u', &words[0] );
				appendBasic( array, 'u', &words[1] );
				mDBus.messageIterCloseContainer( &iter, &array );
				send( reply );
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
					auto scene = mManager.getSceneNode();
					if ( scene && scene->getWindow() ) {
						auto position = scene->getWindow()->getPosition();
						x -= position.x;
						y -= position.y;
					}
				}
				DBusMessage* reply = mDBus.messageNewMethodReturn( request );
				DBusMessageIter iter;
				mDBus.messageIterInitAppend( reply, &iter );
				appendRef( iter, mManager.hitTest( Math::Vector2f( x, y ) ) );
				send( reply );
			} else if ( std::strcmp( member, "GrabFocus" ) == 0 ) {
				int success = mManager.performAction( ref, { AccessibilityAction::Focus, {} } );
				sendBasic( request, 'b', &success );
			} else {
				return 1;
			}
			return 0;
		}

		if ( std::strcmp( interface, "org.a11y.atspi.Action" ) == 0 ) {
			const auto actions = nativeActions( info.actions );
			Int32 count = static_cast<Int32>( __builtin_popcount( actions ) );
			if ( std::strcmp( member, "GetNActions" ) == 0 ) {
				sendBasic( request, 'i', &count );
			} else {
				Int32 index = -1;
				mDBus.messageGetArgs( request, nullptr, 'i', &index, 0 );
				if ( index < 0 || index >= count )
					return 1;
				auto action = actionAt( actions, index );
				if ( std::strcmp( member, "DoAction" ) == 0 ) {
					int success = mManager.performAction( ref, { action, {} } );
					sendBasic( request, 'b', &success );
				} else if ( std::strcmp( member, "GetName" ) == 0 ) {
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

		if ( std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
			 std::strcmp( member, "Get" ) == 0 ) {
			const char* requestedInterface = nullptr;
			const char* property = nullptr;
			if ( !mDBus.messageGetArgs( request, nullptr, 's', &requestedInterface, 's', &property,
										0 ) )
				return 1;
			DBusMessage* reply = mDBus.messageNewMethodReturn( request );
			DBusMessageIter iter;
			DBusMessageIter variant;
			mDBus.messageIterInitAppend( reply, &iter );
			if ( std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) == 0 &&
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
			} else if ( std::strcmp( property, "ChildCount" ) == 0 ) {
				Int32 value = static_cast<Int32>( mManager.getChildCount( ref ) );
				mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
				appendBasic( variant, 'i', &value );
			} else if ( std::strcmp( property, "Parent" ) == 0 ) {
				mDBus.messageIterOpenContainer( &iter, 'v', "(so)", &variant );
				if ( ref == mManager.getRoot() )
					appendDesktopRef( variant );
				else
					appendRef( variant, mManager.getParent( ref ) );
			} else if ( std::strcmp( property, "ToolkitName" ) == 0 ||
						std::strcmp( property, "Version" ) == 0 ) {
				const char* value = std::strcmp( property, "ToolkitName" ) == 0 ? "eepp" : "0.1";
				mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
				appendBasic( variant, 's', &value );
			} else if ( std::strcmp( property, "Id" ) == 0 ) {
				Int32 value = 0;
				mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
				appendBasic( variant, 'i', &value );
			} else {
				mDBus.messageUnref( reply );
				return 1;
			}
			mDBus.messageIterCloseContainer( &iter, &variant );
			send( reply );
			return 0;
		}
		if ( std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
			 std::strcmp( member, "Set" ) == 0 && info.range.valid &&
			 info.actions & accessibilityActionMask( AccessibilityAction::SetValue ) ) {
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
			if ( !mManager.performAction(
					 ref, { AccessibilityAction::SetValue, String( String::toString( value ) ) } ) )
				return 1;
			send( mDBus.messageNewMethodReturn( request ) );
			return 0;
		}
		return 1;
	}
};

} // namespace

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager ) {
	return std::make_unique<AtSpiAccessibilityBackend>( manager );
}

}} // namespace EE::UI

#endif
