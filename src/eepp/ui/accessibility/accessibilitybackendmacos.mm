#define Rect AppleRect
#import <AppKit/AppKit.h>
#undef Rect
#undef BSD

#include "accessibilitybackend.hpp"

#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/window.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace EE;
using namespace EE::UI;

#if __has_feature( objc_arc )
#define EE_OBJC_RELEASE( object )
#define EE_OBJC_AUTORELEASE( object ) object
#else
#define EE_OBJC_RELEASE( object ) [object release]
#define EE_OBJC_AUTORELEASE( object ) [object autorelease]
#endif

static NSString* toNSString( const String& string ) {
	const std::string utf8 = string.toUtf8();
	return utf8.empty() ? @"" : [NSString stringWithUTF8String:utf8.c_str()];
}

static NSString* toNSString( const std::string& string ) {
	return string.empty() ? @"" : [NSString stringWithUTF8String:string.c_str()];
}

static bool isMainThread() {
	return [NSThread isMainThread];
}

static bool hasState( AccessibilityState states, AccessibilityState state ) {
	return ( static_cast<Uint64>( states ) & static_cast<Uint64>( state ) ) != 0;
}

static bool hasAction( AccessibilityActions actions, AccessibilityAction action ) {
	return ( actions & accessibilityActionMask( action ) ) != 0;
}

static double numericValue( const String& value, bool& valid ) {
	double number = 0;
	valid = String::fromString( number, value.toUtf8() );
	return number;
}

static NSAccessibilityRole nativeRole( AccessibilityRole role ) {
	switch ( role ) {
		case AccessibilityRole::Application:
			return NSAccessibilityApplicationRole;
		case AccessibilityRole::Window:
		case AccessibilityRole::Dialog:
			return NSAccessibilityWindowRole;
		case AccessibilityRole::Group:
		case AccessibilityRole::TabPanel:
			return NSAccessibilityGroupRole;
		case AccessibilityRole::Button:
			return NSAccessibilityButtonRole;
		case AccessibilityRole::CheckBox:
			return NSAccessibilityCheckBoxRole;
		case AccessibilityRole::RadioButton:
			return NSAccessibilityRadioButtonRole;
		case AccessibilityRole::Label:
		case AccessibilityRole::Text:
			return NSAccessibilityStaticTextRole;
		case AccessibilityRole::TextBox:
			return NSAccessibilityTextFieldRole;
		case AccessibilityRole::Image:
			return NSAccessibilityImageRole;
		case AccessibilityRole::ComboBox:
			return NSAccessibilityComboBoxRole;
		case AccessibilityRole::Slider:
			return NSAccessibilitySliderRole;
		case AccessibilityRole::SpinButton:
			return NSAccessibilityIncrementorRole;
		case AccessibilityRole::ProgressBar:
			return NSAccessibilityProgressIndicatorRole;
		case AccessibilityRole::TabList:
			return NSAccessibilityTabGroupRole;
		case AccessibilityRole::Tab:
			return NSAccessibilityRadioButtonRole;
		case AccessibilityRole::MenuBar:
			return NSAccessibilityMenuBarRole;
		case AccessibilityRole::Menu:
			return NSAccessibilityMenuRole;
		case AccessibilityRole::MenuItem:
		case AccessibilityRole::CheckMenuItem:
		case AccessibilityRole::RadioMenuItem:
			return NSAccessibilityMenuItemRole;
		case AccessibilityRole::List:
			return NSAccessibilityListRole;
		case AccessibilityRole::ListItem:
			return NSAccessibilityRowRole;
		case AccessibilityRole::Table:
			return NSAccessibilityTableRole;
		case AccessibilityRole::Row:
			return NSAccessibilityRowRole;
		case AccessibilityRole::Cell:
			return NSAccessibilityCellRole;
		case AccessibilityRole::Tree:
			return NSAccessibilityOutlineRole;
		case AccessibilityRole::TreeItem:
			return NSAccessibilityRowRole;
		case AccessibilityRole::None:
		default:
			return NSAccessibilityUnknownRole;
	}
}

static NSAccessibilitySubrole nativeSubrole( const AccessibilityNodeInfo& info ) {
	if ( info.role == AccessibilityRole::TextBox &&
		 hasState( info.states, AccessibilityState::Protected ) )
		return NSAccessibilitySecureTextFieldSubrole;
	switch ( info.role ) {
		case AccessibilityRole::Dialog:
			return NSAccessibilityDialogSubrole;
		case AccessibilityRole::Window:
			return NSAccessibilityStandardWindowSubrole;
		case AccessibilityRole::Tab:
			return NSAccessibilityTabButtonSubrole;
		case AccessibilityRole::Row:
		case AccessibilityRole::ListItem:
			return NSAccessibilityTableRowSubrole;
		case AccessibilityRole::TreeItem:
			return NSAccessibilityOutlineRowSubrole;
		default:
			return nil;
	}
}

static std::u16string utf16String( const String& string ) {
	return string.toUtf16();
}

static NSUInteger codePointToUTF16( const String& string, Int32 offset ) {
	const auto text = utf16String( string );
	NSUInteger codePoints = 0;
	NSUInteger utf16Offset = 0;
	const NSUInteger wanted = offset > 0 ? static_cast<NSUInteger>( offset ) : 0;
	while ( utf16Offset < text.size() && codePoints < wanted ) {
		const char16_t value = text[utf16Offset++];
		if ( value >= 0xd800 && value <= 0xdbff && utf16Offset < text.size() ) {
			const char16_t next = text[utf16Offset];
			if ( next >= 0xdc00 && next <= 0xdfff )
				++utf16Offset;
		}
		++codePoints;
	}
	return utf16Offset;
}

static Int32 utf16ToCodePoint( const String& string, NSUInteger offset ) {
	const auto text = utf16String( string );
	NSUInteger index = 0;
	Int32 codePoints = 0;
	while ( index < text.size() && index < offset ) {
		const char16_t value = text[index++];
		if ( value >= 0xd800 && value <= 0xdbff && index < text.size() && index < offset ) {
			const char16_t next = text[index];
			if ( next >= 0xdc00 && next <= 0xdfff )
				++index;
		}
		++codePoints;
	}
	return codePoints;
}

static NSRange textRange( const AccessibilityNodeInfo& info, const String& value ) {
	if ( !info.text.valid )
		return NSMakeRange( 0, 0 );
	const NSUInteger start = codePointToUTF16( value, info.text.selectionStart );
	const NSUInteger end = codePointToUTF16( value, info.text.selectionEnd );
	return NSMakeRange( start, end >= start ? end - start : 0 );
}

static bool validRange( NSRange range, NSString* string ) {
	return range.location <= string.length && range.length <= string.length - range.location;
}

static NSArray* arrayWithObjects( const std::vector<id>& objects ) {
	NSMutableArray* result = [[NSMutableArray alloc] initWithCapacity:objects.size()];
	for ( id object : objects ) {
		if ( object )
			[result addObject:object];
	}
	return EE_OBJC_AUTORELEASE( result );
}

static void addAccessibilityChild( NSView* view, id child ) {
	if ( !view || !child )
		return;
	NSArray* existing = [view accessibilityChildren];
	for ( id candidate in existing ) {
		if ( candidate == child )
			return;
	}
	NSMutableArray* children = [[NSMutableArray alloc] initWithArray:existing ? existing : @[]];
	[children addObject:child];
	[view setAccessibilityChildren:children];
	EE_OBJC_RELEASE( children );
}

static void removeAccessibilityChild( NSView* view, id child ) {
	if ( !view || !child )
		return;
	NSArray* existing = [view accessibilityChildren];
	NSMutableArray* children = [[NSMutableArray alloc] initWithArray:existing ? existing : @[]];
	[children removeObjectIdenticalTo:child];
	[view setAccessibilityChildren:children];
	EE_OBJC_RELEASE( children );
}

class MacAccessibilityState;
class MacAccessibilityRegistry;
@class EEPPMacAccessibilityElement;
@class EEPPMacAccessibilityWindowElement;

static void markAccessibilityClientObserved();

struct NodeKey {
	AccessibilitySourceId source{ 0 };
	Uint64 id{ 0 };

	bool operator==( const NodeKey& other ) const {
		return source == other.source && id == other.id;
	}
};

struct NodeKeyHash {
	size_t operator()( const NodeKey& key ) const {
		return std::hash<Uint64>()( key.source ) ^ ( std::hash<Uint64>()( key.id ) + 0x9e3779b9 +
													 ( key.source << 6 ) + ( key.source >> 2 ) );
	}
};

static bool gAccessibilityClientObserved = false;

class MacAccessibilityState : public std::enable_shared_from_this<MacAccessibilityState> {
  public:
	MacAccessibilityState( AccessibilityManager& manager, NSWindow* nativeWindow,
						   std::string windowIdentifier ) :
		mManager( &manager ),
		mNativeWindow( nativeWindow ),
		mWindowIdentifier( std::move( windowIdentifier ) ),
		mRoot( manager.getRoot() ),
		mRetainedElements( [[NSMutableArray alloc] init] ) {}

	~MacAccessibilityState() {
		mManager = nullptr;
		mAlive = false;
		EE_OBJC_RELEASE( mRetainedElements );
	}

	void setWindowElement( EEPPMacAccessibilityWindowElement* element ) {
		mWindowElement = element;
	}

	EEPPMacAccessibilityWindowElement* windowElement() const { return mWindowElement; }

	AccessibilityManager* manager() const { return mAlive && isMainThread() ? mManager : nullptr; }
	// NSView is main-thread-only and NSWindow is not generally thread-safe. AppKit dispatches
	// application-side accessibility requests through the UI event path; reject any unexpected
	// off-main invocation instead of crossing into the eepp scene tree from that thread.

	NSWindow* nativeWindow() const { return mAlive ? mNativeWindow : nil; }

