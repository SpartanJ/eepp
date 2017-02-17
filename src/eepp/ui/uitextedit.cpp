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
	mTextInput->visible( true );
	mTextInput->enabled( true );
	mTextInput->addEventListener( UIEvent::EventOnSizeChange		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( UIEvent::EventOnTextChanged		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( UIEvent::EventOnPressEnter		, cb::Make1( this, &UITextEdit::onInputSizeChange ) );
	mTextInput->addEventListener( UIEvent::EventOnCursorPosChange	, cb::Make1( this, &UITextEdit::onCursorPosChange ) );

	UIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.setParent( this );
	ScrollBarP.setPos( mSize.width() - 15, 0 );
	ScrollBarP.Size					= Sizei( 15, mSize.height() );
	ScrollBarP.Flags				= UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar	= true;
	mVScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );
	mVScrollBar->value( 1 );

	ScrollBarP.setPos( 0, mSize.height() - 15 );
	ScrollBarP.Size					= Sizei( mSize.width() - mVScrollBar->size().width(), 15 );
	ScrollBarP.VerticalScrollBar	= false;
	mHScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );

	mVScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UITextEdit::onVScrollValueChange ) );
	mHScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UITextEdit::onHScrollValueChange ) );

	autoPadding();

	onSizeChange();

	applyDefaultTheme();

	mTextInput->size( mSize - Sizei( mPadding.Left + mPadding.Right, mPadding.Top + mPadding.Bottom ) );
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
	mHScrollBar->position( 0, mSize.height() - mHScrollBar->size().height() );
	mVScrollBar->position( mSize.width() - mVScrollBar->size().width(), 0 );

	mHScrollBar->size( mSize.width(), mHScrollBar->size().height() );
	mVScrollBar->size( mVScrollBar->size().width(), mSize.height() );

	mTextInput->position( mPadding.Left, mPadding.Top );

	scrollbarsSet();

	fixScroll();
}

void UITextEdit::onParentSizeChange( const Vector2i& SizeChange ) {
	UIComplexControl::onParentSizeChange( SizeChange );

	onInputSizeChange( NULL );
}

void UITextEdit::onAlphaChange() {
	mTextInput->alpha( mAlpha );
	mHScrollBar->alpha( mAlpha );
	mVScrollBar->alpha( mAlpha );

	UIComplexControl::onAlphaChange();
}

void UITextEdit::fixScroll() {
	int Width		= mSize.width()		- mPadding.Left - mPadding.Right;
	int Height	= mSize.height()	- mPadding.Top	- mPadding.Bottom;

	if ( mHScrollBar->visible() )
		Height -= mHScrollBar->size().height();

	int diff;
	Float pos;

	if ( mTextInput->size().height() - Height >= 0 ) {
		diff = mTextInput->size().height() - Height;

		pos = diff * mVScrollBar->value();

		mTextInput->position( mTextInput->position().x, mPadding.Top - pos );
	}

	if ( mTextInput->size().width() - Width >= 0 ) {
		diff = mTextInput->size().width() - Width;

		pos = diff * mHScrollBar->value();

		mTextInput->position( mPadding.Left - pos, mTextInput->position().y );
	}
}

