#include <eepp/ui/uipushbutton.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UIPushButton * UIPushButton::New() {
	return eeNew( UIPushButton, () );
}

UIPushButton::UIPushButton( const UIPushButton::CreateParams& Params ) :
	UIComplexControl( Params ),
	mFontStyleConfig( Params.FontStyleConfig ),
	mIcon( NULL ),
	mTextBox( NULL ),
	mIconSpace( Params.IconHorizontalMargin )
{
	UIGfx::CreateParams GfxParams;
	GfxParams.setParent( this );
	GfxParams.SubTexture = Params.Icon;

	if ( Params.IconMinSize.x != 0 && Params.IconMinSize.y != 0 ) {
		GfxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	} else {
		GfxParams.Flags = UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	}

	mIcon = eeNew( UIGfx, ( GfxParams ) );

	if ( Params.IconMinSize.x != 0 && Params.IconMinSize.y != 0 ) {
		mIcon->setSize( Params.IconMinSize );
	}

	mIcon->setVisible( true );
	mIcon->setEnabled( false );

	setIcon( Params.Icon );

	UITextBox::CreateParams TxtParams = Params;
	TxtParams.setParent( this );
	TxtParams.Flags 			= HAlignGet( Params.Flags ) | VAlignGet( Params.Flags );
	TxtParams.FontStyleConfig	= Params.FontStyleConfig;

	if ( TxtParams.Flags & UI_CLIP_ENABLE )
		TxtParams.Flags &= ~UI_CLIP_ENABLE;

	mTextBox = eeNew( UITextBox, ( TxtParams ) );
	mTextBox->setVisible( true );
	mTextBox->setEnabled( false );

	if ( Params.IconAutoMargin )
		mControlFlags |= UI_CTRL_FLAG_FREE_USE;

	onSizeChange();

	applyDefaultTheme();
}

UIPushButton::UIPushButton() :
	UIComplexControl(),
	mIcon( NULL ),
	mTextBox( NULL ),
	mIconSpace( 0 )
{
	mFontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();

	setFlags( UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	mIcon = eeNew( UIGfx, () );
	mIcon->setParent( this );
	mIcon->setFlags( UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER );
	mIcon->setVisible( true );
	mIcon->setEnabled( false );

	mTextBox = eeNew( UITextBox, () );
	mTextBox->setParent( this );
	mTextBox->setVisible( true );
	mTextBox->setEnabled( false );
	mTextBox->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	mControlFlags |= UI_CTRL_FLAG_FREE_USE; // IconAutoMargin

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

	mIcon->setPosition( mIconSpace, 0 );
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
		mIconSpace = RMargin.Left;
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

void UIPushButton::setIcon( SubTexture * Icon ) {
	mIcon->setSubTexture( Icon );
	onSizeChange();
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
	mIconSpace = margin;
	onSizeChange();
}

const Int32& UIPushButton::getIconHorizontalMargin() const {
	return mIconSpace;
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
		mTextBox->setFontColor( mFontStyleConfig.FontOverColor );
	} else {
		mTextBox->setFontColor( mFontStyleConfig.FontColor );
	}

	mTextBox->setAlpha( mAlpha );
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
	return mFontStyleConfig.FontColor;
}

void UIPushButton::setFontColor( const ColorA& color ) {
	mFontStyleConfig.FontColor = color;
	onStateChange();
}

const ColorA& UIPushButton::getFontOverColor() const {
	return mFontStyleConfig.FontOverColor;
}

void UIPushButton::setFontOverColor( const ColorA& color ) {
	mFontStyleConfig.FontOverColor = color;
	onStateChange();
}

const ColorA& UIPushButton::getFontShadowColor() const {
	return mTextBox->getFontShadowColor();
}

void UIPushButton::setFontShadowColor( const ColorA& color ) {
	mTextBox->setFontShadowColor( color );
}

FontStyleConfig UIPushButton::getFontStyleConfig() const {
	return mFontStyleConfig;
}

void UIPushButton::setFontStyleConfig(const FontStyleConfig & fontStyleConfig) {
	mFontStyleConfig = fontStyleConfig;
	mTextBox->setFontStyleConfig( fontStyleConfig );
	onStateChange();
}

}}
