#include "dragmanager.hpp"
#include "cocoadraganddrop.hpp"
#include "dragaccess.hpp"
#include "waylanddraganddrop.hpp"
#include "win32draganddrop.hpp"
#include "x11draganddrop.hpp"
#include "x11droptarget.hpp"
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>
#include <memory>

namespace EE { namespace Window { namespace Private {

std::unique_ptr<DragSource> DragManager::sSource;
std::unique_ptr<DropTarget> DragManager::sTarget;
bool DragManager::sPolling = false;
bool DragManager::sTickingTarget = false;

bool DragManager::needsNativeEvents() {
	return sTarget || ( sSource && sSource->owner );
}

bool DragManager::hasSession() {
	return sSource != nullptr;
}

bool DragManager::isFileDragSupported( const NativeWindowContext& context ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return context.kind == NativeWindowContext::Kind::Win32;
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return context.kind == NativeWindowContext::Kind::Cocoa;
#else
#ifdef EE_HAS_X11_DRAG_SOURCE
	if ( context.kind == NativeWindowContext::Kind::X11 )
		return X11DragAndDrop::isSupported();
#endif
#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
	if ( context.kind == NativeWindowContext::Kind::Wayland )
		return WaylandDragAndDrop::isSupported( context );
#endif
	return false;
#endif
}

DragStartResult DragManager::startFileDrag( NativeWindowContext context, const DragOffer& offer,
											std::string payload,
											DragCompletionCallback completion ) {
	DragStartResult result;
#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
	if ( context.kind == NativeWindowContext::Kind::Wayland ) {
		sSource = WaylandDragAndDrop::createSource( context, offer, std::move( payload ),
													std::move( completion ), result );
	}
#endif
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
			if ( !needsNativeEvents() )
				NativeDragAccess::shutdownNativeDragEvents( *context.owner );
			throw;
		}
		if ( !needsNativeEvents() )
			NativeDragAccess::shutdownNativeDragEvents( *context.owner );
	}
#endif
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( context.kind == NativeWindowContext::Kind::Win32 ) {
		sSource = Win32DragAndDrop::createSource( context, offer, std::move( completion ), result );
	}
#endif
#if EE_PLATFORM == EE_PLATFORM_MACOS
	if ( context.kind == NativeWindowContext::Kind::Cocoa )
		sSource = CocoaDragAndDrop::createSource( context, offer, std::move( completion ), result );
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

bool DragManager::setDataDropHandler( NativeWindowContext context,
									  std::vector<std::string> mimeTypes,
									  DataDropCallback callback ) {
	const bool adding = static_cast<bool>( callback );
	return updateTarget( context, adding, [&]( DropTarget& target ) {
		return target.setDataHandler( context, std::move( mimeTypes ), std::move( callback ) );
	} );
}

bool DragManager::setFileSaveDropHandler( NativeWindowContext context,
										  FileSaveDropCallback callback ) {
	const bool adding = static_cast<bool>( callback );
	return updateTarget( context, adding, [&]( DropTarget& target ) {
		return target.setFileSaveHandler( context, std::move( callback ) );
	} );
}

bool DragManager::updateTarget( const NativeWindowContext& context, bool adding,
								const std::function<bool( DropTarget& )>& update ) {
#ifdef EE_HAS_X11_DRAG_SOURCE
	if ( adding && !sTarget && context.kind == NativeWindowContext::Kind::X11 )
		sTarget = X11DropTarget::create();
#endif
	if ( !sTarget || !context.owner )
		return false;
	const bool updated = update( *sTarget );
	releaseEmptyTarget( *context.owner );
	if ( sTarget && !sTarget->empty() )
		NativeDragAccess::initializeNativeDragEvents( *context.owner );
	return updated;
}

void DragManager::releaseEmptyTarget( Window& window ) {
	// While tick() runs, an empty target stays until a later update or window invalidation.
	if ( !sTarget || !sTarget->empty() || sTickingTarget )
		return;
	sTarget.reset();
	if ( !needsNativeEvents() )
		NativeDragAccess::shutdownNativeDragEvents( window );
}

bool DragManager::handleEvent( void* event ) {
	if ( sSource && !sSource->completed && sSource->handleEvent( event ) )
		return true;
	return sTarget && sTarget->handleEvent( event );
}

void DragManager::handleInput( Uint32 windowId, NativeDragInput input ) {
	if ( sSource && !sSource->completed && sSource->owner &&
		 sSource->owner->getWindowID() == windowId )
		sSource->handleInput( input );
}

void DragManager::poll() {
	// A callback may pump input again: the target is ticked once at a time.
	if ( sTarget && !sTickingTarget && !Engine::isShuttingDown() ) {
		sTickingTarget = true;
		sTarget->tick();
		sTickingTarget = false;
	}
	pollSource();
}

void DragManager::pollSource() {
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
	if ( owner && !needsNativeEvents() )
		NativeDragAccess::shutdownNativeDragEvents( *owner );
	sPolling = false;
	if ( callback && !Engine::isShuttingDown() )
		callback( std::move( result ) );
}

void DragManager::invalidateWindow( Window* window ) {
	if ( sSource && sSource->owner == window ) {
		sSource->cancel();
		sSource->owner = nullptr;
		// Disable the bridge while virtual dispatch still reaches the window backend, before its
		// resources are destroyed. Completion may be polled after the owner is gone.
		if ( !needsNativeEvents() )
			NativeDragAccess::shutdownNativeDragEvents( *window );
	}
	if ( sTarget ) {
		sTarget->removeWindow( window );
		releaseEmptyTarget( *window );
	}
}

void DragManager::shutdown() {
	sTarget.reset();
	if ( !sSource )
		return;
	auto source = std::move( sSource );
	source->cancel();
	if ( source->owner )
		NativeDragAccess::shutdownNativeDragEvents( *source->owner );
}

}}} // namespace EE::Window::Private