	const AccessibilityNodeRef& root() const { return mRoot; }

	bool hasActiveClient() const { return mAlive && gAccessibilityClientObserved; }

	void markClientObserved() {
		if ( mAlive && isMainThread() )
			markAccessibilityClientObserved();
	}

	void invalidate() {
		mAlive = false;
		mManager = nullptr;
		mNativeWindow = nil;
		mWindowElement = nil;
		mElements.clear();
		if ( mRetainedElements )
			[mRetainedElements removeAllObjects];
	}

	EEPPMacAccessibilityElement* elementFor( AccessibilityNodeRef ref );
	EEPPMacAccessibilityElement* cachedElementFor( AccessibilityNodeRef ref ) const;
	bool isValid( AccessibilityNodeRef ref ) const;
	void evict( AccessibilityNodeRef ref );
	void evictSource( AccessibilitySourceId source );
	void evictInvalidElements();

	NSArray* childrenFor( AccessibilityNodeRef ref );
	NSArray* childrenFor( AccessibilityNodeRef ref, NSUInteger index, NSUInteger maxCount );
	NSUInteger childCountFor( AccessibilityNodeRef ref );
	NSUInteger indexOfChild( AccessibilityNodeRef parent, id child );
	NSArray* selectedChildrenFor( AccessibilityNodeRef ref );
	AccessibilityNodeRef parentFor( AccessibilityNodeRef ref );
	AccessibilityNodeInfo infoFor( AccessibilityNodeRef ref ) const;
	AccessibilityNodeRef hitTest( NSPoint point );
	AccessibilityNodeRef focused() const;
	bool perform( AccessibilityNodeRef ref, AccessibilityAction action, const String& value = {} );

	NSRect frameFor( const AccessibilityNodeInfo& info ) const {
		if ( !mAlive || !mNativeWindow || !info.boundsValid || !mManager || !isMainThread() )
			return NSZeroRect;
		EE::Window::Window* window =
			mManager->getSceneNode() ? mManager->getSceneNode()->getWindow() : nullptr;
		// eepp world bounds are top-left-origin drawable pixels. NSAccessibility expects
		// bottom-left-origin global screen points, including negative origins on secondary
		// displays. contentRectForFrameRect: preserves that display origin; divide by the SDL
		// drawable scale exactly once to convert pixels to Cocoa points.
		const CGFloat scale = window && window->getScale() > 0 ? window->getScale() : 1;
		const NSRect content = [mNativeWindow contentRectForFrameRect:[mNativeWindow frame]];
		const CGFloat width = std::max<CGFloat>( 0, info.bounds.getWidth() / scale );
		const CGFloat height = std::max<CGFloat>( 0, info.bounds.getHeight() / scale );
		return NSMakeRect( content.origin.x + info.bounds.Left / scale,
						   content.origin.y + content.size.height -
							   ( info.bounds.Top + info.bounds.getHeight() ) / scale,
						   width, height );
	}

	NSString* identifierFor( AccessibilityNodeRef ref ) const {
		return toNSString( "eepp." + mWindowIdentifier + "." + std::to_string( ref.source ) + "." +
						   std::to_string( ref.id ) );
	}

  private:
	AccessibilityManager* mManager{ nullptr };
	NSWindow* mNativeWindow{ nil };
	std::string mWindowIdentifier;
	AccessibilityNodeRef mRoot;
	bool mAlive{ true };
	EEPPMacAccessibilityWindowElement* mWindowElement{ nil };
	NSMutableArray* mRetainedElements{ nil };
	std::unordered_map<NodeKey, EEPPMacAccessibilityElement*, NodeKeyHash> mElements;
};

@interface EEPPMacAccessibilityWindowElement : NSAccessibilityElement {
  @private
	std::weak_ptr<MacAccessibilityState> _state;
	NSString* _identifier;
}
- (void)configureWithState:(const std::shared_ptr<MacAccessibilityState>&)state
				identifier:(NSString*)identifier;
@end

@interface EEPPMacAccessibilityElement : NSAccessibilityElement {
  @private
	std::weak_ptr<MacAccessibilityState> _state;
	AccessibilityNodeRef _ref;
}
- (void)configureWithState:(const std::shared_ptr<MacAccessibilityState>&)state
					   ref:(AccessibilityNodeRef)ref;
@end

EEPPMacAccessibilityElement* MacAccessibilityState::elementFor( AccessibilityNodeRef ref ) {
	if ( !mAlive || !isMainThread() || !mManager || !ref.isValid() || !mManager->isValid( ref ) )
		return nil;
	const NodeKey key{ ref.source, ref.id };
	auto found = mElements.find( key );
	if ( found != mElements.end() )
		return found->second;
	const auto info = mManager->getNodeInfo( ref );
	EEPPMacAccessibilityElement* element = [EEPPMacAccessibilityElement
		accessibilityElementWithRole:nativeRole( info.role )
							   frame:frameFor( info )
							   label:info.name.empty() ? nil : toNSString( info.name )
							  parent:mWindowElement];
	[element configureWithState:shared_from_this() ref:ref];
	mElements.emplace( key, element );
	[mRetainedElements addObject:element];
	return element;
}

EEPPMacAccessibilityElement*
MacAccessibilityState::cachedElementFor( AccessibilityNodeRef ref ) const {
	if ( !ref.isValid() )
		return nil;
	auto found = mElements.find( NodeKey{ ref.source, ref.id } );
	return found != mElements.end() ? found->second : nil;
}

bool MacAccessibilityState::isValid( AccessibilityNodeRef ref ) const {
	return mManager && mAlive && isMainThread() && ref.isValid() && mManager->isValid( ref );
}

void MacAccessibilityState::evict( AccessibilityNodeRef ref ) {
	auto found = mElements.find( NodeKey{ ref.source, ref.id } );
	if ( found == mElements.end() )
		return;
	EEPPMacAccessibilityElement* element = found->second;
	mElements.erase( found );
	[mRetainedElements removeObject:element];
}

void MacAccessibilityState::evictSource( AccessibilitySourceId source ) {
	for ( auto it = mElements.begin(); it != mElements.end(); ) {
		if ( it->first.source == source ) {
			if ( hasActiveClient() )
				NSAccessibilityPostNotification( it->second,
												 NSAccessibilityUIElementDestroyedNotification );
			[mRetainedElements removeObject:it->second];
			it = mElements.erase( it );
		} else
			++it;
	}
}

void MacAccessibilityState::evictInvalidElements() {
	if ( !manager() )
		return;
	for ( auto it = mElements.begin(); it != mElements.end(); ) {
		if ( !mManager->isValid( { it->first.source, it->first.id } ) ) {
			if ( hasActiveClient() )
				NSAccessibilityPostNotification( it->second,
												 NSAccessibilityUIElementDestroyedNotification );
			[mRetainedElements removeObject:it->second];
			it = mElements.erase( it );
		} else
			++it;
	}
}

AccessibilityNodeInfo MacAccessibilityState::infoFor( AccessibilityNodeRef ref ) const {
	return mManager && mAlive && isMainThread() ? mManager->getNodeInfo( ref )
												: AccessibilityNodeInfo();
}

NSArray* MacAccessibilityState::childrenFor( AccessibilityNodeRef ref ) {
	if ( !manager() )
		return nil;
	const auto& children = mManager->getChildren( ref );
	NSMutableArray* result = [[NSMutableArray alloc] initWithCapacity:children.size()];
	for ( const auto& child : children ) {
		if ( auto element = elementFor( child ) )
			[result addObject:element];
	}
	return EE_OBJC_AUTORELEASE( result );
}

NSArray* MacAccessibilityState::childrenFor( AccessibilityNodeRef ref, NSUInteger index,
											 NSUInteger maxCount ) {
	if ( !manager() || maxCount == 0 )
		return @[];
	const auto& children = mManager->getChildren( ref );
	if ( index >= children.size() )
		return @[];
	const NSUInteger end = index + std::min<NSUInteger>( maxCount, children.size() - index );
	NSMutableArray* result = [[NSMutableArray alloc] initWithCapacity:end - index];
	for ( NSUInteger childIndex = index; childIndex < end; ++childIndex ) {
		if ( auto element = elementFor( children[childIndex] ) )
			[result addObject:element];
	}
	return EE_OBJC_AUTORELEASE( result );
}

NSUInteger MacAccessibilityState::childCountFor( AccessibilityNodeRef ref ) {
	return manager() ? mManager->getChildCount( ref ) : 0;
}

NSUInteger MacAccessibilityState::indexOfChild( AccessibilityNodeRef parent, id child ) {
	if ( !manager() || !child )
		return NSNotFound;
	const auto& children = mManager->getChildren( parent );
	for ( NSUInteger index = 0; index < children.size(); ++index ) {
		if ( cachedElementFor( children[index] ) == child )
			return index;
	}
	return NSNotFound;
}

NSArray* MacAccessibilityState::selectedChildrenFor( AccessibilityNodeRef ref ) {
	if ( !manager() )
		return nil;
	const auto& children = mManager->getChildren( ref );
	NSMutableArray* result = [[NSMutableArray alloc] init];
	for ( const auto& child : children ) {
		if ( hasState( mManager->getNodeInfo( child ).states, AccessibilityState::Selected ) ) {
			if ( auto element = elementFor( child ) )
				[result addObject:element];
		}
	}
	return EE_OBJC_AUTORELEASE( result );
}

AccessibilityNodeRef MacAccessibilityState::parentFor( AccessibilityNodeRef ref ) {
	return manager() ? mManager->getParent( ref ) : AccessibilityNodeRef{};
}

