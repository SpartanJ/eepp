#ifndef EEFM_CONFIG_HPP
#define EEFM_CONFIG_HPP

#include <eepp/graphics/font.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/ui/colorschemepreferences.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/window/window.hpp>
#include <eterm/terminal/terminaltypes.hpp>

#include <string>

namespace eefm {

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

struct ThemeConfig {
	ColorSchemeExtPreference uiColorScheme{ ColorSchemeExtPreference::System };
};

struct FontConfig {
	/** Empty: the default UI font. */
	std::string uiPath;
	/** In dp; 0: the theme's size. */
	Float uiSize{ 0 };
	std::string fallbackPath;
	FontHinting hinting{ FontHinting::Full };
	FontAntialiasing antialiasing{ FontAntialiasing::Grayscale };
};

struct FilesConfig {
	/** Open files and folders with a single click (otherwise a double click). */
	bool singleClick{ false };
	/** Show how many items local folders have in the Size column. */
	bool countFolderItems{ true };
	/** Icons from the desktop icon theme (Linux, BSD) or the shell (Windows, macOS). */
	bool systemIcons{ true };
	bool smoothScroll{ false };
};

struct TerminalConfig {
	/** Empty: the bundled monospace font. */
	std::string fontPath;
	Float fontSize{ 11 };
	/** Empty: the default scheme. */
	std::string colorScheme;
	/** Empty: the user's shell. */
	std::string shell;
	std::string shellArguments;
	size_t historySize{ 10000 };
	eterm::Terminal::TerminalCursorMode cursorStyle{
		eterm::Terminal::TerminalCursorMode::SteadyBlock };
	ScrollBarMode scrollBarMode{ ScrollBarMode::Auto };
	/** The shell follows the active pane (cd), as Dolphin. */
	bool followActivePane{ true };
};

struct WindowConfig {
	/** 0: the display's (or the environment's) pixel density. */
	Float pixelDensity{ 0 };
	Uint32 maxFPS{ static_cast<Uint32>( ContextSettings::FrameRateLimitScreenRefreshRate ) };
	bool vsync{ false };
	GraphicsLibraryVersion rendererVersion{ GLv_default };
	Uint32 multisamples{ 0 };
};

/** @brief eefm preferences (config.cfg in the eefm config folder). Session state (window, tabs,
 * columns) is kept apart, in state.json. */
struct AppConfig {
	ThemeConfig theme;
	FontConfig font;
	FilesConfig files;
	TerminalConfig terminal;
	WindowConfig window;

	explicit AppConfig( std::string configDirectory );

	void load();

	bool save();

	const std::string& path() const { return mPath; }

  private:
	std::string mPath;
	IniFile mIni;
};

} // namespace eefm

#endif
