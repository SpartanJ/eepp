#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace UI {

UITextEdit * UITextEdit::New() {
	return eeNew( UITextEdit, () );
}

UITextEdit::UITextEdit() :
	UIWidget( "textedit" ),
	mTextInput( NULL ),
	mHScrollBar( NULL ),
	mVScrollBar( NULL ),
	mHScrollBarMode( UI_SCROLLBAR_AUTO ),
	mVScrollBarMode( UI_SCROLLBAR_AUTO ),
	mSkipValueChange( false )
{
	setFlags( UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED );
	clipEnable();

	mTextInput	= UITextInput::New();
	mTextInput->setLayoutSizeRules( LayoutSizeRules::FIXED, LayoutSizeRules::FIXED );
	mTextInput->setParent( this );
	mTextInput->setFlags( UI_TEXT_SELECTION_ENABLED | UI_VALIGN_TOP );
	mTextInput->unsetFlags( UI_VALIGN_CENTER | UI_AUTO_SIZE );
	mTextInput->clipDisable();
	mTextInput->getInputTextBuffer()->isNewLineEnabled( true );
	mTextInput->setVisible( true );
	mTextInput->setEnabled( true );
	mTextInput->setSize( getSize() );

	mTextInput->addEventListener( Event::OnSizeChange		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( Event::OnTextChanged		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( Event::OnPressEnter		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( Event::OnCursorPosChange	, cb::Make1( this, &UITextEdit::onCursorPosChange ) );

	mVScrollBar = UIScrollBar::New();
	mVScrollBar->setOrientation( UI_VERTICAL );
	mVScrollBar->setParent( this );
	mVScrollBar->setPosition( getSize().getWidth() - 16, 0 );
	mVScrollBar->setSize( 16, getSize().getHeight() );
	mVScrollBar->setValue( 1 );

	mHScrollBar = UIScrollBar::New();
	mHScrollBar->setOrientation( UI_HORIZONTAL );
	mHScrollBar->setParent( this );
	mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 16 );
	mHScrollBar->setPosition( 0, getSize().getHeight() - 16 );

	mVScrollBar->addEventListener( Event::OnValueChange, cb::Make1( this, &UITextEdit::onVScrollValueChange ) );
	mHScrollBar->addEventListener( Event::OnValueChange, cb::Make1( this, &UITextEdit::onHScrollValueChange ) );

	autoPadding();

	applyDefaultTheme();
}


UITextEdit::~UITextEdit() {
}

Uint32 UITextEdit::getType() const {
	return UI_TYPE_TEXTEDIT;
}

bool UITextEdit::isType( const Uint32& type ) const {
	return UITextEdit::getType() == type ? true : UIWidget::isType( type );
}

void UITextEdit::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "textedit" );

	mTextInput->setThemeSkin( Theme, "textedit_box" );

	autoPadding();

	onSizeChange();
	onThemeLoaded();
}

void UITextEdit::onSizeChange() {
	mHScrollBar->setPosition( 0, getSize().getHeight() - mHScrollBar->getSize().getHeight() );
	mVScrollBar->setPosition( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 0 );

	mHScrollBar->setSize( getSize().getWidth(), mHScrollBar->getSize().getHeight() );
	mVScrollBar->setSize( mVScrollBar->getSize().getWidth(), getSize().getHeight() );

	mTextInput->setPixelsPosition( mContainerPadding.Left, mContainerPadding.Top );

	onInputSizeChange( NULL );

	scrollbarsSet();

	fixScroll();

	UIWidget::onSizeChange();
}

void UITextEdit::onParentSizeChange( const Vector2f& SizeChange ) {
	UIWidget::onParentSizeChange( SizeChange );

	onInputSizeChange( NULL );
}

void UITextEdit::onPaddingChange() {
	autoPadding();

	mContainerPadding += mRealPadding;

	onSizeChange();

	UIWidget::onPaddingChange();
}

void UITextEdit::onAlphaChange() {
	mTextInput->setAlpha( mAlpha );
	mHScrollBar->setAlpha( mAlpha );
	mVScrollBar->setAlpha( mAlpha );

	UIWidget::onAlphaChange();
}

