#include "accessibilitybackend.hpp"
#include "accessibilitymodelviewsource.hpp"
#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/accessibility/accessibilitysource.hpp>
#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

namespace {

size_t countSemanticChildren( Node* parent ) {
	size_t count = 0;
	for ( Node* node = parent->getFirstChild(); node; node = node->getNextNode() ) {
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
		UIWidget* widget = node->isWidget() ? node->asType<UIWidget>() : nullptr;
		if ( widget && ( widget->isAccessibilityHidden() || !widget->hasVisibility() ||
						 !widget->getWorldBounds().contains( point ) ) )
			continue;
		if ( auto child = hitSemanticWidget( node, point ) )
			return child;
		if ( widget && widget->isAccessibilityElement() )
			return widget;
	}
	return nullptr;
}

UIWidget* focusedWidget( Node* parent ) {
	if ( parent->isWidget() && parent->hasFocus() )
		return parent->asType<UIWidget>();
	for ( Node* child = parent->getFirstChild(); child; child = child->getNextNode() ) {
		if ( auto focused = focusedWidget( child ) )
			return focused;
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
	if ( !widget || widget->getUISceneNode() != mScene )
		return {};
	auto found = mWidgetIds.find( widget );
	if ( found != mWidgetIds.end() )
		return { WidgetSource, found->second };
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
	if ( !widget || !widget->isType( UI_TYPE_ABSTRACTTABLEVIEW ) )
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
	AccessibilityNodeInfo info;
	if ( auto widget = resolve( ref ) ) {
		info.role = widget->getAccessibilityRole();
		info.name = widget->getAccessibilityName();
		info.description = widget->getAccessibilityDescription();
		if ( includeValue )
			info.value = widget->getAccessibilityValue();
		info.range = widget->getAccessibilityRange();
		info.text = AccessibilityWidgetResolver::getText( widget );
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
	auto widget = resolve( ref );
	if ( !widget || widget == mScene->getRoot() )
		return {};
	for ( Node* parent = widget->getParent(); parent; parent = parent->getParent() ) {
		if ( parent == mScene->getRoot() )
			return getRoot();
		if ( parent->isWidget() && parent->asType<UIWidget>()->isAccessibilityElement() )
			return getNodeRef( parent->asType<UIWidget>() );
	}
	return {};
}

size_t AccessibilityManager::getChildCount( AccessibilityNodeRef ref ) {
	return getChildren( ref ).size();
}

AccessibilityNodeRef AccessibilityManager::getChild( AccessibilityNodeRef ref, size_t index ) {
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

AccessibilityNodeRef AccessibilityManager::getKeyboardFocusedNode() {
	return getNodeRef( focusedWidget( mScene->getRoot() ) );
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

bool AccessibilityManager::hasActiveNativeClients() const {
	return mBackend && mBackend->hasActiveClients();
}

void AccessibilityManager::update() {
	if ( !mBackend )
		mBackend = createAccessibilityBackend( *this );
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
					if ( event == AccessibilityEvent::ModelChanged )
						found->second->reset();
					else
						found->second->invalidate();
				}
			}
		}
	}
	for ( const auto& pending : mPendingEvents ) {
		if ( pending.ref == ref && pending.related == related && pending.type == event )
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
	auto found = mWidgetIds.find( widget );
	if ( found == mWidgetIds.end() )
		return;
	AccessibilityNodeRef ref{ WidgetSource, found->second };
	auto parent = getParent( ref );
	if ( parent.isValid() ) {
		invalidateChildren();
		notify( parent, AccessibilityEvent::Destroyed, ref );
	}
}

void AccessibilityManager::onWidgetDelete( UIWidget* widget ) {
	auto source = mWidgetSources.find( widget );
	if ( source != mWidgetSources.end() ) {
		mSources.erase( source->second );
		mWidgetSources.erase( source );
	}
	auto found = mWidgetIds.find( widget );
	if ( found == mWidgetIds.end() )
		return;
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
	mWidgets.erase( found->second );
	mWidgetIds.erase( found );
	notify( ref, AccessibilityEvent::Destroyed );
}

void AccessibilityManager::invalidateChildren() {
	for ( auto& cache : mChildrenCache ) {
		cache.parent = {};
		cache.children.clear();
	}
}

}} // namespace EE::UI
