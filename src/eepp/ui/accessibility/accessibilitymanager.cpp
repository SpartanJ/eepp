#include "accessibilitybackend.hpp"
#include "accessibilitymodelviewsource.hpp"
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
	if ( !mBackend || !mScene || !mScene->hasActiveAccessibilityClients() || !widget ||
		 !widget->isAccessibilityElement() ||
		 AccessibilityWidgetResolver::getEventTarget( widget, AccessibilityEvent::ValueChanged ) !=
			 widget )
		return;
	const auto* document = AccessibilityWidgetResolver::getTextDocument( widget );
	if ( !document )
		return;
	const auto ref = getNodeRef( widget );
	if ( ref == mSuppressedTextRef )
		return;
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
		textChange.offset = AccessibilityWidgetResolver::getTextOffset(
			*document, change->range.normalized().start() );
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
		mScene->setAccessibilityClientActive( true );
}

void AccessibilityManager::onClientsDisconnected() {
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
	if ( !mBackend ) {
		const bool disabled =
			std::getenv( "EEPP_DISABLE_ACCESSIBILITY" ) ||
			( mScene && mScene->getAccessibilityPolicy() == AccessibilityPolicy::Disabled );
		mBackend =
			disabled ? createNullAccessibilityBackend() : createAccessibilityBackend( *this );
	}
	mTextChangeCounts.clear();
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
	if ( !widget->isAccessibilityElement() || widget->isAccessibilityHidden() )
		return;
	// Children of a leaf control (a button's text view) are not part of the tree.
	if ( AccessibilityWidgetResolver::getLeafOwner( widget ) )
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
	onRelationSubtreeChanged( widget, true );
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
	if ( node->isWidget() && hasActiveClients() ) {
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

bool AccessibilityManager::hasActiveClients() const {
	return mScene && mScene->hasActiveAccessibilityClients();
}

void AccessibilityManager::invalidateChildren() {
	for ( auto& cache : mChildrenCache ) {
		cache.parent = {};
		cache.children.clear();
	}
}

}} // namespace EE::UI
