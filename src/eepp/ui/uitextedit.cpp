#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UITextEdit* UITextEdit::New() {
	return eeNew( UITextEdit, () );
}

UITextEdit::UITextEdit() : UICodeEditor( "textedit", true, true ) {
	setFlags( UI_AUTO_PADDING );
	setClipType( ClipType::ContentBox );
	mFont = NULL;
	mHorizontalScrollBarEnabled = true;

	UITheme* theme = getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL != theme && NULL != theme->getDefaultFont() ) {
		setFont( theme->getDefaultFont() );
	}

	if ( NULL != theme ) {
		setFontSize( theme->getDefaultFontSize() );
	} else {
		setFontSize( getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );
	}

	if ( NULL == getFont() ) {
		if ( NULL != getUISceneNode()->getUIThemeManager()->getDefaultFont() ) {
			setFont( getUISceneNode()->getUIThemeManager()->getDefaultFont() );
		} else {
			Log::error( "UITextEdit::UITextEdit : Created a without a defined font." );
		}
	}

	disableEditorFeatures();
	applyDefaultTheme();
}

UITextEdit::~UITextEdit() {}

Uint32 UITextEdit::getType() const {
	return UI_TYPE_TEXTEDIT;
}

bool UITextEdit::isType( const Uint32& type ) const {
	return UITextEdit::getType() == type ? true : UICodeEditor::isType( type );
}

void UITextEdit::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "textedit" );

	onThemeLoaded();
}

String UITextEdit::getText() const {
	return mDoc->getText();
}

void UITextEdit::setText( const String& text ) {
	mDoc->reset();
	mDoc->textInput( text );
	if ( !hasFocus() )
		mCursorVisible = false;
	invalidateLongestLineWidth();
}

void UITextEdit::setWordWrap( bool enabled ) {
	if ( enabled )
		setFlags( UI_WORD_WRAP );
	else
		unsetFlags( UI_WORD_WRAP );

	setLineWrapKeepIndentation( false );
	setLineWrapType( LineWrapType::Viewport );
	setLineWrapMode( enabled ? LineWrapMode::Word : LineWrapMode::NoWrap );
}

bool UITextEdit::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			setText( attribute.value() );
			break;
		case PropertyId::Wordwrap:
			setWordWrap( attribute.asBool() );
		default:
			return UICodeEditor::applyProperty( attribute );
	}

	return true;
}

void UITextEdit::drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor ) {
	if ( mCursorVisible && !mLocked && isTextSelectionEnabled() ) {
		auto offset = getTextPositionOffset( cursor );
		Vector2f cursorPos( startScroll.x + offset.x, startScroll.y + offset.y );
		Primitives primitives;
		primitives.setColor( Color( mFontStyleConfig.FontColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 1 ), lineHeight ) ) );
	}
}

}} // namespace EE::UI
