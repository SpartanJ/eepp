#include <eepp/ui/cuicombobox.hpp>
#include <eepp/graphics/csubtexture.hpp>

namespace EE { namespace UI {

cUIComboBox::cUIComboBox( cUIComboBox::CreateParams& Params ) :
	cUIDropDownList( Params ),
	mButton( NULL )
{
	AllowEditing( true );

	ApplyDefaultTheme();
}

cUIComboBox::~cUIComboBox() {
}

Uint32 cUIComboBox::Type() const {
	return UI_TYPE_COMBOBOX;
}

bool cUIComboBox::IsType( const Uint32& type ) const {
	return cUIComboBox::Type() == type ? true : cUIDropDownList::IsType( type );
}

void cUIComboBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "combobox" );

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

			cSubTexture * tSubTexture = tComplex->GetSubTextureSide( cUISkinState::StateNormal, cUISkinComplex::Right );

			if ( NULL != tSubTexture )
				btnWidth = tSubTexture->RealSize().Width();
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
	mButton->AddEventListener( cUIEvent::EventMouseEnter, cb::Make1( this, &cUIComboBox::OnButtonEnter ) );
	mButton->AddEventListener( cUIEvent::EventMouseExit, cb::Make1( this, &cUIComboBox::OnButtonExit ) );
}

void cUIComboBox::OnButtonClick( const cUIEvent * Event ) {
	const cUIEventMouse * MEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MEvent->Flags() & EE_BUTTON_LMASK ) {
		ShowListBox();
	}
}

void cUIComboBox::OnButtonEnter( const cUIEvent * Event ) {
	SetSkinState( cUISkinState::StateMouseEnter );
}

void cUIComboBox::OnButtonExit( const cUIEvent * Event ) {
	SetSkinState( cUISkinState::StateMouseExit );
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

void cUIComboBox::OnControlClear( const cUIEvent *Event ) {
}

}}
