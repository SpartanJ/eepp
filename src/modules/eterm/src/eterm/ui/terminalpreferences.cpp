#include <eterm/ui/terminalpreferences.hpp>

#include <eepp/system/inifile.hpp>
#include <eterm/ui/uiterminal.hpp>
#include <limits>

namespace eterm::UI {

using namespace EE;
using namespace EE::UI;
using namespace EE::System;
using namespace eterm::Terminal;

void TerminalPreferences::load( const IniFile& ini ) {
	Uint64 lines;
	if ( String::fromString( lines, ini.getValue( "terminal", "scrollback" ) ) )
		scrollback = lines;
	cursorStyle = TerminalCursorHelper::modeFromString( ini.getValue(
		"terminal", "cursor_style", TerminalCursorHelper::modeToString( cursorStyle ) ) );
	scrollBarType =
		ini.getValue( "terminal", "scrollbar_type",
					  scrollBarType == ScrollViewType::Overlay ? "overlay" : "outside" ) ==
				"outside"
			? ScrollViewType::Outside
			: ScrollViewType::Overlay;
	const auto mode = ini.getValue( "terminal", "scrollbar_mode" );
	if ( !mode.empty() ) {
		scrollBarMode = mode == "always_on" || mode == "on"		? ScrollBarMode::AlwaysOn
						: mode == "always_off" || mode == "off" ? ScrollBarMode::AlwaysOff
																: ScrollBarMode::Auto;
	}
	allowOsc52ClipboardWrite =
		ini.getValueB( "terminal", "allow_osc52_clipboard_write", allowOsc52ClipboardWrite );
	enableOsc8Hyperlinks =
		ini.getValueB( "terminal", "enable_osc8_hyperlinks", enableOsc8Hyperlinks );
	graphemeClustering = ini.getValueB( "terminal", "grapheme_clustering", graphemeClustering );
	detectLinks = ini.getValueB( "terminal", "detect_links", detectLinks );
}

void TerminalPreferences::save( IniFile& ini, bool shortScrollbarModes ) const {
	ini.setValue( "terminal", "scrollback", String::toString( scrollback ) );
	ini.setValue( "terminal", "cursor_style", TerminalCursorHelper::modeToString( cursorStyle ) );
	ini.setValue( "terminal", "scrollbar_type",
				  scrollBarType == ScrollViewType::Overlay ? "overlay" : "outside" );
	ini.setValue(
		"terminal", "scrollbar_mode",
		scrollBarMode == ScrollBarMode::AlwaysOn	? ( shortScrollbarModes ? "on" : "always_on" )
		: scrollBarMode == ScrollBarMode::AlwaysOff ? ( shortScrollbarModes ? "off" : "always_off" )
													: "auto" );
	ini.setValueB( "terminal", "allow_osc52_clipboard_write", allowOsc52ClipboardWrite );
	ini.setValueB( "terminal", "enable_osc8_hyperlinks", enableOsc8Hyperlinks );
	ini.setValueB( "terminal", "grapheme_clustering", graphemeClustering );
	ini.setValueB( "terminal", "detect_links", detectLinks );
}

size_t TerminalPreferences::historySize() const {
	return static_cast<size_t>(
		std::min<Uint64>( scrollback, std::numeric_limits<size_t>::max() ) );
}

TerminalSessionOptions TerminalPreferences::sessionOptions() const {
	return { allowOsc52ClipboardWrite, enableOsc8Hyperlinks, graphemeClustering };
}

void TerminalPreferences::applyInitial( UITerminal* terminal ) const {
	if ( !terminal )
		return;

	terminal->getTerm()->setCursorMode( cursorStyle );
	terminal->getTerm()->setLinkDetectionEnabled( detectLinks );
	terminal->setScrollViewType( scrollBarType );
	terminal->setVerticalScrollMode( scrollBarMode );
}

} // namespace eterm::UI
