#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/accessibility/accessibility.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

std::atomic<Uint32> Accessibility::sActiveScenes{ 0 };

namespace {

// Embedded scenes report their host transition separately. Destroying widgets have already
// invalidated their subtree and must not be queried through their destroyed UIWidget part.
void onNodeParentChange( Node* node, bool removed ) {
	if ( node->isDestroying() || node->isSceneNode() ||
		 ( !node->isWidget() && !node->getFirstChild() ) )
		return;
	auto* nodeScene = node->getSceneNode();
	auto* scene = node->isWidget() ? node->asType<UIWidget>()->getUISceneNode()
				  : nodeScene && nodeScene->isUISceneNode() ? static_cast<UISceneNode*>( nodeScene )
															: nullptr;
	if ( scene && scene->hasActiveAccessibilityClients() )
		scene->getAccessibilityManager()->onNodeParentChange( node, removed );
}

} // namespace

void Accessibility::onSceneClientActivityChanged( bool active ) {
	// Only activity transitions register the bridge. Ordinary hierarchy changes neither
	// allocate a callback nor inspect UI state unless at least one scene has a native client.
	if ( active ) {
		if ( sActiveScenes.fetch_add( 1, std::memory_order_relaxed ) == 0 )
			SceneNode::setNodeParentChangeCallback( onNodeParentChange );
	} else if ( sActiveScenes.fetch_sub( 1, std::memory_order_relaxed ) == 1 ) {
		SceneNode::setNodeParentChangeCallback( nullptr );
	}
}

}} // namespace EE::UI
