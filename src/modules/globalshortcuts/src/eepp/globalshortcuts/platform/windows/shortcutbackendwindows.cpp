#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#include "shortcutbackendwindows.hpp"
#include <algorithm>
#include <atomic>
#include <eepp/core/small_vector.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace EE::System::GlobalShortcuts::Private {

namespace {

using namespace EE::Window;

constexpr int LastHotkeyId = 0xBFFF;
constexpr unsigned MessageBatchSize = 32;

struct WindowsKey {
	UINT key{ 0 };
	UINT modifiers{ 0 };

	bool operator==( const WindowsKey& ) const = default;
};

Error winError( const char* operation, DWORD code ) {
	if ( code == ERROR_HOTKEY_ALREADY_REGISTERED ) {
		return { ErrorCode::Conflict, "Windows hotkey is already registered",
				 static_cast<int>( code ), "ERROR_HOTKEY_ALREADY_REGISTERED" };
	}
	return { ErrorCode::NativeFailure, operation, static_cast<int>( code ) };
}

Error hotkeyIdsExhausted() {
	return { ErrorCode::Busy, "Windows hotkey IDs exhausted. Recreate the shortcut service." };
}

UINT virtualKey( Keycode key ) {
	if ( key >= KEY_A && key <= KEY_Z )
		return 'A' + ( key - KEY_A );
	if ( key >= KEY_0 && key <= KEY_9 )
		return '0' + ( key - KEY_0 );
	if ( key >= KEY_F1 && key <= KEY_F11 )
		return VK_F1 + ( key - KEY_F1 );
	switch ( key ) {
		case KEY_UP:
			return VK_UP;
		case KEY_DOWN:
			return VK_DOWN;
		case KEY_LEFT:
			return VK_LEFT;
		case KEY_RIGHT:
			return VK_RIGHT;
		case KEY_HOME:
			return VK_HOME;
		case KEY_END:
			return VK_END;
		case KEY_PAGEUP:
			return VK_PRIOR;
		case KEY_PAGEDOWN:
			return VK_NEXT;
		case KEY_INSERT:
			return VK_INSERT;
		case KEY_DELETE:
			return VK_DELETE;
		case KEY_RETURN:
			return VK_RETURN;
		case KEY_ESCAPE:
			return VK_ESCAPE;
		case KEY_SPACE:
			return VK_SPACE;
		case KEY_TAB:
			return VK_TAB;
		case KEY_BACKSPACE:
			return VK_BACK;
		default:
			return 0;
	}
}

Result<WindowsKey, Error> windowsKey( const Trigger& trigger ) {
	const auto key = virtualKey( trigger.key );
	if ( !key ) {
		return Err( Error{ ErrorCode::Unsupported,
						   trigger.key == KEY_F12 ? "Windows reserves F12 for the debugger"
												  : "No Windows virtual key for this trigger" } );
	}
	constexpr Uint32 ctrlAlt = KEYMOD_CTRL | KEYMOD_ALT;
	if ( ( trigger.modifiers & ctrlAlt ) == ctrlAlt ) {
		return Err(
			Error{ ErrorCode::Unsupported, "Windows Ctrl+Alt hotkeys can intercept AltGr input" } );
	}
	UINT modifiers = 0;
	if ( trigger.modifiers & KEYMOD_CTRL )
		modifiers |= MOD_CONTROL;
	if ( trigger.modifiers & KEYMOD_ALT )
		modifiers |= MOD_ALT;
	if ( trigger.modifiers & KEYMOD_SHIFT )
		modifiers |= MOD_SHIFT;
	if ( trigger.modifiers & KEYMOD_META )
		modifiers |= MOD_WIN;
	return Ok( WindowsKey{ key, modifiers } );
}

class WindowsShortcutBackend final : public ShortcutBackend {
  public:
	~WindowsShortcutBackend() override;

	Backend type() const override { return Backend::Windows; }

	Result<Capabilities, Error> open() override;

	void reconcile( Client& client, Uint64 request, std::vector<Shortcut> shortcuts,
					bool retry ) override;

	void cancel( Client&, Uint64 ) override {}

	void wait( Client& client ) override;

	void wake() noexcept override;

	Result<void, Error> close() override;

  private:
	struct Registration {
		std::string id;
		std::optional<Error> cleanupError;
		Trigger trigger;
		WindowsKey key;
		int messageId;
	};

	struct Attempt {
		std::string id;
		std::optional<Error> error;
		Trigger trigger;
		WindowsKey key;
		bool pending{ false };
	};

	using Attempts = std::vector<Attempt>;

	Result<int, Error> registerKey( const std::string& id, Trigger trigger, WindowsKey key );

	std::optional<Error> unregisterKey( int messageId );

	void unregisterObsolete( const Attempts& attempts );

	int liveId( const std::string& id ) const;

