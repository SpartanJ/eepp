#include <eepp/graphics/textureregion.hpp>
#include <eepp/ui/uicombobox.hpp>

namespace EE { namespace UI {

UIComboBox* UIComboBox::New() {
	return eeNew( UIComboBox, () );
}

UIComboBox::UIComboBox() : UIWidget( "combobox" ), mDropDownList( NULL ), mButton( NULL ) {
	applyDefaultTheme();
}

UIComboBox::~UIComboBox() {}

Uint32 UIComboBox::getType() const {
	return UI_TYPE_COMBOBOX;
}

bool UIComboBox::isType( const Uint32& type ) const {
	return UIComboBox::getType() == type ? true : UIWidget::isType( type );
}

void UIComboBox::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	if ( NULL == mDropDownList ) {
		mDropDownList = UIDropDownList::New();
		mDropDownList->setParent( this );
		mDropDownList->setFriendControl( this );
		mDropDownList->setVisible( true );
		mDropDownList->setEnabled( true );
		mDropDownList->setAllowEditing( true );
		mDropDownList->getInputTextBuffer()->setFreeEditing( true );
		mDropDownList->addEventListener( Event::OnPaddingChange,
										 [this]( const Event* ) { this->onPaddingChange(); } );
	}

	if ( NULL == mButton ) {
		mButton = UIWidget::NewWithTag( "combobox::button" );
		mButton->setParent( this );
		mButton->setVisible( true );
		mButton->setEnabled( true );
	}

	mDropDownList->setThemeSkin( Theme, "combobox" );
	mDropDownList->onThemeLoaded();

	mButton->setThemeSkin( Theme, "combobox_button" );

	if ( NULL != mDropDownList->getSkin() ) {
		setInternalHeight( mDropDownList->getSkinSize().getHeight() );
	}

	if ( NULL != mButton->getSkin() ) {
		mButton->setSize( mButton->getSkinSize() );
	}

	updateControls();

	onThemeLoaded();
}

UIListBox* UIComboBox::getListBox() {
	return mDropDownList->getListBox();
}

InputTextBuffer* UIComboBox::getInputTextBuffer() {
	return mDropDownList->getInputTextBuffer();
}

const String& UIComboBox::getText() {
	return mDropDownList->getText();
}

void UIComboBox::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UIWidget::loadFromXmlNode( node );

	if ( NULL != mDropDownList )
		mDropDownList->loadFromXmlNode( node );

	endAttributesTransaction();

	updateControls();
}

Uint32 UIComboBox::onMessage( const NodeMessage* Msg ) {
	if ( Msg->getMsg() == NodeMessage::Click && Msg->getSender() == mButton &&
		 ( Msg->getFlags() & EE_BUTTON_LMASK && NULL != mDropDownList ) ) {
		mDropDownList->showList();
	}

	return UIWidget::onMessage( Msg );
}

void UIComboBox::updateControls() {
	if ( NULL != mDropDownList ) {
		if ( ( mFlags & UI_AUTO_SIZE ) ||
			 getSize().getHeight() < mDropDownList->getSkinSize().getHeight() ) {
			onAutoSize();
		}

		mDropDownList->setPosition( 0, 0 );
		mDropDownList->setSize( getSize().getWidth() - mButton->getSize().getWidth(), 0 );
		mDropDownList->getListBox()->setSize( getSize().getWidth(),
											  mDropDownList->getListBox()->getSize().getHeight() );
		mDropDownList->centerVertical();
	}

	if ( NULL != mButton ) {
		mButton->setPosition( getSize().getWidth() - mButton->getSize().getWidth(), 0 );
		mButton->centerVertical();
	}
}

void UIComboBox::onSizeChange() {
	updateControls();

	UIWidget::onSizeChange();
}

void UIComboBox::onPositionChange() {
	updateControls();

	UIWidget::onPositionChange();
}

void UIComboBox::onPaddingChange() {
	updateControls();

	UIWidget::onPaddingChange();
}

void UIComboBox::onAutoSize() {
	if ( NULL != mDropDownList )
		setInternalHeight( mDropDownList->getSkinSize().getHeight() +
						   mDropDownList->getPadding().Top + mDropDownList->getPadding().Bottom );
}

}} // namespace EE::UI
