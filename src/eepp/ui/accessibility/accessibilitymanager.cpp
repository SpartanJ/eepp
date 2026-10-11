#include "accessibilitybackend.hpp"
#include "accessibilitymodelviewsource.hpp"
#include "accessibilitytextindex.hpp"
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/accessibility/accessibilitysource.hpp>
#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>

#include <algorithm>
#include <cstdlib>

namespace EE { namespace UI {

namespace {

/** Depth-first walk; `visitor` returns false to skip a node's children. */
template <typename NodeType, typename Visitor>
void visitNodes( NodeType* node, const Visitor& visitor ) {
	if ( !visitor( node ) )
		return;
	for ( auto* child = node->getFirstChild(); child; child = child->getNextNode() )
		visitNodes( static_cast<NodeType*>( child ), visitor );
}

bool isModelViewImplementationChild( const Node* node ) {
	return AccessibilityWidgetResolver::getOwningModelView( node ) != nullptr;
}

size_t countSemanticChildren( Node* parent ) {
	size_t count = 0;
	for ( Node* node = parent->getFirstChild(); node; node = node->getNextNode() ) {
		if ( node->isDestroying() )
			continue;
		if ( node->isWidget() ) {
			auto widget = node->asType<UIWidget>();
			if ( widget->isAccessibilityHidden() )
				continue;
			if ( widget->isAccessibilityElement() ) {
				++count;
				continue;
			}
		}
		count += countSemanticChildren( node );
	}
	return count;
}

void collectSemanticChildren( Node* parent, std::vector<AccessibilityNodeRef>& children,
							  AccessibilityManager& manager ) {
	for ( Node* node = parent->getFirstChild(); node; node = node->getNextNode() ) {
		if ( node->isDestroying() )
			continue;
		if ( node->isWidget() ) {
			auto widget = node->asType<UIWidget>();
			if ( widget->isAccessibilityHidden() )
				continue;
			if ( widget->isAccessibilityElement() ) {
				children.emplace_back( manager.getNodeRef( widget ) );
				continue;
			}
		}
		collectSemanticChildren( node, children, manager );
	}
}

bool semanticIndexOf( Node* parent, UIWidget* target, size_t& currentIndex, Int32& foundIndex ) {
	for ( Node* node = parent->getFirstChild(); node; node = node->getNextNode() ) {
		if ( node->isDestroying() )
			continue;
		if ( node == target ) {
			foundIndex = static_cast<Int32>( currentIndex );
			return true;
		}
		if ( node->isWidget() ) {
			auto widget = node->asType<UIWidget>();
			if ( widget->isAccessibilityHidden() )
				continue;
			if ( widget->isAccessibilityElement() ) {
				++currentIndex;
				continue;
			}
		}
		if ( semanticIndexOf( node, target, currentIndex, foundIndex ) )
			return true;
	}
	return false;
}

// Whether `node`, the last child of its parent, follows `earlier` in document order. Changing
// such a node cannot shift the semantic index of `earlier`.
bool isLastAfter( const Node* node, const Node* earlier ) {
	if ( node->getNextNode() )
		return false;
	const Node* parent = node->getParent();
	for ( const Node* ancestor = earlier; ancestor; ancestor = ancestor->getParent() ) {
		if ( ancestor == node )
			return false;
		if ( ancestor == parent )
			return true;
	}
	return false;
}

UIWidget* hitSemanticWidget( Node* parent, const Math::Vector2f& point ) {
	for ( Node* node = parent->getLastChild(); node; node = node->getPrevNode() ) {
		if ( node->isDestroying() )
			continue;
		UIWidget* widget = node->isWidget() ? node->asType<UIWidget>() : nullptr;
		if ( widget && ( widget->isAccessibilityHidden() || !widget->hasVisibility() ||
						 !widget->getWorldBounds().contains( point ) ) )
			continue;
		// Only leaves and model views hide their implementation children. Exposed containers
		// (groups, windows, etc.) still expose descendants and must hit-test those first.
		const bool exposed = widget && widget->isAccessibilityElement();
		if ( exposed &&
			 ( AccessibilityWidgetResolver::isLeafRole( widget->getAccessibilityRole() ) ||
			   widget->isType( UI_TYPE_ABSTRACTTABLEVIEW ) ) )
			return widget;
		if ( auto child = hitSemanticWidget( node, point ) )
			return child;
		if ( exposed )
			return widget;
	}
	return nullptr;
}

/** Returns the model view that is currently editing through this widget, if any. */
UIWidget* editedModelView( const UIWidget* widget ) {
	for ( auto* parent = widget->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isWidget() && parent->isType( UI_TYPE_ABSTRACTTABLEVIEW ) ) {
			auto* view = parent->asType<UIWidget>();
			return static_cast<Abstract::UIAbstractView*>( view )->getEditWidget() == widget
					   ? view
					   : nullptr;
		}
	}
	return nullptr;
}

} // namespace

AccessibilityManager::AccessibilityManager( UISceneNode* scene ) : mScene( scene ) {}

AccessibilityManager::~AccessibilityManager() {
	mBackend.reset();
}

AccessibilityNodeRef AccessibilityManager::getRoot() {
	return getNodeRef( mScene ? mScene->getRoot() : nullptr );
}

AccessibilityNodeRef AccessibilityManager::getNodeRef( UIWidget* widget ) {
	if ( !widget || widget->isDestroying() )
		return {};
	const auto* scene = widget->getUISceneNode();
	if ( !scene || ( scene != mScene && scene->getAccessibilityManager() != this ) )
		return {};
	auto found = mWidgetIds.find( widget );
	if ( found != mWidgetIds.end() )
		return { WidgetSource, found->second };
	// Close callbacks may query still-linked children of the subtree we just invalidated.
	// Never resurrect those identities, including descendants not yet in their own destructor.
	for ( auto* parent = widget->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isDestroying() )
			return {};
	}
	const Uint64 id = mNextId++;
	mWidgetIds.emplace( widget, id );
	mWidgets.emplace( id, widget );
	return { WidgetSource, id };
}

