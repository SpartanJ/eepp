#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS

#include "shortcutbackendmacos.hpp"
#include <Carbon/Carbon.h>
#include <algorithm>
#include <array>
#include <eepp/core/small_vector.hpp>
#include <limits>
#include <pthread.h>

namespace EE::System::GlobalShortcuts::Private {

namespace {

using namespace EE::Window;

constexpr OSType HotKeySignature = 0x4545484B; // EEHK

Error carbonError( const char* operation, OSStatus status ) {
	if ( status == eventHotKeyExistsErr )
		return { ErrorCode::Conflict, "macOS hotkey is already registered", status,
				 "eventHotKeyExistsErr" };
	return { ErrorCode::NativeFailure, operation, status };
}

struct CarbonKey {
	UInt32 code;
	UInt32 modifiers;

	bool operator==( const CarbonKey& ) const = default;
};

// Carbon allows only one installation of the same handler on a target. Hotkey IDs stay unique
// across all services and are never reused, since events can outlive unregistration.
class HotKeyDispatcher {
  public:
	struct Recipient {
		UInt32 id;
		ShortcutBackend::Client* client;
		bool pressed{ false };
	};

	static HotKeyDispatcher& instance() {
		static HotKeyDispatcher dispatcher;
		return dispatcher;
	}

	Result<void, Error> attach() {
		if ( !mHandler ) {
			constexpr EventTypeSpec events[] = {
				{ kEventClassKeyboard, kEventHotKeyPressed },
				{ kEventClassKeyboard, kEventHotKeyReleased },
			};
			const auto target = GetApplicationEventTarget();
			if ( !target ) {
				return Err( Error{ ErrorCode::BackendUnavailable,
								   "macOS has no application event target" } );
			}
			const auto status =
				InstallEventHandler( target, handleEvent, 2, events, this, &mHandler );
			if ( status != noErr )
				return Err( carbonError( "InstallEventHandler failed", status ) );
			mTarget = target;
		}
		++mClients;
		return Ok();
	}

	std::optional<Error> detach() {
		if ( --mClients != 0 || !mHandler )
			return {};
		const auto status = RemoveEventHandler( mHandler );
		if ( status != noErr )
			return carbonError( "RemoveEventHandler failed", status );
		mHandler = nullptr;
		mTarget = nullptr;
		return {};
	}

	Result<UInt32, Error> add( CarbonKey key, ShortcutBackend::Client& client,
							   EventHotKeyRef& ref ) {
		if ( mNextId == std::numeric_limits<UInt32>::max() )
			return Err( Error{ ErrorCode::Busy, "macOS hotkey IDs exhausted" } );
		const auto id = ++mNextId;
		mRecipients.push_back( { id, &client } );
		const auto status = RegisterEventHotKey( key.code, key.modifiers, { HotKeySignature, id },
												 mTarget, kEventHotKeyExclusive, &ref );
		if ( status != noErr ) {
			mRecipients.pop_back();
			return Err( carbonError( "RegisterEventHotKey failed", status ) );
		}
		return Ok( id );
	}

	void forget( UInt32 id ) {
		const auto it = std::find_if( mRecipients.begin(), mRecipients.end(),
									  [id]( const auto& item ) { return item.id == id; } );
		if ( it != mRecipients.end() )
			mRecipients.erase( it );
	}

  private:
	static OSStatus handleEvent( EventHandlerCallRef, EventRef event, void* context ) {
		EventHotKeyID id{};
		if ( GetEventParameter( event, kEventParamDirectObject, typeEventHotKeyID, nullptr,
								sizeof( id ), nullptr, &id ) != noErr ||
			 id.signature != HotKeySignature )
			return eventNotHandledErr;
		auto& recipients = static_cast<HotKeyDispatcher*>( context )->mRecipients;
		const auto it = std::find_if( recipients.begin(), recipients.end(),
									  [id]( const auto& item ) { return item.id == id.id; } );
		if ( it == recipients.end() )
			return eventNotHandledErr;
		const bool pressed = GetEventKind( event ) == kEventHotKeyPressed;
		const bool activate = pressed && !it->pressed;
		it->pressed = pressed;
		if ( activate )
			it->client->activate( id.id, {} );
		return noErr;
	}

