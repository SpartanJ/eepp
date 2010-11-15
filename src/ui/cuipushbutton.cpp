#include "cuipushbutton.hpp"

namespace EE { namespace UI {

cUIPushButton::cUIPushButton( const cUIPushButton::CreateParams& Params ) :
	cUIControlAnim( Params ),
	mIcon( NULL ),
	mTextBox( NULL ),
	mIconSpace( Params.IconHorizontalMargin )
{
	mType |= UI_TYPE_GET(UI_TYPE_PUSHBUTTON);

	cUIGfx::CreateParams GfxParams;
	GfxParams.Parent( this );
	GfxParams.Shape = Params.Icon;
	GfxParams.Flags = UI_AUTO_SIZE;
	mIcon = eeNew( cUIGfx, ( GfxParams ) );
	mIcon->Visible( true );
	mIcon->Enabled( false );

	Icon( Params.Icon );

	cUITextBox::CreateParams TxtParams = Params;
	TxtParams.Parent( this );
	TxtParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	mTextBox = eeNew( cUITextBox, ( TxtParams ) );
	mTextBox->Visible( true );
	mTextBox->Enabled( false );

	OnSizeChange();

	ApplyDefaultTheme();
}

void cUIPushButton::OnSizeChange() {
	mTextBox->Size( mSize.Width(), mSize.Height() );
	mTextBox->Pos( 0, 0 );
	mIcon->Pos( mIconSpace, 0 );
	mIcon->CenterVertical();
}

cUIPushButton::~cUIPushButton() {
}

void cUIPushButton::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "button" );

	AutoPadding();
}

void cUIPushButton::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		Padding( MakePadding( true, false, true, false ) );
	}
}

void cUIPushButton::Icon( cShape * Icon ) {
	mIcon->Shape( Icon );
	mIcon->Pos( mIconSpace, 0 );
	mIcon->CenterVertical();
}

cUIGfx * cUIPushButton::Icon() const {
	return mIcon;
}

void cUIPushButton::Text( const std::wstring& text ) {
	mTextBox->Text( text );
	OnSizeChange();
}

void cUIPushButton::Text( const std::string& text ) {
	mTextBox->Text( text );
	OnSizeChange();
}

void cUIPushButton::Padding( const eeRecti& padding ) {
	mTextBox->Padding( padding );
}

const eeRecti& cUIPushButton::Padding() const {
	return mTextBox->Padding();
}

void cUIPushButton::IconHorizontalMargin( Int32 margin ) {
	mIconSpace = margin;
	OnSizeChange();
}

const Int32& cUIPushButton::IconHorizontalMargin() const {
	return mIconSpace;
}

cUITextBox * cUIPushButton::TextBox() const {
	return mTextBox;
}

}}
