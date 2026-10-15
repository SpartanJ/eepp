#ifndef EE_BACKEND_SDL3_DRAG_PREVIEW_HPP
#define EE_BACKEND_SDL3_DRAG_PREVIEW_HPP

#include <eepp/window/backend/SDL3/base.hpp>
#include <eepp/window/draganddrop/dragpreview.hpp>

#ifdef EE_BACKEND_SDL3
#include <array>
#include <eepp/window/draganddrop.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

/** SDL3 snapshot presentation. A single backend instance serves
 * DragManager's one active session; no per-window storage or extra gesture allocation is needed. */
class DragPreviewSDL : public Private::NativeDragPreview {
  public:
	DragPreviewSDL( const DragPreviewSDL& ) = delete;

	DragPreviewSDL& operator=( const DragPreviewSDL& ) = delete;

	static Private::NativeDragPreview* create( Window* owner, SDL_Window* parent,
											   const DragOffer& offer, bool manageCursor );

	~DragPreviewSDL() override;

	void move( int x, int y ) override;

	void updatePosition() override;

	void setFeedback( Private::DragFeedback feedback ) override;

	void reset() override;

  private:
	DragPreviewSDL() = default;

	bool initialize( Window* owner, SDL_Window* parent, const DragOffer& offer, bool manageCursor );

	static SDL_Cursor* createCopyCursor();

	void applyCursor();

	SDL_Window* mWindow{ nullptr };
	SDL_Window* mParent{ nullptr };
	Window* mOwner{ nullptr };
	std::array<SDL_Cursor*, 3> mCursors{};
	Math::Vector2i mHotspot;
	Math::Vector2i mLastPosition;
	Private::DragFeedback mFeedback{ Private::DragFeedback::Rejected };
	bool mPositioned{ false };
};

}}}} // namespace EE::Window::Backend::SDL3
#endif

#endif
