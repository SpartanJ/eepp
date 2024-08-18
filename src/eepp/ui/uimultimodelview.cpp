#include <eepp/ui/uimultimodelview.hpp>

namespace EE { namespace UI {

UIMultiModelView* UIMultiModelView::New() {
	return NewWithTag( "multimodelview" );
}

UIMultiModelView* UIMultiModelView::NewWithTag( const std::string& tag ) {
	return eeNew( UIMultiModelView, ( tag ) );
}

UIMultiModelView::UIMultiModelView( const std::string& tag ) : UIStackWidget( tag ) {
	auto modelEvent = [this]( const Event* event ) {
		const ModelEvent* mevent = static_cast<const ModelEvent*>( event );
		sendEvent( mevent );
	};
	auto selectionChange = [this]() {
		if ( mOnSelectionChange )
			mOnSelectionChange();
	};
	auto selection = [this]( const ModelIndex& index ) {
		if ( mOnSelection )
			mOnSelection( index );
	};
	mList = UIListView::New();
	mTable = UITableView::New();
	mList->setParent( this );
	mTable->setParent( this );
	mList->addEventListener( Event::OnModelEvent, modelEvent );
	mTable->addEventListener( Event::OnModelEvent, modelEvent );
	mList->setOnSelectionChange( selectionChange );
	mTable->setOnSelectionChange( selectionChange );
	mList->setOnSelection( selection );
	mTable->setOnSelection( selection );
	mList->setAutoExpandOnSingleColumn( true );
	setViewMode( List );
}

std::shared_ptr<Model> UIMultiModelView::getModel() const {
	return mModel;
}

void UIMultiModelView::setModel( const std::shared_ptr<Model>& model ) {
	mModel = model;
	mList->setModel( mModel );
	mTable->setModel( mModel );
}

const UIMultiModelView::ViewMode& UIMultiModelView::getViewMode() const {
	return mMode;
}

void UIMultiModelView::setViewMode( const ViewMode& mode ) {
	mMode = mode;
	switch ( mMode ) {
		case Table:
			setActiveWidget( mTable );
			break;
		case List:
		default:
			setActiveWidget( mList );
			break;
	}
}

UIAbstractView* UIMultiModelView::getCurrentView() const {
	switch ( mMode ) {
		case Table:
			return mTable;
		case List:
		default:
			return mList;
	}
}

std::function<void( const ModelIndex& )> UIMultiModelView::getOnSelection() const {
	return mOnSelection;
}

void UIMultiModelView::setOnSelection(
	const std::function<void( const ModelIndex& )>& onSelection ) {
	mOnSelection = onSelection;
}

std::function<void()> UIMultiModelView::getOnSelectionChange() const {
	return mOnSelectionChange;
}

void UIMultiModelView::setOnSelectionChange( const std::function<void()>& onSelectionChange ) {
	mOnSelectionChange = onSelectionChange;
}

void UIMultiModelView::setSelection( const ModelIndex& index, bool scrollToSelection ) {
	mList->setSelection( index, scrollToSelection );
	mTable->setSelection( index, scrollToSelection );
}

void UIMultiModelView::setSingleClickNavigation( bool singleClickNavigation ) {
	mList->setSingleClickNavigation( singleClickNavigation );
	mTable->setSingleClickNavigation( singleClickNavigation );
}

void UIMultiModelView::setMultiSelect( bool enable ) {
	auto kind =
		enable ? UIAbstractView::SelectionKind::Multiple : UIAbstractView::SelectionKind::Single;
	mList->setSelectionKind( kind );
	mTable->setSelectionKind( kind );
}

}} // namespace EE::UI
