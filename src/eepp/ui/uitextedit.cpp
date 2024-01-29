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
	return UITextEdit::getType() == type ? true : UIWidget::isType( type );
}

void UITextEdit::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "textedit" );

	onThemeLoaded();
}

void UITextEdit::wrapText( const Float& maxWidth ) {
	Text text;
	text.setStyleConfig( mFontStyleConfig );
	text.setString( getText() );
	text.wrapText( maxWidth );
	mDoc->reset();
	mDoc->textInput( text.getString() );
}

String UITextEdit::getText() const {
	return mDoc->getText();
}

void UITextEdit::setText( const String& text ) {
	mDoc->reset();
	mDoc->textInput( text );
	if ( mFlags & UI_WORD_WRAP ) {
		wrapText( getViewportWidth( true ) );
	}
	if ( !hasFocus() ) {
		mCursorVisible = false;
	}
	invalidateLongestLineWidth();
}

void UITextEdit::onDocumentLineChanged( const Int64& lineIndex ) {
	UICodeEditor::onDocumentLineChanged( lineIndex );
	updateLineCache( lineIndex );
}

void UITextEdit::drawLineText( const Int64& index, Vector2f position, const Float&, const Float& ) {
	Color fontColor( mFontStyleConfig.FontColor );
	mFontStyleConfig.FontColor.blendAlpha( mAlpha );
	getLineText( index ).draw( position.x, position.y );
	mFontStyleConfig.FontColor = fontColor;
}

bool UITextEdit::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			setText( attribute.value() );
			break;
		default:
			return UICodeEditor::applyProperty( attribute );
	}

	return true;
}

void UITextEdit::drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor ) {
	if ( mCursorVisible && !mLocked && isTextSelectionEnabled() ) {
		Vector2f cursorPos( startScroll.x + getXOffsetCol( cursor ),
							startScroll.y + cursor.line() * lineHeight );
		Primitives primitives;
		primitives.setColor( Color( mFontStyleConfig.FontColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 1 ), lineHeight ) ) );
	}
}

}} // namespace EE::UI
