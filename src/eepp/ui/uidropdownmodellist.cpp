#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIDropDownModelList* UIDropDownModelList::NewWithTag( const std::string& tag ) {
	return eeNew( UIDropDownModelList, ( tag ) );
}

UIDropDownModelList* UIDropDownModelList::New() {
	return eeNew( UIDropDownModelList, () );
}

UIDropDownModelList::UIDropDownModelList( const std::string& tag ) :
	UIDropDown( tag ), mListView( NULL ) {
	mListView = static_cast<UIAbstractTableView*>( createDefaultListView() );
	mListView->setEnabled( false );
	mListView->setVisible( false );
	mListView->setParent( this );
	mListView->setSingleClickNavigation( true );

	mListView->on( Event::OnWidgetFocusLoss, [this]( auto event ) { onPopUpFocusLoss( event ); } );
	mListView->on( Event::OnModelEvent, [this]( auto event ) { onItemSelected( event ); } );
	mListView->on( Event::KeyDown, [this]( auto event ) { onItemKeyDown( event ); } );
	mListView->on( Event::OnClear, [this]( auto event ) { onWidgetClear( event ); } );
	mListViewCloseCb =
		mListView->on( Event::OnClose, [this]( const Event* ) { mListView = nullptr; } );

	mListView->setOnSelectionChange( [this]() {
		if ( !mListView->getSelection().isEmpty() ) {
			updateSelectionIndex();
			sendCommonEvent( Event::OnSelectionChanged );
		}
	} );

	applyDefaultTheme();
}

UIDropDownModelList::~UIDropDownModelList() {
	if ( mListView != nullptr && mListViewCloseCb )
		mListView->removeEventListener( mListViewCloseCb );
	destroyListView();
}

UIWidget* UIDropDownModelList::createDefaultListView() {
	return UIListView::NewWithTag( mTag + "::listview" );
}

Uint32 UIDropDownModelList::getType() const {
	return UI_TYPE_DROPDOWNMODELLIST;
}

bool UIDropDownModelList::isType( const Uint32& type ) const {
	return UIDropDownModelList::getType() == type ? true : UIDropDown::isType( type );
}

UIWidget* UIDropDownModelList::getPopUpWidget() const {
	return mListView;
}

UIAbstractTableView* UIDropDownModelList::getListView() const {
	return mListView;
}

void UIDropDownModelList::setListView( UIAbstractTableView* listView ) {
	if ( listView == mListView )
		return;

	if ( mListView != nullptr ) {
		if ( mListViewCloseCb )
			mListView->removeEventListener( mListViewCloseCb );
		mListView->close();
	}

	mListView = listView;
	mListView->setEnabled( false );
	mListView->setVisible( false );
	mListView->setParent( this );

	mListView->on( Event::OnWidgetFocusLoss, [this]( auto event ) { onPopUpFocusLoss( event ); } );
	mListView->on( Event::OnModelEvent, [this]( auto event ) { onItemSelected( event ); } );
	mListView->on( Event::KeyDown, [this]( auto event ) { onItemKeyDown( event ); } );
	mListView->on( Event::OnClear, [this]( auto event ) { onWidgetClear( event ); } );
	mListViewCloseCb =
		mListView->on( Event::OnClose, [this]( const Event* ) { mListView = nullptr; } );

	mListView->setOnSelectionChange( [this]() {
		if ( !mListView->getSelection().isEmpty() ) {
			sendCommonEvent( Event::OnSelectionChanged );
		}
	} );

	if ( mModel ) {
		mListView->setModel( mModel );
	}
}

std::shared_ptr<Model> UIDropDownModelList::getModel() const {
	return mModel;
}

void UIDropDownModelList::setModel( std::shared_ptr<Model> model ) {
	mModel = model;
	if ( mListView ) {
		mListView->setModel( mModel );
	}
}

Uint32 UIDropDownModelList::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	if ( mEnabled && mVisible && isMouseOver() && NULL != mListView && mModel ) {
		if ( Flags & EE_BUTTONS_WUWD ) {
			if ( Flags & EE_BUTTON_WUMASK ) {
				mListView->moveSelection( -1 );
				updateSelectionIndex();
			} else if ( Flags & EE_BUTTON_WDMASK ) {
				if ( !mListView->getSelection().isEmpty() ) {
					mListView->moveSelection( 1 );
					updateSelectionIndex();
				} else if ( mModel->hasChildren() ) {
					mListView->getSelection().set( mModel->index( 0, 0 ) );
					updateSelectionIndex();
				}
			}
		}
	}

	return UIDropDown::onMouseUp( Pos, Flags );
}

