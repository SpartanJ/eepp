#include <cstdio>
#include <eepp/core/debug.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <sstream>
#include <string>

namespace EE { namespace UI { namespace Doc {

// Text document is loosely based on the SerenityOS (https://github.com/SerenityOS/serenity)
// TextDocument and the lite editor (https://github.com/rxi/lite) implementations.

const char NON_WORD_CHARS[] = " \t\n/\\()\"':,.;<>~!@#$%^&*|+=[]{}`?-";

bool TextDocument::isNonWord( String::StringBaseType ch ) {
	for ( size_t i = 0; i < eeARRAY_SIZE( NON_WORD_CHARS ); i++ ) {
		if ( static_cast<String::StringBaseType>( NON_WORD_CHARS[i] ) == ch ) {
			return true;
		}
	}
	return false;
}

TextDocument::TextDocument() :
	mUndoStack( this ), mDefaultFileName( "untitled" ), mCleanChangeId( 0 ) {
	initializeCommands();
	reset();
}

void TextDocument::reset() {
	mFilePath = mDefaultFileName;
	mSelection.set( {0, 0}, {0, 0} );
	mLines.clear();
	mLines.emplace_back( String( "\n" ) );
	mSyntaxDefinition = SyntaxDefinitionManager::instance()->getPlainStyle();
	mUndoStack.clear();
	cleanChangeId();
	notifyTextChanged();
	notifyCursorChanged();
	notifySelectionChanged();
}

void TextDocument::loadFromPath( const std::string& path ) {
	if ( !FileSystem::fileExists( path ) ) {
		eePRINTL( "File \"%s\" does not exists. Creating a new file.", path.c_str() );
	}
	Clock clock;
	reset();
	mLines.clear();
	mFilePath = path;
	mSyntaxDefinition = SyntaxDefinitionManager::instance()->getStyleByExtension( path );
	// TODO: Reimplement load without using getline, since the standard ignores the last \n and
	// is inconsistent.
	std::string line;
	std::ifstream file( path );
	while ( std::getline( file, line ) ) {
		if ( mLines.empty() && line.size() >= 3 ) {
			// Check UTF-8 BOM header
			if ( (char)0xef == line[0] && (char)0xbb == line[1] && (char)0xbf == line[2] ) {
				line = line.substr( 3 );
				mIsBOM = true;
			}
		}
		// Check CLRF
		if ( !line.empty() && line[line.size() - 1] == '\r' ) {
			line = line.substr( 0, line.size() - 1 );
			mIsCLRF = true;
		}
		mLines.emplace_back( TextDocumentLine( line + "\n" ) );
	}

	if ( mLines.empty() ) {
		mLines.emplace_back( TextDocumentLine( "\n" ) );
	} else if ( mLines[mLines.size() - 1][mLines[mLines.size() - 1].size() - 1] != '\n' ) {
		mLines[mLines.size() - 1].append( '\n' );
	}
	eePRINTL( "Document \"%s\" loaded in %.2fms.", path.c_str(),
			  clock.getElapsedTime().asMilliseconds() );
}

bool TextDocument::save( const std::string& path, const bool& utf8bom ) {
	if ( path.empty() || mDefaultFileName == path )
		return false;
	IOStreamFile file( path, "wb" );
	if ( save( file, utf8bom ) ) {
		mFilePath = path;
		return true;
	}
	return false;
}

bool TextDocument::save( IOStreamFile& stream, const bool& utf8bom ) {
	if ( !stream.isOpen() || mLines.empty() )
		return false;
	char nl = '\n';
	if ( utf8bom ) {
		unsigned char bom[] = {0xEF, 0xBB, 0xBF};
		stream.write( (char*)bom, sizeof( bom ) );
	}
	size_t lastLine = mLines.size() - 1;
	for ( size_t i = 0; i <= lastLine; i++ ) {
		std::string utf8( mLines[i].toUtf8() );
		if ( i == lastLine && utf8.size() > 1 && utf8[utf8.size() - 1] == '\n' ) {
			// Last \n is added by the document but it's not part of the document.
			utf8.pop_back();
		}
		if ( mIsCLRF ) {
			utf8[utf8.size() - 1] = '\r';
			stream.write( utf8.c_str(), utf8.size() );
			stream.write( &nl, 1 );
		} else {
			stream.write( utf8.c_str(), utf8.size() );
		}
	}
	cleanChangeId();
	return true;
}

bool TextDocument::save() {
	return save( mFilePath, mIsBOM );
}

const std::string TextDocument::getFilename() const {
	return mFilePath;
}

void TextDocument::setSelection( TextPosition position ) {
	setSelection( position, position );
}

void TextDocument::setSelection( TextPosition start, TextPosition end, bool swap ) {
	if ( ( start == mSelection.start() && end == mSelection.end() && !swap ) ||
		 ( start == mSelection.end() && end == mSelection.start() && swap ) )
		return;

	if ( swap ) {
		auto posT = start;
		start = end;
		end = posT;
	}

	if ( start == end ) {
		start = end = sanitizePosition( start );
	} else {
		start = sanitizePosition( start );
		end = sanitizePosition( end );
	}

	if ( mSelection != TextRange( start, end ) ) {
		mSelection.set( start, end );
		notifyCursorChanged();
		notifySelectionChanged();
	}
}

void TextDocument::setSelection( TextRange range ) {
	setSelection( range.start(), range.end() );
}

TextRange TextDocument::getSelection( bool sort ) const {
	return sort ? mSelection.normalized() : mSelection;
}

const TextRange& TextDocument::getSelection() const {
	return mSelection;
}

TextDocumentLine& TextDocument::line( const size_t& index ) {
	return mLines[index];
}

const TextDocumentLine& TextDocument::line( const size_t& index ) const {
	return mLines[index];
}

size_t TextDocument::linesCount() const {
	return mLines.size();
}

std::vector<TextDocumentLine>& TextDocument::lines() {
	return mLines;
}

bool TextDocument::hasSelection() const {
	return mSelection.start() != mSelection.end();
}

String TextDocument::getText( const TextRange& range ) const {
	TextRange nrange = range.normalized();
	if ( nrange.start().line() == nrange.end().line() ) {
		return mLines[nrange.start().line()].substr(
			nrange.start().column(), nrange.end().column() - nrange.start().column() );
	}
	std::vector<String> lines = {mLines[nrange.start().line()].substr( nrange.start().column() )};
	for ( auto i = nrange.start().line() + 1; i <= nrange.end().line() - 1; i++ ) {
		lines.emplace_back( mLines[i].getText() );
	}
	lines.emplace_back( mLines[nrange.end().line()].substr( 0, nrange.end().column() ) );
	return String::join( lines, -1 );
}

String TextDocument::getSelectedText() const {
	return getText( getSelection() );
}

String::StringBaseType TextDocument::getChar( const TextPosition& position ) const {
	auto pos = sanitizePosition( position );
	return mLines[pos.line()][pos.column()];
}

TextPosition TextDocument::insert( const TextPosition& position, const String& text ) {
	mUndoStack.clearRedoStack();
	return insert( position, text, mUndoStack.getUndoStackContainer(), mTimer.getElapsedTime() );
}

TextPosition TextDocument::insert( const TextPosition& position, const String& text,
								   UndoStackContainer& undoStack, const Time& time ) {
	TextPosition cursor = position;
	size_t lineCount = mLines.size();

	for ( size_t i = 0; i < text.length(); ++i ) {
		cursor = insert( cursor, text[i] );
	}

	mUndoStack.pushSelection( undoStack, getSelection(), time );
	mUndoStack.pushRemove( undoStack, {position, cursor}, time );

	notifyTextChanged();

	if ( lineCount != mLines.size() ) {
		notifyLineCountChanged( lineCount, mLines.size() );
	}

	return cursor;
}

TextPosition TextDocument::insert( TextPosition position, const String::StringBaseType& ch ) {
	position = sanitizePosition( position );
	bool atHead = position.column() == 0;
	bool atTail = position.column() == (Int64)line( position.line() ).length() - 1;
	if ( ch == '\n' ) {
		if ( atTail || atHead ) {
			size_t row = position.line();
			String line_content;
			for ( size_t i = position.column(); i < line( row ).length(); i++ )
				line_content.append( line( row )[i] );
			mLines.insert( mLines.begin() + position.line() + ( atTail ? 1 : 0 ), String( "\n" ) );
			notifyLineChanged( position.line() );
			return atTail
					   ? TextPosition( position.line() + 1, line( position.line() + 1 ).length() )
					   : TextPosition( position.line() + 1, 0 );
		}
		TextDocumentLine newLine( line( position.line() )
									  .substr( position.column(), line( position.line() ).length() -
																	  position.column() ) );
		TextDocumentLine& oldLine = line( position.line() );
		oldLine.setText( line( position.line() ).substr( 0, position.column() ) );
		// TODO: Investigate why this is needed when undo is used.
		// This fixes the case when a line ends up without an \n at the end of it.
		if ( oldLine.empty() || oldLine[oldLine.size() - 1] != '\n' ) {
			oldLine.append( '\n' );
		}
		if ( newLine.empty() || newLine[newLine.size() - 1] != '\n' ) {
			newLine.append( '\n' );
		}
		mLines.insert( mLines.begin() + position.line() + 1, std::move( newLine ) );
		notifyLineChanged( position.line() );
		return {position.line() + 1, 0};
	}
	line( position.line() ).insertChar( position.column(), ch );
	notifyLineChanged( position.line() );
	return {position.line(), position.column() + 1};
}

void TextDocument::remove( TextPosition position ) {
	remove( TextRange( position, position ) );
}

void TextDocument::remove( TextRange range ) {
	size_t lineCount = mLines.size();
	mUndoStack.clearRedoStack();
	range = range.normalized();
	range.setStart( sanitizePosition( range.start() ) );
	range.setEnd( sanitizePosition( range.end() ) );
	remove( range, mUndoStack.getUndoStackContainer(), mTimer.getElapsedTime() );
	if ( lineCount != mLines.size() ) {
		notifyLineCountChanged( lineCount, mLines.size() );
	}
}

void TextDocument::remove( TextRange range, UndoStackContainer& undoStack, const Time& time ) {
	if ( !range.isValid() )
		return;

	mUndoStack.pushSelection( undoStack, getSelection(), time );
	mUndoStack.pushInsert( undoStack, getText( range ), range.start(), time );

	// First delete all the lines in between the first and last one.
	for ( auto i = range.start().line() + 1; i < range.end().line(); ) {
		mLines.erase( mLines.begin() + i );
		range.end().setLine( range.end().line() - 1 );
	}

	if ( range.start().line() == range.end().line() ) {
		// Delete within same line.
		TextDocumentLine& line = this->line( range.start().line() );
		bool wholeLineIsSelected =
			range.start().column() == 0 && range.end().column() == (Int64)line.length();

		if ( wholeLineIsSelected ) {
			line = "\n";
		} else {
			auto beforeSelection = line.substr( 0, range.start().column() );
			auto afterSelection =
				!line.empty()
					? line.substr( range.end().column(), line.length() - range.end().column() )
					: "";

			if ( !beforeSelection.empty() && beforeSelection[beforeSelection.size() - 1] == '\n' )
				beforeSelection = beforeSelection.substr( 0, beforeSelection.size() - 1 );
			if ( afterSelection.empty() || afterSelection[afterSelection.size() - 1] != '\n' )
				afterSelection += '\n';

			line.setText( beforeSelection + afterSelection );
		}
	} else {
		// Delete across a newline, merging lines.
		eeASSERT( range.start().line() == range.end().line() - 1 );
		TextDocumentLine& firstLine = line( range.start().line() );
		TextDocumentLine& secondLine = line( range.end().line() );
		auto beforeSelection = firstLine.substr( 0, range.start().column() );
		auto afterSelection = !secondLine.empty()
								  ? secondLine.substr( range.end().column(),
													   secondLine.length() - range.end().column() )
								  : "";

		if ( !beforeSelection.empty() && beforeSelection[beforeSelection.size() - 1] == '\n' )
			beforeSelection = beforeSelection.substr( 0, beforeSelection.size() - 1 );
		if ( afterSelection.empty() || afterSelection[afterSelection.size() - 1] != '\n' )
			afterSelection += '\n';

		firstLine.setText( beforeSelection + afterSelection );
		mLines.erase( mLines.begin() + range.end().line() );
	}

	if ( lines().empty() ) {
		mLines.emplace_back( String( "\n" ) );
	}
	notifyTextChanged();
	notifyLineChanged( range.start().line() );
}

TextPosition TextDocument::positionOffset( TextPosition position, int columnOffset ) const {
	position = sanitizePosition( position );
	position.setColumn( position.column() + columnOffset );
	while ( position.line() > 0 && position.column() < 0 ) {
		position.setLine( position.line() - 1 );
		position.setColumn( eemax<Int64>( 0, position.column() + mLines[position.line()].size() ) );
	}
	while ( position.line() < (Int64)mLines.size() - 1 &&
			position.column() > (Int64)eemax<Int64>( 0, mLines[position.line()].size() - 1 ) ) {
		position.setColumn( position.column() - mLines[position.line()].size() - 1 );
		position.setLine( position.line() + 1 );
	}
	return sanitizePosition( position );
}

TextPosition TextDocument::positionOffset( TextPosition position, TextPosition offset ) const {
	return sanitizePosition( position + offset );
}

TextPosition TextDocument::nextChar( TextPosition position ) const {
	return positionOffset( position, TextPosition( 0, 1 ) );
}

TextPosition TextDocument::previousChar( TextPosition position ) const {
	return positionOffset( position, TextPosition( 0, -1 ) );
}

TextPosition TextDocument::previousWordBoundary( TextPosition position ) const {
	auto ch = getChar( positionOffset( position, -1 ) );
	bool inWord = !isNonWord( ch );
	String::StringBaseType nextChar = 0;
	do {
		TextPosition curPos = position;
		position = positionOffset( position, -1 );
		if ( curPos == position ) {
			break;
		}
		nextChar = getChar( positionOffset( position, -1 ) );
	} while ( ( inWord && !isNonWord( nextChar ) ) || ( !inWord && nextChar == ch ) );
	return position;
}

TextPosition TextDocument::nextWordBoundary( TextPosition position ) const {
	auto ch = getChar( position );
	bool inWord = !isNonWord( ch );
	String::StringBaseType nextChar = 0;
	do {
		TextPosition curPos = position;
		position = positionOffset( position, 1 );
		if ( curPos == position ) {
			break;
		}
		nextChar = getChar( position );
	} while ( ( inWord && !isNonWord( nextChar ) ) || ( !inWord && nextChar == ch ) );
	return position;
}

TextPosition TextDocument::startOfWord( TextPosition position ) const {
	while ( true ) {
		TextPosition curPos = positionOffset( position, -1 );
		String::StringBaseType ch = getChar( curPos );
		if ( isNonWord( ch ) || position == curPos ) {
			break;
		}
		position = curPos;
	}
	return position;
}

TextPosition TextDocument::endOfWord( TextPosition position ) const {
	while ( true ) {
		TextPosition curPos = positionOffset( position, 1 );
		String::StringBaseType ch = getChar( position );
		if ( isNonWord( ch ) || position == curPos ) {
			break;
		}
		position = curPos;
	}
	return position;
}

TextPosition TextDocument::startOfLine( TextPosition position ) const {
	position = sanitizePosition( position );
	return TextPosition( position.line(), 0 );
}

TextPosition TextDocument::endOfLine( TextPosition position ) const {
	position = sanitizePosition( position );
	return TextPosition( position.line(), mLines[position.line()].size() - 1 );
}

TextPosition TextDocument::startOfContent( TextPosition start ) {
	start = sanitizePosition( start );
	const String& ln = line( start.line() ).getText();
	size_t to = start.column();
	int indent = 0;
	for ( size_t i = 0; i < to; i++ ) {
		if ( '\t' == ln[i] || ' ' == ln[i] ) {
			indent++;
		} else {
			break;
		}
	}
	return {start.line(), indent};
}

TextPosition TextDocument::startOfDoc() const {
	return TextPosition( 0, 0 );
}

TextPosition TextDocument::endOfDoc() const {
	return TextPosition( mLines.size() - 1, mLines[mLines.size() - 1].size() - 1 );
}

void TextDocument::deleteTo( int offset ) {
	TextPosition cursorPos = getSelection( true ).start();
	if ( hasSelection() ) {
		remove( getSelection() );
	} else {
		TextPosition delPos = positionOffset( cursorPos, offset );
		TextRange range( cursorPos, delPos );
		remove( range );
		range = range.normalized();
		cursorPos = range.start();
	}
	setSelection( cursorPos );
}

void TextDocument::deleteSelection() {
	TextPosition cursorPos = getSelection( true ).start();
	remove( getSelection() );
	setSelection( cursorPos );
}

void TextDocument::selectTo( TextPosition position ) {
	setSelection( TextRange( sanitizePosition( position ), getSelection().end() ) );
}

void TextDocument::selectTo( int offset ) {
	const TextRange& range = getSelection();
	TextPosition posOffset = positionOffset( range.start(), offset );
	setSelection( TextRange( posOffset, range.end() ) );
}

void TextDocument::moveTo( TextPosition offset ) {
	setSelection( offset );
}

void TextDocument::moveTo( int columnOffset ) {
	setSelection( positionOffset( getSelection().start(), columnOffset ) );
}

void TextDocument::textInput( const String& text ) {
	if ( hasSelection() ) {
		deleteTo( 0 );
	}
	setSelection( insert( getSelection().start(), text ) );
}

void TextDocument::registerClient( TextDocument::Client& client ) {
	mClients.insert( &client );
}

void TextDocument::unregisterClient( TextDocument::Client& client ) {
	mClients.erase( &client );
}

void TextDocument::moveToPreviousChar() {
	if ( hasSelection() ) {
		setSelection( getSelection( true ).start() );
	} else {
		setSelection( positionOffset( getSelection().start(), -1 ) );
	}
}

void TextDocument::moveToNextChar() {
	if ( hasSelection() ) {
		setSelection( getSelection( true ).end() );
	} else {
		setSelection( positionOffset( getSelection().start(), 1 ) );
	}
}

void TextDocument::moveToPreviousWord() {
	if ( hasSelection() ) {
		setSelection( getSelection( true ).start() );
	} else {
		setSelection( previousWordBoundary( getSelection().start() ) );
	}
}

void TextDocument::moveToNextWord() {
	if ( hasSelection() ) {
		setSelection( getSelection( true ).end() );
	} else {
		setSelection( nextWordBoundary( getSelection().start() ) );
	}
}

void TextDocument::moveToPreviousLine() {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() - 1 );
	setSelection( pos );
}

void TextDocument::moveToNextLine() {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() + 1 );
	setSelection( pos );
}

void TextDocument::moveToPreviousPage( Int64 pageSize ) {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() - pageSize );
	setSelection( pos );
}

