#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>

namespace EE { namespace UI {

UITextView * UITextView::New() {
	return eeNew( UITextView, () );
}

UITextView * UITextView::NewWithTag( const std::string& tag ) {
	return eeNew( UITextView, ( tag ) );
}

UITextView::UITextView( const std::string& tag ) :
	UIWidget( tag ),
	mRealAlignOffset( 0.f, 0.f ),
	mSelCurInit( -1 ),
	mSelCurEnd( -1 ),
	mLastSelCurInit( -1 ),
	mLastSelCurEnd( -1 ),
	mFontLineCenter( 0 ),
	mSelecting( false )
{
	mTextCache = Text::New();

	UITheme * theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != theme ) {
		mFontStyleConfig.Font = theme->getDefaultFont();
	}

	if ( NULL == getFont() ) {
		if ( NULL != UIThemeManager::instance()->getDefaultFont() )
			setFont( UIThemeManager::instance()->getDefaultFont() );
		else
			eePRINTL( "UITextView::UITextView : Created a without a defined font." );
	}

	alignFix();

	applyDefaultTheme();
}

UITextView::UITextView() :
	UITextView( "textview" )
{}

UITextView::~UITextView() {
	eeSAFE_DELETE( mTextCache );
}

Uint32 UITextView::getType() const {
	return UI_TYPE_TEXTVIEW;
}

bool UITextView::isType( const Uint32& type ) const {
	return UITextView::getType() == type ? true : UIWidget::isType( type );
}

void UITextView::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UINode::draw();

		drawSelection( mTextCache );

		if ( mTextCache->getTextWidth() ) {
			if ( isClipped() ) {
				clipSmartEnable(
						mScreenPos.x + mRealPadding.Left,
						mScreenPos.y + mRealPadding.Top,
						mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
						mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom
				);
			}

			mTextCache->setAlign( getFlags() );
			mTextCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x + (int)mRealPadding.Left, mFontLineCenter + (Float)mScreenPosi.y + (int)mRealAlignOffset.y + (int)mRealPadding.Top, Vector2f::One, 0.f, getBlendMode() );

			if ( isClipped() ) {
				clipSmartDisable();
			}
		}
	}
}

Graphics::Font * UITextView::getFont() const {
	return mTextCache->getFont();
}

UITextView * UITextView::setFont( Graphics::Font * font ) {
	if ( NULL != font && mTextCache->getFont() != font ) {
		mTextCache->setFont( font );
		recalculate();
		onFontChanged();
		notifyLayoutAttrChange();
	}

	return this;
}

Uint32 UITextView::getCharacterSize() const {
	return mTextCache->getCharacterSize();
}

