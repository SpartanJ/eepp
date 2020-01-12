#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/window/engine.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITextInput* UITextInput::New() {
	return eeNew( UITextInput, () );
}

UITextInput* UITextInput::NewWithTag( const std::string& tag ) {
	return eeNew( UITextInput, ( tag ) );
}

UITextInput::UITextInput( const std::string& tag ) :
	UITextView( tag ), mCursorPos( 0 ), mAllowEditing( true ), mShowingWait( true ) {
	mHintCache = Text::New();

	subscribeScheduledUpdate();

	setFlags( UI_AUTO_PADDING | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED );
	clipEnable();

	mTextBuffer.start();
	mTextBuffer.setActive( false );
	mTextBuffer.setFreeEditing( true );
	mTextBuffer.setTextSelectionEnabled( isTextSelectionEnabled() );
	mTextBuffer.setReturnCallback( cb::Make0( this, &UITextInput::privOnPressEnter ) );
	mTextBuffer.setCursorPositionChangeCallback(
		cb::Make0( this, &UITextInput::onCursorPositionChange ) );
	mTextBuffer.setBufferChangeCallback( cb::Make0( this, &UITextInput::onBufferChange ) );
	mTextBuffer.setSelectionChangeCallback(
		cb::Make0( this, &UITextInput::onInputSelectionChange ) );

	applyDefaultTheme();
}

UITextInput::UITextInput() : UITextInput( "textinput" ) {}

UITextInput::~UITextInput() {
	eeSAFE_DELETE( mHintCache );
}

Uint32 UITextInput::getType() const {
	return UI_TYPE_TEXTINPUT;
}

bool UITextInput::isType( const Uint32& type ) const {
	return UITextInput::getType() == type ? true : UITextView::isType( type );
}

void UITextInput::scheduledUpdate( const Time& time ) {
	if ( isMouseOverMeOrChilds() && NULL != mSceneNode ) {
		mSceneNode->setCursor( Cursor::IBeam );
	}

	updateWaitingCursor( time );
}

void UITextInput::onCursorPosChange() {
	sendCommonEvent( Event::OnCursorPosChange );
	invalidateDraw();
}

void UITextInput::drawWaitingCursor() {
	if ( mVisible && mTextBuffer.isActive() && mTextBuffer.isFreeEditingEnabled() &&
		 mShowingWait ) {
		bool disableSmooth = mShowingWait && GLi->isLineSmooth();

		if ( disableSmooth )
			GLi->lineSmooth( false );

		Primitives P;
		P.setColor( mFontStyleConfig.FontColor );

		Float CurPosX = eefloor( mScreenPos.x + mRealAlignOffset.x + mCurPos.x +
								 PixelDensity::dpToPx( 1.f ) + mRealPadding.Left );
		Float CurPosY = mScreenPos.y + mRealAlignOffset.y + mCurPos.y + mRealPadding.Top;

		if ( CurPosX > (Float)mScreenPos.x + (Float)mSize.x )
			CurPosX = (Float)mScreenPos.x + (Float)mSize.x;

		P.drawLine(
			Line2f( Vector2f( CurPosX, CurPosY ),
					Vector2f( CurPosX, CurPosY + mTextCache->getFont()->getFontHeight(
													 mTextCache->getCharacterSizePx() ) ) ) );

		if ( disableSmooth )
			GLi->lineSmooth( true );
	}
}

void UITextInput::updateWaitingCursor( const Time& time ) {
	if ( mVisible && mTextBuffer.isActive() && mTextBuffer.isFreeEditingEnabled() ) {
		mWaitCursorTime += time.asMilliseconds();

		if ( mWaitCursorTime >= 500.f ) {
			mShowingWait = !mShowingWait;
			mWaitCursorTime = 0.f;
			invalidateDraw();
		}
	}
}