void TextDocument::moveToNextPage( Int64 pageSize ) {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() + pageSize );
	setSelection( pos );
}

void TextDocument::moveToStartOfDoc() {
	setSelection( startOfDoc() );
}

void TextDocument::moveToEndOfDoc() {
	setSelection( endOfDoc() );
}

void TextDocument::moveToStartOfContent() {
	TextPosition start = getSelection().start();
	TextPosition indented = startOfContent( getSelection().start() );
	setSelection( indented.column() == start.column() ? TextPosition( start.line(), 0 )
													  : indented );
}

void TextDocument::selectToStartOfContent() {
	TextPosition start = getSelection().start();
	TextPosition indented = startOfContent( getSelection().start() );
	setSelection( {indented.column() == start.column() ? TextPosition( start.line(), 0 ) : indented,
				   getSelection().end()} );
}

void TextDocument::moveToStartOfLine() {
	setSelection( startOfLine( getSelection().start() ) );
}

void TextDocument::moveToEndOfLine() {
	setSelection( endOfLine( getSelection().start() ) );
}

void TextDocument::deleteToPreviousChar() {
	deleteTo( -1 );
}

void TextDocument::deleteToNextChar() {
	deleteTo( 1 );
}

void TextDocument::deleteToPreviousWord() {
	deleteTo( previousWordBoundary( getSelection().start() ) );
}