UITextView *UITextView::setCharacterSize( const Uint32 & characterSize ) {
	if ( mTextCache->getCharacterSize() != characterSize ) {
		mFontStyleConfig.CharacterSize = characterSize;
		mTextCache->setCharacterSize( characterSize );
		recalculate();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

const Uint32 &UITextView::getFontStyle() const {
	return mFontStyleConfig.Style;
}

const Float &UITextView::getOutlineThickness() const {
	return mFontStyleConfig.OutlineThickness;
}

UITextView * UITextView::setOutlineThickness( const Float & outlineThickness ) {
	if ( mFontStyleConfig.OutlineThickness != outlineThickness ) {
		mTextCache->setOutlineThickness( outlineThickness );
		mFontStyleConfig.OutlineThickness = outlineThickness;
		recalculate();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

const Color &UITextView::getOutlineColor() const {
	return mFontStyleConfig.OutlineColor;
}

UITextView * UITextView::setOutlineColor(const Color & outlineColor) {
	if ( mFontStyleConfig.OutlineColor != outlineColor ) {
		mFontStyleConfig.OutlineColor = outlineColor;
		Color newColor( outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a * mAlpha / 255.f );
		mTextCache->setOutlineColor( newColor );
		invalidateDraw();
	}

	return this;
}

UITextView * UITextView::setFontStyle(const Uint32 & fontStyle) {
	if ( mFontStyleConfig.Style != fontStyle ) {
		mTextCache->setStyle( fontStyle );
		mFontStyleConfig.Style = fontStyle;
		recalculate();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

const String& UITextView::getText() {
	if ( mFlags & UI_WORD_WRAP )
		return mString;

	return mTextCache->getString();
}

UITextView * UITextView::setText( const String& text ) {
	if ( ( mFlags & UI_WORD_WRAP ) ) {
		if ( mString != text ) {
			mString = text;
			mTextCache->setString( mString );

			recalculate();
			onTextChanged();
			notifyLayoutAttrChange();
		}
	} else if ( mTextCache->getString() != text ) {
		mTextCache->setString( text );

		recalculate();
		onTextChanged();
		notifyLayoutAttrChange();
	}

	return this;
}

const Color& UITextView::getFontColor() const {
	return mFontStyleConfig.FontColor;
}

UITextView * UITextView::setFontColor( const Color& color ) {
	if ( mFontStyleConfig.FontColor != color ) {
		mFontStyleConfig.FontColor = color;
		Color newColor( color.r, color.g, color.b, color.a * mAlpha / 255.f );
		mTextCache->setFillColor( newColor );
	}

	return this;
}

const Color& UITextView::getFontShadowColor() const {
	return mFontStyleConfig.ShadowColor;
}

UITextView * UITextView::setFontShadowColor( const Color& color ) {
	if ( mFontStyleConfig.ShadowColor != color ) {
		mFontStyleConfig.ShadowColor = color;
		Color newColor( color.r, color.g, color.b, color.a * mAlpha / 255.f );
		mTextCache->setShadowColor( newColor );
		invalidateDraw();
	}

	return this;
}

const Color& UITextView::getSelectionBackColor() const {
	return mFontStyleConfig.FontSelectionBackColor;
}

UITextView * UITextView::setSelectionBackColor( const Color& color ) {
	if ( mFontStyleConfig.FontSelectionBackColor != color ) {
		mFontStyleConfig.FontSelectionBackColor = color;
		invalidateDraw();
	}

	return this;
}

void UITextView::autoShrink() {
	if ( mFlags & UI_WORD_WRAP ) {
		shrinkText( mSize.getWidth() );
	}
}

void UITextView::shrinkText( const Uint32& MaxWidth ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mTextCache->setString( mString );
	}

	mTextCache->shrinkText( MaxWidth );
	invalidateDraw();
}

void UITextView::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE && 0 == getSize().getWidth() ) ) {
		setInternalPixelsSize( Sizef( mTextCache->getTextWidth(), mTextCache->getTextHeight() ) );
	}

	if ( mLayoutWidthRules == WRAP_CONTENT ) {
		setInternalPixelsWidth( (int)mTextCache->getTextWidth() + mRealPadding.Left + mRealPadding.Right );
	}

	if ( mLayoutHeightRules == WRAP_CONTENT ) {
		setInternalPixelsHeight( (int)mTextCache->getTextHeight() + mRealPadding.Top + mRealPadding.Bottom );
	}
}

void UITextView::alignFix() {
	switch ( fontHAlignGet( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x = (Float)( (Int32)( ( mSize.x - mRealPadding.Left - mRealPadding.Right ) / 2 - mTextCache->getTextWidth() / 2 ) );
			break;
		case UI_HALIGN_RIGHT:
			mRealAlignOffset.x = ( (Float)mSize.x - mRealPadding.Left - mRealPadding.Right - (Float)mTextCache->getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mRealAlignOffset.x = 0;
			break;
	}

	switch ( fontVAlignGet( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mRealAlignOffset.y = (Float)( (Int32)( ( mSize.y - mRealPadding.Top - mRealPadding.Bottom ) / 2 - mTextCache->getTextHeight() / 2 ) ) - 1;
			break;
		case UI_VALIGN_BOTTOM:
			mRealAlignOffset.y = ( (Float)mSize.y - mRealPadding.Top - mRealPadding.Bottom - (Float)mTextCache->getTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mRealAlignOffset.y = 0;
			break;
	}

	mAlignOffset = PixelDensity::pxToDp( mRealAlignOffset );
}

Uint32 UITextView::onFocusLoss() {
	UIWidget::onFocusLoss();

	selCurInit( -1 );
	selCurEnd( -1 );
	onSelectionChange();

	return 1;
}

void UITextView::onSizeChange() {
	recalculate();
	UINode::onSizeChange();
}

void UITextView::onTextChanged() {
	sendCommonEvent( Event::OnTextChanged );
	invalidateDraw();
}

void UITextView::onFontChanged() {
	sendCommonEvent( Event::OnFontChanged );
	invalidateDraw();
}

void UITextView::onAlphaChange() {
	Color color( getFontColor() );
	Color newColor( color.r, color.g, color.b, color.a * mAlpha / 255.f );
	mTextCache->setFillColor( newColor );

	color = getFontShadowColor();
	newColor = Color( color.r, color.g, color.b, color.a * mAlpha / 255.f );
	mTextCache->setShadowColor( newColor );

	color = getOutlineColor();
	newColor = Color( color.r, color.g, color.b, color.a * mAlpha / 255.f );
	mTextCache->setOutlineColor( newColor );

	invalidateDraw();
}

void UITextView::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	onThemeLoaded();
}

Float UITextView::getTextWidth() {
	return mTextCache->getTextWidth();
}

Float UITextView::getTextHeight() {
	return mTextCache->getTextHeight();
}

const int& UITextView::getNumLines() const {
	return mTextCache->getNumLines();
}

const Vector2f& UITextView::getAlignOffset() const {
	return mAlignOffset;
}

Uint32 UITextView::onMouseDoubleClick( const Vector2i& Pos, const Uint32& Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( controlPos );
		controlPos = PixelDensity::dpToPx( controlPos );

		Int32 curPos = mTextCache->findCharacterFromPos( Vector2i( controlPos.x, controlPos.y ) );

		if ( -1 != curPos ) {
			Int32 tSelCurInit, tSelCurEnd;

			mTextCache->findWordFromCharacterIndex( curPos, tSelCurInit, tSelCurEnd );

			selCurInit( tSelCurInit );
			selCurEnd( tSelCurEnd );
			onSelectionChange();

			mSelecting = false;
		}
	}

	return UIWidget::onMouseDoubleClick( Pos, Flags );
}

Uint32 UITextView::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		if ( selCurInit() == selCurEnd() ) {
			selCurInit( -1 );
			selCurEnd( -1 );
			onSelectionChange();
		}

		mSelecting = false;
	}

	return UIWidget::onMouseClick( Pos, Flags );
}

Uint32 UITextView::onMouseDown( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != getEventDispatcher() && isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && getEventDispatcher()->getDownControl() == this ) {
		Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( controlPos );
		controlPos = PixelDensity::dpToPx( controlPos ) - mRealAlignOffset;

		Int32 curPos = mTextCache->findCharacterFromPos( Vector2i( controlPos.x, controlPos.y ) );

		if ( -1 != curPos ) {
			if ( -1 == selCurInit() || !mSelecting ) {
				selCurInit( curPos );
				selCurEnd( curPos );
			} else {
				selCurEnd( curPos );
			}

			onSelectionChange();
		}

		mSelecting = true;
	}

	return UIWidget::onMouseDown( Pos, Flags );
}

void UITextView::drawSelection( Text * textCache ) {
	if ( selCurInit() != selCurEnd() ) {
		Int32 init		= eemin( selCurInit(), selCurEnd() );
		Int32 end		= eemax( selCurInit(), selCurEnd() );

		if ( init < 0 && end > (Int32)textCache->getString().size() ) {
			return;
		}

		Int32 lastEnd;
		Vector2f initPos, endPos;

		if ( mLastSelCurInit != selCurInit() || mLastSelCurEnd != selCurEnd() ) {
			mSelPosCache.clear();
			mLastSelCurInit = selCurInit();
			mLastSelCurEnd = selCurEnd();

			do {
				initPos	= textCache->findCharacterPos( init );
				lastEnd = textCache->getString().find_first_of( '\n', init );

				if ( lastEnd < end && -1 != lastEnd ) {
					endPos	= textCache->findCharacterPos( lastEnd );
					init	= lastEnd + 1;
				} else {
					endPos	= textCache->findCharacterPos( end );
					lastEnd = end;
				}

				mSelPosCache.push_back( SelPosCache( initPos, endPos ) );
			} while ( end != lastEnd );
		}

		if ( !mSelPosCache.empty() ) {
			Primitives P;
			P.setColor( mFontStyleConfig.FontSelectionBackColor );
			Float vspace = textCache->getFont()->getFontHeight( textCache->getCharacterSizePx() );

			for ( size_t i = 0; i < mSelPosCache.size(); i++ ) {
				initPos = mSelPosCache[i].initPos;
				endPos = mSelPosCache[i].endPos;

				P.drawRectangle( Rectf( mScreenPos.x + initPos.x + mRealAlignOffset.x + mRealPadding.Left,
										  mScreenPos.y + initPos.y + mRealAlignOffset.y + mRealPadding.Top,
										  mScreenPos.x + endPos.x + mRealAlignOffset.x + mRealPadding.Left,
										  mScreenPos.y + endPos.y + vspace + mRealAlignOffset.y + mRealPadding.Top )
				);
			}
		}
	}
}

bool UITextView::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

const UIFontStyleConfig& UITextView::getFontStyleConfig() const {
	return mFontStyleConfig;
}

void UITextView::setFontStyleConfig( const UIFontStyleConfig& fontStyleConfig ) {
	mFontStyleConfig = fontStyleConfig;
	setFont( fontStyleConfig.getFont() );
	setCharacterSize( fontStyleConfig.getFontCharacterSize() );
	setFontColor( fontStyleConfig.getFontColor() );
	setFontShadowColor( fontStyleConfig.getFontShadowColor() );
	setOutlineThickness( fontStyleConfig.getOutlineThickness() );
	setOutlineColor( fontStyleConfig.getOutlineColor() );
	setFontStyle( fontStyleConfig.getFontStyle() );
}

void UITextView::selCurInit( const Int32& init ) {
	if ( mSelCurInit != init ) {
		mSelCurInit = init;
		invalidateDraw();
	}
}

void UITextView::selCurEnd( const Int32& end ) {
	if ( mSelCurEnd != end ) {
		mSelCurEnd = end;
		invalidateDraw();
	}
}

Int32 UITextView::selCurInit() {
	return mSelCurInit;
}

Int32 UITextView::selCurEnd() {
	return mSelCurEnd;
}

void UITextView::onAlignChange() {
	UIWidget::onAlignChange();
	alignFix();
}

void UITextView::onSelectionChange() {
	mTextCache->invalidateColors();

	if ( selCurInit() != selCurEnd() ) {
		Color color( mFontStyleConfig.getFontSelectedColor() );
		color.a =  mFontStyleConfig.getFontSelectedColor().a * mAlpha / 255.f;
		mTextCache->setFillColor( color, eemin<Int32>( selCurInit(), selCurEnd() ), eemax<Int32>( selCurInit(), selCurEnd() ) - 1 );
	} else {
		Color color( mFontStyleConfig.getFontColor() );
		color.a =  mFontStyleConfig.getFontColor().a * mAlpha / 255.f;
		mTextCache->setFillColor( color );
	}

	invalidateDraw();
}

const Int32 &UITextView::getFontLineCenter() {
	return mFontLineCenter;
}

void UITextView::recalculate() {
	int fontHeight = mTextCache->getCharacterSizePx();
	mFontLineCenter = eefloor((Float)( ( mTextCache->getFont()->getLineSpacing(fontHeight) - fontHeight ) / 2 ));

	autoShrink();
	onAutoSize();
	alignFix();
	resetSelCache();
}

void UITextView::resetSelCache() {
	mLastSelCurInit = mLastSelCurEnd = -1;
	invalidateDraw();
	onSelectionChange();
}

#define SAVE_NORMAL_STATE_ATTR( ATTR_FORMATED ) \
	if ( state != UIState::StateFlagNormal || ( state == UIState::StateFlagNormal && attribute.isVolatile() ) ) { \
		CSS::StyleSheetProperty oldAttribute = mStyle->getStatelessStyleSheetProperty( attribute.getName() ); \
		if ( oldAttribute.isEmpty() && mStyle->getPreviousState() == UIState::StateFlagNormal ) { \
			mStyle->setStyleSheetProperty( CSS::StyleSheetProperty( attribute.getName(), ATTR_FORMATED ) ); \
		} \
	}

bool UITextView::setAttribute( const StyleSheetProperty& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "text" == name ) {
		if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
			setText( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( attribute.asString() ) );
	} else if ( "color" == name || "textcolor" == name ) {
		SAVE_NORMAL_STATE_ATTR( getFontColor().toHexString() );

		Color color = attribute.asColor();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( attribute.getName() ) ) {
			CSS::TransitionDefinition transitionInfo( mStyle->getTransition( attribute.getName() ) );

			Action * action = Actions::Tint::New( getFontColor(), color, true, transitionInfo.duration, transitionInfo.timingFunction, Actions::Tint::Text );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setFontColor( color );
		}
	} else if ( "shadow-color" == name || "textshadowcolor" == name ) {
		SAVE_NORMAL_STATE_ATTR( getFontShadowColor().toHexString() );

		Color color = attribute.asColor();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( attribute.getName() ) ) {
			CSS::TransitionDefinition transitionInfo( mStyle->getTransition( attribute.getName() ) );

			Action * action = Actions::Tint::New( getFontShadowColor(), color, true, transitionInfo.duration, transitionInfo.timingFunction, Actions::Tint::TextShadow );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setFontShadowColor( color );
		}
	} else if ( "selected-color" == name || "textselectedcolor" == name ) {
		mFontStyleConfig.FontSelectedColor = attribute.asColor();
	} else if ( "selection-back-color" == name || "textselectionbackcolor" == name ) {
		setSelectionBackColor( attribute.asColor() );
	} else if ( "font-family" == name || "font-name" == name || "fontfamily" == name || "fontname" == name ) {
		SAVE_NORMAL_STATE_ATTR( getFont()->getName() );

		setFont( FontManager::instance()->getByName( attribute.asString() ) );
	} else if ( "font-size" == name || "textsize" == name || "fontsize" == name ) {
		SAVE_NORMAL_STATE_ATTR( String::format( "%dpx", getCharacterSize() ) );

		setCharacterSize( attribute.asDpDimensionI() );
	} else if ( "font-style" == name || "textstyle" == name || "fontstyle" == name ||
				"text-decoration" == name || "textdecoration" == name ) {
		Uint32 flags = attribute.asFontStyle();

		SAVE_NORMAL_STATE_ATTR( Text::styleFlagToString( getFontStyle() ) );

		if ( flags & UI_WORD_WRAP ) {
			mFlags |= UI_WORD_WRAP;
			flags &= ~ UI_WORD_WRAP;
			autoShrink();
		}

		setFontStyle( flags );
	} else if ( "wordwrap" == name || "word-wrap" == name ) {
		if ( attribute.asBool() )
			mFlags |= UI_WORD_WRAP;
		else
			mFlags &= ~UI_WORD_WRAP;

		autoShrink();
	} else if ( "text-stroke-width" == name || "fontoutlinethickness" == name ) {
		SAVE_NORMAL_STATE_ATTR( String::toStr( PixelDensity::dpToPx( getOutlineThickness() ) ) )

		setOutlineThickness( PixelDensity::dpToPx( attribute.asDpDimension() ) );
	} else if ( "text-stroke-color" == name || "fontoutlinecolor" == name ) {
		SAVE_NORMAL_STATE_ATTR( getOutlineColor().toHexString() );

		Color color = attribute.asColor();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( attribute.getName() ) ) {
			CSS::TransitionDefinition transitionInfo( mStyle->getTransition( attribute.getName() ) );

			Action * action = Actions::Tint::New( getOutlineColor(), color, true, transitionInfo.duration, transitionInfo.timingFunction, Actions::Tint::TextOutline );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setOutlineColor( color );
		}
	} else if ( "text-selection" == name || "textselection" == name ) {
		mFlags|= UI_TEXT_SELECTION_ENABLED;
	} else if ( "text-align" == name || "textalign" == name ) {
		std::string align = String::toLower( attribute.value() );

		if ( align == "center" ) setFlags( UI_HALIGN_CENTER );
		else if ( align == "left" ) setFlags( UI_HALIGN_LEFT );
		else if ( align == "right" ) setFlags( UI_HALIGN_RIGHT );
	} else {
		return UIWidget::setAttribute( attribute, state );
	}

	return true;
}

}}
