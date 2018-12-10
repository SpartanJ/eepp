#include <eepp/ui/uipushbutton.hpp>
#include <eepp/graphics/text.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>

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
	mIcon->setLayoutSizeRules( FIXED, FIXED );
	mIcon->setFlags( GfxFlags );
	mIcon->unsetFlags( UI_AUTO_SIZE );
	mIcon->setScaleType( UIScaleType::FitInside );

	if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
		mIcon->setSize( mStyleConfig.IconMinSize.asFloat() );
	}

	mIcon->setVisible( true );
	mIcon->setEnabled( false );

	mTextBox = UITextView::New();
	mTextBox->setLayoutSizeRules( WRAP_CONTENT, WRAP_CONTENT );
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

void UIPushButton::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() ) {
		setInternalHeight( getSkinSize().getHeight() );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() && ( 0 == mDpSize.getHeight() || mLayoutHeightRules == WRAP_CONTENT ) ) {
		Float h = eemax<Float>( PixelDensity::dpToPx( getSkinSize().getHeight() ), mTextBox->getTextHeight() );

		setInternalPixelsHeight( h + mRealPadding.Top + mRealPadding.Bottom );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) || mLayoutWidthRules == WRAP_CONTENT ) {
		Int32 txtW = NULL != mTextBox ? mTextBox->getTextWidth() : 0;

		Int32 minSize = txtW +
						( NULL != mIcon ? mIcon->getRealSize().getWidth() : 0 ) +
						PixelDensity::dpToPxI( mStyleConfig.IconHorizontalMargin ) + mRealPadding.Left + mRealPadding.Right +
						( NULL != getSkin() ? PixelDensity::dpToPxI( getSkin()->getBorderSize().Left + getSkin()->getBorderSize().Right ) : 0 );

		if ( minSize > mSize.getWidth() ) {
			setInternalPixelsWidth( minSize );
		}
	}
}

void UIPushButton::onPaddingChange() {
	onSizeChange();

	UIWidget::onPaddingChange();
}

void UIPushButton::onSizeChange() {
	onAutoSize();

	Rectf autoPadding;

	if ( mFlags & UI_AUTO_PADDING ) {
		autoPadding = makePadding( true, true, true, true );
	}

	if ( mRealPadding.Top > autoPadding.Top ) autoPadding.Top = mRealPadding.Top;
	if ( mRealPadding.Bottom > autoPadding.Bottom ) autoPadding.Bottom = mRealPadding.Bottom;
	if ( mRealPadding.Left > autoPadding.Left ) autoPadding.Left = mRealPadding.Left;
	if ( mRealPadding.Right > autoPadding.Right ) autoPadding.Right = mRealPadding.Right;

	mIcon->setPixelsPosition( autoPadding.Left + mStyleConfig.IconHorizontalMargin, 0 );
	mIcon->centerVertical();

	if ( NULL != mTextBox ) {
		Vector2f position;

		switch ( fontVAlignGet( getFlags() ) ) {
			case UI_VALIGN_CENTER:
				position.y = ( mSize.getHeight() - mTextBox->getRealSize().getHeight() ) / 2;
				break;
			case UI_VALIGN_BOTTOM:
				position.y = mSize.y - mTextBox->getRealSize().getHeight() - autoPadding.Bottom;
				break;
			case UI_VALIGN_TOP:
				position.y = autoPadding.Top;
				break;
		}

		switch ( fontHAlignGet( getFlags() ) ) {
			case UI_HALIGN_RIGHT:
				position.x = mSize.getWidth() - mTextBox->getRealSize().getWidth() - autoPadding.Right;
				break;
			case UI_HALIGN_CENTER:
				position.x = ( mSize.getWidth() - mTextBox->getRealSize().getWidth() ) / 2;

				if ( NULL != mIcon->getDrawable() ) {
					Uint32 iconPos = mIcon->getRealPosition().x + mIcon->getRealSize().getWidth();

					if ( iconPos >= position.x ) {
						Float px = PixelDensity::dpToPx(1);

						position.x = iconPos + px;
					}
				}

				break;
			case UI_HALIGN_LEFT:
				position.x = mIcon->getRealPosition().x + mIcon->getRealSize().getWidth();
				break;
		}

		mTextBox->setPixelsPosition( position );
	}

	if ( NULL != mTextBox && mTextBox->getText().empty() ) {
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

	onAutoSize();

	UIWidget::onThemeLoaded();
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

Uint32 UIPushButton::onKeyDown( const KeyEvent& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		NodeMessage Msg( this, NodeMessage::Click, EE_BUTTON_LMASK );
		messagePost( &Msg );
		onMouseClick( Vector2i(0,0), EE_BUTTON_LMASK );

		setSkinState( UISkinState::StateMouseDown );
	}

	return UIWidget::onKeyDown( Event );
}

Uint32 UIPushButton::onKeyUp( const KeyEvent& Event ) {
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

bool UIPushButton::setAttribute( const NodeAttribute& attribute ) {
	const std::string& name = attribute.getName();

	bool attributeSet = true;

	if ( "text" == name ) {
		if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
			setText( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( attribute.asString() ) );
	} else if ( "textovercolor" == name ) {
		setFontOverColor( Color::fromString( attribute.asString() ) );
	} else if ( "icon" == name ) {
		std::string val = attribute.asString();
		Drawable * icon = NULL;

		if ( NULL != mTheme && NULL != ( icon = mTheme->getIconByName( val ) ) ) {
			setIcon( icon );
		} else if ( NULL != ( icon = DrawableSearcher::searchByName( val ) ) ) {
			setIcon( icon );
		}
	} else {
		attributeSet = UIWidget::setAttribute( attribute );
	}

	if ( !attributeSet && ( String::startsWith( name, "text" ) || String::startsWith( name, "font" ) ) )
		mTextBox->setAttribute( attribute );

	return attributeSet;
}

}}
