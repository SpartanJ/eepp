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

	TxtBoxParams.setParent( this );
	TxtBoxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	TxtBoxParams.setPos( 0, 0 );

	mTextBox = eeNew( UITextBox, ( TxtBoxParams ) );
	mTextBox->enabled( false );

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

		mParallax->setColor( C );
		mParallax->setPosition( Vector2f( mScreenPos.x + mFillerMargin.Left, mScreenPos.y + mFillerMargin.Top ) );
		mParallax->draw();
	}
}

void UIProgressBar::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "progressbar" );

	if ( mFlags & UI_AUTO_SIZE ) {
		size( mSize.x, getSkinSize().getHeight() );
	}

	UISkin * tSkin = Theme->getByName( Theme->abbr() + "_progressbar_filler" );

	if ( tSkin ) {
		SubTexture * tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			eeSAFE_DELETE( mParallax );

			Float Height = (Float)getSkinSize().getHeight();

			if ( !mVerticalExpand )
				Height = (Float)tSubTexture->realSize().getHeight();

			if ( Height > mSize.getHeight() )
				Height = mSize.getHeight();

			mParallax = eeNew( ScrollParallax, ( tSubTexture, Vector2f( mScreenPos.x + mFillerMargin.Left, mScreenPos.y + mFillerMargin.Top ), Sizef( ( ( mSize.getWidth() - mFillerMargin.Left - mFillerMargin.Right ) * mProgress ) / mTotalSteps, Height - mFillerMargin.Top - mFillerMargin.Bottom ), mSpeed ) );
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
		Float Height = (Float)mSize.getHeight();

		if ( !mVerticalExpand && mParallax->getSubTexture() )
			Height = (Float)mParallax->getSubTexture()->realSize().getHeight();

		if ( Height > mSize.getHeight() )
			Height = mSize.getHeight();

		mParallax->setSize( Sizef( ( ( mSize.getWidth() - mFillerMargin.Left - mFillerMargin.Right ) * mProgress ) / mTotalSteps, Height - mFillerMargin.Top - mFillerMargin.Bottom ) );
	}

	updateTextBox();
}

void UIProgressBar::progress( Float Val ) {
	mProgress = Val;

	onValueChange();
	updateTextBox();
}

const Float& UIProgressBar::progress() const {
	return mProgress;
}

void UIProgressBar::totalSteps( const Float& Steps ) {
	mTotalSteps = Steps;

	onSizeChange();
	updateTextBox();
}

const Float& UIProgressBar::totalSteps() const {
	return mTotalSteps;
}

void UIProgressBar::movementSpeed( const Vector2f& Speed ) {
	mSpeed = Speed;

	if ( NULL != mParallax )
		mParallax->setSpeed( mSpeed );
}

const Vector2f& UIProgressBar::movementSpeed() const {
	return mSpeed;
}

void UIProgressBar::verticalExpand( const bool& VerticalExpand ) {
	if ( VerticalExpand != mVerticalExpand ) {
		mVerticalExpand = VerticalExpand;

		onSizeChange();
	}
}

const bool& UIProgressBar::verticalExpand() const {
	return mVerticalExpand;
}

void UIProgressBar::fillerMargin( const Rectf& margin ) {
	mFillerMargin = margin;

	onPositionChange();
	onSizeChange();
}

const Rectf& UIProgressBar::fillerMargin() const {
	return mFillerMargin;
}

void UIProgressBar::displayPercent( const bool& DisplayPercent ) {
	mDisplayPercent = DisplayPercent;

	updateTextBox();
}

const bool& UIProgressBar::displayPercent() const {
	return mDisplayPercent;
}

void UIProgressBar::updateTextBox() {
	mTextBox->visible( mDisplayPercent );
	mTextBox->size( mSize );
	mTextBox->text( String::toStr( (Int32)( ( mProgress / mTotalSteps ) * 100.f ) ) + "%" );
}

UITextBox * UIProgressBar::getTextBox() const {
	return mTextBox;
}

void UIProgressBar::onAlphaChange() {
	UIControlAnim::onAlphaChange();
	
	mTextBox->alpha( mAlpha );
}

}}
