#include <algorithm>
#include <cmath>
#include <eepp/core/containers.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextselectioncontroller.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/keycodes.hpp>
#include <eepp/window/window.hpp>
#include <limits>

using namespace EE::Scene;

namespace EE { namespace UI {

static const Node* selectionSourceNode( RichText::InlineSource source ) {
	const Node* node = nullptr;
	if ( source.type == RichText::InlineSourceType::Widget )
		node = static_cast<const UIWidget*>( source.ptr );
	else if ( source.type == RichText::InlineSourceType::TextNode )
		node = static_cast<const UITextNode*>( source.ptr );
	return node;
}

static const UIHTMLWidget* selectionElementForSource( const UIRichText* owner,
													  RichText::InlineSource source ) {
	const Node* node = selectionSourceNode( source );
	for ( ; node; node = node->getParent() ) {
		if ( node->isType( UI_TYPE_HTML_WIDGET ) )
			return node->asConstType<UIHTMLWidget>();
		if ( node == owner )
			break;
	}
	return owner;
}

static RichText::InlineSource selectionFragmentSource( const UIRichText* owner,
													   const RichText::InlineFragment& fragment ) {
	RichText::InlineSource source = fragment.source;
	if ( source.type == RichText::InlineSourceType::None ) {
		// Some layout fragments only retain an item path; follow it back to the HTML source.
		const auto* items = &owner->getRichText().getInlineItems();
		for ( size_t index : fragment.itemPath ) {
			if ( index >= items->size() )
				break;
			const auto& item = ( *items )[index];
			if ( item.isBox() ) {
				if ( item.asBox().source.ptr )
					source = item.asBox().source;
				items = &item.asBox().children;
			} else {
				if ( item.isTextRun() && item.asTextRun().source.ptr )
					source = item.asTextRun().source;
				else if ( item.isAtomicBox() && item.asAtomicBox().source.ptr )
					source = item.asAtomicBox().source;
				break;
			}
		}
	}
	return source;
}

static CSSUserSelect selectionPolicyForFragment( const UIRichText* owner,
												 const RichText::InlineFragment& fragment ) {
	auto source = selectionFragmentSource( owner, fragment );
	for ( const Node* node = selectionSourceNode( source ); node && node != owner;
		  node = node->getParent() ) {
		if ( node->isType( UI_TYPE_RICHTEXT ) &&
			 !node->asConstType<UIRichText>()->isTextSelectionEnabled() )
			return CSSUserSelect::None;
	}
	return selectionElementForSource( owner, source )->getUsedUserSelect();
}

static bool nodeWithin( const Node* node, const Node* ancestor ) {
	for ( ; node; node = node->getParent() ) {
		if ( node == ancestor )
			return true;
	}
	return false;
}

static const UIAnchorSpan* linkForNode( const Node* node, const Node* root ) {
	for ( ; node; node = node->getParent() ) {
		if ( node->isType( UI_TYPE_TEXTSPAN ) ) {
			auto* span = node->asConstType<UITextSpan>();
			if ( span->getElementTag() == "a" )
				return static_cast<const UIAnchorSpan*>( span );
		}
		if ( node == root )
			break;
	}
	return nullptr;
}

static bool precedesInTree( const Node* left, const Node* right ) {
	if ( left == right )
		return false;
	size_t leftDepth = 0;
	size_t rightDepth = 0;
	for ( const Node* node = left; node; node = node->getParent() )
		++leftDepth;
	for ( const Node* node = right; node; node = node->getParent() )
		++rightDepth;
	const bool leftIsShallower = leftDepth < rightDepth;
	while ( leftDepth > rightDepth ) {
		left = left->getParent();
		--leftDepth;
	}
	while ( rightDepth > leftDepth ) {
		right = right->getParent();
		--rightDepth;
	}
	if ( left == right )
		return leftIsShallower;
	while ( left->getParent() != right->getParent() ) {
		left = left->getParent();
		right = right->getParent();
	}
	for ( const Node* sibling = left->getNextNode(); sibling; sibling = sibling->getNextNode() ) {
		if ( sibling == right )
			return true;
	}
	return false;
}

static const UIHTMLWidget* selectionElementAtPoint( const UIRichText* owner, Int64 offset ) {
	const auto& fragments = owner->getRichText().getInlineFragments();
	for ( const auto& fragment : fragments ) {
		if ( fragment.type == RichText::InlineFragment::Type::Box )
			continue;
		if ( offset >= fragment.startCharIndex && offset < fragment.endCharIndex )
			return selectionElementForSource( owner, selectionFragmentSource( owner, fragment ) );
	}
	return owner;
}

static bool isIndependentlyRenderedFragment( const UIRichText* owner,
											 const RichText::InlineFragment& fragment,
											 const UnorderedSet<const Node*>& ownerSubtreeNodes ) {
	// An atomic slot may stand for a nested widget that paints its own RichText. Its parent
	// placeholder must not be selected or copied in addition to that descendant owner.
	if ( fragment.type != RichText::InlineFragment::Type::AtomicBox )
		return false;
	const Node* source = selectionSourceNode( selectionFragmentSource( owner, fragment ) );
	return source && source != owner && nodeWithin( source, owner ) &&
		   ownerSubtreeNodes.find( source ) != ownerSubtreeNodes.end();
}

static bool hasOwnSelectionContent( const UIRichText* owner,
									const UnorderedSet<const Node*>& ownerSubtreeNodes ) {
	const auto& fragments = owner->getRichText().getInlineFragments();
	if ( fragments.empty() )
		return owner->getTextCharacterCount() > 0;
	for ( const auto& fragment : fragments ) {
		if ( fragment.type != RichText::InlineFragment::Type::Box &&
			 fragment.startCharIndex < fragment.endCharIndex &&
			 !isIndependentlyRenderedFragment( owner, fragment, ownerSubtreeNodes ) )
			return true;
	}
	return false;
}

static SmallVector<TextSelectionRange, 4>
selectionExclusions( const UIRichText* owner, const UnorderedSet<const Node*>& ownerSubtreeNodes ) {
	// Feed the same omitted ranges to range resolution and RichText's paint/copy paths.
	SmallVector<TextSelectionRange, 4> exclusions;
	const auto& fragments = owner->getRichText().getInlineFragments();
	for ( const auto& fragment : fragments ) {
		if ( fragment.type == RichText::InlineFragment::Type::Box ||
			 fragment.startCharIndex >= fragment.endCharIndex )
			continue;
		if ( isIndependentlyRenderedFragment( owner, fragment, ownerSubtreeNodes ) ||
			 selectionPolicyForFragment( owner, fragment ) == CSSUserSelect::None )
			exclusions.push_back( { fragment.startCharIndex, fragment.endCharIndex } );
	}
	if ( fragments.empty() && owner->getUsedUserSelect() == CSSUserSelect::None )
		exclusions.push_back( { 0, owner->getTextCharacterCount() } );
	std::sort( exclusions.begin(), exclusions.end(),
			   []( const auto& a, const auto& b ) { return a.start < b.start; } );
	SmallVector<TextSelectionRange, 4> merged;
	for ( const auto& range : exclusions ) {
		if ( !merged.empty() && range.start <= merged.back().end )
			merged.back().end = std::max( merged.back().end, range.end );
		else
			merged.push_back( range );
	}
	return merged;
}

static bool hasSelectableInterval( Int64 start, Int64 end,
								   const SmallVector<TextSelectionRange, 4>& exclusions ) {
	for ( const auto& range : exclusions ) {
		if ( range.end <= start )
			continue;
		if ( range.start > start )
			return true;
		start = std::max( start, range.end );
		if ( start >= end )
			return false;
	}
	return start < end;
}

static Int64 firstSelectableOffset( Int64 start,
									const SmallVector<TextSelectionRange, 4>& exclusions ) {
	Int64 offset = start;
	for ( const auto& range : exclusions ) {
		if ( range.start > offset )
			break;
		offset = std::max( offset, range.end );
	}
	return offset;
}

static Int64 lastSelectableOffset( Int64 end,
								   const SmallVector<TextSelectionRange, 4>& exclusions ) {
	Int64 offset = end;
	for ( const auto& range : exclusions ) {
		if ( range.start >= offset )
			break;
		if ( range.end >= offset )
			offset = range.start;
	}
	return offset;
}

UITextSelectionController::UITextSelectionController( UIWidget* host ) : mHost( host ) {}

UITextSelectionController::~UITextSelectionController() {
	clear();
}

void UITextSelectionController::setHost( UIWidget* host ) {
	mHost = host;
}

void UITextSelectionController::setSelectionRoot( UIWidget* root ) {
	clear();
	mRoot = root;
}

bool UITextSelectionController::isDescendantOf( const UIWidget* widget, const UIWidget* ancestor ) {
	for ( const Node* node = widget; node; node = node->getParent() ) {
		if ( node == ancestor )
			return true;
	}
	return false;
}

void UITextSelectionController::collectOwners() {
	// Drop the previous destructor registrations before rebuilding the owner list.
	for ( auto* owner : mOwners )
		owner->mDocumentSelectionController = nullptr;
	mOwners.clear();
	mOwnerSubtreeNodes.clear();
	mSlices.clear();
	if ( !mRoot )
		return;
	SmallVector<Node*, 32> stack;
	stack.push_back( mRoot );
	while ( !stack.empty() ) {
		Node* node = stack.back();
		stack.pop_back();
		if ( !node->isVisible() )
			continue;
		if ( node->isType( UI_TYPE_HTML_WIDGET ) ) {
			auto* html = node->asType<UIHTMLWidget>();
			if ( html->getDisplay() == CSSDisplay::None ||
				 html->getVisibility() != CSSVisibility::Visible )
				continue;
		}
		if ( node->isType( UI_TYPE_RICHTEXT ) ) {
			auto* text = node->asType<UIRichText>();
			if ( text->isTextSelectionEnabled() && text->isTextSelectionOwner() )
				mOwners.push_back( text );
		}
		SmallVector<Node*, 16> children;
		for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() )
			children.push_back( child );
		for ( auto it = children.rbegin(); it != children.rend(); ++it )
			stack.push_back( *it );
	}
	for ( auto* owner : mOwners ) {
		for ( const Node* node = owner; node; node = node->getParent() ) {
			mOwnerSubtreeNodes.insert( node );
			if ( node == mRoot )
				break;
		}
	}
	SmallVector<UIRichText*, 8> selectableOwners;
	for ( auto* owner : mOwners ) {
		if ( hasOwnSelectionContent( owner, mOwnerSubtreeNodes ) )
			selectableOwners.push_back( owner );
	}
	mOwners = std::move( selectableOwners );
	// Registered owners notify us before their RichText members are destroyed.
	for ( auto* owner : mOwners )
		owner->mDocumentSelectionController = this;
	// Split a parent owner at each independently painted child. Sorting these slices by
	// source tree order places child text between the parent's text before and after it.
	auto appendSlice = [&]( UIRichText* owner, Int64 start, Int64 end ) {
		if ( start >= end )
			return;
		const auto& fragments = owner->getRichText().getInlineFragments();
		const Node* source = nullptr;
		bool hasContent = fragments.empty();
		for ( const auto& fragment : fragments ) {
			if ( fragment.type == RichText::InlineFragment::Type::Box ||
				 fragment.endCharIndex <= start || fragment.startCharIndex >= end ||
				 isIndependentlyRenderedFragment( owner, fragment, mOwnerSubtreeNodes ) )
				continue;
			hasContent = true;
			source = selectionSourceNode( selectionFragmentSource( owner, fragment ) );
			if ( source )
				break;
		}
		if ( !hasContent )
			return;
		if ( !source || !nodeWithin( source, owner ) )
			source = owner;
		mSlices.push_back( { owner, start, end, source } );
	};
	for ( auto* owner : mOwners ) {
		Int64 start = 0;
		for ( const auto& fragment : owner->getRichText().getInlineFragments() ) {
			if ( !isIndependentlyRenderedFragment( owner, fragment, mOwnerSubtreeNodes ) )
				continue;
			appendSlice( owner, start, fragment.startCharIndex );
			start = fragment.endCharIndex;
		}
		appendSlice( owner, start, owner->getTextCharacterCount() );
	}
	// Sort only the text-bearing slices. Comparing their tree paths avoids an order map
	// with an entry and allocation for every non-text document node.
	std::sort( mSlices.begin(), mSlices.end(), []( const auto& a, const auto& b ) {
		if ( a.source != b.source )
			return precedesInTree( a.source, b.source );
		if ( a.owner != b.owner )
			return precedesInTree( a.owner, b.owner );
		return a.start < b.start;
	} );
	// Source nodes can be replaced without destroying their paint owner; only the order survives.
	for ( auto& slice : mSlices )
		slice.source = nullptr;
}

void UITextSelectionController::onOwnerWillBeDestroyed( UIRichText* owner ) {
	// OnClose arrives from the base destructor, after the owner's RichText has been torn down.
	if ( std::find( mOwners.begin(), mOwners.end(), owner ) != mOwners.end() )
		clear();
}

int UITextSelectionController::indexOf( const UIRichText* owner ) const {
	for ( size_t i = 0; i < mOwners.size(); ++i ) {
		if ( mOwners[i] == owner )
			return static_cast<int>( i );
	}
	return -1;
}

int UITextSelectionController::sliceIndexOf( Point point ) const {
	int previous = -1;
	for ( size_t i = 0; i < mSlices.size(); ++i ) {
		const auto& slice = mSlices[i];
		if ( slice.owner != point.owner )
			continue;
		if ( point.offset < slice.start || point.offset <= slice.end )
			return static_cast<int>( i );
		previous = static_cast<int>( i );
	}
	return previous;
}

int UITextSelectionController::compare( Point a, Point b ) const {
	// Offsets in the same paint owner remain ordered even when child slices lie between them.
	if ( a.owner == b.owner )
		return a.offset < b.offset ? -1 : a.offset > b.offset ? 1 : 0;
	int ai = sliceIndexOf( a );
	int bi = sliceIndexOf( b );
	if ( ai < 0 || bi < 0 ) {
		ai = indexOf( a.owner );
		bi = indexOf( b.owner );
	}
	if ( ai != bi )
		return ai < bi ? -1 : 1;
	return 0;
}

UITextSelectionController::Point
UITextSelectionController::firstPointIn( const UIHTMLWidget* subtree ) const {
	for ( const auto& slice : mSlices ) {
		auto* owner = slice.owner;
		auto exclusions = selectionExclusions( owner, mOwnerSubtreeNodes );
		if ( isDescendantOf( owner, subtree ) &&
			 hasSelectableInterval( slice.start, slice.end, exclusions ) )
			return { owner, firstSelectableOffset( slice.start, exclusions ) };
		if ( nodeWithin( subtree, owner ) ) {
			for ( const auto& fragment : owner->getRichTextPtr()->getInlineFragments() ) {
				if ( fragment.type == RichText::InlineFragment::Type::Box ||
					 fragment.endCharIndex <= slice.start || fragment.startCharIndex >= slice.end ||
					 selectionPolicyForFragment( owner, fragment ) == CSSUserSelect::None )
					continue;
				if ( nodeWithin( selectionSourceNode( selectionFragmentSource( owner, fragment ) ),
								 subtree ) )
					return { owner, std::max( slice.start, fragment.startCharIndex ) };
			}
		}
	}
	return {};
}

UITextSelectionController::Point
UITextSelectionController::lastPointIn( const UIHTMLWidget* subtree ) const {
	for ( auto it = mSlices.rbegin(); it != mSlices.rend(); ++it ) {
		auto* owner = it->owner;
		auto exclusions = selectionExclusions( owner, mOwnerSubtreeNodes );
		if ( isDescendantOf( owner, subtree ) &&
			 hasSelectableInterval( it->start, it->end, exclusions ) )
			return { owner, lastSelectableOffset( it->end, exclusions ) };
		if ( nodeWithin( subtree, owner ) ) {
			const auto& fragments = owner->getRichTextPtr()->getInlineFragments();
			for ( auto fragment = fragments.rbegin(); fragment != fragments.rend(); ++fragment ) {
				if ( fragment->type == RichText::InlineFragment::Type::Box ||
					 fragment->endCharIndex <= it->start || fragment->startCharIndex >= it->end ||
					 selectionPolicyForFragment( owner, *fragment ) == CSSUserSelect::None )
					continue;
				if ( nodeWithin( selectionSourceNode( selectionFragmentSource( owner, *fragment ) ),
								 subtree ) )
					return { owner, std::min( it->end, fragment->endCharIndex ) };
			}
		}
	}
	return {};
}

bool UITextSelectionController::hasSelection() const {
	return !mSelectedOwners.empty();
}

void UITextSelectionController::clear() {
	mMenuItemConnection.disconnect();
	mMenuCloseConnection.disconnect();
	if ( mCurrentMenu ) {
		mCurrentMenu->close();
		mCurrentMenu = nullptr;
	}
	if ( mDragging && mHost && mHost->getInput() )
		mHost->getInput()->captureMouse( false );
	mDragging = false;
	mExplicitSelectAll = false;
	mSuppressClick = false;
	for ( auto* owner : mOwners )
		owner->mDocumentSelectionController = nullptr;
	for ( auto* owner : mSelectedOwners )
		owner->setTextSelectionRange( { 0, 0 } );
	mSelectedOwners.clear();
	mSelectedSlices.clear();
	mOwners.clear();
	mSlices.clear();
	mOwnerSubtreeNodes.clear();
	mRawSelection = {};
	mResolvedSelection = {};
	mContainRoot = nullptr;
	mScrollTarget = nullptr;
}

void UITextSelectionController::onDocumentWillChange() {
	clear();
}

void UITextSelectionController::onDocumentChanged() {
	collectOwners();
}

void UITextSelectionController::refresh() {
	if ( mRawSelection.isValid() ) {
		// Re-resolve a CSS/capability change from the original endpoints. Explicit Select All
		// ignores containment even though setSelection() normally records a contain root.
		Selection raw = mRawSelection;
		bool explicitSelectAll = mExplicitSelectAll;
		setSelection( raw.anchor, raw.focus );
		if ( explicitSelectAll ) {
			mExplicitSelectAll = true;
			mContainRoot = nullptr;
			resolveAndProject();
		}
	}
}

void UITextSelectionController::setSelection( Point anchor, Point focus ) {
	collectOwners();
	if ( indexOf( anchor.owner ) < 0 || indexOf( focus.owner ) < 0 ) {
		clear();
		return;
	}
	mExplicitSelectAll = false;
	mContainRoot = nullptr;
	for ( const Node* node = selectionElementAtPoint( anchor.owner, anchor.offset ); node;
		  node = node->getParent() ) {
		if ( node->isType( UI_TYPE_HTML_WIDGET ) ) {
			auto* html = node->asConstType<UIHTMLWidget>();
			if ( html->getUsedUserSelect() == CSSUserSelect::Contain ) {
				mContainRoot = const_cast<UIHTMLWidget*>( html );
				break;
			}
		}
	}
	mRawSelection = { anchor, focus };
	resolveAndProject();
}

void UITextSelectionController::selectAll() {
	collectOwners();
	Point first, last;
	for ( const auto& slice : mSlices ) {
		auto exclusions = selectionExclusions( slice.owner, mOwnerSubtreeNodes );
		if ( !hasSelectableInterval( slice.start, slice.end, exclusions ) )
			continue;
		if ( !first.isValid() )
			first = { slice.owner, firstSelectableOffset( slice.start, exclusions ) };
		last = { slice.owner, lastSelectableOffset( slice.end, exclusions ) };
	}
	if ( first.isValid() ) {
		mExplicitSelectAll = true;
		mContainRoot = nullptr;
		mRawSelection = { first, last };
		resolveAndProject();
	}
}

void UITextSelectionController::resolveAndProject() {
	if ( !mRawSelection.isValid() )
		return;
	// Resolve CSS constraints from the raw pointer positions each time. Expanded all/contain
	// endpoints must not replace the raw positions or dragging back would get stuck.
	Point anchor = mRawSelection.anchor;
	Point focus = mRawSelection.focus;
	bool forward = compare( anchor, focus ) <= 0;
	if ( !mExplicitSelectAll && mContainRoot ) {
		if ( !nodeWithin( selectionElementAtPoint( focus.owner, focus.offset ), mContainRoot ) )
			focus = forward ? lastPointIn( mContainRoot ) : firstPointIn( mContainRoot );
	} else if ( !mExplicitSelectAll ) {
		for ( const Node* node = selectionElementAtPoint( focus.owner, focus.offset ); node;
			  node = node->getParent() ) {
			if ( node->isType( UI_TYPE_HTML_WIDGET ) ) {
				auto* html = node->asConstType<UIHTMLWidget>();
				if ( html->getUsedUserSelect() == CSSUserSelect::Contain &&
					 !nodeWithin( selectionElementAtPoint( anchor.owner, anchor.offset ), html ) ) {
					focus = forward ? firstPointIn( html ) : lastPointIn( html );
					break;
				}
			}
		}
	}
	if ( !focus.isValid() )
		focus = anchor;
	Point start = forward ? anchor : focus;
	Point end = forward ? focus : anchor;
	for ( Point* point : { &start, &end } ) {
		for ( const Node* node = selectionElementAtPoint( point->owner, point->offset ); node;
			  node = node->getParent() ) {
			if ( !node->isType( UI_TYPE_HTML_WIDGET ) )
				continue;
			auto* html = node->asConstType<UIHTMLWidget>();
			if ( html->getUsedUserSelect() != CSSUserSelect::All )
				break;
			Point boundary = point == &start ? firstPointIn( html ) : lastPointIn( html );
			if ( boundary.isValid() )
				*point = boundary;
		}
	}
	if ( compare( start, end ) > 0 )
		std::swap( start, end );
	mResolvedSelection = { start, end };
	int first = sliceIndexOf( start );
	int last = sliceIndexOf( end );
	if ( first < 0 || last < 0 )
		return;
	SmallVector<UIRichText*, 8> nextSelected;
	SmallVector<TextSelectionRange, 8> projectedRanges;
	mSelectedSlices.clear();
	// Copy keeps document-order slices; painting receives one local range per owner plus
	// exclusions for nested children and user-select:none fragments.
	for ( int i = first; i <= last; ++i ) {
		const auto& slice = mSlices[i];
		auto* owner = slice.owner;
		Int64 from = i == first ? std::max( slice.start, start.offset ) : slice.start;
		Int64 to = i == last ? std::min( slice.end, end.offset ) : slice.end;
		auto exclusions = selectionExclusions( owner, mOwnerSubtreeNodes );
		if ( from < to && hasSelectableInterval( from, to, exclusions ) ) {
			mSelectedSlices.push_back( { owner, from, to } );
			auto found = std::find( nextSelected.begin(), nextSelected.end(), owner );
			if ( found == nextSelected.end() ) {
				nextSelected.push_back( owner );
				projectedRanges.push_back( { from, to } );
			} else {
				auto& range = projectedRanges[found - nextSelected.begin()];
				range.start = std::min( range.start, from );
				range.end = std::max( range.end, to );
			}
		}
	}
	for ( size_t i = 0; i < nextSelected.size(); ++i ) {
		auto* owner = nextSelected[i];
		auto range = projectedRanges[i];
		if ( owner->getTextSelectionRange() != std::make_pair( range.start, range.end ) )
			owner->setTextSelectionRange( range );
		owner->setTextSelectionExclusions( selectionExclusions( owner, mOwnerSubtreeNodes ) );
	}
	for ( auto* owner : mSelectedOwners ) {
		if ( std::find( nextSelected.begin(), nextSelected.end(), owner ) == nextSelected.end() )
			owner->setTextSelectionRange( { 0, 0 } );
	}
	mSelectedOwners = std::move( nextSelected );
}

UITextSelectionController::Point
UITextSelectionController::hitTest( const Vector2f& position ) const {
	if ( mOwners.empty() )
		return {};
	if ( mRoot ) {
		auto rootRect = mRoot->getScreenRect();
		if ( position.y < rootRect.Top )
			return { mOwners.front(), 0 };
		if ( position.y > rootRect.Bottom )
			return { mOwners.back(), mOwners.back()->getTextCharacterCount() };
	}
	UIRichText* nearest = nullptr;
	Float nearestDistance = std::numeric_limits<Float>::max();
	// Geometry chooses a boundary while the pointer crosses gaps. mSlices still determines
	// logical selection order; visual coordinates never reorder the document.
	for ( auto* owner : mOwners ) {
		auto rect = owner->getScreenRect();
		Float dx = std::max( { rect.Left - position.x, 0.f, position.x - rect.Right } );
		Float dy = std::max( { rect.Top - position.y, 0.f, position.y - rect.Bottom } );
		Float distance = dx * dx + dy * dy;
		if ( distance < nearestDistance ||
			 ( distance == nearestDistance && nearest && nodeWithin( owner, nearest ) ) ) {
			nearestDistance = distance;
			nearest = owner;
		}
	}
	if ( !nearest )
		return {};
	return { nearest, nearest->findTextCharacterFromWorldPosition( position ) };
}

bool UITextSelectionController::onMouseDown( UIRichText* source, const Vector2i& position,
											 Uint32 flags ) {
	// EventDispatcher sends MouseDown on every frame while the button remains pressed.
	if ( mDragging )
		return true;
	if ( !( flags & EE_BUTTON_LMASK ) || !mRoot || !mHost || !source->isTextSelectionEnabled() )
		return false;
	auto* dispatcher = source->getEventDispatcher();
	if ( !dispatcher )
		return false;
	Node* mouseDown = dispatcher->getMouseDownNode();
	if ( !mouseDown || !nodeWithin( mouseDown, mRoot ) )
		return false;
	const UIHTMLWidget* target = source;
	bool foundTarget = false;
	for ( const Node* node = mouseDown; node; node = node->getParent() ) {
		if ( node->isType( UI_TYPE_SCROLLBAR ) )
			return false;
		if ( !foundTarget && node->isType( UI_TYPE_HTML_WIDGET ) ) {
			target = node->asConstType<UIHTMLWidget>();
			foundTarget = true;
		}
		if ( node == mRoot )
			break;
	}
	collectOwners();
	Point point = hitTest( position.asFloat() );
	// The event source may be a containing rich text while the pointer is on a selectable
	// descendant inside user-select:none. Check the actual HTML target before rejecting it.
	if ( point.isValid() && target->getUsedUserSelect() == CSSUserSelect::None )
		target = selectionElementAtPoint( point.owner, point.offset );
	if ( !point.isValid() || !isDescendantOf( source, mRoot ) ||
		 target->getUsedUserSelect() == CSSUserSelect::None )
		return false;
	clear();
	collectOwners();
	mExplicitSelectAll = false;
	mRawSelection = { point, point };
	for ( const Node* node = target; node; node = node->getParent() ) {
		if ( node->isType( UI_TYPE_HTML_WIDGET ) ) {
			auto* html = node->asConstType<UIHTMLWidget>();
			if ( html->getUsedUserSelect() == CSSUserSelect::Contain ) {
				mContainRoot = const_cast<UIHTMLWidget*>( html );
				break;
			}
		}
	}
	for ( Node* node = mHost; node; node = node->getParent() ) {
		if ( node->isType( UI_TYPE_SCROLLVIEW ) ) {
			mScrollTarget = node->asType<UIScrollView>();
			break;
		}
	}
	mDragging = true;
	mDragStart = position;
	resolveAndProject();
	if ( mHost->getInput() )
		mHost->getInput()->captureMouse( true );
	return true;
}

bool UITextSelectionController::onMouseUp( const Vector2i& position, Uint32 flags ) {
	if ( !mDragging || !( flags & EE_BUTTON_LMASK ) )
		return false;
	Point point = hitTest( position.asFloat() );
	if ( point.isValid() ) {
		mRawSelection.focus = point;
		resolveAndProject();
	}
	mDragging = false;
	mSuppressClick = mSuppressClick || std::abs( position.x - mDragStart.x ) > 3 ||
					 std::abs( position.y - mDragStart.y ) > 3;
	if ( mHost && mHost->getInput() )
		mHost->getInput()->captureMouse( false );
	return true;
}

bool UITextSelectionController::onMouseDoubleClick( UIRichText* source, const Vector2i& position,
													Uint32 flags ) {
	if ( !( flags & EE_BUTTON_LMASK ) || !mRoot || !source->isTextSelectionEnabled() ||
		 !isDescendantOf( source, mRoot ) )
		return false;
	collectOwners();
	Point point = hitTest( position.asFloat() );
	if ( !point.isValid() ||
		 selectionElementAtPoint( point.owner, point.offset )->getUsedUserSelect() ==
			 CSSUserSelect::None )
		return false;
	auto* richText = point.owner->getRichTextPtr();
	// Read the full local text for word boundaries, then restore the visible selection state.
	auto savedSelection = richText->getSelection();
	auto savedExclusions = richText->getSelectionExclusions();
	richText->setSelectionExclusions( {} );
	richText->setSelection( { 0, point.owner->getTextCharacterCount() } );
	String content = richText->getSelectionString();
	richText->setSelection( savedSelection );
	richText->setSelectionExclusions( std::move( savedExclusions ) );
	if ( content.empty() )
		return false;
	size_t offset = std::min( static_cast<size_t>( point.offset ), content.size() - 1 );
	size_t start = offset;
	size_t end = offset + 1;
	while ( start > 0 && !content.isWordBoundary( start ) )
		--start;
	while ( end < content.size() && !content.isWordBoundary( end ) )
		++end;
	if ( mDragging && mHost && mHost->getInput() )
		mHost->getInput()->captureMouse( false );
	mDragging = false;
	setSelection( { point.owner, static_cast<Int64>( start ) },
				  { point.owner, static_cast<Int64>( end ) } );
	return true;
}

bool UITextSelectionController::onMouseUpMessage( const NodeMessage* message ) {
	if ( message->getMsg() != NodeMessage::MouseUp || !( message->getFlags() & EE_BUTTON_RMASK ) ||
		 !mHost || !mRoot ||
		 !( message->getSender() == mRoot || mRoot->isParentOf( message->getSender() ) ) )
		return false;
	if ( !mHost->getInput() || !mHost->getUISceneNode() )
		return false;
	Vector2i position =
		mHost->getInput()
			->getMousePosFromView( mHost->getUISceneNode()->getWindow()->getDefaultView() )
			.asInt();
	return showContextMenu( position, message->getFlags(), message->getSender() );
}

bool UITextSelectionController::showContextMenu( const Vector2i& position, Uint32 flags,
												 const Node* target ) {
	if ( mCurrentMenu || !mHost || !mHost->getUISceneNode() )
		return false;
	const bool selected = hasSelection();
	std::string linkHref;
	if ( !selected && target && nodeWithin( target, mRoot ) ) {
		if ( const auto* link = linkForNode( target, mRoot ); link && !link->getHref().empty() ) {
			linkHref = mLinkResolverCb ? mLinkResolverCb( link->getHref() )
									   : link->getUISceneNode()
											 ->solveRelativePath( URI( link->getHref() ) )
											 .toString();
		}
	}
	auto* menu = UIPopUpMenu::New();
	menu->setParent( mHost->getUISceneNode()->getRoot() );
	menu->addClass( "text-selection-menu" );
	auto* copy = menu->add( mHost->i18n( "uicodeeditor_copy", "Copy" ) );
	copy->setId( "copy" );
	copy->setEnabled( selected );
	if ( !linkHref.empty() ) {
		auto* copyLink = menu->add( mHost->i18n( "uihtml_copy_link", "Copy Link" ) );
		copyLink->setId( "copy-link" );
	}
	auto* selectAllItem = menu->add( mHost->i18n( "uicodeeditor_select_all", "Select All" ) );
	selectAllItem->setId( "select-all" );
	ContextMenuEvent event( mHost, menu, Event::OnCreateContextMenu, position, flags, target );
	mHost->sendEvent( &event );
	if ( menu->getCount() == 0 ) {
		menu->close();
		return false;
	}
	menu->setCloseOnHide( true );
	menu->setCloseSubMenusOnClose( true );
	mMenuItemConnection = menu->connect(
		Event::OnItemClicked, [this, menu, linkHref = std::move( linkHref )]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) ||
				 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
				return;
			const std::string& id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id == "copy" )
				copySelection();
			else if ( id == "copy-link" )
				mHost->getUISceneNode()->getWindow()->getClipboard()->setText( linkHref );
			else if ( id == "select-all" )
				selectAll();
			menu->hide();
		} );
	mMenuCloseConnection =
		menu->connect( Event::OnClose, [this]( const Event* ) { mCurrentMenu = nullptr; } );
	mCurrentMenu = menu;
	menu->showAtScreenPosition( position.asFloat() );
	return true;
}

