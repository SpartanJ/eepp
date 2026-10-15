#define Rect AppleRect
#import <AppKit/AppKit.h>
#undef Rect
#undef BSD

#include "cocoadraganddrop.hpp"
#include <cstring>
#include <eepp/graphics/image.hpp>

#if __has_feature( objc_arc )
#define EE_OBJC_RELEASE( object )
#define EE_OBJC_RETAIN( object )
#else
#define EE_OBJC_RELEASE( object ) [object release]
#define EE_OBJC_RETAIN( object ) [object retain]
#endif

namespace EE { namespace Window { namespace Private {
class CocoaDragSource;
}}} // namespace EE::Window::Private

/** AppKit retains the session source only while it needs it. This object holds one extra reference
 * from beginDraggingSession until the session ends, so the C++ source may be destroyed first. */
@interface EEFileDraggingSource : NSObject <NSDraggingSource> {
  @public
	EE::Window::Private::CocoaDragSource* owner;
}
@end

namespace EE { namespace Window { namespace Private {

class CocoaDragSource : public DragSource {
  public:
	CocoaDragSource( NativeWindowContext context, DragCompletionCallback completion ) :
		DragSource( context.owner, std::move( completion ) ),
		window( static_cast<NSWindow*>( context.handles.window ) ) {}

	~CocoaDragSource() override {
		if ( source ) {
			source->owner = nullptr;
			EE_OBJC_RELEASE( source );
		}
	}

	DragStartResult begin( const DragOffer& offer ) {
		NSView* view = window ? [window contentView] : nil;
		if ( !view || !( [NSEvent pressedMouseButtons] & 1 ) )
			return { "A left-button pointer gesture is required", DragStartStatus::InvalidGesture };
		NSEvent* event = gestureEvent();
		if ( !event )
			return { "Could not create the drag gesture event", DragStartStatus::Failed };
		const CGFloat scale = [window backingScaleFactor] > 0 ? [window backingScaleFactor] : 1;
		const NSPoint pointer = [view convertPoint:[event locationInWindow] fromView:nil];
		NSImage* snapshot = offer.preview.image ? createImage( *offer.preview.image, scale ) : nil;
		NSMutableArray<NSDraggingItem*>* items =
			[NSMutableArray arrayWithCapacity:offer.files.size()];
		for ( const auto& file : offer.files ) {
			NSString* path = [NSString stringWithUTF8String:file.path.c_str()];
			NSURL* url = path ? [NSURL fileURLWithPath:path] : nil;
			if ( !url ) {
				EE_OBJC_RELEASE( snapshot );
				return { "Could not convert a UTF-8 file path", DragStartStatus::InvalidOffer };
			}
			NSDraggingItem* item = [[NSDraggingItem alloc] initWithPasteboardWriter:url];
			// The snapshot represents the whole offer; AppKit badges the item count. Without one,
			// every item shows its Finder icon, stacked under the pointer like a Finder drag.
			NSImage* contents = snapshot ? ( [items count] ? nil : snapshot )
										 : [[NSWorkspace sharedWorkspace] iconForFile:path];
			const NSSize size = snapshot ? [snapshot size] : NSMakeSize( 64, 64 );
			[item setDraggingFrame:frameFor( view, pointer, size, offer.preview.hotspot, scale )
						  contents:contents];
			[items addObject:item];
			EE_OBJC_RELEASE( item );
		}
		EE_OBJC_RELEASE( snapshot );
		source = [[EEFileDraggingSource alloc] init];
		source->owner = this;
		NSDraggingSession* session = [view beginDraggingSessionWithItems:items
																   event:event
																  source:source];
		if ( !session )
			return { "AppKit refused to start the drag session", DragStartStatus::Failed };
		EE_OBJC_RETAIN( source ); // Released by draggingSession:endedAtPoint:operation:.
		[session setAnimatesToStartingPositionsOnCancelOrFail:YES];
		return { {}, DragStartStatus::Started };
	}

	void ended( NSDragOperation operation ) {
		endedOperation = operation;
		sessionEnded = true;
	}

	NSDragOperation operationMask( NSDraggingContext context ) const {
		if ( cancelled )
			return NSDragOperationNone;
		// SDL2's Cocoa windows accept only Generic drags. Offer it to this application's own
		// windows only: Finder may treat Generic as a move, while the native file contract is
		// copy-only.
		return context == NSDraggingContextWithinApplication
				   ? NSDragOperationCopy | NSDragOperationGeneric
				   : NSDragOperationCopy;
	}

