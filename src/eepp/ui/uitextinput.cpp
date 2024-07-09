#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITextInput* UITextInput::New() {
	return eeNew( UITextInput, () );
}

UITextInput* UITextInput::NewWithTag( const std::string& tag ) {
	return eeNew( UITextInput, ( tag ) );
}

UITextInput::UITextInput( const std::string& tag ) :
	UITextView( tag ),
	mCursorPos( 0 ),
	mAllowEditing( true ),
	mShowingWait( true ),
	mOnlyNumbers( false ),
	mAllowFloat( false ),
	mMouseDown( false ),
	mKeyBindings( getUISceneNode()->getWindow()->getInput() ) {
	mHintCache = Text::New();

	UITheme* theme = getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL != theme && NULL != theme->getDefaultFont() ) {
		setHintFont( theme->getDefaultFont() );
	}

	if ( NULL == mHintCache->getFont() ) {
		if ( NULL != getUISceneNode()->getUIThemeManager()->getDefaultFont() ) {
			setHintFont( getUISceneNode()->getUIThemeManager()->getDefaultFont() );
		} else {
			Log::error( "UITextInput::UITextInput : Created a without a defined font." );
		}
	}

	if ( NULL != theme ) {
		setHintFontSize( theme->getDefaultFontSize() );
	} else {
		setHintFontSize( getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );
	}

	subscribeScheduledUpdate();

	setFlags( UI_AUTO_PADDING | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED );
	setClipType( ClipType::ContentBox );

	mDoc.registerClient( this );
	registerCommands();
	registerKeybindings();

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
	updateWaitingCursor( time );
	if ( mMouseDown ) {
		if ( !( getUISceneNode()->getWindow()->getInput()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
			mMouseDown = false;
			mSelecting = false;
			getUISceneNode()->getWindow()->getInput()->captureMouse( false );
		} else {
			onMouseDown( getUISceneNode()->getEventDispatcher()->getMousePos(),
						 getUISceneNode()->getEventDispatcher()->getPressTrigger() );
		}
	}
}

void UITextInput::onCursorPosChange() {
	sendCommonEvent( Event::OnCursorPosChange );
	updateIMELocation();
	invalidateDraw();
}

void UITextInput::drawWaitingCursor() {
	if ( mVisible && hasFocus() && mShowingWait && mAllowEditing ) {
		Vector2f cursor( eefloor( mScreenPos.x + mRealAlignOffset.x + mCurPos.x + mPaddingPx.Left ),
						 mScreenPos.y + mRealAlignOffset.y + mCurPos.y + mPaddingPx.Top );

		Primitives primitives;
		primitives.setColor( Color( mFontStyleConfig.FontColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle( Rectf(
			cursor, Sizef( PixelDensity::dpToPx( 1 ), mTextCache->getFont()->getFontHeight(
														  mTextCache->getCharacterSize() ) ) ) );
	}
}

void UITextInput::updateWaitingCursor( const Time& time ) {
	if ( mVisible && hasFocus() ) {
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
			mTextCache->setAlign( getFlags() );
			mTextCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x + (int)mPaddingPx.Left,
							  (Float)mScreenPosi.y + (int)mRealAlignOffset.y + (int)mPaddingPx.Top,
							  Vector2f::One, 0.f, getBlendMode() );
		} else if ( !mHintCache->getString().empty() &&
					( mHintDisplay == HintDisplay::Always || hasFocus() ) ) {
			mHintCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x + (int)mPaddingPx.Left,
							  (Float)mScreenPosi.y + (int)mRealAlignOffset.y + (int)mPaddingPx.Top,
							  Vector2f::One, 0.f, getBlendMode() );
		}
	}

	if ( hasFocus() && getUISceneNode()->getWindow()->getIME().isEditing() ) {
		updateIMELocation();
		Vector2f cursor( eefloor( mScreenPos.x + mRealAlignOffset.x + mCurPos.x + mPaddingPx.Left ),
						 mScreenPos.y + mRealAlignOffset.y + mCurPos.y + mPaddingPx.Top );
		FontStyleConfig config( mFontStyleConfig );
		config.FontColor = mFontStyleConfig.getFontSelectedColor();
		getUISceneNode()->getWindow()->getIME().draw(
			cursor, getTextHeight(), mFontStyleConfig,
			Color( mFontStyleConfig.FontColor ).blendAlpha( mAlpha ) );
	} else {
		drawWaitingCursor();
	}
}