AccessibilityNodeRef MacAccessibilityState::hitTest( NSPoint point ) {
	if ( !manager() || !mNativeWindow || ![mNativeWindow isVisible] ||
		 [mNativeWindow isMiniaturized] )
		return {};
	const NSRect content = [mNativeWindow contentRectForFrameRect:[mNativeWindow frame]];
	if ( !NSPointInRect( point, content ) )
		return {};
	EE::Window::Window* window =
		mManager->getSceneNode() ? mManager->getSceneNode()->getWindow() : nullptr;
	const CGFloat scale = window && window->getScale() > 0 ? window->getScale() : 1;
	const Math::Vector2f local( ( point.x - content.origin.x ) * scale,
								( content.origin.y + content.size.height - point.y ) * scale );
	return mManager->hitTest( local );
}

AccessibilityNodeRef MacAccessibilityState::focused() const {
	return mManager && mAlive && isMainThread() ? mManager->getKeyboardFocusedNode()
												: AccessibilityNodeRef{};
}

bool MacAccessibilityState::perform( AccessibilityNodeRef ref, AccessibilityAction action,
									 const String& value ) {
	if ( !manager() )
		return false;
	if ( action == AccessibilityAction::Focus && mNativeWindow && ![mNativeWindow isKeyWindow] )
		[mNativeWindow makeKeyAndOrderFront:nil];
	return mManager->performAction( ref, { action, value } );
}

@implementation EEPPMacAccessibilityWindowElement

- (void)configureWithState:(const std::shared_ptr<MacAccessibilityState>&)state
				identifier:(NSString*)identifier {
	_state = state;
	_identifier = [identifier copy];
}

- (void)dealloc {
	EE_OBJC_RELEASE( _identifier );
#if !__has_feature( objc_arc )
	[super dealloc];
#endif
}

- (std::shared_ptr<MacAccessibilityState>)accessibilityState {
	if ( !isMainThread() )
		return {};
	auto state = _state.lock();
	if ( state )
		state->markClientObserved();
	return state;
}

- (BOOL)isAccessibilityElement {
	auto state = [self accessibilityState];
	return state && state->nativeWindow() != nil;
}

- (BOOL)accessibilityNotifiesWhenDestroyed {
	return YES;
}

- (NSRect)accessibilityFrame {
	auto state = [self accessibilityState];
	return state && state->nativeWindow()
			   ? [state->nativeWindow() contentRectForFrameRect:[state->nativeWindow() frame]]
			   : NSZeroRect;
}

- (NSPoint)accessibilityActivationPoint {
	const NSRect frame = [self accessibilityFrame];
	return NSMakePoint( NSMidX( frame ), NSMidY( frame ) );
}

- (id)accessibilityTopLevelUIElement {
	auto state = [self accessibilityState];
	return state ? state->nativeWindow() : nil;
}

- (id)accessibilityWindow {
	return self.accessibilityTopLevelUIElement;
}

- (id)accessibilityParent {
	auto state = [self accessibilityState];
	return state && state->nativeWindow() ? [state->nativeWindow() contentView] : nil;
}

- (NSAccessibilityRole)accessibilityRole {
	return NSAccessibilityGroupRole;
}

- (NSAccessibilitySubrole)accessibilitySubrole {
	return nil;
}

- (NSString*)accessibilityRoleDescription {
	NSAccessibilityRole role = self.accessibilityRole;
	return role ? NSAccessibilityRoleDescription( role, self.accessibilitySubrole ) : nil;
}

- (NSString*)accessibilityTitle {
	return nil;
}

- (NSString*)accessibilityIdentifier {
	return _identifier;
}

- (NSString*)accessibilityLabel {
	return nil;
}

- (BOOL)isAccessibilityEnabled {
	return YES;
}

- (BOOL)isAccessibilityFocused {
	return NO;
}

- (BOOL)isAccessibilityHidden {
	auto state = [self accessibilityState];
	return !state || !state->nativeWindow() || ![state->nativeWindow() isVisible];
}

- (NSArray*)accessibilityChildren {
	auto state = [self accessibilityState];
	return state ? state->childrenFor( state->root() ) : nil;
}

- (NSArray*)accessibilityVisibleChildren {
	return self.accessibilityChildren;
}

- (NSArray*)accessibilityChildrenInNavigationOrder {
	return self.accessibilityChildren;
}

- (NSArray*)accessibilityContents {
	return self.accessibilityChildren;
}

- (id)accessibilityFocusedUIElement {
	auto state = [self accessibilityState];
	if ( !state )
		return nil;
	const auto ref = state->focused();
	return ref.isValid() ? state->elementFor( ref ) : nil;
}

- (BOOL)accessibilityIsAttributeSettable:(NSAccessibilityAttributeName)attribute {
	(void)attribute;
	return NO;
}

- (NSUInteger)accessibilityIndexOfChild:(id)child {
	auto state = [self accessibilityState];
	return state ? state->indexOfChild( state->root(), child ) : NSNotFound;
}

- (NSUInteger)accessibilityArrayAttributeCount:(NSAccessibilityAttributeName)attribute {
	if ( ![attribute isEqualToString:NSAccessibilityChildrenAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityVisibleChildrenAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityChildrenInNavigationOrderAttribute] )
		return 0;
	auto state = [self accessibilityState];
	return state ? state->childCountFor( state->root() ) : 0;
}

- (NSArray*)accessibilityArrayAttributeValues:(NSAccessibilityAttributeName)attribute
										index:(NSUInteger)index
									 maxCount:(NSUInteger)maxCount {
	if ( ![attribute isEqualToString:NSAccessibilityChildrenAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityVisibleChildrenAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityChildrenInNavigationOrderAttribute] )
		return @[];
	auto state = [self accessibilityState];
	return state ? state->childrenFor( state->root(), index, maxCount ) : @[];
}

- (id)accessibilityAttributeValue:(NSAccessibilityAttributeName)attribute {
	if ( [attribute isEqualToString:NSAccessibilityChildrenAttribute] )
		return self.accessibilityChildren;
	if ( [attribute isEqualToString:NSAccessibilityVisibleChildrenAttribute] )
		return self.accessibilityVisibleChildren;
	if ( [attribute isEqualToString:NSAccessibilityChildrenInNavigationOrderAttribute] )
		return self.accessibilityChildrenInNavigationOrder;
	if ( [attribute isEqualToString:NSAccessibilityParentAttribute] )
		return self.accessibilityParent;
	if ( [attribute isEqualToString:NSAccessibilityWindowAttribute] )
		return self.accessibilityWindow;
	if ( [attribute isEqualToString:NSAccessibilityTopLevelUIElementAttribute] )
		return self.accessibilityTopLevelUIElement;
	if ( [attribute isEqualToString:NSAccessibilityRoleAttribute] )
		return self.accessibilityRole;
	if ( [attribute isEqualToString:NSAccessibilitySubroleAttribute] )
		return self.accessibilitySubrole;
	if ( [attribute isEqualToString:NSAccessibilityRoleDescriptionAttribute] )
		return self.accessibilityRoleDescription;
	if ( [attribute isEqualToString:NSAccessibilityTitleAttribute] )
		return self.accessibilityTitle;
	if ( [attribute isEqualToString:NSAccessibilityIdentifierAttribute] )
		return self.accessibilityIdentifier;
	if ( [attribute isEqualToString:NSAccessibilityPositionAttribute] )
		return [NSValue valueWithPoint:self.accessibilityFrame.origin];
	if ( [attribute isEqualToString:NSAccessibilitySizeAttribute] )
		return [NSValue valueWithSize:self.accessibilityFrame.size];
	if ( [attribute isEqualToString:NSAccessibilityEnabledAttribute] )
		return @( self.isAccessibilityEnabled );
	if ( [attribute isEqualToString:NSAccessibilityHiddenAttribute] )
		return @( self.isAccessibilityHidden );
	if ( [attribute isEqualToString:NSAccessibilityFocusedUIElementAttribute] )
		return self.accessibilityFocusedUIElement;
	return nil;
}

- (NSArray*)accessibilityAttributeNames {
	return @[
		NSAccessibilityRoleAttribute, NSAccessibilitySubroleAttribute,
		NSAccessibilityRoleDescriptionAttribute, NSAccessibilityTitleAttribute,
		NSAccessibilityIdentifierAttribute, NSAccessibilityPositionAttribute,
		NSAccessibilitySizeAttribute, NSAccessibilityParentAttribute,
		NSAccessibilityChildrenAttribute, NSAccessibilityVisibleChildrenAttribute,
		NSAccessibilityChildrenInNavigationOrderAttribute, NSAccessibilityWindowAttribute,
		NSAccessibilityTopLevelUIElementAttribute, NSAccessibilityFocusedUIElementAttribute,
		NSAccessibilityEnabledAttribute, NSAccessibilityHiddenAttribute
	];
}

- (BOOL)accessibilityIsIgnored {
	return !self.isAccessibilityElement;
}

- (id)accessibilityHitTest:(NSPoint)point {
	auto state = [self accessibilityState];
	if ( !state || self.isAccessibilityHidden || !NSPointInRect( point, self.accessibilityFrame ) )
		return nil;
	const auto ref = state->hitTest( point );
	return ref.isValid() ? state->elementFor( ref ) : self;
}

@end

@implementation EEPPMacAccessibilityElement

- (void)configureWithState:(const std::shared_ptr<MacAccessibilityState>&)state
					   ref:(AccessibilityNodeRef)ref {
	_state = state;
	_ref = ref;
}

- (std::shared_ptr<MacAccessibilityState>)accessibilityState {
	if ( !isMainThread() )
		return {};
	auto state = _state.lock();
	if ( state )
		state->markClientObserved();
	return state;
}

- (AccessibilityNodeInfo)nodeInfo {
	auto state = [self accessibilityState];
	return state ? state->infoFor( _ref ) : AccessibilityNodeInfo();
}

