#ifndef EE_WINDOW_WAYLAND_DRAG_PREVIEW_HPP
#define EE_WINDOW_WAYLAND_DRAG_PREVIEW_HPP

#include "waylanddraganddrop.hpp"

#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
#include "dragpreview.hpp"
#include "waylandlibrary.hpp"

namespace EE { namespace Window { namespace Private {

/** A compositor-owned drag-icon role, independent of SDL popup positioning or pointer capture. */
class WaylandDragPreview final : public NativeDragPreview {
  public:
	explicit WaylandDragPreview( WaylandLibrary& library ) : library( library ) {}

	~WaylandDragPreview() override;

	bool initialize( wl_compositor* compositor, wl_subcompositor* subcompositor, wl_shm* shm,
					 const DragOffer& offer, float scale );

	wl_surface* surface() const { return icon; }

	void move( int, int ) override {}

	void updatePosition() override {}

	void setFeedback( DragFeedback feedback ) override;

	void reset() override;

  private:
	WaylandLibrary& library;
	wl_surface* icon{ nullptr };
	wl_surface* badge{ nullptr };
	wl_subsurface* badgeRole{ nullptr };
	wl_buffer* buffer{ nullptr };
	std::array<wl_buffer*, 3> feedbackBuffers{};
	DragFeedback feedback{ DragFeedback::Pending };
};

}}} // namespace EE::Window::Private
#endif
#endif