void TextDocument::deleteToNextWord() {
	deleteTo( nextWordBoundary( getSelection().start() ) );
}

void TextDocument::selectToPreviousChar() {
	selectTo( -1 );
}

void TextDocument::selectToNextChar() {
	selectTo( 1 );
}

void TextDocument::selectToPreviousWord() {
	setSelection( {previousWordBoundary( getSelection().start() ), getSelection().end()} );
}

void TextDocument::selectToNextWord() {
	setSelection( {nextWordBoundary( getSelection().start() ), getSelection().end()} );
}

void TextDocument::selectWord() {
	setSelection( {nextWordBoundary( getSelection().start() ),
				   previousWordBoundary( getSelection().start() )} );
}

void TextDocument::selectToPreviousLine() {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() - 1 );
	setSelection( TextRange( pos, getSelection().end() ) );
}

void TextDocument::selectToNextLine() {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() + 1 );
	setSelection( TextRange( pos, getSelection().end() ) );
}

void TextDocument::selectToStartOfLine() {
	selectTo( startOfLine( getSelection().start() ) );
}

void TextDocument::selectToEndOfLine() {
	selectTo( endOfLine( getSelection().start() ) );
}

void TextDocument::selectToStartOfDoc() {
	selectTo( startOfDoc() );
}