	void tick() override {
		if ( completed || !sessionEnded )
			return;
		if ( cancelled )
			finish( DragEndReason::Cancelled );
		else if ( endedOperation & ( NSDragOperationCopy | NSDragOperationGeneric ) )
			finish( DragEndReason::Dropped, DragAction::Copy );
		else
			finish( DragEndReason::Cancelled );
	}

	bool handleEvent( void* ) override { return false; }

	void cancel() override {
		// AppKit cannot abort a running session. Refusing every operation turns its drop into a
		// cancellation, while completion is delivered now so the source window may close.
		cancelled = true;
		finish( DragEndReason::Cancelled );
	}

  private:
	NSEvent* gestureEvent() const {
		NSEvent* current = [NSApp currentEvent];
		if ( current && [current window] == window &&
			 ( [current type] == NSEventTypeLeftMouseDragged ||
			   [current type] == NSEventTypeLeftMouseDown ) )
			return current;
		// eepp starts drags from its input update, after SDL has dispatched the AppKit event.
		return [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged
								  location:[window mouseLocationOutsideOfEventStream]
							 modifierFlags:[NSEvent modifierFlags]
								 timestamp:[[NSProcessInfo processInfo] systemUptime]
							  windowNumber:[window windowNumber]
								   context:nil
							   eventNumber:0
								clickCount:1
								  pressure:1];
	}

	static NSImage* createImage( const Graphics::Image& image, CGFloat scale ) {
		const NSInteger width = image.getWidth();
		const NSInteger height = image.getHeight();
		const NSInteger channels = image.getChannels();
		NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc]
			initWithBitmapDataPlanes:nullptr
						  pixelsWide:width
						  pixelsHigh:height
					   bitsPerSample:8
					 samplesPerPixel:channels
							hasAlpha:channels == 4
							isPlanar:NO
					  colorSpaceName:NSDeviceRGBColorSpace
						bitmapFormat:channels == 4 ? NSBitmapFormatAlphaNonpremultiplied : 0
						 bytesPerRow:0
						bitsPerPixel:0];
		if ( !bitmap )
			return nil;
		const auto* pixels = image.getPixels();
		const size_t rowBytes = static_cast<size_t>( width * channels );
		for ( NSInteger row = 0; row < height; ++row ) {
			std::memcpy( [bitmap bitmapData] + row * [bitmap bytesPerRow], pixels + row * rowBytes,
						 rowBytes );
		}
		NSImage* result =
			[[NSImage alloc] initWithSize:NSMakeSize( width / scale, height / scale )];
		[result addRepresentation:bitmap];
		EE_OBJC_RELEASE( bitmap );
		return result;
	}

	/** Places the image so that the pointer sits at the hotspot, given in desktop pixels from the
	 * image's top-left corner. */
	static NSRect frameFor( NSView* view, NSPoint pointer, NSSize size, const Vector2i& hotspot,
							CGFloat scale ) {
		const CGFloat x = pointer.x - hotspot.x / scale;
		const CGFloat y = [view isFlipped] ? pointer.y - hotspot.y / scale
										   : pointer.y - ( size.height - hotspot.y / scale );
		return NSMakeRect( x, y, size.width, size.height );
	}

	NSWindow* window;
	EEFileDraggingSource* source{ nil };
	NSDragOperation endedOperation{ NSDragOperationNone };
	bool sessionEnded{ false };
	bool cancelled{ false };
};

std::unique_ptr<DragSource> CocoaDragAndDrop::createSource( NativeWindowContext context,
															const DragOffer& offer,
															DragCompletionCallback completion,
															DragStartResult& result ) {
	auto source = std::make_unique<CocoaDragSource>( context, std::move( completion ) );
	result = source->begin( offer );
	if ( result.status != DragStartStatus::Started )
		return nullptr;
	return source;
}

}}} // namespace EE::Window::Private

@implementation EEFileDraggingSource

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
	sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
	return owner ? owner->operationMask( context ) : NSDragOperationNone;
}

- (BOOL)ignoreModifierKeysForDraggingSession:(NSDraggingSession*)session {
	return YES;
}

- (void)draggingSession:(NSDraggingSession*)session
		   endedAtPoint:(NSPoint)screenPoint
			  operation:(NSDragOperation)operation {
	if ( owner )
		owner->ended( operation );
	EE_OBJC_RELEASE( self );
}

@end
