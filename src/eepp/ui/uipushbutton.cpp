#include <eepp/ui/uipushbutton.hpp>
#include <eepp/graphics/text.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIPushButton * UIPushButton::New() {
	return eeNew( UIPushButton, () );
}

UIPushButton::UIPushButton() :
	UIWidget(),
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

	mIcon = UIImage::New();
	mIcon->setParent( this );
	mIcon->setFlags( GfxFlags );
	mIcon->unsetFlags( UI_AUTO_SIZE );

	if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
		mIcon->setSize( mStyleConfig.IconMinSize.asFloat() );
	}

	mIcon->setVisible( true );
	mIcon->setEnabled( false );

	mTextBox = UITextView::New();
	mTextBox->setLayoutSizeRules( FIXED, FIXED );
	mTextBox->setParent( this );
	mTextBox->setVisible( true );
	mTextBox->setEnabled( false );
	mTextBox->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	if ( mStyleConfig.IconAutoMargin )
		mNodeFlags |= NODE_FLAG_FREE_USE;

	onSizeChange();

	applyDefaultTheme();
}

UIPushButton::~UIPushButton() {
}

Uint32 UIPushButton::getType() const {
	return UI_TYPE_PUSHBUTTON;
}

bool UIPushButton::isType( const Uint32& type ) const {
	return UIPushButton::getType() == type ? true : UIWidget::isType( type );
}

void UIPushButton::onSizeChange() {	
	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() && 0 == mDpSize.getHeight() ) {
		setInternalHeight( getSkinSize().getHeight() );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) ) {
		Int32 txtW = NULL != mTextBox ? PixelDensity::pxToDpI( mTextBox->getTextWidth() ) : 0;
		Int32 minSize = txtW + ( NULL != mIcon ? mIcon->getSize().getWidth() : 0 )
						+ mStyleConfig.IconHorizontalMargin + mTextBox->getPadding().Left + mTextBox->getPadding().Right +
						(  NULL != getSkin() ? getSkin()->getBorderSize().Left + getSkin()->getBorderSize().Right : 0 );

		if ( minSize > mDpSize.getWidth() ) {
			setInternalWidth( minSize );
		}
	}

	if ( NULL != mTextBox ) {
		mTextBox->setSize( mDpSize );
		mTextBox->setPosition( 0, 0 );
	}

	mIcon->setPosition( mStyleConfig.IconHorizontalMargin, 0 );
	mIcon->centerVertical();

	if ( NULL != mTextBox ) {
		switch ( fontHAlignGet( getFlags() ) ) {
			case UI_HALIGN_LEFT:
				mTextBox->setPosition( mIcon->getPosition().x + mIcon->getSize().getWidth(), 0 );
				mTextBox->setSize( mDpSize.getWidth() - mIcon->getPosition().x - mIcon->getSize().getWidth(), mDpSize.getHeight() );
				break;
			case UI_HALIGN_CENTER:
				if ( NULL != mIcon->getDrawable() ) {
					Uint32 iconPos = mIcon->getPosition().x + mIcon->getSize().getWidth();
					Uint32 txtOff = mTextBox->getPosition().x + mTextBox->getAlignOffset().x;

					if ( iconPos >= txtOff) {
						Float px = PixelDensity::dpToPx(1);

						mTextBox->setPosition( iconPos + px, mTextBox->getPosition().y );

						mTextBox->setSize( mDpSize.getWidth() - mIcon->getPosition().x - mIcon->getSize().getWidth() - px, mDpSize.getHeight() );
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
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "button" );

	onThemeLoaded();
}

void UIPushButton::onThemeLoaded() {
	if ( NULL != mTextBox && NULL == mTextBox->getFont() && NULL != mSkinState && NULL != mSkinState->getSkin() && NULL != mSkinState->getSkin()->getTheme() && NULL != mSkinState->getSkin()->getTheme()->getFontStyleConfig().getFont() )
		mTextBox->setFont( mSkinState->getSkin()->getTheme()->getFontStyleConfig().getFont() );

	if ( mNodeFlags & NODE_FLAG_FREE_USE ) {
		Rectf RMargin = makePadding( true, false, false, false, true );
		mStyleConfig.IconHorizontalMargin = RMargin.Left;
	}

	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() ) {
		setInternalHeight( getSkinSize().getHeight() );
	}

	autoPadding();

	onSizeChange();

	UIWidget::onThemeLoaded();
}

void UIPushButton::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mTextBox->setPadding( makePadding( true, false, true, false ) );
	}
}

UIPushButton * UIPushButton::setIcon( Drawable * Icon ) {
	mIcon->setDrawable( Icon );
	onSizeChange();
	return this;
}

UIImage * UIPushButton::getIcon() const {
	return mIcon;
}

UIPushButton * UIPushButton::setText( const String& text ) {
	mTextBox->setText( text );
	onSizeChange();
	return this;
}

const String& UIPushButton::getText() {
	return mTextBox->getText();
}

void UIPushButton::setPadding( const Rectf& padding ) {
	mTextBox->setPadding( padding );
}

const Rectf& UIPushButton::getPadding() const {
	return mTextBox->getPadding();
}

void UIPushButton::setIconHorizontalMargin( Int32 margin ) {
	mStyleConfig.IconHorizontalMargin = margin;
	onSizeChange();
}

