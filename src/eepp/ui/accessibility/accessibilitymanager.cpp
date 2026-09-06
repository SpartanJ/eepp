#include "accessibilitybackend.hpp"
#include "accessibilitymodelviewsource.hpp"
#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/accessibility/accessibilitysource.hpp>
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

UIWidget* semanticChildAt( Node* parent, size_t wantedIndex, size_t& currentIndex ) {
	for ( Node* node = parent->getFirstChild(); node; node = node->getNextNode() ) {
		if ( node->isWidget() ) {
			auto widget = node->asType<UIWidget>();
			if ( widget->isAccessibilityHidden() )
				continue;
			if ( widget->isAccessibilityElement() ) {
				if ( currentIndex++ == wantedIndex )
					return widget;
				continue;
			}
		}
		if ( auto found = semanticChildAt( node, wantedIndex, currentIndex ) )
			return found;
	}
	return nullptr;
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

AccessibilityManager::AccessibilityManager( UISceneNode* scene ) : mScene( scene ) {
	mBackend = createAccessibilityBackend( *this );
}

AccessibilityManager::~AccessibilityManager() = default;

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

AccessibilityNodeInfo AccessibilityManager::getNodeInfo( AccessibilityNodeRef ref ) const {
	AccessibilityNodeInfo info;
	if ( auto widget = resolve( ref ) ) {
		info.role = widget->getAccessibilityRole();
		info.name = widget->getAccessibilityName();
		info.description = widget->getAccessibilityDescription();
		info.value = widget->getAccessibilityValue();
		info.range = widget->getAccessibilityRange();
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
	if ( auto source = resolveSource( ref ) )
		return source->getChildCount( ref.id );
	if ( auto widget = resolve( ref ) ) {
		if ( auto source = sourceFor( widget ) )
			return source->getRootChildCount();
		return countSemanticChildren( widget );
	}
	return 0;
}

AccessibilityNodeRef AccessibilityManager::getChild( AccessibilityNodeRef ref, size_t index ) {
	if ( auto source = resolveSource( ref ) )
		return source->getChild( ref.id, index );
	size_t currentIndex = 0;
	auto widget = resolve( ref );
	if ( auto source = sourceFor( widget ) )
		return source->getRootChild( index );
	return widget ? getNodeRef( semanticChildAt( widget, index, currentIndex ) )
				  : AccessibilityNodeRef{};
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
	if ( mBackend ) {
		mBackend->update();
		mPendingEvents.clear();
	}
}

UISceneNode* AccessibilityManager::getSceneNode() const {
	return mScene;
}

void AccessibilityManager::notify( AccessibilityNodeRef ref, AccessibilityEvent event ) {
	if ( !isValid( ref ) && event != AccessibilityEvent::Destroyed )
		return;
	for ( const auto& pending : mPendingEvents ) {
		if ( pending.ref == ref && pending.type == event )
			return;
	}
	mPendingEvents.push_back( { ref, event } );
	if ( mBackend )
		mBackend->onEvent( mPendingEvents.back() );
}

const std::vector<AccessibilityPendingEvent>& AccessibilityManager::getPendingEvents() const {
	return mPendingEvents;
}

void AccessibilityManager::clearPendingEvents() {
	mPendingEvents.clear();
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
	mWidgets.erase( found->second );
	mWidgetIds.erase( found );
	notify( ref, AccessibilityEvent::Destroyed );
}

}} // namespace EE::UI
