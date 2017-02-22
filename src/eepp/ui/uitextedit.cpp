#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/textcache.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace UI {

UITextEdit::UITextEdit( UITextEdit::CreateParams& Params ) :
	UIComplexControl( Params ),
	mTextInput( NULL ),
	mHScrollBar( NULL ),
	mVScrollBar( NULL ),
	mHScrollBarMode( Params.HScrollBar ),
	mVScrollBarMode( Params.VScrollBar ),
	mSkipValueChange( false )
{
	Uint32 extraFlags = 0;

	if ( mFlags & UI_ANCHOR_LEFT )
		extraFlags |= UI_ANCHOR_LEFT;

	if ( mFlags & UI_ANCHOR_RIGHT )
		extraFlags |= UI_ANCHOR_RIGHT;

	if ( mFlags & UI_ANCHOR_TOP )
		extraFlags |= UI_ANCHOR_TOP;

	if ( mFlags & UI_ANCHOR_BOTTOM )
		extraFlags |= UI_ANCHOR_BOTTOM;

	UITextInput::CreateParams TIParams;
	TIParams.setParent( this );
	TIParams.Size				= mSize;
	TIParams.Flags				= UI_VALIGN_TOP | UI_HALIGN_LEFT | UI_TEXT_SELECTION_ENABLED | extraFlags;
	TIParams.MaxLength			= 1024 * 1024 * 10;
	TIParams.Font				= Params.Font;
	TIParams.FontColor			= Params.FontColor;
	TIParams.FontShadowColor	= Params.FontShadowColor;

	if ( Params.WordWrap && !( mFlags & UI_AUTO_SHRINK_TEXT ) )
		mFlags |= UI_AUTO_SHRINK_TEXT;

	mTextInput	= eeNew( UITextInput, ( TIParams ) );
	mTextInput->getInputTextBuffer()->isNewLineEnabled( true );
	mTextInput->setVisible( true );
	mTextInput->setEnabled( true );
	mTextInput->addEventListener( UIEvent::EventOnSizeChange		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( UIEvent::EventOnTextChanged		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( UIEvent::EventOnPressEnter		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( UIEvent::EventOnCursorPosChange	, cb::Make1( this, &UITextEdit::onCursorPosChange ) );

	UIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.setParent( this );
	ScrollBarP.setPosition( mSize.getWidth() - 15, 0 );
	ScrollBarP.Size					= Sizei( 15, mSize.getHeight() );
	ScrollBarP.Flags				= UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar	= true;
	mVScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );
	mVScrollBar->setValue( 1 );

	ScrollBarP.setPosition( 0, mSize.getHeight() - 15 );
	ScrollBarP.Size					= Sizei( mSize.getWidth() - mVScrollBar->getSize().getWidth(), 15 );
	ScrollBarP.VerticalScrollBar	= false;
	mHScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );

	mVScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UITextEdit::onVScrollValueChange ) );
	mHScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UITextEdit::onHScrollValueChange ) );

	autoPadding();

	onSizeChange();

	applyDefaultTheme();

	mTextInput->setPixelsSize( mRealSize - Sizei( mPadding.Left + mPadding.Right, mPadding.Top + mPadding.Bottom ) );
}

UITextEdit::~UITextEdit() {
}

Uint32 UITextEdit::getType() const {
	return UI_TYPE_TEXTEDIT;
}

bool UITextEdit::isType( const Uint32& type ) const {
	return UITextEdit::getType() == type ? true : UIComplexControl::isType( type );
}

void UITextEdit::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "textedit" );

	mTextInput->setThemeControl( Theme, "textedit_box" );

	autoPadding();

	onSizeChange();
}

