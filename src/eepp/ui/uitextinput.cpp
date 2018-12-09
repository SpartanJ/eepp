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

UITextInput::UITextInput() :
	UITextView(),
	mCursorPos(0),
	mAllowEditing( true ),
	mShowingWait( true )
{
	setFlags( UI_AUTO_PADDING | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED );
	clipEnable();

	mTextBuffer.start();
	mTextBuffer.setActive( false );
	mTextBuffer.setFreeEditing( true );
	mTextBuffer.setTextSelectionEnabled( isTextSelectionEnabled() );
	mTextBuffer.setReturnCallback( cb::Make0( this, &UITextInput::privOnPressEnter ) );

	applyDefaultTheme();
}

UITextInput::~UITextInput() {
}

Uint32 UITextInput::getType() const {
	return UI_TYPE_TEXTINPUT;
}

bool UITextInput::isType( const Uint32& type ) const {
	return UITextInput::getType() == type ? true : UITextView::isType( type );
}

void UITextInput::update( const Time& time ) {
	if ( isMouseOverMeOrChilds() && NULL != mSceneNode ) {
		mSceneNode->setCursor( EE_CURSOR_IBEAM );
	}

	UITextView::update( time );

	if ( mTextBuffer.changedSinceLastUpdate() ) {
		Vector2f offSet = mRealAlignOffset;

		UITextView::setText( mTextBuffer.getBuffer() );

		updateText();

		mRealAlignOffset = offSet;

		resetWaitCursor();

		alignFix();

		mCursorPos = mTextBuffer.getCursorPos();

		mTextBuffer.setChangedSinceLastUpdate( false );

		invalidateDraw();

		return;
	}

	if ( mCursorPos != mTextBuffer.getCursorPos() ) {
		alignFix();
		mCursorPos = mTextBuffer.getCursorPos();
		mWaitCursorTime = 0.f;
		mShowingWait = true;
		onCursorPosChange();
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

		Float CurPosX = mScreenPos.x + mRealAlignOffset.x + mCurPos.x + PixelDensity::dpToPx( 1.f ) + mRealPadding.Left;
		Float CurPosY = mScreenPos.y + mRealAlignOffset.y + mCurPos.y + mRealPadding.Top;

		if ( CurPosX > (Float)mScreenPos.x + (Float)mSize.x )
			CurPosX = (Float)mScreenPos.x + (Float)mSize.x;

		P.drawLine( Line2f( Vector2f( CurPosX, CurPosY ), Vector2f( CurPosX, CurPosY + mTextCache->getFont()->getLineSpacing( mTextCache->getCharacterSizePx() ) ) ) );

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

		textCache.setString( mTextBuffer.getBuffer().substr( NLPos, mTextBuffer.getCursorPos() - NLPos ) );

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
	if ( ( mFlags & UI_AUTO_SIZE ) && 0 == mDpSize.getHeight() ) {
		setSize( mDpSize.x, getSkinSize().getHeight() + mRealPadding.Top + mRealPadding.Bottom );
	}

	if ( mLayoutHeightRules == WRAP_CONTENT ) {
		int minHeight = eemax<int>( mTextCache->getTextHeight(), PixelDensity::dpToPxI( getSkinSize().getHeight() ) );
		setInternalPixelsHeight( minHeight + mRealPadding.Top + mRealPadding.Bottom );
	}
}

void UITextInput::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		setPadding( makePadding( true, true, false, false ) );
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

Uint32 UITextInput::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( controlPos );
		controlPos = PixelDensity::dpToPx( controlPos ) - mRealAlignOffset;

		Int32 curPos = mTextCache->findCharacterFromPos( Vector2i( controlPos.x, controlPos.y ) );

		if ( -1 != curPos ) {
			mTextBuffer.setCursorPos( curPos );
			resetWaitCursor();
		}
	}

	return UITextView::onMouseClick( Pos, Flags );
}

Uint32 UITextInput::onMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	UITextView::onMouseDoubleClick( Pos, Flags );

	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && selCurEnd() != -1 ) {
		mTextBuffer.setCursorPos( selCurEnd() );
		resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UINode::onMouseExit( Pos, Flags );

	if ( NULL != mSceneNode )
		mSceneNode->setCursor( EE_CURSOR_ARROW );

	return 1;
}

void UITextInput::selCurInit( const Int32& init ) {
	mTextBuffer.selCurInit( init );
}

void UITextInput::selCurEnd( const Int32& end ) {
	mTextBuffer.selCurEnd( end );

	if ( mTextBuffer.selCurEnd() != mTextBuffer.selCurInit() ) {
		mTextBuffer.setCursorPos( end );
	}
}

Int32 UITextInput::selCurInit() {
	return mTextBuffer.selCurInit();
}

Int32 UITextInput::selCurEnd() {
	return mTextBuffer.selCurEnd();
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

bool UITextInput::setAttribute( const NodeAttribute& attribute ) {
	const std::string& name = attribute.getName();

	if ( "text" == name ) {
		if ( NULL != mSceneNode && mSceneNode->isUISceneNode() ) {
			setText( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( attribute.asString() ) );
		}
	} else if ( "allowediting" == name ) {
		setAllowEditing( attribute.asBool() );
	} else if ( "maxlength" == name ) {
		setMaxLength( attribute.asUint() );
	} else if ( "freeediting" == name ) {
		setFreeEditing( attribute.asBool() );
	} else if ( "onlynumbers" == name ) {
		getInputTextBuffer()->setAllowOnlyNumbers( attribute.asBool(), getInputTextBuffer()->dotsInNumbersAllowed() );
	} else if ( "allowdot" == name ) {
		getInputTextBuffer()->setAllowOnlyNumbers( getInputTextBuffer()->onlyNumbersAllowed(), attribute.asBool() );
	} else {
		return UITextView::setAttribute( attribute );
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