void UITextEdit::scrollbarsSet() {
	switch ( mHScrollBarMode ) {
		case UI_SCROLLBAR_ALWAYS_OFF:
		{
			mHScrollBar->setVisible( false );
			mHScrollBar->setEnabled( false );
			break;
		}
		case UI_SCROLLBAR_ALWAYS_ON:
		{
			mHScrollBar->setVisible( true );
			mHScrollBar->setEnabled( true );
			break;
		}
		case UI_SCROLLBAR_AUTO:
		{
			if ( mTextInput->getPixelsSize().getWidth() > mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right ) {
				mHScrollBar->setVisible( true );
				mHScrollBar->setEnabled( true );
			} else {
				mHScrollBar->setVisible( false );
				mHScrollBar->setEnabled( false );
			}
			break;
		}
	}

	switch ( mVScrollBarMode ) {
		case UI_SCROLLBAR_ALWAYS_OFF:
		{
			mVScrollBar->setVisible( false );
			mVScrollBar->setEnabled( false );
			break;
		}
		case UI_SCROLLBAR_ALWAYS_ON:
		{
			mVScrollBar->setVisible( true );
			mVScrollBar->setEnabled( true );
			break;
		}
		case UI_SCROLLBAR_AUTO:
		{
			int extraH = 0;

			if ( mHScrollBar->isVisible() )
				extraH = mHScrollBar->getPixelsSize().getHeight();

			if ( mTextInput->getTextHeight() > mSize.getHeight() - mContainerPadding.Top - mContainerPadding.Bottom - extraH ) {
				mVScrollBar->setVisible( true );
				mVScrollBar->setEnabled( true );
			} else {
				mVScrollBar->setVisible( false );
				mVScrollBar->setEnabled( false );
			}
			break;
		}
	}

	if ( !mVScrollBar->isVisible() && mHScrollBar->isVisible() ) {
		mHScrollBar->setSize( getSize().getWidth(), mHScrollBar->getSize().getHeight() );
	} else {
		mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(), mHScrollBar->getSize().getHeight() );
	}

	if ( UI_SCROLLBAR_AUTO == mHScrollBarMode && mVScrollBar->isVisible() && !mHScrollBar->isVisible() ) {
		if ( mTextInput->getTextWidth() > mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right - mVScrollBar->getPixelsSize().getWidth() ) {
			mHScrollBar->setVisible( true );
			mHScrollBar->setEnabled( true );
		}
	}

	mSkipValueChange = true;

	if ( mVScrollBar->isVisible() ) {
		int extraH = 0;

		if ( mHScrollBar->isVisible() )
			extraH = mHScrollBar->getPixelsSize().getHeight();

		Int32 totH = mSize.getHeight() - mContainerPadding.Top - mContainerPadding.Bottom - extraH;

		if ( mTextInput->getTextHeight() > totH ) {
			mVScrollBar->setPageStep( (Float)totH / (Float)mTextInput->getTextHeight() );
		}
	}

	if ( mHScrollBar->isVisible() ) {
		Int32 totW = mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right - mVScrollBar->getPixelsSize().getWidth();

		if ( mTextInput->getTextWidth() > totW && 0 != mTextInput->getTextWidth() ) {
			mHScrollBar->setPageStep( (Float)totW / (Float)mTextInput->getTextWidth() );
		}
	}

	mSkipValueChange = false;

	invalidateDraw();
}

void UITextEdit::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mContainerPadding = PixelDensity::dpToPx( makePadding() );
	}
}

void UITextEdit::onVScrollValueChange( const Event * ) {
	if ( !mSkipValueChange )
		fixScroll();
}

void UITextEdit::onHScrollValueChange( const Event * ) {
	if ( !mSkipValueChange )
		fixScroll();
}

UITextInput * UITextEdit::getTextInput() const {
	return mTextInput;
}

UIScrollBar * UITextEdit::getHScrollBar() const {
	return mHScrollBar;
}

UIScrollBar * UITextEdit::getVScrollBar() const {
	return mVScrollBar;
}

const String& UITextEdit::getText() const {
	return mTextInput->getText();
}

void UITextEdit::setText( const String& Txt ) {
	mTextInput->setText( Txt );

	onInputSizeChange();

	onSizeChange();
}