	SmallVector<Recipient, 4> mRecipients;
	EventTargetRef mTarget{ nullptr };
	EventHandlerRef mHandler{ nullptr };
	UInt32 mNextId{ 0 };
	size_t mClients{ 0 };
};

class KeyLayout {
  public:
	void refresh() {
		mCharacters.fill( Unmapped );
		auto source = TISCopyCurrentKeyboardLayoutInputSource();
		auto data = layoutData( source );
		if ( !data ) {
			if ( source )
				CFRelease( source );
			source = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
			data = layoutData( source );
		}
		if ( data ) {
			const auto* layout =
				reinterpret_cast<const UCKeyboardLayout*>( CFDataGetBytePtr( data ) );
			findCharacters( layout, 0 );
			// Layouts such as AZERTY place digits on the shifted level of the main row.
			// This selects the key position. A shortcut adds Shift only when requested.
			findCharacters( layout, shiftKey >> 8 );
		}
		if ( source )
			CFRelease( source );
	}

	std::optional<CarbonKey> encode( const Trigger& trigger ) const {
		const auto code = keyCode( trigger.key );
		if ( code == Unmapped )
			return {};
		UInt32 modifiers = 0;
		if ( trigger.modifiers & KEYMOD_CTRL )
			modifiers |= controlKey;
		if ( trigger.modifiers & KEYMOD_ALT )
			modifiers |= optionKey;
		if ( trigger.modifiers & KEYMOD_SHIFT )
			modifiers |= shiftKey;
		if ( trigger.modifiers & KEYMOD_META )
			modifiers |= cmdKey;
		return CarbonKey{ code, modifiers };
	}

  private:
	static constexpr UInt16 Unmapped = std::numeric_limits<UInt16>::max();

	static CFDataRef layoutData( TISInputSourceRef source ) {
		return source ? static_cast<CFDataRef>(
							TISGetInputSourceProperty( source, kTISPropertyUnicodeKeyLayoutData ) )
					  : nullptr;
	}

	void findCharacters( const UCKeyboardLayout* layout, UInt32 shift ) {
		// Scan the printable main section only. Keypad digits and function keys have separate
		// codes.
		constexpr UInt16 keys[] = {
			kVK_ANSI_A,			kVK_ANSI_B,			kVK_ANSI_C,			  kVK_ANSI_D,
			kVK_ANSI_E,			kVK_ANSI_F,			kVK_ANSI_G,			  kVK_ANSI_H,
			kVK_ANSI_I,			kVK_ANSI_J,			kVK_ANSI_K,			  kVK_ANSI_L,
			kVK_ANSI_M,			kVK_ANSI_N,			kVK_ANSI_O,			  kVK_ANSI_P,
			kVK_ANSI_Q,			kVK_ANSI_R,			kVK_ANSI_S,			  kVK_ANSI_T,
			kVK_ANSI_U,			kVK_ANSI_V,			kVK_ANSI_W,			  kVK_ANSI_X,
			kVK_ANSI_Y,			kVK_ANSI_Z,			kVK_ANSI_0,			  kVK_ANSI_1,
			kVK_ANSI_2,			kVK_ANSI_3,			kVK_ANSI_4,			  kVK_ANSI_5,
			kVK_ANSI_6,			kVK_ANSI_7,			kVK_ANSI_8,			  kVK_ANSI_9,
			kVK_ANSI_Equal,		kVK_ANSI_Minus,		kVK_ANSI_LeftBracket, kVK_ANSI_RightBracket,
			kVK_ANSI_Semicolon, kVK_ANSI_Quote,		kVK_ANSI_Backslash,	  kVK_ANSI_Comma,
			kVK_ANSI_Slash,		kVK_ANSI_Period,	kVK_ANSI_Grave,		  kVK_ISO_Section,
			kVK_JIS_Yen,		kVK_JIS_Underscore,
		};
		const auto keyboard = LMGetKbdType();
		for ( const auto key : keys ) {
			UInt32 dead = 0;
			UniChar chars[4];
			UniCharCount length = 0;
			if ( UCKeyTranslate( layout, key, kUCKeyActionDown, shift, keyboard,
								 kUCKeyTranslateNoDeadKeysMask, &dead, 4, &length,
								 chars ) != noErr ||
				 length != 1 )
				continue;
			const auto ch = chars[0];
			const bool digit = ch >= '0' && ch <= '9';
			const bool letter = !shift && ch >= 'a' && ch <= 'z';
			const bool upper = !shift && ch >= 'A' && ch <= 'Z';
			if ( !digit && !letter && !upper )
				continue;
			auto& code = mCharacters[upper ? ch + ( 'a' - 'A' ) : ch];
			if ( code == Unmapped && ( !shift || std::find( mCharacters.begin(), mCharacters.end(),
															key ) == mCharacters.end() ) )
				code = key;
		}
	}

