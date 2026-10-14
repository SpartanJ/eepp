#include "dragpreviewsdl2.hpp"
#include <eepp/graphics/image.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/window.hpp>
#if EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/window/backend/SDL2/syswm.hpp>
#endif

#ifdef EE_BACKEND_SDL2
namespace EE { namespace Window { namespace Backend { namespace SDL2 {

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
	mWindow =
		SDL_CreateWindow( "eepp drag preview", 0, 0, width, height,
						  SDL_WINDOW_TOOLTIP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SKIP_TASKBAR |
							  SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_HIDDEN );
	if ( !mWindow )
		return false;
#if EE_PLATFORM == EE_PLATFORM_WIN
	SDL_SysWMinfo info{};
	SDL_VERSION( &info.version );
	if ( !SDL_GetWindowWMInfo( mWindow, &info ) || info.subsystem != SDL_SYSWM_WINDOWS )
		return false;
	auto window = info.info.win.window;
	// SDL2 exposes no input-transparent tooltip API on Windows. Layered + transparent lets
	// Windows hit testing reach targets underneath, including targets in another process.
	const auto style = GetWindowLongPtrW( window, GWL_EXSTYLE );
	SetLastError( ERROR_SUCCESS );
	if ( !SetWindowLongPtrW( window, GWL_EXSTYLE,
							 style | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED |
								 WS_EX_TRANSPARENT ) &&
		 GetLastError() != ERROR_SUCCESS )
		return false;
	if ( !SetLayeredWindowAttributes( window, 0, 255, LWA_ALPHA ) ||
		 !SetWindowPos( window, nullptr, 0, 0, 0, 0,
						SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
							SWP_FRAMECHANGED ) )
		return false;
#endif
	auto* destination = SDL_GetWindowSurface( mWindow );
	if ( !destination || SDL_FillRect( destination, nullptr,
									   SDL_MapRGB( destination->format, 35, 43, 57 ) ) != 0 ) {
		return false;
	}
	bool copied;
	if ( !offer.preview.image && destination->w == width && destination->h == height ) {
		copied = SDL_ConvertPixels( width, height, SDL_PIXELFORMAT_RGBA32, image->getPixels(),
									width * 4, destination->format->format, destination->pixels,
									destination->pitch ) == 0;
	} else {
		const auto channels = image->getChannels();
		const auto format = channels == 4 ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24;
		auto* surface = SDL_CreateRGBSurfaceWithFormatFrom(
			image->getPixels(), width, height, channels * 8, width * channels, format );
		if ( !surface )
			return false;
		copied = SDL_BlitScaled( surface, nullptr, destination, nullptr ) == 0;
		SDL_FreeSurface( surface );
	}
	if ( !copied )
		return false;
	if ( manageCursor ) {
		mCursors[0] = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_NO );
		mCursors[1] = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_WAITARROW );
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
		SDL_CreateRGBSurfaceWithFormatFrom( image.getPixels(), image.getWidth(), image.getHeight(),
											32, image.getWidth() * 4, SDL_PIXELFORMAT_RGBA32 );
	if ( !surface )
		return nullptr;
	auto* cursor = SDL_CreateColorCursor( surface, 0, 0 );
	SDL_FreeSurface( surface );
	return cursor;
}

void DragPreviewSDL::move( int x, int y ) {
	if ( !mWindow )
		return;
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
	int x, y;
	SDL_GetGlobalMouseState( &x, &y );
	move( x, y );
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
		if ( SDL_ShowCursor( SDL_QUERY ) == SDL_DISABLE )
			SDL_ShowCursor( SDL_ENABLE );
	}
}

void DragPreviewSDL::reset() {
	const bool managedCursor = mCursors[0] != nullptr;
	if ( mWindow ) {
		RestoreGLContext restore;
		SDL_DestroyWindow( mWindow );
	}
	for ( auto*& cursor : mCursors ) {
		if ( cursor )
			SDL_FreeCursor( cursor );
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

}}}} // namespace EE::Window::Backend::SDL2
#endif