AccessibilityNodeRef AccessibilityManager::findNodeRef( UIWidget* widget ) const {
	auto found = mWidgetIds.find( widget );
	return found != mWidgetIds.end() ? AccessibilityNodeRef{ WidgetSource, found->second }
									 : AccessibilityNodeRef{};
}

UIWidget* AccessibilityManager::resolve( AccessibilityNodeRef ref ) const {
	if ( ref.source != WidgetSource )
		return nullptr;
	auto found = mWidgets.find( ref.id );
	return found != mWidgets.end() ? found->second : nullptr;
}

AccessibilitySource* AccessibilityManager::resolveSource( AccessibilityNodeRef ref ) const {
	if ( ref.source == WidgetSource )
		return nullptr;
	auto found = mSources.find( ref.source );
	return found != mSources.end() ? found->second.get() : nullptr;
}

AccessibilitySource* AccessibilityManager::sourceFor( UIWidget* widget ) {
	if ( !widget || widget->isDestroying() || !widget->isType( UI_TYPE_ABSTRACTTABLEVIEW ) )
		return nullptr;
	auto found = mWidgetSources.find( widget );
	if ( found != mWidgetSources.end() ) {
		auto source = mSources.find( found->second );
		return source != mSources.end() ? source->second.get() : nullptr;
	}
	auto sourceId = mNextSourceId++;
	auto source = createAccessibilityModelViewSource( *this, sourceId, widget );
	if ( !source )
		return nullptr;
	auto ptr = source.get();
	mSources.emplace( sourceId, std::move( source ) );
	mWidgetSources.emplace( widget, sourceId );
	return ptr;
}

bool AccessibilityManager::isValid( AccessibilityNodeRef ref ) const {
	if ( auto source = resolveSource( ref ) )
		return source->isValid( ref.id );
	return resolve( ref ) != nullptr;
}

AccessibilityNodeInfo AccessibilityManager::getNodeInfo( AccessibilityNodeRef ref,
														 bool includeValue ) const {
	return getNodeInfo( ref, includeValue, true );
}

AccessibilityRole AccessibilityManager::getNodeRole( AccessibilityNodeRef ref ) const {
	if ( auto* widget = resolve( ref ) )
		return widget->getAccessibilityRole();
	if ( auto* source = resolveSource( ref ) )
		return source->getRole( ref.id );
	return AccessibilityRole::None;
}

AccessibilityState AccessibilityManager::getNodeState( AccessibilityNodeRef ref ) const {
	if ( auto* widget = resolve( ref ) )
		return widget->getAccessibilityState();
	if ( auto* source = resolveSource( ref ) )
		return source->getState( ref.id );
	return AccessibilityState::None;
}

AccessibilityNodeInfo AccessibilityManager::getNodeInfo( AccessibilityNodeRef ref,
														 bool includeValue,
														 bool includeTextOffsets ) const {
	AccessibilityNodeInfo info;
	if ( auto widget = resolve( ref ) ) {
		info.role = widget->getAccessibilityRole();
		// A client now holds a name or description that depends on another widget.
		if ( !widget->getAccessibilityLabelledBy().empty() )
			mRelationTargets.insert( String::hash( widget->getAccessibilityLabelledBy() ) );
		if ( !widget->getAccessibilityDescribedBy().empty() )
			mRelationTargets.insert( String::hash( widget->getAccessibilityDescribedBy() ) );
		info.name = widget->getAccessibilityName();
		info.description = widget->getAccessibilityDescription();
		info.shortcut = AccessibilityWidgetResolver::getShortcut( widget );
		if ( includeValue )
			info.value = widget->getAccessibilityValue();
		info.range = widget->getAccessibilityRange();
		info.text = AccessibilityWidgetResolver::getText( widget, includeTextOffsets );
		info.states = widget->getAccessibilityState();
		info.actions = widget->getAccessibilityActions();
		info.bounds = widget->getWorldBounds();
		info.boundsValid = true;
	} else if ( auto source = resolveSource( ref ) )
		info = source->getInfo( ref.id );
	return info;
}

