#include <eepp/core/debug.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <sstream>
#include <string>

namespace EE { namespace UI { namespace Doc {

const char NON_WORD_CHARS[] = " \t\n/\\()\"':,.;<>~!@#$%^&*|+=[]{}`?-";

bool TextDocument::isNonWord( String::StringBaseType ch ) {
	for ( size_t i = 0; i < eeARRAY_SIZE( NON_WORD_CHARS ); i++ ) {
		if ( static_cast<String::StringBaseType>( NON_WORD_CHARS[i] ) == ch ) {
			return true;
		}
	}
	return false;
}

TextDocument::TextDocument() {}

void TextDocument::reset() {
	mFilename = "unsaved";
	mSelection.set( {0, 0}, {0, 0} );
	mLines.clear();
	notifyTextChanged();
	notifyCursorChanged();
	notifySelectionChanged();
}

void TextDocument::loadFromPath( const std::string& path ) {
	reset();
	mFilename = path;
	std::string line;
	std::ifstream file( path );
	while ( std::getline( file, line ) ) {
		std::istringstream iss( line );
		if ( mLines.empty() && line.size() >= 3 ) {
			// Check UTF-8 BOM header
			if ( (char)0xef == line[0] && (char)0xbb == line[1] && (char)0xbf == line[2] ) {
				line = line.substr( 3 );
			}
		}
		// Check CLRF
		if ( !line.empty() && line[line.size() - 1] == '\r' ) {
			line = line.substr( 0, line.size() - 1 );
			mIsCLRF = true;
		}
		mLines.emplace_back( String( line + "\n" ) );
	}
	if ( mLines.empty() ) {
		mLines.emplace_back( String( "\n" ) );
	} else if ( mLines[mLines.size() - 1].at( mLines[mLines.size() - 1].size() - 1 ) == '\n' ) {
		mLines.emplace_back( String( "\n" ) );
	}
}

void TextDocument::save( const std::string& ) {}

const std::string TextDocument::getFilename() const {
	return mFilename;
}

void TextDocument::setSelection( TextPosition position ) {
	setSelection( position, position );
}

void TextDocument::setSelection( TextPosition start, TextPosition end, bool swap ) {
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

String& TextDocument::line( const size_t& index ) {
	return mLines[index];
}

const String& TextDocument::line( const size_t& index ) const {
	return mLines[index];
}

size_t TextDocument::lineCount() const {
	return mLines.size();
}

std::vector<String>& TextDocument::lines() {
	return mLines;
}

bool TextDocument::hasSelection() const {
	return mSelection.start() != mSelection.end();
}

String TextDocument::getText( const TextRange& range ) const {
	TextRange nrange = range.normalized();
	if ( nrange.start().line() == nrange.end().line() ) {
		return mLines[nrange.start().line()].substr( nrange.start().column(),
													 nrange.end().column() );
	}
	std::vector<String> lines = {mLines[nrange.start().line()].substr( nrange.start().column() )};
	for ( auto i = nrange.start().line() + 1; i < nrange.end().line() - 1; i++ ) {
		lines.emplace_back( mLines[i] );
	}
	lines.emplace_back( mLines[nrange.end().line()].substr( 0, nrange.end().column() ) );
	return String::join( lines, -1 );
}

String::StringBaseType TextDocument::getChar( const TextPosition& position ) const {
	auto pos = sanitizePosition( position );
	return mLines[pos.line()][pos.column()];
}

TextPosition TextDocument::insert( const TextPosition& position, const String& text ) {
	TextPosition cursor = position;
	for ( size_t i = 0; i < text.length(); ++i )
		cursor = insert( cursor, text[i] );
	return cursor;
}

TextPosition TextDocument::insert( TextPosition position, const String::StringBaseType& ch ) {
	position = sanitizePosition( position );
	bool atHead = position.column() == 0;
	bool atTail = position.column() == (Int64)line( position.line() ).length() - 1;
	if ( ch == '\n' ) {
		if ( atTail || atHead ) {
			String newLineContents( "\n" );
			size_t row = position.line();
			String line_content;
			for ( size_t i = position.column(); i < line( row ).length(); i++ )
				line_content.append( line( row )[i] );
			mLines.insert( mLines.begin() + position.line() + ( atTail ? 1 : 0 ), newLineContents );
			notifyTextChanged();
			if ( atTail )
				return {position.line() + 1, (Int64)line( position.line() + 1 ).length()};
			return {position.line() + 1, 0};
		}
		String newLine =
			line( position.line() )
				.substr( position.column(), line( position.line() ).length() - position.column() );
		line( position.line() ) = line( position.line() ).substr( 0, position.column() );
		mLines.insert( mLines.begin() + position.line() + 1, std::move( newLine ) );
		notifyTextChanged();
		return {position.line() + 1, 0};
	} /* else if ( ch == '\t' ) {
		 Int64 nextSoftTabStop =
			( ( position.column() + tabWidth ) / getTabWidth() ) * getTabWidth();
		 size_t spacesToInsert = nextSoftTabStop - position.column();
		 for ( size_t i = 0; i < spacesToInsert; ++i ) {
			 line( position.line() )
				 .insert( line( position.line() ).begin() + position.column(), ' ' );
		 }
		 return {position.line(), nextSoftTabStop};
	 }*/
	line( position.line() ).insert( line( position.line() ).begin() + position.column(), ch );
	notifyTextChanged();
	return {position.line(), position.column() + 1};
}

void TextDocument::remove( TextPosition position ) {
	remove( TextRange( position, position ) );
}

void TextDocument::remove( TextRange range ) {
	if ( !range.isValid() )
		return;

	range = range.normalized();
	range.setStart( sanitizePosition( range.start() ) );
	range.setEnd( sanitizePosition( range.end() ) );

	// First delete all the lines in between the first and last one.
	for ( auto i = range.start().line() + 1; i < range.end().line(); ) {
		mLines.erase( mLines.begin() + i );
		range.end().setLine( range.end().line() - 1 );
	}

	if ( range.start().line() == range.end().line() ) {
		// Delete within same line.
		auto& line = this->line( range.start().line() );
		bool wholeLineIsSelected =
			range.start().column() == 0 && range.end().column() == (Int64)line.length();

		if ( wholeLineIsSelected ) {
			line.clear();
		} else {
			auto beforeSelection = line.substr( 0, range.start().column() );
			auto afterSelection =
				line.substr( range.end().column(), line.length() - range.end().column() );
			line.assign( beforeSelection + afterSelection );
		}
	} else {
		// Delete across a newline, merging lines.
		eeASSERT( range.start().line() == range.end().line() - 1 );
		auto& firstLine = line( range.start().line() );
		auto& secondLine = line( range.end().line() );
		auto beforeSelection = firstLine.substr( 0, range.start().column() );
		auto afterSelection =
			secondLine.substr( range.end().column(), secondLine.length() - range.end().column() );
		firstLine.assign( beforeSelection + afterSelection );
		mLines.erase( mLines.begin() + range.end().line() );
	}

	if ( lines().empty() ) {
		mLines.emplace_back( String() );
	}
	notifyTextChanged();
}

TextPosition TextDocument::positionOffset( TextPosition position, int columnOffset ) const {
	position = sanitizePosition( position );
	position.setColumn( position.column() + columnOffset );
	while ( position.line() > 0 && position.column() < 0 ) {
		position.setLine( position.line() - 1 );
		position.setColumn( eemax<Int64>( 0, position.column() + mLines[position.line()].size() ) );
	}
	while ( position.line() < (Int64)mLines.size() &&
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
	} while ( ( inWord && isNonWord( nextChar ) ) || ( !inWord && nextChar != ch ) );
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
	} while ( ( inWord && isNonWord( nextChar ) ) || ( !inWord && nextChar != ch ) );
	return position;
}

TextPosition TextDocument::startOfWord( TextPosition position ) const {
	while ( true ) {
		TextPosition curPos = positionOffset( position, -1 );
		String::StringBaseType ch = getChar( curPos );
		if ( isNonWord( ch ) or position == curPos ) {
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
		if ( isNonWord( ch ) or position == curPos ) {
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

TextPosition TextDocument::startOfDoc() const {
	return TextPosition( 0, 0 );
}

TextPosition TextDocument::endOfDoc() const {
	return TextPosition( mLines.size() - 1, mLines[mLines.size() - 1].size() - 1 );
}

TextPosition TextDocument::getAbsolutePosition( TextPosition position ) const {
	position = sanitizePosition( position );
	const String& string = line( position.line() );
	size_t tabCount = string.substr( 0, position.column() ).countChar( '\t' );
	return TextPosition( position.line(), position.column() - tabCount + tabCount * getTabWidth() );
}

Int64 TextDocument::getRelativeColumnOffset( TextPosition position ) const {
	const String& line = mLines[position.line()];
	Int64 length = eemin<Int64>( position.column(), line.size() - 1 );
	Int64 offset = 0;
	for ( Int64 i = 0; i <= length; ++i ) {
		if ( offset >= position.column() ) {
			return i;
		}
		if ( line[i] == '\t' ) {
			offset += getTabWidth();
		} else if ( line[i] != '\n' && line[i] != '\r' ) {
			offset += 1;
		}
	}
	return length;
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

void TextDocument::selectTo( TextPosition offset ) {
	TextRange range = getSelection();
	TextPosition posOffset = positionOffset( range.start(), offset );
	setSelection( TextRange( range.start(), posOffset ) );
}

void TextDocument::moveTo( TextPosition offset ) {
	setSelection( getSelection().start() + offset );
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
		TextRange selection = getSelection( true );
		setSelection( selection.end() );
	} else {
		setSelection( positionOffset( getSelection().start(), -1 ) );
	}
}

void TextDocument::moveToNextChar() {
	if ( hasSelection() ) {
		TextRange selection = getSelection( true );
		setSelection( selection.start() );
	} else {
		setSelection( positionOffset( getSelection().start(), 1 ) );
	}
}

void TextDocument::moveToPreviousLine( Int64 lastColIndex ) {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() - 1 );
	if ( pos.line() >= 0 ) {
		lastColIndex = getRelativeColumnOffset( TextPosition( pos.line(), lastColIndex ) );
	}
	pos.setColumn( lastColIndex );
	setSelection( pos );
}

void TextDocument::moveToNextLine( Int64 lastColIndex ) {
	TextPosition pos = getSelection().start();
	pos.setLine( pos.line() + 1 );
	if ( pos.line() < (Int64)mLines.size() ) {
		lastColIndex = getRelativeColumnOffset( TextPosition( pos.line(), lastColIndex ) );
	}
	pos.setColumn( lastColIndex );
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

void TextDocument::deleteToPreviousChar() {
	deleteTo( -1 );
}

void TextDocument::deleteToNextChar() {
	deleteTo( 1 );
}

void TextDocument::newLine() {
	String input( "\n" );
	TextPosition start = getSelection().start();
	if ( start.line() >= 0 && start.line() < (Int64)mLines.size() ) {
		String& ln = line( start.line() );
		size_t to = eemin<size_t>( ln.size(), start.column() );
		int indent = 0;
		for ( size_t i = 0; i < to; i++ ) {
			if ( '\t' == ln[i] || ' ' == ln[i] ) {
				indent++;
			} else {
				break;
			}
		}
		if ( indent ) {
			input.append( ln.substr( 0, indent ) );
		}
	}
	textInput( input );
}

void TextDocument::insertAtStartOfSelectedLines( String text, bool skipEmpty ) {
	TextPosition prevStart = getSelection().start();
	TextRange range = getSelection( true );
	bool swap = prevStart != range.start();
	for ( auto i = range.start().line(); i <= range.end().line(); i++ ) {
		const String& line = this->line( i );
		if ( !skipEmpty || line.length() != 1 ) {
			insert( {i, 0}, text );
		}
	}
	setSelection( TextPosition( range.start().line(), range.start().column() + text.size() ),
				  TextPosition( range.end().line(), range.end().column() + text.size() ), swap );
}

void TextDocument::removeFromStartOfSelectedLines( String text, bool skipEmpty ) {
	TextPosition prevStart = getSelection().start();
	TextRange range = getSelection( true );
	bool swap = prevStart != range.start();
	for ( auto i = range.start().line(); i <= range.end().line(); i++ ) {
		const String& line = this->line( i );
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

void TextDocument::deleteTo( TextPosition offset ) {
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

TextDocument::Client::~Client() {}

}}} // namespace EE::UI::Doc