void UITextEdit::onInputSizeChange( const Event * Event ) {
	if ( mNodeFlags & NODE_FLAG_FREE_USE )
		return;

	int Width	= mSize.getWidth()	- mContainerPadding.Left - mContainerPadding.Right;
	int Height	= mSize.getHeight()	- mContainerPadding.Top	- mContainerPadding.Bottom;

	if ( NULL != Event ) {
		if ( Event->getType() == Event::OnPressEnter ) {
			mHScrollBar->setValue( 0 );
		}
	}

	scrollbarsSet();

	if ( mHScrollBar->isVisible() )
		Height	-= mHScrollBar->getPixelsSize().getHeight();

	String text( mTextInput->getInputTextBuffer()->getBuffer() );

	shrinkText( Width );

	if ( mTextInput->getTextHeight() > Height ) {
		Width	-= mVScrollBar->getPixelsSize().getWidth();

		mNodeFlags |= NODE_FLAG_FREE_USE;
		mTextInput->getInputTextBuffer()->setBuffer( text );
		mNodeFlags &= ~NODE_FLAG_FREE_USE;

		shrinkText( Width );

		scrollbarsSet();
	}

	if ( ( mFlags & UI_WORD_WRAP ) && mTextInput->getTextHeight() < Height ) {
		mVScrollBar->setVisible( false );
		mVScrollBar->setEnabled( false );
	}

	if ( mTextInput->getPixelsSize().getWidth() < Width || mTextInput->getPixelsSize().getHeight() < Height ) {
		if ( mTextInput->getPixelsSize().getWidth() < Width && mTextInput->getPixelsSize().getHeight() < Height ) {
			mTextInput->setPixelsSize( Width, Height );
		} else {
			if ( mTextInput->getPixelsSize().getWidth() < Width ) {
				mTextInput->setPixelsSize( Width, mTextInput->getPixelsSize().getHeight() );
			} else {
				mTextInput->setPixelsSize( mTextInput->getPixelsSize().getWidth(), Height );
			}
		}
	}

	if ( mTextInput->getTextWidth() > Width || mTextInput->getTextHeight() > Height ) {
		if ( mTextInput->getTextWidth() > Width && mTextInput->getTextHeight() > Height ) {
			mTextInput->setPixelsSize( mTextInput->getTextWidth(), mTextInput->getTextHeight() );
		} else {
			if ( mTextInput->getTextWidth() > Width ) {
				mTextInput->setPixelsSize( mTextInput->getTextWidth(), Height );
			} else {
				mTextInput->setPixelsSize( Width, mTextInput->getTextHeight() );
			}
		}
	} else {
		mTextInput->setPixelsSize( Width, Height );
	}

	fixScroll();
	fixScrollToCursor();
}

void UITextEdit::onCursorPosChange( const Event * ) {
	fixScrollToCursor();
}

void UITextEdit::fixScroll() {
	int Width	= mSize.getWidth()	- mContainerPadding.Left - mContainerPadding.Right;
	int Height	= mSize.getHeight()	- mContainerPadding.Top	- mContainerPadding.Bottom;

	if ( mVScrollBar->isVisible() )
		Width -= mVScrollBar->getPixelsSize().getWidth();

	if ( mHScrollBar->isVisible() )
		Height -= mHScrollBar->getPixelsSize().getHeight();

	int diff;
	Float pos;

	diff = mTextInput->getPixelsSize().getHeight() - Height;
	if ( diff >= 0 ) {
		pos = diff * mVScrollBar->getValue();

		mTextInput->setPixelsPosition( mTextInput->getPixelsPosition().x, mContainerPadding.Top - pos );
	}

	diff = mTextInput->getPixelsSize().getWidth() - Width;
	if ( diff >= 0 ) {
		pos = diff * mHScrollBar->getValue();

		mTextInput->setPixelsPosition( mContainerPadding.Left - pos, mTextInput->getPixelsPosition().y );
	}

	invalidateDraw();
}

