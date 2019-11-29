#include <eepp/ui/uitextinput.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/text.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

UITextInput * UITextInput::New() {
	return eeNew( UITextInput, () );
}

UITextInput * UITextInput::NewWithTag( const std::string& tag ) {
	return eeNew( UITextInput, ( tag ) );
}

UITextInput::UITextInput( const std::string& tag ) :
	UITextView( tag ),
	mCursorPos(0),
	mAllowEditing( true ),
	mShowingWait( true )
{
	subscribeScheduledUpdate();

	setFlags( UI_AUTO_PADDING | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED );
	clipEnable();

	mTextBuffer.start();
	mTextBuffer.setActive( false );
	mTextBuffer.setFreeEditing( true );
	mTextBuffer.setTextSelectionEnabled( isTextSelectionEnabled() );
	mTextBuffer.setReturnCallback( cb::Make0( this, &UITextInput::privOnPressEnter ) );
	mTextBuffer.setCursorPositionChangeCallback( cb::Make0( this, &UITextInput::onCursorPositionChange ) );
	mTextBuffer.setBufferChangeCallback( cb::Make0( this, &UITextInput::onBufferChange ) );
	mTextBuffer.setSelectionChangeCallback( cb::Make0( this, &UITextInput::onInputSelectionChange ) );

	applyDefaultTheme();
}

UITextInput::UITextInput() :
	UITextInput( "textinput" )
{}

UITextInput::~UITextInput() {
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
	if ( mVisible && mTextBuffer.isActive() && mTextBuffer.isFreeEditingEnabled() && mShowingWait ) {
		bool disableSmooth = mShowingWait && GLi->isLineSmooth();

		if ( disableSmooth )
			GLi->lineSmooth( false );

		Primitives P;
		P.setColor( mFontStyleConfig.FontColor );

		Float CurPosX = eefloor( mScreenPos.x + mRealAlignOffset.x + mCurPos.x + PixelDensity::dpToPx( 1.f ) + mRealPadding.Left );
		Float CurPosY = mScreenPos.y + mRealAlignOffset.y + mCurPos.y + mRealPadding.Top;

		if ( CurPosX > (Float)mScreenPos.x + (Float)mSize.x )
			CurPosX = (Float)mScreenPos.x + (Float)mSize.x;

		P.drawLine( Line2f( Vector2f( CurPosX, CurPosY ), Vector2f( CurPosX, CurPosY + mTextCache->getFont()->getFontHeight( mTextCache->getCharacterSizePx() ) ) ) );

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
	UITextView::draw();

	drawWaitingCursor();
}

Uint32 UITextInput::onFocus() {
	UINode::onFocus();

	if ( mAllowEditing ) {
		mTextBuffer.setActive( true );

		resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onFocusLoss() {
	mTextBuffer.setActive( false );
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

	if ( fontHAlignGet( getFlags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum	= mTextBuffer.getCurPosLinePos( NLPos );

		Text textCache( mTextCache->getFont(), mTextCache->getCharacterSize() );

		textCache.setString( mTextBuffer.getBuffer().substr( NLPos, mTextBuffer.getCursorPosition() - NLPos ) );

		Float tW	= textCache.getTextWidth();
		Float tX	= mRealAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mTextCache->getFont()->getLineSpacing( mTextCache->getCharacterSizePx() );

		if ( !mTextBuffer.setSupportNewLine() ) {
			if ( tX < 0.f )
				mRealAlignOffset.x = -( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
			else if ( tX > mSize.getWidth() - mRealPadding.Left - mRealPadding.Right )
				mRealAlignOffset.x = mSize.getWidth() - mRealPadding.Left - mRealPadding.Right - ( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
		}
	}
}

void UITextInput::setTheme( UITheme * Theme ) {
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
		setInternalPixelsHeight( PixelDensity::dpToPxI( getSkinSize().getHeight() ) + mRealPadding.Top + mRealPadding.Bottom );
	}

	if ( mLayoutHeightRules == WRAP_CONTENT ) {
		int minHeight = eemax<int>( mTextCache->getTextHeight(), PixelDensity::dpToPxI( getSkinSize().getHeight() ) );
		setInternalPixelsHeight( minHeight + mRealPadding.Top + mRealPadding.Bottom );
	}
}

void UITextInput::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		setPadding( Rectf() );
	}
}

InputTextBuffer * UITextInput::getInputTextBuffer() {
	return &mTextBuffer;
}

UITextInput * UITextInput::setAllowEditing( const bool& allow ) {
	mAllowEditing = allow;

	if ( !mAllowEditing && mTextBuffer.isActive() )
		mTextBuffer.setActive( false );

	return this;
}

const bool& UITextInput::getAllowEditing() const {
	return mAllowEditing;
}

UITextView * UITextInput::setText( const String& text ) {
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

void UITextInput::updateText() {
}

Uint32 UITextInput::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	UITextView::onMouseClick( Pos, Flags );

	if ( ( Flags & EE_BUTTON_LMASK ) ) {
		if ( selCurInit() == selCurEnd() ) {
			Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
			worldToNode( controlPos );
			controlPos = PixelDensity::dpToPx( controlPos ) - mRealAlignOffset;

			Int32 curPos = mTextCache->findCharacterFromPos( Vector2i( controlPos.x, controlPos.y ) );

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

Uint32 UITextInput::onMouseDown(const Vector2i& position, const Uint32& flags) {
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

UITextInput * UITextInput::setMaxLength( Uint32 maxLength ) {
	mTextBuffer.setMaxLength( maxLength );
	return this;
}

Uint32 UITextInput::getMaxLength() {
	return mTextBuffer.getMaxLength();
}

UITextInput * UITextInput::setFreeEditing( bool support ) {
	mTextBuffer.setFreeEditing( support );
	return this;
}

bool UITextInput::isFreeEditingEnabled() {
	return mTextBuffer.isFreeEditingEnabled();
}

bool UITextInput::applyProperty( const StyleSheetProperty& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "text" == name ) {
		if ( NULL != mSceneNode && mSceneNode->isUISceneNode() ) {
			setText( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( attribute.asString() ) );
		}
	} else if ( "allow-editing" == name || "allowediting" == name ) {
		setAllowEditing( attribute.asBool() );
	} else if ( "max-length" == name || "maxlength" == name ) {
		setMaxLength( attribute.asUint() );
	} else if ( "free-editing" == name || "freeediting" == name ) {
		setFreeEditing( attribute.asBool() );
	} else if ( "only-numbers" == name || "onlynumbers" == name ) {
		getInputTextBuffer()->setAllowOnlyNumbers( attribute.asBool(), getInputTextBuffer()->dotsInNumbersAllowed() );
	} else if ( "allow-dot" == name || "allowdot" == name ) {
		getInputTextBuffer()->setAllowOnlyNumbers( getInputTextBuffer()->onlyNumbersAllowed(), attribute.asBool() );
	} else {
		return UITextView::applyProperty( attribute, state );
	}

	return true;
}

UIWidget * UITextInput::setPadding( const Rectf& padding ) {
	Rectf autoPadding;

	if ( mFlags & UI_AUTO_PADDING ) {
		autoPadding = makePadding( true, true, false, false );
	}

	UITextView::setPadding( autoPadding + padding );

	return this;
}

}}
