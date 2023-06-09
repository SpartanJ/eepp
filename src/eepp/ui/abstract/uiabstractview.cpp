#include <eepp/system/thread.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace UI { namespace Abstract {

UIAbstractView::UIAbstractView( const std::string& tag ) :
	UIScrollableWidget( tag ), mSelection( this ) {}

UIAbstractView::~UIAbstractView() {
	eeSAFE_DELETE( mEditingDelegate );
}

UIAbstractView::SelectionType UIAbstractView::getSelectionType() const {
	return mSelectionType;
}

void UIAbstractView::setSelectionType( SelectionType selectionType ) {
	mSelectionType = selectionType;

	if ( selectionType == UIAbstractView::SelectionType::Cell ) {
		addClass( "selection_type_cell" );
	} else {
		removeClass( "selection_type_cell" );
	}
}

KeyBindings::Shortcut UIAbstractView::getEditShortcut() const {
	return mEditShortcut;
}

void UIAbstractView::setEditShortcut( const KeyBindings::Shortcut& editShortcut ) {
	mEditShortcut = editShortcut;
}

Uint32 UIAbstractView::getEditTriggers() const {
	return mEditTriggers;
}

void UIAbstractView::setEditTriggers( Uint32 editTriggers ) {
	mEditTriggers = editTriggers;
}

bool UIAbstractView::isEditable() const {
	return mEditable;
}

void UIAbstractView::setEditable( bool editable ) {
	mEditable = editable;

	if ( editable ) {
		addClass( "editable_cells" );
	} else {
		removeClass( "editable_cells" );
	}
}

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

bool UIAbstractView::isCellSelection() const {
	return mSelectionType == UIAbstractView::SelectionType::Cell;
}

bool UIAbstractView::isRowSelection() const {
	return mSelectionType == UIAbstractView::SelectionType::Row;
}

Uint32 UIAbstractView::getType() const {
	return UI_TYPE_ABSTRACTVIEW;
}

bool UIAbstractView::isType( const Uint32& type ) const {
	return UIAbstractView::getType() == type ? true : UIScrollableWidget::isType( type );
}

void UIAbstractView::setModel( const std::shared_ptr<Model>& model ) {
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
		getSelection().removeAllMatching(
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

void UIAbstractView::beginEditing( const ModelIndex& index, UIWidget* editedWidget ) {
	if ( !isEditable() || !mModel || mEditIndex == index || !mModel->isEditable( index ) ||
		 !onCreateEditingDelegate )
		return;

	if ( mEditWidget ) {
		mEditWidget->setVisible( false )->setEnabled( false )->close();
		mEditWidget = nullptr;
	}
	eeSAFE_DELETE( mEditingDelegate );

	mEditIndex = index;
	mEditingDelegate = onCreateEditingDelegate( index );
	mEditingDelegate->bind( mModel, index );
	mEditingDelegate->setValue( index.data() );
	mEditWidget = mEditingDelegate->getWidget();
	mEditWidget->setParent( editedWidget );
	mEditWidget->setSize( editedWidget->getSize() );
	mEditWidget->setFocus();
	mEditWidget->toFront();
	mEditingDelegate->willBeginEditing();
	mEditingDelegate->onCommit = [this]() {
		if ( getModel() && mEditIndex.isValid() )
			getModel()->setData( mEditIndex, mEditingDelegate->getValue() );
		stopEditing();
	};
	mEditingDelegate->onRollback = [this]() { stopEditing(); };
	mEditingDelegate->onChange = [this, index]() { editingWidgetDidChange( index ); };
}

void UIAbstractView::stopEditing() {
	bool recoverFocus = false;
	mEditIndex = {};
	if ( mEditWidget ) {
		recoverFocus = mEditWidget->hasFocusWithin();
		mEditWidget->setVisible( false )->setEnabled( false )->close();
		mEditWidget = nullptr;
	}
	if ( recoverFocus )
		setFocus();
}

}}} // namespace EE::UI::Abstract
