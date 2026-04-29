#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIDropDownList* UIDropDownList::NewWithTag( const std::string& tag ) {
	return eeNew( UIDropDownList, ( tag ) );
}

UIDropDownList* UIDropDownList::New() {
	return eeNew( UIDropDownList, () );
}

UIDropDownList::UIDropDownList( const std::string& tag ) : UIDropDown( tag ), mListBox( NULL ) {
	applyDefaultTheme();

	mFlags |= UI_LOADS_ITS_CHILDREN;

	mListBox = UIListBox::NewWithTag( mTag + "::listbox" );
	mListBox->setSize( getSize().getWidth(),
					   mStyleConfig.MaxNumVisibleItems * getSize().getHeight() );
	mListBox->setEnabled( false );
	mListBox->setVisible( false );
	// This will force to change the parent when shown, and force the CSS style reload.
	mListBox->setParent( this );

	mListBox->on( Event::OnWidgetFocusLoss, [this]( auto event ) { onPopUpFocusLoss( event ); } );
	mListBox->on( Event::OnItemSelected, [this]( auto event ) { onItemSelected( event ); } );
	mListBox->on( Event::OnItemClicked, [this]( auto event ) { onItemClicked( event ); } );
	mListBox->on( Event::OnItemKeyDown, [this]( auto event ) { onItemKeyDown( event ); } );
	mListBox->on( Event::KeyDown, [this]( auto event ) { onItemKeyDown( event ); } );
	mListBox->on( Event::OnClear, [this]( auto event ) { onWidgetClear( event ); } );
	mListBoxCloseCb =
		mListBox->on( Event::OnClose, [this]( const Event* ) { mListBox = nullptr; } );
	mListBox->on( Event::OnSelectionChanged, [this]( auto ) {
		if ( !mListBox->hasSelection() )
			mListBox->setSelected( 0 );
		sendCommonEvent( Event::OnSelectionChanged );
	} );
	mListBox->on( Event::OnItemValueChange, [this]( const Event* event ) {
		if ( mListBox->getItemSelectedIndex() == event->asItemValueEvent()->getItemIndex() )
			setText( mListBox->getItemSelectedText() );
	} );
}

UIDropDownList::~UIDropDownList() {
	if ( mListBox != nullptr && mListBoxCloseCb )
		mListBox->removeEventListener( mListBoxCloseCb );
	destroyListBox();
}

Uint32 UIDropDownList::getType() const {
	return UI_TYPE_DROPDOWNLIST;
}

bool UIDropDownList::isType( const Uint32& type ) const {
	return UIDropDownList::getType() == type ? true : UIDropDown::isType( type );
}

UIWidget* UIDropDownList::getPopUpWidget() const {
	return mListBox;
}

UIListBox* UIDropDownList::getListBox() const {
	return mListBox;
}

Uint32 UIDropDownList::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	if ( mEnabled && mVisible && isMouseOver() && NULL != mListBox ) {
		if ( Flags & EE_BUTTONS_WUWD ) {
			if ( Flags & EE_BUTTON_WUMASK ) {
				mListBox->selectPrev();
			} else if ( Flags & EE_BUTTON_WDMASK ) {
				if ( mListBox->getItemSelectedIndex() != eeINDEX_NOT_FOUND ) {
					mListBox->selectNext();
				} else {
					mListBox->setSelected( 0 );
				}
			}
		}
	}

	return UIDropDown::onMouseUp( Pos, Flags );
}

Uint32 UIDropDownList::onKeyDown( const KeyEvent& Event ) {
	if ( NULL != mListBox )
		mListBox->onKeyDown( Event );

	return UIDropDown::onKeyDown( Event );
}

