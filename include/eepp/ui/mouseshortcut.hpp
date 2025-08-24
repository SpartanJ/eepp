#pragma once

#include "eepp/core/containers.hpp"
#include <eepp/config.hpp>
#include <eepp/window/keycodes.hpp>
#include <map>
#include <string>

using namespace EE::Window;
namespace EE { namespace Window {
class Input;
}} // namespace EE::Window

namespace EE { namespace UI {

class EE_API MouseBindings {
  public:
	struct Shortcut {
		Shortcut() {}

		Shortcut( MouseAction action, MouseButtonsMask mouseMask, Uint32 mod ) :
			action( action ), key( mouseMask ), mod( mod ) {}

		bool operator<( const Shortcut& other ) const {
			if ( action != other.action )
				return action < other.action;
			if ( key != other.key )
				return key < other.key;
			return mod < other.mod;
		}

		bool operator==( const Shortcut& other ) const {
			return action == other.action && key == other.key && mod == other.mod;
		}

		bool operator!=( const Shortcut& other ) const { return !( *this == other ); }

		bool empty() const { return 0 == mod && 0 == key; }

		MouseAction action{ MouseAction::Click };
		MouseButtonsMask key{ KEY_UNKNOWN };
		Uint32 mod{ 0 };
	};

	typedef std::map<Shortcut, std::string> ShortcutMap;

	static MouseButtonsMask getMouseButtonsMask( const std::string& str );

	static std::string getMouseButtonsName( MouseButtonsMask mask );

	static MouseBindings::Shortcut sanitizeShortcut( const MouseBindings::Shortcut& shortcut );

	static std::string mousebindFormat( std::string str );

	static Shortcut toShortcut( const std::string& keys );

	static std::string fromShortcut( MouseBindings::Shortcut shortcut,
									 bool format = false );

	MouseBindings();

	void addMousebindsString( const std::map<std::string, std::string>& binds );

	void addMousebinds( const std::map<Shortcut, std::string>& binds );

	void addMousebindsStringUnordered( const std::unordered_map<std::string, std::string>& binds );

	void addMousebindsUnordered( const std::unordered_map<Shortcut, std::string>& binds );

	void addMousebindString( const std::string& key, const std::string& command );

	void addMousebind( const Shortcut& key, const std::string& command );

	/** If the command is already on the list, it will remove the previous keybind. */
	void replaceMousebindString( const std::string& keys, const std::string& command );

	/** If the command is already on the list, it will remove the previous keybind. */
	void replaceMousebind( const Shortcut& keys, const std::string& command );

	Shortcut getShortcutFromString( const std::string& keys );

	void removeMousebind( const Shortcut& keys );

	void removeMousebind( const std::string& kb );

	bool existsMousebind( const Shortcut& keys );

	bool hasCommand( const std::string& command );

	void removeCommandMousebind( const std::string& command );

	void removeCommandsMousebind( const std::vector<std::string>& command );

	std::string getCommandFromMousebind( const Shortcut& keys );

	std::string getCommandMousebindString( const std::string& command ) const;

	void reset();

	const ShortcutMap& getShortcutMap() const;

	const std::map<std::string, Shortcut> getMousebindings() const;

	Shortcut getShortcutFromCommand( const std::string& cmd ) const;

	std::string getShortcutString( Shortcut shortcut, bool format = false ) const;

  protected:
	ShortcutMap mShortcuts;
	std::map<std::string, Shortcut> mMousebindingsInvert;
};

}} // namespace EE::UI

namespace std {
template <> struct hash<EE::UI::MouseBindings::Shortcut> {
	size_t operator()( const EE::UI::MouseBindings::Shortcut& shortcut ) const noexcept {
		return hashCombine( static_cast<size_t>( shortcut.action ),
							static_cast<size_t>( shortcut.key ), shortcut.mod );
	}
};

} // namespace std
