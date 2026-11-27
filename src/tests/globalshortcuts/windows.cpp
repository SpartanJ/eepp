#include "shortcuttest.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <array>
#include <iostream>

using namespace EE;
using namespace EE::System::GlobalShortcuts;
using namespace EE::System::GlobalShortcuts::Test;
using namespace EE::Window;

namespace {

constexpr Uint32 Modifiers = KEYMOD_CTRL | KEYMOD_SHIFT;
constexpr UINT NativeModifiers = MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT;

Shortcut shortcut( std::string id, Keycode key, Uint32 modifiers = Modifiers ) {
	return { std::move( id ), "Windows integration test", Trigger{ key, modifiers } };
}

void expectBound( const ServiceSnapshot& snapshot, std::string_view id, Keycode key ) {
	const auto& entry = binding( snapshot, id );
	expect( entry.status == BindingStatus::Bound &&
				entry.effectiveTrigger == Trigger{ key, Modifiers },
			"Expected a confirmed Windows shortcut with its effective trigger" );
}

class ScopedHotkey {
  public:
	explicit ScopedHotkey( UINT key ) {
		if ( !RegisterHotKey( nullptr, 1, NativeModifiers, key ) ) {
			const auto error = GetLastError();
			throw std::runtime_error( "Test hotkey already in use or unavailable, Win32 " +
									  std::to_string( error ) );
		}
	}