void UITextInput::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UINode::draw();

		if ( mTextCache->getTextWidth() ) {
			drawSelection( mTextCache );

			if ( isClipped() ) {
				clipSmartEnable( mScreenPos.x + mRealPadding.Left, mScreenPos.y + mRealPadding.Top,
								 mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
								 mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
			}

			mTextCache->setAlign( getFlags() );
			mTextCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x +
								  (int)mRealPadding.Left,
							  mFontLineCenter + (Float)mScreenPosi.y + (int)mRealAlignOffset.y +
								  (int)mRealPadding.Top,
							  Vector2f::One, 0.f, getBlendMode() );

			if ( isClipped() ) {
				clipSmartDisable();
			}
		} else if ( !mHintCache->getString().empty() && !mTextBuffer.isActive() ) {
			if ( isClipped() ) {
				clipSmartEnable( mScreenPos.x + mRealPadding.Left, mScreenPos.y + mRealPadding.Top,
								 mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
								 mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
			}

			mHintCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x +
								  (int)mRealPadding.Left,
							  mFontLineCenter + (Float)mScreenPosi.y + (int)mRealAlignOffset.y +
								  (int)mRealPadding.Top,
							  Vector2f::One, 0.f, getBlendMode() );

			if ( isClipped() ) {
				clipSmartDisable();
			}
		}
	}

	drawWaitingCursor();
}

Uint32 UITextInput::onFocus() {
	UINode::onFocus();

	if ( mAllowEditing ) {
		mTextBuffer.setActive( true );

		resetWaitCursor();

		getSceneNode()->getWindow()->startTextInput();
	}

	return 1;
}

Uint32 UITextInput::onFocusLoss() {
	mTextBuffer.setActive( false );
	getSceneNode()->getWindow()->stopTextInput();
	return UITextView::onFocusLoss();
}

Uint32 UITextInput::onPressEnter() {
	sendCommonEvent( Event::OnPressEnter );
	return 0;
}

void UITextInput::privOnPressEnter() {
	onPressEnter();
}

void UITextInput::pushIgnoredChar( const Uint32& ch ) {
	mTextBuffer.pushIgnoredChar( ch );
}

void UITextInput::resetWaitCursor() {
	mShowingWait = true;
	mWaitCursorTime = 0.f;
}

void UITextInput::alignFix() {
	UITextView::alignFix();

	if ( Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos = 0;
		Uint32 LineNum = mTextBuffer.getCurPosLinePos( NLPos );

		Text textCache( mTextCache->getFont(), mTextCache->getCharacterSize() );

		textCache.setString(
			mTextBuffer.getBuffer().substr( NLPos, mTextBuffer.getCursorPosition() - NLPos ) );

		Float tW = textCache.getTextWidth();
		Float tX = mRealAlignOffset.x + tW;

		mCurPos.x = tW;
		mCurPos.y = (Float)LineNum * (Float)mTextCache->getFont()->getLineSpacing(
										 mTextCache->getCharacterSizePx() );

		if ( !mTextBuffer.setSupportNewLine() ) {
			if ( tX < 0.f )
				mRealAlignOffset.x = -( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
			else if ( tX > mSize.getWidth() - mRealPadding.Left - mRealPadding.Right )
				mRealAlignOffset.x = mSize.getWidth() - mRealPadding.Left - mRealPadding.Right -
									 ( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
		}
	}
}

void UITextInput::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "textinput" );

	onThemeLoaded();
}

void UITextInput::onThemeLoaded() {
	UITextView::onThemeLoaded();

	mMinControlSize.y = eemax( mMinControlSize.y, getSkinSize().getHeight() );

	autoPadding();
	onAutoSize();

	UIWidget::onThemeLoaded();
}

void UITextInput::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) && 0 == getSize().getHeight() ) {
		setInternalPixelsHeight( PixelDensity::dpToPxI( getSkinSize().getHeight() ) +
								 mRealPadding.Top + mRealPadding.Bottom );
	}

	if ( mLayoutHeightRule == LayoutSizeRule::WrapContent ) {
		int minHeight = eemax<int>( mTextCache->getTextHeight(),
									PixelDensity::dpToPxI( getSkinSize().getHeight() ) );
		setInternalPixelsHeight( minHeight + mRealPadding.Top + mRealPadding.Bottom );
	}
}

