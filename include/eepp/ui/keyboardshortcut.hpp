#ifndef EE_UI_KEYBOARDSHORTCUT_HPP
#define EE_UI_KEYBOARDSHORTCUT_HPP

#include <eepp/config.hpp>
#include <eepp/window/keycodes.hpp>
#include <map>
#include <string>
#include <vector>

using namespace EE::Window;
namespace EE { namespace Window {
class Input;
}} // namespace EE::Window

namespace EE { namespace UI {

class UIWidget;

typedef std::map<Uint64, std::string> ShortcutMap;

class EE_API KeyBindings {
  public:
	struct Shortcut {
		Shortcut() {}
		Shortcut( Keycode key, Uint32 mod ) : key( key ), mod( mod ) {}
		Shortcut( const Uint64& code ) :
			key( (Keycode)( code & 0xFFFFFFFF ) ), mod( ( code >> 32 ) & 0xFFFFFFFF ) {}
		Keycode key{ KEY_UNKNOWN };
		Uint32 mod{ 0 };
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

	void reset();

	const ShortcutMap& getShortcutMap() const;

	std::string getShortcutString( Shortcut shortcut );

	Uint32 getDefaultModifier() const;

	void setDefaultModifier( Uint32 newDefaultModifier );

  protected:
	const Window::Input* mInput;
	ShortcutMap mShortcuts;
};

}} // namespace EE::UI

#endif // EE_UI_KEYBOARDSHORTCUT_HPP
