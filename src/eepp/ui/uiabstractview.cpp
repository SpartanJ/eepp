#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/uiabstractview.hpp>
#include <eepp/window/input.hpp>

using namespace EE::Scene;
using namespace EE::Window;

namespace EE { namespace UI {

UIAbstractView::UIAbstractView() : m_selection( this ) {}

UIAbstractView::~UIAbstractView() {}

void UIAbstractView::set_model( std::shared_ptr<Model> model ) {
	if ( model.get() == m_model.get() )
		return;
	if ( m_model )
		m_model->unregisterView( this );
	m_model = model;
	if ( m_model )
		m_model->registerView( this );
	didUpdateModel( Model::InvalidateAllIndexes );
}

void UIAbstractView::didUpdateModel( unsigned flags ) {
	m_hovered_index = {};
	if ( !model() || ( flags & Model::InvalidateAllIndexes ) ) {
		selection().clear();
	} else {
		selection().removeMatching( [this]( auto& index ) { return !model()->isValid( index ); } );
	}
}

void UIAbstractView::didUpdateSelection() {
	if ( model() && on_selection && selection().first().isValid() )
		on_selection( selection().first() );
}

void UIAbstractView::did_scroll() {}

void UIAbstractView::activate( const ModelIndex& index ) {
	if ( on_activation )
		on_activation( index );
}

void UIAbstractView::activate_selected() {
	if ( !on_activation )
		return;

	selection().forEachIndex( [this]( auto& index ) { on_activation( index ); } );
}

void UIAbstractView::notifySelectionChange() {
	didUpdateSelection();
	if ( on_selection_change )
		on_selection_change();
}

}} // namespace EE::UI
