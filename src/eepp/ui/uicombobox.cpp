#include <eepp/ui/uicombobox.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UIComboBox::UIComboBox( UIComboBox::CreateParams& Params ) :
	UIDropDownList( Params ),
	mButton( NULL )
{
	setAllowEditing( true );

	applyDefaultTheme();
}

UIComboBox::UIComboBox() :
	UIDropDownList(),
	mButton( NULL )
{
	setAllowEditing( true );

	applyDefaultTheme();
}

UIComboBox::~UIComboBox() {
}

Uint32 UIComboBox::getType() const {
	return UI_TYPE_COMBOBOX;
}

bool UIComboBox::isType( const Uint32& type ) const {
	return UIComboBox::getType() == type ? true : UIDropDownList::isType( type );
}

void UIComboBox::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "combobox" );

	autoSizeControl();

	autoPadding();

	onSizeChange();
}

void UIComboBox::createButton() {
	Int32 btnWidth = 0;

	if ( NULL != mSkinState && NULL != mSkinState->getSkin() ) {
		if ( mSkinState->getSkin()->getType() == UISkin::SkinComplex ) {
			UISkinComplex * tComplex = reinterpret_cast<UISkinComplex*> ( mSkinState->getSkin() );

			SubTexture * tSubTexture = tComplex->getSubTextureSide( UISkinState::StateNormal, UISkinComplex::Right );

			if ( NULL != tSubTexture )
				btnWidth = tSubTexture->getRealSize().getWidth();
		}
	}

	if ( NULL == mButton ) {
		mButton = eeNew( UIControl, () );
		mButton->setParent( this );
		mButton->setVisible( true );
		mButton->setEnabled( true );
		mButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIComboBox::onButtonClick ) );
		mButton->addEventListener( UIEvent::EventMouseEnter, cb::Make1( this, &UIComboBox::onButtonEnter ) );
		mButton->addEventListener( UIEvent::EventMouseExit, cb::Make1( this, &UIComboBox::onButtonExit ) );

	}

	mButton->setPixelsSize( btnWidth, mRealSize.getHeight() );
	mButton->setPixelsPosition( mRealSize.getWidth() - btnWidth, 0 );
}

void UIComboBox::onButtonClick( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
		showListBox();
	}
}

void UIComboBox::onButtonEnter( const UIEvent * Event ) {
	setSkinState( UISkinState::StateMouseEnter );
}

void UIComboBox::onButtonExit( const UIEvent * Event ) {
	setSkinState( UISkinState::StateMouseExit );
}

Uint32 UIComboBox::onMouseClick( const Vector2i& position, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		UITextInput::onMouseClick( position, Flags );

		if ( mListBox->isVisible() ) {
			hide();
		}
	}

	return 1;
}

void UIComboBox::onControlClear( const UIEvent *Event ) {
}

void UIComboBox::onSizeChange() {
	UIDropDownList::onSizeChange();

	createButton();
}

}}
