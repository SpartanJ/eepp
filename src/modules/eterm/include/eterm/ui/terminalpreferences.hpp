#ifndef ETERM_UI_TERMINALPREFERENCES_HPP
#define ETERM_UI_TERMINALPREFERENCES_HPP

#include <eterm/config.hpp>

#include <eepp/ui/uiscrollview.hpp>
#include <eterm/terminal/terminaltypes.hpp>

namespace EE::System {

class IniFile;

} // namespace EE::System

namespace eterm::Terminal {

struct TerminalSessionOptions;

} // namespace eterm::Terminal

namespace eterm::UI {

class UITerminal;

struct EE_ETERM_API TerminalPreferences {
	EE::Uint64 scrollback{ 10000 };
	Terminal::TerminalCursorMode cursorStyle{ Terminal::TerminalCursorMode::SteadyUnderline };
	EE::UI::ScrollViewType scrollBarType{ EE::UI::ScrollViewType::Overlay };
	EE::UI::ScrollBarMode scrollBarMode{ EE::UI::ScrollBarMode::Auto };
	bool allowOsc52ClipboardWrite{ true };
	bool enableOsc8Hyperlinks{ true };
	bool graphemeClustering{ true };
	bool detectLinks{ true };

	void load( const EE::System::IniFile& ini );

	void save( EE::System::IniFile& ini, bool shortScrollbarModes = false ) const;

	size_t historySize() const;

	Terminal::TerminalSessionOptions sessionOptions() const;

	void applyInitial( UITerminal* terminal ) const;
};

} // namespace eterm::UI

#endif
