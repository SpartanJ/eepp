#ifndef EEFM_TERMINALPANEL_HPP
#define EEFM_TERMINALPANEL_HPP

#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eterm/terminal/terminalcolorscheme.hpp>
#include <map>
#include <string>

namespace eterm { namespace UI {
class UITerminal;
}} // namespace eterm::UI

namespace eefm {

using namespace EE;
using namespace EE::Graphics;
using namespace EE::UI;

class App;

/** @brief The terminal panel (F4) below the panes, created on first use. Hiding it parks the
 * terminal: the shell keeps running until it is shown again. */
class TerminalPanel {
  public:
	/** @param splitter The vertical splitter the panel is shown in, below the files. */
	TerminalPanel( App& app, UISplitter* splitter );

	~TerminalPanel();

	void toggle();

	void show( bool show );

	/** Bind the configured toggle-terminal shortcuts on the terminal too: it takes every key, so
	 * the window's bindings never see them while it has focus. */
	void applyKeybindings();

	bool isVisible() const;

	/** Remembers the panel's share of the window, while it is shown. */
	void rememberSize();

	/** Share of the files above the terminal (persisted). */
	const std::string& partition() const { return mPartition; }

	void setPartition( std::string partition ) { mPartition = std::move( partition ); }

	/** The shell follows the active pane (cd), unless a full screen program runs. */
	void syncDirectory();

	/** Applies the Terminal appearance settings to the terminal, if any. */
	void applyConfig();

	/** The terminal color schemes bundled with eterm and the user's (lazily loaded). */
	const std::map<std::string, eterm::Terminal::TerminalColorScheme>& colorSchemes();

	/** The font in use, null until the terminal was first created. */
	FontTrueType* terminalFont() const { return mTerminalFont; }

	void setTerminalFont( FontTrueType* font );

	void setFontSize( Float sizeDp );

	/** Applies the font hinting and anti-aliasing settings to the terminal. */
	void syncFontRendering();

  private:
	bool create();

	void close();

	App& mApp;
	UISceneNode* mUI;
	UISplitter* mSplitter;
	eterm::UI::UITerminal* mTerminal{ nullptr };
	/** The terminal's own bindings, before the toggle shortcuts are added. */
	KeyBindings::ShortcutMap mTerminalBindings;
	FontTrueType* mTerminalFont{ nullptr };
	std::map<std::string, eterm::Terminal::TerminalColorScheme> mTerminalColorSchemes;
	/** Last directory the shell was sent to, and when (the shell needs a moment to run it). */
	std::string mTerminalDirectory;
	EE::System::Clock mTerminalDirectorySent;
	/** Active pane and folder last followed: the shell follows changes only, so a directory
	 * changed in the shell stays until the files move. */
	std::string mTerminalFollowed;
	std::string mPartition{ "70%" };
	bool mTerminalColorSchemesLoaded{ false };
};

} // namespace eefm

#endif
