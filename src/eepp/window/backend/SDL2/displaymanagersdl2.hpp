#ifndef EE_WINDOW_DISPLAYMANAGERSDL2_HPP
#define EE_WINDOW_DISPLAYMANAGERSDL2_HPP

#include <eepp/window/displaymanager.hpp>
#include <optional>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API DisplaySDL2 : public Display {
  public:
	DisplaySDL2( int index );

	std::string getName() const;

	Rect getBounds() const;

	Rect getUsableBounds() const;

	Float getDPI();
	Float getPhysicalDPI();

	std::string getPersistentId() const;

	const int& getIndex() const;

	DisplayMode getCurrentMode() const;

	DisplayMode getClosestDisplayMode( DisplayMode wantedMode ) const;

	const std::vector<DisplayMode>& getModes() const;

	Uint32 getRefreshRate() const;

	Sizeu getSize() const;

  protected:
	friend class DisplayManagerSDL2;

	void invalidateMetrics();

	Sizei mPhysicalDPIResolution;
	Float mPhysicalDPI{ -1.f };
};

class EE_API DisplayManagerSDL2 : public DisplayManager {
  public:
#if EE_PLATFORM == EE_PLATFORM_WIN
	static void setDPIAwareness();
#endif
#if defined( EE_X11_PLATFORM )
	Float getX11DesktopScale( eeWindowHandle display );
#endif

	int getDisplayCount();

	Display* getDisplayIndex( int index );

	void invalidateDisplayMetrics();

	void enableScreenSaver();

	void disableScreenSaver();

	void enableMouseFocusClickThrough();

	void disableMouseFocusClickThrough();

	void disableBypassCompositor();

	void enableBypassCompositor();

  protected:
	int mDisplayCount{ 0 };
#if defined( EE_X11_PLATFORM )
	eeWindowHandle mX11Display{};
	std::optional<Float> mX11DesktopScale;
#endif
};

}}}} // namespace EE::Window::Backend::SDL2

#endif