void TextDocument::selectToEndOfDoc() {
	selectTo( endOfDoc() );
}

void TextDocument::selectToPreviousPage( Int64 pageSize ) {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() - pageSize );
	selectTo( pos );
}

void TextDocument::selectToNextPage( Int64 pageSize ) {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() + pageSize );
	selectTo( pos );
}

void TextDocument::selectAll() {
	setSelection( startOfDoc(), endOfDoc() );
}

void TextDocument::newLine() {
	String input( "\n" );
	TextPosition start = getSelection().start();
	TextPosition indent = startOfContent( getSelection().start() );
	if ( indent.column() != 0 )
		input.append( line( start.line() ).getText().substr( 0, indent.column() ) );
	textInput( input );
}

void TextDocument::newLineAbove() {
	String input( "\n" );
	TextPosition start = getSelection().start();
	TextPosition indent = startOfContent( getSelection().start() );
	if ( indent.column() != 0 )
		input.insert( 0, line( start.line() ).getText().substr( 0, indent.column() ) );
	insert( {start.line(), 0}, input );
	setSelection( {start.line(), (Int64)input.size()} );
}

void TextDocument::insertAtStartOfSelectedLines( const String& text, bool skipEmpty ) {
	TextPosition prevStart = getSelection().start();
	TextRange range = getSelection( true );
	bool swap = prevStart != range.start();
	for ( auto i = range.start().line(); i <= range.end().line(); i++ ) {
		const String& line = this->line( i ).getText();
		if ( !skipEmpty || line.length() != 1 ) {
			insert( {i, 0}, text );
		}
	}
	setSelection( TextPosition( range.start().line(), range.start().column() + text.size() ),
				  TextPosition( range.end().line(), range.end().column() + text.size() ), swap );
}

