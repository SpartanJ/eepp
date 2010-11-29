#include "cuicombobox.hpp"

namespace EE { namespace UI {

cUIComboBox::cUIComboBox( cUIComboBox::CreateParams& Params ) :
	cUIDropDownList( Params )
{
	mType |= UI_TYPE_GET( UI_TYPE_COMBOBOX );

	AllowEditing( true );

	ApplyDefaultTheme();
}

cUIComboBox::~cUIComboBox() {
}

void cUIComboBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "combobox" );

	AutoPadding();

	OnSizeChange();
}

Uint32 cUIComboBox::OnMouseClick( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		eeVector2i CtrlPos = Pos;
		ScreenToControl( CtrlPos );

		Int32 btnWidth = 0;

		if ( NULL != mSkinState && NULL != mSkinState->GetSkin() ) {
			if ( mSkinState->GetSkin()->GetType() == cUISkin::UISkinComplex ) {

				cUISkinComplex * tComplex = reinterpret_cast<cUISkinComplex*> ( mSkinState->GetSkin() );

				cShape * tShape = tComplex->GetShapeSide( cUISkinState::StateNormal, cUISkinComplex::Right );

				if ( NULL != tShape )
					btnWidth = tShape->RealSize().Width();
			}
		}
		// Fix for scaled/rotated combobox
		if ( CtrlPos.x >= mSize.Width() - btnWidth ) {
			ShowListBox();
		} else {
			cUITextInput::OnMouseClick( Pos, Flags );
		}
	}

	return 1;
}

void cUIComboBox::OnItemSelected( const cUIEvent * Event ) {
	if ( mListBox->Visible() ) {
		mListBox->Enabled( false );
		mListBox->Visible( false );
	}

	mTextBuffer.Buffer( mListBox->GetItemSelectedText() );

	eeVector2f offSet = mAlignOffset;

	Text( mTextBuffer.Buffer() );

	mAlignOffset = offSet;

	ResetWaitCursor();

	AlignFix();

	mCursorPos = mTextBuffer.CurPos();
}

}}
