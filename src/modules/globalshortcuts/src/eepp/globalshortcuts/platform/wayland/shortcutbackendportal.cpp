#include <eepp/config.hpp>

#ifdef EE_GLOBALSHORTCUTS_PORTAL

#include "shortcutbackendportal.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <eepp/system/dbus/dbus.hpp>
#include <eepp/system/sys.hpp>
#include <string_view>

namespace EE::System::GlobalShortcuts::Private {

namespace {

using namespace EE::System::Dbus;
using namespace EE::Window;

constexpr char Portal[] = "org.freedesktop.portal.Desktop";
constexpr char DesktopPath[] = "/org/freedesktop/portal/desktop";
constexpr char Shortcuts[] = "org.freedesktop.portal.GlobalShortcuts";
constexpr char Request[] = "org.freedesktop.portal.Request";
constexpr char Session[] = "org.freedesktop.portal.Session";
constexpr char Registry[] = "org.freedesktop.host.portal.Registry";
constexpr size_t MaxShortcuts = 1024;
constexpr size_t MaxText = 8192;
constexpr size_t MaxDeclarationText = 512 * 1024;

using Bindings = std::vector<ShortcutBackend::Binding>;
using Resolver = TaskResolver<Result<void, Error>>;

enum class Stage : Uint8 {
	Idle,
	WatchOwner,
	FindOwner,
	StartPortal,
	RegisterApp,
	Version,
	WatchPortal,
	CloseSession,
	Create,
	Bind,
	List,
	Configure
};

Error failure( const char* message, ErrorCode code = ErrorCode::NativeFailure ) {
	return { code, message };
}

Error dbusError( std::string_view name ) {
	auto code = ErrorCode::NativeFailure;
	if ( name == DBUS_ERROR_NO_REPLY || name == DBUS_ERROR_TIMEOUT )
		code = ErrorCode::Timeout;
	else if ( name == DBUS_ERROR_DISCONNECTED || name == DBUS_ERROR_NAME_HAS_NO_OWNER )
		code = ErrorCode::Disconnected;
	else if ( name == DBUS_ERROR_SERVICE_UNKNOWN )
		code = ErrorCode::BackendUnavailable;
	else if ( name == DBUS_ERROR_UNKNOWN_INTERFACE || name == DBUS_ERROR_UNKNOWN_METHOD ||
			  name == DBUS_ERROR_UNKNOWN_PROPERTY )
		code = ErrorCode::Unsupported;
	else if ( name == DBUS_ERROR_ACCESS_DENIED ||
			  name == "org.freedesktop.portal.Error.NotAllowed" )
		code = ErrorCode::PermissionDenied;
	return { code, "Desktop portal method failed", {}, std::string( name ) };
}

std::optional<std::string> token() {
	std::array<unsigned char, 16> random{};
	if ( !System::Sys::getRandomBytes( random.data(), random.size() ) )
		return {};
	std::string result = "eepp_";
	constexpr char hex[] = "0123456789abcdef";
	for ( auto byte : random ) {
		result += hex[byte >> 4];
		result += hex[byte & 15];
	}
	return result;
}

bool dictionary( DbusWriter& writer, auto append ) {
	return writer.container( DBUS_TYPE_ARRAY, "{sv}", append );
}

bool option( DbusWriter& writer, const char* key, const char* value ) {
	return writer.container( DBUS_TYPE_DICT_ENTRY, nullptr, [&]( DbusWriter& entry ) {
		return entry.string( key ) &&
			   entry.container( DBUS_TYPE_VARIANT, "s",
								[&]( DbusWriter& variant ) { return variant.string( value ); } );
	} );
}

DbusReader property( DbusReader entries, const char* name ) {
	auto it = entries.child( DBUS_TYPE_ARRAY );
	for ( ; it.type() == DBUS_TYPE_DICT_ENTRY; it.next() ) {
		auto entry = it.child( DBUS_TYPE_DICT_ENTRY );
		const auto* key = entry.string();
		if ( key && std::string_view( key ) == name && entry.next() )
			return entry.child( DBUS_TYPE_VARIANT );
	}
	return it.child( DBUS_TYPE_VARIANT );
}

std::string preference( const Trigger& trigger ) {
	std::string value;
	if ( trigger.modifiers & KEYMOD_CTRL )
		value += "CTRL+";
	if ( trigger.modifiers & KEYMOD_ALT )
		value += "ALT+";
	if ( trigger.modifiers & KEYMOD_SHIFT )
		value += "SHIFT+";
	if ( trigger.modifiers & KEYMOD_META )
		value += "LOGO+";
	const auto key = trigger.key;
	if ( ( key >= KEY_A && key <= KEY_Z ) || ( key >= KEY_0 && key <= KEY_9 ) ) {
		value += static_cast<char>( key );
	} else if ( key >= KEY_F1 && key <= KEY_F12 ) {
		value += 'F';
		value += std::to_string( key - KEY_F1 + 1 );
	} else {
		switch ( key ) {
			case KEY_UP:
				value += "Up";
				break;
			case KEY_DOWN:
				value += "Down";
				break;
			case KEY_LEFT:
				value += "Left";
				break;
			case KEY_RIGHT:
				value += "Right";
				break;
			case KEY_HOME:
				value += "Home";
				break;
			case KEY_END:
				value += "End";
				break;
			case KEY_PAGEUP:
				value += "Page_Up";
				break;
			case KEY_PAGEDOWN:
				value += "Page_Down";
				break;
			case KEY_INSERT:
				value += "Insert";
				break;
			case KEY_DELETE:
				value += "Delete";
				break;
			case KEY_RETURN:
				value += "Return";
				break;
			case KEY_ESCAPE:
				value += "Escape";
				break;
			case KEY_SPACE:
				value += "space";
				break;
			case KEY_TAB:
				value += "Tab";
				break;
			case KEY_BACKSPACE:
				value += "BackSpace";
				break;
			default:
				return {};
		}
	}
	return value;
}

class PortalShortcutBackend final : public ShortcutBackend {
  public:
	explicit PortalShortcutBackend( Options options ) : mOptions( std::move( options ) ) {}

