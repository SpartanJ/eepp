#include "dragpreviewsdl3.hpp"
#include <eepp/graphics/image.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/window.hpp>
#include <utility>

#ifdef EE_BACKEND_SDL3
namespace EE { namespace Window { namespace Backend { namespace SDL3 {

using Private::DragFeedback;

namespace {
// SDL's software-window surface can use an accelerated framebuffer internally. Preserve the
// application's GL context when uploading/presenting the independent preview window.
class RestoreGLContext {
  public:
	RestoreGLContext() :
		window( SDL_GL_GetCurrentWindow() ), context( SDL_GL_GetCurrentContext() ) {}

	~RestoreGLContext() {
		if ( window &&
			 ( SDL_GL_GetCurrentContext() != context || SDL_GL_GetCurrentWindow() != window ) )
			SDL_GL_MakeCurrent( window, context );
	}

  private:
	SDL_Window* window;
	SDL_GLContext context;
};
} // namespace

DragPreviewSDL::~DragPreviewSDL() {
	reset();
}

Private::NativeDragPreview* DragPreviewSDL::create( Window* owner, SDL_Window* parent,
													const DragOffer& offer, bool manageCursor ) {
	static DragPreviewSDL preview;
	if ( !preview.initialize( owner, parent, offer, manageCursor ) ) {
		preview.reset();
		return nullptr;
	}
	return &preview;
}

bool DragPreviewSDL::initialize( Window* owner, SDL_Window* parent, const DragOffer& offer,
								 bool manageCursor ) {
	RestoreGLContext restore;
	mOwner = owner;
	mParent = parent;
	if ( !mParent )
		return false;
	mHotspot = offer.preview.hotspot;
	// Only the fallback uses this 21,504-byte stack buffer. The Image borrows its pixels;
	// SDL copies them into the window surface before this call returns.
	FileImagePixels pixels;
	Graphics::Image generated;
	const auto* image = offer.preview.image;
	if ( !image ) {
		generated = makeFileImage( pixels, offer.files.size() );
		image = &generated;
	}
	const int width = image->getWidth();
	const int height = image->getHeight();
	mWindow = SDL_CreatePopupWindow( mParent, 0, 0, width, height,
									 SDL_WINDOW_TOOLTIP | SDL_WINDOW_HIDDEN );
	if ( !mWindow )
		return false;
	auto* destination = SDL_GetWindowSurface( mWindow );
	if ( !destination || !SDL_FillSurfaceRect( destination, nullptr,
											   SDL_MapSurfaceRGB( destination, 35, 43, 57 ) ) ) {
		return false;
	}
	bool copied;
	if ( !offer.preview.image && destination->w == width && destination->h == height ) {
		copied =
			SDL_ConvertPixels( width, height, SDL_PIXELFORMAT_RGBA32, image->getPixels(), width * 4,
							   destination->format, destination->pixels, destination->pitch );
	} else {
		const auto channels = image->getChannels();
		const auto format = channels == 4 ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24;
		auto* surface =
			SDL_CreateSurfaceFrom( width, height, format, image->getPixels(), width * channels );
		if ( !surface )
			return false;
		copied =
			SDL_BlitSurfaceScaled( surface, nullptr, destination, nullptr, SDL_SCALEMODE_NEAREST );
		SDL_DestroySurface( surface );
	}
	if ( !copied )
		return false;
	if ( manageCursor ) {
		mCursors[0] = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_NOT_ALLOWED );
		mCursors[1] = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_PROGRESS );
		mCursors[2] = createCopyCursor();
		if ( !mCursors[0] || !mCursors[1] || !mCursors[2] )
			return false;
	}
	updatePosition();
	SDL_ShowWindow( mWindow );
	SDL_UpdateWindowSurface( mWindow );
	applyCursor();
	return true;
}

SDL_Cursor* DragPreviewSDL::createCopyCursor() {
	// The native cursor copies these 2,304 bytes during creation.
	CopyCursorPixels pixels;
	auto image = makeCopyCursorImage( pixels );
	auto* surface =
		SDL_CreateSurfaceFrom( image.getWidth(), image.getHeight(), SDL_PIXELFORMAT_RGBA32,
							   image.getPixels(), image.getWidth() * 4 );
	if ( !surface )
		return nullptr;
	auto* cursor = SDL_CreateColorCursor( surface, 0, 0 );
	SDL_DestroySurface( surface );
	return cursor;
}

void DragPreviewSDL::move( int x, int y ) {
	if ( !mWindow )
		return;
	int parentX, parentY;
	SDL_GetWindowPosition( mParent, &parentX, &parentY );
	x -= parentX;
	y -= parentY;
	x -= mHotspot.x;
	y -= mHotspot.y;
	if ( !mPositioned || mLastPosition != Vector2i( x, y ) ) {
		RestoreGLContext restore;
		mLastPosition = { x, y };
		mPositioned = true;
		SDL_SetWindowPosition( mWindow, x, y );
		// Re-present stored pixels after a move/exposure; the widget/image is never re-rendered.
		SDL_UpdateWindowSurface( mWindow );
	}
	applyCursor();
}

void DragPreviewSDL::updatePosition() {
	float x, y;
	SDL_GetGlobalMouseState( &x, &y );
	move( static_cast<int>( x ), static_cast<int>( y ) );
}

void DragPreviewSDL::setFeedback( DragFeedback feedback ) {
	mFeedback = feedback;
	applyCursor();
}

void DragPreviewSDL::applyCursor() {
	auto* cursor = mCursors[static_cast<size_t>( mFeedback )];
	if ( cursor ) {
		if ( SDL_GetCursor() != cursor )
			SDL_SetCursor( cursor );
		if ( !SDL_CursorVisible() )
			SDL_ShowCursor();
	}
}

void DragPreviewSDL::reset() {
	const bool managedCursor = mCursors[0] != nullptr;
	// Destroying the window pumps native events, which can reach reset() again: detach it first.
	if ( auto* window = std::exchange( mWindow, nullptr ) ) {
		RestoreGLContext restore;
		SDL_DestroyWindow( window );
	}
	for ( auto*& cursor : mCursors ) {
		if ( cursor )
			SDL_DestroyCursor( cursor );
		cursor = nullptr;
	}
	if ( managedCursor && mOwner ) {
		// Restore the cursor currently requested by the application, including changes made
		// during the drag, without retaining a borrowed cursor that might have been deleted.
		// Do this last: destroying the popup/current native cursor can reset SDL's cursor.
		mOwner->getCursorManager()->reload();
	}
	mWindow = nullptr;
	mParent = nullptr;
	mOwner = nullptr;
	mPositioned = false;
	mFeedback = DragFeedback::Rejected;
}

}}}} // namespace EE::Window::Backend::SDL3
#endif