- (BOOL)isAccessibilityElement {
	auto state = [self accessibilityState];
	if ( !state )
		return NO;
	const auto info = state->infoFor( _ref );
	return info.role != AccessibilityRole::None &&
		   hasState( info.states, AccessibilityState::Visible ) &&
		   hasState( info.states, AccessibilityState::Showing );
}

- (BOOL)accessibilityNotifiesWhenDestroyed {
	return YES;
}

- (NSRect)accessibilityFrame {
	auto state = [self accessibilityState];
	return state ? state->frameFor( [self nodeInfo] ) : NSZeroRect;
}

- (NSPoint)accessibilityActivationPoint {
	const NSRect frame = self.accessibilityFrame;
	return NSMakePoint( NSMidX( frame ), NSMidY( frame ) );
}

- (id)accessibilityTopLevelUIElement {
	auto state = [self accessibilityState];
	return state && state->isValid( _ref ) ? state->nativeWindow() : nil;
}

- (id)accessibilityWindow {
	return self.accessibilityTopLevelUIElement;
}

- (id)accessibilityParent {
	auto state = [self accessibilityState];
	if ( !state || !state->isValid( _ref ) )
		return nil;
	const auto parent = state->parentFor( _ref );
	if ( !parent.isValid() || parent == state->root() )
		return state->windowElement();
	return state->elementFor( parent );
}

- (NSAccessibilityRole)accessibilityRole {
	auto state = [self accessibilityState];
	return state && state->isValid( _ref ) ? nativeRole( state->infoFor( _ref ).role ) : nil;
}

- (NSAccessibilitySubrole)accessibilitySubrole {
	return nativeSubrole( [self nodeInfo] );
}

- (NSString*)accessibilityRoleDescription {
	NSAccessibilityRole role = self.accessibilityRole;
	return role ? NSAccessibilityRoleDescription( role, self.accessibilitySubrole ) : nil;
}

- (NSString*)accessibilityLabel {
	const auto info = [self nodeInfo];
	if ( info.role == AccessibilityRole::Label || info.role == AccessibilityRole::Text ||
		 ( info.name == info.value &&
		   ( info.role == AccessibilityRole::ListItem || info.role == AccessibilityRole::Row ||
			 info.role == AccessibilityRole::TreeItem ) ) )
		return nil;
	const String& name = info.name;
	return name.empty() ? nil : toNSString( name );
}

- (NSString*)accessibilityTitle {
	return nil;
}

- (NSString*)accessibilityHelp {
	const String description = [self nodeInfo].description;
	return description.empty() ? nil : toNSString( description );
}

- (NSString*)accessibilityIdentifier {
	auto state = [self accessibilityState];
	return state && state->isValid( _ref ) ? state->identifierFor( _ref ) : nil;
}

- (id)accessibilityValue {
	const auto info = [self nodeInfo];
	if ( info.role == AccessibilityRole::Label || info.role == AccessibilityRole::Text )
		return info.name.empty() ? nil : toNSString( info.name );
	if ( info.role == AccessibilityRole::CheckBox || info.role == AccessibilityRole::RadioButton ||
		 info.role == AccessibilityRole::CheckMenuItem ||
		 info.role == AccessibilityRole::RadioMenuItem )
		return @( hasState( info.states, AccessibilityState::Checked ) ||
				  hasState( info.states, AccessibilityState::Selected ) );
	if ( info.range.valid ) {
		bool valid = false;
		const double value = numericValue( info.value, valid );
		return valid ? @( value ) : nil;
	}
	return info.value.empty() ? nil : toNSString( info.value );
}

- (id)accessibilityValueDescription {
	return nil;
}

- (id)accessibilityMinValue {
	const auto info = [self nodeInfo];
	return info.range.valid ? @( info.range.minimum ) : nil;
}

- (id)accessibilityMaxValue {
	const auto info = [self nodeInfo];
	return info.range.valid ? @( info.range.maximum ) : nil;
}

- (BOOL)isAccessibilityFocused {
	return hasState( [self nodeInfo].states, AccessibilityState::Focused );
}

- (BOOL)isAccessibilitySelected {
	return hasState( [self nodeInfo].states, AccessibilityState::Selected );
}

- (BOOL)isAccessibilityExpanded {
	return hasState( [self nodeInfo].states, AccessibilityState::Expanded );
}

- (BOOL)isAccessibilityEnabled {
	return hasState( [self nodeInfo].states, AccessibilityState::Enabled );
}

- (BOOL)isAccessibilityHidden {
	return !self.isAccessibilityElement;
}

- (NSArray*)accessibilityChildren {
	auto state = [self accessibilityState];
	return state ? state->childrenFor( _ref ) : nil;
}

- (NSArray*)accessibilityVisibleChildren {
	return self.accessibilityChildren;
}

- (NSArray*)accessibilityChildrenInNavigationOrder {
	return self.accessibilityChildren;
}

- (NSArray*)accessibilityContents {
	return self.accessibilityChildren;
}

- (NSArray*)accessibilitySelectedChildren {
	auto state = [self accessibilityState];
	return state ? state->selectedChildrenFor( _ref ) : nil;
}

- (NSArray*)accessibilityRows {
	const auto role = [self nodeInfo].role;
	return role == AccessibilityRole::Table || role == AccessibilityRole::List ||
				   role == AccessibilityRole::Tree
			   ? self.accessibilityChildren
			   : nil;
}

- (NSArray*)accessibilityVisibleRows {
	return self.accessibilityRows;
}

- (NSArray*)accessibilitySelectedRows {
	const auto role = [self nodeInfo].role;
	return role == AccessibilityRole::Table || role == AccessibilityRole::List ||
				   role == AccessibilityRole::Tree
			   ? self.accessibilitySelectedChildren
			   : nil;
}

- (NSInteger)accessibilityIndex {
	auto state = [self accessibilityState];
	if ( !state || !state->isValid( _ref ) )
		return NSNotFound;
	const auto parent = state->parentFor( _ref );
	return parent.isValid() ? state->indexOfChild( parent, self ) : NSNotFound;
}

- (NSArray*)accessibilityLinkedUIElements {
	auto state = [self accessibilityState];
	if ( !state )
		return nil;
	const auto info = state->infoFor( _ref );
	std::vector<id> related;
	for ( const auto& relation : info.relations ) {
		if ( auto element = state->elementFor( relation.target ) )
			related.emplace_back( element );
	}
	return arrayWithObjects( related );
}

- (BOOL)isAccessibilityProtectedContent {
	return hasState( [self nodeInfo].states, AccessibilityState::Protected );
}

- (NSArray*)accessibilityActionNames {
	const auto info = [self nodeInfo];
	const auto actions = info.actions;
	const bool selectUsesPress = info.role == AccessibilityRole::RadioButton ||
								 info.role == AccessibilityRole::RadioMenuItem ||
								 info.role == AccessibilityRole::Tab;
	NSMutableArray* names = [[NSMutableArray alloc] init];
	if ( !hasState( info.states, AccessibilityState::Enabled ) )
		return EE_OBJC_AUTORELEASE( names );
	if ( hasAction( actions, AccessibilityAction::Press ) ||
		 hasAction( actions, AccessibilityAction::Toggle ) ||
		 ( hasAction( actions, AccessibilityAction::Select ) && selectUsesPress ) )
		[names addObject:NSAccessibilityPressAction];
	if ( hasAction( actions, AccessibilityAction::Select ) && !selectUsesPress )
		[names addObject:NSAccessibilityPickAction];
	if ( hasAction( actions, AccessibilityAction::Increment ) )
		[names addObject:NSAccessibilityIncrementAction];
	if ( hasAction( actions, AccessibilityAction::Decrement ) )
		[names addObject:NSAccessibilityDecrementAction];
	if ( info.role == AccessibilityRole::ComboBox &&
		 ( hasAction( actions, AccessibilityAction::Expand ) ||
		   hasAction( actions, AccessibilityAction::Collapse ) ) )
		[names addObject:NSAccessibilityShowMenuAction];
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
	if ( hasAction( actions, AccessibilityAction::ScrollTo ) ) {
		if ( @available( macOS 26.0, * ) )
			[names addObject:NSAccessibilityScrollToVisibleAction];
	}
#endif
	return EE_OBJC_AUTORELEASE( names );
}

- (NSString*)accessibilityActionDescription:(NSAccessibilityActionName)action {
	return NSAccessibilityActionDescription( action );
}

- (BOOL)performAccessibilityAction:(AccessibilityAction)action {
	auto state = [self accessibilityState];
	if ( !state )
		return NO;
	const auto info = state->infoFor( _ref );
	return hasState( info.states, AccessibilityState::Enabled ) &&
		   hasAction( info.actions, action ) && state->perform( _ref, action );
}

- (void)accessibilityPerformAction:(NSAccessibilityActionName)action {
	if ( [action isEqualToString:NSAccessibilityPressAction] ||
		 [action isEqualToString:NSAccessibilityConfirmAction] ) {
		const auto info = [self nodeInfo];
		if ( hasAction( info.actions, AccessibilityAction::Press ) )
			[self performAccessibilityAction:AccessibilityAction::Press];
		else if ( hasAction( info.actions, AccessibilityAction::Toggle ) )
			[self performAccessibilityAction:AccessibilityAction::Toggle];
		else if ( hasAction( info.actions, AccessibilityAction::Select ) )
			[self performAccessibilityAction:AccessibilityAction::Select];
	} else if ( [action isEqualToString:NSAccessibilityPickAction] ) {
		if ( hasAction( [self nodeInfo].actions, AccessibilityAction::Select ) )
			[self performAccessibilityAction:AccessibilityAction::Select];
	} else if ( [action isEqualToString:NSAccessibilityIncrementAction] ) {
		[self performAccessibilityAction:AccessibilityAction::Increment];
	} else if ( [action isEqualToString:NSAccessibilityDecrementAction] ) {
		[self performAccessibilityAction:AccessibilityAction::Decrement];
	} else if ( [action isEqualToString:NSAccessibilityShowMenuAction] ) {
		const auto info = [self nodeInfo];
		const auto next = hasState( info.states, AccessibilityState::Expanded )
							  ? AccessibilityAction::Collapse
							  : AccessibilityAction::Expand;
		[self performAccessibilityAction:next];
	}
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
	else if ( @available( macOS 26.0, * ) ) {
		if ( [action isEqualToString:NSAccessibilityScrollToVisibleAction] )
			[self performAccessibilityAction:AccessibilityAction::ScrollTo];
	}
#endif
}