	Backend type() const override { return Backend::WaylandPortal; }

	Result<Capabilities, Error> open() override {
		return Ok( Capabilities{ false, true, false, false } );
	}

	void reconcile( Client& client, Uint64 request, std::vector<Shortcut> shortcuts,
					bool retry ) override;

	void cancel( Client& client, Uint64 request ) override;

	void configure( std::string activationToken, Resolver resolve ) override;

	void wait( Client& client ) override;

	void wake() noexcept override { mBus.wake(); }

	Result<void, Error> close() override;

  private:
	DbusMessage desktop( const char* interface, const char* method ) const;

	bool busMethod( const char* method, Stage stage, auto append );

	bool send( DbusMessage message, Stage stage );

	bool request( Stage stage );

	bool closeSession();

	void restart( Client& client, Error error );

	void reset();

	void finish( Client& client );

	void fail( Client& client, Error error );

	void handleReply( Client& client, DbusMessage message );

	void handleSignal( Client& client, DbusMessage message );

	void handleResponse( Client& client, DbusMessage message );

	std::optional<Bindings> bindings( DbusReader reader );

	void publish( Client& client, Bindings bindings );

	Options mOptions;
	DbusConnection mBus;
	std::string mOwner;
	std::string mPeer;
	std::string mRequestPath;
	std::string mExpectedSession;
	std::string mSession;
	std::vector<Shortcut> mDesired;
	Bindings mBindings;
	DbusMessage mEarlyResponse;
	std::optional<Error> mRegistrationError;
	std::optional<Resolver> mConfigure;
	Uint64 mRequest{ 0 };
	Uint64 mNextIncarnation{ 0 };
	Uint32 mVersion{ 0 };
	Stage mStage{ Stage::Idle };
	bool mMethodReplied{ false };
	bool mStartingPortal{ false };
	bool mRefreshBindings{ false };
};

DbusMessage PortalShortcutBackend::desktop( const char* interface, const char* method ) const {
	return mBus.method( mOwner.c_str(), DesktopPath, interface, method );
}

bool PortalShortcutBackend::send( DbusMessage message, Stage stage ) {
	if ( !mBus.call( std::move( message ) ) )
		return false;
	mStage = stage;
	return true;
}

bool PortalShortcutBackend::busMethod( const char* method, Stage stage, auto append ) {
	auto message = mBus.method( DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, method );
	if ( !message )
		return false;
	auto writer = message.write();
	return append( writer ) && send( std::move( message ), stage );
}

bool PortalShortcutBackend::request( Stage stage ) {
	const auto handle = token();
	if ( !handle )
		return false;
	mRequestPath = "/org/freedesktop/portal/desktop/request/" + mPeer + "/" + *handle;
	mEarlyResponse = {};
	mMethodReplied = false;
	const char* method = stage == Stage::Create ? "CreateSession"
						 : stage == Stage::Bind ? "BindShortcuts"
												: "ListShortcuts";
	auto message = desktop( Shortcuts, method );
	if ( !message )
		return false;
	auto writer = message.write();
	if ( stage != Stage::Create && !writer.string( mSession.c_str(), DBUS_TYPE_OBJECT_PATH ) )
		return false;
	if ( stage == Stage::Bind ) {
		if ( !writer.container(
				 DBUS_TYPE_ARRAY, "(sa{sv})",
				 [&]( DbusWriter& array ) {
					 for ( const auto& shortcut : mDesired ) {
						 if ( !array.container(
								  DBUS_TYPE_STRUCT, nullptr, [&]( DbusWriter& tuple ) {
									  return tuple.string( shortcut.id.c_str() ) &&
											 dictionary( tuple, [&]( DbusWriter& options ) {
												 if ( !option( options, "description",
															   shortcut.description.c_str() ) )
													 return false;
												 if ( !shortcut.preferred )
													 return true;
												 const auto trigger =
													 preference( *shortcut.preferred );
												 return !trigger.empty() &&
														option( options, "preferred_trigger",
																trigger.c_str() );
											 } );
								  } ) )
							 return false;
					 }
					 return true;
				 } ) ||
			 !writer.string( "" ) )
			return false;
	}
	if ( !dictionary( writer, [&]( DbusWriter& options ) {
			 if ( !option( options, "handle_token", handle->c_str() ) )
				 return false;
			 if ( stage != Stage::Create )
				 return true;
			 const auto session = token();
			 if ( !session )
				 return false;
			 mExpectedSession = "/org/freedesktop/portal/desktop/session/" + mPeer + "/" + *session;
			 return option( options, "session_handle_token", session->c_str() );
		 } ) )
		return false;
	return send( std::move( message ), stage );
}

bool PortalShortcutBackend::closeSession() {
	return send( mBus.method( mOwner.c_str(), mSession.c_str(), Session, "Close" ),
				 Stage::CloseSession );
}

void PortalShortcutBackend::reconcile( Client& client, Uint64 id, std::vector<Shortcut> shortcuts,
									   bool retry ) {
	mRequest = id;
	size_t bytes = 0;
	if ( shortcuts.size() > MaxShortcuts ||
		 std::any_of( shortcuts.begin(), shortcuts.end(), [&]( const Shortcut& s ) {
			 if ( s.id.size() > MaxText || s.description.size() > MaxText )
				 return true;
			 bytes += s.id.size() + s.description.size();
			 return bytes > MaxDeclarationText;
		 } ) ) {
		mRequest = 0;
		client.complete( id, Err( failure( "Shortcut declaration exceeds portal limits",
										   ErrorCode::InvalidShortcut ) ) );
		return;
	}
	if ( !retry && ( !mSession.empty() || mRegistrationError ) &&
		 std::equal( shortcuts.begin(), shortcuts.end(), mDesired.begin(), mDesired.end(),
					 []( const Shortcut& a, const Shortcut& b ) {
						 return a.id == b.id && a.preferred == b.preferred;
					 } ) ) {
		mDesired = std::move( shortcuts );
		if ( mRegistrationError ) {
			mRequest = 0;
			client.complete( id, Err( *mRegistrationError ) );
		} else {
			finish( client );
		}
		return;
	}
	mDesired = std::move( shortcuts );
	mRegistrationError.reset();
	if ( mConfigure ) {
		auto previous = std::move( *mConfigure );
		mConfigure.reset();
		previous( Err( failure( "Shortcut declaration changed", ErrorCode::Superseded ) ) );
	}
	mBus.cancel();
	mRefreshBindings = false;
	if ( !mSession.empty() ) {
		mBindings.clear();
		client.bindingsChanged( {}, {} );
		if ( closeSession() )
			return;
	} else if ( mDesired.empty() ) {
		finish( client );
		return;
	} else if ( mVersion ) {
		if ( request( Stage::Create ) )
			return;
	} else if ( mBus.open() ) {
		const auto name = mBus.uniqueName();
		if ( !name.empty() && name.front() == ':' ) {
			mPeer = name.substr( 1 );
			std::replace( mPeer.begin(), mPeer.end(), '.', '_' );
			if ( busMethod( "AddMatch", Stage::WatchOwner, []( DbusWriter& writer ) {
					 return writer.string(
						 "type='signal',sender='org.freedesktop.DBus',"
						 "interface='org.freedesktop.DBus',member='NameOwnerChanged',"
						 "arg0='org.freedesktop.portal.Desktop'" );
				 } ) )
				return;
		}
	}
	restart( client, failure( "Could not open the Wayland shortcut portal",
							  ErrorCode::BackendUnavailable ) );
}

void PortalShortcutBackend::cancel( Client& client, Uint64 id ) {
	if ( mRequest != id )
		return;
	mRequest = 0;
	mRegistrationError = failure( "Wayland shortcut request cancelled", ErrorCode::Cancelled );
	// The portal owns requests and sessions by the caller's unique name. Disconnecting
	// also releases a session whose CreateSession reply has not reached this worker.
	reset();
	client.bindingsChanged( {}, {} );
	client.capabilitiesChanged( { false, true, false, false } );
}

void PortalShortcutBackend::configure( std::string activationToken, Resolver resolve ) {
	if ( activationToken.size() > MaxText || activationToken.find( '\0' ) != std::string::npos ) {
		resolve( Err(
			failure( "Invalid shortcut settings activation token", ErrorCode::InvalidShortcut ) ) );
		return;
	}
	if ( mVersion < 2 || mSession.empty() ) {
		resolve(
			Err( failure( "No configurable Wayland shortcut session", ErrorCode::Unsupported ) ) );
		return;
	}
	if ( mStage != Stage::Idle ) {
		resolve( Err( failure( "The Wayland shortcut portal is busy", ErrorCode::Busy ) ) );
		return;
	}
	auto message = desktop( Shortcuts, "ConfigureShortcuts" );
	if ( message ) {
		auto writer = message.write();
		if ( writer.string( mSession.c_str(), DBUS_TYPE_OBJECT_PATH ) && writer.string( "" ) &&
			 dictionary( writer,
						 [&]( DbusWriter& options ) {
							 return activationToken.empty() ||
									option( options, "activation_token", activationToken.c_str() );
						 } ) &&
			 send( std::move( message ), Stage::Configure ) ) {
			mConfigure = std::move( resolve );
			return;
		}
	}
	resolve( Err( failure( "Could not request Wayland shortcut settings" ) ) );
}

void PortalShortcutBackend::wait( Client& client ) {
	const auto step = mBus.wait();
	if ( step == DbusConnection::Step::Failed ) {
		restart( client,
				 failure( "Wayland shortcut portal connection lost", ErrorCode::Disconnected ) );
		return;
	}
	if ( auto signal = mBus.signal() )
		handleSignal( client, std::move( signal ) );
	if ( auto reply = mBus.reply() )
		handleReply( client, std::move( reply ) );
	if ( mRefreshBindings && mStage == Stage::Idle && !mSession.empty() ) {
		mRefreshBindings = false;
		if ( !request( Stage::List ) )
			client.bindingsChanged( mBindings, failure( "Could not refresh Wayland shortcuts" ) );
	}
}

Result<void, Error> PortalShortcutBackend::close() {
	if ( mConfigure ) {
		( *mConfigure )( Err( failure( "Shortcut service is closing", ErrorCode::ShuttingDown ) ) );
		mConfigure.reset();
	}
	if ( !mSession.empty() && mBus.connected() ) {
		mBus.cancel();
		// Prefer an acknowledged Close for a known session. Disconnect also closes the
		// session if its owner disappears or fails to acknowledge it.
		if ( mBus.call( mBus.method( mOwner.c_str(), mSession.c_str(), Session, "Close" ), 500 ) ) {
			while ( !mBus.reply() && mBus.wait() != DbusConnection::Step::Failed )
				mBus.signal();
		}
	}
	reset();
	return Ok();
}

void PortalShortcutBackend::reset() {
	mBus.close();
	mOwner.clear();
	mPeer.clear();
	mSession.clear();
	mExpectedSession.clear();
	mBindings.clear();
	mEarlyResponse = {};
	mRequestPath.clear();
	mStage = Stage::Idle;
	mVersion = 0;
	mStartingPortal = false;
	mRefreshBindings = false;
}

void PortalShortcutBackend::restart( Client& client, Error error ) {
	const bool active = mRequest != 0;
	mRegistrationError = error;
	if ( mConfigure ) {
		( *mConfigure )( Err( error ) );
		mConfigure.reset();
	}
	reset();
	client.capabilitiesChanged( { false, true, false, false } );
	client.bindingsChanged( {}, active ? std::nullopt : std::optional<Error>( error ) );
	if ( active )
		fail( client, std::move( error ) );
}

void PortalShortcutBackend::finish( Client& client ) {
	const auto id = std::exchange( mRequest, 0 );
	client.complete( id, Ok( mBindings ) );
}

void PortalShortcutBackend::fail( Client& client, Error error ) {
	mStage = Stage::Idle;
	const auto id = std::exchange( mRequest, 0 );
	if ( id )
		client.complete( id, Err( std::move( error ) ) );
	else
		client.bindingsChanged( mBindings, std::move( error ) );
}

void PortalShortcutBackend::handleReply( Client& client, DbusMessage message ) {
	const auto stage = mStage;
	const auto name = message.error();
	const bool busStage = stage == Stage::WatchOwner || stage == Stage::FindOwner ||
						  stage == Stage::StartPortal || stage == Stage::WatchPortal;
	const auto sender = message.sender();
	if ( sender !=
			 ( busStage ? std::string_view( DBUS_SERVICE_DBUS ) : std::string_view( mOwner ) ) &&
		 !( !name.empty() && ( sender == DBUS_SERVICE_DBUS || sender.empty() ) ) ) {
		restart( client, failure( "Unexpected sender for a shortcut portal reply" ) );
		return;
	}

	if ( stage == Stage::Configure ) {
		mStage = Stage::Idle;
		auto resolver = std::move( *mConfigure );
		mConfigure.reset();
		if ( !name.empty() )
			resolver( Err( dbusError( name ) ) );
		else if ( !message.signature( "" ) )
			resolver( Err( failure( "Invalid ConfigureShortcuts reply" ) ) );
		else
			resolver( Ok() );
		return;
	}
	if ( stage == Stage::FindOwner && name == DBUS_ERROR_NAME_HAS_NO_OWNER && !mStartingPortal ) {
		mStartingPortal = true;
		if ( busMethod( "StartServiceByName", Stage::StartPortal,
						[]( DbusWriter& w ) { return w.string( Portal ) && w.write( 0 ); } ) )
			return;
	}
	if ( !name.empty() &&
		 !( stage == Stage::RegisterApp &&
			( name == DBUS_ERROR_UNKNOWN_INTERFACE || name == DBUS_ERROR_UNKNOWN_METHOD ) ) ) {
		auto error = dbusError( name );
		if ( stage == Stage::Version && name == DBUS_ERROR_INVALID_ARGS )
			error.code = ErrorCode::Unsupported;
		restart( client, std::move( error ) );
		return;
	}

	bool sent = false;
	switch ( stage ) {
		case Stage::WatchOwner:
		case Stage::StartPortal:
			if ( !message.signature( stage == Stage::WatchOwner ? "" : "u" ) )
				break;
			sent = busMethod( "GetNameOwner", Stage::FindOwner,
							  []( DbusWriter& writer ) { return writer.string( Portal ); } );
			break;
		case Stage::FindOwner: {
			const auto* owner = message.signature( "s" ) ? message.read().string() : nullptr;
			if ( !owner || *owner != ':' )
				break;
			mOwner = owner;
			if ( !mOptions.appId.empty() ) {
				auto call = desktop( Registry, "Register" );
				if ( call ) {
					auto writer = call.write();
					sent = writer.string( mOptions.appId.c_str() ) &&
						   dictionary( writer, []( DbusWriter& ) { return true; } ) &&
						   send( std::move( call ), Stage::RegisterApp );
				}
				break;
			}
			[[fallthrough]];
		}
		case Stage::RegisterApp: {
			if ( stage == Stage::RegisterApp && name.empty() && !message.signature( "" ) )
				break;
			auto call = desktop( DBUS_INTERFACE_PROPERTIES, "Get" );
			if ( call ) {
				auto writer = call.write();
				sent = writer.string( Shortcuts ) && writer.string( "version" ) &&
					   send( std::move( call ), Stage::Version );
			}
			break;
		}
		case Stage::Version: {
			if ( !message.signature( "v" ) ||
				 !message.read().child( DBUS_TYPE_VARIANT ).read( mVersion ) || !mVersion ) {
				restart( client, failure( "The desktop exposes no Global Shortcuts portal",
										  ErrorCode::Unsupported ) );
				return;
			}
			const auto rule = "type='signal',sender='" + mOwner +
							  "',path_namespace='/org/freedesktop/portal/desktop'";
			sent = busMethod( "AddMatch", Stage::WatchPortal,
							  [&]( DbusWriter& writer ) { return writer.string( rule.c_str() ); } );
			break;
		}
		case Stage::WatchPortal:
			if ( !message.signature( "" ) )
				break;
			sent = request( Stage::Create );
			break;
		case Stage::CloseSession:
			if ( !message.signature( "" ) )
				break;
			mSession.clear();
			mStage = Stage::Idle;
			client.capabilitiesChanged( { true, true, true, false } );
			if ( mDesired.empty() ) {
				finish( client );
				return;
			}
			sent = request( Stage::Create );
			break;
		case Stage::Create:
		case Stage::Bind:
		case Stage::List: {
			const auto* path =
				message.signature( "o" ) ? message.read().string( DBUS_TYPE_OBJECT_PATH ) : nullptr;
			if ( !path || !std::string_view( path ).starts_with(
							  "/org/freedesktop/portal/desktop/request/" ) )
				break;
			mRequestPath = path;
			mMethodReplied = true;
			if ( mEarlyResponse && mEarlyResponse.path() == mRequestPath )
				handleResponse( client, std::move( mEarlyResponse ) );
			else
				mEarlyResponse = {};
			return;
		}
		default:
			return;
	}
	if ( !sent )
		restart( client, failure( "Could not advance the Wayland shortcut portal request" ) );
}

void PortalShortcutBackend::handleSignal( Client& client, DbusMessage message ) {
	if ( message.sender() == DBUS_SERVICE_DBUS && message.path() == DBUS_PATH_DBUS &&
		 message.signal( DBUS_INTERFACE_DBUS, "NameOwnerChanged" ) && message.signature( "sss" ) ) {
		auto reader = message.read();
		const auto* name = reader.string();
		if ( name && std::string_view( name ) == Portal && reader.next() ) {
			const auto* old = reader.string();
			if ( old && !mOwner.empty() && old == mOwner )
				restart( client, failure( "The Wayland shortcut portal was replaced",
										  ErrorCode::Disconnected ) );
		}
		return;
	}
	if ( mOwner.empty() || message.sender() != mOwner )
		return;
	if ( message.signal( Request, "Response" ) && message.signature( "ua{sv}" ) &&
		 ( mStage == Stage::Create || mStage == Stage::Bind || mStage == Stage::List ) ) {
		if ( mMethodReplied && message.path() == mRequestPath ) {
			handleResponse( client, std::move( message ) );
		} else if ( !mMethodReplied &&
					message.path().starts_with( "/org/freedesktop/portal/desktop/request/" ) &&
					( !mEarlyResponse || message.path() == mRequestPath ) ) {
			mEarlyResponse = std::move( message );
		}
		return;
	}
	if ( !mSession.empty() && message.path() == mSession && message.signal( Session, "Closed" ) &&
		 message.signature( "a{sv}" ) ) {
		if ( mStage != Stage::CloseSession )
			restart( client,
					 failure( "The Wayland shortcut session closed", ErrorCode::Disconnected ) );
		return;
	}
	if ( message.path() != DesktopPath || mSession.empty() )
		return;
	auto reader = message.read();
	const auto* session = reader.string( DBUS_TYPE_OBJECT_PATH );
	if ( !session || session != mSession || !reader.next() )
		return;
	if ( message.signal( Shortcuts, "ShortcutsChanged" ) && message.signature( "oa(sa{sv})" ) ) {
		mRefreshBindings = true;
	} else if ( message.signal( Shortcuts, "Activated" ) && message.signature( "osta{sv}" ) ) {
		const auto* id = reader.string();
		if ( !id )
			return;
		const auto it =
			std::find_if( mBindings.begin(), mBindings.end(),
						  [&]( const Binding& binding ) { return binding.shortcut.id == id; } );
		if ( it == mBindings.end() || !reader.next() || !reader.next() )
			return;
		const auto* value = property( reader, "activation_token" ).string();
		client.activate( it->incarnation, value && std::string_view( value ).size() <= MaxText
											  ? std::optional<std::string>( value )
											  : std::nullopt );
	}
}

void PortalShortcutBackend::handleResponse( Client& client, DbusMessage message ) {
	const auto stage = mStage;
	mStage = Stage::Idle;
	mRequestPath.clear();
	mEarlyResponse = {};
	auto reader = message.read();
	Uint32 code = 2;
	reader.read( code );
	reader.next();
	if ( code != 0 ) {
		auto error = failure( code == 1 ? "Wayland shortcut request cancelled"
										: "Wayland shortcut request rejected",
							  code == 1 ? ErrorCode::Cancelled : ErrorCode::NativeFailure );
		if ( stage == Stage::List )
			fail( client, std::move( error ) );
		else
			restart( client, std::move( error ) );
		return;
	}
	if ( stage == Stage::Create ) {
		auto value = property( reader, "session_handle" );
		const auto* path = value.string();
		if ( !path )
			path = value.string( DBUS_TYPE_OBJECT_PATH );
		if ( !path || path != mExpectedSession ) {
			restart( client, failure( "Portal returned an invalid shortcut session handle" ) );
			return;
		}
		mSession = path;
		client.capabilitiesChanged( { true, true, true, false } );
		if ( !request( Stage::Bind ) )
			restart( client, failure( "Could not request Wayland shortcut registration" ) );
		return;
	}
	if ( auto effective = bindings( property( reader, "shortcuts" ) ) ) {
		publish( client, std::move( *effective ) );
		if ( stage == Stage::Bind ) {
			client.capabilitiesChanged( { true, true, true, mVersion >= 2 } );
			finish( client );
		}
	} else if ( stage == Stage::List ) {
		fail( client, failure( "Invalid Wayland shortcut list" ) );
	} else {
		restart( client, failure( "Invalid Wayland shortcut binding result" ) );
	}
}

std::optional<Bindings> PortalShortcutBackend::bindings( DbusReader reader ) {
	if ( reader.elementType() != DBUS_TYPE_STRUCT )
		return {};
	Bindings result;
	unsigned count = 0;
	for ( auto entries = reader.child( DBUS_TYPE_ARRAY ); entries.type() != DBUS_TYPE_INVALID;
		  entries.next() ) {
		auto entry = entries.child( DBUS_TYPE_STRUCT );
		const auto* id = entry.string();
		if ( !id || ++count > MaxShortcuts || !entry.next() ||
			 entry.elementType() != DBUS_TYPE_DICT_ENTRY )
			return {};
		const auto desired =
			std::lower_bound( mDesired.begin(), mDesired.end(), std::string_view( id ),
							  []( const Shortcut& a, std::string_view b ) { return a.id < b; } );
		if ( desired == mDesired.end() || desired->id != id )
			continue;
		if ( std::any_of( result.begin(), result.end(),
						  [&]( const Binding& b ) { return b.shortcut.id == id; } ) )
			return {};
		const auto* description = property( entry, "trigger_description" ).string();
		const auto text = description ? std::string_view( description ) : std::string_view{};
		if ( text.size() > MaxText )
			return {};
		const auto previous =
			std::find_if( mBindings.begin(), mBindings.end(), [&]( const Binding& b ) {
				return b.shortcut.id == id && b.shortcut.triggerDescription == text;
			} );
		const auto incarnation =
			previous == mBindings.end() ? ++mNextIncarnation : previous->incarnation;
		result.push_back(
			{ { id, std::string( text ), {}, {}, BindingStatus::Bound }, incarnation } );
	}
	return result;
}

void PortalShortcutBackend::publish( Client& client, Bindings bindings ) {
	mBindings = std::move( bindings );
	if ( !mRequest )
		client.bindingsChanged( mBindings, {} );
}

} // namespace

bool isWaylandShortcutSession() {
	for ( const char* key : { "SDL_VIDEO_DRIVER", "SDL_VIDEODRIVER" } ) {
		const auto* value = std::getenv( key );
		if ( value &&
			 ( std::string_view( value ) == "dummy" || std::string_view( value ) == "offscreen" ) )
			return false;
	}
	for ( const char* key : { "WAYLAND_DISPLAY", "WAYLAND_SOCKET", "XDG_SESSION_TYPE",
							  "SDL_VIDEO_DRIVER", "SDL_VIDEODRIVER" } ) {
		const auto* value = std::getenv( key );
		if ( !value || !*value )
			continue;
		const auto name = std::string_view( key );
		if ( name == "WAYLAND_DISPLAY" || name == "WAYLAND_SOCKET" ||
			 std::string_view( value ) == "wayland" )
			return true;
	}
	return false;
}

std::unique_ptr<ShortcutBackend> createPortalShortcutBackend( Options options ) {
	return std::make_unique<PortalShortcutBackend>( std::move( options ) );
}

} // namespace EE::System::GlobalShortcuts::Private

#endif