bool UITextSelectionController::consumeSuppressedClick() {
	// The link receives MouseClick separately from mouse-up. Consume the drag marker once so
	// a text drag cannot activate the link, while the next ordinary click remains available.
	bool suppress = mSuppressClick;
	mSuppressClick = false;
	return suppress;
}

void UITextSelectionController::updateSelectionDrag() {
	if ( !mDragging || !mHost || !mHost->getInput() )
		return;
	auto* input = mHost->getInput();
	Vector2i mousePosition =
		input->getMousePosFromView( mHost->getUISceneNode()->getWindow()->getDefaultView() )
			.asInt();
	if ( std::abs( mousePosition.x - mDragStart.x ) > 3 ||
		 std::abs( mousePosition.y - mDragStart.y ) > 3 )
		mSuppressClick = true;
	if ( !input->isMouseLeftPressed() ) {
		onMouseUp( mousePosition, EE_BUTTON_LMASK );
		return;
	}
	auto position = mousePosition.asFloat();
	if ( mScrollTarget ) {
		auto rect = mScrollTarget->getContainer()->getScreenRect();
		const UIRichText* owner = mRawSelection.focus.owner;
		Float lineHeight =
			owner && owner->getFont()
				? static_cast<Float>( owner->getFont()->getFontHeight( owner->getFontSize() ) )
				: 0.f;
		if ( lineHeight <= 0.f && owner && !owner->getRichText().getLines().empty() )
			lineHeight = owner->getRichText().getLines().front().height;
		if ( lineHeight <= 0.f )
			lineHeight = PixelDensity::dpToPx( 12.f );
		const Float edge = std::min( lineHeight * 1.5f, rect.getHeight() / 4.f );
		Float distance = position.y < rect.Top + edge	   ? position.y - rect.Top - edge
						 : position.y > rect.Bottom - edge ? position.y - rect.Bottom + edge
														   : 0.f;
		if ( distance != 0.f && mScrollTarget->getVerticalScrollBar()->isEnabled() &&
			 mScrollTarget->getScrollView() && mScrollTarget->getScrollView()->isUINode() ) {
			auto* bar = mScrollTarget->getVerticalScrollBar();
			Float scrollRange = std::max(
				0.f, mScrollTarget->getScrollView()->asType<UINode>()->getPixelsSize().getHeight() -
						 mScrollTarget->getContainer()->getPixelsSize().getHeight() );
			if ( scrollRange > 0.f ) {
				// The scrollbar value is normalized; convert a font-scaled pixel step to its range.
				Float pixelStep =
					lineHeight *
					std::min( 0.35f + std::abs( distance ) /
										  std::max( edge, std::numeric_limits<Float>::epsilon() ),
							  1.5f );
				bar->setValue( bar->getValue() +
							   std::copysign( pixelStep, distance ) / scrollRange );
			}
		}
	}
	Point point = hitTest( position );
	if ( point.isValid() && ( point.owner != mRawSelection.focus.owner ||
							  point.offset != mRawSelection.focus.offset ) ) {
		mRawSelection.focus = point;
		resolveAndProject();
	}
}

