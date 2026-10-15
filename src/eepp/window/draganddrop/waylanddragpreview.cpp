#include "waylanddragpreview.hpp"

#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
#include <cmath>
#include <eepp/graphics/image.hpp>
#include <eepp/system/sys.hpp>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#if EE_PLATFORM == EE_PLATFORM_LINUX
#include <sys/syscall.h>
#endif

namespace EE { namespace Window { namespace Private {

WaylandDragPreview::~WaylandDragPreview() {
	reset();
}

bool WaylandDragPreview::initialize( wl_compositor* compositor, wl_subcompositor* subcompositor,
									 wl_shm* shm, const DragOffer& offer, float scale ) {
	FileImagePixels pixels;
	Graphics::Image generated;
	const auto* image = offer.preview.image;
	if ( !image ) {
		generated = makeFileImage( pixels, offer.files.size() );
		image = &generated;
	}
	if ( !std::isfinite( scale ) || scale < 1 )
		scale = 1;
	const int bufferScale = static_cast<int>( std::ceil( scale ) );
	const int inputWidth = image->getWidth(), inputHeight = image->getHeight();
	const int width = static_cast<int>( std::ceil( inputWidth / scale ) ) * bufferScale;
	const int height = static_cast<int>( std::ceil( inputHeight / scale ) ) * bufferScale;
	const size_t imageBytes = static_cast<size_t>( width ) * height * 4;
	const int badgeSize = 16 * bufferScale;
	const size_t badgeBytes = static_cast<size_t>( badgeSize ) * badgeSize * 4;
	const size_t bytes = imageBytes + feedbackBuffers.size() * badgeBytes;
	// Prefer an anonymous, close-on-exec memory file on Linux without requiring a newer libc.
	// BSD, older kernels and sandbox policies can use an unlinked, mode-0600 temp file instead.
	int fd = -1;
#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( SYS_memfd_create ) && defined( MFD_CLOEXEC )
	fd = static_cast<int>( syscall( SYS_memfd_create, "eepp-drag-icon", MFD_CLOEXEC ) );
#endif
	if ( fd < 0 ) {
		auto path = System::Sys::getTempPath();
		path += "eepp-drag-icon-XXXXXX";
		fd = mkstemp( path.data() );
		if ( fd < 0 )
			return false;
		if ( unlink( path.c_str() ) != 0 || fcntl( fd, F_SETFD, FD_CLOEXEC ) < 0 ) {
			close( fd );
			return false;
		}
	}
	if ( ftruncate( fd, bytes ) != 0 ) {
		close( fd );
		return false;
	}
	// The compositor retains its own mapping; the local mapping is needed only for this upload.
	auto* output =
		static_cast<Uint32*>( mmap( nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 ) );
	if ( output == MAP_FAILED ) {
		close( fd );
		return false;
	}
	// Convert directly into shared memory. Fractional display scales need dimensions that are
	// multiples of the integer buffer scale; no intermediate image is allocated.
	auto upload = []( const Graphics::Image& image, Uint32* output, int width, int height ) {
		const auto* pixelsIn = image.getPixels();
		const int inputWidth = image.getWidth(), inputHeight = image.getHeight();
		const auto channels = image.getChannels();
		for ( int y = 0; y < height; ++y ) {
			const auto* row =
				pixelsIn + static_cast<size_t>( y * inputHeight / height ) * inputWidth * channels;
			for ( int x = 0; x < width; ++x ) {
				const auto* input = row + ( x * inputWidth / width ) * channels;
				const Uint32 alpha = channels == 4 ? input[3] : 255;
				output[static_cast<size_t>( y ) * width + x] =
					alpha << 24 | ( input[0] * alpha / 255 ) << 16 |
					( input[1] * alpha / 255 ) << 8 | input[2] * alpha / 255;
			}
		}
	};
	upload( *image, output, width, height );
	FeedbackImagePixels badgePixels;
	for ( size_t i = 0; i < feedbackBuffers.size(); ++i ) {
		auto badgeImage = makeFeedbackImage( badgePixels, static_cast<DragFeedback>( i ) );
		upload( badgeImage, output + ( imageBytes + i * badgeBytes ) / 4, badgeSize, badgeSize );
	}
	auto* pool = library.create<wl_shm_pool>( shm, WL_SHM_CREATE_POOL, library.shm_pool, fd,
											  static_cast<int>( bytes ) );
	close( fd );
	munmap( output, bytes );
	if ( !pool )
		return false;
	buffer = library.create<wl_buffer>( pool, WL_SHM_POOL_CREATE_BUFFER, library.buffer, 0, width,
										height, width * 4, WL_SHM_FORMAT_ARGB8888 );
	for ( size_t i = 0; i < feedbackBuffers.size(); ++i ) {
		feedbackBuffers[i] =
			library.create<wl_buffer>( pool, WL_SHM_POOL_CREATE_BUFFER, library.buffer,
									   static_cast<int>( imageBytes + i * badgeBytes ), badgeSize,
									   badgeSize, badgeSize * 4, WL_SHM_FORMAT_ARGB8888 );
	}
	library.destroy( pool, WL_SHM_POOL_DESTROY );
	icon = library.create<wl_surface>( compositor, WL_COMPOSITOR_CREATE_SURFACE, library.surface );
	badge = library.create<wl_surface>( compositor, WL_COMPOSITOR_CREATE_SURFACE, library.surface );
	if ( !buffer || !icon || !badge ||
		 std::any_of( feedbackBuffers.begin(), feedbackBuffers.end(),
					  []( const auto* buffer ) { return !buffer; } ) )
		return false;
	badgeRole = library.create<wl_subsurface>( subcompositor, WL_SUBCOMPOSITOR_GET_SUBSURFACE,
											   library.subsurface, badge, icon );
	auto* region =
		library.create<wl_region>( compositor, WL_COMPOSITOR_CREATE_REGION, library.region );
	if ( !badgeRole || !region ) {
		library.destroy( region, WL_REGION_DESTROY );
		return false;
	}
	library.request( badge, WL_SURFACE_SET_INPUT_REGION, region );
	library.destroy( region, WL_REGION_DESTROY );
	library.request( badgeRole, WL_SUBSURFACE_SET_POSITION, width / bufferScale - 8,
					 height / bufferScale - 8 );
	library.request( badge, WL_SURFACE_SET_BUFFER_SCALE, bufferScale );
	static constexpr wl_buffer_listener bufferListener{ []( void*, wl_buffer* ) {} };
	library.listen( buffer, bufferListener, nullptr );
	for ( auto* feedbackBuffer : feedbackBuffers )
		library.listen( feedbackBuffer, bufferListener, nullptr );
	// Each badge has an immutable buffer, so changing feedback never overwrites pixels the
	// compositor may still be reading and never copies the main snapshot.
	feedback = DragFeedback::Rejected;
	library.request( badge, WL_SURFACE_ATTACH, feedbackBuffers[static_cast<size_t>( feedback )], 0,
					 0 );
	library.request( badge, WL_SURFACE_DAMAGE, 0, 0, badgeSize, badgeSize );
	library.request( badge, WL_SURFACE_COMMIT );
	library.request( icon, WL_SURFACE_SET_BUFFER_SCALE, bufferScale );
	// Bind compositor version 4 so attach offsets specify the icon hotspot on old compositors too.
	library.request( icon, WL_SURFACE_ATTACH, buffer,
					 static_cast<int>( std::lround( -offer.preview.hotspot.x / scale ) ),
					 static_cast<int>( std::lround( -offer.preview.hotspot.y / scale ) ) );
	library.request( icon, WL_SURFACE_DAMAGE, 0, 0, width, height );
	// The caller assigns the drag-icon role with start_drag before committing this buffer.
	return true;
}

void WaylandDragPreview::setFeedback( DragFeedback next ) {
	if ( !badge || feedback == next )
		return;
	feedback = next;
	library.request( badge, WL_SURFACE_ATTACH, feedbackBuffers[static_cast<size_t>( feedback )], 0,
					 0 );
	library.request( badge, WL_SURFACE_DAMAGE, 0, 0, 16, 16 );
	library.request( badge, WL_SURFACE_COMMIT );
	// The badge is a synchronized subsurface; its new state applies with the parent commit.
	library.request( icon, WL_SURFACE_COMMIT );
}

void WaylandDragPreview::reset() {
	library.destroy( badgeRole, WL_SUBSURFACE_DESTROY );
	library.destroy( badge, WL_SURFACE_DESTROY );
	library.destroy( icon, WL_SURFACE_DESTROY );
	library.destroy( buffer, WL_BUFFER_DESTROY );
	for ( auto*& feedbackBuffer : feedbackBuffers )
		library.destroy( feedbackBuffer, WL_BUFFER_DESTROY );
}

}}} // namespace EE::Window::Private
#endif