AccessibilityNodeRef AccessibilityManager::getParent( AccessibilityNodeRef ref ) {
	if ( auto source = resolveSource( ref ) )
		return source->getParent( ref.id );
	return getWidgetParent( resolve( ref ) );
}

AccessibilityNodeRef AccessibilityManager::getWidgetParent( UIWidget* widget ) {
	// Resolves the semantic parent without registering the widget itself, so callers handling a
	// widget no client has seen do not allocate an identity only to tear it down again.
	if ( !widget || widget == mScene->getRoot() )
		return {};
	// A cell editor hangs below the virtual row or cell it edits, not below its cell widget.
	if ( auto* source = sourceFor( editedModelView( widget ) ) ) {
		auto host = source->getEmbeddedWidgetParent( widget );
		if ( host.isValid() )
			return host;
	}
	for ( Node* parent = widget->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isDestroying() )
			return {};
		if ( parent == mScene->getRoot() )
			return getRoot();
		if ( parent->isWidget() && parent->asType<UIWidget>()->isAccessibilityElement() )
			return getNodeRef( parent->asType<UIWidget>() );
	}
	return {};
}

AccessibilityTextInfo AccessibilityManager::getTextInfo( AccessibilityNodeRef ref ) const {
	if ( auto* widget = resolve( ref ) )
		return AccessibilityWidgetResolver::getText( widget );
	return {};
}

AccessibilityTextIndex&
AccessibilityManager::textIndexFor( const UIWidget* widget,
									const Doc::TextDocument& document ) const {
	if ( !mTextIndexes )
		mTextIndexes = std::make_unique<AccessibilityTextIndexes>();
	return mTextIndexes->forWidget( widget, document );
}

Int32 AccessibilityManager::getTextLength( AccessibilityNodeRef ref ) const {
	auto* widget = resolve( ref );
	return widget ? AccessibilityWidgetResolver::getTextLength( widget ) : 0;
}

String AccessibilityManager::getTextRange( AccessibilityNodeRef ref, Int32 start,
										   Int32 end ) const {
	auto* widget = resolve( ref );
	return widget ? AccessibilityWidgetResolver::getTextRange( widget, start, end ) : String();
}

bool AccessibilityManager::getTextLineBounds( AccessibilityNodeRef ref, Int32 offset, Int32& start,
											  Int32& end ) const {
	auto* widget = resolve( ref );
	return widget && AccessibilityWidgetResolver::getTextLineBounds( widget, offset, start, end );
}

AccessibilityTextRevision AccessibilityManager::getTextRevision( AccessibilityNodeRef ref ) const {
	auto* widget = resolve( ref );
	return widget ? AccessibilityWidgetResolver::getTextRevision( widget )
				  : AccessibilityTextRevision{};
}

void AccessibilityManager::onTextChanged( UIWidget* widget,
										  const Doc::DocumentContentChange* change ) {
	if ( !mScene || !mScene->hasActiveAccessibilityClients() || !widget )
		return;
	if ( !mTextIndexes && ( !mBackend || !mBackend->supportsTextChanges() ) )
		return;
	// Index maintenance is independent of native edit support, suppression, rejection and the
	// frame budget. A queried document must remain current even when no edit event is sent.
	const auto* document = AccessibilityWidgetResolver::getTextDocument( widget );
	bool exactChange = true;
	if ( mTextIndexes ) {
		if ( document )
			exactChange = mTextIndexes->changed( widget, *document, change );
		else
			mTextIndexes->detach( widget );
	}
	mHasAmbiguousTextChanges |= !exactChange;
	if ( !mBackend || !mBackend->supportsTextChanges() || !widget->isAccessibilityElement() ||
		 AccessibilityWidgetResolver::getEventTarget( widget, AccessibilityEvent::ValueChanged ) !=
			 widget )
		return;
	if ( !document )
		return;
	const auto ref = getNodeRef( widget );
	if ( ref == mSuppressedTextRef )
		return;
	// Nested common-event callbacks can forward a later edit before the outer edit. Never send
	// out-of-order exact deltas; the existing whole-text path covers the final state this frame.
	if ( !exactChange )
		change = nullptr;
	auto counter = std::find_if( mTextChangeCounts.begin(), mTextChangeCounts.end(),
								 [&ref]( const auto& entry ) { return entry.first == ref; } );
	if ( counter == mTextChangeCounts.end() ) {
		mTextChangeCounts.emplace_back( ref, 0 );
		counter = mTextChangeCounts.end() - 1;
	}
	// Once this frame's edits collapsed into a whole-text change, that change covers the rest.
	if ( counter->second > MaxExactTextChangesPerFrame )
		return;
	const auto previousCount = counter->second;
	AccessibilityTextChange textChange;
	if ( change && ++counter->second <= MaxExactTextChangesPerFrame ) {
		textChange.offset =
			textIndexFor( widget, *document ).offset( change->range.normalized().start() );
		textChange.removed = document->getNotifiedRemovedText();
		textChange.inserted = change->text;
	} else {
		counter->second = MaxExactTextChangesPerFrame + 1;
	}
	// An ignored change (no client listening, for example) must not consume the budget, or
	// later edits would be dropped while nothing covers them.
	if ( !mBackend->onTextChanged( ref, textChange ) )
		counter->second = previousCount;
}