Uint32 UITextInput::onFocus( NodeFocusReason reason ) {
	UIWidget::onFocus( reason );

	if ( mAllowEditing ) {
		resetWaitCursor();

		getSceneNode()->getWindow()->startTextInput();

		mLastExecuteEventId = getUISceneNode()->getWindow()->getInput()->getEventsSentId();

		updateIMELocation();
	}

	if ( mSelectAllDocOnTabNavigate && reason == NodeFocusReason::Tab )
		mDoc.selectAll();

	return 1;
}

Uint32 UITextInput::onFocusLoss() {
	getSceneNode()->getWindow()->stopTextInput();
	return UITextView::onFocusLoss();
}

Uint32 UITextInput::onPressEnter() {
	sendCommonEvent( Event::OnPressEnter );
	return 0;
}

void UITextInput::resetWaitCursor() {
	mShowingWait = true;
	mWaitCursorTime = 0.f;
}

void UITextInput::alignFix() {
	Vector2f rOffset( mRealAlignOffset );
	UITextView::alignFix();

	if ( Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_LEFT ) {
		Float tW = getVisibleTextCache()->findCharacterPos( selCurInit() ).x;
		mCurPos.x = tW;
		mCurPos.y = 0;

		if ( mSize.getWidth() > 0 ) {
			mRealAlignOffset.x = rOffset.x;
			Float tX = mRealAlignOffset.x + tW;

			if ( tX < 0.f ) {
				mRealAlignOffset.x = -( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
			} else if ( tX > mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right ) {
				mRealAlignOffset.x = mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right -
									 ( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
			}
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

	setMinHeight( eemax( mMinSize.y, getSkinSize().getHeight() ) );

	autoPadding();
	onAutoSize();

	UIWidget::onThemeLoaded();
}

void UITextInput::onAutoSize() {
	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		int minHeight = eemax<int>( mTextCache->getLineSpacing(),
									PixelDensity::dpToPxI( getSkinSize().getHeight() ) );
		setInternalPixelsHeight( minHeight + mPaddingPx.Top + mPaddingPx.Bottom );
	} else if ( ( mFlags & UI_AUTO_SIZE ) && 0 == getSize().getHeight() ) {
		setInternalPixelsHeight( PixelDensity::dpToPxI( getSkinSize().getHeight() ) +
								 mPaddingPx.Top + mPaddingPx.Bottom );
	}
}

void UITextInput::onSizeChange() {
	mRealAlignOffset = Vector2f::Zero;
	UITextView::onSizeChange();
}

void UITextInput::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		setPadding( Rectf() );
	}
}

UITextInput* UITextInput::setAllowEditing( const bool& allow ) {
	if ( allow != mAllowEditing ) {
		mAllowEditing = allow;
		invalidateDraw();
	}
	return this;
}

const bool& UITextInput::isEditingAllowed() const {
	return mAllowEditing;
}

UITextView* UITextInput::setText( const String& text ) {
	UITextView::setText( text );

	mDoc.reset();
	mDoc.textInput( text );

	return this;
}

const String& UITextInput::getText() const {
	return UITextView::getText();
}

void UITextInput::wrapText( const Uint32& ) {}

void UITextInput::updateText() {}

Uint32 UITextInput::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	int endPos = selCurEnd();

	if ( getUISceneNode()->getWindow()->getInput()->isShiftPressed() )
		mSelecting = true;

	UITextView::onMouseDown( position, flags );

	if ( NULL != getEventDispatcher() && isTextSelectionEnabled() && ( flags & EE_BUTTON_LMASK ) &&
		 getEventDispatcher()->getMouseDownNode() == this && !mMouseDown ) {
		getUISceneNode()->getWindow()->getInput()->captureMouse( true );
		mMouseDown = true;
	}

	if ( endPos != selCurEnd() && -1 != selCurEnd() ) {
		resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( flags & EE_BUTTON_LMASK ) {
		if ( mMouseDown ) {
			mMouseDown = false;
			getUISceneNode()->getWindow()->getInput()->captureMouse( false );
		}
	} else if ( ( flags & EE_BUTTON_RMASK ) && mEnabledCreateContextMenu ) {
		onCreateContextMenu( position, flags );
	}
	return UITextView::onMouseUp( position, flags );
}

Uint32 UITextInput::onMouseClick( const Vector2i& position, const Uint32& flags ) {
	UITextView::onMouseClick( position, flags );
	if ( ( flags & EE_BUTTON_LMASK ) &&
		 mLastDoubleClick.getElapsedTime() < Milliseconds( 300.f ) ) {
		mDoc.selectLine();
	}
	return 1;
}

Uint32 UITextInput::onMouseDoubleClick( const Vector2i& Pos, const Uint32& Flags ) {
	UITextView::onMouseDoubleClick( Pos, Flags );

	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		mLastDoubleClick.restart();
		if ( selCurEnd() != -1 )
			resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	if ( NULL != mSceneNode )
		mSceneNode->setCursor( Cursor::IBeam );

	return UITextView::onMouseOver( position, flags );
}

Uint32 UITextInput::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	UITextView::onMouseLeave( Pos, Flags );

	if ( NULL != mSceneNode )
		mSceneNode->setCursor( Cursor::Arrow );

	return 1;
}

