#include "shortcuttest.hpp"
#include <eepp/system/dbus/dbus.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

using namespace EE;
using namespace EE::System::GlobalShortcuts;
using namespace EE::System::GlobalShortcuts::Test;
using namespace EE::Window;

namespace {

const Shortcut toggle{ "toggle", "Toggle eterm", Trigger{ KEY_F1, KEYMOD_ALT } };
const Shortcut secondary{ "second", "Another shortcut", {} };

ServiceSnapshot waitForState( Service& service, DeferredExecutor& executor, auto predicate ) {
	const auto deadline = std::chrono::steady_clock::now() + TestDeadline;
	for ( ;; ) {
		auto snapshot = service.snapshot();
		if ( predicate( snapshot ) )
			return snapshot;
		expect( std::chrono::steady_clock::now() < deadline && executor.wait(),
				"Timed out waiting for portal state" );
		executor.drain();
	}
}

void testTransport() {
	using namespace EE::System::Dbus;
	DbusConnection a;
	DbusConnection b;
	expect( a.open() && b.open() && a.uniqueName() != b.uniqueName(),
			"Private bus names must differ" );
	auto pending = a.method( "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
							 "org.freedesktop.impl.eepp.Test", "Hang" );
	expect( a.call( std::move( pending ), 120 ), "Could not start timeout test call" );
	DbusMessage reply;
	while ( !( reply = a.reply() ) )
		expect( a.wait() != DbusConnection::Step::Failed, "D-Bus wait failed" );
	expect( reply.error() == DBUS_ERROR_NO_REPLY, "Expected a method timeout" );

	pending = a.method( "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
						"org.freedesktop.impl.eepp.Test", "Hang" );
	expect( a.call( std::move( pending ) ), "Could not start cancellation call" );
	a.cancel();
	a.close();
	const auto name = std::string( b.uniqueName() );
	auto echo = b.method( DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "NameHasOwner" );
	auto writer = echo.write();
	expect( writer.string( name.c_str() ) && b.call( std::move( echo ) ),
			"Could not query second connection" );
	while ( !( reply = b.reply() ) )
		expect( b.wait() != DbusConnection::Step::Failed, "Second D-Bus connection failed" );
	b.close();
	expect( reply.signature( "b" ), "Reply did not survive connection teardown" );
}

void waitForMarker( const std::filesystem::path& path ) {
	const auto deadline = std::chrono::steady_clock::now() + TestDeadline;
	while ( !std::ifstream( path ).good() ) {
		expect( std::chrono::steady_clock::now() < deadline,
				"Mock portal did not reach its checkpoint" );
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	}
}

void testPortal( const std::filesystem::path& marker ) {
	DeferredExecutor executor;
	Service service( executor, Options{ "org.eepp.PortalTest" } );
	std::vector<Activation> activated;
	service.setActivationCallback( [&]( const auto& value ) { activated.push_back( value ); } );
	service.setStateChangedCallback( []( const auto& ) {} );
	auto registered = registration( service.setShortcuts( { toggle, secondary } ) );
	expect( registered.backend == Backend::WaylandPortal &&
				registered.capabilities.canConfigureShortcuts,
			"Expected a v2 shortcut portal" );
	const auto& entry = binding( registered, "toggle" );
	expect( registered.state == State::Degraded && entry.status == BindingStatus::Bound &&
				entry.triggerDescription == "Alt+F1" && !entry.effectiveTrigger &&
				binding( registered, "second" ).status == BindingStatus::Unbound,
			"Portal should report the grants and compositor descriptions" );

	auto configured = await( service.configureShortcuts( "settings-token" ) );
	expect( static_cast<bool>( configured ), "Portal configuration request failed" );
	waitForState( service, executor, []( const auto& snapshot ) {
		return binding( snapshot, "toggle" ).triggerDescription == "Alt+F2";
	} );
	const auto deadline = std::chrono::steady_clock::now() + TestDeadline;
	while ( activated.empty() ) {
		expect( std::chrono::steady_clock::now() < deadline && executor.wait(),
				"Did not receive an activation from the portal" );
		executor.drain();
	}
	expect( activated.size() == 1 && activated[0].id == "toggle" &&
				activated[0].activationToken == "window-token",
			"Portal activation lost its ID or token" );

	auto renamed = toggle;
	renamed.description = "Localized toggle";
	expect( binding( registration( service.setShortcuts( { renamed, secondary } ) ), "toggle" )
					.triggerDescription == "Alt+F2",
			"Metadata edit recreated the portal session" );

	CancellationSource cancel;
	auto pending =
		service.setShortcuts( { { "hold", "Keep request pending", {} } }, cancel.token() );
	waitForMarker( marker );
	expect( !pending.isReady(), "Consent finished without a Response signal" );
	cancel.cancel();
	auto cancelled = await( std::move( pending ) );
	expect( !cancelled && cancelled.error().code == ErrorCode::Cancelled &&
				service.snapshot().desired.front().id == "hold",
			"Portal cancellation lost the desired set" );
	auto localized = await( service.setShortcuts( { { "hold", "Description changed", {} } } ) );
	expect( !localized && localized.error().code == ErrorCode::Cancelled,
			"A description change reopened the cancelled request" );

	expect( binding( registration( service.setShortcuts( { toggle } ) ), "toggle" ).status ==
				BindingStatus::Bound,
			"Could not register again after cancellation" );
	expect( static_cast<bool>( await( service.configureShortcuts( "replace-owner" ) ) ),
			"Could not trigger portal replacement" );
	auto revoked = waitForState( service, executor, []( const auto& snapshot ) {
		return snapshot.lastBackendIssue &&
			   snapshot.lastBackendIssue->code == ErrorCode::Disconnected;
	} );
	expect( binding( revoked, "toggle" ).status == BindingStatus::Unbound &&
				!revoked.capabilities.canConfigureShortcuts,
			"Portal owner loss did not revoke the live session" );
	waitForMarker( marker.parent_path() / "owner-replaced" );
	expect( binding( registration( service.retry() ), "toggle" ).status == BindingStatus::Bound,
			"Could not retry on the replacement portal" );
	executor.drain();
	if ( executor.wait( std::chrono::milliseconds( 150 ) ) )
		executor.drain();
	expect( activated.size() == 1, "The old portal owner delivered an activation" );
	expect( registration( service.clear() ).state == State::Idle,
			"Clear did not close the portal session" );
	const auto closeMarker = marker.parent_path() / "active-close";
	std::filesystem::remove( closeMarker );
	expect( binding( registration( service.setShortcuts( { toggle } ) ), "toggle" ).status ==
				BindingStatus::Bound,
			"Could not create a session after clear" );
	expect( static_cast<bool>( await( service.closeAsync() ) ), "Portal service did not close" );
	waitForMarker( closeMarker );
}

void probe( std::string_view mode, const char* appId ) {
	DeferredExecutor executor;
	Service service( executor, Options{ appId } );
	bool activated = false;
	service.setActivationCallback(
		[&]( const Activation& activation ) { activated = activation.id == toggle.id; } );
	CancellationSource cancel;
	auto shortcut = toggle;
	if ( mode == "--interactive" )
		shortcut.preferred = Trigger{ KEY_F11, KEYMOD_CTRL | KEYMOD_SHIFT };
	auto pending = service.setShortcuts( { std::move( shortcut ) }, cancel.token() );
	if ( mode == "--unsupported" ) {
		auto response = await( std::move( pending ) );
		expect( !response && response.error().code == ErrorCode::Unsupported &&
					service.snapshot().state == State::Unavailable,
				"Expected unsupported portal interface" );
	} else if ( mode == "--interactive" ) {
		expect( pending.waitFor( std::chrono::seconds( 90 ) ) == TaskState::Ready,
				"Timed out waiting for manual portal consent" );
		auto report = registration( std::move( pending ) );
		const auto& shortcut = binding( report, toggle.id );
		expect( shortcut.status == BindingStatus::Bound, "The shortcut was not granted" );
		std::cout << "READY " << shortcut.triggerDescription << std::endl;
		expect( executor.wait( std::chrono::seconds( 90 ) ),
				"Timed out waiting for shortcut input" );
		executor.drain();
		expect( activated, "The portal did not activate the toggle command" );
		expect( registration( service.clear() ).state == State::Idle,
				"Could not release manual shortcut" );
	} else {
		const bool completed = pending.waitFor( std::chrono::seconds( 2 ) ) == TaskState::Ready;
		if ( !completed )
			cancel.cancel();
		auto response = await( std::move( pending ) );
		if ( !response && ( completed || response.error().code != ErrorCode::Cancelled ) ) {
			const auto& error = response.error();
			throw std::runtime_error( error.message + " " + error.nativeName );
		}
		std::cout << ( completed
						   ? "Real portal returned a completed registration report\n"
						   : "Real portal registration stayed pending and cancelled cleanly\n" );
	}
	expect( static_cast<bool>( await( service.closeAsync() ) ), "Probe close failed" );
}

} // namespace

int main( int argc, char** argv ) {
	try {
		if ( argc > 1 && ( std::string_view( argv[1] ) == "--unsupported" ||
						   std::string_view( argv[1] ) == "--probe" ||
						   std::string_view( argv[1] ) == "--interactive" ) ) {
			probe( argv[1], argc == 3 ? argv[2] : "" );
		} else {
			expect( argc == 2, "Supply the mock portal's consent marker filename" );
			testTransport();
			testPortal( argv[1] );
		}
		std::cout << "Wayland shortcut checks passed\n";
	} catch ( const std::exception& error ) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
