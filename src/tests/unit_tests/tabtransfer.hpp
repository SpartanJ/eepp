#ifndef EE_TESTS_UNIT_TESTS_TABTRANSFER_HPP
#define EE_TESTS_UNIT_TESTS_TABTRANSFER_HPP

#include <eepp/scene/nodemessage.hpp>
#include <eepp/ui/uitabwidget.hpp>

/**
 * Drives the production tab drag & drop transfer path ( the Drop message handled by
 * UITabWidget::onMessage ), which performs the removeTab + add + setTabSelected sequence used to
 * move a tab between tab widgets. The destination split function is cleared so the drop always
 * transfers the tab into the destination instead of creating a new split.
 */
inline void transferTabTo( EE::UI::UITab* tab, EE::UI::UITabWidget* destination ) {
	destination->setSplitFunction( nullptr );
	EE::Scene::NodeDropMessage drop( destination, EE::Scene::NodeMessage::Drop, tab );
	destination->messagePost( &drop );
}

#endif