void UITextInput::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		setPadding( Rectf() );
	}
}

InputTextBuffer* UITextInput::getInputTextBuffer() {
	return &mTextBuffer;
}

UITextInput* UITextInput::setAllowEditing( const bool& allow ) {
	mAllowEditing = allow;

	if ( !mAllowEditing && mTextBuffer.isActive() )
		mTextBuffer.setActive( false );

	return this;
}

const bool& UITextInput::isEditingAllowed() const {
	return mAllowEditing;
}

UITextView* UITextInput::setText( const String& text ) {
	UITextView::setText( text );

	mTextBuffer.setBuffer( text );

	mTextBuffer.cursorToEnd();

	return this;
}

const String& UITextInput::getText() {
	return UITextView::getText();
}

void UITextInput::shrinkText( const Uint32& MaxWidth ) {
	mTextCache->setString( mTextBuffer.getBuffer() );

	UITextView::shrinkText( MaxWidth );

	mTextBuffer.setBuffer( mTextCache->getString() );

	alignFix();
}

void UITextInput::updateText() {}

Uint32 UITextInput::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	UITextView::onMouseClick( Pos, Flags );

	if ( ( Flags & EE_BUTTON_LMASK ) ) {
		if ( selCurInit() == selCurEnd() ) {
			Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
			worldToNode( controlPos );
			controlPos = PixelDensity::dpToPx( controlPos ) - mRealAlignOffset;

			Int32 curPos =
				mTextCache->findCharacterFromPos( Vector2i( controlPos.x, controlPos.y ) );

			if ( -1 != curPos ) {
				mTextBuffer.setCursorPosition( curPos );
				onCursorPositionChange();
				resetWaitCursor();
			}
		} else {
			mTextBuffer.setCursorPosition( selCurEnd() );
			onCursorPositionChange();
			resetWaitCursor();
		}
	}

	return 1;
}