- (BOOL)accessibilityPerformPress {
	const auto info = [self nodeInfo];
	if ( hasAction( info.actions, AccessibilityAction::Press ) )
		return [self performAccessibilityAction:AccessibilityAction::Press];
	if ( hasAction( info.actions, AccessibilityAction::Toggle ) )
		return [self performAccessibilityAction:AccessibilityAction::Toggle];
	if ( hasAction( info.actions, AccessibilityAction::Select ) )
		return [self performAccessibilityAction:AccessibilityAction::Select];
	return NO;
}

- (BOOL)accessibilityPerformIncrement {
	return [self performAccessibilityAction:AccessibilityAction::Increment];
}

- (BOOL)accessibilityPerformDecrement {
	return [self performAccessibilityAction:AccessibilityAction::Decrement];
}

- (BOOL)accessibilityPerformShowMenu {
	const auto info = [self nodeInfo];
	return [self performAccessibilityAction:( hasState( info.states, AccessibilityState::Expanded )
												  ? AccessibilityAction::Collapse
												  : AccessibilityAction::Expand )];
}

- (BOOL)accessibilityIsAttributeSettable:(NSAccessibilityAttributeName)attribute {
	auto state = [self accessibilityState];
	if ( !state )
		return NO;
	const auto info = state->infoFor( _ref );
	if ( !hasState( info.states, AccessibilityState::Enabled ) )
		return NO;
	if ( [attribute isEqualToString:NSAccessibilityFocusedAttribute] )
		return hasAction( info.actions, AccessibilityAction::Focus );
	if ( [attribute isEqualToString:NSAccessibilitySelectedAttribute] )
		return hasAction( info.actions, AccessibilityAction::Select );
	if ( [attribute isEqualToString:NSAccessibilityExpandedAttribute] )
		return hasAction( info.actions, AccessibilityAction::Expand ) ||
			   hasAction( info.actions, AccessibilityAction::Collapse );
	if ( [attribute isEqualToString:NSAccessibilityValueAttribute] )
		return hasAction( info.actions, AccessibilityAction::SetValue ) ||
			   hasAction( info.actions, AccessibilityAction::SetText );
	if ( [attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute] ||
		 [attribute isEqualToString:NSAccessibilitySelectedTextRangesAttribute] )
		return hasAction( info.actions, AccessibilityAction::SetTextSelection );
	if ( [attribute isEqualToString:NSAccessibilitySelectedTextAttribute] )
		return info.text.valid && hasAction( info.actions, AccessibilityAction::SetText );
	return NO;
}

- (void)setAccessibilityFocused:(BOOL)focused {
	if ( !focused )
		return;
	auto state = [self accessibilityState];
	if ( !state )
		return;
	const auto info = state->infoFor( _ref );
	if ( hasAction( info.actions, AccessibilityAction::Focus ) )
		state->perform( _ref, AccessibilityAction::Focus );
}

- (void)setAccessibilitySelected:(BOOL)selected {
	if ( !selected )
		return;
	auto state = [self accessibilityState];
	if ( !state )
		return;
	const auto info = state->infoFor( _ref );
	if ( hasAction( info.actions, AccessibilityAction::Select ) )
		state->perform( _ref, AccessibilityAction::Select );
}

- (void)setAccessibilityExpanded:(BOOL)expanded {
	auto state = [self accessibilityState];
	if ( !state )
		return;
	const auto info = state->infoFor( _ref );
	const auto action = expanded ? AccessibilityAction::Expand : AccessibilityAction::Collapse;
	if ( hasAction( info.actions, action ) )
		state->perform( _ref, action );
}

- (void)setAccessibilityValue:(id)value {
	auto state = [self accessibilityState];
	if ( !state || !value )
		return;
	const auto info = state->infoFor( _ref );
	AccessibilityAction action = AccessibilityAction::SetValue;
	if ( hasAction( info.actions, AccessibilityAction::SetText ) )
		action = AccessibilityAction::SetText;
	if ( !hasAction( info.actions, action ) )
		return;
	NSString* nativeValue = nil;
	if ( [value isKindOfClass:[NSString class]] )
		nativeValue = value;
	else if ( [value respondsToSelector:@selector( stringValue )] )
		nativeValue = [value stringValue];
	if ( nativeValue )
		state->perform( _ref, action,
						String::fromUtf8( std::string( nativeValue.UTF8String ?: "" ) ) );
}

- (void)setAccessibilitySelectedTextRange:(NSRange)range {
	auto state = [self accessibilityState];
	if ( !state )
		return;
	const auto info = state->infoFor( _ref );
	NSString* nativeText = toNSString( info.value );
	if ( !info.text.valid || !validRange( range, nativeText ) ||
		 !hasAction( info.actions, AccessibilityAction::SetTextSelection ) )
		return;
	const Int32 start = utf16ToCodePoint( info.value, range.location );
	const Int32 end = utf16ToCodePoint( info.value, NSMaxRange( range ) );
	state->perform( _ref, AccessibilityAction::SetTextSelection,
					String( std::to_string( start ) + ":" + std::to_string( end ) ) );
}

- (void)setAccessibilitySelectedTextRanges:(NSArray<NSValue*>*)ranges {
	if ( ranges.count > 0 )
		self.accessibilitySelectedTextRange = ranges.firstObject.rangeValue;
}

- (void)setAccessibilitySelectedText:(NSString*)replacement {
	auto state = [self accessibilityState];
	if ( !state || !replacement )
		return;
	const auto info = state->infoFor( _ref );
	if ( !info.text.valid || !hasAction( info.actions, AccessibilityAction::SetText ) )
		return;
	NSMutableString* updated = [toNSString( info.value ) mutableCopy];
	const NSRange selected = textRange( info, info.value );
	if ( !validRange( selected, updated ) ) {
		EE_OBJC_RELEASE( updated );
		return;
	}
	[updated replaceCharactersInRange:selected withString:replacement];
	const String updatedText = String::fromUtf8( std::string( updated.UTF8String ?: "" ) );
	const NSUInteger insertionPoint = selected.location + replacement.length;
	if ( state->perform( _ref, AccessibilityAction::SetText, updatedText ) &&
		 hasAction( info.actions, AccessibilityAction::SetTextSelection ) ) {
		const Int32 caret = utf16ToCodePoint( updatedText, insertionPoint );
		state->perform( _ref, AccessibilityAction::SetTextSelection,
						String( std::to_string( caret ) + ":" + std::to_string( caret ) ) );
	}
	EE_OBJC_RELEASE( updated );
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSAccessibilityAttributeName)attribute {
	if ( [attribute isEqualToString:NSAccessibilityFocusedAttribute] && [value boolValue] ) {
		self.accessibilityFocused = YES;
	} else if ( [attribute isEqualToString:NSAccessibilitySelectedAttribute] &&
				[value boolValue] ) {
		self.accessibilitySelected = YES;
	} else if ( [attribute isEqualToString:NSAccessibilityExpandedAttribute] ) {
		self.accessibilityExpanded = [value boolValue];
	} else if ( [attribute isEqualToString:NSAccessibilityValueAttribute] ) {
		self.accessibilityValue = value;
	} else if ( [attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute] &&
				[value isKindOfClass:[NSValue class]] ) {
		self.accessibilitySelectedTextRange = [value rangeValue];
	} else if ( [attribute isEqualToString:NSAccessibilitySelectedTextRangesAttribute] &&
				[value isKindOfClass:[NSArray class]] ) {
		self.accessibilitySelectedTextRanges = value;
	} else if ( [attribute isEqualToString:NSAccessibilitySelectedTextAttribute] &&
				[value isKindOfClass:[NSString class]] ) {
		self.accessibilitySelectedText = value;
	}
}