String UITextSelectionController::getSelectionString() const {
	String result;
	UIRichText* previous = nullptr;
	auto containingCell = []( const UIRichText* owner ) -> const UIHTMLWidget* {
		for ( const Node* node = owner; node; node = node->getParent() ) {
			if ( node->isType( UI_TYPE_HTML_WIDGET ) ) {
				auto* html = node->asConstType<UIHTMLWidget>();
				if ( html->getDisplay() == CSSDisplay::TableCell )
					return html;
			}
		}
		return nullptr;
	};
	for ( const auto& slice : mSelectedSlices ) {
		auto* owner = slice.owner;
		String fragment = owner->getRichText().getSelectionString( { slice.start, slice.end } );
		if ( fragment.empty() )
			continue;
		// A parent can occur again after a nested owner. Only crossings between owners
		// introduce document separators; RichText already supplies local line breaks.
		if ( previous && previous != owner && !result.empty() &&
			 result[result.size() - 1] != '\n' && fragment[0] != '\n' ) {
			const UIHTMLWidget* previousCell = containingCell( previous );
			const UIHTMLWidget* nextCell = containingCell( owner );
			if ( previousCell && nextCell && previousCell != nextCell &&
				 previousCell->getParent() == nextCell->getParent() )
				result += '\t';
			else
				result += '\n';
		}
		result += fragment;
		previous = owner;
	}
	return result;
}

bool UITextSelectionController::copySelection() {
	if ( !hasSelection() || !mHost || !mHost->getUISceneNode() )
		return false;
	mHost->getUISceneNode()->getWindow()->getClipboard()->setText( getSelectionString().toUtf8() );
	return true;
}

bool UITextSelectionController::onKeyDown( const KeyEvent& event ) {
	if ( !mHost || event.getSanitizedMod() != KeyMod::getDefaultModifier() )
		return false;
	if ( event.getKeyCode() == KEY_A ) {
		selectAll();
		return true;
	}
	if ( event.getKeyCode() == KEY_C )
		return copySelection();
	return false;
}

void UITextSelectionController::setLinkResolverCb(
	std::function<std::string( const std::string& )> cb ) {
	mLinkResolverCb = std::move( cb );
}

}} // namespace EE::UI