void UITextEdit::fixScrollToCursor() {
	if ( fontHAlignGet( mTextInput->getFlags() ) == UI_HALIGN_LEFT ) {
		int Width	= mSize.getWidth()	- mContainerPadding.Left - mContainerPadding.Right;
		int Height	= mSize.getHeight()	- mContainerPadding.Top	- mContainerPadding.Bottom;

		if ( mVScrollBar->isVisible() )
			Width -= mVScrollBar->getPixelsSize().getWidth();

		if ( mHScrollBar->isVisible() )
			Height -= mHScrollBar->getPixelsSize().getHeight();

		Uint32 NLPos	= 0;
		Uint32 LineNum = mTextInput->getInputTextBuffer()->getCurPosLinePos( NLPos );

		Text textCache( mTextInput->getFont(), mTextInput->getFontStyleConfig().getFontCharacterSize() );
		textCache.setString(
			mTextInput->getInputTextBuffer()->getBuffer().substr(
				NLPos, mTextInput->getInputTextBuffer()->getCursorPosition() - NLPos
			)
		);

		mSkipValueChange = true;

		Float lineHeight = (Float)textCache.getFont()->getLineSpacing( textCache.getCharacterSizePx() );
		Float currentLineY	= LineNum * lineHeight;
		Float visibleLines = eefloor( Height / lineHeight );
		Float scrollLines = (Float)mTextInput->getNumLines() - visibleLines;

		if ( mTextInput->getNumLines() > 0 ) {
			if ( mTextInput->getPixelsPosition().y + currentLineY < 0 ) {
				mVScrollBar->setValue( LineNum / scrollLines );
			} else if ( mTextInput->getPixelsPosition().y + currentLineY + lineHeight > Height ) {
				mVScrollBar->setValue( (LineNum + 1 - visibleLines) / scrollLines );
			}
		}

		mHScrollBar->setValue( textCache.getTextWidth() / mTextInput->getPixelsSize().getWidth() );

		fixScroll();

		mSkipValueChange = false;
	}

	invalidateDraw();
}

void UITextEdit::shrinkText( const Uint32& Width ) {
	if ( getFlags() & UI_WORD_WRAP ) {
		mTextInput->shrinkText( Width );
	}
}

Uint32 UITextEdit::onMessage(const NodeMessage * Msg) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp:
		{
			if ( NULL != getEventDispatcher() && mTextInput->isEnabled() && mTextInput->isVisible() && mTextInput->isMouseOver() && mVScrollBar->isVisible() ) {
				if ( Msg->getFlags() & EE_BUTTONS_WUWD )
					mVScrollBar->getSlider()->manageClick( Msg->getFlags() );
			}

			return 1;
		}
	}

	return 0;
}

void UITextEdit::setAllowEditing( const bool& allow ) {
	mTextInput->setAllowEditing( allow );
	invalidateDraw();
}

const bool& UITextEdit::isEditingAllowed() const {
	return mTextInput->getAllowEditing();
}

void UITextEdit::setVerticalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mVScrollBarMode ) {
		mVScrollBarMode = Mode;

		scrollbarsSet();
	}
}

const UI_SCROLLBAR_MODE& UITextEdit::getVerticalScrollMode() {
	return mVScrollBarMode;
}

void UITextEdit::setHorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mHScrollBarMode ) {
		mHScrollBarMode = Mode;

		scrollbarsSet();
	}
}

const UI_SCROLLBAR_MODE& UITextEdit::getHorizontalScrollMode() {
	return mHScrollBarMode;
}

UIFontStyleConfig UITextEdit::getFontStyleConfig() const {
	return mTextInput->getFontStyleConfig();
}

bool UITextEdit::applyProperty( const StyleSheetProperty& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	bool attributeSet = true;

	if ( "text" == name ) {
		if ( NULL != mSceneNode && mSceneNode->isUISceneNode() ) {
			setText( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( attribute.asString() ) );
		}
	} else if ( "allow-editing" == name || "allowediting" == name ) {
		setAllowEditing( attribute.asBool() );
	} else if ( "vscroll-mode" == name || "vscrollmode" == name ) {
		std::string val = attribute.asString();
		if ( "auto" == val ) setVerticalScrollMode( UI_SCROLLBAR_AUTO );
		else if ( "on" == val ) setVerticalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
		else if ( "off" == val ) setVerticalScrollMode( UI_SCROLLBAR_ALWAYS_OFF );
	} else if ( "hscroll-mode" == name || "hscrollmode" == name ) {
		std::string val = attribute.asString();
		if ( "auto" == val ) setHorizontalScrollMode( UI_SCROLLBAR_AUTO );
		else if ( "on" == val ) setHorizontalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
		else if ( "off" == val ) setHorizontalScrollMode( UI_SCROLLBAR_ALWAYS_OFF );
	} else {
		attributeSet = UIWidget::applyProperty( attribute, state );
	}

	if ( !attributeSet && ( String::startsWith( name, "text" ) || String::startsWith( name, "font" ) ) )
		mTextInput->applyProperty( attribute, state );

	return attributeSet;
}

}}
