#ifndef EE_WINDOW_DISPLAYMANAGER_HPP
#define EE_WINDOW_DISPLAYMANAGER_HPP

#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/window/window.hpp>
using namespace EE::Math;
using namespace EE::Graphics;

namespace EE { namespace Window {

class EE_API Display {
  public:
	Display( int displayIndex );

	virtual std::string getName() = 0;

	virtual Rect getBounds() = 0;

	virtual Rect getUsableBounds() = 0;

	virtual Float getDPI() = 0;

	virtual const int& getIndex() const = 0;

	virtual DisplayMode getCurrentMode() = 0;

	virtual DisplayMode getClosestDisplayMode( DisplayMode wantedMode ) = 0;

	virtual const std::vector<DisplayMode>& getModes() const = 0;

	virtual ~Display();

	PixelDensitySize getPixelDensitySize();

	Float getPixelDensity();

  protected:
	int index;
	mutable std::vector<DisplayMode> displayModes;
};

class EE_API DisplayManager {
  public:
	virtual int getDisplayCount() = 0;

	virtual Display* getDisplayIndex( int index ) = 0;

	virtual void enableScreenSaver();

	virtual void disableScreenSaver();

	virtual void enableMouseFocusClickThrough();

	virtual void disableMouseFocusClickThrough();

	virtual void disableBypassCompositor();

	virtual void enableBypassCompositor();

	virtual ~DisplayManager();

  protected:
	std::vector<Display*> displays;
};

}} // namespace EE::Window

#endif