	UInt16 keyCode( Keycode key ) const {
		if ( ( key >= KEY_A && key <= KEY_Z ) || ( key >= KEY_0 && key <= KEY_9 ) )
			return mCharacters[key];
		if ( key >= KEY_F1 && key <= KEY_F12 ) {
			constexpr UInt16 functionKeys[] = {
				kVK_F1, kVK_F2, kVK_F3, kVK_F4,	 kVK_F5,  kVK_F6,
				kVK_F7, kVK_F8, kVK_F9, kVK_F10, kVK_F11, kVK_F12,
			};
			return functionKeys[key - KEY_F1];
		}
		switch ( key ) {
			case KEY_UP:
				return kVK_UpArrow;
			case KEY_DOWN:
				return kVK_DownArrow;
			case KEY_LEFT:
				return kVK_LeftArrow;
			case KEY_RIGHT:
				return kVK_RightArrow;
			case KEY_HOME:
				return kVK_Home;
			case KEY_END:
				return kVK_End;
			case KEY_PAGEUP:
				return kVK_PageUp;
			case KEY_PAGEDOWN:
				return kVK_PageDown;
			case KEY_INSERT:
				return kVK_Help;
			case KEY_DELETE:
				return kVK_ForwardDelete;
			case KEY_RETURN:
				return kVK_Return;
			case KEY_ESCAPE:
				return kVK_Escape;
			case KEY_SPACE:
				return kVK_Space;
			case KEY_TAB:
				return kVK_Tab;
			case KEY_BACKSPACE:
				return kVK_Delete;
			default:
				return Unmapped;
		}
	}

	std::array<UInt16, 128> mCharacters{};
};

class MacOSShortcutBackend final : public ShortcutBackend {
  public:
	Backend type() const override { return Backend::MacOS; }
	bool usesApplicationThread() const override { return true; }
	void wait( Client& ) override {}
	void wake() noexcept override {}
	void cancel( Client&, Uint64 ) override {}

	Result<Capabilities, Error> open() override {
		if ( !pthread_main_np() ) {
			return Err( Error{ ErrorCode::BackendUnavailable,
							   "macOS shortcuts require the main application thread" } );
		}
		auto attached = HotKeyDispatcher::instance().attach();
		if ( !attached )
			return Err( std::move( attached ).error() );
		mAttached = true;
		mLayout.refresh();
		mNotifications = CFNotificationCenterGetDistributedCenter();
		if ( mNotifications ) {
			CFNotificationCenterAddObserver( mNotifications, this, inputSourceChanged,
											 kTISNotifySelectedKeyboardInputSourceChanged, nullptr,
											 CFNotificationSuspensionBehaviorCoalesce );
		}
		return Ok( Capabilities{ true, false, true, false } );
	}

	void reconcile( Client& client, Uint64 request, std::vector<Shortcut> shortcuts,
					bool ) override {
		mClient = &client;
		refreshLayout();
		mDesired = std::move( shortcuts );
		client.complete( request, Ok( apply() ) );
	}

	void processEvents( Client& client ) override {
		if ( !mLayoutDirty )
			return;
		refreshLayout();
		auto bindings = apply();
		std::optional<Error> issue;
		for ( const auto& binding : bindings ) {
			if ( binding.shortcut.error ) {
				issue = binding.shortcut.error;
				break;
			}
		}
		client.bindingsChanged( std::move( bindings ), std::move( issue ) );
	}

	Result<void, Error> close() override {
		if ( !mAttached )
			return Ok();
		if ( mNotifications ) {
			CFNotificationCenterRemoveObserver(
				mNotifications, this, kTISNotifySelectedKeyboardInputSourceChanged, nullptr );
			mNotifications = nullptr;
		}
		mClient = nullptr;
		std::optional<Error> failure;
		for ( size_t i = mRegistrations.size(); i > 0; --i ) {
			if ( auto error = remove( mRegistrations[i - 1].nativeId ); error && !failure )
				failure = std::move( error );
		}
		mRegistrations.clear();
		if ( auto error = HotKeyDispatcher::instance().detach(); error && !failure )
			failure = std::move( error );
		mAttached = false;
		if ( failure )
			return Err( std::move( *failure ) );
		return Ok();
	}