void TextDocument::removeFromStartOfSelectedLines( const String& text, bool skipEmpty ) {
	TextPosition prevStart = getSelection().start();
	TextRange range = getSelection( true );
	bool swap = prevStart != range.start();
	for ( auto i = range.start().line(); i <= range.end().line(); i++ ) {
		const String& line = this->line( i ).getText();
		if ( !skipEmpty || line.length() != 1 ) {
			if ( line.substr( 0, text.length() ) == text ) {
				remove( {{i, 0}, {i, static_cast<Int64>( text.length() )}} );
			}
		}
	}
	setSelection( TextPosition( range.start().line(), range.start().column() - text.size() ),
				  TextPosition( range.end().line(), range.end().column() - text.size() ), swap );
}

void TextDocument::indent() {
	if ( hasSelection() ) {
		insertAtStartOfSelectedLines( getIndentString(), false );
	} else {
		textInput( getIndentString() );
	}
}

void TextDocument::unindent() {
	removeFromStartOfSelectedLines( getIndentString(), false );
}

void TextDocument::moveLinesUp() {
	TextRange range = getSelection( true );
	bool swap = getSelection( true ) != getSelection();
	appendLineIfLastLine( range.end().line() );
	if ( range.start().line() > 0 ) {
		auto& text = line( range.start().line() - 1 );
		insert( {range.end().line() + 1, 0}, text.getText() );
		remove( {{range.start().line() - 1, 0}, {range.start().line(), 0}} );
		setSelection( {range.start().line() - 1, range.start().column()},
					  {range.end().line() - 1, range.end().column()}, swap );
	}
}