void UITextEdit::scrollbarsSet() {
	switch ( mHScrollBarMode ) {
		case UI_SCROLLBAR_ALWAYS_OFF:
		{
			mHScrollBar->visible( false );
			mHScrollBar->enabled( false );
			break;
		}
		case UI_SCROLLBAR_ALWAYS_ON:
		{
			mHScrollBar->visible( true );
			mHScrollBar->enabled( true );
			break;
		}
		case UI_SCROLLBAR_AUTO:
		{
			if ( mTextInput->getTextWidth() > mSize.width() - mPadding.Left - mPadding.Right ) {
				mHScrollBar->visible( true );
				mHScrollBar->enabled( true );
			} else {
				mHScrollBar->visible( false );
				mHScrollBar->enabled( false );
			}
			break;
		}
	}

	switch ( mVScrollBarMode ) {
		case UI_SCROLLBAR_ALWAYS_OFF:
		{
			mVScrollBar->visible( false );
			mVScrollBar->enabled( false );
			break;
		}
		case UI_SCROLLBAR_ALWAYS_ON:
		{
			mVScrollBar->visible( true );
			mVScrollBar->enabled( true );
			break;
		}
		case UI_SCROLLBAR_AUTO:
		{
			int extraH = 0;

			if ( mHScrollBar->visible() )
				extraH = mHScrollBar->size().height();

			if ( mTextInput->getTextHeight() > mSize.height() - mPadding.Top - mPadding.Bottom - extraH ) {
				mVScrollBar->visible( true );
				mVScrollBar->enabled( true );
			} else {
				mVScrollBar->visible( false );
				mVScrollBar->enabled( false );
			}
			break;
		}
	}

	if ( !mVScrollBar->visible() && mHScrollBar->visible() ) {
		mHScrollBar->size( mSize.width(), mHScrollBar->size().height() );
	} else {
		mHScrollBar->size( mSize.width() - mVScrollBar->size().width(), mHScrollBar->size().height() );
	}

	if ( UI_SCROLLBAR_AUTO == mHScrollBarMode && mVScrollBar->visible() && !mHScrollBar->visible() ) {
		if ( mTextInput->getTextWidth() > mSize.width() - mPadding.Left - mPadding.Right - mVScrollBar->size().width() ) {
			mHScrollBar->visible( true );
			mHScrollBar->enabled( true );
		}
	}

	if ( mFlags & UI_AUTO_SHRINK_TEXT ) {
		mVScrollBar->visible( true );
		mVScrollBar->enabled( true );
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

const String& UITextEdit::text() const {
	return mTextInput->text();
}

void UITextEdit::text( const String& Txt ) {
	mTextInput->text( Txt );

	onInputSizeChange();

	onSizeChange();
}

void UITextEdit::onInputSizeChange( const UIEvent * Event ) {
	int Width		= mSize.width()		- mPadding.Left - mPadding.Right;
	int Height	= mSize.height()	- mPadding.Top	- mPadding.Bottom;

	if ( NULL != Event ) {
		if ( Event->getEventType() == UIEvent::EventOnPressEnter ) {
			mHScrollBar->value( 0 );
		}
	}

	scrollbarsSet();

	if ( mHScrollBar->visible() )
		Height	-= mHScrollBar->size().height();

	if ( mVScrollBar->visible() )
		Width	-= mVScrollBar->size().width();

	shrinkText( Width );

	if ( ( mFlags & UI_AUTO_SHRINK_TEXT ) && mTextInput->getTextHeight() < Height ) {
		mVScrollBar->visible( false );
		mVScrollBar->enabled( false );
	}

	if ( mTextInput->size().width() < Width || mTextInput->size().height() < Height ) {
		if ( mTextInput->size().width() < Width && mTextInput->size().height() < Height ) {
			mTextInput->size( Width, Height );
		} else {
			if ( mTextInput->size().width() < Width ) {
				mTextInput->size( Width, mTextInput->size().height() );
			} else {
				mTextInput->size( mTextInput->size().width(), Height );
			}
		}
	}

	if ( mTextInput->getTextWidth() > Width || mTextInput->getTextHeight() > Height ) {
		if ( mTextInput->getTextWidth() > Width && mTextInput->getTextHeight() > Height ) {
			mTextInput->size( mTextInput->getTextWidth(), mTextInput->getTextHeight() );
		} else {
			if ( mTextInput->getTextWidth() > Width ) {
				mTextInput->size( mTextInput->getTextWidth(), Height );
			} else {
				mTextInput->size( Width, mTextInput->getTextHeight() );
			}
		}
	} else {
		mTextInput->size( Width, Height );
	}

	fixScroll();
	fixScrollToCursor();
}

void UITextEdit::onCursorPosChange( const UIEvent * Event ) {
	fixScrollToCursor();
}

void UITextEdit::fixScrollToCursor() {
	int Width		= mSize.width()		- mPadding.Left - mPadding.Right;
	int Height	= mSize.height()	- mPadding.Top	- mPadding.Bottom;

	if ( mVScrollBar->visible() )
		Width -= mVScrollBar->size().width();

	if ( mHScrollBar->visible() )
		Height -= mHScrollBar->size().height();

	if ( FontHAlignGet( mTextInput->flags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum = mTextInput->getInputTextBuffer()->getCurPosLinePos( NLPos );

		mTextInput->getTextCache()->font()->setText(
			mTextInput->getInputTextBuffer()->getBuffer().substr(
				NLPos, mTextInput->getInputTextBuffer()->getCursorPos() - NLPos
			)
		);

		mSkipValueChange = true;

		Float tW	= mTextInput->getTextCache()->font()->getTextWidth();
		Float tH	= (Float)(LineNum + 1) * (Float)mTextInput->getTextCache()->font()->getFontHeight();

		if ( tW > Width ) {
			mTextInput->position( mPadding.Left + Width - tW, mTextInput->position().y );
		} else {
			mTextInput->position( mPadding.Left, mTextInput->position().y );
		}

		if ( tH > Height ) {
			mTextInput->position( mTextInput->position().x, mPadding.Top + Height - tH );
		} else {
			mTextInput->position( mTextInput->position().x, mPadding.Top );
		}

		mHScrollBar->value( tW / mTextInput->size().width() );
		mVScrollBar->value( tH / mTextInput->size().height() );

		mSkipValueChange = false;
	}
}

void UITextEdit::shrinkText( const Uint32& Width ) {
	if ( flags() & UI_AUTO_SHRINK_TEXT ) {
		mTextInput->shrinkText( Width );
	}
}

void UITextEdit::update() {
	UIControlAnim::update();

	if ( mTextInput->enabled() && mTextInput->visible() && mTextInput->isMouseOver() && mVScrollBar->visible() ) {
		Uint32 Flags 			= UIManager::instance()->getInput()->getClickTrigger();

		if ( Flags & EE_BUTTONS_WUWD )
			mVScrollBar->getSlider()->manageClick( Flags );
	}
}

void UITextEdit::allowEditing( const bool& allow ) {
	mTextInput->allowEditing( allow );
}

const bool& UITextEdit::allowEditing() const {
	return mTextInput->allowEditing();
}

}}