  private:
	struct Registration {
		std::string id;
		Trigger trigger;
		CarbonKey key;
		EventHotKeyRef ref{ nullptr };
		UInt32 nativeId{ 0 };
		std::optional<Error> cleanupError;
	};

	struct Attempt {
		const Shortcut* shortcut;
		std::optional<CarbonKey> key;
		std::optional<Error> error;
		bool pending;
	};

	using Attempts = SmallVector<Attempt, 4>;

	static void inputSourceChanged( CFNotificationCenterRef, void* context, CFStringRef,
									const void*, CFDictionaryRef ) {
		auto& self = *static_cast<MacOSShortcutBackend*>( context );
		self.mLayoutDirty = true;
		if ( self.mClient )
			self.mClient->scheduleNativeWork();
	}

	void refreshLayout() {
		if ( mLayoutDirty ) {
			mLayoutDirty = false;
			mLayout.refresh();
		}
	}

	Result<UInt32, Error> add( const std::string& id, Trigger trigger, CarbonKey key ) {
		auto& entry =
			mRegistrations.emplace_back( Registration{ id, trigger, key, nullptr, 0, {} } );
		auto result = HotKeyDispatcher::instance().add( key, *mClient, entry.ref );
		if ( !result ) {
			mRegistrations.pop_back();
			return result;
		}
		entry.nativeId = result.value();
		return result;
	}

	std::optional<Error> remove( UInt32 id ) {
		const auto it = std::find_if( mRegistrations.begin(), mRegistrations.end(),
									  [id]( const auto& item ) { return item.nativeId == id; } );
		if ( it == mRegistrations.end() )
			return {};
		HotKeyDispatcher::instance().forget( id );
		const auto status = UnregisterEventHotKey( it->ref );
		if ( status != noErr ) {
			it->cleanupError = carbonError( "UnregisterEventHotKey failed", status );
			return it->cleanupError;
		}
		mRegistrations.erase( it );
		return {};
	}

	const Registration* owner( CarbonKey key ) const {
		const auto it = std::find_if( mRegistrations.begin(), mRegistrations.end(),
									  [key]( const auto& item ) { return item.key == key; } );
		return it == mRegistrations.end() ? nullptr : &*it;
	}

	UInt32 liveId( const std::string& id ) const {
		const auto it =
			std::find_if( mRegistrations.begin(), mRegistrations.end(),
						  [&]( const auto& item ) { return item.id == id && !item.cleanupError; } );
		return it == mRegistrations.end() ? 0 : it->nativeId;
	}

	static Attempt* findAttempt( Attempts& attempts, const std::string& id ) {
		const auto it = std::lower_bound(
			attempts.begin(), attempts.end(), id,
			[]( const Attempt& item, const std::string& key ) { return item.shortcut->id < key; } );
		return it != attempts.end() && it->shortcut->id == id ? &*it : nullptr;
	}

	void replace( Attempt& attempt ) {
		const auto& shortcut = *attempt.shortcut;
		const auto old = liveId( shortcut.id );
		auto added = add( shortcut.id, *shortcut.preferred, *attempt.key );
		if ( !added ) {
			attempt.error = std::move( added ).error();
		} else if ( auto error = remove( old ) ) {
			attempt.error = std::move( error );
			remove( added.value() );
		}
	}