void TextDocument::moveLinesDown() {
	TextRange range = getSelection( true );
	bool swap = getSelection( true ) != getSelection();
	appendLineIfLastLine( range.end().line() + 1 );
	if ( range.end().line() < (Int64)mLines.size() - 1 ) {
		auto text = line( range.end().line() + 1 );
		remove( {{range.end().line() + 1, 0}, {range.end().line() + 2, 0}} );
		insert( {range.start().line(), 0}, text.getText() );
		setSelection( {range.start().line() + 1, range.start().column()},
					  {range.end().line() + 1, range.end().column()}, swap );
	}
}

void TextDocument::appendLineIfLastLine( Int64 line ) {
	if ( line >= (Int64)mLines.size() - 1 ) {
		insert( endOfDoc(), "\n" );
	}
}

String TextDocument::getIndentString() {
	if ( IndentSpaces == mIndentType ) {
		return String( std::string( mTabWidth, ' ' ) );
	}
	return String( "\t" );
}

const Uint32& TextDocument::getTabWidth() const {
	return mTabWidth;
}

void TextDocument::setTabWidth( const Uint32& tabWidth ) {
	mTabWidth = tabWidth;
}

void TextDocument::deleteTo( TextPosition position ) {
	TextPosition cursorPos = getSelection( true ).start();
	if ( hasSelection() ) {
		remove( getSelection() );
	} else {
		TextRange range( cursorPos, position );
		remove( range );
		range = range.normalized();
		cursorPos = range.start();
	}
	setSelection( cursorPos );
}

