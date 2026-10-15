#ifndef EE_WINDOW_DRAGANDDROP_HPP
#define EE_WINDOW_DRAGANDDROP_HPP

#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/math/vector2.hpp>
#include <functional>
#include <string>

namespace EE { namespace Graphics {
class Image;
}} // namespace EE::Graphics

namespace EE { namespace Window {

class Window;

enum class DragAction : Uint8 { None = 0, Copy = 1, Move = 2, Link = 4, Ask = 8 };
using DragActions = Uint32;

enum class DragEndReason : Uint8 { Dropped, Cancelled, Unsupported, Failed };

enum class DragStartStatus : Uint8 {
	Started,
	Busy,
	InvalidOffer,
	InvalidGesture,
	Unsupported,
	Failed
};

struct DragFile {
	/** Absolute local filesystem path, encoded as UTF-8. */
	std::string path;
};

struct DragPreview {
	/** Optional RGB/RGBA snapshot, borrowed only during startDrag(). If absent, a file/count icon
	 * is generated. The native session copies the pixels; it never retains a UI widget. */
	const Graphics::Image* image{ nullptr };
	/** Pointer position relative to the image's top-left corner, in desktop pixels (-2048..2048).
	 */
	Math::Vector2i hotspot{ -12, -16 };
};

struct DragOffer {
	SmallVector<DragFile, 1> files;
	DragActions actions{ static_cast<DragActions>( DragAction::Copy ) };
	DragPreview preview;
	/** The application completes drops on its own window (DragResult::releasedOverSource on
	 * X11): the preview shows a pending drop there instead of a rejected one. */
	bool sourceHandlesDrops{ false };
};

struct DragResult {
	std::string error;
	DragEndReason reason{ DragEndReason::Cancelled };
	DragAction performedAction{ DragAction::None };
	/** The gesture was released over its own source window, which X11 never offers as a native
	 * target: the session ends Cancelled, and the application can complete it as an internal
	 * drop. Elsewhere the source window receives ordinary FileDropped events instead. */
	bool releasedOverSource{ false };
};

struct DragStartResult {
	std::string error;
	DragStartStatus status{ DragStartStatus::Unsupported };
};

struct DragCapabilities {
	bool fileSource{ false };
	bool dataSource{ false };
	/** Legacy SDL file reception; does not imply MIME/action negotiation. */
	bool fileTarget{ false };
	bool negotiatedTarget{ false };
	bool dragImage{ false };
};

using DragCompletionCallback = std::function<void( DragResult )>;

/** Native cross-application file dragging. All methods must run on the window/UI thread.
 * This lightweight facade borrows its Window and must not outlive it. A started session owns its
 * serialized payload and completion callback. Rejected starts never invoke completion.
 * Completions run once, outside native event hooks, when input is updated; they may start another
 * drag. Cancellation/closing a source window also completes the session. Callbacks must not borrow
 * objects that may be destroyed before completion (use weak ownership when appropriate).
 * Engine teardown cancels and releases pending sessions without invoking their callbacks.
 * Initially only Copy is supported. Dropped means the native transfer finished, not that a browser
 * uploaded or a file manager copied a file. Existing internal UI dragging is independent.
 */
class EE_API DragAndDrop {
  public:
	static DragAndDrop forWindow( Window& window );

	DragCapabilities getCapabilities() const;

	/** Call after a movement threshold, while the initiating left mouse button is still down.
	 * Preview images must be RGB/RGBA, at most 2048 pixels per side and 1,048,576 pixels total.
	 * The offer is read synchronously; the session retains only its own native representation. */
	DragStartResult startDrag( const DragOffer& offer, DragCompletionCallback completion = {} );

	void cancelDrag();

	bool isDragging() const;

	/** Serialize an offer as text/uri-list (CRLF). Rejects empty, relative, invalid UTF-8 and NUL
	 * paths. Preserves order and duplicate entries. On failure output is empty. */
	static bool encodeFileUriList( const DragOffer& offer, std::string& output,
								   std::string* error = nullptr );

  private:
	explicit DragAndDrop( Window& window );

	Window* mWindow;
};

}} // namespace EE::Window

#endif
