#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uimanager.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>

namespace EE { namespace UI {

UIProgressBar *UIProgressBar::New() {
	return eeNew( UIProgressBar, () );
}

UIProgressBar::UIProgressBar() :
	UIWidget(),
	mProgress( 0.f ),
	mTotalSteps( 100.f ),
	mFillerSkin( NULL )
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
}

Uint32 UIProgressBar::getType() const {
	return UI_TYPE_PROGRESSBAR;
}

bool UIProgressBar::isType( const Uint32& type ) const {
	return UIProgressBar::getType() == type ? true : UIWidget::isType( type );
}

void UIProgressBar::draw() {
	UINode::draw();

	if ( NULL == mFillerSkin )
		return;

	Rectf fillerPadding = PixelDensity::dpToPx( mStyleConfig.FillerPadding );

	Float Height = (Float)mRealSize.getHeight();

	if ( !mStyleConfig.VerticalExpand )
		Height = (Float)mFillerSkin->getSize().getHeight();

	if ( Height > mRealSize.getHeight() )
		Height = mRealSize.getHeight();

	Sizef fSize( ( ( mRealSize.getWidth() - fillerPadding.Left - fillerPadding.Right ) * mProgress ) / mTotalSteps, Height - fillerPadding.Top - fillerPadding.Bottom );
	Sizei rSize( PixelDensity::dpToPxI( mFillerSkin->getSize() ) );
	Sizei numTiles( (Int32)eeceil( (Float)fSize.getWidth() / (Float)rSize.getWidth() + 2 ),
				(Int32)eeceil( (Float)fSize.getHeight() / (Float)rSize.getHeight() ) + 2 );

	UIManager::instance()->clipSmartEnable( this, mScreenPos.x + fillerPadding.Left, mScreenPos.y + fillerPadding.Top, fSize.getWidth(), fSize.getHeight() );

	for ( int y = -1; y < numTiles.y; y++ ) {
		for ( int x = -1; x < numTiles.x; x++ ) {
			mFillerSkin->draw( (Int32)mOffset.x + mScreenPos.x + fillerPadding.Left + x * rSize.getWidth(), mOffset.y + mScreenPos.y + fillerPadding.Top + y * rSize.getHeight(), rSize.getWidth(), rSize.getHeight(), 255, UISkinState::StateNormal );
		}
	}

	UIManager::instance()->clipSmartDisable( this );
}

void UIProgressBar::update() {
	UINode::update();

	if ( NULL == mFillerSkin )
		return;

	Vector2f offset( mOffset );

	mOffset += mStyleConfig.MovementSpeed * (Float)( getElapsed().asSeconds() );

	Sizei rSize( PixelDensity::dpToPxI( mFillerSkin->getSize() ) );

	if ( mOffset.x > rSize.getWidth() || mOffset.x < -rSize.getWidth() )
		mOffset.x = 0.f;

	if ( mOffset.y > rSize.getHeight() || mOffset.y < -rSize.getHeight() )
		mOffset.y = 0.f;

	if ( offset != mOffset )
		invalidateDraw();
}

void UIProgressBar::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "progressbar" );

	mFillerSkin = Theme->getSkin( "progressbar_filler" );

	if ( mFillerSkin ) {
		if ( mFlags & UI_AUTO_PADDING ) {
			Float meH = (Float)getSkinSize().getHeight();
			Float otH = (Float)mFillerSkin->getSize().getHeight();
			Float res = Math::roundUp( ( meH - otH ) * 0.5f );
			mStyleConfig.FillerPadding = Rectf( res, res, res, res );
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

	UIWidget::onThemeLoaded();
}

Uint32 UIProgressBar::onValueChange() {
	UINode::onValueChange();

	onSizeChange();

	return 1;
}

void UIProgressBar::onSizeChange() {
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
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "totalsteps" == name ) {
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

	endPropertiesTransaction();
}

void UIProgressBar::onAlphaChange() {
	UINode::onAlphaChange();
	
	mTextBox->setAlpha( mAlpha );
}

}}