void TextDocument::print() const {
	for ( size_t i = 0; i < mLines.size(); i++ )
		printf( "%s", mLines[i].toUtf8().c_str() );
}

TextPosition TextDocument::sanitizePosition( const TextPosition& position ) const {
	Int64 line = eeclamp<Int64>( position.line(), 0UL, mLines.size() - 1 );
	Int64 col =
		eeclamp<Int64>( position.column(), 0UL, eemax<Int64>( 0, mLines[line].size() - 1 ) );
	return {line, col};
}

const TextDocument::IndentType& TextDocument::getIndentType() const {
	return mIndentType;
}

void TextDocument::setIndentType( const IndentType& indentType ) {
	mIndentType = indentType;
}

void TextDocument::undo() {
	mUndoStack.undo();
}

void TextDocument::redo() {
	mUndoStack.redo();
}

const SyntaxDefinition& TextDocument::getSyntaxDefinition() const {
	return mSyntaxDefinition;
}

Uint64 TextDocument::getCurrentChangeId() const {
	return mUndoStack.getCurrentChangeId();
}

const std::string& TextDocument::getDefaultFileName() const {
	return mDefaultFileName;
}

void TextDocument::setDefaultFileName( const std::string& defaultFileName ) {
	mDefaultFileName = defaultFileName;
}

const std::string& TextDocument::getFilePath() const {
	return mFilePath;
}

bool TextDocument::isDirty() const {
	return mCleanChangeId != getCurrentChangeId();
}

void TextDocument::execute( const std::string& command ) {
	auto cmdIt = mCommands.find( command );
	if ( cmdIt != mCommands.end() ) {
		cmdIt->second();
	}
}

void TextDocument::setCommand( const std::string& command, TextDocument::DocumentCommand func ) {
	mCommands[command] = func;
}

const Uint32& TextDocument::getPageSize() const {
	return mPageSize;
}

void TextDocument::setPageSize( const Uint32& pageSize ) {
	mPageSize = pageSize;
}

void TextDocument::cleanChangeId() {
	mCleanChangeId = getCurrentChangeId();
}

void TextDocument::notifyTextChanged() {
	for ( auto& client : mClients ) {
		client->onDocumentTextChanged();
	}
}

