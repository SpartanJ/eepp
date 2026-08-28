#include <algorithm>
#include <eepp/system/thread.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uimodelcreator.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace UI { namespace Abstract {

static constexpr String::HashType OnModelUpdateTag = String::hash( "onModelUpdate" );

UIAbstractView::UIAbstractView( const std::string& tag ) :
	UIScrollableWidget( tag ), mSelection( this ) {}

UIAbstractView::~UIAbstractView() {
	// Unregister first so the view leaves the model's view set before its
	// members are torn down. This is defense-in-depth only: view callbacks are
	// safe because model events run on the main thread, not because of this
	// ordering.
	if ( mModel )
		mModel->unregisterView( this );
	eeSAFE_DELETE( mEditingDelegate );
}

UIAbstractView::SelectionKind UIAbstractView::getSelectionKind() const {
	return mSelectionKind;
}

void UIAbstractView::setSelectionKind( UIAbstractView::SelectionKind selectionKind ) {
	mSelectionKind = selectionKind;
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

Uint32 UIAbstractView::onModelEvent( const std::function<void( const ModelEvent* )>& callback,
									 const Event::EventType& triggerEventType ) {
	return on( Event::OnModelEvent, [callback, triggerEventType]( const Event* event ) {
		auto modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModel() && modelEvent->getModelIndex().isValid() &&
			 ( triggerEventType == Event::EventType::NoEvent ||
			   modelEvent->getTriggerEvent()->getType() == triggerEventType ) ) {
			callback( modelEvent );
		}
	} );
}

std::vector<KeyBindings::Shortcut> UIAbstractView::getEditShortcuts() const {
	return mEditShortcuts;
}

void UIAbstractView::setEditShortcuts( const std::vector<KeyBindings::Shortcut>& editShortcuts ) {
	mEditShortcuts = editShortcuts;
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

bool UIAbstractView::isEditing() const {
	return mEditIndex.isValid();
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
	sendCommonEvent( Event::OnModelChanged );
}

void UIAbstractView::modelUpdate( unsigned flags ) {
	mPendingUpdateFlags = 0;
	if ( !getModel() || ( flags & Model::UpdateFlag::InvalidateAllIndexes ) ) {
		getSelection().clear();
	} else {
		getSelection().removeAllMatching(
			[this]( auto& index ) { return !getModel()->isValid( index ); } );
	}
}

void UIAbstractView::onModelIndexDeleted( const void* internalData ) {
	auto indexes = getSelection().indexes();
	auto firstRemoved =
		std::remove_if( indexes.begin(), indexes.end(), [internalData]( const ModelIndex& index ) {
			return index.internalData() == internalData;
		} );
	if ( firstRemoved == indexes.end() )
		return;
	indexes.erase( firstRemoved, indexes.end() );
	// Model-driven deletion must not invoke user selection callbacks re-entrantly.
	getSelection().set( indexes, false );
}

void UIAbstractView::onModelUpdate( unsigned flags ) {
	mPendingUpdateFlags.fetch_or( flags );
	if ( !Engine::instance()->isMainThread() ) {
		removeActionsByTag( OnModelUpdateTag );
		runOnMainThread( [this] { modelUpdate( mPendingUpdateFlags.exchange( 0 ) ); }, Time::Zero,
						 OnModelUpdateTag );
	} else {
		modelUpdate( mPendingUpdateFlags.exchange( 0 ) );
	}
}

void UIAbstractView::onModelSelectionChange() {
	if ( getModel() && mOnSelection && getSelection().first().isValid() )
		mOnSelection( getSelection().first() );
	invalidateDraw();
}

void UIAbstractView::notifySelectionChange() {
	if ( !Engine::isMainThread() ) {
		debounce( [this] { notifySelectionChange(); }, Time::Zero,
				  String::hash( "notifySelectionChange" ) );
		return;
	}

	onModelSelectionChange();
	sendCommonEvent( Event::OnSelectionChanged );
	if ( mOnSelectionChange )
		mOnSelectionChange();
}

ModelIndex UIAbstractView::findRowWithText( const std::string&, const bool&,
											FindRowWithTextMatchKind ) const {
	return {};
}

void UIAbstractView::beginEditing( const ModelIndex& index, UIWidget* editedWidget ) {
	if ( !isEditable() || !mModel || mEditIndex == index || !mModel->isEditable( index ) ||
		 !onCreateEditingDelegate || !editedWidget )
		return;

	if ( mEditWidget ) {
		mEditWidget->setVisible( false )->setEnabled( false )->close();
		mEditWidget = nullptr;
	}
	eeSAFE_DELETE( mEditingDelegate );

	mEditIndex = index;
	mEditingDelegate = onCreateEditingDelegate( index );
	mEditingDelegate->bind( mModel, index );
	mEditingDelegate->setValue( index.data( mEditingDelegate->pullDataFrom() ) );
	mEditWidget = mEditingDelegate->getWidget();
	mEditWidget->setParent( editedWidget );
	mEditWidget->setSize( editedWidget->getSize() );
	mEditWidget->setFocus();
	mEditWidget->toFront();
	mEditingDelegate->willBeginEditing();
	mEditingDelegate->onCommit = [this]() {
		if ( getModel() && mEditIndex.isValid() ) {
			getModel()->setData( mEditIndex, mEditingDelegate->getValue() );
			if ( mEditingDelegate->onValueSet )
				mEditingDelegate->onValueSet();
		}
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

bool UIAbstractView::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Editable:
			setEditable( attribute.asBool() );
			break;
		case PropertyId::SelectionType: {
			setSelectionType( String::iequals( attribute.getValue(), "cell" )
								  ? SelectionType::Cell
								  : SelectionType::Row );
			break;
		}
		case PropertyId::SelectionKind: {
			setSelectionKind( String::iequals( attribute.getValue(), "multiple" )
								  ? SelectionKind::Multiple
								  : SelectionKind::Single );
			break;
		}
		case PropertyId::TableModel: {
			const std::string& val = attribute.asString();
			if ( !val.empty() ) {
				auto model = UIModelCreator::createFromName( val, this );
				if ( model )
					setModel( std::move( model ) );
			}
			break;
		}
		default:
			return UIScrollableWidget::applyProperty( attribute );
	}

	return true;
}

std::string UIAbstractView::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Editable:
			return isEditable() ? "true" : "false";
		case PropertyId::SelectionType:
			return isCellSelection() ? "cell" : "row";
		case PropertyId::SelectionKind:
			return mSelectionKind == SelectionKind::Multiple ? "multiple" : "single";
		default:
			return UIScrollableWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIAbstractView::getPropertiesImplemented() const {
	auto props = UIScrollableWidget::getPropertiesImplemented();
	props.insert( props.end(), { PropertyId::Editable, PropertyId::SelectionType,
								 PropertyId::SelectionKind, PropertyId::TableModel } );
	return props;
}

}}} // namespace EE::UI::Abstract
