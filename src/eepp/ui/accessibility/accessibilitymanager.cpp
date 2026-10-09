#include "accessibilitybackend.hpp"
#include "accessibilitymodelviewsource.hpp"
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/accessibility/accessibilitysource.hpp>
#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>

#include <cstdlib>

namespace EE { namespace UI {

namespace {

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

UIWidget* hitSemanticWidget( Node* parent, const Math::Vector2f& point ) {
	for ( Node* node = parent->getLastChild(); node; node = node->getPrevNode() ) {
		if ( node->isDestroying() )
			continue;
		UIWidget* widget = node->isWidget() ? node->asType<UIWidget>() : nullptr;
		if ( widget && ( widget->isAccessibilityHidden() || !widget->hasVisibility() ||
						 !widget->getWorldBounds().contains( point ) ) )
			continue;
		// An accessible widget is a semantic boundary: getChildren() does not expose its
		// implementation children, so hit testing must not return one of those hidden descendants.
		if ( widget && widget->isAccessibilityElement() )
			return widget;
		if ( auto child = hitSemanticWidget( node, point ) )
			return child;
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

AccessibilityNodeInfo AccessibilityManager::getNodeInfo( AccessibilityNodeRef ref,
														 bool includeValue,
														 bool includeTextOffsets ) const {
	AccessibilityNodeInfo info;
	if ( auto widget = resolve( ref ) ) {
		info.role = widget->getAccessibilityRole();
		info.name = widget->getAccessibilityName();
		info.description = widget->getAccessibilityDescription();
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
	if ( auto* source = sourceFor( resolve( ref ) ) )
		return source->getRootChildCount();
	return getChildren( ref ).size();
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
	for ( const auto& child : getChildren( ref ) ) {
		if ( static_cast<Uint64>( getNodeInfo( child, false, false ).states ) &
			 static_cast<Uint64>( AccessibilityState::Selected ) )
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
		mScene->mAccessibilityState |= UISceneNode::AccessibilityClientActive;
}

void AccessibilityManager::update() {
	if ( !mBackend ) {
		const bool disabled =
			std::getenv( "EEPP_DISABLE_ACCESSIBILITY" ) ||
			( mScene && mScene->getAccessibilityPolicy() == AccessibilityPolicy::Disabled );
		mBackend =
			disabled ? createNullAccessibilityBackend() : createAccessibilityBackend( *this );
	}
	if ( mBackend ) {
		mBackend->update();
		mPendingEvents.clear();
	}
}

UISceneNode* AccessibilityManager::getSceneNode() const {
	return mScene;
}

void AccessibilityManager::notify( AccessibilityNodeRef ref, AccessibilityEvent event,
								   AccessibilityNodeRef related, Int32 index ) {
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
}

void AccessibilityManager::onWidgetParentChange( UIWidget* widget ) {
	if ( !widget || !widget->isAccessibilityElement() || widget->isAccessibilityHidden() )
		return;
	// Model sources, not recycled cell widgets, own the accessible children of a view.
	if ( isModelViewImplementationChild( widget ) )
		return;
	auto ref = getNodeRef( widget );
	auto parent = getParent( ref );
	if ( !parent.isValid() )
		return;
	invalidateChildren();
	Int32 index = -1;
	const size_t childCount = getChildCount( parent );
	for ( size_t i = 0; i < childCount; ++i ) {
		if ( getChild( parent, i ) == ref ) {
			index = static_cast<Int32>( i );
			break;
		}
	}
	notify( parent, AccessibilityEvent::Created, ref, index );
}

void AccessibilityManager::onWidgetRemovedFromParent( UIWidget* widget ) {
	if ( isModelViewImplementationChild( widget ) )
		return;
	auto found = mWidgetIds.find( widget );
	if ( found == mWidgetIds.end() )
		return;
	AccessibilityNodeRef ref{ WidgetSource, found->second };
	auto parent = getParent( ref );
	if ( parent.isValid() ) {
		invalidateChildren();
		Int32 index = -1;
		size_t currentIndex = 0;
		if ( auto* parentWidget = resolve( parent ) )
			semanticIndexOf( parentWidget, widget, currentIndex, index );
		notify( parent, AccessibilityEvent::Destroyed, ref, index );
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
	if ( mWidgetIds.empty() && mWidgetSources.empty() )
		return;
	for ( auto* parent = widget->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isDestroying() )
			return;
	}
	if ( isModelViewImplementationChild( widget ) ) {
		removeSubtreeIdentities( widget );
		return;
	}
	auto found = mWidgetIds.find( widget );
	if ( found == mWidgetIds.end() ) {
		// Native roots can exist without any client or queried descendants. Keep that dormant
		// case constant-time, including destruction of large unqueried widget hierarchies.
		if ( mWidgetSources.empty() && mWidgetIds.size() == 1 &&
			 mWidgetIds.find( mScene->getRoot() ) != mWidgetIds.end() )
			return;
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
	auto parent = getParent( ref );
	if ( parent.isValid() ) {
		invalidateChildren();
		Int32 index = -1;
		size_t currentIndex = 0;
		if ( auto parentWidget = resolve( parent ) )
			semanticIndexOf( parentWidget, widget, currentIndex, index );
		notify( parent, AccessibilityEvent::Destroyed, ref, index );
	}
	// getParent() and native destruction notifications can register more widgets, invalidating
	// dense-map iterators. Keep the copied identity and erase by key instead.
	removeSubtreeIdentities( widget );
}

void AccessibilityManager::onSubtreeRemoved( Scene::Node* node ) {
	if ( !node || !removeSubtreeIdentities( node ) )
		return;
	invalidateChildren();
	if ( node->isWidget() ) {
		auto parent = getWidgetParent( node->asType<UIWidget>() );
		if ( parent.isValid() )
			notify( parent, AccessibilityEvent::ChildrenChanged );
	}
}

bool AccessibilityManager::removeSubtreeIdentities( Scene::Node* node ) {
	if ( mWidgetIds.empty() && mWidgetSources.empty() )
		return false;
	// Invalidate descendants while the top-level widget is still alive. Later descendant
	// destructors find no identity and never walk back into a destroyed UIWidget parent.
	bool removed = false;
	for ( auto* child = node->getFirstChild(); child; child = child->getNextNode() )
		removed |= removeSubtreeIdentities( child );
	if ( !node->isWidget() )
		return removed;
	auto* widget = node->asType<UIWidget>();
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

void AccessibilityManager::invalidateChildren() {
	for ( auto& cache : mChildrenCache ) {
		cache.parent = {};
		cache.children.clear();
	}
}

}} // namespace EE::UI
