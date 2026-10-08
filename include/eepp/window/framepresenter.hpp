#ifndef EE_WINDOW_FRAMEPRESENTER_HPP
#define EE_WINDOW_FRAMEPRESENTER_HPP

#include <eepp/math/size.hpp>

namespace EE { namespace Window {

class Window;

/** Presentation boundary used to export completed Window frames to an external transport. */
class FramePresenter {
  public:
	virtual ~FramePresenter() = default;

	/** Initializes the presenter for a window.
	 * @return True when presentation can begin.
	 */
	virtual bool initialize( Window& ) = 0;

	/** Queues or presents the window's completed frame. */
	virtual void present( Window& ) = 0;

	/** Notifies the presenter that the logical framebuffer size changed. */
	virtual void resized( Window&, const Math::Sizei& ) {}

	/** Releases resources associated with the window. */
	virtual void shutdown( Window& ) {}
};

}} // namespace EE::Window

#endif