void UITextEdit::onSizeChange() {
	mHScrollBar->setPosition( 0, mSize.getHeight() - mHScrollBar->getSize().getHeight() );
	mVScrollBar->setPosition( mSize.getWidth() - mVScrollBar->getSize().getWidth(), 0 );

	mHScrollBar->setSize( mSize.getWidth(), mHScrollBar->getSize().getHeight() );
	mVScrollBar->setSize( mVScrollBar->getSize().getWidth(), mSize.getHeight() );

	mTextInput->setPosition( pxToDpI( mPadding.Left ), pxToDpI( mPadding.Top ) );

	scrollbarsSet();

	fixScroll();
}

void UITextEdit::onParentSizeChange( const Vector2i& SizeChange ) {
	UIComplexControl::onParentSizeChange( SizeChange );

	onInputSizeChange( NULL );
}

void UITextEdit::onAlphaChange() {
	mTextInput->setAlpha( mAlpha );
	mHScrollBar->setAlpha( mAlpha );
	mVScrollBar->setAlpha( mAlpha );

	UIComplexControl::onAlphaChange();
}

void UITextEdit::fixScroll() {
	int Width	= mRealSize.getWidth()	- mPadding.Left - mPadding.Right;
	int Height	= mRealSize.getHeight()	- mPadding.Top	- mPadding.Bottom;

	if ( mHScrollBar->isVisible() )
		Height -= mHScrollBar->getRealSize().getHeight();

	int diff;
	Float pos;

	if ( mTextInput->getRealSize().getHeight() - Height >= 0 ) {
		diff = mTextInput->getRealSize().getHeight() - Height;

		pos = diff * mVScrollBar->getValue();

		mTextInput->setPixelsPosition( mTextInput->getRealPosition().x, mPadding.Top - pos );
	}

	if ( mTextInput->getRealSize().getWidth() - Width >= 0 ) {
		diff = mTextInput->getRealSize().getWidth() - Width;

		pos = diff * mHScrollBar->getValue();

		mTextInput->setPixelsPosition( mPadding.Left - pos, mTextInput->getRealPosition().y );
	}
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
			if ( mTextInput->getTextWidth() > mRealSize.getWidth() - mPadding.Left - mPadding.Right ) {
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
				extraH = mHScrollBar->getRealSize().getHeight();

			if ( mTextInput->getTextHeight() > mRealSize.getHeight() - mPadding.Top - mPadding.Bottom - extraH ) {
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
		mHScrollBar->setSize( mSize.getWidth(), mHScrollBar->getSize().getHeight() );
	} else {
		mHScrollBar->setSize( mSize.getWidth() - mVScrollBar->getSize().getWidth(), mHScrollBar->getSize().getHeight() );
	}

	if ( UI_SCROLLBAR_AUTO == mHScrollBarMode && mVScrollBar->isVisible() && !mHScrollBar->isVisible() ) {
		if ( mTextInput->getTextWidth() > mRealSize.getWidth() - mPadding.Left - mPadding.Right - mVScrollBar->getRealSize().getWidth() ) {
			mHScrollBar->setVisible( true );
			mHScrollBar->setEnabled( true );
		}
	}

	if ( mFlags & UI_AUTO_SHRINK_TEXT ) {
		mVScrollBar->setVisible( true );
		mVScrollBar->setEnabled( true );
	}
}

void UITextEdit::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = makePadding();
	}
}

void UITextEdit::onVScrollValueChange( const UIEvent * Event ) {
	if ( !mSkipValueChange )
		fixScroll();
}

