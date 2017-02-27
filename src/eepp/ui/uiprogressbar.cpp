#include <eepp/ui/uiprogressbar.hpp>

namespace EE { namespace UI {

UIProgressBar::UIProgressBar( const UIProgressBar::CreateParams& Params ) :
	UIComplexControl( Params ),
	mVerticalExpand( Params.VerticalExpand ),
	mSpeed( Params.MovementSpeed ),
	mFillerPadding( Params.FillerMargin ),
	mDisplayPercent( Params.DisplayPercent ),
	mProgress( 0.f ),
	mTotalSteps( 100.f ),
	mParallax( NULL )
{
	UITextBox::CreateParams TxtBoxParams = Params;

	TxtBoxParams.setParent( this );
	TxtBoxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	TxtBoxParams.setPosition( 0, 0 );

	mTextBox = eeNew( UITextBox, ( TxtBoxParams ) );
	mTextBox->setEnabled( false );

	updateTextBox();

	applyDefaultTheme();
}

UIProgressBar::UIProgressBar() :
	UIComplexControl(),
	mVerticalExpand( true ),
	mSpeed( 64.f, 0.f ),
	mFillerPadding(),
	mDisplayPercent( false ),
	mProgress( 0.f ),
	mTotalSteps( 100.f ),
	mParallax( NULL )
{
	setFlags( UI_AUTO_PADDING | UI_AUTO_SIZE );

	mTextBox = eeNew( UITextBox, () );
	mTextBox->setHorizontalAlign( UI_HALIGN_CENTER );
	mTextBox->setParent( this );
	mTextBox->setEnabled( false );

	updateTextBox();

	applyDefaultTheme();
}

UIProgressBar::~UIProgressBar() {
	eeSAFE_DELETE( mParallax );
}

Uint32 UIProgressBar::getType() const {
	return UI_TYPE_PROGRESSBAR;
}

bool UIProgressBar::isType( const Uint32& type ) const {
	return UIProgressBar::getType() == type ? true : UIComplexControl::isType( type );
}

void UIProgressBar::draw() {
	UIControlAnim::draw();

	if ( NULL != mParallax && 0.f != mAlpha ) {
		ColorA C( mParallax->getColor() );
		C.Alpha = (Uint8)mAlpha;

		Rectf fillerMargin = PixelDensity::dpToPx( mFillerPadding );

		mParallax->setColor( C );
		mParallax->setPosition( Vector2f( mScreenPos.x + fillerMargin.Left, mScreenPos.y + fillerMargin.Top ) );
		mParallax->draw();
	}
}

void UIProgressBar::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "progressbar" );

	if ( mFlags & UI_AUTO_SIZE ) {
		setSize( mSize.x, getSkinSize().getHeight() );
	}

	UISkin * tSkin = Theme->getByName( Theme->getAbbr() + "_progressbar_filler" );

	if ( tSkin ) {
		SubTexture * tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			eeSAFE_DELETE( mParallax );

			Float Height = (Float)PixelDensity::dpToPx( getSkinSize().getHeight() );

			if ( !mVerticalExpand )
				Height = (Float)tSubTexture->getSize().getHeight();

			if ( Height > mRealSize.getHeight() )
				Height = mRealSize.getHeight();

			if ( mFlags & UI_AUTO_PADDING ) {
				Float meH = (Float)getSkinSize().getHeight();
				Float otH = (Float)tSkin->getSize().getHeight();
				Float res = Math::roundUp( ( meH - otH ) * 0.5f );
				mFillerPadding = Rectf( res, res, res, res );
			}

			Rectf fillerPadding = PixelDensity::dpToPx( mFillerPadding );

			mParallax = eeNew( ScrollParallax, ( tSubTexture, Vector2f( mScreenPos.x + fillerPadding.Left, mScreenPos.y + fillerPadding.Top ), Sizef( ( ( mRealSize.getWidth() - fillerPadding.Left - fillerPadding.Right ) * mProgress ) / mTotalSteps, Height - fillerPadding.Top - fillerPadding.Bottom ), mSpeed ) );
		}
	}
}

Uint32 UIProgressBar::onValueChange() {
	UIControlAnim::onValueChange();

	onSizeChange();

	return 1;
}

void UIProgressBar::onSizeChange() {
	if ( NULL != mParallax ) {
		Float Height = (Float)mRealSize.getHeight();

		if ( !mVerticalExpand && mParallax->getSubTexture() )
			Height = (Float)mParallax->getSubTexture()->getSize().getHeight();

		if ( Height > mRealSize.getHeight() )
			Height = mRealSize.getHeight();

		Rectf fillerPadding = PixelDensity::dpToPx( mFillerPadding );

		mParallax->setSize( Sizef( ( ( mRealSize.getWidth() - fillerPadding.Left - fillerPadding.Right ) * mProgress ) / mTotalSteps, Height - fillerPadding.Top - fillerPadding.Bottom ) );
	}

	updateTextBox();
}

void UIProgressBar::setProgress( Float Val ) {
	mProgress = Val;

	onValueChange();
	updateTextBox();
}

const Float& UIProgressBar::getProgress() const {
	return mProgress;
}

void UIProgressBar::setTotalSteps( const Float& Steps ) {
	mTotalSteps = Steps;

	onSizeChange();
	updateTextBox();
}

const Float& UIProgressBar::getTotalSteps() const {
	return mTotalSteps;
}

void UIProgressBar::setMovementSpeed( const Vector2f& Speed ) {
	mSpeed = Speed;

	if ( NULL != mParallax )
		mParallax->setSpeed( PixelDensity::dpToPx( mSpeed ) );
}

const Vector2f& UIProgressBar::getMovementSpeed() const {
	return mSpeed;
}

void UIProgressBar::setVerticalExpand( const bool& VerticalExpand ) {
	if ( VerticalExpand != mVerticalExpand ) {
		mVerticalExpand = VerticalExpand;

		onSizeChange();
	}
}

const bool& UIProgressBar::getVerticalExpand() const {
	return mVerticalExpand;
}

void UIProgressBar::setFillerPadding( const Rectf& margin ) {
	mFillerPadding = margin;

	onPositionChange();
	onSizeChange();
}

const Rectf& UIProgressBar::getFillerPadding() const {
	return mFillerPadding;
}

void UIProgressBar::setDisplayPercent( const bool& DisplayPercent ) {
	mDisplayPercent = DisplayPercent;

	updateTextBox();
}

const bool& UIProgressBar::getDisplayPercent() const {
	return mDisplayPercent;
}

void UIProgressBar::updateTextBox() {
	mTextBox->setVisible( mDisplayPercent );
	mTextBox->setSize( mSize );
	mTextBox->setText( String::toStr( (Int32)( ( mProgress / mTotalSteps ) * 100.f ) ) + "%" );
}

UITextBox * UIProgressBar::getTextBox() const {
	return mTextBox;
}

void UIProgressBar::onAlphaChange() {
	UIControlAnim::onAlphaChange();
	
	mTextBox->setAlpha( mAlpha );
}

}}
