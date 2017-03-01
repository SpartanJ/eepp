#include <eepp/ui/uicombobox.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UIComboBox *UIComboBox::New() {
	return eeNew( UIComboBox, () );
}

UIComboBox::UIComboBox() :
	UIComplexControl(),
	mDropDownList( NULL ),
	mButton( NULL )
{
	applyDefaultTheme();
}

UIComboBox::~UIComboBox() {
}

Uint32 UIComboBox::getType() const {
	return UI_TYPE_COMBOBOX;
}

bool UIComboBox::isType( const Uint32& type ) const {
	return UIComboBox::getType() == type ? true : UIComplexControl::isType( type );
}

void UIComboBox::setTheme( UITheme * Theme ) {
	if ( NULL == mDropDownList ) {
		mDropDownList = UIDropDownList::New();
		mDropDownList->setParent( this );
		mDropDownList->setVisible( true );
		mDropDownList->setEnabled( true );
		mDropDownList->setAllowEditing( true );
		mDropDownList->getInputTextBuffer()->setFreeEditing( true );
		mDropDownList->setFriendControl( this );
	}

	if ( NULL == mButton ) {
		mButton = eeNew( UIComplexControl, () );
		mButton->setParent( this );
		mButton->setVisible( true );
		mButton->setEnabled( true );
		mButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIComboBox::onButtonClick ) );
	}

	mDropDownList->setThemeControl( Theme, "combobox" );
	mDropDownList->doAfterSetTheme();

	mButton->setThemeControl( Theme, "combobox_button" );

	if ( NULL != mDropDownList->getSkin() ) {
		setInternalHeight( mDropDownList->getSkin()->getSize().getHeight() );
	}

	if ( NULL != mButton->getSkin() ) {
		mButton->setSize( mButton->getSkin()->getSize() );
	}

	updateControls();
}

UIListBox *UIComboBox::getListBox() {
	return mDropDownList->getListBox();
}

InputTextBuffer *UIComboBox::getInputTextBuffer() {
	return mDropDownList->getInputTextBuffer();
}

const String& UIComboBox::getText() {
	return mDropDownList->getText();
}

void UIComboBox::updateControls() {
	if ( ( mFlags & UI_AUTO_SIZE ) || mSize.getHeight() < mDropDownList->getSkin()->getSize().getHeight() ) {
		onAutoSize();
	}

	mDropDownList->setPosition( 0, 0 );
	mDropDownList->setSize( mSize.getWidth() - mButton->getSize().getWidth(), mSize.getHeight() );
	mDropDownList->getListBox()->setSize( mSize.getWidth(), mDropDownList->getListBox()->getSize().getHeight() );

	mButton->setPosition( mSize.getWidth() - mButton->getSize().getWidth(), 0 );
	mButton->centerVertical();
}

void UIComboBox::onButtonClick( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
		mDropDownList->showList();
	}
}

void UIComboBox::onSizeChange() {
	UIComplexControl::onSizeChange();

	updateControls();
}

void UIComboBox::onPositionChange() {
	UIComplexControl::onPositionChange();

	updateControls();
}

void UIComboBox::onAutoSize() {
	setInternalHeight( mDropDownList->getSkin()->getSize().getHeight() );
}

}}