void UITextEdit::onHScrollValueChange( const UIEvent * Event ) {
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

void UITextEdit::onInputSizeChange( const UIEvent * Event ) {
	int Width	= mRealSize.getWidth()	- mPadding.Left - mPadding.Right;
	int Height	= mRealSize.getHeight()	- mPadding.Top	- mPadding.Bottom;

	if ( NULL != Event ) {
		if ( Event->getEventType() == UIEvent::EventOnPressEnter ) {
			mHScrollBar->setValue( 0 );
		}
	}

	scrollbarsSet();

	if ( mHScrollBar->isVisible() )
		Height	-= mHScrollBar->getRealSize().getHeight();

	if ( mVScrollBar->isVisible() )
		Width	-= mVScrollBar->getRealSize().getWidth();

	shrinkText( Width );

	if ( ( mFlags & UI_AUTO_SHRINK_TEXT ) && mTextInput->getTextHeight() < Height ) {
		mVScrollBar->setVisible( false );
		mVScrollBar->setEnabled( false );
	}

	if ( mTextInput->getRealSize().getWidth() < Width || mTextInput->getRealSize().getHeight() < Height ) {
		if ( mTextInput->getRealSize().getWidth() < Width && mTextInput->getRealSize().getHeight() < Height ) {
			mTextInput->setPixelsSize( Width, Height );
		} else {
			if ( mTextInput->getRealSize().getWidth() < Width ) {
				mTextInput->setPixelsSize( Width, mTextInput->getRealSize().getHeight() );
			} else {
				mTextInput->setPixelsSize( mTextInput->getRealSize().getWidth(), Height );
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

void UITextEdit::onCursorPosChange( const UIEvent * Event ) {
	fixScrollToCursor();
}

void UITextEdit::fixScrollToCursor() {
	int Width	= mRealSize.getWidth()	- mPadding.Left - mPadding.Right;
	int Height	= mRealSize.getHeight()	- mPadding.Top	- mPadding.Bottom;

	if ( mVScrollBar->isVisible() )
		Width -= mVScrollBar->getRealSize().getWidth();

	if ( mHScrollBar->isVisible() )
		Height -= mHScrollBar->getRealSize().getHeight();

	if ( fontHAlignGet( mTextInput->getFlags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum = mTextInput->getInputTextBuffer()->getCurPosLinePos( NLPos );

		mTextInput->getTextCache()->getFont()->setText(
			mTextInput->getInputTextBuffer()->getBuffer().substr(
				NLPos, mTextInput->getInputTextBuffer()->getCursorPos() - NLPos
			)
		);

		mSkipValueChange = true;

		Float tW	= mTextInput->getTextCache()->getFont()->getTextWidth();
		Float tH	= (Float)(LineNum + 1) * (Float)mTextInput->getTextCache()->getFont()->getFontHeight();

		if ( tW > Width ) {
			mTextInput->setPixelsPosition( mPadding.Left + Width - tW, mTextInput->getRealPosition().y );
		} else {
			mTextInput->setPixelsPosition( mPadding.Left, mTextInput->getRealPosition().y );
		}

		if ( tH > Height ) {
			mTextInput->setPixelsPosition( mTextInput->getRealPosition().x, mPadding.Top + Height - tH );
		} else {
			mTextInput->setPixelsPosition( mTextInput->getRealPosition().x, mPadding.Top );
		}

		mHScrollBar->setValue( tW / mTextInput->getRealSize().getWidth() );
		mVScrollBar->setValue( tH / mTextInput->getRealSize().getHeight() );

		mSkipValueChange = false;
	}
}

void UITextEdit::shrinkText( const Uint32& Width ) {
	if ( getFlags() & UI_AUTO_SHRINK_TEXT ) {
		mTextInput->shrinkText( Width );
	}
}

void UITextEdit::update() {
	UIControlAnim::update();

	if ( mTextInput->isEnabled() && mTextInput->isVisible() && mTextInput->isMouseOver() && mVScrollBar->isVisible() ) {
		Uint32 Flags 			= UIManager::instance()->getInput()->getClickTrigger();

		if ( Flags & EE_BUTTONS_WUWD )
			mVScrollBar->getSlider()->manageClick( Flags );
	}
}

void UITextEdit::setAllowEditing( const bool& allow ) {
	mTextInput->setAllowEditing( allow );
}

const bool& UITextEdit::getAllowEditing() const {
	return mTextInput->getAllowEditing();
}

}}
