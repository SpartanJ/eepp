#ifndef EE_UI_KEYBOARDSHORTCUT_HPP
#define EE_UI_KEYBOARDSHORTCUT_HPP

#include <eepp/config.hpp>
#include <list>

namespace EE { namespace UI {

class UIWidget;

class KeyboardShortcut {
  public:
	KeyboardShortcut() : KeyCode( 0 ), Mod( 0 ), Widget( NULL ) {}

	KeyboardShortcut( const Uint32& KeyCode, const Uint32& Mod, UIWidget* Widget ) :
		KeyCode( KeyCode ), Mod( Mod ), Widget( Widget ) {}

	Uint32 KeyCode;
	Uint32 Mod;
	UIWidget* Widget;
};

typedef std::list<KeyboardShortcut> KeyboardShortcuts;

}} // namespace EE::UI

#endif // EE_UI_KEYBOARDSHORTCUT_HPP
