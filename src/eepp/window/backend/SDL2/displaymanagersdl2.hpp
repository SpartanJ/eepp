#ifndef EE_WINDOW_DISPLAYMANAGERSDL2_HPP
#define EE_WINDOW_DISPLAYMANAGERSDL2_HPP

#include <eepp/window/displaymanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API DisplaySDL2 : public Display {
  public:
	DisplaySDL2( int index );

	std::string getName();

	Rect getBounds();

	Rect getUsableBounds();

	Float getDPI();

	const int& getIndex() const;

	DisplayMode getCurrentMode();

	DisplayMode getClosestDisplayMode( DisplayMode wantedMode );

	const std::vector<DisplayMode>& getModes() const;
};

class EE_API DisplayManagerSDL2 : public DisplayManager {
  public:
	int getDisplayCount();

	Display* getDisplayIndex( int index );

	void enableScreenSaver();

	void disableScreenSaver();
};

}}}} // namespace EE::Window::Backend::SDL2

#endif