const Int32& UIPushButton::getIconHorizontalMargin() const {
	return mStyleConfig.IconHorizontalMargin;
}

UITextView * UIPushButton::getTextBox() const {
	return mTextBox;
}

void UIPushButton::setFont(Font * font) {
	mTextBox->setFont( font );
}

Font * UIPushButton::getFont() {
	return mTextBox->getFont();
}

void UIPushButton::onAlphaChange() {
	UIWidget::onAlphaChange();

	mIcon->setAlpha( mAlpha );
	mTextBox->setAlpha( mAlpha );
}

void UIPushButton::onStateChange() {
	UIWidget::onStateChange();

	if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
		mTextBox->setFontColor( mStyleConfig.FontOverColor );
	} else {
		mTextBox->setFontColor( mStyleConfig.FontColor );
	}

	mTextBox->setAlpha( mAlpha );
}

void UIPushButton::onAlignChange() {
	UIWidget::onAlignChange();

	mTextBox->setHorizontalAlign( getHorizontalAlign() );
	mTextBox->setVerticalAlign( getVerticalAlign() );
}

Uint32 UIPushButton::onKeyDown( const UIEventKey& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		UIMessage Msg( this, UIMessage::Click, EE_BUTTON_LMASK );
		messagePost( &Msg );
		onMouseClick( Vector2i(0,0), EE_BUTTON_LMASK );

		setSkinState( UISkinState::StateMouseDown );
	}

	return UIWidget::onKeyDown( Event );
}

Uint32 UIPushButton::onKeyUp( const UIEventKey& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		setPrevSkinState();
	}

	return UIWidget::onKeyUp( Event );
}
const Color& UIPushButton::getFontColor() const {
	return mStyleConfig.FontColor;
}

void UIPushButton::setFontColor( const Color& color ) {
	mStyleConfig.FontColor = color;
	onStateChange();
}

const Color& UIPushButton::getFontOverColor() const {
	return mStyleConfig.FontOverColor;
}

void UIPushButton::setFontOverColor( const Color& color ) {
	mStyleConfig.FontOverColor = color;
	onStateChange();
}

const Color& UIPushButton::getFontShadowColor() const {
	return mTextBox->getFontShadowColor();
}

void UIPushButton::setFontShadowColor( const Color& color ) {
	mTextBox->setFontShadowColor( color );
}

Uint32 UIPushButton::getCharacterSize() {
	return mTextBox->getCharacterSize();
}

void UIPushButton::setCharacterSize(const Uint32 & characterSize) {
	mTextBox->setCharacterSize( characterSize );
	onSizeChange();
}

const Uint32 &UIPushButton::getFontStyle() const {
	return mStyleConfig.Style;
}

const Float &UIPushButton::getOutlineThickness() const {
	return mStyleConfig.OutlineThickness;
}

UIPushButton * UIPushButton::setOutlineThickness( const Float & outlineThickness ) {
	if ( mStyleConfig.OutlineThickness != outlineThickness ) {
		mTextBox->setOutlineThickness( outlineThickness );
		mStyleConfig.OutlineThickness = outlineThickness;
	}

	return this;
}

const Color &UIPushButton::getOutlineColor() const {
	return mStyleConfig.OutlineColor;
}

UIPushButton * UIPushButton::setOutlineColor(const Color & outlineColor) {
	if ( mStyleConfig.OutlineColor != outlineColor ) {
		mTextBox->setOutlineColor( outlineColor );
		mStyleConfig.OutlineColor = outlineColor;
	}

	return this;
}

UIPushButton * UIPushButton::setFontStyle(const Uint32 & fontStyle) {
	if ( mStyleConfig.Style != fontStyle ) {
		mTextBox->setFontStyle( fontStyle );
		mStyleConfig.Style = fontStyle;
	}

	return this;
}

UITooltipStyleConfig UIPushButton::getStyleConfig() const {
	return mStyleConfig;
}

void UIPushButton::setStyleConfig(const UIPushButtonStyleConfig & styleConfig) {
	mStyleConfig = styleConfig;
	mTextBox->setFontStyleConfig( styleConfig );

	if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
		Sizef minSize( eemax( mDpSize.x, (Float)mStyleConfig.IconMinSize.x ), eemax( mDpSize.y, (Float)mStyleConfig.IconMinSize.y ) );

		if ( minSize != mDpSize ) {
			mIcon->setSize( minSize );
			onSizeChange();
		}
	}

	onStateChange();
}

void UIPushButton::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	mTextBox->loadFromXmlNode( node );
	mTextBox->setLayoutSizeRules( FIXED, FIXED );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "text" == name ) {
			setText( UIManager::instance()->getTranslatorString( ait->as_string() ) );
		} else if ( "textovercolor" == name ) {
			setFontOverColor( Color::fromString( ait->as_string() ) );
		} else if ( "icon" == name ) {
			std::string val = ait->as_string();
			Drawable * icon = NULL;

			if ( NULL != mTheme && NULL != ( icon = mTheme->getIconByName( val ) ) ) {
				setIcon( icon );
			} else if ( NULL != ( icon = GlobalTextureAtlas::instance()->getByName( val ) ) ) {
				setIcon( icon );
			}
		}
	}

	endPropertiesTransaction();
}

}}