	void exchange( Attempts& attempts, Attempt& first ) {
		struct Saved {
			Attempt* attempt;
			Trigger trigger;
			CarbonKey key;
			UInt32 oldId;
			UInt32 newId{ 0 };
		};
		// Desired native pairs are unique. When every pending pair is held by another pending
		// shortcut, the ownership links form disjoint cycles.
		SmallVector<Saved, 4> cycle;
		auto* next = &first;
		do {
			const auto* held = owner( *next->key );
			next = findAttempt( attempts, held->id );
			cycle.push_back( { next, held->trigger, held->key, held->nativeId } );
		} while ( next != &first );

		std::optional<Error> failure;
		size_t released = 0;
		for ( auto& saved : cycle ) {
			failure = remove( saved.oldId );
			if ( failure )
				break;
			++released;
		}
		if ( !failure ) {
			for ( auto& saved : cycle ) {
				const auto& shortcut = *saved.attempt->shortcut;
				auto result = add( shortcut.id, *shortcut.preferred, *saved.attempt->key );
				if ( !result ) {
					failure = std::move( result ).error();
					break;
				}
				saved.newId = result.value();
			}
		}
		if ( failure ) {
			for ( const auto& saved : cycle )
				remove( saved.newId );
			for ( size_t i = 0; i < released; ++i ) {
				const auto& saved = cycle[i];
				auto restored = add( saved.attempt->shortcut->id, saved.trigger, saved.key );
				if ( !restored )
					saved.attempt->error = std::move( restored ).error();
			}
		}
		for ( auto& saved : cycle ) {
			if ( !saved.attempt->error )
				saved.attempt->error = failure;
			saved.attempt->pending = false;
		}
	}

	void reconcilePending( Attempts& attempts ) {
		for ( ;; ) {
			Attempt* blocked = nullptr;
			bool progressed = false;
			for ( auto& attempt : attempts ) {
				if ( !attempt.pending )
					continue;
				if ( const auto* held = owner( *attempt.key ) ) {
					auto* holder = findAttempt( attempts, held->id );
					if ( holder == &attempt && !held->cleanupError ) {
						// Preserve a registration whose native trigger stays the same.
					} else if ( holder && holder->pending && !held->cleanupError ) {
						if ( !blocked )
							blocked = &attempt;
						continue;
					} else {
						attempt.error = held->cleanupError.value_or( Error{
							ErrorCode::Conflict, "Another shortcut still owns this trigger" } );
					}
				} else {
					replace( attempt );
				}
				attempt.pending = false;
				progressed = true;
			}
			if ( !blocked )
				return;
			if ( !progressed )
				exchange( attempts, *blocked );
		}
	}

	std::vector<Binding> apply() {
		Attempts attempts;
		attempts.reserve( mDesired.size() );
		for ( const auto& shortcut : mDesired ) {
			auto key = mLayout.encode( *shortcut.preferred );
			std::optional<Error> error;
			if ( !key )
				error =
					Error{ ErrorCode::Unsupported, "No key for this trigger in the macOS layout" };
			attempts.push_back( { &shortcut, key, std::move( error ), key.has_value() } );
		}
		for ( size_t i = mRegistrations.size(); i > 0; --i ) {
			const auto& entry = mRegistrations[i - 1];
			// A changed layout can make an old physical key represent a different character.
			if ( entry.cleanupError || !findAttempt( attempts, entry.id ) ||
				 mLayout.encode( entry.trigger ) != entry.key )
				remove( entry.nativeId );
		}
		reconcilePending( attempts );

		std::vector<Binding> bindings;
		bindings.reserve( attempts.size() );
		for ( auto& attempt : attempts ) {
			bindings.push_back( { { attempt.shortcut->id,
									{},
									{},
									std::move( attempt.error ),
									BindingStatus::Unbound },
								  0 } );
		}
		for ( const auto& entry : mRegistrations ) {
			const auto it =
				std::find_if( bindings.begin(), bindings.end(),
							  [&]( const auto& item ) { return item.shortcut.id == entry.id; } );
			auto& binding =
				it != bindings.end()
					? *it
					: bindings.emplace_back( Binding{ { entry.id, {}, {}, {}, {} }, 0 } );
			if ( entry.cleanupError ) {
				binding.shortcut.error = entry.cleanupError;
				if ( binding.shortcut.status != BindingStatus::Bound )
					binding.shortcut.status = BindingStatus::Unknown;
			} else {
				binding.shortcut.status = BindingStatus::Bound;
				binding.shortcut.effectiveTrigger = entry.trigger;
				binding.incarnation = entry.nativeId;
			}
		}
		return bindings;
	}

	SmallVector<Registration, 4> mRegistrations;
	std::vector<Shortcut> mDesired;
	KeyLayout mLayout;
	Client* mClient{ nullptr };
	CFNotificationCenterRef mNotifications{ nullptr };
	bool mAttached{ false };
	bool mLayoutDirty{ false };
};

} // namespace

std::unique_ptr<ShortcutBackend> createMacOSShortcutBackend() {
	return std::make_unique<MacOSShortcutBackend>();
}

} // namespace EE::System::GlobalShortcuts::Private

#endif