- (id)accessibilityAttributeValue:(NSAccessibilityAttributeName)attribute {
	auto state = [self accessibilityState];
	if ( !state || !state->isValid( _ref ) )
		return nil;
	const auto info = state->infoFor( _ref );
	if ( [attribute isEqualToString:NSAccessibilityRoleAttribute] )
		return self.accessibilityRole;
	if ( [attribute isEqualToString:NSAccessibilitySubroleAttribute] )
		return self.accessibilitySubrole;
	if ( [attribute isEqualToString:NSAccessibilityRoleDescriptionAttribute] )
		return self.accessibilityRoleDescription;
	if ( [attribute isEqualToString:NSAccessibilityDescriptionAttribute] )
		return self.accessibilityLabel;
	if ( [attribute isEqualToString:NSAccessibilityHelpAttribute] )
		return self.accessibilityHelp;
	if ( [attribute isEqualToString:NSAccessibilityTitleAttribute] )
		return self.accessibilityTitle;
	if ( [attribute isEqualToString:NSAccessibilityValueAttribute] )
		return self.accessibilityValue;
	if ( [attribute isEqualToString:NSAccessibilityMinValueAttribute] )
		return self.accessibilityMinValue;
	if ( [attribute isEqualToString:NSAccessibilityMaxValueAttribute] )
		return self.accessibilityMaxValue;
	if ( [attribute isEqualToString:NSAccessibilityIdentifierAttribute] )
		return self.accessibilityIdentifier;
	if ( [attribute isEqualToString:NSAccessibilityEnabledAttribute] )
		return @( self.isAccessibilityEnabled );
	if ( [attribute isEqualToString:NSAccessibilityFocusedAttribute] )
		return @( self.isAccessibilityFocused );
	if ( [attribute isEqualToString:NSAccessibilitySelectedAttribute] )
		return @( self.isAccessibilitySelected );
	if ( [attribute isEqualToString:NSAccessibilityExpandedAttribute] )
		return @( self.isAccessibilityExpanded );
	if ( [attribute isEqualToString:NSAccessibilityHiddenAttribute] )
		return @( self.isAccessibilityHidden );
	if ( [attribute isEqualToString:NSAccessibilityContainsProtectedContentAttribute] )
		return @( self.isAccessibilityProtectedContent );
	if ( [attribute isEqualToString:NSAccessibilityChildrenAttribute] )
		return self.accessibilityChildren;
	if ( [attribute isEqualToString:NSAccessibilityVisibleChildrenAttribute] )
		return self.accessibilityVisibleChildren;
	if ( [attribute isEqualToString:NSAccessibilityChildrenInNavigationOrderAttribute] )
		return self.accessibilityChildrenInNavigationOrder;
	if ( [attribute isEqualToString:NSAccessibilityContentsAttribute] )
		return self.accessibilityContents;
	if ( [attribute isEqualToString:NSAccessibilitySelectedChildrenAttribute] )
		return self.accessibilitySelectedChildren;
	if ( [attribute isEqualToString:NSAccessibilityRowsAttribute] )
		return self.accessibilityRows;
	if ( [attribute isEqualToString:NSAccessibilityVisibleRowsAttribute] )
		return self.accessibilityVisibleRows;
	if ( [attribute isEqualToString:NSAccessibilitySelectedRowsAttribute] )
		return self.accessibilitySelectedRows;
	if ( [attribute isEqualToString:NSAccessibilityRowCountAttribute] )
		return @( self.accessibilityRows.count );
	if ( [attribute isEqualToString:NSAccessibilityIndexAttribute] )
		return @( self.accessibilityIndex );
	if ( [attribute isEqualToString:NSAccessibilityParentAttribute] )
		return self.accessibilityParent;
	if ( [attribute isEqualToString:NSAccessibilityWindowAttribute] )
		return self.accessibilityWindow;
	if ( [attribute isEqualToString:NSAccessibilityTopLevelUIElementAttribute] )
		return self.accessibilityTopLevelUIElement;
	if ( [attribute isEqualToString:NSAccessibilityLinkedUIElementsAttribute] )
		return self.accessibilityLinkedUIElements;
	if ( [attribute isEqualToString:NSAccessibilityPositionAttribute] )
		return [NSValue valueWithPoint:self.accessibilityFrame.origin];
	if ( [attribute isEqualToString:NSAccessibilitySizeAttribute] )
		return [NSValue valueWithSize:self.accessibilityFrame.size];
	if ( info.text.valid ) {
		const String text = info.value;
		NSString* string = toNSString( text );
		const NSRange selected = textRange( info, text );
		if ( [attribute isEqualToString:NSAccessibilitySelectedTextAttribute] )
			return [string substringWithRange:selected];
		if ( [attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute] )
			return [NSValue valueWithRange:selected];
		if ( [attribute isEqualToString:NSAccessibilitySelectedTextRangesAttribute] )
			return @[ [NSValue valueWithRange:selected] ];
		if ( [attribute isEqualToString:NSAccessibilityNumberOfCharactersAttribute] )
			return @( string.length );
		if ( [attribute isEqualToString:NSAccessibilityVisibleCharacterRangeAttribute] )
			return [NSValue valueWithRange:NSMakeRange( 0, string.length )];
		if ( [attribute isEqualToString:NSAccessibilityInsertionPointLineNumberAttribute] )
			return @( self.accessibilityInsertionPointLineNumber );
	}
	return nil;
}

- (NSArray*)accessibilityAttributeNames {
	auto state = [self accessibilityState];
	if ( !state || !state->isValid( _ref ) )
		return @[];
	const auto info = state->infoFor( _ref );
	NSMutableArray* names = [[NSMutableArray alloc] initWithArray:@[
		NSAccessibilityRoleAttribute, NSAccessibilityRoleDescriptionAttribute,
		NSAccessibilityIdentifierAttribute, NSAccessibilityEnabledAttribute,
		NSAccessibilityFocusedAttribute, NSAccessibilityChildrenAttribute,
		NSAccessibilityVisibleChildrenAttribute, NSAccessibilityChildrenInNavigationOrderAttribute,
		NSAccessibilityContentsAttribute, NSAccessibilityParentAttribute,
		NSAccessibilityWindowAttribute, NSAccessibilityTopLevelUIElementAttribute,
		NSAccessibilityPositionAttribute, NSAccessibilitySizeAttribute,
		NSAccessibilityHiddenAttribute
	]];
	if ( nativeSubrole( info ) )
		[names addObject:NSAccessibilitySubroleAttribute];
	const bool hasLabel =
		!info.name.empty() && info.role != AccessibilityRole::Label &&
		info.role != AccessibilityRole::Text &&
		!( info.name == info.value &&
		   ( info.role == AccessibilityRole::ListItem || info.role == AccessibilityRole::Row ||
			 info.role == AccessibilityRole::TreeItem ) );
	if ( hasLabel )
		[names addObject:NSAccessibilityDescriptionAttribute];
	if ( !info.description.empty() )
		[names addObject:NSAccessibilityHelpAttribute];
	const bool hasBooleanValue = info.role == AccessibilityRole::CheckBox ||
								 info.role == AccessibilityRole::RadioButton ||
								 info.role == AccessibilityRole::CheckMenuItem ||
								 info.role == AccessibilityRole::RadioMenuItem;
	const bool hasEditableTextValue = info.role == AccessibilityRole::TextBox &&
									  !hasState( info.states, AccessibilityState::Protected );
	if ( !info.value.empty() || info.range.valid || hasBooleanValue || hasEditableTextValue ||
		 info.role == AccessibilityRole::ComboBox || info.role == AccessibilityRole::Label ||
		 info.role == AccessibilityRole::Text )
		[names addObject:NSAccessibilityValueAttribute];
	if ( hasAction( info.actions, AccessibilityAction::Select ) ||
		 info.role == AccessibilityRole::Row || info.role == AccessibilityRole::ListItem ||
		 info.role == AccessibilityRole::TreeItem )
		[names addObject:NSAccessibilitySelectedAttribute];
	if ( hasAction( info.actions, AccessibilityAction::Expand ) ||
		 hasAction( info.actions, AccessibilityAction::Collapse ) )
		[names addObject:NSAccessibilityExpandedAttribute];
	if ( info.role == AccessibilityRole::Table || info.role == AccessibilityRole::List ||
		 info.role == AccessibilityRole::Tree ) {
		[names addObjectsFromArray:@[
			NSAccessibilitySelectedChildrenAttribute, NSAccessibilityRowsAttribute,
			NSAccessibilityVisibleRowsAttribute, NSAccessibilitySelectedRowsAttribute,
			NSAccessibilityRowCountAttribute
		]];
	}
	if ( info.role == AccessibilityRole::Row || info.role == AccessibilityRole::ListItem ||
		 info.role == AccessibilityRole::TreeItem || info.role == AccessibilityRole::Cell )
		[names addObject:NSAccessibilityIndexAttribute];
	if ( !info.relations.empty() )
		[names addObject:NSAccessibilityLinkedUIElementsAttribute];
	if ( info.range.valid )
		[names addObjectsFromArray:@[
			NSAccessibilityMinValueAttribute, NSAccessibilityMaxValueAttribute
		]];
	if ( hasState( info.states, AccessibilityState::Protected ) )
		[names addObject:NSAccessibilityContainsProtectedContentAttribute];
	if ( info.text.valid ) {
		[names addObjectsFromArray:@[
			NSAccessibilitySelectedTextAttribute, NSAccessibilitySelectedTextRangeAttribute,
			NSAccessibilitySelectedTextRangesAttribute, NSAccessibilityNumberOfCharactersAttribute,
			NSAccessibilityVisibleCharacterRangeAttribute,
			NSAccessibilityInsertionPointLineNumberAttribute
		]];
	}
	return EE_OBJC_AUTORELEASE( names );
}

- (NSUInteger)accessibilityIndexOfChild:(id)child {
	auto state = [self accessibilityState];
	return state ? state->indexOfChild( _ref, child ) : NSNotFound;
}

- (NSUInteger)accessibilityArrayAttributeCount:(NSAccessibilityAttributeName)attribute {
	if ( ![attribute isEqualToString:NSAccessibilityChildrenAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityVisibleChildrenAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityChildrenInNavigationOrderAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityRowsAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityVisibleRowsAttribute] )
		return 0;
	auto state = [self accessibilityState];
	return state ? state->childCountFor( _ref ) : 0;
}

- (NSArray*)accessibilityArrayAttributeValues:(NSAccessibilityAttributeName)attribute
										index:(NSUInteger)index
									 maxCount:(NSUInteger)maxCount {
	if ( ![attribute isEqualToString:NSAccessibilityChildrenAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityVisibleChildrenAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityChildrenInNavigationOrderAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityRowsAttribute] &&
		 ![attribute isEqualToString:NSAccessibilityVisibleRowsAttribute] )
		return @[];
	auto state = [self accessibilityState];
	return state ? state->childrenFor( _ref, index, maxCount ) : @[];
}

- (BOOL)accessibilityIsIgnored {
	return !self.isAccessibilityElement;
}

- (id)accessibilityHitTest:(NSPoint)point {
	auto state = [self accessibilityState];
	if ( !state || !NSPointInRect( point, self.accessibilityFrame ) )
		return nil;
	const auto ref = state->hitTest( point );
	return ref.isValid() ? state->elementFor( ref ) : self;
}

