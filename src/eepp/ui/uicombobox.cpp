#include <eepp/ui/uicombobox.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UIComboBox::UIComboBox( UIComboBox::CreateParams& Params ) :
	UIDropDownList( Params ),
	mButton( NULL )
{
	AllowEditing( true );

	ApplyDefaultTheme();
}

UIComboBox::~UIComboBox() {
}

Uint32 UIComboBox::Type() const {
	return UI_TYPE_COMBOBOX;
}

bool UIComboBox::IsType( const Uint32& type ) const {
	return UIComboBox::Type() == type ? true : UIDropDownList::IsType( type );
}

void UIComboBox::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "combobox" );

	AutoSizeControl();

	CreateButton();

	AutoPadding();

	OnSizeChange();
}

void UIComboBox::CreateButton() {
	eeSAFE_DELETE( mButton );

	Int32 btnWidth = 0;

	if ( NULL != mSkinState && NULL != mSkinState->GetSkin() ) {
		if ( mSkinState->GetSkin()->GetType() == UISkin::SkinComplex ) {
			UISkinComplex * tComplex = reinterpret_cast<UISkinComplex*> ( mSkinState->GetSkin() );

			SubTexture * tSubTexture = tComplex->GetSubTextureSide( UISkinState::StateNormal, UISkinComplex::Right );

			if ( NULL != tSubTexture )
				btnWidth = tSubTexture->realSize().width();
		}
	}

	UIControl::CreateParams Params;
	Params.Parent( this ),
	Params.Size = Sizei( btnWidth, mSize.height() );
	Params.PosSet( mSize.width() - btnWidth, 0 );
	mButton = eeNew( UIControl, ( Params ) );
	mButton->Visible( true );
	mButton->Enabled( true );
	mButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIComboBox::OnButtonClick ) );
	mButton->AddEventListener( UIEvent::EventMouseEnter, cb::Make1( this, &UIComboBox::OnButtonEnter ) );
	mButton->AddEventListener( UIEvent::EventMouseExit, cb::Make1( this, &UIComboBox::OnButtonExit ) );
}

void UIComboBox::OnButtonClick( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MEvent->Flags() & EE_BUTTON_LMASK ) {
		ShowListBox();
	}
}

void UIComboBox::OnButtonEnter( const UIEvent * Event ) {
	SetSkinState( UISkinState::StateMouseEnter );
}

void UIComboBox::OnButtonExit( const UIEvent * Event ) {
	SetSkinState( UISkinState::StateMouseExit );
}

Uint32 UIComboBox::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		UITextInput::OnMouseClick( Pos, Flags );

		if ( mListBox->Visible() ) {
			Hide();
		}
	}

	return 1;
}

void UIComboBox::OnControlClear( const UIEvent *Event ) {
}

}}