void AccessibilityManager::setSuppressedTextChanges( AccessibilityNodeRef ref ) {
	mSuppressedTextRef = ref;
}

Int32 AccessibilityManager::getIndexInParent( AccessibilityNodeRef ref ) {
	if ( auto* source = resolveSource( ref ) ) {
		const auto index = source->getIndexInParent( ref.id );
		if ( index != AccessibilitySource::UnknownIndex )
			return index;
	}
	auto parent = getParent( ref );
	if ( !parent.isValid() )
		return -1;
	auto* widget = resolve( ref );
	auto* parentWidget = widget ? resolve( parent ) : nullptr;
	if ( parentWidget && !sourceFor( parentWidget ) ) {
		Int32 index = -1;
		size_t current = 0;
		semanticIndexOf( parentWidget, widget, current, index );
		return index;
	}
	const auto count = getChildCount( parent );
	for ( size_t index = 0; index < count; ++index ) {
		if ( getChild( parent, index ) == ref )
			return static_cast<Int32>( index );
	}
	return -1;
}

size_t AccessibilityManager::getChildCount( AccessibilityNodeRef ref ) {
	if ( auto* source = resolveSource( ref ) )
		return source->getChildCount( ref.id );
	auto* widget = resolve( ref );
	if ( auto* source = sourceFor( widget ) )
		return source->getRootChildCount();
	if ( !widget )
		return 0;
	if ( hasActiveNativeClients() ) {
		for ( Uint8 i = 0; i < 2; ++i ) {
			if ( mChildrenCache[i].parent == ref ) {
				mMostRecentlyUsedChildrenCache = i;
				return mChildrenCache[i].children.size();
			}
		}
	}
	if ( AccessibilityWidgetResolver::isLeafRole( widget->getAccessibilityRole() ) )
		return 0;
	// Count queries must not assign identities to children a client has not requested.
	return countSemanticChildren( widget );
}

AccessibilityNodeRef AccessibilityManager::getChild( AccessibilityNodeRef ref, size_t index ) {
	if ( auto* source = resolveSource( ref ) ) {
		return index < source->getChildCount( ref.id ) ? source->getChild( ref.id, index )
													   : AccessibilityNodeRef{};
	}
	if ( auto* source = sourceFor( resolve( ref ) ) )
		return source->getRootChild( index );
	const auto& children = getChildren( ref );
	return index < children.size() ? children[index] : AccessibilityNodeRef{};
}

const std::vector<AccessibilityNodeRef>&
AccessibilityManager::getChildren( AccessibilityNodeRef ref ) {
	const bool cacheResult = hasActiveNativeClients();
	if ( cacheResult ) {
		for ( Uint8 i = 0; i < 2; ++i ) {
			if ( ref == mChildrenCache[i].parent ) {
				mMostRecentlyUsedChildrenCache = i;
				return mChildrenCache[i].children;
			}
		}
	}
	Uint8 cacheIndex = cacheResult ? 1 - mMostRecentlyUsedChildrenCache : 0;
	auto& cache = mChildrenCache[cacheIndex];
	cache.children.clear();
	cache.parent = cacheResult ? ref : AccessibilityNodeRef{};
	if ( cacheResult )
		mMostRecentlyUsedChildrenCache = cacheIndex;
	if ( auto source = resolveSource( ref ) ) {
		const size_t childCount = source->getChildCount( ref.id );
		cache.children.reserve( childCount );
		for ( size_t i = 0; i < childCount; ++i )
			cache.children.emplace_back( source->getChild( ref.id, i ) );
		return cache.children;
	}
	auto widget = resolve( ref );
	if ( !widget )
		return cache.children;
	if ( auto source = sourceFor( widget ) ) {
		const size_t childCount = source->getRootChildCount();
		cache.children.reserve( childCount );
		for ( size_t i = 0; i < childCount; ++i )
			cache.children.emplace_back( source->getRootChild( i ) );
		return cache.children;
	}
	if ( AccessibilityWidgetResolver::isLeafRole( widget->getAccessibilityRole() ) )
		return cache.children;
	cache.children.reserve( countSemanticChildren( widget ) );
	collectSemanticChildren( widget, cache.children, *this );
	return cache.children;
}

