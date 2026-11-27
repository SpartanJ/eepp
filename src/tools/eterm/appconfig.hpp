#ifndef ETERM_CONFIG_HPP
#define ETERM_CONFIG_HPP

#include <eepp/graphics/font.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/math/size.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/thirdparty/nlohmann/json_fwd.hpp>
#include <eepp/ui/colorschemepreferences.hpp>
#include <eepp/ui/uidirection.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/window/window.hpp>
#include <eterm/terminal/programstatusalerts.hpp>
#include <eterm/ui/terminalpreferences.hpp>
#include <string>
#include <string_view>

namespace eterm {

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Math;
using namespace EE::System;
using namespace EE::UI;
using EE::Window::DisplayDensityOverrides;
using EE::Window::PixelDensityMode;

enum class NewTerminalBehavior { CurrentTabBar, VerticalSplit, HorizontalSplit };

struct TerminalConfig : eterm::UI::TerminalPreferences {

	std::string shell;
	std::string shellArguments;
	std::string defaultWorkingDirectory;
	std::string executeInShell;
	Terminal::ProgramStatusPreferences programStatus;
	size_t initialTabs{ 1 };
	bool useFrameBuffer{ false };
	bool closeOnExit{ true };
	bool exclusiveMode{ false };
	bool inheritWorkingDirectory{ true };
	bool useCustomWorkingDirectory{ false };
	bool restoreLastSession{ true };
	bool recoverHarnessSessions{ false };
	NewTerminalBehavior newTerminalBehavior{ NewTerminalBehavior::CurrentTabBar };
};

struct UIConfig {
	bool smoothScroll{ false };
};

struct QuakeConfig {
	static constexpr int MinSizePercent = 10;
	static constexpr int MaxSizePercent = 100;
	static constexpr int MaxAnimationMs = 1000;

	std::string shortcut{ "mod+shift+f11" };
	std::string secondaryShortcut;
	int widthPercent{ MaxSizePercent };
	int heightPercent{ 90 };
	int animationMs{ 160 };
	UIHorizontalAlignment horizontalAlignment{ UIHorizontalAlignment::Center };
	bool launchHidden{ true };
	bool hideOnFocusLoss{ false };
};

struct FontConfig {
	std::string path;
	std::string fallbackPath;
	FontHinting hinting{ FontHinting::Full };
	FontAntialiasing antialiasing{ FontAntialiasing::Grayscale };
	Float size{ 11 };
	std::string uiPath;
	/** In dp; 0: the theme's size. */
	Float uiSize{ 0 };
};

struct WindowConfig {
	Float pixelDensity{ 0 };
	PixelDensityMode pixelDensityMode{ PixelDensityMode::Default };
	DisplayDensityOverrides displayDensityOverrides;
	Uint32 maxFPS{ static_cast<Uint32>( ContextSettings::FrameRateLimitScreenRefreshRate ) };
	bool vsync{ false };
	bool benchmarkMode{ false };
	bool warnBeforeClose{ false };
	bool exitOnClose{ EE_PLATFORM != EE_PLATFORM_MACOS };
	bool alwaysShowTabBar{ false };
	bool showTabBarWhenSplit{ true };
	UITabWidget::TabPosition tabPosition{ UITabWidget::TabPosition::Top };
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
	Float tabBarWidth{ 200.f };
	bool maximized{ false };
};

struct AppConfig {
	TerminalConfig terminal;
	FontConfig font;
	WindowConfig window;
	ThemeConfig theme;
	UIConfig ui;
	WindowStateConfig windowState;
	QuakeConfig quake;

	explicit AppConfig( std::string configPath, bool quakeMode = false );

	bool isQuakeMode() const { return mQuakeMode; }

	void load();

	bool save();

	bool savePreferences();

	bool saveWindowState();

	/** Record the first unavailable-backend notice in Quake state. */
	bool takeQuakeShortcutUnavailableNotice();

	void captureWindowState( EE::Window::Window* window );

	/** Hands over the saved session and removes it, so another instance won't open it again.
	 * @return null without a readable session. */
	nlohmann::json takeSession();

	bool saveSession( const nlohmann::json& session );

	const std::string& getConfigPath() const { return mConfigPath; }

	const std::string& getStateDirectory() const { return mStateDirectory; }

  private:
	std::string sessionPath() const { return mStateDirectory + "session.json"; }

	std::string_view tabPreferencesSection() const;

	std::string_view restorePreferencesSection() const;

	IniFile preferences() const;

	std::string mConfigPath;
	std::string mStateDirectory;
	IniFile mSavedPreferences;
	IniFile mState;
	bool mQuakeMode;
};

} // namespace eterm

#endif
