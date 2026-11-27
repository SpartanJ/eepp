#include "shortcuttest.hpp"
#include <cstdlib>
#include <iostream>
#include <xcb/xtest.h>
#include <xkbcommon/xkbcommon-x11.h>

#define explicit explicit_
#include <xcb/xkb.h>
#undef explicit

using namespace EE;
using namespace EE::System::GlobalShortcuts;
using namespace EE::System::GlobalShortcuts::Test;
using namespace EE::Window;

namespace {

constexpr Uint32 Modifiers = KEYMOD_CTRL | KEYMOD_SHIFT;
constexpr Uint16 NativeModifiers = XCB_MOD_MASK_CONTROL | XCB_MOD_MASK_SHIFT;

class TestDisplay {
  public:
	TestDisplay() {
		expect( !xcb_connection_has_error( mConnection.get() ), "Could not open the test display" );
		expect( xkb_x11_setup_xkb_extension( mConnection.get(), 1, 0,
											 XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr,
											 nullptr, nullptr ),
				"The test display requires XKB" );
		for ( auto screen = xcb_setup_roots_iterator( xcb_get_setup( mConnection.get() ) );
			  screen.rem; xcb_screen_next( &screen ) )
			roots.push_back( screen.data->root );
		std::sort( roots.begin(), roots.end() );
	}

	xcb_keycode_t code( xkb_keysym_t symbol ) {
		const auto device = xkb_x11_get_core_keyboard_device_id( mConnection.get() );
		std::unique_ptr<xkb_context, decltype( &xkb_context_unref )> context(
			xkb_context_new( XKB_CONTEXT_NO_FLAGS ), &xkb_context_unref );
		std::unique_ptr<xkb_keymap, decltype( &xkb_keymap_unref )> map(
			xkb_x11_keymap_new_from_device( context.get(), mConnection.get(), device,
											XKB_KEYMAP_COMPILE_NO_FLAGS ),
			&xkb_keymap_unref );
		expect( static_cast<bool>( map ), "Could not read the test XKB layout" );
		for ( auto key = xkb_keymap_min_keycode( map.get() );
			  key <= xkb_keymap_max_keycode( map.get() ); ++key ) {
			const xkb_keysym_t* symbols;
			if ( xkb_keymap_key_get_syms_by_level( map.get(), key, 0, 0, &symbols ) == 1 &&
				 symbols[0] == symbol )
				return static_cast<xcb_keycode_t>( key );
		}
		throw std::runtime_error( "A test key is missing from the base XKB layout" );
	}

	Uint16 lockMask( xcb_keycode_t code ) {
		std::unique_ptr<xcb_get_modifier_mapping_reply_t, decltype( &std::free )> map(
			xcb_get_modifier_mapping_reply(
				mConnection.get(), xcb_get_modifier_mapping( mConnection.get() ), nullptr ),
			&std::free );
		expect( static_cast<bool>( map ), "Could not read test modifier map" );
		const auto* keys = xcb_get_modifier_mapping_keycodes( map.get() );
		for ( unsigned bit = 0; bit < 8; ++bit ) {
			const auto* end = keys + map->keycodes_per_modifier;
			if ( std::find( keys, end, code ) != end )
				return 1U << bit;
			keys = end;
		}
		throw std::runtime_error( "A test lock key has no native modifier" );
	}

	unsigned grab( xcb_window_t root, xcb_keycode_t code, Uint16 modifiers ) {
		return check( xcb_grab_key_checked( mConnection.get(), 0, root, modifiers, code,
											XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC ) );
	}

	void ungrab( xcb_window_t root, xcb_keycode_t code, Uint16 modifiers ) {
		expect( !check( xcb_ungrab_key_checked( mConnection.get(), code, root, modifiers ) ),
				"Test X11 ungrab failed" );
	}

