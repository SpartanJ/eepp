#ifndef EE_WINDOW_DISPLAYMANAGERSDL2_HPP
#define EE_WINDOW_DISPLAYMANAGERSDL2_HPP

#include <eepp/window/displaymanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API DisplaySDL2 : public Display {
  public:
	DisplaySDL2( int index );

	std::string getName() const;

	Rect getBounds() const;

	Rect getUsableBounds() const;

	Float getDPI();

	const int& getIndex() const;

	DisplayMode getCurrentMode() const;

	DisplayMode getClosestDisplayMode( DisplayMode wantedMode ) const;

	const std::vector<DisplayMode>& getModes() const;

	Uint32 getRefreshRate() const;

	Sizeu getSize() const;
};

class EE_API DisplayManagerSDL2 : public DisplayManager {
  public:
#if EE_PLATFORM == EE_PLATFORM_WIN
	static void setDPIAwareness();
#endif

	int getDisplayCount();

	Display* getDisplayIndex( int index );

	void enableScreenSaver();

	void disableScreenSaver();

	void enableMouseFocusClickThrough();

	void disableMouseFocusClickThrough();

	void disableBypassCompositor();

	void enableBypassCompositor();
};

}}}} // namespace EE::Window::Backend::SDL2

#endif