AccessibilityNodeRef AccessibilityManager::hitTest( const Math::Vector2f& screenPosition ) {
	auto widget = hitSemanticWidget( mScene->getRoot(), screenPosition );
	if ( auto source = sourceFor( widget ) )
		return source->hitTest( screenPosition );
	return getNodeRef( widget );
}

std::vector<AccessibilityNodeRef>
AccessibilityManager::getSelectedChildren( AccessibilityNodeRef ref ) {
	std::vector<AccessibilityNodeRef> selected;
	auto* source = sourceFor( resolve( ref ) );
	if ( source && source->getSelectedChildren( selected ) )
		return selected;
	// Only the state is needed: building each child's node info would copy its name and text,
	// and record its relation targets as if a client had read them.
	for ( const auto& child : getChildren( ref ) ) {
		if ( hasState( getNodeState( child ), AccessibilityState::Selected ) )
			selected.emplace_back( child );
	}
	return selected;
}

AccessibilityNodeRef AccessibilityManager::getKeyboardFocusedNode() {
	auto* dispatcher = mScene ? mScene->getEventDispatcher() : nullptr;
	Node* focused = dispatcher ? dispatcher->getFocusNode() : nullptr;
	return focused ? getNodeRef(
						 AccessibilityWidgetResolver::getFocusOwner( focused, mScene->getRoot() ) )
				   : AccessibilityNodeRef{};
}

bool AccessibilityManager::performAction( AccessibilityNodeRef ref,
										  const AccessibilityActionRequest& request ) {
	ScopedDispatch dispatch( *this );
	if ( auto source = resolveSource( ref ) )
		return source->performAction( ref.id, request );
	auto widget = resolve( ref );
	return widget && widget->isEnabled() && widget->performAccessibilityAction( request );
}

bool AccessibilityManager::isBackendAvailable() const {
	return mBackend && mBackend->isAvailable();
}

bool AccessibilityManager::isBackendInitializationComplete() const {
	return mBackend && mBackend->isInitializationComplete();
}

bool AccessibilityManager::hasActiveNativeClients() const {
	return mBackend && mBackend->hasActiveClients();
}

void AccessibilityManager::onNativeClientObserved() {
	if ( mScene )
		mScene->setAccessibilityClientActive( true );
}

void AccessibilityManager::onClientsDisconnected() {
	mTextIndexes.reset();
	// Queried rows hold persistent model registrations, which the model updates on every row
	// insert, delete and move. Without a client nothing reads them.
	for ( auto& source : mSources ) {
		if ( mBackend )
			mBackend->onSourceInvalidated( source.first );
		source.second->reset();
	}
	mRelationTargets.clear();
	invalidateChildren();
}

void AccessibilityManager::update() {
	// User callbacks can re-enter the scene loop. Never recursively drain native requests or
	// mutate a backend's event queue while it is already dispatching one of those callbacks.
	if ( mDispatchDepth != 0 )
		return;
	ScopedDispatch dispatch( *this );
	if ( !mBackend ) {
		const bool disabled =
			std::getenv( "EEPP_DISABLE_ACCESSIBILITY" ) ||
			( mScene && mScene->getAccessibilityPolicy() == AccessibilityPolicy::Disabled );
		mBackend =
			disabled ? createNullAccessibilityBackend() : createAccessibilityBackend( *this );
	}
	mTextChangeCounts.clear();
	if ( mHasAmbiguousTextChanges ) {
		if ( mTextIndexes )
			mTextIndexes->clearEditAmbiguity();
		mHasAmbiguousTextChanges = false;
	}
	if ( mBackend ) {
		mBackend->update();
		mPendingEvents.clear();
		// Announcements follow the frame's events, so focus and state reach the client first.
		for ( const auto& announcement : mPendingAnnouncements ) {
			// A live region may have been hidden or removed since its change was queued.
			if ( announcement.source.isValid() ) {
				const auto* source = resolve( announcement.source );
				if ( !source || !source->hasVisibility() ||
					 AccessibilityWidgetResolver::isHiddenFromAccessibility( source ) )
					continue;
			}
			mBackend->announce( announcement.message, announcement.priority );
		}
	}
	mPendingAnnouncements.clear();
}

void AccessibilityManager::setBackend( std::unique_ptr<AccessibilityBackend> backend ) {
	mBackend = std::move( backend );
}

UISceneNode* AccessibilityManager::getSceneNode() const {
	return mScene;
}

