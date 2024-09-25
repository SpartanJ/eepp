#ifndef EE_UI_KEYBOARDSHORTCUT_HPP
#define EE_UI_KEYBOARDSHORTCUT_HPP

#include <eepp/config.hpp>
#include <eepp/window/keycodes.hpp>
#include <map>
#include <string>
#include <unordered_map>
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

	static KeyBindings::Shortcut sanitizeShortcut( const KeyBindings::Shortcut& shortcut );

	static std::string keybindFormat( std::string str );

	static Shortcut toShortcut( const Window::Input* input, const std::string& keys );

	static std::string fromShortcut( const Window::Input* input, KeyBindings::Shortcut shortcut,
									 bool format = false );

	KeyBindings( const Window::Input* input );

	void addKeybindsString( const std::map<std::string, std::string>& binds );

	void addKeybinds( const std::map<Shortcut, std::string>& binds );

	void addKeybindsStringUnordered( const std::unordered_map<std::string, std::string>& binds );

	void addKeybindsUnordered( const std::unordered_map<Shortcut, std::string>& binds );

	void addKeybindString( const std::string& key, const std::string& command );

	void addKeybind( const Shortcut& key, const std::string& command );

	/** If the command is already on the list, it will remove the previous keybind. */
	void replaceKeybindString( const std::string& keys, const std::string& command );

	/** If the command is already on the list, it will remove the previous keybind. */
	void replaceKeybind( const Shortcut& keys, const std::string& command );

	Shortcut getShortcutFromString( const std::string& keys );

	void removeKeybind( const Shortcut& keys );

	bool existsKeybind( const Shortcut& keys );

	void removeCommandKeybind( const std::string& command );

	void removeCommandsKeybind( const std::vector<std::string>& command );

	std::string getCommandFromKeyBind( const Shortcut& keys );

	std::string getCommandKeybindString( const std::string& command ) const;

	void reset();

	const ShortcutMap& getShortcutMap() const;

	const std::map<std::string, Uint64> getKeybindings() const;

	std::string getShortcutString( Shortcut shortcut, bool format = false ) const;

  protected:
	const Window::Input* mInput;
	ShortcutMap mShortcuts;
	std::map<std::string, Uint64> mKeybindingsInvert;
};

}} // namespace EE::UI

template <> struct std::hash<EE::UI::KeyBindings::Shortcut> {
	std::size_t operator()( EE::UI::KeyBindings::Shortcut const& s ) const noexcept {
		return s.toUint64();
	}
};

#endif // EE_UI_KEYBOARDSHORTCUT_HPP