void UITextInput::selCurInit( const Int32& init ) {
	if ( mDoc.getSelection().start().column() != init && -1 != init ) {
		mDoc.setSelection( { { 0, init }, mDoc.getSelection().end() } );
	}
}

void UITextInput::selCurEnd( const Int32& end ) {
	if ( mDoc.getSelection().end().column() != end && -1 != end ) {
		mDoc.setSelection( { mDoc.getSelection().start(), { 0, end } } );
	} else if ( -1 == end ) {
		mDoc.setSelection( mDoc.getSelection().end() );
	}
}

Int32 UITextInput::selCurInit() {
	return mDoc.getSelection().start().column();
}

Int32 UITextInput::selCurEnd() {
	return mDoc.getSelection().end().column();
}

void UITextInput::onDocumentTextChanged( const DocumentContentChange& ) {
	Vector2f offSet = mRealAlignOffset;

	const String& text = mDoc.line( 0 ).getText();

	UITextView::setText( !text.empty() ? text.substr( 0, text.size() - 1 ) : "" );

	updateText();

	mRealAlignOffset = offSet;

	resetWaitCursor();

	alignFix();

	invalidateDraw();

	sendCommonEvent( Event::OnBufferChange );
}

void UITextInput::onDocumentCursorChange( const TextPosition& ) {
	alignFix();
	mWaitCursorTime = 0.f;
	mShowingWait = true;
	onCursorPosChange();
}

void UITextInput::onDocumentSelectionChange( const TextRange& ) {
	onSelectionChange();
}

void UITextInput::onDocumentLineCountChange( const size_t&, const size_t& ) {}

void UITextInput::onDocumentLineChanged( const Int64& ) {}

void UITextInput::onDocumentUndoRedo( const TextDocument::UndoRedo& ) {
	onSelectionChange();
}

void UITextInput::onDocumentSaved( TextDocument* ) {}

void UITextInput::onDocumentMoved( TextDocument* ) {}

UITextInput* UITextInput::setMaxLength( const Uint32& maxLength ) {
	mMaxLength = maxLength;
	return this;
}

const Uint32& UITextInput::getMaxLength() const {
	return mMaxLength;
}

