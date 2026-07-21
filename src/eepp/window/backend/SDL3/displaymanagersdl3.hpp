#ifndef EE_WINDOW_DISPLAYMANAGERSDL3_HPP
#define EE_WINDOW_DISPLAYMANAGERSDL3_HPP

#include <eepp/window/backend/SDL3/base.hpp>
#include <eepp/window/displaymanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API DisplaySDL3 : public Display {
  public:
	DisplaySDL3( int index, SDL_DisplayID displayId );

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

  protected:
	mutable std::vector<DisplayMode> displayModes;
	SDL_DisplayID mDisplayId;
};

class EE_API DisplayManagerSDL3 : public DisplayManager {
  public:
	int getDisplayCount();

	Display* getDisplayIndex( int index );

	void enableScreenSaver();

	void disableScreenSaver();

	void enableMouseFocusClickThrough();

	void disableMouseFocusClickThrough();

	void disableBypassCompositor();

	void enableBypassCompositor();

	int getDisplayIndexFromID( SDL_DisplayID id ) const;

  protected:
	std::vector<SDL_DisplayID> mDisplayIds;
};

}}}} // namespace EE::Window::Backend::SDL3

#endif
