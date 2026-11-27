#ifndef EE_GLOBALSHORTCUTS_SHORTCUTKEYMAPX11_HPP
#define EE_GLOBALSHORTCUTS_SHORTCUTKEYMAPX11_HPP

#include <array>
#include <compare>
#include <eepp/globalshortcuts/service.hpp>
#include <memory>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>

namespace EE::System::GlobalShortcuts::Private {

struct X11ShortcutKey {
	xcb_keycode_t code;
	Uint8 modifiers;

	auto operator<=>( const X11ShortcutKey& ) const = default;
};

class X11ShortcutKeymap {
  public:
	Result<void, Error> open( xcb_connection_t* connection );

	void close();

	/** @return Whether the active mapping changed. */
	Result<bool, Error> update( const xcb_generic_event_t& event );

	Result<std::vector<X11ShortcutKey>, Error> resolve( const Trigger& trigger ) const;

	bool detectableRepeat() const { return mDetectableRepeat; }

  private:
	Result<void, Error> reload();

	static std::array<Uint8, 5> modifierMasks( xkb_keymap* map,
											   const xcb_get_modifier_mapping_reply_t& modifiers );

	xcb_connection_t* mConnection{ nullptr };
	std::unique_ptr<xkb_context, decltype( &xkb_context_unref )> mContext{ nullptr,
																		   &xkb_context_unref };
	std::unique_ptr<xkb_keymap, decltype( &xkb_keymap_unref )> mMap{ nullptr, &xkb_keymap_unref };
	std::unique_ptr<xkb_state, decltype( &xkb_state_unref )> mState{ nullptr, &xkb_state_unref };
	std::array<Uint8, 5> mModifierMasks{};
	Uint8 mEventBase{ 0 };
	bool mDetectableRepeat{ false };
};

} // namespace EE::System::GlobalShortcuts::Private

#endif