	void focus( xcb_window_t root ) {
		expect( !check( xcb_warp_pointer_checked( mConnection.get(), XCB_NONE, root, 0, 0, 0, 0, 20,
												  20 ) ),
				"Could not move the pointer to the test screen" );
		expect( !check( xcb_set_input_focus_checked(
					mConnection.get(), XCB_INPUT_FOCUS_POINTER_ROOT, root, XCB_CURRENT_TIME ) ),
				"Could not focus the test screen" );
	}

	void key( xcb_keycode_t key, bool down ) {
		expect( !check( xcb_test_fake_input_checked( mConnection.get(),
													 down ? XCB_KEY_PRESS : XCB_KEY_RELEASE, key,
													 XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0 ) ),
				"XTEST keyboard injection failed" );
	}

	void type( xcb_keycode_t keyCode ) {
		key( keyCode, true );
		key( keyCode, false );
	}

	void hotkey( xcb_keycode_t keyCode ) {
		key( control, true );
		key( shift, true );
		type( keyCode );
		key( shift, false );
		key( control, false );
	}

	void group( Uint8 group ) {
		expect( !check( xcb_xkb_latch_lock_state_checked(
					mConnection.get(), XCB_XKB_ID_USE_CORE_KBD, 0, 0, 1, group, 0, 0, 0 ) ),
				"Could not switch the test XKB group" );
	}

	void remap( xcb_keycode_t key, xcb_keysym_t symbol ) {
		expect(
			!check( xcb_change_keyboard_mapping_checked( mConnection.get(), 1, key, 1, &symbol ) ),
			"Could not change the test XKB keymap" );
	}

	bool repeats( xcb_keycode_t key ) {
		std::unique_ptr<xcb_get_keyboard_control_reply_t, decltype( &std::free )> controls(
			xcb_get_keyboard_control_reply(
				mConnection.get(), xcb_get_keyboard_control( mConnection.get() ), nullptr ),
			&std::free );
		return controls && controls->global_auto_repeat == XCB_AUTO_REPEAT_MODE_ON &&
			   ( controls->auto_repeats[key / 8] & ( 1U << ( key % 8 ) ) );
	}

	std::vector<xcb_window_t> roots;
	xcb_keycode_t control{ 0 };
	xcb_keycode_t shift{ 0 };

  private:
	unsigned check( xcb_void_cookie_t cookie ) {
		std::unique_ptr<xcb_generic_error_t, decltype( &std::free )> error(
			xcb_request_check( mConnection.get(), cookie ), &std::free );
		return error ? error->error_code : 0;
	}

