#include <eepp/ui/uipushbutton.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UIPushButton * UIPushButton::New() {
	return eeNew( UIPushButton, () );
}

UIPushButton::UIPushButton() :
	UIComplexControl(),
	mIcon( NULL ),
	mTextBox( NULL )
{
	UITheme * theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != theme ) {
		mStyleConfig = theme->getPushButtonStyleConfig();
	}

	mFlags |= ( UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	Uint32 GfxFlags;

	if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
		GfxFlags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	} else {
		GfxFlags = UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	}

	mIcon = UIGfx::New();
	mIcon->setParent( this );
	mIcon->setFlags( GfxFlags );
	mIcon->unsetFlags( UI_AUTO_SIZE | UI_FIT_TO_CONTROL );

	if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
		mIcon->setSize( mStyleConfig.IconMinSize );
	}

	mIcon->setVisible( true );
	mIcon->setEnabled( false );

	mTextBox = UITextBox::New();
	mTextBox->setParent( this );
	mTextBox->setVisible( true );
	mTextBox->setEnabled( false );
	mTextBox->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	if ( mStyleConfig.IconAutoMargin )
		mControlFlags |= UI_CTRL_FLAG_FREE_USE;

	onSizeChange();

	applyDefaultTheme();
}

UIPushButton::~UIPushButton() {
}

Uint32 UIPushButton::getType() const {
	return UI_TYPE_PUSHBUTTON;
}

bool UIPushButton::isType( const Uint32& type ) const {
	return UIPushButton::getType() == type ? true : UIComplexControl::isType( type );
}

void UIPushButton::onSizeChange() {	
	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() && 0 == mSize.getHeight() ) {
		setInternalHeight( getSkinSize().getHeight() );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) ) {
		Int32 txtW = NULL != mTextBox ? PixelDensity::pxToDpI( mTextBox->getTextCache()->getTextWidth() ) : 0;
		Int32 minSize = txtW + ( NULL != mIcon ? mIcon->getSize().getWidth() : 0 ) + getSkinSize().getWidth();

		if ( minSize > mSize.getWidth() ) {
			setInternalWidth( minSize );
		}
	}

	if ( NULL != mTextBox ) {
		mTextBox->setSize( mSize );
		mTextBox->setPosition( 0, 0 );
	}

	mIcon->setPosition( mStyleConfig.IconHorizontalMargin, 0 );
	mIcon->centerVertical();

	if ( NULL != mTextBox ) {
		switch ( fontHAlignGet( getFlags() ) ) {
			case UI_HALIGN_LEFT:
				mTextBox->setPosition( mIcon->getPosition().x + mIcon->getSize().getWidth(), 0 );
				mTextBox->setSize( mSize.getWidth() - mIcon->getPosition().x - mIcon->getSize().getWidth(), mSize.getHeight() );
				break;
			case UI_HALIGN_CENTER:
				if ( NULL != mIcon->getSubTexture() ) {
					Uint32 iconPos = mIcon->getPosition().x + mIcon->getSize().getWidth();
					Uint32 txtOff = mTextBox->getPosition().x + mTextBox->getAlignOffset().x;

					if ( iconPos >= txtOff) {
						Int32 px = PixelDensity::dpToPxI(1);

						mTextBox->setPosition( iconPos + px, mTextBox->getPosition().y );

						mTextBox->setSize( mSize.getWidth() - mIcon->getPosition().x - mIcon->getSize().getWidth() - px, mTextBox->getSize().getHeight() );
					}
				}

				break;
		}
	}

	if ( NULL != mTextBox && 0 == mTextBox->getText().size() ) {
		mIcon->center();
	}
}

void UIPushButton::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "button" );

	doAfterSetTheme();
}

void UIPushButton::doAfterSetTheme() {
	if ( NULL != mTextBox && NULL == mTextBox->getFont() && NULL != mSkinState && NULL != mSkinState->getSkin() && NULL != mSkinState->getSkin()->getTheme() && NULL != mSkinState->getSkin()->getTheme()->getFontStyleConfig().getFont() )
		mTextBox->setFont( mSkinState->getSkin()->getTheme()->getFontStyleConfig().getFont() );

	if ( mControlFlags & UI_CTRL_FLAG_FREE_USE ) {
		Recti RMargin = makePadding( true, false, false, false, true );
		mStyleConfig.IconHorizontalMargin = RMargin.Left;
	}

	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() ) {
		setInternalHeight( getSkin()->getSize().getHeight() );
	}

	autoPadding();

	onSizeChange();
}

