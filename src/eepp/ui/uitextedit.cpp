#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
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
	clipEnable();
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
	return mDoc->getText( { mDoc->startOfDoc(), mDoc->endOfDoc() } );
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

void UITextEdit::onFontChanged() {
	if ( mFont )
		invalidateLinesCache();
}

void UITextEdit::onFontStyleChanged() {
	onFontChanged();
}

void UITextEdit::onDocumentLineChanged( const Int64& lineIndex ) {
	UICodeEditor::onDocumentLineChanged( lineIndex );
	updateLineCache( lineIndex );
}

void UITextEdit::drawLineText( const Int64& index, Vector2f position, const Float&, const Float& ) {
	ensureLineUpdated( index );
	mLines[index].text.draw( position.x, position.y );
}

Int64 UITextEdit::getColFromXOffset( Int64 line, const Float& x ) const {
	if ( mFont && !mFont->isMonospace() ) {
		const_cast<UITextEdit*>( this )->ensureLineUpdated( line );
		return mLines.at( line ).text.findCharacterFromPos( Vector2i( x, 0 ) );
	}
	return UICodeEditor::getColFromXOffset( line, x );
}

Float UITextEdit::getColXOffset( TextPosition position ) {
	if ( mFont && !mFont->isMonospace() ) {
		return getXOffsetCol( position );
	}
	return UICodeEditor::getColXOffset( position );
}

Float UITextEdit::getXOffsetCol( const TextPosition& position ) {
	if ( mFont && !mFont->isMonospace() ) {
		ensureLineUpdated( position.line() );
		return mLines[position.line()]
			.text
			.findCharacterPos(
				( position.column() == (Int64)mDoc->line( position.line() ).getText().size() )
					? position.column() - 1
					: position.column() )
			.x;
	}
	return UICodeEditor::getXOffsetCol( position );
}

Float UITextEdit::getLineWidth( const Int64& lineIndex ) {
	ensureLineUpdated( lineIndex );
	return mLines[lineIndex].text.getTextWidth();
}

void UITextEdit::ensureLineUpdated( const Int64& lineIndex ) {
	if ( mLines.find( lineIndex ) == mLines.end() ||
		 mDoc->line( lineIndex ).getHash() != mLines[lineIndex].hash ) {
		updateLineCache( lineIndex );
	}
}

bool UITextEdit::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			setText( attribute.asString() );
			break;
		default:
			return UICodeEditor::applyProperty( attribute );
	}

	return true;
}

void UITextEdit::invalidateLinesCache() {
	mLines.clear();
	invalidateDraw();
}

void UITextEdit::updateLineCache( const Int64& lineIndex ) {
	if ( lineIndex >= 0 && lineIndex < (Int64)mDoc->linesCount() ) {
		TextDocumentLine& line = mDoc->line( lineIndex );
		auto& cacheLine = mLines[lineIndex];
		cacheLine.text.setStyleConfig( mFontStyleConfig );
		cacheLine.text.setString( line.getText() );
		cacheLine.hash = line.getHash();
	}
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

void UITextEdit::onDocumentChanged() {
	UICodeEditor::onDocumentChanged();
	invalidateLinesCache();
}

}} // namespace EE::UI
