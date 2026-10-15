#ifndef EE_WINDOW_NATIVE_DRAG_PREVIEW_HPP
#define EE_WINDOW_NATIVE_DRAG_PREVIEW_HPP

#include <array>
#include <cstddef>
#include <eepp/config.hpp>

namespace EE { namespace Graphics {
class Image;
}} // namespace EE::Graphics

namespace EE { namespace Window { namespace Private {

enum class DragFeedback : Uint8 { Rejected, Pending, Copy };

/** Backend presentation for the single process-wide drag. The transport must reset it
 * before releasing its source window. Backend resources never outlive reset(). */
class NativeDragPreview {
  public:
	virtual ~NativeDragPreview() = default;

	virtual void move( int x, int y ) = 0;

	virtual void updatePosition() = 0;

	virtual void setFeedback( DragFeedback feedback ) = 0;

	virtual void reset() = 0;

  protected:
	using FileImagePixels = std::array<Uint8, 112 * 48 * 4>;
	using CopyCursorPixels = std::array<Uint8, 24 * 24 * 4>;
	using FeedbackImagePixels = std::array<Uint8, 16 * 16 * 4>;

	/** Images borrow the supplied scratch storage and must not outlive it. */
	static Graphics::Image makeFileImage( FileImagePixels& pixels, size_t count );

	static Graphics::Image makeCopyCursorImage( CopyCursorPixels& pixels );

	static Graphics::Image makeFeedbackImage( FeedbackImagePixels& pixels, DragFeedback feedback );
};

}}} // namespace EE::Window::Private

#endif
