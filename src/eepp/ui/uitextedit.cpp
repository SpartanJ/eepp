#include <eepp/graphics/font.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitextedit.hpp>

namespace EE { namespace UI {

UITextEdit* UITextEdit::New() {
	return eeNew( UITextEdit, () );
}

UITextEdit::UITextEdit() :
	UIWidget( "textedit" ),
	mTextInput( NULL ),
	mHScrollBar( NULL ),
	mVScrollBar( NULL ),
	mHScrollBarMode( ScrollBarMode::Auto ),
	mVScrollBarMode( ScrollBarMode::Auto ),
	mSkipValueChange( false ) {
	setFlags( UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED );
	clipEnable();

	mTextInput = UITextInput::NewWithTag( mTag + "::input" );
	mTextInput->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mTextInput->setParent( this );
	mTextInput->setFlags( UI_TEXT_SELECTION_ENABLED | UI_VALIGN_TOP );
	mTextInput->unsetFlags( UI_VALIGN_CENTER | UI_AUTO_SIZE );
	mTextInput->clipDisable();
	//mTextInput->getInputTextBuffer()->isNewLineEnabled( true );
	mTextInput->setVisible( true );
	mTextInput->setEnabled( true );
	mTextInput->setSize( getSize() );

	auto cb = [&]( const Event* event ) { onInputSizeChange( event ); };

	mTextInput->addEventListener( Event::OnSizeChange, cb );
	mTextInput->addEventListener( Event::OnTextChanged, cb );
	mTextInput->addEventListener( Event::OnPressEnter, cb );
	mTextInput->addEventListener( Event::OnCursorPosChange,
								  cb::Make1( this, &UITextEdit::onCursorPosChange ) );
	mTextInput->addEventListener( Event::OnFocus, [&]( const Event* ) { onFocus(); } );
	mTextInput->addEventListener( Event::OnFocusLoss, [&]( const Event* ) { onFocusLoss(); } );

	mVScrollBar = UIScrollBar::NewVertical();
	mVScrollBar->setParent( this );
	mVScrollBar->setSize( 0, getSize().getHeight() );
	mVScrollBar->setPosition( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 0 );
	mVScrollBar->setValue( 1 );

	mHScrollBar = UIScrollBar::NewHorizontal();
	mHScrollBar->setParent( this );
	mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 0 );
	mHScrollBar->setPosition( 0, getSize().getHeight() - mHScrollBar->getSize().getHeight() );

	mVScrollBar->addEventListener( Event::OnValueChange,
								   cb::Make1( this, &UITextEdit::onVScrollValueChange ) );
	mHScrollBar->addEventListener( Event::OnValueChange,
								   cb::Make1( this, &UITextEdit::onHScrollValueChange ) );

	autoPadding();