	const Registration* owner( WindowsKey key ) const;

	static Attempt* findAttempt( Attempts& attempts, const std::string& id );

	void reconcilePending( Attempts& attempts );

	void replace( Attempt& attempt );

	void exchange( Attempts& attempts, Attempt& first );

	std::vector<Binding> report() const;

	void deliver( Client& client, const MSG& message ) const;

	void failWait( Client& client, Error error );

	std::vector<Registration> mRegistrations;
	Attempts mAttempts;
	std::optional<Error> mWaitError;
	std::atomic<HANDLE> mWakeEvent{ nullptr };
	std::atomic<DWORD> mWorkerId{ 0 };
	std::atomic<DWORD> mWakeError{ 0 };
	std::atomic<bool> mPendingWake{ false };
	int mNextId{ 1 };
};

WindowsShortcutBackend::~WindowsShortcutBackend() {
	// The service joins the worker before destruction, including all cancellation wakes.
	if ( auto event = mWakeEvent.load() )
		CloseHandle( event );
}

Result<Capabilities, Error> WindowsShortcutBackend::open() {
	// PeekMessage creates the queue used for windowless hotkeys and the fallback wake message.
	MSG ignored;
	PeekMessageW( &ignored, nullptr, 0, 0, PM_NOREMOVE );
	auto event = CreateEventW( nullptr, FALSE, FALSE, nullptr );
	if ( !event ) {
		const auto code = GetLastError();
		return Err( Error{ ErrorCode::BackendUnavailable,
						   "Could not create Windows hotkey wake event",
						   static_cast<int>( code ) } );
	}
	mWorkerId.store( GetCurrentThreadId() );
	mWakeEvent.store( event );
	return Ok( Capabilities{ true, false, true } );
}

void WindowsShortcutBackend::wake() noexcept {
	mPendingWake.store( true );
	mPendingWake.notify_one();
	if ( auto event = mWakeEvent.load(); event && !SetEvent( event ) ) {
		const auto code = GetLastError();
		mWakeError.store( code ? code : ERROR_GEN_FAILURE );
		// The worker owns a message queue for as long as the event is published. A full
		// queue already gives its native wait work to process.
		PostThreadMessageW( mWorkerId.load(), WM_NULL, 0, 0 );
	}
}

void WindowsShortcutBackend::wait( Client& client ) {
	if ( auto code = mWakeError.exchange( 0 ); code && !mWaitError )
		failWait( client, winError( "Could not signal Windows hotkey worker", code ) );
	if ( mPendingWake.exchange( false ) )
		return;
	const auto event = mWakeEvent.load();
	if ( !event || mWaitError ) {
		mPendingWake.wait( false );
		return;
	}

	const auto result =
		MsgWaitForMultipleObjectsEx( 1, &event, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE );
	if ( result == WAIT_OBJECT_0 )
		return;
	if ( result != WAIT_OBJECT_0 + 1 ) {
		const auto code = result == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
		failWait( client, winError( "Windows hotkey message wait failed", code ) );
		return;
	}

	MSG message;
	for ( unsigned count = 0;
		  count < MessageBatchSize && PeekMessageW( &message, nullptr, 0, 0, PM_REMOVE );
		  ++count ) {
		if ( message.message == WM_QUIT ) {
			failWait( client, { ErrorCode::Disconnected,
								"Windows hotkey message queue received WM_QUIT" } );
			return;
		}
		if ( message.message == WM_HOTKEY && !message.hwnd )
			deliver( client, message );
	}
}

Result<void, Error> WindowsShortcutBackend::close() {
	std::optional<Error> failure;
	for ( size_t i = mRegistrations.size(); i > 0; --i ) {
		if ( auto error = unregisterKey( mRegistrations[i - 1].messageId ); error && !failure )
			failure = std::move( error );
	}
	if ( failure )
		return Err( std::move( *failure ) );
	return Ok();
}

Result<int, Error> WindowsShortcutBackend::registerKey( const std::string& id, Trigger trigger,
														WindowsKey key ) {
	if ( mNextId > LastHotkeyId ) {
		return Err( hotkeyIdsExhausted() );
	}
	// Stage ownership before the OS call. A successful registration always has a tracked record.
	mRegistrations.push_back( { id, {}, trigger, key, mNextId } );
	if ( !RegisterHotKey( nullptr, mNextId, key.modifiers | MOD_NOREPEAT, key.key ) ) {
		const auto code = GetLastError();
		mRegistrations.pop_back();
		return Err( winError( "RegisterHotKey failed", code ) );
	}
	// A queued WM_HOTKEY can survive UnregisterHotKey. IDs stay retired for this worker.
	return Ok( mNextId++ );
}

std::optional<Error> WindowsShortcutBackend::unregisterKey( int messageId ) {
	const auto it = std::find_if(
		mRegistrations.begin(), mRegistrations.end(),
		[messageId]( const Registration& entry ) { return entry.messageId == messageId; } );
	if ( it == mRegistrations.end() )
		return {};
	if ( !UnregisterHotKey( nullptr, messageId ) ) {
		const auto code = GetLastError();
		if ( code != ERROR_HOTKEY_NOT_REGISTERED ) {
			it->cleanupError = winError( "UnregisterHotKey failed", code );
			return it->cleanupError;
		}
	}
	mRegistrations.erase( it );
	return {};
}

void WindowsShortcutBackend::unregisterObsolete( const Attempts& attempts ) {
	for ( size_t i = mRegistrations.size(); i > 0; --i ) {
		const auto& entry = mRegistrations[i - 1];
		const auto it = std::lower_bound(
			attempts.begin(), attempts.end(), entry.id,
			[]( const Attempt& attempt, const std::string& id ) { return attempt.id < id; } );
		if ( entry.cleanupError || it == attempts.end() || it->id != entry.id )
			unregisterKey( entry.messageId );
	}
}

int WindowsShortcutBackend::liveId( const std::string& id ) const {
	const auto it =
		std::find_if( mRegistrations.begin(), mRegistrations.end(),
					  [&]( const auto& entry ) { return !entry.cleanupError && entry.id == id; } );
	return it == mRegistrations.end() ? 0 : it->messageId;
}

const WindowsShortcutBackend::Registration* WindowsShortcutBackend::owner( WindowsKey key ) const {
	const auto it = std::find_if( mRegistrations.begin(), mRegistrations.end(),
								  [key]( const auto& entry ) { return entry.key == key; } );
	return it == mRegistrations.end() ? nullptr : &*it;
}

WindowsShortcutBackend::Attempt* WindowsShortcutBackend::findAttempt( Attempts& attempts,
																	  const std::string& id ) {
	const auto it = std::lower_bound(
		attempts.begin(), attempts.end(), id,
		[]( const Attempt& attempt, const std::string& key ) { return attempt.id < key; } );
	return it != attempts.end() && it->id == id ? &*it : nullptr;
}

void WindowsShortcutBackend::reconcile( Client& client, Uint64 request,
										std::vector<Shortcut> shortcuts, bool retry ) {
	if ( mWaitError && !shortcuts.empty() ) {
		client.complete( request, Err( *mWaitError ) );
		return;
	}
	const bool sameTriggers =
		std::equal( shortcuts.begin(), shortcuts.end(), mAttempts.begin(), mAttempts.end(),
					[]( const Shortcut& shortcut, const Attempt& previous ) {
						return shortcut.id == previous.id && shortcut.preferred == previous.trigger;
					} );
	if ( !retry && sameTriggers &&
		 std::none_of(
			 mRegistrations.begin(), mRegistrations.end(),
			 []( const Registration& entry ) { return entry.cleanupError.has_value(); } ) ) {
		client.complete( request, Ok( report() ) );
		return;
	}

	Attempts attempts;
	attempts.reserve( shortcuts.size() );
	for ( auto& shortcut : shortcuts ) {
		Attempt attempt{ std::move( shortcut.id ), {}, *shortcut.preferred, {} };
		auto key = windowsKey( attempt.trigger );
		if ( key ) {
			attempt.key = key.value();
			attempt.pending = true;
		} else {
			attempt.error = std::move( key ).error();
		}
		attempts.push_back( std::move( attempt ) );
	}
	unregisterObsolete( attempts );
	reconcilePending( attempts );
	mAttempts = std::move( attempts );
	client.complete( request, Ok( report() ) );
}

void WindowsShortcutBackend::reconcilePending( Attempts& attempts ) {
	for ( ;; ) {
		Attempt* firstBlocked = nullptr;
		bool progressed = false;
		for ( auto& attempt : attempts ) {
			if ( !attempt.pending )
				continue;
			if ( const auto* current = owner( attempt.key ) ) {
				const auto* holder = findAttempt( attempts, current->id );
				if ( current->id == attempt.id && !current->cleanupError ) {
					attempt.pending = false;
				} else if ( holder && holder->pending && !current->cleanupError ) {
					firstBlocked = firstBlocked ? firstBlocked : &attempt;
					continue;
				} else {
					attempt.error = current->cleanupError
										? current->cleanupError
										: std::optional<Error>(
											  Error{ ErrorCode::Conflict,
													 "Another shortcut still owns this trigger" } );
					attempt.pending = false;
				}
			} else {
				replace( attempt );
			}
			progressed = true;
		}
		if ( !firstBlocked )
			return;
		if ( !progressed )
			exchange( attempts, *firstBlocked );
	}
}

void WindowsShortcutBackend::replace( Attempt& attempt ) {
	attempt.pending = false;
	const int old = liveId( attempt.id );
	auto added = registerKey( attempt.id, attempt.trigger, attempt.key );
	if ( !added ) {
		attempt.error = std::move( added ).error();
		return;
	}
	if ( auto error = unregisterKey( old ) ) {
		attempt.error = std::move( error );
		if ( auto cleanup = unregisterKey( added.value() ) )
			attempt.error = std::move( cleanup );
	}
}

void WindowsShortcutBackend::exchange( Attempts& attempts, Attempt& first ) {
	struct Saved {
		Attempt* attempt;
		Trigger trigger;
		WindowsKey key;
		int oldId;
		int newId{ 0 };
	};
	SmallVector<Saved, 4> cycle;
	// Each supported logical trigger maps to a unique Win32 pair. With no remaining free
	// pair, the pending dependencies form disjoint cycles.
	auto* next = &first;
	do {
		const auto* held = owner( next->key );
		auto* holder = findAttempt( attempts, held->id );
		cycle.push_back( { holder, held->trigger, held->key, held->messageId } );
		next = holder;
	} while ( next != &first );

	// Keep enough distinct IDs for rollback if another process claims a released pair.
	const auto available = static_cast<size_t>( LastHotkeyId - mNextId + 1 );
	if ( available < cycle.size() * 2 - 1 ) {
		for ( auto& saved : cycle ) {
			saved.attempt->error = hotkeyIdsExhausted();
			saved.attempt->pending = false;
		}
		return;
	}

	std::optional<Error> failure;
	size_t released = 0;
	for ( auto& saved : cycle ) {
		failure = unregisterKey( saved.oldId );
		if ( failure )
			break;
		++released;
	}
	if ( !failure ) {
		for ( auto& saved : cycle ) {
			auto result =
				registerKey( saved.attempt->id, saved.attempt->trigger, saved.attempt->key );
			if ( !result ) {
				failure = std::move( result ).error();
				break;
			}
			saved.newId = result.value();
		}
	}
	if ( failure ) {
		for ( auto& saved : cycle ) {
			saved.attempt->error = *failure;
			if ( auto cleanup = unregisterKey( saved.newId ) )
				saved.attempt->error = std::move( cleanup );
		}
		for ( size_t i = 0; i < released; ++i ) {
			auto& saved = cycle[i];
			auto restored = registerKey( saved.attempt->id, saved.trigger, saved.key );
			if ( !restored )
				saved.attempt->error = std::move( restored ).error();
		}
	}
	for ( auto& saved : cycle )
		saved.attempt->pending = false;
}

std::vector<ShortcutBackend::Binding> WindowsShortcutBackend::report() const {
	std::vector<Binding> bindings;
	bindings.reserve( mAttempts.size() );
	for ( const auto& attempt : mAttempts ) {
		bindings.push_back( { { attempt.id, {}, {}, attempt.error, BindingStatus::Unbound }, 0 } );
	}
	for ( const auto& entry : mRegistrations ) {
		const auto it =
			std::find_if( bindings.begin(), bindings.end(), [&]( const Binding& binding ) {
				return binding.shortcut.id == entry.id;
			} );
		auto& binding = it != bindings.end()
							? *it
							: bindings.emplace_back(
								  Binding{ { entry.id, {}, {}, {}, BindingStatus::Unbound }, 0 } );
		if ( entry.cleanupError ) {
			binding.shortcut.error = entry.cleanupError;
			if ( binding.shortcut.status != BindingStatus::Bound ) {
				binding.shortcut.status = BindingStatus::Unknown;
				binding.shortcut.effectiveTrigger = entry.trigger;
			}
		} else {
			binding.shortcut.status = BindingStatus::Bound;
			binding.shortcut.effectiveTrigger = entry.trigger;
			binding.incarnation = entry.messageId;
		}
	}
	return bindings;
}

void WindowsShortcutBackend::deliver( Client& client, const MSG& message ) const {
	const auto it = std::find_if(
		mRegistrations.begin(), mRegistrations.end(), [&]( const Registration& entry ) {
			return static_cast<WPARAM>( entry.messageId ) == message.wParam;
		} );
	if ( it != mRegistrations.end() && !it->cleanupError &&
		 it->key.key == HIWORD( message.lParam ) &&
		 it->key.modifiers == ( LOWORD( message.lParam ) & ~static_cast<UINT>( MOD_NOREPEAT ) ) )
		client.activate( it->messageId, {} );
}

void WindowsShortcutBackend::failWait( Client& client, Error error ) {
	mWaitError = error;
	close();
	mAttempts.clear();
	client.bindingsChanged( report(), std::move( error ) );
}

} // namespace

std::unique_ptr<ShortcutBackend> createWindowsShortcutBackend() {
	return std::make_unique<WindowsShortcutBackend>();
}

} // namespace EE::System::GlobalShortcuts::Private

#endif
