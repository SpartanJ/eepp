#ifndef ETERM_CONFIG_HPP
#define ETERM_CONFIG_HPP

#include <eepp/graphics/font.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/math/size.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/ui/colorschemepreferences.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/window/window.hpp>
#include <eterm/terminal/terminaltypes.hpp>
#include <string>

namespace eterm {

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Math;
using namespace EE::System;
using namespace EE::UI;

enum class NewTerminalBehavior { CurrentTabBar, VerticalSplit, HorizontalSplit };

struct TerminalConfig {
	std::string shell;
	std::string shellArguments;
	std::string workingDirectory;
	std::string executeInShell;
	size_t historySize{ 10000 };
	size_t initialTabs{ 1 };
	Terminal::TerminalCursorMode cursorStyle{ Terminal::TerminalCursorMode::SteadyUnderline };
	bool useFrameBuffer{ false };
	bool closeOnExit{ false };
	bool exclusiveMode{ false };
	NewTerminalBehavior newTerminalBehavior{ NewTerminalBehavior::CurrentTabBar };
	ScrollViewType scrollBarType{ ScrollViewType::Overlay };
	ScrollBarMode scrollBarMode{ ScrollBarMode::Auto };
};

struct FontConfig {
	std::string path;
	std::string fallbackPath;
	FontHinting hinting{ FontHinting::Full };
	FontAntialiasing antialiasing{ FontAntialiasing::Grayscale };
	Float size{ 11 };
	std::string uiPath;
	Float uiSize{ 11 };
};

struct WindowConfig {
	Float pixelDensity{ 0 };
	Uint32 maxFPS{ static_cast<Uint32>( ContextSettings::FrameRateLimitScreenRefreshRate ) };
	bool vsync{ false };
	bool benchmarkMode{ false };
	bool warnBeforeClose{ false };
	bool alwaysShowTabBar{ false };
	GraphicsLibraryVersion rendererVersion{ GLv_default };
	Uint32 multisamples{ 0 };
};

struct ThemeConfig {
	std::string colorScheme;
	ColorSchemeExtPreference uiColorScheme{ ColorSchemeExtPreference::System };
};

struct WindowStateConfig {
	Sizei size{ 1280, 720 };
	Vector2i position{ -1, -1 };
	int displayIndex{ 0 };
	bool maximized{ false };
};

struct AppConfig {
	TerminalConfig terminal;
	FontConfig font;
	WindowConfig window;
	ThemeConfig theme;
	WindowStateConfig windowState;

	explicit AppConfig( std::string configPath );

	void load();

	bool save();

	bool savePreferences();

	bool saveWindowState();

	void captureWindowState( EE::Window::Window* window );

	const std::string& getConfigPath() const { return mConfigPath; }

  private:
	std::string mConfigPath;
	IniFile mIni;
	IniFile mState;
};

} // namespace eterm

#endif