void AccessibilityManager::notify( AccessibilityNodeRef ref, AccessibilityEvent event,
								   AccessibilityNodeRef related, Int32 index ) {
	ScopedDispatch dispatch( *this );
	if ( !isValid( ref ) && event != AccessibilityEvent::Destroyed )
		return;
	if ( event == AccessibilityEvent::ChildrenChanged ||
		 event == AccessibilityEvent::ModelChanged || event == AccessibilityEvent::Created ||
		 event == AccessibilityEvent::Destroyed ) {
		invalidateChildren();
	}
	if ( event == AccessibilityEvent::ChildrenChanged ||
		 event == AccessibilityEvent::ModelChanged ) {
		if ( auto widget = resolve( ref ) ) {
			auto source = mWidgetSources.find( widget );
			if ( source != mWidgetSources.end() ) {
				auto found = mSources.find( source->second );
				if ( found != mSources.end() ) {
					if ( event == AccessibilityEvent::ModelChanged ) {
						if ( mBackend )
							mBackend->onSourceInvalidated( source->second );
						found->second->reset();
					} else {
						found->second->invalidate();
						if ( mBackend )
							mBackend->onSourceChanged( source->second );
					}
				}
			}
		}
	}
	// Without a client nothing can be delivered: queueing would only grow the vector until the
	// next update() discards it. Identity invalidation (Destroyed without a related node) still
	// reaches the backend, which detaches native wrappers and per-node state on it.
	if ( !hasActiveClients() ) {
		if ( mBackend && event == AccessibilityEvent::Destroyed && !related.isValid() )
			mBackend->onEvent( { ref, related, index, event } );
		return;
	}
	// Only adjacent query-again hints may be coalesced. They carry no historical state; source
	// invalidation above must still run for every mutation. State transitions and structural
	// sequences such as add/remove/add must always retain their original dispatch order.
	if ( ( event == AccessibilityEvent::ModelChanged ||
		   event == AccessibilityEvent::ChildrenChanged ||
		   event == AccessibilityEvent::BoundsChanged ) &&
		 !mPendingEvents.empty() ) {
		const auto& previous = mPendingEvents.back();
		if ( previous.ref == ref && previous.related == related && previous.type == event )
			return;
	}
	mPendingEvents.push_back( { ref, related, index, event } );
	if ( mBackend )
		mBackend->onEvent( mPendingEvents.back() );
}

const std::vector<AccessibilityPendingEvent>& AccessibilityManager::getPendingEvents() const {
	return mPendingEvents;
}

void AccessibilityManager::clearPendingEvents() {
	mPendingEvents.clear();
	mPendingAnnouncements.clear();
}

void AccessibilityManager::announce( const String& message, AccessibilityLive priority ) {
	if ( message.empty() || priority == AccessibilityLive::Off || !mScene ||
		 !mScene->hasActiveAccessibilityClients() )
		return;
	mPendingAnnouncements.push_back( { message, priority, {} } );
}

void AccessibilityManager::onLiveRegionChanged( UIWidget* widget, AccessibilityLive priority ) {
	if ( !widget || priority == AccessibilityLive::Off || !widget->hasVisibility() )
		return;
	String message = widget->getAccessibilityValue();
	if ( message.empty() )
		message = widget->getAccessibilityName();
	const auto ref = getNodeRef( widget );
	auto pending = std::find_if( mPendingAnnouncements.begin(), mPendingAnnouncements.end(),
								 [&ref]( const auto& item ) { return item.source == ref; } );
	if ( pending != mPendingAnnouncements.end() ) {
		if ( message.empty() ) {
			mPendingAnnouncements.erase( pending );
		} else {
			pending->message = std::move( message );
			pending->priority = priority;
		}
	} else if ( !message.empty() ) {
		mPendingAnnouncements.push_back( { std::move( message ), priority, ref } );
	}
}

const std::vector<AccessibilityAnnouncement>&
AccessibilityManager::getPendingAnnouncements() const {
	return mPendingAnnouncements;
}

bool AccessibilityManager::isRelationTarget( String::HashType idHash ) const {
	return !mRelationTargets.empty() && mScene && mScene->hasActiveAccessibilityClients() &&
		   mRelationTargets.find( idHash ) != mRelationTargets.end();
}

void AccessibilityManager::onRelationTargetChanged( const std::string& id,
													const Scene::Node* excluded ) {
	if ( id.empty() || !isRelationTarget( String::hash( id ) ) )
		return;
	auto notifyDependent = [this]( UIWidget* widget, AccessibilityEvent event ) {
		if ( widget->isAccessibilityHidden() )
			return;
		auto* target = AccessibilityWidgetResolver::getEventTarget( widget, event );
		if ( target && target->isAccessibilityElement() )
			notify( getNodeRef( target ), event );
	};
	// Relations are rare and resolved by id from the scene root, so scan the scene.
	if ( mScene->getRoot() )
		visitNodes( static_cast<Scene::Node*>( mScene->getRoot() ), [&]( Scene::Node* node ) {
			if ( node == excluded )
				return false;
			if ( node->isWidget() ) {
				auto* widget = node->asType<UIWidget>();
				if ( widget->getAccessibilityLabelledBy() == id )
					notifyDependent( widget, AccessibilityEvent::NameChanged );
				if ( widget->getAccessibilityDescribedBy() == id )
					notifyDependent( widget, AccessibilityEvent::DescriptionChanged );
			}
			return true;
		} );
}

