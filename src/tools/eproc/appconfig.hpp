#ifndef EPROC_APPCONFIG_HPP
#define EPROC_APPCONFIG_HPP

#include <eepp/graphics/font.hpp>
#include <eepp/math/size.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/ui/colorschemepreferences.hpp>
#include <eepp/window/window.hpp>

#include <string>

using namespace EE;
using namespace EE::Math;
using namespace EE::System;
using namespace EE::Window;

namespace eproc {

struct WindowStateConfig {
	Sizei size{ 1280, 720 };
	Vector2i position{ -1, -1 };
	int displayIndex{ 0 };
	bool maximized{ false };
};

class AppConfig {
  public:
	static constexpr int MinRefreshIntervalMs = 250;
	static constexpr int MaxRefreshIntervalMs = 60000;

	explicit AppConfig( std::string configPath );

	void load();
	bool save();
	void captureWindowState( EE::Window::Window* window );

	const std::string& getConfigPath() const { return mConfigPath; }

	WindowStateConfig windowState;
	// Serialized process table columns, widths, and sorting state.
	std::string processTableState;
	bool divideCpuUsage{ false };
	bool performancePerCore{ false };
	bool treeView{ false };
	bool vsync{ false };
	int filterMode{ 0 };
	int refreshIntervalMs{ 1000 };
	Float pixelDensity{ 0 };
	FontHinting fontHinting{ FontHinting::Full };
	FontAntialiasing fontAntialiasing{ FontAntialiasing::Grayscale };
	EE::UI::ColorSchemeExtPreference colorScheme{ EE::UI::ColorSchemeExtPreference::System };
	Uint32 frameRateLimit{ ContextSettings::FrameRateLimitScreenRefreshRate };
	GraphicsLibraryVersion rendererVersion{ GLv_default };
	Uint32 multisamples{ 4 };

  private:
	std::string mConfigPath;
	IniFile mState;
};

} // namespace eproc

#endif // EPROC_APPCONFIG_HPP
