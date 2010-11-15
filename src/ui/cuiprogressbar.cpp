#include "cuiprogressbar.hpp"

namespace EE { namespace UI {

cUIProgressBar::cUIProgressBar( const cUIProgressBar::CreateParams& Params ) :
	cUIControlAnim( Params ),
	mVerticalExpand( Params.VerticalExpand ),
	mSpeed( Params.MovementSpeed ),
	mFillerMargin( Params.FillerMargin ),
	mDisplayPercent( Params.DisplayPercent ),
	mProgress( 0.f ),
	mTotalSteps( 100.f ),
	mParallax( NULL )
{
	mType |= UI_TYPE_GET(UI_TYPE_PROGRESSBAR);

	cUITextBox::CreateParams TxtBoxParams = Params;

	TxtBoxParams.Parent( this );
	TxtBoxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	TxtBoxParams.PosSet( 0, 0 );

	mTextBox = eeNew( cUITextBox, ( TxtBoxParams ) );
	mTextBox->Enabled( false );

	UpdateTextBox();

	ApplyDefaultTheme();
}

cUIProgressBar::~cUIProgressBar() {
	eeSAFE_DELETE( mParallax );
}

void cUIProgressBar::Draw() {
	cUIControlAnim::Draw();

	if ( NULL != mParallax ) {
		mParallax->Position( eeVector2f( mScreenPos.x + mFillerMargin.Left, mScreenPos.y + mFillerMargin.Top ) );
		mParallax->Draw();
	}
}

void cUIProgressBar::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "progressbar" );

	cUISkin * tSkin = Theme->GetByName( Theme->Abbr() + "_progressbar_filler" );

	if ( tSkin ) {
		cShape * tShape = tSkin->GetShape( cUISkinState::StateNormal );

		if ( NULL != tShape ) {
			eeSAFE_DELETE( mParallax );

			eeFloat Height = (eeFloat)mSize.Height();

			if ( !mVerticalExpand )
				Height = (eeFloat)tShape->RealSize().Height();

			mParallax = eeNew( cScrollParallax, ( tShape, mScreenPos.x + mFillerMargin.Left, mScreenPos.y + mFillerMargin.Top, ( ( mSize.Width() - mFillerMargin.Left - mFillerMargin.Right ) * mProgress ) / mTotalSteps, Height - mFillerMargin.Top - mFillerMargin.Bottom, mSpeed ) );
		}
	}
}

Uint32 cUIProgressBar::OnValueChange() {
	cUIControlAnim::OnValueChange();

	OnSizeChange();

	return 1;
}

void cUIProgressBar::OnSizeChange() {
	if ( NULL != mParallax ) {
		eeFloat Height = (eeFloat)mSize.Height();

		if ( !mVerticalExpand && mParallax->Shape() )
			Height = (eeFloat)mParallax->Shape()->RealSize().Height();

		mParallax->Size( ( ( mSize.Width() - mFillerMargin.Left - mFillerMargin.Right ) * mProgress ) / mTotalSteps, Height - mFillerMargin.Top - mFillerMargin.Bottom );
	}

	UpdateTextBox();
}

void cUIProgressBar::Progress( eeFloat Val ) {
	mProgress = Val;

	OnValueChange();
	UpdateTextBox();
}

const eeFloat& cUIProgressBar::Progress() const {
	return mProgress;
}

void cUIProgressBar::TotalSteps( const eeFloat& Steps ) {
	mTotalSteps = Steps;

	OnSizeChange();
	UpdateTextBox();
}

const eeFloat& cUIProgressBar::TotalSteps() const {
	return mTotalSteps;
}

void cUIProgressBar::MovementSpeed( const eeVector2f& Speed ) {
	mSpeed = Speed;

	if ( NULL != mParallax )
		mParallax->Speed( mSpeed );
}

const eeVector2f& cUIProgressBar::MovementSpeed() const {
	return mSpeed;
}

void cUIProgressBar::VerticalExpand( const bool& VerticalExpand ) {
	if ( VerticalExpand != mVerticalExpand ) {
		mVerticalExpand = VerticalExpand;

		OnSizeChange();
	}
}

const bool& cUIProgressBar::VerticalExpand() const {
	return mVerticalExpand;
}

void cUIProgressBar::FillerMargin( const eeRectf& margin ) {
	mFillerMargin = margin;

	OnPosChange();
	OnSizeChange();
}

const eeRectf& cUIProgressBar::FillerMargin() const {
	return mFillerMargin;
}

void cUIProgressBar::DisplayPercent( const bool& DisplayPercent ) {
	mDisplayPercent = DisplayPercent;

	UpdateTextBox();
}

const bool& cUIProgressBar::DisplayPercent() const {
	return mDisplayPercent;
}

void cUIProgressBar::UpdateTextBox() {
	mTextBox->Visible( mDisplayPercent );
	mTextBox->Size( mSize );
	mTextBox->Text( toWStr( (Int32)( ( mProgress / mTotalSteps ) * 100.f ) ) + L"%" );
}

cUITextBox * cUIProgressBar::TextBox() const {
	return mTextBox;
}

}}