Uint32 UITextInput::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	int endPos = selCurEnd();

	UITextView::onMouseDown( position, flags );

	if ( endPos != selCurEnd() && -1 != selCurEnd() ) {
		mTextBuffer.setCursorPosition( selCurEnd() );
		onCursorPositionChange();
		resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onMouseDoubleClick( const Vector2i& Pos, const Uint32& Flags ) {
	UITextView::onMouseDoubleClick( Pos, Flags );

	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && selCurEnd() != -1 ) {
		mTextBuffer.setCursorPosition( selCurEnd() );
		onCursorPositionChange();
		resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	UINode::onMouseLeave( Pos, Flags );

	if ( NULL != mSceneNode )
		mSceneNode->setCursor( Cursor::Arrow );

	return 1;
}

void UITextInput::selCurInit( const Int32& init ) {
	if ( mTextBuffer.selCurInit() != init ) {
		mTextBuffer.selCurInit( init );
		invalidateDraw();
	}
}

void UITextInput::selCurEnd( const Int32& end ) {
	if ( mTextBuffer.selCurEnd() != end ) {
		mTextBuffer.selCurEnd( end );
		invalidateDraw();

		if ( mTextBuffer.selCurEnd() != mTextBuffer.selCurInit() ) {
			mTextBuffer.setCursorPosition( end );
			onCursorPosChange();
		}
	}
}

Int32 UITextInput::selCurInit() {
	return mTextBuffer.selCurInit();
}

Int32 UITextInput::selCurEnd() {
	return mTextBuffer.selCurEnd();
}

void UITextInput::onCursorPositionChange() {
	if ( mCursorPos != mTextBuffer.getCursorPosition() ) {
		alignFix();
		mCursorPos = mTextBuffer.getCursorPosition();
		mWaitCursorTime = 0.f;
		mShowingWait = true;
		onCursorPosChange();
	}
}

void UITextInput::onBufferChange() {
	Vector2f offSet = mRealAlignOffset;

	UITextView::setText( mTextBuffer.getBuffer() );

	updateText();

	mRealAlignOffset = offSet;

	resetWaitCursor();

	alignFix();

	mCursorPos = mTextBuffer.getCursorPosition();

	mTextBuffer.setChangedSinceLastUpdate( false );

	invalidateDraw();

	sendCommonEvent( Event::OnBufferChange );
}

void UITextInput::onInputSelectionChange() {
	onSelectionChange();
}

UITextInput* UITextInput::setMaxLength( Uint32 maxLength ) {
	mTextBuffer.setMaxLength( maxLength );
	return this;
}

Uint32 UITextInput::getMaxLength() {
	return mTextBuffer.getMaxLength();
}

UITextInput* UITextInput::setFreeEditing( bool support ) {
	mTextBuffer.setFreeEditing( support );
	return this;
}

bool UITextInput::isFreeEditingEnabled() {
	return mTextBuffer.isFreeEditingEnabled();
}

std::string UITextInput::getPropertyString( const PropertyDefinition* propertyDef ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
			return getText().toUtf8();
		case PropertyId::AllowEditing:
			return isEditingAllowed() ? "true" : "false";
		case PropertyId::MaxLength:
			return String::toStr( getMaxLength() );
		case PropertyId::FreeEditing:
			return isFreeEditingEnabled() ? "true" : "false";
		case PropertyId::OnlyNumbers:
			return getInputTextBuffer()->onlyNumbersAllowed() ? "true" : "false";
		case PropertyId::AllowDot:
			return getInputTextBuffer()->dotsInNumbersAllowed() ? "true" : "false";
		case PropertyId::Hint:
			return getHint().toUtf8();
		case PropertyId::HintColor:
			return getHintColor().toHexString();
		case PropertyId::HintShadowColor:
			return getHintShadowColor().toHexString();
		case PropertyId::HintFontSize:
			return String::format( "%ddp", getHintCharacterSize() );
		case PropertyId::HintFontFamily:
			return NULL != getHintFont() ? getFont()->getName() : "";
		case PropertyId::HintFontStyle:
			return Text::styleFlagToString( getHintFontStyle() );
		case PropertyId::HintStrokeWidth:
			return String::toStr( PixelDensity::dpToPx( getHintOutlineThickness() ) );
		case PropertyId::HintStrokeColor:
			return getHintOutlineColor().toHexString();
		default:
			return UITextView::getPropertyString( propertyDef );
	}
}

bool UITextInput::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			if ( NULL != getUISceneNode() )
				setText( getUISceneNode()->getTranslatorString( attribute.asString() ) );
			break;
		case PropertyId::AllowEditing:
			setAllowEditing( attribute.asBool() );
			break;
		case PropertyId::MaxLength:
			setMaxLength( attribute.asUint() );
			break;
		case PropertyId::FreeEditing:
			setFreeEditing( attribute.asBool() );
			break;
		case PropertyId::OnlyNumbers:
			getInputTextBuffer()->setAllowOnlyNumbers(
				attribute.asBool(), getInputTextBuffer()->dotsInNumbersAllowed() );
			break;
		case PropertyId::AllowDot:
			getInputTextBuffer()->setAllowOnlyNumbers( getInputTextBuffer()->onlyNumbersAllowed(),
													   attribute.asBool() );
			break;
		case PropertyId::Hint:
			if ( NULL != getUISceneNode() )
				setHint( getUISceneNode()->getTranslatorString( attribute.asString() ) );
			break;
		case PropertyId::HintColor:
			setHintColor( attribute.asColor() );
			break;
		case PropertyId::HintShadowColor:
			setHintShadowColor( attribute.asColor() );
			break;
		case PropertyId::HintFontSize:
			setHintCharacterSize( attribute.asDpDimensionI() );
			break;
		case PropertyId::HintFontFamily:
			setHintFont( FontManager::instance()->getByName( attribute.asString() ) );
			break;
		case PropertyId::HintFontStyle:
			setHintFontStyle( attribute.asFontStyle() );
			break;
		case PropertyId::HintStrokeWidth:
			setHintOutlineThickness( PixelDensity::dpToPx( attribute.asDpDimension() ) );
			break;
		case PropertyId::HintStrokeColor:
			setHintOutlineColor( attribute.asColor() );
			break;
		default:
			return UITextView::applyProperty( attribute );
	}

	return true;
}