void TextDocument::notifyCursorChanged() {
	for ( auto& client : mClients ) {
		client->onDocumentCursorChange( getSelection().start() );
	}
}

void TextDocument::notifySelectionChanged() {
	for ( auto& client : mClients ) {
		client->onDocumentSelectionChange( getSelection() );
	}
}

void TextDocument::notifyLineCountChanged( const size_t& lastCount, const size_t& newCount ) {
	for ( auto& client : mClients ) {
		client->onDocumentLineCountChange( lastCount, newCount );
	}
}

void TextDocument::notifyLineChanged( const Int64& lineIndex ) {
	for ( auto& client : mClients ) {
		client->onDocumentLineChanged( lineIndex );
	}
}

void TextDocument::initializeCommands() {
	mCommands["reset"] = [&] { reset(); };
	mCommands["save"] = [&] { save(); };
	mCommands["delete-to-previous-word"] = [&] { deleteToPreviousWord(); };
	mCommands["delete-to-previous-char"] = [&] { deleteToPreviousChar(); };
	mCommands["delete-to-next-word"] = [&] { deleteToNextWord(); };
	mCommands["delete-to-next-char"] = [&] { deleteToNextChar(); };
	mCommands["delete-selection"] = [&] { deleteSelection(); };
	mCommands["move-to-previous-char"] = [&] { moveToPreviousChar(); };
	mCommands["move-to-previous-word"] = [&] { moveToPreviousWord(); };
	mCommands["move-to-next-char"] = [&] { moveToNextChar(); };
	mCommands["move-to-next-word"] = [&] { moveToNextWord(); };
	mCommands["move-to-previous-line"] = [&] { moveToPreviousLine(); };
	mCommands["move-to-next-line"] = [&] { moveToNextLine(); };
	mCommands["move-to-previous-page"] = [&] { moveToPreviousPage( mPageSize ); };
	mCommands["move-to-next-page"] = [&] { moveToNextPage( mPageSize ); };
	mCommands["move-to-start-of-doc"] = [&] { moveToStartOfDoc(); };
	mCommands["move-to-end-of-doc"] = [&] { moveToEndOfDoc(); };
	mCommands["move-to-start-of-line"] = [&] { moveToStartOfLine(); };
	mCommands["move-to-end-of-line"] = [&] { moveToEndOfLine(); };
	mCommands["move-to-start-of-content"] = [&] { moveToStartOfContent(); };
	mCommands["move-lines-up"] = [&] { moveLinesUp(); };
	mCommands["move-lines-down"] = [&] { moveLinesDown(); };
	mCommands["select-to-previous-char"] = [&] { selectToPreviousChar(); };
	mCommands["select-to-previous-word"] = [&] { selectToPreviousWord(); };
	mCommands["select-to-previous-line"] = [&] { selectToPreviousLine(); };
	mCommands["select-to-next-char"] = [&] { selectToNextChar(); };
	mCommands["select-to-next-word"] = [&] { selectToNextWord(); };
	mCommands["select-to-next-line"] = [&] { selectToNextLine(); };
	mCommands["select-word"] = [&] { selectWord(); };
	mCommands["select-to-start-of-line"] = [&] { selectToStartOfLine(); };
	mCommands["select-to-end-of-line"] = [&] { selectToEndOfLine(); };
	mCommands["select-to-start-of-doc"] = [&] { selectToStartOfDoc(); };
	mCommands["select-to-start-of-content"] = [&] { selectToStartOfContent(); };
	mCommands["select-to-end-of-doc"] = [&] { selectToEndOfDoc(); };
	mCommands["select-to-previous-page"] = [&] { selectToPreviousPage( mPageSize ); };
	mCommands["select-to-next-page"] = [&] { selectToNextPage( mPageSize ); };
	mCommands["select-all"] = [&] { selectAll(); };
	mCommands["new-line"] = [&] { newLine(); };
	mCommands["new-line-above"] = [&] { newLineAbove(); };
	mCommands["indent"] = [&] { indent(); };
	mCommands["unindent"] = [&] { unindent(); };
	mCommands["undo"] = [&] { undo(); };
	mCommands["redo"] = [&] { redo(); };
}

TextDocument::Client::~Client() {}

}}} // namespace EE::UI::Doc