	std::unique_ptr<xcb_connection_t, decltype( &xcb_disconnect )> mConnection{
		xcb_connect( nullptr, nullptr ), &xcb_disconnect };
};

Shortcut shortcut( std::string id, Keycode key ) {
	return { std::move( id ), "X11 integration test", Trigger{ key, Modifiers } };
}

void expectBound( const ServiceSnapshot& snapshot, std::string_view id, Keycode key ) {
	const auto& result = binding( snapshot, id );
	if ( result.status != BindingStatus::Bound ||
		 result.effectiveTrigger != Trigger{ key, Modifiers } ) {
		throw std::runtime_error( "Expected a confirmed X11 hotkey for " + std::string( id ) +
								  ( result.error ? " (" + result.error->message + ")" : "" ) );
	}
}

void waitForActivation( DeferredExecutor& executor ) {
	expect( executor.wait(), "Timed out waiting for an X11 hotkey activation" );
	executor.drain();
}

ServiceSnapshot waitForState( Service& service, DeferredExecutor& executor, auto predicate ) {
	const auto deadline = std::chrono::steady_clock::now() + TestDeadline;
	auto snapshot = service.snapshot();
	while ( !predicate( snapshot ) ) {
		expect( std::chrono::steady_clock::now() < deadline && executor.wait(),
				"Timed out waiting for the X11 shortcut state" );
		executor.drain();
		snapshot = service.snapshot();
	}
	executor.drain();
	return snapshot;
}

void testGrabs( TestDisplay& display ) {
	DeferredExecutor executor;
	Service service( executor );
	std::vector<std::string> activations;
	service.setActivationCallback(
		[&]( const Activation& activation ) { activations.push_back( activation.id ); } );
	expect( registration( service.clear() ).backend == Backend::None,
			"Empty reconciliation opened an X11 worker" );
	auto first = shortcut( "first", KEY_F6 );
	auto next = shortcut( first.id, KEY_F7 );
	auto independent = shortcut( "independent", KEY_F8 );
	const auto f6 = display.code( XKB_KEY_F6 );
	const auto f7 = display.code( XKB_KEY_F7 );
	const auto f8 = display.code( XKB_KEY_F8 );
	const auto caps = display.code( XKB_KEY_Caps_Lock );
	const auto num = display.code( XKB_KEY_Num_Lock );
	const auto locks = display.lockMask( caps ) | display.lockMask( num );
	auto initial = registration( service.setShortcuts( { first } ) );
	expect( initial.backend == Backend::X11 && initial.capabilities.desktopWide,
			"Expected the X11 backend and full display coverage" );
	expectBound( initial, first.id, KEY_F6 );

	expect( !display.grab( display.roots.back(), f7, NativeModifiers | locks ),
			"Could not reserve the contended lock variant" );
	auto blocked = registration( service.setShortcuts( { next, independent } ) );
	expectBound( blocked, first.id, KEY_F6 );
	expectBound( blocked, independent.id, KEY_F8 );
	const auto& error = binding( blocked, first.id ).error;
	expect( blocked.state == State::Degraded && error && error->code == ErrorCode::Conflict &&
				error->nativeCode == XCB_ACCESS,
			"Expected BadAccess and a retained binding after partial failure" );
	for ( auto root : display.roots ) {
		expect( !display.grab( root, f7, NativeModifiers ),
				"A failed X11 transaction leaked a partial grab" );
		display.ungrab( root, f7, NativeModifiers );
	}
	display.ungrab( display.roots.back(), f7, NativeModifiers | locks );
	auto retried = registration( service.retry() );
	expectBound( retried, first.id, KEY_F7 );
	expectBound( retried, independent.id, KEY_F8 );

	display.type( caps );
	display.type( num );
	for ( auto root : display.roots ) {
		display.focus( root );
		display.hotkey( f7 );
		waitForActivation( executor );
		expect( activations.back() == first.id, "Wrong shortcut on a locked X11 screen" );
	}
	expect( activations.size() == display.roots.size(), "Lock variants or screen delivery failed" );
	display.type( num );
	display.type( caps );

	// Leave a native event behind an in-flight dispatch, then swap the pair owners.
	executor.hold();
	display.hotkey( f7 );
	expect( executor.wait(), "Could not hold an X11 activation in flight" );
	display.hotkey( f7 );
	auto swap = service.setShortcuts(
		{ shortcut( first.id, KEY_F8 ), shortcut( independent.id, KEY_F7 ) } );
	executor.resume();
	auto swapped = registration( std::move( swap ) );
	expectBound( swapped, first.id, KEY_F8 );
	expectBound( swapped, independent.id, KEY_F7 );
	executor.drain();
	display.hotkey( f7 );
	waitForActivation( executor );
	expect( activations.size() == display.roots.size() + 1 && activations.back() == independent.id,
			"An old native event crossed the X11 binding incarnation" );
	display.hotkey( f8 );
	waitForActivation( executor );
	expect( activations.back() == first.id, "The second swapped trigger was lost" );

	expect( registration( service.clear() ).state == State::Idle, "Clear left an X11 key grab" );
	for ( auto root : display.roots ) {
		expect( !display.grab( root, f6, NativeModifiers ) &&
					!display.grab( root, f7, NativeModifiers ),
				"X11 shortcut was unavailable after clear" );
		display.ungrab( root, f6, NativeModifiers );
		display.ungrab( root, f7, NativeModifiers );
	}
	expectBound( registration( service.setShortcuts( { first } ) ), first.id, KEY_F6 );
	expect( static_cast<bool>( await( service.closeAsync() ) ), "X11 close failed" );
	expect( !display.grab( display.roots.front(), f6, NativeModifiers ),
			"X11 close did not release its grab" );
	display.ungrab( display.roots.front(), f6, NativeModifiers );
}

void testLayout( TestDisplay& display ) {
	DeferredExecutor executor;
	Service service( executor );
	size_t activations = 0;
	service.setStateChangedCallback( []( const ServiceSnapshot& ) {} );
	service.setActivationCallback( [&]( const Activation& ) { ++activations; } );
	auto expectActivations = [&]( size_t count ) {
		const auto deadline = std::chrono::steady_clock::now() + TestDeadline;
		while ( activations < count ) {
			expect( std::chrono::steady_clock::now() < deadline,
					"Timed out waiting for the expected X11 activations" );
			waitForActivation( executor );
		}
		expect( activations == count, "Unexpected number of X11 activations" );
	};
	const auto y = display.code( XKB_KEY_y );
	const auto z = display.code( XKB_KEY_z );
	const auto root = display.roots.back();
	display.focus( root );
	display.group( 0 );
	auto first = registration( service.setShortcuts( { shortcut( "logical", KEY_Y ) } ) );
	expectBound( first, "logical", KEY_Y );
	executor.drain();
	display.hotkey( y );
	expectActivations( 1 );

	const auto revision = service.snapshot().revision;
	display.group( 1 );
	expectBound( waitForState( service, executor,
							   [revision]( const ServiceSnapshot& snapshot ) {
								   return snapshot.revision > revision;
							   } ),
				 "logical", KEY_Y );
	expect( !display.grab( root, y, NativeModifiers ), "Old physical key is still grabbed" );
	display.ungrab( root, y, NativeModifiers );
	expect( display.grab( root, z, NativeModifiers ) == XCB_ACCESS,
			"XKB group change did not grab the new physical key" );
	expectBound( service.snapshot(), "logical", KEY_Y );
	executor.drain();
	display.hotkey( z );
	expectActivations( 2 );

	expect( display.repeats( z ), "The test display must have keyboard repeat enabled for Z" );
	display.key( display.control, true );
	display.key( display.shift, true );
	display.key( z, true );
	expectActivations( 3 );
	const auto holdUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds( 500 );
	for ( auto now = std::chrono::steady_clock::now(); now < holdUntil;
		  now = std::chrono::steady_clock::now() ) {
		if ( !executor.wait(
				 std::chrono::duration_cast<std::chrono::milliseconds>( holdUntil - now ) ) )
			break;
		executor.drain();
		expect( activations == 3, "X11 autorepeat generated a second shortcut activation" );
	}
	display.key( z, false );
	display.key( display.shift, false );
	display.key( display.control, false );
	display.hotkey( z );
	display.hotkey( z );
	expectActivations( 5 );

	display.group( 0 );
	display.remap( y, XKB_KEY_F11 );
	auto removed = waitForState( service, executor, []( const ServiceSnapshot& snapshot ) {
		return binding( snapshot, "logical" ).status == BindingStatus::Unbound;
	} );
	expect( binding( removed, "logical" ).error.has_value(),
			"A missing logical key should have a binding error" );
	expect( !display.grab( root, y, NativeModifiers ),
			"XKB map change left the removed physical mapping grabbed" );
	display.ungrab( root, y, NativeModifiers );
	display.remap( z, XKB_KEY_y );
	auto restored = waitForState( service, executor, []( const ServiceSnapshot& snapshot ) {
		return binding( snapshot, "logical" ).status == BindingStatus::Bound;
	} );
	expectBound( restored, "logical", KEY_Y );
	display.hotkey( z );
	expectActivations( 6 );
	expect( static_cast<bool>( await( service.closeAsync() ) ), "XKB layout test close failed" );
}

void testAltGr( TestDisplay& display ) {
	DeferredExecutor executor;
	Service service( executor );
	unsigned activations = 0;
	service.setActivationCallback( [&]( const Activation& ) { ++activations; } );
	const auto alt = display.code( XKB_KEY_Alt_L );
	const auto altGr = display.code( XKB_KEY_ISO_Level3_Shift );
	const auto key = display.code( XKB_KEY_j );
	display.focus( display.roots.front() );
	auto result = registration( service.setShortcuts(
		{ { "alt", "Distinguish Alt from AltGr", Trigger{ KEY_J, KEYMOD_CTRL | KEYMOD_ALT } } } ) );
	expect( result.state == State::Ready, "XKB did not resolve the plain Alt modifier" );

	display.key( display.control, true );
	display.key( altGr, true );
	display.type( key );
	display.key( altGr, false );
	expect( !executor.wait( std::chrono::milliseconds( 120 ) ),
			"AltGr activated a plain Alt shortcut" );
	display.key( alt, true );
	display.type( key );
	display.key( alt, false );
	display.key( display.control, false );
	expect( executor.wait(), "Timed out waiting for the plain Alt hotkey" );
	executor.drain();
	expect( activations == 1, "Plain Alt did not deliver exactly one activation" );
	expect( static_cast<bool>( await( service.closeAsync() ) ), "AltGr test close failed" );
}

void testDisconnect( bool reconnect ) {
	DeferredExecutor executor;
	Service service( executor );
	service.setStateChangedCallback( []( const ServiceSnapshot& ) {} );
	expectBound( registration( service.setShortcuts( { shortcut( "disconnect", KEY_F6 ) } ) ),
				 "disconnect", KEY_F6 );
	executor.drain();
	std::cout << "READY" << std::endl;
	while ( !service.snapshot().lastBackendIssue ) {
		expect( executor.wait(), "Timed out waiting for X11 disconnect" );
		executor.drain();
	}
	const auto snapshot = service.snapshot();
	expect( snapshot.lastBackendIssue->code == ErrorCode::Disconnected &&
				binding( snapshot, "disconnect" ).status == BindingStatus::Unbound,
			"Disconnect did not revoke the effective X11 binding" );
	if ( reconnect ) {
		std::cout << "LOST" << std::endl;
		expect( std::cin.get() != std::char_traits<char>::eof(),
				"Reconnect test needs a restarted test server" );
		expectBound( registration( service.retry() ), "disconnect", KEY_F6 );
	}
	expect( static_cast<bool>( await( service.closeAsync() ) ),
			"Close failed after X11 disconnect" );
}

} // namespace

int main( int argc, char** argv ) {
	try {
		const std::string_view mode = argc > 1 ? argv[1] : "";
		if ( mode == "--unsupported" ) {
			DeferredExecutor executor;
			Service service( executor );
			auto result = await( service.setShortcuts( { shortcut( "unsupported", KEY_F6 ) } ) );
			expect( !result && result.error().code == ErrorCode::Unsupported,
					"Expected the X11 backend to remain unavailable in this runtime" );
		} else if ( mode == "--disconnect" || mode == "--reconnect" ) {
			testDisconnect( mode == "--reconnect" );
		} else {
			TestDisplay display;
			expect( display.roots.size() >= 2, "Use an isolated X server with two screens" );
			display.control = display.code( XKB_KEY_Control_L );
			display.shift = display.code( XKB_KEY_Shift_L );
			testGrabs( display );
			testAltGr( display );
			testLayout( display );
		}
		std::cout << "X11 global shortcut integration checks passed\n";
	} catch ( const std::exception& error ) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
