#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/uicodeedit.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI {

UICodeEdit* UICodeEdit::New() {
	return eeNew( UICodeEdit, () );
}

UICodeEdit::UICodeEdit() :
	UIWidget( "codeedit" ),
	mFont( FontManager::instance()->getByName( "monospace" ) ),
	mDirtyEditor( false ),
	mCursorVisible( false ),
	mTabWidth( 4 ),
	mLastColOffset( 0 ) {
	clipEnable();
	if ( NULL == mFont ) {
		mFont = FontTrueType::New( "monospace", "assets/fonts/DejaVuSansMono.ttf" );
	}
	mDoc.registerClient( *this );
	subscribeScheduledUpdate();
}

UICodeEdit::~UICodeEdit() {
	mDoc.unregisterClient( *this );
}

Uint32 UICodeEdit::getType() const {
	return UI_TYPE_CODEEDIT;
}

bool UICodeEdit::isType( const Uint32& type ) const {
	return type == getType() || UIWidget::isType( type );
}

void UICodeEdit::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "textedit" );
}

void UICodeEdit::draw() {
	if ( mDirtyEditor ) {
		updateEditor();
	}

	if ( mFont == NULL )
		return;

	Float charSize = getCharacterSize();
	Float lineHeight = getLineHeight();
	Vector2f start( mScreenPos.x + mRealPadding.Left, mScreenPos.y + mRealPadding.Top );
	Vector2f startScroll( start - mScroll );
	std::pair<int, int> lineRange = getVisibleLineRange();
	for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
		Text line( mDoc.line( i ), mFont, charSize );
		line.draw( startScroll.x, startScroll.y + lineHeight * i );
	}

	Primitives primitives;
	TextPosition cursor( mDoc.getSelection().start() );

	primitives.setColor( Color( 255, 255, 255, 50 ) );
	primitives.drawRectangle(
		Rectf( Vector2f( startScroll.x, startScroll.y + cursor.line() * lineHeight ),
			   Sizef( mSize.getWidth(), lineHeight ) ) );

	if ( mDoc.hasSelection() ) {
	}

	if ( mCursorVisible ) {
		Vector2f cursorPos( startScroll.x + getXOffsetCol( cursor ),
							startScroll.y + cursor.line() * lineHeight );

		primitives.setColor( Color::White );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 2 ), lineHeight ) ) );
	}
}

void UICodeEdit::scheduledUpdate( const Time& ) {
	if ( hasFocus() ) {
		if ( mBlinkTimer.getElapsedTime().asSeconds() > 0.5f ) {
			mCursorVisible = !mCursorVisible;
			mBlinkTimer.restart();
			invalidateDraw();
		}
	}
}

void UICodeEdit::reset() {}

void UICodeEdit::loadFromFile( const std::string& path ) {
	mDoc.loadFromPath( path );
	invalidateEditor();
}

Font* UICodeEdit::getFont() const {
	return mFont;
}

void UICodeEdit::setFont( Font* font ) {
	if ( mFont != font ) {
		mFont = font;
		invalidateDraw();
		invalidateEditor();
	}
}

const Uint32& UICodeEdit::getTabWidth() const {
	return mTabWidth;
}

void UICodeEdit::setTabWidth( const Uint32& tabWidth ) {
	if ( mTabWidth != tabWidth ) {
		mTabWidth = tabWidth;
		mDoc.setTabWidth( mTabWidth );
	}
}

void UICodeEdit::invalidateEditor() {
	mDirtyEditor = true;
}

Uint32 UICodeEdit::onKeyDown( const KeyEvent& event ) {
	switch ( event.getKeyCode() ) {
		case KEY_UP: {
			mDoc.moveToPreviousLine( mLastColOffset );
			break;
		}
		case KEY_DOWN: {
			mDoc.moveToNextLine( mLastColOffset );
			break;
		}
		case KEY_LEFT: {
			mDoc.moveToPreviousChar();
			updateLastColumnOffset();
			break;
		}
		case KEY_RIGHT: {
			mDoc.moveToNextChar();
			updateLastColumnOffset();
			break;
		}
		case KEY_HOME: {
			mDoc.setSelection( mDoc.startOfLine( mDoc.getSelection().start() ) );
			updateLastColumnOffset();
			break;
		}
		case KEY_END: {
			mDoc.setSelection( mDoc.endOfLine( mDoc.getSelection().start() ) );
			updateLastColumnOffset();
			break;
		}
		case KEY_PAGEUP: {
			mDoc.moveToPreviousPage( getVisibleLinesCount() );
			updateLastColumnOffset();
			break;
		}
		case KEY_PAGEDOWN: {
			mDoc.moveToNextPage( getVisibleLinesCount() );
			updateLastColumnOffset();
			break;
		}
		default:
			break;
	}

	return 1;
}