std::string UITextInput::getPropertyString( const PropertyDefinition* propertyDef,
											const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
			return getText().toUtf8();
		case PropertyId::AllowEditing:
			return isEditingAllowed() ? "true" : "false";
		case PropertyId::MaxLength:
			return String::toString( getMaxLength() );
		case PropertyId::Numeric:
			return onlyNumbersAllowed() ? "true" : "false";
		case PropertyId::AllowFloat:
			return floatingPointAllowed() ? "true" : "false";
		case PropertyId::Hint:
			return getHint().toUtf8();
		case PropertyId::HintColor:
			return getHintColor().toHexString();
		case PropertyId::HintShadowColor:
			return getHintShadowColor().toHexString();
		case PropertyId::HintShadowOffset:
			return String::fromFloat( getHintShadowOffset().x ) + " " +
				   String::fromFloat( getHintShadowOffset().y );
		case PropertyId::HintFontSize:
			return String::format( "%ddp", getHintFontSize() );
		case PropertyId::HintFontFamily:
			return NULL != getHintFont() ? getFont()->getName() : "";
		case PropertyId::HintFontStyle:
			return Text::styleFlagToString( getHintFontStyle() );
		case PropertyId::HintStrokeWidth:
			return String::fromFloat( PixelDensity::dpToPx( getHintOutlineThickness() ), "px" );
		case PropertyId::HintStrokeColor:
			return getHintOutlineColor().toHexString();
		case PropertyId::HintDisplay:
			return mHintDisplay == HintDisplay::Always ? "always" : "focus";
		default:
			return UITextView::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITextInput::getPropertiesImplemented() const {
	auto props = UITextView::getPropertiesImplemented();
	auto local = { PropertyId::Text,
				   PropertyId::AllowEditing,
				   PropertyId::MaxLength,
				   PropertyId::Numeric,
				   PropertyId::AllowFloat,
				   PropertyId::Hint,
				   PropertyId::HintColor,
				   PropertyId::HintShadowColor,
				   PropertyId::HintShadowOffset,
				   PropertyId::HintFontSize,
				   PropertyId::HintFontFamily,
				   PropertyId::HintFontStyle,
				   PropertyId::HintStrokeWidth,
				   PropertyId::HintStrokeColor,
				   PropertyId::HintDisplay };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UITextInput::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			setText( getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::AllowEditing:
			setAllowEditing( attribute.asBool() );
			break;
		case PropertyId::MaxLength:
			setMaxLength( attribute.asUint() );
			break;
		case PropertyId::Numeric:
			setAllowOnlyNumbers( attribute.asBool(), floatingPointAllowed() );
			break;
		case PropertyId::AllowFloat:
			setAllowOnlyNumbers( onlyNumbersAllowed(), attribute.asBool() );
			break;
		case PropertyId::Hint:
			setHint( getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::HintColor:
			setHintColor( attribute.asColor() );
			break;
		case PropertyId::HintShadowColor:
			setHintShadowColor( attribute.asColor() );
			break;
		case PropertyId::HintShadowOffset:
			setHintShadowOffset( attribute.asVector2f() );
			break;
		case PropertyId::HintFontSize:
			setHintFontSize( lengthFromValue( attribute ) );
			break;
		case PropertyId::HintFontFamily:
			setHintFont( FontManager::instance()->getByName( attribute.value() ) );
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
		case PropertyId::HintDisplay:
			setHintDisplay( String::toLower( attribute.asString() ) == "focus"
								? HintDisplay::Focus
								: HintDisplay::Always );
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

const Vector2f& UITextInput::getHintShadowOffset() const {
	return mHintStyleConfig.getFontShadowOffset();
}

UITextInput* UITextInput::setHintShadowOffset( const Vector2f& shadowOffset ) {
	if ( shadowOffset != mHintStyleConfig.getFontShadowOffset() ) {
		mHintCache->setShadowOffset( shadowOffset );
		mHintStyleConfig.ShadowOffset = shadowOffset;
		invalidateDraw();
	}

	return this;
}

Font* UITextInput::getHintFont() const {
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

Uint32 UITextInput::getHintFontSize() const {
	return mHintCache->getCharacterSize();
}

UITextView* UITextInput::setHintFontSize( const Uint32& characterSize ) {
	if ( mHintCache->getCharacterSize() != characterSize ) {
		mHintStyleConfig.CharacterSize = characterSize;
		mHintCache->setFontSize( characterSize );
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

void UITextInput::copy() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText().toUtf8() );
}

void UITextInput::cut() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText().toUtf8() );
	mDoc.deleteSelection();
}

void UITextInput::paste() {
	String pasted( getUISceneNode()->getWindow()->getClipboard()->getText() );
	if ( mEscapePastedText ) {
		pasted.escape();
	} else {
		String::replaceAll( pasted, "\n", "" );
	}
	mDoc.textInput( pasted );
	onTextChanged();
	sendCommonEvent( Event::OnTextPasted );
}

void UITextInput::registerCommands() {
	mDoc.setCommand( "copy", [this] { copy(); } );
	mDoc.setCommand( "cut", [this] { cut(); } );
	mDoc.setCommand( "paste", [this] { paste(); } );
	mDoc.setCommand( "press-enter", [this] { onPressEnter(); } );
}

void UITextInput::registerKeybindings() {
	mKeyBindings.addKeybinds( {
		{ { KEY_BACKSPACE, KeyMod::getDefaultModifier() }, "delete-to-previous-word" },
		{ { KEY_BACKSPACE, KEYMOD_SHIFT }, "delete-to-previous-char" },
		{ { KEY_BACKSPACE, 0 }, "delete-to-previous-char" },
		{ { KEY_DELETE, KeyMod::getDefaultModifier() }, "delete-to-next-word" },
		{ { KEY_DELETE, 0 }, "delete-to-next-char" },
		{ { KEY_KP_ENTER, 0 }, "press-enter" },
		{ { KEY_RETURN, 0 }, "press-enter" },
		{ { KEY_LEFT, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-to-previous-word" },
		{ { KEY_LEFT, KeyMod::getDefaultModifier() }, "move-to-previous-word" },
		{ { KEY_LEFT, KEYMOD_SHIFT }, "select-to-previous-char" },
		{ { KEY_LEFT, 0 }, "move-to-previous-char" },
		{ { KEY_RIGHT, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-to-next-word" },
		{ { KEY_RIGHT, KeyMod::getDefaultModifier() }, "move-to-next-word" },
		{ { KEY_RIGHT, KEYMOD_SHIFT }, "select-to-next-char" },
		{ { KEY_RIGHT, 0 }, "move-to-next-char" },
		{ { KEY_Z, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "redo" },
		{ { KEY_HOME, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-to-start-of-doc" },
		{ { KEY_HOME, KEYMOD_SHIFT }, "select-to-start-of-content" },
		{ { KEY_HOME, KeyMod::getDefaultModifier() }, "move-to-start-of-doc" },
		{ { KEY_HOME, 0 }, "move-to-start-of-content" },
		{ { KEY_END, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "select-to-end-of-doc" },
		{ { KEY_END, KEYMOD_SHIFT }, "select-to-end-of-line" },
		{ { KEY_END, KeyMod::getDefaultModifier() }, "move-to-end-of-doc" },
		{ { KEY_END, 0 }, "move-to-end-of-line" },
		{ { KEY_Y, KeyMod::getDefaultModifier() }, "redo" },
		{ { KEY_Z, KeyMod::getDefaultModifier() }, "undo" },
		{ { KEY_C, KeyMod::getDefaultModifier() }, "copy" },
		{ { KEY_X, KeyMod::getDefaultModifier() }, "cut" },
		{ { KEY_V, KeyMod::getDefaultModifier() }, "paste" },
		{ { KEY_A, KeyMod::getDefaultModifier() }, "select-all" },
	} );
}

Uint32 UITextInput::onKeyDown( const KeyEvent& event ) {
	if ( getUISceneNode()->getWindow()->getIME().isEditing() )
		return 0;

	std::string cmd = mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
	if ( !cmd.empty() ) {
		// Allow copy selection on locked mode
		if ( mAllowEditing ) {
			mDoc.execute( cmd );
			mLastExecuteEventId = getUISceneNode()->getWindow()->getInput()->getEventsSentId();
			return 1;
		}
	}
	return UITextView::onKeyDown( event );
}

Uint32 UITextInput::onTextInput( const TextInputEvent& event ) {
	if ( !mAllowEditing )
		return 0;
	Input* input = getUISceneNode()->getWindow()->getInput();

	if ( ( input->isLeftAltPressed() && !event.getText().empty() && event.getText()[0] == '\t' ) ||
		 ( input->isLeftControlPressed() && !input->isLeftAltPressed() &&
		   !input->isAltGrPressed() ) ||
		 input->isMetaPressed() || ( input->isLeftAltPressed() && !input->isLeftControlPressed() ) )
		return 0;

	if ( mLastExecuteEventId == getUISceneNode()->getWindow()->getInput()->getEventsSentId() )
		return 0;

	const String& text = event.getText();

	for ( size_t i = 0; i < text.size(); i++ ) {
		if ( text[i] == '\n' )
			return 0;
		if ( mOnlyNumbers && ( ( mAllowFloat && text[i] == '.' && mDoc.find( "." ).isValid() ) ||
							   !String::isNumber( text[i], mAllowFloat ) ) ) {
			if ( !( i == 0 && mDoc.getSelection( true ).start().column() == 0 &&
					( text[i] == '-' || text[i] == '+' ) ) ) {
				return 0;
			}
		}
	}

	mDoc.textInput( text );
	return 1;
}

void UITextInput::updateIMELocation() {
	if ( mDoc.getActiveClient() != this || !Engine::isRunninMainThread() )
		return;
	Vector2f cursor( eefloor( mScreenPos.x + mRealAlignOffset.x + mCurPos.x + mPaddingPx.Left ),
					 mScreenPos.y + mRealAlignOffset.y + mCurPos.y + mPaddingPx.Top );
	Float h = mTextCache->getFont()->getFontHeight( mTextCache->getCharacterSize() );
	getUISceneNode()->getWindow()->getIME().setLocation( Rectf( cursor, { 0, h } ).asInt() );
}

Uint32 UITextInput::onTextEditing( const TextEditingEvent& event ) {
	UITextView::onTextEditing( event );
	mDoc.imeTextEditing( event.getText() );
	updateIMELocation();
	invalidateDraw();
	return 1;
}

void UITextInput::setAllowOnlyNumbers( const bool& onlyNumbers, const bool& allowFloat ) {
	mOnlyNumbers = onlyNumbers;
	mAllowFloat = allowFloat;
}

bool UITextInput::onlyNumbersAllowed() const {
	return mOnlyNumbers;
}

bool UITextInput::floatingPointAllowed() const {
	return mAllowFloat;
}

TextDocument& UITextInput::getDocument() {
	return mDoc;
}

KeyBindings& UITextInput::getKeyBindings() {
	return mKeyBindings;
}

size_t UITextInput::getMenuIconSize() const {
	return mMenuIconSize;
}

void UITextInput::setMenuIconSize( size_t menuIconSize ) {
	mMenuIconSize = menuIconSize;
}

bool UITextInput::getEscapePastedText() const {
	return mEscapePastedText;
}

void UITextInput::setEscapePastedText( bool escapePastedText ) {
	mEscapePastedText = escapePastedText;
}

Drawable* UITextInput::findIcon( const std::string& name ) {
	UIIcon* icon = getUISceneNode()->findIcon( name );
	if ( icon )
		return icon->getSize( mMenuIconSize );
	return nullptr;
}

UIMenuItem* UITextInput::menuAdd( UIPopUpMenu* menu, const String& translateString,
								  const std::string& icon, const std::string& cmd ) {
	UIMenuItem* menuItem =
		menu->add( translateString, findIcon( icon ), mKeyBindings.getCommandKeybindString( cmd ) );
	menuItem->setId( cmd );
	return menuItem;
}

void UITextInput::createDefaultContextMenuOptions( UIPopUpMenu* menu ) {
	if ( !mCreateDefaultContextMenuOptions )
		return;

	menuAdd( menu, i18n( "uicodeeditor_undo", "Undo" ), "undo", "undo" )
		->setEnabled( mDoc.hasUndo() );
	menuAdd( menu, i18n( "uicodeeditor_redo", "Redo" ), "redo", "redo" )
		->setEnabled( mDoc.hasRedo() );
	menu->addSeparator();

	menuAdd( menu, i18n( "uicodeeditor_cut", "Cut" ), "cut", "cut" )
		->setEnabled( mDoc.hasSelection() );
	menuAdd( menu, i18n( "uicodeeditor_copy", "Copy" ), "copy", "copy" )
		->setEnabled( mDoc.hasSelection() );
	menuAdd( menu, i18n( "uicodeeditor_paste", "Paste" ), "paste", "paste" );
	menuAdd( menu, i18n( "uicodeeditor_delete", "Delete" ), "delete-text", "delete-to-next-char" );
	menu->addSeparator();
	menuAdd( menu, i18n( "uicodeeditor_select_all", "Select All" ), "select-all", "select-all" );
}

bool UITextInput::onCreateContextMenu( const Vector2i& position, const Uint32& flags ) {
	if ( mCurrentMenu )
		return false;

	UIPopUpMenu* menu = UIPopUpMenu::New();

	createDefaultContextMenuOptions( menu );

	ContextMenuEvent event( this, menu, Event::OnCreateContextMenu, position, flags );
	sendEvent( &event );

	if ( menu->getCount() == 0 ) {
		menu->close();
		return false;
	}

	menu->setCloseOnHide( true );
	menu->addEventListener( Event::OnItemClicked, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string txt( item->getId() );
		mDoc.execute( txt );
	} );

	Vector2f pos( position.asFloat() );
	Int32 init = selCurInit();
	Int32 end = selCurEnd();
	menu->nodeToWorldTranslation( pos );
	UIMenu::findBestMenuPos( pos, menu );
	menu->setPixelsPosition( pos );
	menu->show();
	menu->addEventListener( Event::OnClose, [this]( const Event* ) { mCurrentMenu = nullptr; } );
	mCurrentMenu = menu;
	selCurInit( init );
	selCurEnd( end );
	return true;
}

void UITextInput::setHintDisplay( HintDisplay display ) {
	mHintDisplay = display;
}

HintDisplay UITextInput::getHintDisplay() const {
	return mHintDisplay;
}

bool UITextInput::getSelectAllDocOnTabNavigate() const {
	return mSelectAllDocOnTabNavigate;
}

void UITextInput::setSelectAllDocOnTabNavigate( bool selectAllDocOnTabNavigate ) {
	mSelectAllDocOnTabNavigate = selectAllDocOnTabNavigate;
}

}} // namespace EE::UI
