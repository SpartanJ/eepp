#ifndef EE_WINDOW_DISPLAYMANAGER_HPP
#define EE_WINDOW_DISPLAYMANAGER_HPP

#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/window/window.hpp>
using namespace EE::Math;
using namespace EE::Graphics;

namespace EE { namespace Window {

class DisplayManager;

class EE_API Display {
  public:
	Display( int displayIndex );

	virtual std::string getName() const = 0;

	virtual Rect getBounds() const = 0;

	virtual Rect getUsableBounds() const = 0;

	virtual Float getDPI() = 0;

	/** Physical pixels per inch, independent of the desktop UI scale; zero if unavailable. */
	virtual Float getPhysicalDPI();

	/** Native identity suitable for persisted settings, or empty when unavailable. */
	virtual std::string getPersistentId() const;

	virtual const int& getIndex() const = 0;

	virtual DisplayMode getCurrentMode() const = 0;

	virtual DisplayMode getClosestDisplayMode( DisplayMode wantedMode ) const = 0;

	virtual const std::vector<DisplayMode>& getModes() const = 0;

	virtual Uint32 getRefreshRate() const = 0;

	virtual Sizeu getSize() const = 0;

	/** Native panel dimensions in pixels when available, otherwise the current display mode. */
	Sizeu getNativePixelSize() const;

	virtual ~Display();

	PixelDensitySize getPixelDensitySize();

	Float getPixelDensity();

  protected:
	int index;
	mutable std::vector<DisplayMode> displayModes;

	/** Persistent identity from the display's label, for backends without native monitor
	 * identities such as Wayland. Empty unless the label is meaningful and unique among the
	 * manager's displays. */
	std::string getLabelPersistentId( DisplayManager* manager ) const;
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
