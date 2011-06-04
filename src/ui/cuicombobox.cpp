#include "cuicombobox.hpp"

namespace EE { namespace UI {

cUIComboBox::cUIComboBox( cUIComboBox::CreateParams& Params ) :
	cUIDropDownList( Params ),
	mButton( NULL )
{
	mType = UI_TYPE_COMBOBOX;

	AllowEditing( true );

	ApplyDefaultTheme();
}

cUIComboBox::~cUIComboBox() {
}

void cUIComboBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "combobox" );

	AutoSizeControl();

	CreateButton();

	AutoPadding();

	OnSizeChange();
}

void cUIComboBox::CreateButton() {
	eeSAFE_DELETE( mButton );

	Int32 btnWidth = 0;

	if ( NULL != mSkinState && NULL != mSkinState->GetSkin() ) {
		if ( mSkinState->GetSkin()->GetType() == cUISkin::UISkinComplex ) {
			cUISkinComplex * tComplex = reinterpret_cast<cUISkinComplex*> ( mSkinState->GetSkin() );

			cShape * tShape = tComplex->GetShapeSide( cUISkinState::StateNormal, cUISkinComplex::Right );

			if ( NULL != tShape )
				btnWidth = tShape->RealSize().Width();
		}
	}

	cUIControl::CreateParams Params;
	Params.Parent( this ),
	Params.Size = eeSize( btnWidth, mSize.Height() );
	Params.PosSet( mSize.Width() - btnWidth, 0 );
	mButton = eeNew( cUIControl, ( Params ) );
	mButton->Visible( true );
	mButton->Enabled( true );
	mButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUIComboBox::OnButtonClick ) );
}

void cUIComboBox::OnButtonClick( const cUIEvent * Event ) {
	const cUIEventMouse * MEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MEvent->Flags() & EE_BUTTON_LMASK ) {
		ShowListBox();
	}
}

Uint32 cUIComboBox::OnMouseClick( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		cUITextInput::OnMouseClick( Pos, Flags );

		if ( mListBox->Visible() ) {
			Hide();
		}
	}

	return 1;
}

}}