UIDropDownList* UIDropDownList::showList() {
	if ( NULL == mListBox )
		return this;

	if ( !mListBox->isVisible() ) {
		if ( mListBox->getItemsCount() ) {
			Rectf tPadding = mListBox->getContainerPadding();
			Float sliderValue = mListBox->getVerticalScrollBar()->getValue();

			Float contentsWidth = eeceil( PixelDensity::pxToDp(
				mListBox->getMaxTextWidth() +
				PixelDensity::dpToPx( mListBox->getContainerPadding().getWidth() ) +
				mListBox->getReferenceItem()->getPixelsPadding().getWidth() +
				mListBox->getVerticalScrollBar()->getPixelsSize().getWidth() ) );

			Float width = getPopUpWidth( contentsWidth );

			Float height = std::ceil(
				std::ceil( eemin( mListBox->getItemsCount(), mStyleConfig.MaxNumVisibleItems ) *
						   mListBox->getRowHeight() ) +
				tPadding.Top + tPadding.Bottom +
				( mListBox->getHorizontalScrollBar() &&
						  mListBox->getHorizontalScrollBar()->isVisible() &&
						  PixelDensity::dpToPx( width ) < mListBox->getMaxTextWidth()
					  ? mListBox->getHorizontalScrollBar()->getSize().getHeight()
					  : 0.f ) );

			mListBox->setSize( width, height );
			mListBox->getVerticalScrollBar()->setValue( sliderValue );

			alignPopUp( mListBox );
		}
	} else {
		hide();
	}
	return this;
}

UIDropDownList* UIDropDownList::setMaxNumVisibleItems( const Uint32& maxNumVisibleItems ) {
	if ( maxNumVisibleItems != mStyleConfig.MaxNumVisibleItems ) {
		mStyleConfig.MaxNumVisibleItems = maxNumVisibleItems;

		if ( NULL != mListBox )
			mListBox->setSize( getSize().getWidth(), std::min( mStyleConfig.MaxNumVisibleItems,
															   getListBox()->getItemsCount() ) *
														 mListBox->getRowHeight() );
	}
	return this;
}

void UIDropDownList::onItemSelected( const Event* ) {
	setText( mListBox->getItemSelectedText() );

	NodeMessage Msg( this, NodeMessage::Selected, mListBox->getItemSelectedIndex() );
	messagePost( &Msg );

	sendCommonEvent( Event::OnItemSelected );
	sendCommonEvent( Event::OnValueChange );
	sendCommonEvent( Event::OnSelectionChanged );
}

void UIDropDownList::destroyListBox() {
	if ( !SceneManager::instance()->isShuttingDown() && NULL != mListBox &&
		 mListBox->getParent() != this ) {
		mListBox->setParent( this );
	}
}

bool UIDropDownList::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::SelectedIndex:
		case PropertyId::SelectedText:
		case PropertyId::ScrollBarStyle:
		case PropertyId::RowHeight:
		case PropertyId::VScrollMode:
		case PropertyId::HScrollMode:
			if ( NULL != mListBox )
				return mListBox->applyProperty( attribute );
			else
				return false;
		default:
			return UIDropDown::applyProperty( attribute );
	}

	return true;
}

std::string UIDropDownList::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::SelectedIndex:
		case PropertyId::SelectedText:
		case PropertyId::ScrollBarStyle:
		case PropertyId::RowHeight:
		case PropertyId::VScrollMode:
		case PropertyId::HScrollMode:
			if ( NULL != mListBox )
				return mListBox->getPropertyString( propertyDef, propertyIndex );
		default:
			return UIDropDown::getPropertyString( propertyDef, propertyIndex );
	}

	return "";
}

std::vector<PropertyId> UIDropDownList::getPropertiesImplemented() const {
	auto props = UIDropDown::getPropertiesImplemented();
	auto local = { PropertyId::SelectedIndex, PropertyId::SelectedText, PropertyId::ScrollBarStyle,
				   PropertyId::RowHeight,	  PropertyId::VScrollMode,	PropertyId::HScrollMode };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UIDropDownList::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	if ( NULL != mListBox )
		mListBox->loadItemsFromXmlNode( node );

	UIDropDown::loadFromXmlNode( node );

	endAttributesTransaction();
}

void UIDropDownList::onClassChange() {
	if ( mListBox )
		mListBox->setClasses( getClasses() );
}

}} // namespace EE::UI
