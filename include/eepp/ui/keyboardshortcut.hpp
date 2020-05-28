#ifndef EE_UI_KEYBOARDSHORTCUT_HPP
#define EE_UI_KEYBOARDSHORTCUT_HPP

#include <eepp/config.hpp>
#include <map>
#include <string>
#include <vector>

namespace EE { namespace Window {
class Input;
}} // namespace EE::Window

namespace EE { namespace UI {

class UIWidget;

class EE_API UIKeyShortcut {
  public:
	UIKeyShortcut() : KeyCode( 0 ), Mod( 0 ), Widget( NULL ) {}

	UIKeyShortcut( const Uint32& KeyCode, const Uint32& Mod, UIWidget* Widget ) :
		KeyCode( KeyCode ), Mod( Mod ), Widget( Widget ) {}

	Uint32 KeyCode;
	Uint32 Mod;
	UIWidget* Widget;
};

typedef std::vector<UIKeyShortcut> UIKeyboardShortcuts;

class EE_API KeyBindings {
  public:
	struct Shortcut {
		Shortcut() {}
		Shortcut( Uint32 key, Uint32 mod ) : key( key ), mod( mod ) {}
		Shortcut( const Uint64& code ) :
			key( code & 0xFFFFFFFF ), mod( ( code >> 32 ) & 0xFFFFFFFF ) {}
		Uint32 key{0};
		Uint32 mod{0};
		Uint64 toUint64() const { return (Uint64)mod << 32 | (Uint64)key; }
		operator Uint64() const { return toUint64(); }
		bool empty() const { return 0 == mod && 0 == key; }
	};

	static bool isKeyMod( std::string key );

	static Uint32 getKeyMod( std::string key );

	KeyBindings( const Window::Input* input );

	void addKeybindsString( const std::map<std::string, std::string>& binds );

	void addKeybinds( const std::map<Shortcut, std::string>& binds );

	void addKeybindString( const std::string& keys, const std::string& command );

	void addKeybind( const Shortcut& keys, const std::string& command );

	/** If the command is already on the list, it will remove the previous keybind. */
	void replaceKeybindString( const std::string& keys, const std::string& command );

	/** If the command is already on the list, it will remove the previous keybind. */
	void replaceKeybind( const Shortcut& keys, const std::string& command );

	Shortcut getShortcutFromString( const std::string& keys );

	void removeKeybind( const Shortcut& keys );

	bool existsKeybind( const Shortcut& keys );

	std::string getCommandFromKeyBind( const Shortcut& keys );

  protected:
	const Window::Input* mInput;
	/** Map first keys, then Mods (Shortcut) to get the command */
	typedef std::map<Uint64, std::string> ShortcutMap;
	ShortcutMap mShortcuts;
};

}} // namespace EE::UI

#endif // EE_UI_KEYBOARDSHORTCUT_HPP