- (NSString*)accessibilitySelectedText {
	const auto info = [self nodeInfo];
	if ( !info.text.valid )
		return nil;
	NSString* string = toNSString( info.value );
	const NSRange selected = textRange( info, info.value );
	return validRange( selected, string ) ? [string substringWithRange:selected] : nil;
}

- (NSArray<NSValue*>*)accessibilitySelectedTextRanges {
	const auto info = [self nodeInfo];
	return info.text.valid ? @[ [NSValue valueWithRange:textRange( info, info.value )] ] : nil;
}

- (NSInteger)accessibilityNumberOfCharacters {
	const auto info = [self nodeInfo];
	return info.text.valid ? static_cast<NSInteger>( toNSString( info.value ).length ) : 0;
}

- (NSInteger)accessibilityInsertionPointLineNumber {
	const auto info = [self nodeInfo];
	return info.text.valid
			   ? [self accessibilityLineForIndex:static_cast<NSInteger>( codePointToUTF16(
													 info.value, info.text.caretOffset ) )]
			   : NSNotFound;
}

- (NSArray*)accessibilityParameterizedAttributeNames {
	if ( ![self nodeInfo].text.valid )
		return @[];
	return @[
		NSAccessibilityStringForRangeParameterizedAttribute,
		NSAccessibilityRangeForIndexParameterizedAttribute,
		NSAccessibilityLineForIndexParameterizedAttribute,
		NSAccessibilityRangeForLineParameterizedAttribute,
		NSAccessibilityBoundsForRangeParameterizedAttribute,
		NSAccessibilityAttributedStringForRangeParameterizedAttribute
	];
}

- (id)accessibilityAttributeValue:(NSAccessibilityParameterizedAttributeName)attribute
					 forParameter:(id)parameter {
	const auto info = [self nodeInfo];
	if ( !info.text.valid )
		return nil;
	NSString* string = toNSString( info.value );
	if ( [attribute isEqualToString:NSAccessibilityStringForRangeParameterizedAttribute] ||
		 [attribute
			 isEqualToString:NSAccessibilityAttributedStringForRangeParameterizedAttribute] ||
		 [attribute isEqualToString:NSAccessibilityBoundsForRangeParameterizedAttribute] ) {
		if ( ![parameter isKindOfClass:[NSValue class]] )
			return nil;
		NSRange range;
		[parameter getValue:&range];
		if ( !validRange( range, string ) )
			return nil;
		if ( [attribute isEqualToString:NSAccessibilityStringForRangeParameterizedAttribute] )
			return [string substringWithRange:range];
		if ( [attribute
				 isEqualToString:NSAccessibilityAttributedStringForRangeParameterizedAttribute] )
			return EE_OBJC_AUTORELEASE(
				[[NSAttributedString alloc] initWithString:[string substringWithRange:range]] );
		return [NSValue valueWithRect:self.accessibilityFrame];
	}
	if ( [attribute isEqualToString:NSAccessibilityRangeForIndexParameterizedAttribute] &&
		 [parameter respondsToSelector:@selector( unsignedIntegerValue )] ) {
		const NSUInteger index = [parameter unsignedIntegerValue];
		if ( index >= string.length )
			return nil;
		return [NSValue valueWithRange:[string rangeOfComposedCharacterSequenceAtIndex:index]];
	}
	if ( [attribute isEqualToString:NSAccessibilityLineForIndexParameterizedAttribute] &&
		 [parameter respondsToSelector:@selector( integerValue )] )
		return @( [self accessibilityLineForIndex:[parameter integerValue]] );
	if ( [attribute isEqualToString:NSAccessibilityRangeForLineParameterizedAttribute] &&
		 [parameter respondsToSelector:@selector( integerValue )] )
		return [NSValue valueWithRange:[self accessibilityRangeForLine:[parameter integerValue]]];
	return nil;
}

- (NSRange)accessibilitySelectedTextRange {
	const auto info = [self nodeInfo];
	return textRange( info, info.value );
}

- (NSRange)accessibilityVisibleCharacterRange {
	const auto info = [self nodeInfo];
	return info.text.valid ? NSMakeRange( 0, toNSString( info.value ).length )
						   : NSMakeRange( 0, 0 );
}

- (NSRange)accessibilityRangeForIndex:(NSInteger)index {
	const auto info = [self nodeInfo];
	NSString* string = toNSString( info.value );
	return index >= 0 && static_cast<NSUInteger>( index ) < string.length
			   ? [string rangeOfComposedCharacterSequenceAtIndex:static_cast<NSUInteger>( index )]
			   : NSMakeRange( NSNotFound, 0 );
}

- (NSString*)accessibilityStringForRange:(NSRange)range {
	const auto info = [self nodeInfo];
	if ( !info.text.valid )
		return nil;
	NSString* string = toNSString( info.value );
	return validRange( range, string ) ? [string substringWithRange:range] : nil;
}

- (NSAttributedString*)accessibilityAttributedStringForRange:(NSRange)range {
	NSString* string = [self accessibilityStringForRange:range];
	return string ? EE_OBJC_AUTORELEASE( [[NSAttributedString alloc] initWithString:string] ) : nil;
}

- (NSInteger)accessibilityLineForIndex:(NSInteger)index {
	const auto info = [self nodeInfo];
	NSString* string = toNSString( info.value );
	if ( !info.text.valid || index < 0 || static_cast<NSUInteger>( index ) > string.length )
		return NSNotFound;
	NSInteger line = 0;
	for ( NSInteger offset = 0; offset < index; ++offset ) {
		if ( [string characterAtIndex:static_cast<NSUInteger>( offset )] == '\n' )
			++line;
	}
	return line;
}

- (NSRange)accessibilityRangeForLine:(NSInteger)wantedLine {
	const auto info = [self nodeInfo];
	NSString* string = toNSString( info.value );
	if ( !info.text.valid || wantedLine < 0 )
		return NSMakeRange( NSNotFound, 0 );
	NSUInteger start = 0;
	for ( NSInteger line = 0; start <= string.length; ++line ) {
		NSUInteger lineStart = 0;
		NSUInteger lineEnd = 0;
		[string getLineStart:&lineStart
						 end:&lineEnd
				 contentsEnd:nil
					forRange:NSMakeRange( start, 0 )];
		if ( line == wantedLine )
			return NSMakeRange( lineStart, lineEnd - lineStart );
		if ( lineEnd <= start )
			break;
		start = lineEnd;
	}
	return NSMakeRange( NSNotFound, 0 );
}

- (NSRect)accessibilityFrameForRange:(NSRange)range {
	(void)range;
	// The shared query model exposes element geometry, but not per-glyph layout. Returning the
	// element's screen frame is conservative and keeps range geometry in the correct coordinate
	// space until glyph-level bounds become available in shared text semantics.
	return self.accessibilityFrame;
}

@end

class MacAccessibilityRegistry {
  public:
	struct WindowChanges {
		bool main{ false };
		bool focused{ false };
	};

	static MacAccessibilityRegistry& instance() {
		static MacAccessibilityRegistry registry;
		return registry;
	}

	std::shared_ptr<MacAccessibilityState> registerManager( AccessibilityManager& manager ) {
		if ( !isMainThread() )
			return {};
		UISceneNode* scene = manager.getSceneNode();
		EE::Window::Window* window = scene ? scene->getWindow() : nullptr;
		NSWindow* nativeWindow =
			window ? reinterpret_cast<NSWindow*>( window->getWindowHandler() ) : nil;
		const std::string windowId = window ? std::to_string( window->getWindowID() ) : "0";
		auto state = std::make_shared<MacAccessibilityState>( manager, nativeWindow, windowId );
		if ( gAccessibilityClientObserved )
			manager.onNativeClientObserved();
		if ( !nativeWindow || !NSApp )
			return state;
		NSString* identifier = toNSString( "eepp.window." + windowId );
		NSView* contentView = [nativeWindow contentView];
		const NSRect contentFrame = [nativeWindow contentRectForFrameRect:[nativeWindow frame]];
		EEPPMacAccessibilityWindowElement* element =
			[EEPPMacAccessibilityWindowElement accessibilityElementWithRole:NSAccessibilityGroupRole
																	  frame:contentFrame
																	  label:nil
																	 parent:contentView];
		[element configureWithState:state identifier:identifier];
		state->setWindowElement( element );
		mStates.push_back( state );
		[mRetainedWindows addObject:element];
		// SDL owns the NSWindow and content NSView. Keep AppKit's native application/window
		// hierarchy and add one lazy semantic root below the content view.
		addAccessibilityChild( contentView, element );
		const auto changes = syncApplication( gAccessibilityClientObserved );
		if ( gAccessibilityClientObserved ) {
			NSAccessibilityPostNotification( nativeWindow,
											 NSAccessibilityLayoutChangedNotification );
			postWindowChanges( changes );
		}
		return state;
	}

	void unregisterState( const std::shared_ptr<MacAccessibilityState>& state ) {
		if ( !state || !isMainThread() )
			return;
		EEPPMacAccessibilityWindowElement* element = state->windowElement();
		NSWindow* nativeWindow = state->nativeWindow();
		const bool observed = gAccessibilityClientObserved;
		if ( observed && element )
			NSAccessibilityPostNotification( element,
											 NSAccessibilityUIElementDestroyedNotification );
		if ( nativeWindow )
			removeAccessibilityChild( [nativeWindow contentView], element );
		for ( size_t i = 0; i < mStates.size(); ++i ) {
			if ( mStates[i] == state ) {
				mStates.erase( mStates.begin() + i );
				break;
			}
		}
		state->invalidate();
		const auto changes = syncApplication( gAccessibilityClientObserved );
		if ( element )
			[mRetainedWindows removeObject:element];
		if ( observed && NSApp ) {
			NSAccessibilityPostNotification( NSApp, NSAccessibilityLayoutChangedNotification );
			postWindowChanges( changes );
		}
	}

