#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UITooltip::UITooltip( UITooltip::CreateParams& Params, UIControl * TooltipOf ) :
	UIControlAnim( Params ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mAlignOffset( 0.f, 0.f ),
	mPadding( Params.Padding ),
	mTooltipTime( Time::Zero ),
	mTooltipOf( TooltipOf )
{
	mTextCache = eeNew( TextCache, () );
	mTextCache->setFont( Params.Font );
	mTextCache->setColor( mFontColor );
	mTextCache->setShadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != UIThemeManager::instance()->getDefaultFont() )
			mTextCache->setFont( UIThemeManager::instance()->getDefaultFont() );
		else
			eePRINTL( "UITooltip::UITextBox : Created a UI TextBox without a defined font." );
	}

	autoPadding();

	if ( Params.ParentCtrl != UIManager::instance()->mainControl() )
		setParent( UIManager::instance()->mainControl() );

	applyDefaultTheme();
}

UITooltip::~UITooltip() {
	eeSAFE_DELETE( mTextCache );

	if ( NULL != mTooltipOf && mTooltipOf->isComplex() ) {
		reinterpret_cast<UIComplexControl*>( mTooltipOf )->tooltipRemove();
	}
}

Uint32 UITooltip::getType() const {
	return UI_TYPE_TOOLTIP;
}

bool UITooltip::isType( const Uint32& type ) const {
	return UITooltip::getType() == type ? true : UIControlAnim::isType( type );
}

void UITooltip::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "tooltip" );

	autoPadding();

	if ( NULL == mTextCache->getFont() && NULL != Theme->getFont() ) {
		mTextCache->setFont( Theme->getFont() );
	}
}

void UITooltip::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = makePadding( true, true, true, true );
	}
}

void UITooltip::show() {
	if ( !isVisible() || 0 == mAlpha ) {
		toFront();

		setVisible( true );

		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			startAlphaAnim( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->getControlsFadeInTime() );
		}
	}
}

void UITooltip::hide() {
	if ( isVisible() ) {
		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			disableFadeOut( UIThemeManager::instance()->getControlsFadeOutTime() );
		} else {
			setVisible( false );
		}
	}
}

void UITooltip::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIControlAnim::draw();

		if ( mTextCache->getTextWidth() ) {
			mTextCache->setFlags( getFlags() );
			mTextCache->draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, Vector2f::One, 0.f, getBlendMode() );
		}
	}
}

Graphics::Font * UITooltip::getFont() const {
	return mTextCache->getFont();
}

void UITooltip::setFont( Graphics::Font * font ) {
	if ( mTextCache->getFont() != font ) {
		mTextCache->setFont( font );
		autoPadding();
		autoSize();
		autoAlign();
		onFontChanged();
	}
}

const String& UITooltip::getText() {
	return mTextCache->getText();
}

void UITooltip::setText( const String& text ) {
	mTextCache->setText( text );
	autoPadding();
	autoSize();
	autoAlign();
	onTextChanged();
}

const ColorA& UITooltip::getColor() const {
	return mFontColor;
}

void UITooltip::setColor( const ColorA& color ) {
	mFontColor = color;
	setAlpha( color.a() );
}

const ColorA& UITooltip::getShadowColor() const {
	return mFontShadowColor;
}

void UITooltip::setShadowColor( const ColorA& color ) {
	mFontShadowColor = color;
	setAlpha( color.a() );
	mTextCache->setShadowColor( mFontColor );
}

void UITooltip::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->setColor( mFontColor );
}

void UITooltip::autoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.setWidth( (int)mTextCache->getTextWidth() + mPadding.Left + mPadding.Right );
		mSize.setHeight( (int)mTextCache->getTextHeight() + mPadding.Top + mPadding.Bottom );
	}
}

void UITooltip::autoAlign() {
	Uint32 Width	= mSize.getWidth()		- mPadding.Left - mPadding.Right;
	Uint32 Height	= mSize.getHeight()	- mPadding.Top	- mPadding.Bottom;

	switch ( FontHAlignGet( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = mPadding.Left + (Float)( (Int32)( Width - mTextCache->getTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (Float)Width - (Float)mTextCache->getTextWidth() ) - mPadding.Right;
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = mPadding.Left;
			break;
	}

	switch ( FontVAlignGet( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = mPadding.Top + (Float)( ( (Int32)( Height - mTextCache->getTextHeight() ) ) / 2 );
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (Float)Height - (Float)mTextCache->getTextHeight() ) - mPadding.Bottom;
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = mPadding.Top;
			break;
	}
}

void UITooltip::onSizeChange() {
	autoPadding();
	autoSize();
	autoAlign();

	UIControlAnim::onSizeChange();

	mTextCache->cacheWidth();
}

void UITooltip::onTextChanged() {
	sendCommonEvent( UIEvent::EventOnTextChanged );
}

void UITooltip::onFontChanged() {
	sendCommonEvent( UIEvent::EventOnFontChanged );
}

void UITooltip::setPadding( const Recti& padding ) {
	mPadding = padding;
}

const Recti& UITooltip::getPadding() const {
	return mPadding;
}

TextCache * UITooltip::getTextCache() {
	return mTextCache;
}

Float UITooltip::getTextWidth() {
	return mTextCache->getTextWidth();
}

Float UITooltip::getTextHeight() {
	return mTextCache->getTextHeight();
}

const int& UITooltip::getNumLines() const {
	return mTextCache->getNumLines();
}

const Vector2f& UITooltip::getAlignOffset() const {
	return mAlignOffset;
}

void UITooltip::setTooltipTime( const Time& Time ) {
	mTooltipTime = Time;
}

void UITooltip::addTooltipTime( const Time& Time ) {
	mTooltipTime += Time;
}

const Time& UITooltip::getTooltipTime() const {
	return mTooltipTime;
}

}}