void AccessibilityManager::onRelationSubtreeChanged( const Scene::Node* subtree, bool removed ) {
	if ( !subtree || mRelationTargets.empty() || !mScene ||
		 !mScene->hasActiveAccessibilityClients() )
		return;
	std::vector<std::string> ids;
	visitNodes( subtree, [&]( const Scene::Node* node ) {
		if ( !node->getId().empty() &&
			 mRelationTargets.find( node->getIdHash() ) != mRelationTargets.end() )
			ids.push_back( node->getId() );
		return true;
	} );
	for ( const auto& id : ids )
		onRelationTargetChanged( id, removed ? subtree : nullptr );
}

void AccessibilityManager::onWidgetParentChange( UIWidget* widget ) {
	if ( !widget )
		return;
	// An arriving label renames the controls it labels, whatever its own exposure.
	onRelationSubtreeChanged( widget, false );
	onSemanticSubtreeChange( widget, false );
}

void AccessibilityManager::onWidgetRemovedFromParent( UIWidget* widget ) {
	onRelationSubtreeChanged( widget, true );
	onSemanticSubtreeChange( widget, true );
}

void AccessibilityManager::onSemanticSubtreeChange( UIWidget* widget, bool removed ) {
	if ( !widget || !hasActiveClients() || isModelViewImplementationChild( widget ) ||
		 AccessibilityWidgetResolver::getLeafOwner( widget ) ||
		 AccessibilityWidgetResolver::isHiddenFromAccessibility( widget ) )
		return;
	// The previous addition's index survives an append right after it, and any change under
	// another semantic parent: widgets are created under the scene root, then reparented.
	auto* lastAdded = mLastAddedWidget;
	const auto lastAddedIndex = mLastAddedIndex;
	const auto lastAddedParent = mLastAddedParent;
	invalidateChildren();
	// Moving a layout does not move its descendants individually. Report its semantic roots,
	// not the unexposed layout, and retain their identities when moving within this manager.
	Int32 index = -1;
	bool first = true;
	AccessibilityNodeRef parent;
	visitNodes( widget, [&]( Node* node ) {
		if ( node->isDestroying() )
			return false;
		if ( !node->isWidget() )
			return true;
		auto* child = node->asType<UIWidget>();
		if ( child->isAccessibilityHidden() )
			return false;
		if ( !child->isAccessibilityElement() )
			return true;
		if ( !first && index >= 0 )
			++index;
		AccessibilityNodeRef ref;
		if ( removed ) {
			// Constructors temporarily attach their implementation widgets to the scene root.
			// Do not register/announce such unseen nodes just because they are then reparented.
			const auto found = mWidgetIds.find( child );
			if ( found == mWidgetIds.end() )
				return false;
			ref = { WidgetSource, found->second };
		} else {
			ref = getNodeRef( child );
		}
		if ( first ) {
			// Resolve the parent lazily: an empty transparent layout needs no new identity.
			parent = getWidgetParent( widget );
			if ( !removed && child == widget && lastAdded && widget->getPrevNode() == lastAdded &&
				 parent == lastAddedParent ) {
				// Appending right after the previous addition, with no change to this parent
				// since (it would have cleared the hint): the index follows the previous one.
				index = lastAddedIndex + 1;
			} else {
				size_t currentIndex = 0;
				if ( auto* parentWidget = resolve( parent ) )
					semanticIndexOf( parentWidget, child, currentIndex, index );
			}
			first = false;
		}
		// The semantic roots of one subtree are contiguous siblings. Scan the parent once,
		// not once per descendant, when a layout with many controls is moved.
		if ( parent.isValid() ) {
			notify( parent, removed ? AccessibilityEvent::Destroyed : AccessibilityEvent::Created,
					ref, index );
		}
		return false;
	} );
	// Only a widget reported as its own single root can be followed by a sibling at the next
	// index. Otherwise keep the previous hint when this change cannot have shifted it: under
	// another parent, or after it (a new widget leaving its temporary place under the root).
	// The remembered widget is alive while it holds an identity.
	if ( !removed && parent.isValid() && index >= 0 && widget->isAccessibilityElement() ) {
		mLastAddedWidget = widget;
		mLastAddedIndex = index;
		mLastAddedParent = parent;
	} else if ( lastAdded && mWidgetIds.find( lastAdded ) != mWidgetIds.end() ) {
		if ( first )
			parent = getWidgetParent( widget );
		if ( parent != lastAddedParent || isLastAfter( widget, lastAdded ) ) {
			mLastAddedWidget = lastAdded;
			mLastAddedIndex = lastAddedIndex;
			mLastAddedParent = lastAddedParent;
		}
	}
}

bool AccessibilityManager::onWidgetAccessibilitySourceDelete( UIWidget* widget ) {
	auto source = mWidgetSources.find( widget );
	if ( source == mWidgetSources.end() )
		return false;
	const auto sourceId = source->second;
	if ( mBackend )
		mBackend->onSourceInvalidated( sourceId );
	mSources.erase( sourceId );
	mWidgetSources.erase( widget );
	invalidateChildren();
	return true;
}