	~ScopedHotkey() { UnregisterHotKey( nullptr, 1 ); }
};

void testRegistration() {
	DeferredExecutor executor;
	Service service( executor );
	expect( service.snapshot().state == State::Idle, "Service should begin idle" );
	expect( registration( service.clear() ).backend == Backend::None,
			"Empty declaration opened the worker" );
	auto first = shortcut( "first", KEY_F6 );
	auto second = shortcut( "second", KEY_F7 );
	auto stable = shortcut( "stable", KEY_F8 );
	auto initial = registration( service.setShortcuts( { first, second, stable } ) );
	expect( initial.backend == Backend::Windows && initial.capabilities.desktopWide &&
				initial.state == State::Ready,
			"Failed to register Windows shortcuts" );
	expectBound( initial, first.id, KEY_F6 );
	expectBound( initial, second.id, KEY_F7 );
	expectBound( initial, stable.id, KEY_F8 );

	auto change = shortcut( first.id, KEY_F9 );
	{
		ScopedHotkey conflict( VK_F9 );
		auto blocked = registration( service.setShortcuts( { change, second, stable } ) );
		const auto& retained = binding( blocked, first.id );
		expect( blocked.state == State::Degraded && retained.error &&
					retained.error->code == ErrorCode::Conflict &&
					retained.error->nativeCode == ERROR_HOTKEY_ALREADY_REGISTERED,
				"Expected the Windows conflict and its native error code" );
		expectBound( blocked, first.id, KEY_F6 );
		expectBound( blocked, stable.id, KEY_F8 );
	}
	change.description = "Description updated";
	auto metadata = registration( service.setShortcuts( { change, second, stable } ) );
	expectBound( metadata, first.id, KEY_F6 );
	expect( metadata.state == State::Degraded, "A description edit retried a failed hotkey" );
	auto retried = registration( service.retry() );
	expectBound( retried, first.id, KEY_F9 );
	expect( retried.state == State::Ready, "Explicit retry failed" );
	{
		ScopedHotkey released( VK_F6 );
	}

	auto swapped = registration( service.setShortcuts(
		{ shortcut( first.id, KEY_F7 ), shortcut( second.id, KEY_F9 ), stable } ) );
	expectBound( swapped, first.id, KEY_F7 );
	expectBound( swapped, second.id, KEY_F9 );
	expectBound( swapped, stable.id, KEY_F8 );

	auto rotated = registration(
		service.setShortcuts( { shortcut( first.id, KEY_F8 ), shortcut( second.id, KEY_F7 ),
								shortcut( stable.id, KEY_F9 ) } ) );
	expectBound( rotated, first.id, KEY_F8 );
	expectBound( rotated, second.id, KEY_F7 );
	expectBound( rotated, stable.id, KEY_F9 );

	auto limited = registration(
		service.setShortcuts( { shortcut( first.id, KEY_F8 ), shortcut( "reserved", KEY_F12 ),
								shortcut( "altgr", KEY_F6, KEYMOD_CTRL | KEYMOD_ALT ) } ) );
	expectBound( limited, first.id, KEY_F8 );
	for ( auto id : { "reserved", "altgr" } ) {
		const auto& rejected = binding( limited, id );
		expect( rejected.status == BindingStatus::Unbound && rejected.error &&
					rejected.error->code == ErrorCode::Unsupported,
				"Expected a per-binding error for the unsupported trigger" );
	}
	expect( registration( service.clear() ).state == State::Idle,
			"Clear left an effective binding" );
	{
		ScopedHotkey released( VK_F8 );
	}
	expect( registration( service.setShortcuts( { first } ) ).state == State::Ready,
			"Failed to register again after clear" );
	expect( static_cast<bool>( await( service.closeAsync() ) ), "Close failed" );
	expect( static_cast<bool>( await( service.closeAsync() ) ), "Repeated close failed" );
	{
		ScopedHotkey released( VK_F6 );
	}
}

void sendHotkey( WORD key, bool repeat = false ) {
	std::array<INPUT, 7> input{};
	UINT count = 0;
	auto add = [&]( WORD keyCode, DWORD flags = 0 ) {
		auto& event = input[count++];
		event.type = INPUT_KEYBOARD;
		event.ki.wVk = keyCode;
		event.ki.dwFlags = flags;
	};
	add( VK_CONTROL );
	add( VK_SHIFT );
	add( key );
	if ( repeat )
		add( key );
	add( key, KEYEVENTF_KEYUP );
	add( VK_SHIFT, KEYEVENTF_KEYUP );
	add( VK_CONTROL, KEYEVENTF_KEYUP );
	expect( SendInput( count, input.data(), sizeof( INPUT ) ) == count, "SendInput failed" );
}

void testDelivery( bool repeat ) {
	DeferredExecutor executor;
	Service service( executor );
	const auto applicationThread = GetCurrentThreadId();
	std::vector<std::string> activations;
	service.setActivationCallback( [&]( const Activation& event ) {
		expect( GetCurrentThreadId() == applicationThread,
				"Activation ran off the application thread" );
		activations.push_back( event.id );
	} );
	expectBound( registration( service.setShortcuts( { shortcut( "command", KEY_F6 ) } ) ),
				 "command", KEY_F6 );
	sendHotkey( VK_F6, repeat );
	expect( executor.wait(), "Expected a windowless WM_HOTKEY activation" );
	executor.drain();
	expect( activations.size() == 1 && activations.front() == "command", "Unexpected activation" );
	if ( repeat ) {
		expect( !executor.wait( std::chrono::milliseconds( 200 ) ),
				"Received a repeated hotkey activation before release" );
	}

	sendHotkey( VK_F6 );
	expect( executor.wait(), "Expected activation before rebinding" );
	expectBound( registration( service.setShortcuts( { shortcut( "command", KEY_F7 ) } ) ),
				 "command", KEY_F7 );
	executor.drain();
	expect( activations.size() == 1, "Rebinding delivered a stale activation" );
	sendHotkey( VK_F7 );
	expect( executor.wait(), "Expected activation from replacement hotkey" );
	expect( static_cast<bool>( await( service.closeAsync() ) ), "Close failed during delivery" );
	executor.drain();
	expect( activations.size() == 1, "Close delivered a queued activation" );
}

void testExternalConflict() {
	DeferredExecutor executor;
	Service service( executor );
	auto snapshot = registration( service.setShortcuts(
		{ shortcut( "available", KEY_F6 ), shortcut( "contested", KEY_F9 ) } ) );
	expectBound( snapshot, "available", KEY_F6 );
	const auto& contested = binding( snapshot, "contested" );
	expect( snapshot.state == State::Degraded && contested.status == BindingStatus::Unbound &&
				contested.error && contested.error->code == ErrorCode::Conflict,
			"Expected a partial registration while another process holds F9" );
	expect( static_cast<bool>( await( service.closeAsync() ) ),
			"Close after partial registration failed" );
}

void testOffscreen() {
	DeferredExecutor executor;
	Service service( executor );
	auto result = await( service.setShortcuts( { shortcut( "offscreen", KEY_F6 ) } ) );
	expect( !result && result.error().code == ErrorCode::Unsupported,
			"Expected the unsupported backend in an offscreen runtime" );
	expect( service.snapshot().backend == Backend::None,
			"Offscreen runtime opened a native backend" );
}

} // namespace

int main( int argc, char** argv ) {
	try {
		const std::string_view mode = argc > 1 ? argv[1] : "";
		if ( mode == "--hold-f9" ) {
			ScopedHotkey conflict( VK_F9 );
			std::cout << "READY" << std::endl;
			std::cin.get();
			return 0;
		}
		if ( mode == "--external-conflict" ) {
			testExternalConflict();
			std::cout << "Windows interprocess hotkey conflict passed\n";
			return 0;
		}
		if ( mode == "--offscreen" ) {
			testOffscreen();
			std::cout << "Windows offscreen runtime support check passed\n";
			return 0;
		}
		testRegistration();
		std::cout << "Windows hotkey registration, conflicts, exchanges, and cleanup passed\n";
		if ( mode == "--delivery" || mode == "--repeat" ) {
			testDelivery( mode == "--repeat" );
			std::cout << "Windows hotkey delivery, rebinding, and close passed\n";
		}
	} catch ( const std::exception& error ) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