Uint32 UIDropDownModelList::onKeyDown( const KeyEvent& Event ) {
	if ( NULL != mListView )
		mListView->onKeyDown( Event );

	return UIDropDown::onKeyDown( Event );
}

UIDropDownModelList* UIDropDownModelList::showList() {
	if ( NULL == mListView || NULL == mModel )
		return this;

	if ( !mListView->isVisible() ) {
		if ( !mModel->hasChildren() )
			return this;

		Rectf tPadding = mListView->getPadding();

		Float sliderValue = 0;
		if ( mListView->getVerticalScrollBar() )
			sliderValue = mListView->getVerticalScrollBar()->getValue();

		Float contentsWidth = eeceil( PixelDensity::pxToDp(
			mListView->getMaxColumnContentWidth( 0, true ) +
			PixelDensity::dpToPx( mListView->getPadding().getWidth() ) +
			( mListView->getVerticalScrollBar()
				  ? mListView->getVerticalScrollBar()->getPixelsSize().getWidth()
				  : 0.f ) ) );

		Float width = getPopUpWidth( contentsWidth );

		Float height =
			(Int32)( eemin( (Uint32)mModel->rowCount(), mStyleConfig.MaxNumVisibleItems ) *
					 mListView->getRowHeight() ) +
			tPadding.Top + tPadding.Bottom + mListView->getHeaderHeight() +
			( mListView->getHorizontalScrollBar() &&
					  mListView->getHorizontalScrollBar()->isVisible()
				  ? mListView->getHorizontalScrollBar()->getSize().getHeight()
				  : 0.f );

		mListView->setSize( width, height );

		if ( mListView->getVerticalScrollBar() )
			mListView->getVerticalScrollBar()->setValue( sliderValue );

		alignPopUp( mListView );
	} else {
		hide();
	}
	return this;
}

UIDropDownModelList*
UIDropDownModelList::setMaxNumVisibleItems( const Uint32& maxNumVisibleItems ) {
	if ( maxNumVisibleItems != mStyleConfig.MaxNumVisibleItems ) {
		mStyleConfig.MaxNumVisibleItems = maxNumVisibleItems;

		if ( NULL != mListView && mModel )
			mListView->setSize( getSize().getWidth(), std::min( mStyleConfig.MaxNumVisibleItems,
																(Uint32)mModel->rowCount() ) *
														  mListView->getRowHeight() );
	}
	return this;
}

void UIDropDownModelList::onItemClicked( const Event* ) {
	updateSelectionIndex();
	hide();
	setFocus();
}

void UIDropDownModelList::updateSelectionIndex() {
	if ( mListView->getSelection().isEmpty() )
		return;
	ModelIndex idx = mListView->getSelection().first();
	if ( idx.isValid() ) {
		Variant var = mModel->data( idx, ModelRole::Display );
		if ( var.isValid() && var.isString() )
			setText( var.toString() );
		sendCommonEvent( Event::OnItemSelected );
		sendCommonEvent( Event::OnValueChange );
	}
}

void UIDropDownModelList::onItemSelected( const Event* event ) {
	if ( event->getType() == Event::OnModelEvent ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			updateSelectionIndex();
			hide();
			setFocus();
		}
	}
}

void UIDropDownModelList::destroyListView() {
	if ( !SceneManager::instance()->isShuttingDown() && NULL != mListView &&
		 mListView->getParent() != this ) {
		mListView->setParent( this );
	}
}

std::string UIDropDownModelList::getPropertyString( const PropertyDefinition* propertyDef,
													const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	std::string res = UIDropDown::getPropertyString( propertyDef, propertyIndex );
	if ( res.empty() && NULL != mListView ) {
		res = mListView->getPropertyString( propertyDef, propertyIndex );
	}
	return res;
}

std::vector<PropertyId> UIDropDownModelList::getPropertiesImplemented() const {
	auto props = UIDropDown::getPropertiesImplemented();
	if ( mListView ) {
		auto listProps = mListView->getPropertiesImplemented();
		props.insert( props.end(), listProps.begin(), listProps.end() );
	}
	return props;
}

void UIDropDownModelList::onClassChange() {
	if ( mListView )
		mListView->setClasses( getClasses() );
}

}} // namespace EE::UI
