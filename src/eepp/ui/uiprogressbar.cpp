#include <eepp/ui/uiprogressbar.hpp>

namespace EE { namespace UI {

UIProgressBar::UIProgressBar( const UIProgressBar::CreateParams& Params ) :
	UIComplexControl( Params ),
	mVerticalExpand( Params.VerticalExpand ),
	mSpeed( Params.MovementSpeed ),
	mFillerMargin( Params.FillerMargin ),
	mDisplayPercent( Params.DisplayPercent ),
	mProgress( 0.f ),
	mTotalSteps( 100.f ),
	mParallax( NULL )
{
	UITextBox::CreateParams TxtBoxParams = Params;

	TxtBoxParams.Parent( this );
	TxtBoxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	TxtBoxParams.PosSet( 0, 0 );

	mTextBox = eeNew( UITextBox, ( TxtBoxParams ) );
	mTextBox->Enabled( false );

	UpdateTextBox();

	ApplyDefaultTheme();
}

UIProgressBar::~UIProgressBar() {
	eeSAFE_DELETE( mParallax );
}

Uint32 UIProgressBar::Type() const {
	return UI_TYPE_PROGRESSBAR;
}

bool UIProgressBar::IsType( const Uint32& type ) const {
	return UIProgressBar::Type() == type ? true : UIComplexControl::IsType( type );
}

void UIProgressBar::Draw() {
	UIControlAnim::Draw();

	if ( NULL != mParallax && 0.f != mAlpha ) {
		ColorA C( mParallax->color() );
		C.Alpha = (Uint8)mAlpha;

		mParallax->color( C );
		mParallax->position( Vector2f( mScreenPos.x + mFillerMargin.Left, mScreenPos.y + mFillerMargin.Top ) );
		mParallax->draw();
	}
}

void UIProgressBar::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "progressbar" );

	if ( mFlags & UI_AUTO_SIZE ) {
		Size( mSize.x, GetSkinSize().height() );
	}

	UISkin * tSkin = Theme->getByName( Theme->Abbr() + "_progressbar_filler" );

	if ( tSkin ) {
		SubTexture * tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			eeSAFE_DELETE( mParallax );

			Float Height = (Float)GetSkinSize().height();

			if ( !mVerticalExpand )
				Height = (Float)tSubTexture->realSize().height();

			if ( Height > mSize.height() )
				Height = mSize.height();

			mParallax = eeNew( ScrollParallax, ( tSubTexture, Vector2f( mScreenPos.x + mFillerMargin.Left, mScreenPos.y + mFillerMargin.Top ), Sizef( ( ( mSize.width() - mFillerMargin.Left - mFillerMargin.Right ) * mProgress ) / mTotalSteps, Height - mFillerMargin.Top - mFillerMargin.Bottom ), mSpeed ) );
		}
	}
}

Uint32 UIProgressBar::OnValueChange() {
	UIControlAnim::OnValueChange();

	OnSizeChange();

	return 1;
}

void UIProgressBar::OnSizeChange() {
	if ( NULL != mParallax ) {
		Float Height = (Float)mSize.height();

		if ( !mVerticalExpand && mParallax->subTexture() )
			Height = (Float)mParallax->subTexture()->realSize().height();

		if ( Height > mSize.height() )
			Height = mSize.height();

		mParallax->size( Sizef( ( ( mSize.width() - mFillerMargin.Left - mFillerMargin.Right ) * mProgress ) / mTotalSteps, Height - mFillerMargin.Top - mFillerMargin.Bottom ) );
	}

	UpdateTextBox();
}

void UIProgressBar::Progress( Float Val ) {
	mProgress = Val;

	OnValueChange();
	UpdateTextBox();
}

const Float& UIProgressBar::Progress() const {
	return mProgress;
}

void UIProgressBar::TotalSteps( const Float& Steps ) {
	mTotalSteps = Steps;

	OnSizeChange();
	UpdateTextBox();
}

const Float& UIProgressBar::TotalSteps() const {
	return mTotalSteps;
}

void UIProgressBar::MovementSpeed( const Vector2f& Speed ) {
	mSpeed = Speed;

	if ( NULL != mParallax )
		mParallax->speed( mSpeed );
}

const Vector2f& UIProgressBar::MovementSpeed() const {
	return mSpeed;
}

void UIProgressBar::VerticalExpand( const bool& VerticalExpand ) {
	if ( VerticalExpand != mVerticalExpand ) {
		mVerticalExpand = VerticalExpand;

		OnSizeChange();
	}
}

const bool& UIProgressBar::VerticalExpand() const {
	return mVerticalExpand;
}

void UIProgressBar::FillerMargin( const Rectf& margin ) {
	mFillerMargin = margin;

	OnPosChange();
	OnSizeChange();
}

const Rectf& UIProgressBar::FillerMargin() const {
	return mFillerMargin;
}

void UIProgressBar::DisplayPercent( const bool& DisplayPercent ) {
	mDisplayPercent = DisplayPercent;

	UpdateTextBox();
}

const bool& UIProgressBar::DisplayPercent() const {
	return mDisplayPercent;
}

void UIProgressBar::UpdateTextBox() {
	mTextBox->Visible( mDisplayPercent );
	mTextBox->Size( mSize );
	mTextBox->Text( String::toStr( (Int32)( ( mProgress / mTotalSteps ) * 100.f ) ) + "%" );
}

UITextBox * UIProgressBar::TextBox() const {
	return mTextBox;
}

void UIProgressBar::OnAlphaChange() {
	UIControlAnim::OnAlphaChange();
	
	mTextBox->Alpha( mAlpha );
}

}}
