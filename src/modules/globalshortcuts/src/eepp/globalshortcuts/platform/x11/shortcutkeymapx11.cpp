#include <eepp/config.hpp>

#ifdef EE_GLOBALSHORTCUTS_X11

#include "shortcutkeymapx11.hpp"
#include <algorithm>
#include <cstdlib>
#include <xkbcommon/xkbcommon-x11.h>

// The XCB protocol header uses a C++ keyword as a field name.
#define explicit explicit_
#include <xcb/xkb.h>
#undef explicit

namespace EE::System::GlobalShortcuts::Private {

namespace {

using namespace EE::Window;

xkb_keysym_t symbol( Keycode key ) {
	if ( key >= KEY_A && key <= KEY_Z )
		return XKB_KEY_a + ( key - KEY_A );
	if ( key >= KEY_0 && key <= KEY_9 )
		return XKB_KEY_0 + ( key - KEY_0 );
	if ( key >= KEY_F1 && key <= KEY_F12 )
		return XKB_KEY_F1 + ( key - KEY_F1 );
	switch ( key ) {
		case KEY_UP:
			return XKB_KEY_Up;
		case KEY_DOWN:
			return XKB_KEY_Down;
		case KEY_LEFT:
			return XKB_KEY_Left;
		case KEY_RIGHT:
			return XKB_KEY_Right;
		case KEY_HOME:
			return XKB_KEY_Home;
		case KEY_END:
			return XKB_KEY_End;
		case KEY_PAGEUP:
			return XKB_KEY_Page_Up;
		case KEY_PAGEDOWN:
			return XKB_KEY_Page_Down;
		case KEY_INSERT:
			return XKB_KEY_Insert;
		case KEY_DELETE:
			return XKB_KEY_Delete;
		case KEY_RETURN:
			return XKB_KEY_Return;
		case KEY_ESCAPE:
			return XKB_KEY_Escape;
		case KEY_SPACE:
			return XKB_KEY_space;
		case KEY_TAB:
			return XKB_KEY_Tab;
		case KEY_BACKSPACE:
			return XKB_KEY_BackSpace;
		default:
			return XKB_KEY_NoSymbol;
	}
}

enum ModifierRole : Uint8 {
	Shift = 1 << 0,
	Control = 1 << 1,
	Alt = 1 << 2,
	Super = 1 << 3,
	Lock = 1 << 4,
	LevelSelector = 1 << 5,
	OtherModifier = 1 << 6
};

Uint8 role( xkb_keysym_t sym ) {
	switch ( sym ) {
		case XKB_KEY_Shift_L:
		case XKB_KEY_Shift_R:
			return Shift;
		case XKB_KEY_Control_L:
		case XKB_KEY_Control_R:
			return Control;
		case XKB_KEY_Alt_L:
		case XKB_KEY_Alt_R:
			return Alt;
		case XKB_KEY_Super_L:
		case XKB_KEY_Super_R:
			return Super;
		case XKB_KEY_Caps_Lock:
		case XKB_KEY_Shift_Lock:
		case XKB_KEY_Num_Lock:
		case XKB_KEY_Scroll_Lock:
			return Lock;
		case XKB_KEY_Mode_switch:
		case XKB_KEY_ISO_Level3_Shift:
		case XKB_KEY_ISO_Level3_Latch:
		case XKB_KEY_ISO_Level3_Lock:
		case XKB_KEY_ISO_Level5_Shift:
		case XKB_KEY_ISO_Level5_Latch:
		case XKB_KEY_ISO_Level5_Lock:
			return LevelSelector;
		case XKB_KEY_NoSymbol:
		case XKB_KEY_Meta_L:
		case XKB_KEY_Meta_R:
		case XKB_KEY_Hyper_L:
		case XKB_KEY_Hyper_R:
			// Common alternate symbols on an Alt or Super modifier key.
			return 0;
		default:
			return OtherModifier;
	}
}

} // namespace

Result<void, Error> X11ShortcutKeymap::open( xcb_connection_t* connection ) {
	mConnection = connection;
	if ( !xkb_x11_setup_xkb_extension(
			 connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
			 XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, &mEventBase, nullptr ) ) {
		return Err( Error{ ErrorCode::BackendUnavailable, "The X server does not support XKB" } );
	}
	mContext.reset( xkb_context_new( XKB_CONTEXT_NO_FLAGS ) );
	if ( !mContext )
		return Err( Error{ ErrorCode::BackendUnavailable, "Could not create an XKB context" } );

	constexpr Uint16 maps = XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS |
							XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_KEY_ACTIONS |
							XCB_XKB_MAP_PART_VIRTUAL_MODS | XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
	constexpr Uint16 all = XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY | XCB_XKB_EVENT_TYPE_MAP_NOTIFY;
	xcb_xkb_select_events_details_t details{};
	// Listen only for group changes. Ordinary typing must leave the shortcut worker asleep.
	details.affectState = XCB_XKB_STATE_PART_GROUP_STATE;
	details.stateDetails = XCB_XKB_STATE_PART_GROUP_STATE;
	auto selection = xcb_xkb_select_events_aux_checked( connection, XCB_XKB_ID_USE_CORE_KBD,
														all | XCB_XKB_EVENT_TYPE_STATE_NOTIFY, 0,
														all, maps, maps, &details );
	std::unique_ptr<xcb_generic_error_t, decltype( &std::free )> error(
		xcb_request_check( connection, selection ), &std::free );
	if ( error ) {
		return Err( Error{ ErrorCode::BackendUnavailable, "Could not subscribe to XKB changes",
						   error->error_code } );
	}

	constexpr Uint32 repeat = XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT;
	const auto flags =
		xcb_xkb_per_client_flags( connection, XCB_XKB_ID_USE_CORE_KBD, repeat, repeat, 0, 0, 0 );
	std::unique_ptr<xcb_xkb_per_client_flags_reply_t, decltype( &std::free )> reply(
		xcb_xkb_per_client_flags_reply( connection, flags, nullptr ), &std::free );
	mDetectableRepeat = reply && ( reply->supported & reply->value & repeat );
	return reload();
}

void X11ShortcutKeymap::close() {
	mState.reset();
	mMap.reset();
	mContext.reset();
	mConnection = nullptr;
}

Result<void, Error> X11ShortcutKeymap::reload() {
	const auto device = xkb_x11_get_core_keyboard_device_id( mConnection );
	std::unique_ptr<xkb_keymap, decltype( &xkb_keymap_unref )> map(
		device < 0 ? nullptr
				   : xkb_x11_keymap_new_from_device( mContext.get(), mConnection, device,
													 XKB_KEYMAP_COMPILE_NO_FLAGS ),
		&xkb_keymap_unref );
	std::unique_ptr<xkb_state, decltype( &xkb_state_unref )> state(
		map ? xkb_x11_state_new_from_device( map.get(), mConnection, device ) : nullptr,
		&xkb_state_unref );
	const auto cookie = xcb_get_modifier_mapping( mConnection );
	std::unique_ptr<xcb_get_modifier_mapping_reply_t, decltype( &std::free )> modifiers(
		xcb_get_modifier_mapping_reply( mConnection, cookie, nullptr ), &std::free );
	if ( !state || !modifiers ) {
		return Err(
			Error{ ErrorCode::BackendUnavailable, "Could not read the current XKB keymap" } );
	}
	mModifierMasks = modifierMasks( map.get(), *modifiers );
	mState = std::move( state );
	mMap = std::move( map );
	return Ok();
}

Result<bool, Error> X11ShortcutKeymap::update( const xcb_generic_event_t& event ) {
	if ( event.response_type != mEventBase )
		return Ok( false );
	switch ( reinterpret_cast<const Uint8*>( &event )[1] ) {
		case XCB_XKB_NEW_KEYBOARD_NOTIFY:
		case XCB_XKB_MAP_NOTIFY: {
			auto result = reload();
			return result ? Result<bool, Error>( Ok( true ) )
						  : Result<bool, Error>( Err( std::move( result ).error() ) );
		}
		case XCB_XKB_STATE_NOTIFY: {
			const auto& state = reinterpret_cast<const xcb_xkb_state_notify_event_t&>( event );
			if ( xkb_state_serialize_layout( mState.get(), XKB_STATE_LAYOUT_EFFECTIVE ) ==
				 state.group )
				return Ok( false );
			xkb_state_update_mask( mState.get(), 0, 0, 0, 0, 0, state.group );
			return Ok( true );
		}
		default:
			return Ok( false );
	}
}

std::array<Uint8, 5>
X11ShortcutKeymap::modifierMasks( xkb_keymap* keymap,
								  const xcb_get_modifier_mapping_reply_t& modifiers ) {
	std::array<Uint8, 5> masks{};
	auto* keys = xcb_get_modifier_mapping_keycodes( &modifiers );
	// Each slot in the core modifier map names the physical keys for one real modifier bit.
	for ( unsigned slot = 0; slot < 8; ++slot ) {
		Uint8 roles = 0;
		for ( unsigned i = 0; i < modifiers.keycodes_per_modifier; ++i, ++keys ) {
			if ( !*keys )
				continue;
			for ( xkb_layout_index_t group = 0;
				  group < xkb_keymap_num_layouts_for_key( keymap, *keys ); ++group ) {
				for ( xkb_level_index_t level = 0;
					  level < xkb_keymap_num_levels_for_key( keymap, *keys, group ); ++level ) {
					const xkb_keysym_t* symbols;
					const int n =
						xkb_keymap_key_get_syms_by_level( keymap, *keys, group, level, &symbols );
					for ( int j = 0; j < n; ++j )
						roles |= role( symbols[j] );
				}
			}
		}
		for ( unsigned i = 0; i < masks.size(); ++i ) {
			if ( roles == ( 1U << i ) )
				masks[i] |= 1U << slot;
		}
	}
	return masks;
}

Result<std::vector<X11ShortcutKey>, Error>
X11ShortcutKeymap::resolve( const Trigger& trigger ) const {
	constexpr Uint32 groups[] = { KEYMOD_SHIFT, KEYMOD_CTRL, KEYMOD_ALT, KEYMOD_META };
	std::array<Uint8, 4> required{};
	unsigned allowed = mModifierMasks.back();
	for ( unsigned i = 0; i < required.size(); ++i ) {
		if ( !( trigger.modifiers & groups[i] ) )
			continue;
		required[i] = mModifierMasks[i];
		if ( !required[i] ) {
			return Err( Error{ ErrorCode::Unsupported,
							   "The XKB map has no unambiguous modifier for this shortcut" } );
		}
		allowed |= required[i];
	}

	const auto wanted = symbol( trigger.key );
	std::vector<X11ShortcutKey> pairs;
	for ( auto key = xkb_keymap_min_keycode( mMap.get() );
		  key <= xkb_keymap_max_keycode( mMap.get() ); ++key ) {
		const auto group = xkb_state_key_get_layout( mState.get(), key );
		if ( group == XKB_LAYOUT_INVALID || key > 255 )
			continue;
		const xkb_keysym_t* symbols;
		const auto count = xkb_keymap_key_get_syms_by_level( mMap.get(), key, group, 0, &symbols );
		if ( count != 1 || symbols[0] != wanted || wanted == XKB_KEY_NoSymbol )
			continue;
		for ( unsigned mask = 0; mask <= 255; ++mask ) {
			if ( mask & ~allowed )
				continue;
			if ( std::all_of( required.begin(), required.end(), [mask]( Uint8 groupMask ) {
					 return !groupMask || ( mask & groupMask );
				 } ) )
				pairs.push_back(
					{ static_cast<xcb_keycode_t>( key ), static_cast<Uint8>( mask ) } );
		}
	}
	if ( pairs.empty() ) {
		return Err( Error{ ErrorCode::Unsupported,
						   "The shortcut key is absent from the active XKB group's base level" } );
	}
	return Ok( std::move( pairs ) );
}

} // namespace EE::System::GlobalShortcuts::Private

#endif