UIWidget* UITextInput::setPadding( const Rectf& padding ) {
	Rectf autoPadding;

	if ( mFlags & UI_AUTO_PADDING ) {
		autoPadding = makePadding( true, true, false, false );
	}

	UITextView::setPadding( autoPadding + padding );

	return this;
}

void UITextInput::onFontChanged() {
	if ( getHintFont() == NULL ) {
		setHintFont( getFont() );
	}
}

const String& UITextInput::getHint() const {
	return mHintCache->getString();
}

UITextInput* UITextInput::setHint( const String& hint ) {
	if ( hint != mHintCache->getString() ) {
		mHintCache->setString( hint );
		invalidateDraw();
	}

	return this;
}

const Color& UITextInput::getHintColor() const {
	return mHintStyleConfig.getFontColor();
}

UITextInput* UITextInput::setHintColor( const Color& hintColor ) {
	if ( hintColor != mHintStyleConfig.getFontColor() ) {
		mHintCache->setFillColor( hintColor );
		mHintStyleConfig.FontColor = hintColor;
		invalidateDraw();
	}

	return this;
}

const Color& UITextInput::getHintShadowColor() const {
	return mHintStyleConfig.getFontShadowColor();
}

UITextInput* UITextInput::setHintShadowColor( const Color& shadowColor ) {
	if ( shadowColor != mHintStyleConfig.getFontShadowColor() ) {
		mHintCache->setShadowColor( shadowColor );
		mHintStyleConfig.ShadowColor = shadowColor;
		invalidateDraw();
	}

	return this;
}

Font* UITextInput::getHintFont() {
	return mHintStyleConfig.getFont();
}

UITextInput* UITextInput::setHintFont( Font* font ) {
	if ( mHintStyleConfig.getFont() != font ) {
		mHintCache->setFont( font );
		mHintStyleConfig.Font = font;
		invalidateDraw();
	}

	return this;
}

Uint32 UITextInput::getHintCharacterSize() const {
	return mHintCache->getCharacterSize();
}

UITextView* UITextInput::setHintCharacterSize( const Uint32& characterSize ) {
	if ( mHintCache->getCharacterSize() != characterSize ) {
		mHintCache->setCharacterSize( characterSize );
		mHintStyleConfig.CharacterSize = characterSize;
		invalidateDraw();
	}

	return this;
}

const Uint32& UITextInput::getHintFontStyle() const {
	return mHintStyleConfig.Style;
}

const Float& UITextInput::getHintOutlineThickness() const {
	return mHintStyleConfig.OutlineThickness;
}

UITextView* UITextInput::setHintOutlineThickness( const Float& outlineThickness ) {
	if ( mHintStyleConfig.OutlineThickness != outlineThickness ) {
		mHintCache->setOutlineThickness( outlineThickness );
		mHintStyleConfig.OutlineThickness = outlineThickness;
		invalidateDraw();
	}

	return this;
}

const Color& UITextInput::getHintOutlineColor() const {
	return mHintStyleConfig.OutlineColor;
}

UITextView* UITextInput::setHintOutlineColor( const Color& outlineColor ) {
	if ( mHintStyleConfig.OutlineColor != outlineColor ) {
		mHintStyleConfig.OutlineColor = outlineColor;
		Color newColor( outlineColor.r, outlineColor.g, outlineColor.b,
						outlineColor.a * mAlpha / 255.f );
		mHintCache->setOutlineColor( newColor );
		invalidateDraw();
	}

	return this;
}

UITextView* UITextInput::setHintFontStyle( const Uint32& fontStyle ) {
	if ( mHintStyleConfig.Style != fontStyle ) {
		mHintCache->setStyle( fontStyle );
		mHintStyleConfig.Style = fontStyle;
		invalidateDraw();
	}

	return this;
}

}} // namespace EE::UI