void AccessibilityManager::onWidgetDelete( UIWidget* widget ) {
	if ( mTextIndexes )
		mTextIndexes->detach( widget );
	if ( mWidgetIds.empty() && mWidgetSources.empty() &&
		 ( !mTextIndexes || mTextIndexes->size() == 0 ) )
		return;
	// Native roots can exist without any client or queried descendants. Keep that dormant case
	// constant-time, before any ancestor walk: the scene root is never inside another widget's
	// subtree, so a deleted non-root widget holds nothing to invalidate. It may still be a
	// relation target of the root a client has read, which must be renamed.
	if ( mWidgetSources.empty() && mWidgetIds.size() == 1 && widget != mScene->getRoot() &&
		 mWidgetIds.find( mScene->getRoot() ) != mWidgetIds.end() &&
		 ( mRelationTargets.empty() || !hasActiveClients() ) )
		return;
	for ( auto* parent = widget->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isDestroying() )
			return;
	}
	// The subtree is still intact here; its children are destroyed after this widget.
	onRelationSubtreeChanged( widget, true );
	if ( isModelViewImplementationChild( widget ) ) {
		removeSubtreeIdentities( widget );
		return;
	}
	if ( hasActiveClients() && AccessibilityWidgetResolver::isHiddenFromAccessibility( widget ) ) {
		// A client may retain identities queried before the subtree was hidden. Evict those,
		// but do not announce a child removal from a parent that no longer exposes the child.
		removeSubtreeIdentities( widget );
		invalidateChildren();
		return;
	}
	auto found = mWidgetIds.find( widget );
	if ( found == mWidgetIds.end() ) {
		// Only a subtree holding identities a client may have cached changes the tree it knows.
		if ( !removeSubtreeIdentities( widget ) )
			return;
		invalidateChildren();
		if ( hasActiveNativeClients() ) {
			auto parent = getWidgetParent( widget );
			if ( parent.isValid() )
				notify( parent, AccessibilityEvent::ChildrenChanged );
		}
		return;
	}
	AccessibilityNodeRef ref{ WidgetSource, found->second };
	if ( hasActiveClients() ) {
		auto parent = getParent( ref );
		if ( parent.isValid() ) {
			invalidateChildren();
			Int32 index = -1;
			size_t currentIndex = 0;
			if ( auto parentWidget = resolve( parent ) )
				semanticIndexOf( parentWidget, widget, currentIndex, index );
			notify( parent, AccessibilityEvent::Destroyed, ref, index );
		}
	} else {
		// No client to tell: do not register the parent or compute its index just to announce
		// this removal. Cached children still drop the widget.
		invalidateChildren();
	}
	// getParent() and native destruction notifications can register more widgets, invalidating
	// dense-map iterators. Keep the copied identity and erase by key instead.
	removeSubtreeIdentities( widget );
}

void AccessibilityManager::onSubtreeRemoved( Scene::Node* node ) {
	if ( !node || !removeSubtreeIdentities( node ) )
		return;
	invalidateChildren();
	if ( node->isWidget() && hasActiveClients() &&
		 !AccessibilityWidgetResolver::isHiddenFromAccessibility( node->asType<UIWidget>() ) ) {
		auto parent = getWidgetParent( node->asType<UIWidget>() );
		if ( parent.isValid() )
			notify( parent, AccessibilityEvent::ChildrenChanged );
	}
}

bool AccessibilityManager::removeSubtreeIdentities( Scene::Node* node ) {
	if ( mWidgetIds.empty() && mWidgetSources.empty() &&
		 ( !mTextIndexes || mTextIndexes->size() == 0 ) )
		return false;
	// Invalidate descendants while the top-level widget is still alive. Later descendant
	// destructors find no identity and never walk back into a destroyed UIWidget parent.
	bool removed = false;
	for ( auto* child = node->getFirstChild(); child; child = child->getNextNode() )
		removed |= removeSubtreeIdentities( child );
	if ( !node->isWidget() )
		return removed;
	auto* widget = node->asType<UIWidget>();
	if ( mTextIndexes )
		mTextIndexes->detach( widget );
	removed |= onWidgetAccessibilitySourceDelete( widget );
	auto found = mWidgetIds.find( widget );
	if ( found == mWidgetIds.end() )
		return removed;
	const AccessibilityNodeRef ref{ WidgetSource, found->second };
	mWidgetIds.erase( widget );
	mWidgets.erase( ref.id );
	notify( ref, AccessibilityEvent::Destroyed );
	return true;
}

bool AccessibilityManager::hasActiveClients() const {
	return mScene && mScene->hasActiveAccessibilityClients();
}

void AccessibilityManager::invalidateChildren() {
	mLastAddedWidget = nullptr;
	for ( auto& cache : mChildrenCache ) {
		cache.parent = {};
		cache.children.clear();
	}
}

}} // namespace EE::UI
