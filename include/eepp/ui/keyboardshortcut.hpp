#ifndef EE_UI_KEYBOARDSHORTCUT_HPP
#define EE_UI_KEYBOARDSHORTCUT_HPP

#include <eepp/config.hpp>
#include <eepp/core/containers.hpp>
#include <eepp/window/keycodes.hpp>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace EE::Window;
namespace EE { namespace Window {
class Input;
}} // namespace EE::Window

namespace EE { namespace UI {

class UIWidget;

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
		bool operator<( const Shortcut& other ) const { return toUint64() < other.toUint64(); }
		bool operator==( const Shortcut& other ) const {
			return key == other.key && mod == other.mod;
		}
		bool operator!=( const Shortcut& other ) const { return !( *this == other ); }
		bool empty() const { return 0 == mod && 0 == key; }
	};

	struct ShortcutHash {
		size_t operator()( const Shortcut& shortcut ) const noexcept {
			return hashCombine( static_cast<size_t>( shortcut.key ),
								static_cast<size_t>( shortcut.mod ) );
		}
	};

	typedef UnorderedMap<Shortcut, std::string, ShortcutHash> ShortcutMap;

	static KeyBindings::Shortcut sanitizeShortcut( const KeyBindings::Shortcut& shortcut );

	/** Returns the shortcut keys in ascending packed-value order. This provides deterministic
	 * iteration at boundaries where the order affects the selected reverse binding or serialized
	 * output, while keeping ShortcutMap optimized for lookup. */
	static std::vector<Shortcut> getOrderedShortcuts( const ShortcutMap& bindings );

	static std::string keybindFormat( std::string str );

	static Shortcut toShortcut( const Window::Input* input, const std::string& keys );

	static std::string fromShortcut( const Window::Input* input, KeyBindings::Shortcut shortcut,
									 bool format = false );

	KeyBindings( const Window::Input* input );

	/** Shares @p bindings storage while keeping this instance's input source. */
	void setKeybinds( const KeyBindings& bindings );

	void addKeybindsString( const std::map<std::string, std::string>& binds );

	void addKeybinds( const ShortcutMap& binds );

	void addKeybindsStringUnordered( const std::unordered_map<std::string, std::string>& binds );

	void addKeybindString( const std::string& key, const std::string& command );

	void addKeybind( const Shortcut& key, const std::string& command );

	/** If the command is already on the list, it will remove the previous keybind. */
	void replaceKeybindString( const std::string& keys, const std::string& command );

	/** If the command is already on the list, it will remove the previous keybind. */
	void replaceKeybind( const Shortcut& keys, const std::string& command );

	Shortcut getShortcutFromString( const std::string& keys );

	void removeKeybind( const Shortcut& keys );

	void removeKeybind( const std::string& kb );

	bool existsKeybind( const Shortcut& keys );

	bool hasCommand( const std::string& command );

	void removeCommandKeybind( const std::string& command );

	void removeCommandsKeybind( const std::vector<std::string>& command );

	std::string getCommandFromKeyBind( const Shortcut& keys );

	std::string getCommandKeybindString( const std::string& command ) const;

	void reset();

	const ShortcutMap& getShortcutMap() const;

	const std::map<std::string, Shortcut>& getKeybindings() const;

	Shortcut getShortcutFromCommand( const std::string& cmd ) const;

	std::string getShortcutString( Shortcut shortcut, bool format = false ) const;

  protected:
	struct Storage {
		ShortcutMap shortcuts;
		std::map<std::string, Shortcut> keybindingsInvert;
	};

	const Window::Input* mInput;
	std::shared_ptr<Storage> mStorage;

	static std::shared_ptr<Storage> getEmptyStorage();

	void ensureUniqueStorage();
};

}} // namespace EE::UI

template <> struct std::hash<EE::UI::KeyBindings::Shortcut> {
	std::size_t operator()( EE::UI::KeyBindings::Shortcut const& s ) const noexcept {
		return EE::UI::KeyBindings::ShortcutHash{}( s );
	}
};

#endif // EE_UI_KEYBOARDSHORTCUT_HPP
