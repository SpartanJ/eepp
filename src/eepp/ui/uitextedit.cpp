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
	mTextInput->setLayoutSizeRules( LayoutSizeRule::Fixed, LayoutSizeRule::WrapContent );
	mTextInput->setParent( this );
	mTextInput->setFlags( UI_TEXT_SELECTION_ENABLED | UI_VALIGN_TOP );
	mTextInput->unsetFlags( UI_VALIGN_CENTER | UI_AUTO_SIZE );
	mTextInput->clipDisable();
	mTextInput->getInputTextBuffer()->isNewLineEnabled( true );
	mTextInput->setVisible( true );
	mTextInput->setEnabled( true );
	mTextInput->setMinHeight( getSize().getHeight() );
	mTextInput->setSize( getSize() );

	auto cb = [&]( const Event* event ) {
		mNodeFlags |= NODE_FLAG_FREE_USE;
		onInputSizeChange( event );
		mNodeFlags &= ~NODE_FLAG_FREE_USE;
	};

	mTextInput->addEventListener( Event::OnSizeChange, cb );
	mTextInput->addEventListener( Event::OnTextChanged, cb );
	mTextInput->addEventListener( Event::OnPressEnter, cb );
	mTextInput->addEventListener( Event::OnCursorPosChange,
								  cb::Make1( this, &UITextEdit::onCursorPosChange ) );

	mVScrollBar = UIScrollBar::New();
	mVScrollBar->setOrientation( UIOrientation::Vertical );
	mVScrollBar->setParent( this );
	mVScrollBar->setSize( 0, getSize().getHeight() );
	mVScrollBar->setPosition( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 0 );
	mVScrollBar->setValue( 1 );

	mHScrollBar = UIScrollBar::New();
	mHScrollBar->setOrientation( UIOrientation::Horizontal );
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

	mTextInput->setPixelsPosition( mContainerPadding.Left, mContainerPadding.Top );
	mTextInput->setMinHeight( getSize().getHeight() );
	mTextInput->setSize( getSize() );

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

			if ( mTextInput->getTextHeight() >
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
		if ( mTextInput->getTextWidth() > mSize.getWidth() - mContainerPadding.Left -
											  mContainerPadding.Right -
											  mVScrollBar->getPixelsSize().getWidth() ) {
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
		Int32 totW = mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right -
					 mVScrollBar->getPixelsSize().getWidth();

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

	int Width = mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right;
	int Height = mSize.getHeight() - mContainerPadding.Top - mContainerPadding.Bottom;

	if ( NULL != Event ) {
		if ( Event->getType() == Event::OnPressEnter ) {
			mHScrollBar->setValue( 0 );
		}
	}

	if ( mHScrollBar->isVisible() )
		Height -= mHScrollBar->getPixelsSize().getHeight();

	String text( mTextInput->getInputTextBuffer()->getBuffer() );

	shrinkText( Width );

	if ( mTextInput->getTextHeight() > Height ) {
		Width -= mVScrollBar->getPixelsSize().getWidth();

		mTextInput->getInputTextBuffer()->setBuffer( text );

		shrinkText( Width );

		scrollbarsSet();
	}

	if ( ( mFlags & UI_WORD_WRAP ) && mTextInput->getTextHeight() < Height ) {
		mVScrollBar->setVisible( false );
		mVScrollBar->setEnabled( false );
	}

	if ( mTextInput->getPixelsSize().getWidth() < Width ||
		 mTextInput->getPixelsSize().getHeight() < Height ) {
		if ( mTextInput->getPixelsSize().getWidth() < Width &&
			 mTextInput->getPixelsSize().getHeight() < Height ) {
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

void UITextEdit::onCursorPosChange( const Event* ) {
	fixScrollToCursor();
}

void UITextEdit::fixScroll() {
	int Width = mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right;
	int Height = mSize.getHeight() - mContainerPadding.Top - mContainerPadding.Bottom;

	if ( mVScrollBar->isVisible() )
		Width -= mVScrollBar->getPixelsSize().getWidth();

	if ( mHScrollBar->isVisible() )
		Height -= mHScrollBar->getPixelsSize().getHeight();

	int diff;
	Float pos;

	diff = mTextInput->getPixelsSize().getHeight() - Height;
	if ( diff >= 0 ) {
		pos = diff * mVScrollBar->getValue();

		mTextInput->setPixelsPosition( mTextInput->getPixelsPosition().x,
									   mContainerPadding.Top - pos );
	}

	diff = mTextInput->getPixelsSize().getWidth() - Width;
	if ( diff >= 0 ) {
		pos = diff * mHScrollBar->getValue();

		mTextInput->setPixelsPosition( mContainerPadding.Left - pos,
									   mTextInput->getPixelsPosition().y );
	}

	invalidateDraw();
}

void UITextEdit::fixScrollToCursor() {
	if ( Font::getHorizontalAlign( mTextInput->getFlags() ) == UI_HALIGN_LEFT ) {
		int Width = mSize.getWidth() - mContainerPadding.Left - mContainerPadding.Right;
		int Height = mSize.getHeight() - mContainerPadding.Top - mContainerPadding.Bottom;

		if ( mVScrollBar->isVisible() )
			Width -= mVScrollBar->getPixelsSize().getWidth();

		if ( mHScrollBar->isVisible() )
			Height -= mHScrollBar->getPixelsSize().getHeight();

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
		Float visibleLines = eefloor( Height / lineHeight );
		Float scrollLines = (Float)mTextInput->getNumLines() - visibleLines;

		if ( mTextInput->getNumLines() > 0 ) {
			if ( mTextInput->getPixelsPosition().y + currentLineY < 0 ) {
				mVScrollBar->setValue( LineNum / scrollLines );
			} else if ( mTextInput->getPixelsPosition().y + currentLineY + lineHeight > Height ) {
				mVScrollBar->setValue( ( LineNum + 1 - visibleLines ) / scrollLines );
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
		default:
			attributeSet = UIWidget::applyProperty( attribute );
	}

	if ( !attributeSet && ( String::startsWith( attribute.getName(), "text" ) ||
							String::startsWith( attribute.getName(), "font" ) ) )
		mTextInput->applyProperty( attribute );

	return attributeSet;
}

}} // namespace EE::UI
