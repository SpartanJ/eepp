#include <eepp/system/thread.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace UI { namespace Abstract {

UIAbstractView::UIAbstractView( const std::string& tag ) :
	UIScrollableWidget( tag ), mSelection( this ) {}

UIAbstractView::~UIAbstractView() {}

std::function<void( const ModelIndex& )> UIAbstractView::getOnSelection() const {
	return mOnSelection;
}

void UIAbstractView::setOnSelection( const std::function<void( const ModelIndex& )>& onSelection ) {
	mOnSelection = onSelection;
}

std::function<void()> UIAbstractView::getOnSelectionChange() const {
	return mOnSelectionChange;
}

void UIAbstractView::setOnSelectionChange( const std::function<void()>& onSelectionChange ) {
	mOnSelectionChange = onSelectionChange;
}

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

void UIAbstractView::modelUpdate( unsigned flags ) {
	if ( !getModel() || ( flags & Model::InvalidateAllIndexes ) ) {
		getSelection().clear();
	} else {
		getSelection().removeMatching(
			[this]( auto& index ) { return !getModel()->isValid( index ); } );
	}
}

void UIAbstractView::onModelUpdate( unsigned flags ) {
	if ( !Engine::instance()->isMainThread() ) {
		runOnMainThread( [&, flags] { modelUpdate( flags ); } );
	} else {
		modelUpdate( flags );
	}
}

void UIAbstractView::onModelSelectionChange() {
	if ( getModel() && mOnSelection && getSelection().first().isValid() )
		mOnSelection( getSelection().first() );
	invalidateDraw();
}

void UIAbstractView::notifySelectionChange() {
	onModelSelectionChange();
	if ( mOnSelectionChange )
		mOnSelectionChange();
}

ModelIndex UIAbstractView::findRowWithText( const std::string&, const bool&, const bool& ) const {
	return {};
}

}}} // namespace EE::UI::Abstract
