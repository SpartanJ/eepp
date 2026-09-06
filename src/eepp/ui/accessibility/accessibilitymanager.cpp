#include "accessibilitybackend.hpp"
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
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

bool AccessibilityManager::isValid( AccessibilityNodeRef ref ) const {
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
	}
	return info;
}

AccessibilityNodeRef AccessibilityManager::getParent( AccessibilityNodeRef ref ) {
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
	if ( auto widget = resolve( ref ) )
		return countSemanticChildren( widget );
	return 0;
}

AccessibilityNodeRef AccessibilityManager::getChild( AccessibilityNodeRef ref, size_t index ) {
	size_t currentIndex = 0;
	auto widget = resolve( ref );
	return widget ? getNodeRef( semanticChildAt( widget, index, currentIndex ) )
				  : AccessibilityNodeRef{};
}

AccessibilityNodeRef AccessibilityManager::hitTest( const Math::Vector2f& screenPosition ) {
	return getNodeRef( hitSemanticWidget( mScene->getRoot(), screenPosition ) );
}

AccessibilityNodeRef AccessibilityManager::getKeyboardFocusedNode() {
	return getNodeRef( focusedWidget( mScene->getRoot() ) );
}

bool AccessibilityManager::performAction( AccessibilityNodeRef ref,
										  const AccessibilityActionRequest& request ) {
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
	auto found = mWidgetIds.find( widget );
	if ( found == mWidgetIds.end() )
		return;
	AccessibilityNodeRef ref{ WidgetSource, found->second };
	mWidgets.erase( found->second );
	mWidgetIds.erase( found );
	notify( ref, AccessibilityEvent::Destroyed );
}

}} // namespace EE::UI