TextPosition UICodeEdit::resolveScreenPosition( const Vector2f& position ) const {
	Vector2f localPos( position );
	worldToNode( localPos );
	localPos = PixelDensity::dpToPx( localPos );
	localPos += mScroll;
	localPos.x -= mRealPadding.Left;
	localPos.y -= mRealPadding.Top;
	Int64 line =
		eeclamp<Int64>( (Int64)eefloor( localPos.y / getLineHeight() ), 0, mDoc.lineCount() - 1 );
	return TextPosition( line, getColFromXOffset( line, localPos.x ) );
}

Uint32 UICodeEdit::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( flags & EE_BUTTON_LMASK ) {
		mDoc.setSelection( resolveScreenPosition( position.asFloat() ) );
	}
	return UIWidget::onMouseDown( position, flags );
}

void UICodeEdit::onSizeChange() {
	UIWidget::onSizeChange();
	invalidateEditor();
}

void UICodeEdit::onPaddingChange() {
	UIWidget::onPaddingChange();
	invalidateEditor();
}

void UICodeEdit::updateEditor() {
	scrollToMakeVisible( mDoc.getSelection().start() );
}

void UICodeEdit::onDocumentTextChanged() {
	invalidateDraw();
}

void UICodeEdit::onDocumentCursorChange( const Doc::TextPosition& ) {
	mCursorVisible = true;
	mBlinkTimer.restart();
	invalidateEditor();
	invalidateDraw();
}

void UICodeEdit::onDocumentSelectionChange( const Doc::TextRange& ) {
	mCursorVisible = true;
	mBlinkTimer.restart();
	invalidateDraw();
}

std::pair<int, int> UICodeEdit::getVisibleLineRange() {
	Float lineHeight = getLineHeight();
	Float minLine = eemax( 0.f, eefloor( mScroll.y / lineHeight ) );
	Float maxLine = eemin( mDoc.lineCount() - 1.f,
						   eefloor( ( mSize.getHeight() + mScroll.y ) / lineHeight ) + 1 );
	return std::make_pair<int, int>( (int)minLine, (int)maxLine );
}

int UICodeEdit::getVisibleLinesCount() {
	auto lines = getVisibleLineRange();
	return lines.second - lines.first;
}

void UICodeEdit::scrollToMakeVisible( const TextPosition& position ) {
	Float lineHeight = getLineHeight();
	Float min = lineHeight * ( eemax<Float>( 0, position.line() - 1 ) );
	Float max = lineHeight * ( position.line() + 2 ) - mSize.getHeight();
	mScroll.y = eemin( mScroll.y, min );
	mScroll.y = eemax( mScroll.y, max );
	invalidateDraw();
}

Float UICodeEdit::getXOffsetCol( const TextPosition& position ) const {
	const String& line = mDoc.line( position.line() );
	Float glyphWidth = getGlyphWidth();
	Float x = 0;
	for ( auto i = 0; i < position.column(); i++ ) {
		if ( line[i] == '\t' ) {
			x += glyphWidth * mTabWidth;
		} else if ( line[i] != '\n' && line[i] != '\r' ) {
			x += glyphWidth;
		}
	}
	return x;
}

Int64 UICodeEdit::getColFromXOffset( Int64 _line, const Float& offset ) const {
	if ( offset <= 0 )
		return 0;
	TextPosition pos = mDoc.sanitizePosition( TextPosition( _line, 0 ) );
	const String& line = mDoc.line( pos.line() );
	Float glyphWidth = getGlyphWidth();
	Float x = 0;
	for ( size_t i = 0; i < line.size(); i++ ) {
		if ( line[i] == '\t' ) {
			x += glyphWidth * mTabWidth;
		} else if ( line[i] != '\n' && line[i] != '\r' ) {
			x += glyphWidth;
		}
		if ( x >= offset ) {
			return i;
		}
	}
	return line.size() - 1;
}

Float UICodeEdit::getLineHeight() const {
	return mFont->getLineSpacing( getCharacterSize() );
}

Float UICodeEdit::getCharacterSize() const {
	return PixelDensity::pxToDp( mFontStyleConfig.getFontCharacterSize() );
}

Float UICodeEdit::getGlyphWidth() const {
	return mFont->getGlyph( KEY_SPACE, getCharacterSize(), false ).advance;
}

void UICodeEdit::updateLastColumnOffset() {
	mLastColOffset = mDoc.getAbsolutePosition( mDoc.getSelection().start() ).column();
}

}} // namespace EE::UI
