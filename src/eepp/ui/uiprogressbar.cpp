#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>

namespace EE { namespace UI {

UIProgressBar *UIProgressBar::New() {
	return eeNew( UIProgressBar, () );
}

UIProgressBar::UIProgressBar() :
	UIWidget(),
	mProgress( 0.f ),
	mTotalSteps( 100.f ),
	mParallax( NULL )
{
	setFlags( UI_AUTO_PADDING | UI_AUTO_SIZE );

	UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

	mTextBox = UITextView::New();
	mTextBox->setHorizontalAlign( UI_HALIGN_CENTER );
	mTextBox->setParent( this );
	mTextBox->setEnabled( false );

	if ( NULL != Theme ) {
		mStyleConfig = Theme->getProgressBarStyleConfig();
		mTextBox->setFontStyleConfig( mStyleConfig );
	}

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
	return UIProgressBar::getType() == type ? true : UIWidget::isType( type );
}

void UIProgressBar::draw() {
	UIControlAnim::draw();

	if ( NULL != mParallax && 0.f != mAlpha ) {
		ColorA C( mParallax->getColor() );
		C.Alpha = (Uint8)mAlpha;

		Rectf fillerMargin = PixelDensity::dpToPx( mStyleConfig.FillerPadding );

		mParallax->setColor( C );
		mParallax->setPosition( Vector2f( mScreenPos.x + fillerMargin.Left, mScreenPos.y + fillerMargin.Top ) );
		mParallax->draw();
	}
}

void UIProgressBar::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );
	setThemeControl( Theme, "progressbar" );

	UISkin * tSkin = Theme->getByName( Theme->getAbbr() + "_progressbar_filler" );

	if ( tSkin ) {
		SubTexture * tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			eeSAFE_DELETE( mParallax );

			Float Height = (Float)PixelDensity::dpToPx( getSkinSize().getHeight() );

			if ( !mStyleConfig.VerticalExpand )
				Height = (Float)tSubTexture->getSize().getHeight();

			if ( Height > mRealSize.getHeight() )
				Height = mRealSize.getHeight();

			if ( mFlags & UI_AUTO_PADDING ) {
				Float meH = (Float)getSkinSize().getHeight();
				Float otH = (Float)tSkin->getSize().getHeight();
				Float res = Math::roundUp( ( meH - otH ) * 0.5f );
				mStyleConfig.FillerPadding = Rectf( res, res, res, res );
			}

			Rectf fillerPadding = PixelDensity::dpToPx( mStyleConfig.FillerPadding );

			mParallax = eeNew( ScrollParallax, ( tSubTexture, Vector2f( mScreenPos.x + fillerPadding.Left, mScreenPos.y + fillerPadding.Top ), Sizef( ( ( mRealSize.getWidth() - fillerPadding.Left - fillerPadding.Right ) * mProgress ) / mTotalSteps, Height - fillerPadding.Top - fillerPadding.Bottom ), mStyleConfig.MovementSpeed ) );
		}
	}

	onThemeLoaded();
}

void UIProgressBar::onThemeLoaded() {
	mMinControlSize.x = eemax( mMinControlSize.x, getSkinSize().getWidth() );
	mMinControlSize.y = eemax( mMinControlSize.y, getSkinSize().getHeight() );

	if ( mFlags & UI_AUTO_SIZE ) {
		setSize( mSize.x, getSkinSize().getHeight() );
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

		if ( !mStyleConfig.VerticalExpand && mParallax->getSubTexture() )
			Height = (Float)mParallax->getSubTexture()->getSize().getHeight();

		if ( Height > mRealSize.getHeight() )
			Height = mRealSize.getHeight();

		Rectf fillerPadding = PixelDensity::dpToPx( mStyleConfig.FillerPadding );

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
	mStyleConfig.MovementSpeed = Speed;

	if ( NULL != mParallax )
		mParallax->setSpeed( PixelDensity::dpToPx( Speed ) );
}

const Vector2f& UIProgressBar::getMovementSpeed() const {
	return mStyleConfig.MovementSpeed;
}

void UIProgressBar::setVerticalExpand( const bool& VerticalExpand ) {
	if ( VerticalExpand != mStyleConfig.VerticalExpand ) {
		mStyleConfig.VerticalExpand = VerticalExpand;

		onSizeChange();
	}
}

const bool& UIProgressBar::getVerticalExpand() const {
	return mStyleConfig.VerticalExpand;
}

void UIProgressBar::setFillerPadding( const Rectf& padding ) {
	mStyleConfig.FillerPadding = padding;

	onPositionChange();
	onSizeChange();
}

const Rectf& UIProgressBar::getFillerPadding() const {
	return mStyleConfig.FillerPadding;
}

void UIProgressBar::setDisplayPercent( const bool& DisplayPercent ) {
	mStyleConfig.DisplayPercent = DisplayPercent;

	updateTextBox();
}

const bool& UIProgressBar::getDisplayPercent() const {
	return mStyleConfig.DisplayPercent;
}

void UIProgressBar::updateTextBox() {
	mTextBox->setVisible( mStyleConfig.DisplayPercent );
	mTextBox->setText( String::toStr( (Int32)( ( mProgress / mTotalSteps ) * 100.f ) ) + "%" );
	mTextBox->center();
}

UITextView * UIProgressBar::getTextBox() const {
	return mTextBox;
}

void UIProgressBar::loadFromXmlNode(const pugi::xml_node & node) {
	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "src" == name ) {
			SubTexture * subTexture = GlobalTextureAtlas::instance()->getByName( ait->as_string() );

			if ( NULL != mParallax && NULL != subTexture )
				mParallax->setSubTexture( subTexture );
		} else if ( "totalsteps" == name ) {
			setTotalSteps( ait->as_float() );
		} else if ( "progress" == name ) {
			setProgress( ait->as_float() );
		} else if ( "verticalexpand" == name ) {
			setVerticalExpand( ait->as_bool() );
		} else if ( "displaypercent" == name ) {
			setDisplayPercent( ait->as_bool() );
		} else if ( "fillerpadding" == name ) {
			Float val = PixelDensity::toDpFromString( ait->as_string() );
			setFillerPadding( Rectf( val, val, val, val ) );
		} else if ( "fillerpaddingleft" == name ) {
			setFillerPadding( Rectf( PixelDensity::toDpFromString( ait->as_string() ), mStyleConfig.FillerPadding.Top, mStyleConfig.FillerPadding.Right, mStyleConfig.FillerPadding.Bottom ) );
		} else if ( "fillerpaddingright" == name ) {
			setFillerPadding( Rectf( mStyleConfig.FillerPadding.Left, mStyleConfig.FillerPadding.Top, PixelDensity::toDpFromString( ait->as_string() ), mStyleConfig.FillerPadding.Bottom ) );
		} else if ( "fillerpaddingtop" == name ) {
			setFillerPadding( Rectf( mStyleConfig.FillerPadding.Left, PixelDensity::toDpFromString( ait->as_string() ), mStyleConfig.FillerPadding.Right, mStyleConfig.FillerPadding.Bottom ) );
		} else if ( "fillerpaddingbottom" == name ) {
			setFillerPadding( Rectf( mStyleConfig.FillerPadding.Left, mStyleConfig.FillerPadding.Top, mStyleConfig.FillerPadding.Right, PixelDensity::toDpFromString( ait->as_string() ) ) );
		}
	}
}

void UIProgressBar::onAlphaChange() {
	UIControlAnim::onAlphaChange();
	
	mTextBox->setAlpha( mAlpha );
}

}}