	void onSourceInvalidated( const std::shared_ptr<MacAccessibilityState>& state,
							  AccessibilitySourceId source ) {
		if ( state )
			state->evictSource( source );
	}

	void onEvent( const std::shared_ptr<MacAccessibilityState>& state,
				  const AccessibilityPendingEvent& event ) {
		if ( !state || !state->hasActiveClient() || !isMainThread() )
			return;
		if ( event.type == AccessibilityEvent::ChildrenChanged )
			state->evictInvalidElements();
		EEPPMacAccessibilityElement* target = state->cachedElementFor( event.ref );
		EEPPMacAccessibilityElement* related = state->cachedElementFor( event.related );
		const AccessibilityNodeRef relatedRef = event.related;
		if ( !target && event.type != AccessibilityEvent::Destroyed )
			target = state->elementFor( event.ref );
		if ( event.type == AccessibilityEvent::FocusChanged ) {
			postWindowChanges( syncApplication( true ) );
			const auto focusedRef = state->focused();
			id focusedElement = focusedRef.isValid() ? state->elementFor( focusedRef ) : nil;
			if ( focusedElement )
				NSAccessibilityPostNotification(
					focusedElement, NSAccessibilityFocusedUIElementChangedNotification );
			return;
		}
		if ( event.type == AccessibilityEvent::Destroyed ) {
			EEPPMacAccessibilityElement* destroyed = related ? related : target;
			if ( destroyed )
				NSAccessibilityPostNotification( destroyed,
												 NSAccessibilityUIElementDestroyedNotification );
			if ( relatedRef.isValid() )
				state->evict( relatedRef );
			else
				state->evict( event.ref );
			if ( target && target != destroyed )
				NSAccessibilityPostNotification( target, NSAccessibilityLayoutChangedNotification );
		} else if ( event.type == AccessibilityEvent::StateChanged ) {
			if ( target ) {
				const auto info = state->infoFor( event.ref );
				if ( info.role == AccessibilityRole::TreeItem &&
					 ( hasAction( info.actions, AccessibilityAction::Expand ) ||
					   hasAction( info.actions, AccessibilityAction::Collapse ) ) )
					NSAccessibilityPostNotification(
						target, hasState( info.states, AccessibilityState::Expanded )
									? NSAccessibilityRowExpandedNotification
									: NSAccessibilityRowCollapsedNotification );
				else
					NSAccessibilityPostNotification( target,
													 NSAccessibilityValueChangedNotification );
			}
		} else if ( event.type == AccessibilityEvent::ValueChanged ||
					event.type == AccessibilityEvent::EnabledChanged ) {
			if ( target )
				NSAccessibilityPostNotification( target, NSAccessibilityValueChangedNotification );
		} else if ( event.type == AccessibilityEvent::NameChanged ) {
			if ( target )
				NSAccessibilityPostNotification( target, NSAccessibilityTitleChangedNotification );
		} else if ( event.type == AccessibilityEvent::SelectionChanged ) {
			if ( target ) {
				const auto info = state->infoFor( event.ref );
				const auto notification = info.text.valid
											  ? NSAccessibilitySelectedTextChangedNotification
										  : ( info.role == AccessibilityRole::Table ||
											  info.role == AccessibilityRole::List ||
											  info.role == AccessibilityRole::Tree )
											  ? NSAccessibilitySelectedRowsChangedNotification
											  : NSAccessibilitySelectedChildrenChangedNotification;
				NSAccessibilityPostNotification( target, notification );
			}
		} else if ( event.type == AccessibilityEvent::BoundsChanged ) {
			if ( target )
				NSAccessibilityPostNotification( target, NSAccessibilityLayoutChangedNotification );
		} else if ( event.type == AccessibilityEvent::Created ) {
			if ( !related && event.related.isValid() )
				related = state->elementFor( event.related );
			if ( related )
				NSAccessibilityPostNotification( related, NSAccessibilityCreatedNotification );
			if ( target ) {
				NSDictionary* userInfo = related
											 ? @{NSAccessibilityUIElementsKey : @[ related ]}
											 : nil;
				NSAccessibilityPostNotificationWithUserInfo(
					target, NSAccessibilityLayoutChangedNotification, userInfo );
			}
		} else if ( target ) {
			NSAccessibilityPostNotification( target, NSAccessibilityLayoutChangedNotification );
		}
	}

	void clientObserved() {
		if ( gAccessibilityClientObserved || !isMainThread() )
			return;
		gAccessibilityClientObserved = true;
		for ( const auto& state : mStates ) {
			if ( auto manager = state->manager() )
				manager->onNativeClientObserved();
		}
		// macOS has no supported equivalent to UiaClientsAreListening() or AT-SPI listener
		// registration. The first actual NSAccessibility query activates notifications and focus
		// projection. There is no process polling or per-frame Cocoa work while dormant.
		// Do not mutate NSApplication while AppKit is serializing that first query. Deferring the
		// focused-element projection to the next scene update avoids re-entering AXWindows.
		mNeedsFocusSync = true;
	}

	void update() {
		if ( !gAccessibilityClientObserved || !NSApp || !isMainThread() )
			return;
		if ( mNeedsFocusSync || [NSApp keyWindow] != mLastKeyWindow ||
			 [NSApp mainWindow] != mLastMainWindow ) {
			postWindowChanges( syncApplication( true ) );
			mNeedsFocusSync = false;
		}
	}

  private:
	MacAccessibilityRegistry() : mRetainedWindows( [[NSMutableArray alloc] init] ) {
		// Secure text controls use AXSecureTextField and never vend their value or text ranges.
		// Do not call NSAccessibilitySetMayContainProtectedContent() here: enabling that global
		// mode made the native AXWindows attribute unavailable to ordinary trusted AX clients on
		// macOS 15, preventing application discovery before any semantic element was queried.
	}

	~MacAccessibilityRegistry() { EE_OBJC_RELEASE( mRetainedWindows ); }

	WindowChanges syncApplication( bool resolveFocus ) {
		if ( !NSApp || !isMainThread() )
			return {};
		NSWindow* nativeMainWindow = [NSApp mainWindow];
		NSWindow* nativeFocusedWindow = [NSApp keyWindow];
		const WindowChanges changes{ nativeMainWindow != mLastMainWindow,
									 nativeFocusedWindow != mLastKeyWindow };
		if ( resolveFocus ) {
			id focusedElement = nil;
			for ( const auto& state : mStates ) {
				if ( nativeFocusedWindow && state->nativeWindow() != nativeFocusedWindow )
					continue;
				const auto ref = state->focused();
				if ( ref.isValid() ) {
					focusedElement = state->elementFor( ref );
					if ( focusedElement )
						break;
				}
			}
			[NSApp setAccessibilityApplicationFocusedUIElement:focusedElement];
		}
		mLastMainWindow = nativeMainWindow;
		mLastKeyWindow = nativeFocusedWindow;
		return changes;
	}

	void postWindowChanges( const WindowChanges& changes ) {
		if ( !NSApp )
			return;
		if ( changes.main )
			NSAccessibilityPostNotification( NSApp, NSAccessibilityMainWindowChangedNotification );
		if ( changes.focused )
			NSAccessibilityPostNotification( NSApp,
											 NSAccessibilityFocusedWindowChangedNotification );
	}

	std::vector<std::shared_ptr<MacAccessibilityState>> mStates;
	NSMutableArray* mRetainedWindows{ nil };
	NSWindow* mLastMainWindow{ nil };
	NSWindow* mLastKeyWindow{ nil };
	bool mNeedsFocusSync{ false };
};

static void markAccessibilityClientObserved() {
	MacAccessibilityRegistry::instance().clientObserved();
}

class MacAccessibilityBackend final : public AccessibilityBackend {
  public:
	explicit MacAccessibilityBackend( AccessibilityManager& manager ) :
		mState( MacAccessibilityRegistry::instance().registerManager( manager ) ) {}

	~MacAccessibilityBackend() override {
		MacAccessibilityRegistry::instance().unregisterState( mState );
	}

	bool isAvailable() const override { return mState && mState->nativeWindow() != nil; }

	bool hasActiveClients() const override { return mState && mState->hasActiveClient(); }

	void update() override {
		if ( !mState || !mState->hasActiveClient() )
			return;
		for ( const auto& event : mPendingEvents )
			MacAccessibilityRegistry::instance().onEvent( mState, event );
		mPendingEvents.clear();
		MacAccessibilityRegistry::instance().update();
	}

	void onEvent( const AccessibilityPendingEvent& event ) override {
		if ( !mState || !mState->hasActiveClient() )
			return;
		// UI and document callbacks can hold eepp locks. Queue stable identifiers and enter AppKit
		// only from the next main-thread scene update. Focus is process-global, so one final-state
		// notification per update is sufficient even when both the old and new widget report it.
		if ( event.type == AccessibilityEvent::FocusChanged ) {
			for ( auto& pending : mPendingEvents ) {
				if ( pending.type == AccessibilityEvent::FocusChanged ) {
					pending = event;
					return;
				}
			}
		}
		mPendingEvents.emplace_back( event );
	}

	void onSourceInvalidated( AccessibilitySourceId source ) override {
		// AppKit drops UIElementDestroyed once the source no longer resolves. The manager calls
		// this on the main thread immediately before reset(), outside model locks, so destruction
		// is the one notification that must be delivered synchronously while wrappers are valid.
		MacAccessibilityRegistry::instance().onSourceInvalidated( mState, source );
	}

  private:
	std::shared_ptr<MacAccessibilityState> mState;
	std::vector<AccessibilityPendingEvent> mPendingEvents;
};

namespace EE { namespace UI {

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager ) {
	return std::make_unique<MacAccessibilityBackend>( manager );
}

}} // namespace EE::UI