	applyDefaultTheme();
}

UITextEdit::~UITextEdit() {}

Uint32 UITextEdit::getType() const {
	return UI_TYPE_TEXTEDIT;
}

bool UITextEdit::isType( const Uint32& type ) const {
	return UITextEdit::getType() == type ? true : UIWidget::isType( type );
}

void UITextEdit::setTheme( UITheme* Theme ) {
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

	mTextInput->setPosition( mContainerPadding.Left, mContainerPadding.Top );

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
	mContainerPadding = Rectf();
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
		case ScrollBarMode::AlwaysOff: {
			mHScrollBar->setVisible( false );
			mHScrollBar->setEnabled( false );
			break;
		}
		case ScrollBarMode::AlwaysOn: {
			mHScrollBar->setVisible( true );
			mHScrollBar->setEnabled( true );
			break;
		}
		case ScrollBarMode::Auto: {
			if ( mTextInput->getPixelsSize().getWidth() >
				 mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right ) {
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
		case ScrollBarMode::AlwaysOff: {
			mVScrollBar->setVisible( false );
			mVScrollBar->setEnabled( false );
			break;
		}
		case ScrollBarMode::AlwaysOn: {
			mVScrollBar->setVisible( true );
			mVScrollBar->setEnabled( true );
			break;
		}
		case ScrollBarMode::Auto: {
			int extraH = 0;

			if ( mHScrollBar->isVisible() )
				extraH = mHScrollBar->getPixelsSize().getHeight();

			if ( mTextInput->getPixelsSize().getHeight() >
				 mSize.getHeight() - mContainerPadding.Top - mContainerPadding.Bottom - extraH ) {
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
		mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(),
							  mHScrollBar->getSize().getHeight() );
	}

	if ( ScrollBarMode::Auto == mHScrollBarMode && mVScrollBar->isVisible() &&
		 !mHScrollBar->isVisible() ) {
		if ( mTextInput->getPixelsSize().getWidth() >
			 mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right -
				 mVScrollBar->getPixelsSize().getWidth() ) {
			mHScrollBar->setVisible( true );
			mHScrollBar->setEnabled( true );
		}
	}

	mSkipValueChange = true;

	if ( mVScrollBar->isVisible() ) {
		Float extraH = mHScrollBar->isVisible() ? mHScrollBar->getPixelsSize().getHeight() : 0;
		Float totH = eefloor( mSize.getHeight() - mContainerPadding.Top - mContainerPadding.Bottom -
							  extraH );

		if ( mTextInput->getTextHeight() > totH ) {
			mVScrollBar->setPageStep( (Float)totH / (Float)mTextInput->getTextHeight() );
		}
	}

	if ( mHScrollBar->isVisible() ) {
		Float extraW = mVScrollBar->isVisible() ? mVScrollBar->getPixelsSize().getWidth() : 0;
		Float totW =
			eefloor( mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right - extraW );

		if ( mTextInput->getPixelsSize().getWidth() > totW &&
			 0 != mTextInput->getPixelsSize().getWidth() ) {
			mHScrollBar->setPageStep( (Float)totW / mTextInput->getPixelsSize().getWidth() );
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

void UITextEdit::onVScrollValueChange( const Event* ) {
	if ( !mSkipValueChange )
		fixScroll();
}

void UITextEdit::onHScrollValueChange( const Event* ) {
	if ( !mSkipValueChange )
		fixScroll();
}

UITextInput* UITextEdit::getTextInput() const {
	return mTextInput;
}

UIScrollBar* UITextEdit::getHScrollBar() const {
	return mHScrollBar;
}

UIScrollBar* UITextEdit::getVScrollBar() const {
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

void UITextEdit::onInputSizeChange( const Event* Event ) {
	scrollbarsSet();

	if ( mNodeFlags & NODE_FLAG_FREE_USE )
		return;

	mNodeFlags |= NODE_FLAG_FREE_USE;

	Sizef aSize( getAvailableSize() );
	Sizef iSize( getInputSize() );

	if ( NULL != Event ) {
		if ( Event->getType() == Event::OnPressEnter ) {
			mHScrollBar->setValue( 0 );
		}
	}

	if ( mHScrollBar->isVisible() )
		aSize.y -= mHScrollBar->getPixelsSize().getHeight();

	/*String text( mTextInput->getInputTextBuffer()->getBuffer() );

	shrinkText( aSize.getWidth() );

	if ( iSize.getHeight() > aSize.getHeight() ) {
		aSize.x -= mVScrollBar->getPixelsSize().getWidth();

		mTextInput->getInputTextBuffer()->setBuffer( text );

		shrinkText( aSize.getWidth() );
	}*/

	textInputTryResize( aSize, iSize );

	fixScroll();
	fixScrollToCursor();

	mNodeFlags &= ~NODE_FLAG_FREE_USE;
}

void UITextEdit::textInputTryResize( Sizef aSize, Sizef iSize ) {
	if ( ( mFlags & UI_WORD_WRAP ) && iSize.getHeight() < aSize.getHeight() ) {
		mVScrollBar->setVisible( false );
		mVScrollBar->setEnabled( false );
	}

	if ( mTextInput->getPixelsSize().getWidth() < aSize.getWidth() ||
		 mTextInput->getPixelsSize().getHeight() < aSize.getHeight() ) {
		if ( mTextInput->getPixelsSize().getWidth() < aSize.getWidth() &&
			 mTextInput->getPixelsSize().getHeight() < aSize.getHeight() ) {
			mTextInput->setPixelsSize( aSize );
		} else {
			if ( mTextInput->getPixelsSize().getWidth() < aSize.getWidth() ) {
				mTextInput->setPixelsSize( aSize.getWidth(),
										   mTextInput->getPixelsSize().getHeight() );
			} else {
				mTextInput->setPixelsSize( mTextInput->getPixelsSize().getWidth(),
										   aSize.getHeight() );
			}
		}
	}

	if ( iSize.getWidth() > aSize.getWidth() || iSize.getHeight() > aSize.getHeight() ) {
		if ( iSize.getWidth() > aSize.getWidth() && iSize.getHeight() > aSize.getHeight() ) {
			mTextInput->setPixelsSize( iSize );
		} else {
			if ( iSize.getWidth() > aSize.getWidth() ) {
				mTextInput->setPixelsSize( iSize.getWidth(), aSize.getHeight() );
			} else {
				mTextInput->setPixelsSize( aSize.getWidth(), iSize.getHeight() );
			}
		}
	} else {
		mTextInput->setPixelsSize( aSize );
	}
}

Sizef UITextEdit::getAvailableSize() {
	Float width = mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right;
	Float height = mSize.getHeight() - mContainerPadding.Top - mContainerPadding.Bottom;
	return Sizef( width, height );
}

Sizef UITextEdit::getInputSize() {
	Float width = mTextInput->getTextWidth() + mTextInput->getPixelsPadding().Left +
				  mTextInput->getPixelsPadding().Right;
	Float height = mTextInput->getTextHeight() + mTextInput->getPixelsPadding().Top +
				   mTextInput->getPixelsPadding().Bottom;
	return Sizef( width, height );
}

void UITextEdit::onCursorPosChange( const Event* ) {
	fixScrollToCursor();
}

void UITextEdit::fixScroll() {
	Sizef aSize( getAvailableSize() );

	if ( mVScrollBar->isVisible() )
		aSize.x -= mVScrollBar->getPixelsSize().getWidth();

	if ( mHScrollBar->isVisible() )
		aSize.y -= mHScrollBar->getPixelsSize().getHeight();

	Float diff;
	Float pos;

	diff = mTextInput->getPixelsSize().getHeight() - aSize.getHeight();
	if ( diff >= 0 ) {
		pos = diff * mVScrollBar->getValue();

		mTextInput->setPixelsPosition( mTextInput->getPixelsPosition().x,
									   mContainerPadding.Top - pos );
	}

	diff = mTextInput->getPixelsSize().getWidth() - aSize.getWidth();
	if ( diff >= 0 ) {
		pos = diff * mHScrollBar->getValue();

		mTextInput->setPixelsPosition( mContainerPadding.Left - pos,
									   mTextInput->getPixelsPosition().y );
	}
}

void UITextEdit::fixScrollToCursor() {
	/*if ( Font::getHorizontalAlign( mTextInput->getFlags() ) == UI_HALIGN_LEFT ) {
		Sizef aSize( getAvailableSize() );

		if ( mVScrollBar->isVisible() )
			aSize.x -= mVScrollBar->getPixelsSize().getWidth();

		if ( mHScrollBar->isVisible() )
			aSize.y -= mHScrollBar->getPixelsSize().getHeight();

		Uint32 NLPos = 0;
		Uint32 LineNum = mTextInput->getInputTextBuffer()->getCurPosLinePos( NLPos );

		Text textCache( mTextInput->getFont(),
						mTextInput->getFontStyleConfig().getFontCharacterSize() );
		textCache.setString( mTextInput->getInputTextBuffer()->getBuffer().substr(
			NLPos, mTextInput->getInputTextBuffer()->getCursorPosition() - NLPos ) );

		mSkipValueChange = true;

		Float lineHeight =
			(Float)textCache.getFont()->getLineSpacing( textCache.getCharacterSizePx() );
		Float currentLineY = LineNum * lineHeight;
		Float visibleLines = eefloor( aSize.getHeight() / lineHeight );
		Float scrollLines = (Float)mTextInput->getNumLines() - visibleLines;

		if ( mTextInput->getNumLines() > 0 ) {
			if ( mTextInput->getPixelsPosition().y + currentLineY < 0 ) {
				mVScrollBar->setValue( LineNum / scrollLines );
			} else if ( mTextInput->getPixelsPosition().y + currentLineY + lineHeight >
						aSize.getHeight() ) {
				mVScrollBar->setValue( ( LineNum + 1 - visibleLines ) / scrollLines );
			}
		}

		Vector2f cursorPos( mTextInput->getPixelsPadding().Left + textCache.getTextWidth(),
							mTextInput->getPixelsPadding().Top + currentLineY );

		Float tW = mTextInput->getPixelsPosition().x + cursorPos.x;
		Float contDiff = mTextInput->getPixelsSize().getWidth() - aSize.getWidth();

		if ( contDiff != 0 ) {
			if ( tW <= 0.f ) {
				if ( mTextInput->getInputTextBuffer()->getCursorPosition() - NLPos == 0 ) {
					mHScrollBar->setValue( 0 );
				} else {
					mHScrollBar->setValue( ( cursorPos.x - mTextInput->getPixelsPadding().Left ) /
										   contDiff );
				}
			} else if ( tW >= aSize.getWidth() ) {
				mHScrollBar->setValue( ( cursorPos.x - aSize.getWidth() ) / contDiff );
			}
		}

		fixScroll();

		mSkipValueChange = false;
	}*/

	invalidateDraw();
}

void UITextEdit::shrinkText( const Float& width ) {
	if ( ( getFlags() & UI_WORD_WRAP ) && width > 0 ) {
		mTextInput->shrinkText( width );
		textInputTryResize( getAvailableSize(), getInputSize() );
		scrollbarsSet();
	}
}

Uint32 UITextEdit::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp: {
			if ( NULL != getEventDispatcher() && mTextInput->isEnabled() &&
				 mTextInput->isVisible() && mTextInput->isMouseOver() &&
				 mVScrollBar->isVisible() ) {
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
	return mTextInput->isEditingAllowed();
}

void UITextEdit::setVerticalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mVScrollBarMode ) {
		mVScrollBarMode = Mode;

		scrollbarsSet();
	}
}

const ScrollBarMode& UITextEdit::getVerticalScrollMode() {
	return mVScrollBarMode;
}

void UITextEdit::setHorizontalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mHScrollBarMode ) {
		mHScrollBarMode = Mode;

		scrollbarsSet();
	}
}

const ScrollBarMode& UITextEdit::getHorizontalScrollMode() {
	return mHScrollBarMode;
}

UIFontStyleConfig UITextEdit::getFontStyleConfig() const {
	return mTextInput->getFontStyleConfig();
}

std::string UITextEdit::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
			return getText().toUtf8();
		case PropertyId::AllowEditing:
			return isEditingAllowed() ? "true" : "false";
		case PropertyId::VScrollMode:
			return getVerticalScrollMode() == ScrollBarMode::Auto
					   ? "auto"
					   : ( getVerticalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off" );
		case PropertyId::HScrollMode:
			return getHorizontalScrollMode() == ScrollBarMode::Auto
					   ? "auto"
					   : ( getHorizontalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off" );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UITextEdit::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	bool attributeSet = true;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() ) {
				setText( static_cast<UISceneNode*>( mSceneNode )
							 ->getTranslatorString( attribute.asString() ) );
			}
			break;
		case PropertyId::AllowEditing:
			setAllowEditing( attribute.asBool() );
			break;
		case PropertyId::VScrollMode: {
			std::string val = attribute.asString();
			if ( "auto" == val )
				setVerticalScrollMode( ScrollBarMode::Auto );
			else if ( "on" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOff );
			break;
		}
		case PropertyId::HScrollMode: {
			std::string val = attribute.asString();
			if ( "auto" == val )
				setHorizontalScrollMode( ScrollBarMode::Auto );
			else if ( "on" == val )
				setHorizontalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
			break;
		}
		case PropertyId::MaxLength:
		case PropertyId::Numeric:
		case PropertyId::AllowFloat:
		case PropertyId::Hint:
		case PropertyId::HintColor:
		case PropertyId::HintShadowColor:
		case PropertyId::HintFontSize:
		case PropertyId::HintFontFamily:
		case PropertyId::HintFontStyle:
		case PropertyId::HintStrokeWidth:
		case PropertyId::HintStrokeColor:
		case PropertyId::Color:
		case PropertyId::ShadowColor:
		case PropertyId::SelectionColor:
		case PropertyId::SelectionBackColor:
		case PropertyId::FontFamily:
		case PropertyId::FontSize:
		case PropertyId::FontStyle:
		case PropertyId::TextStrokeWidth:
		case PropertyId::TextStrokeColor:
		case PropertyId::Wordwrap:
		case PropertyId::TextSelection:
		case PropertyId::TextAlign:
			return mTextInput->applyProperty( attribute );
		default:
			attributeSet = UIWidget::applyProperty( attribute );
	}

	return attributeSet;
}

}} // namespace EE::UI
