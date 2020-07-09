#include <eepp/ui/abstract/uiabstractview.hpp>

namespace EE { namespace UI { namespace Abstract {

UIAbstractView::UIAbstractView( const std::string& tag ) :
	UIScrollableWidget( tag ), mSelection( this ) {}

UIAbstractView::~UIAbstractView() {}

Uint32 UIAbstractView::getType() const {
	return UI_TYPE_ABSTRACTVIEW;
}

bool UIAbstractView::isType( const Uint32& type ) const {
	return UIAbstractView::getType() == type ? true : UIScrollableWidget::isType( type );
}

void UIAbstractView::setModel( std::shared_ptr<Model> model ) {
	if ( model.get() == mModel.get() )
		return;
	if ( mModel )
		mModel->unregisterView( this );
	mModel = model;
	if ( mModel )
		mModel->registerView( this );
	onModelUpdate( Model::InvalidateAllIndexes );
}

void UIAbstractView::onModelUpdate( unsigned flags ) {
	mHoveredIndex = {};
	if ( !getModel() || ( flags & Model::InvalidateAllIndexes ) ) {
		getSelection().clear();
	} else {
		getSelection().removeMatching(
			[this]( auto& index ) { return !getModel()->isValid( index ); } );
	}
}

void UIAbstractView::onModelSelectionChange() {
	if ( getModel() && mOnSelection && getSelection().first().isValid() )
		mOnSelection( getSelection().first() );
}

void UIAbstractView::activate( const ModelIndex& index ) {
	if ( mOnActivation )
		mOnActivation( index );
}

void UIAbstractView::activateSelected() {
	if ( !mOnActivation )
		return;

	getSelection().forEachIndex( [this]( auto& index ) { mOnActivation( index ); } );
}

void UIAbstractView::notifySelectionChange() {
	onModelSelectionChange();
	if ( mOnSelectionChange )
		mOnSelectionChange();
}

}}} // namespace EE::UI::Abstract