void UIPushButton::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mTextBox->setPadding( makePadding( true, false, true, false ) );
	}
}

UIPushButton * UIPushButton::setIcon( SubTexture * Icon ) {
	mIcon->setSubTexture( Icon );
	onSizeChange();
	return this;
}

UIGfx * UIPushButton::getIcon() const {
	return mIcon;
}

void UIPushButton::setText( const String& text ) {
	mTextBox->setText( text );
	onSizeChange();
}

const String& UIPushButton::getText() {
	return mTextBox->getText();
}

void UIPushButton::setPadding( const Recti& padding ) {
	mTextBox->setPadding( padding );
}

const Recti& UIPushButton::getPadding() const {
	return mTextBox->getPadding();
}

void UIPushButton::setIconHorizontalMargin( Int32 margin ) {
	mStyleConfig.IconHorizontalMargin = margin;
	onSizeChange();
}

const Int32& UIPushButton::getIconHorizontalMargin() const {
	return mStyleConfig.IconHorizontalMargin;
}

UITextBox * UIPushButton::getTextBox() const {
	return mTextBox;
}

void UIPushButton::setFont(Font * font) {
	mTextBox->setFont( font );
}

Font *UIPushButton::getFont() {
	return mTextBox->getFont();
}

void UIPushButton::onAlphaChange() {
	UIControlAnim::onAlphaChange();

	mIcon->setAlpha( mAlpha );
	mTextBox->setAlpha( mAlpha );
}

void UIPushButton::onStateChange() {
	if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
		mTextBox->setFontColor( mStyleConfig.FontOverColor );
	} else {
		mTextBox->setFontColor( mStyleConfig.FontColor );
	}

	mTextBox->setAlpha( mAlpha );
}

void UIPushButton::onAlignChange() {
	mTextBox->setHorizontalAlign( getHorizontalAlign() );
	mTextBox->setVerticalAlign( getVerticalAlign() );
}

Uint32 UIPushButton::onKeyDown( const UIEventKey& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		UIMessage Msg( this, UIMessage::MsgClick, EE_BUTTON_LMASK );
		messagePost( &Msg );
		onMouseClick( Vector2i(0,0), EE_BUTTON_LMASK );

		setSkinState( UISkinState::StateMouseDown );
	}

	return UIComplexControl::onKeyDown( Event );
}

Uint32 UIPushButton::onKeyUp( const UIEventKey& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		setPrevSkinState();
	}

	return UIComplexControl::onKeyUp( Event );
}
const ColorA& UIPushButton::getFontColor() const {
	return mStyleConfig.FontColor;
}

void UIPushButton::setFontColor( const ColorA& color ) {
	mStyleConfig.FontColor = color;
	onStateChange();
}

const ColorA& UIPushButton::getFontOverColor() const {
	return mStyleConfig.FontOverColor;
}

void UIPushButton::setFontOverColor( const ColorA& color ) {
	mStyleConfig.FontOverColor = color;
	onStateChange();
}

const ColorA& UIPushButton::getFontShadowColor() const {
	return mTextBox->getFontShadowColor();
}

void UIPushButton::setFontShadowColor( const ColorA& color ) {
	mTextBox->setFontShadowColor( color );
}

TooltipStyleConfig UIPushButton::getStyleConfig() const {
	return mStyleConfig;
}

void UIPushButton::setStyleConfig(const PushButtonStyleConfig & styleConfig) {
	mStyleConfig = styleConfig;
	mTextBox->setFontStyleConfig( styleConfig );

	if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
		Sizei minSize( eemax( mSize.x, mStyleConfig.IconMinSize.x ), eemax( mSize.y, mStyleConfig.IconMinSize.y ) );

		if ( minSize != mSize ) {
			mIcon->setSize( minSize );
			onSizeChange();
		}
	}

	onStateChange();
}

}}
