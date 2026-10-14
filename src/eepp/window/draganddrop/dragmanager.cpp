#include "dragmanager.hpp"
#include "dragaccess.hpp"
#include "win32draganddrop.hpp"
#include "x11draganddrop.hpp"
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>
#include <memory>

namespace EE { namespace Window { namespace Private {

std::unique_ptr<DragSource> DragManager::sSource;
bool DragManager::sPolling = false;

bool DragManager::hasSession() {
	return sSource != nullptr;
}

bool DragManager::isFileDragSupported( const NativeWindowContext& context ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return context.kind == NativeWindowContext::Kind::Win32;
#elif defined( EE_HAS_X11_DRAG_SOURCE )
	return context.kind == NativeWindowContext::Kind::X11 && X11DragAndDrop::isSupported();
#else
	return false;
#endif
}

DragStartResult DragManager::startFileDrag( NativeWindowContext context, const DragOffer& offer,
											std::string payload,
											DragCompletionCallback completion ) {
	DragStartResult result;
#ifdef EE_HAS_X11_DRAG_SOURCE
	if ( context.kind == NativeWindowContext::Kind::X11 ) {
		// Enable backend protocol forwarding only for a session, so idle windows do not
		// generate extra native events or event-pool allocations.
		NativeDragAccess::initializeNativeDragEvents( *context.owner );
		try {
			sSource = X11DragAndDrop::createSource( context, offer, std::move( payload ),
													std::move( completion ), result );
		} catch ( ... ) {
			// A failed C++ session allocation must also release the temporary native bridge.
			NativeDragAccess::shutdownNativeDragEvents( *context.owner );
			throw;
		}
		if ( !sSource )
			NativeDragAccess::shutdownNativeDragEvents( *context.owner );
	}
#endif
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( context.kind == NativeWindowContext::Kind::Win32 ) {
		sSource = Win32DragAndDrop::createSource( context, offer, std::move( completion ), result );
	}
#endif
	return result;
}

void DragManager::cancelDrag( Window& window ) {
	if ( sSource && sSource->owner == &window )
		sSource->cancel();
}

bool DragManager::isDragging( const Window& window ) {
	return sSource && sSource->owner == &window && !sSource->completed;
}

bool DragManager::handleEvent( void* event ) {
	if ( sSource && !sSource->completed )
		return sSource->handleEvent( event );
	return false;
}

void DragManager::handleInput( Uint32 windowId, NativeDragInput input ) {
	if ( sSource && !sSource->completed && sSource->owner &&
		 sSource->owner->getWindowID() == windowId )
		sSource->handleInput( input );
}

void DragManager::poll() {
	if ( !sSource || sPolling )
		return;
	sPolling = true;
	if ( !sSource->completed )
		sSource->tick();
	if ( !sSource->completed ) {
		sPolling = false;
		return;
	}
	// Detach and release all native resources before calling user code. In particular, a callback
	// may destroy its window, pump input recursively or start a new drag.
	auto finished = std::move( sSource );
	auto* owner = finished->owner;
	auto callback = std::move( finished->completion );
	auto result = std::move( finished->result );
	finished.reset();
	if ( owner )
		NativeDragAccess::shutdownNativeDragEvents( *owner );
	sPolling = false;
	if ( callback && !Engine::isShuttingDown() )
		callback( std::move( result ) );
}

void DragManager::invalidateWindow( Window* window ) {
	if ( sSource && sSource->owner == window ) {
		sSource->cancel();
		// Disable the bridge while virtual dispatch still reaches the window backend, before
		// its resources are destroyed. Completion may be polled after the owner is gone.
		NativeDragAccess::shutdownNativeDragEvents( *window );
		sSource->owner = nullptr;
	}
}

}}} // namespace EE::Window::Private
