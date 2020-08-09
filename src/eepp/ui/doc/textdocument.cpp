#include <algorithm>
#include <cstdio>
#include <eepp/core/debug.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <sstream>
#include <string>

namespace EE { namespace UI { namespace Doc {

// Text document is loosely based on the SerenityOS (https://github.com/SerenityOS/serenity)
// TextDocument and the lite editor (https://github.com/rxi/lite) implementations.

const char DEFAULT_NON_WORD_CHARS[] = " \t\n/\\()\"':,.;<>~!@#$%^&*|+=[]{}`?-";

bool TextDocument::isNonWord( String::StringBaseType ch ) const {
	return mNonWordChars.find_first_of( ch ) != String::InvalidPos;
}

TextDocument::TextDocument( bool verbose ) :
	mUndoStack( this ),
	mDefaultFileName( "untitled" ),
	mCleanChangeId( 0 ),
	mNonWordChars( DEFAULT_NON_WORD_CHARS ) {
	initializeCommands();
	reset();
}

TextDocument::~TextDocument() {
	notifyDocumentClosed();
}

bool TextDocument::hasFilepath() {
	return mDefaultFileName != mFilePath;
}

bool TextDocument::isEmpty() {
	return linesCount() == 1 && line( 0 ).size() == 1;
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

static String ptrGetLine( char* data, const size_t& size, size_t& position ) {
	position = 0;
	while ( position < size && data[position] != '\n' )
		position++;
	if ( position < size )
		position++;
	return String( data, position );
}

bool TextDocument::loadFromStream( IOStream& file ) {
	return loadFromStream( file, "untitled" );
}

bool TextDocument::loadFromStream( IOStream& file, std::string path ) {
	Clock clock;
	reset();
	mLines.clear();
	if ( file.isOpen() ) {
		const size_t BLOCK_SIZE = EE_1MB;
		size_t total = file.getSize();
		size_t pending = total;
		size_t blockSize = eemin( total, BLOCK_SIZE );
		size_t read = 0;
		String lineBuffer;
		size_t position;
		int consume;
		char* bufferPtr;
		TScopedBuffer<char> data( blockSize );
		while ( pending ) {
			read = file.read( data.get(), blockSize );
			bufferPtr = data.get();
			consume = read;

			if ( pending == total ) {
				// Check UTF-8 BOM header
				if ( (char)0xef == data.get()[0] && (char)0xbb == data.get()[1] &&
					 (char)0xbf == data.get()[2] ) {
					bufferPtr += 3;
					consume -= 3;
					mIsBOM = true;
				}
			}

			while ( consume ) {
				lineBuffer += ptrGetLine( bufferPtr, consume, position );
				bufferPtr += position;
				consume -= position;

				if ( lineBuffer[lineBuffer.size() - 1] == '\n' || !consume ) {
					if ( mLines.empty() && lineBuffer.size() > 1 &&
						 lineBuffer[lineBuffer.size() - 2] == '\r' ) {
						mLineEnding = LineEnding::CRLF;
					}

					if ( mLineEnding == LineEnding::CRLF && lineBuffer.size() > 1 ) {
						lineBuffer[lineBuffer.size() - 2] = '\n';
						lineBuffer.resize( lineBuffer.size() - 1 );
					}

					mLines.push_back( lineBuffer );
					lineBuffer.resize( 0 );
				}

				if ( consume < 0 ) {
					eeASSERT( !consume );
					break;
				}
			}

			if ( !mLines.empty() ) {
				const String& lastLine = mLines[mLines.size() - 1].getText();
				if ( lastLine[lastLine.size() - 1] == '\n' ) {
					mLines.push_back( String( "\n" ) );
				} else {
					mLines[mLines.size() - 1].append( "\n" );
				}
			}

			if ( !read )
				break;
			pending -= read;
			blockSize = eemin( pending, BLOCK_SIZE );
		};
	}

	if ( mLines.empty() )
		mLines.push_back( String( "\n" ) );

	if ( mAutoDetectIndentType )
		guessIndentType();

	notifyTextChanged();

	if ( mVerbose )
		eePRINTL( "Document \"%s\" loaded in %.2fms.", path.c_str(),
				  clock.getElapsedTime().asMilliseconds() );
	return true;
}

void TextDocument::guessIndentType() {
	int guessSpaces = 0;
	int guessTabs = 0;
	std::map<int, int> guessWidth;
	int guessCoundown = 10;
	size_t linesCount = eemin<size_t>( 100, mLines.size() );
	for ( size_t i = 0; i < linesCount; i++ ) {
		const String& text = mLines[i].getText();
		std::string match =
			LuaPattern::match( text.size() > 128 ? text.substr( 0, 12 ) : text, "^  +" );
		if ( !match.empty() ) {
			guessSpaces++;
			guessWidth[match.size()]++;
			guessCoundown--;
		} else {
			match = LuaPattern::match( mLines[i].getText(), "^\t+" );
			if ( !match.empty() ) {
				guessTabs++;
				guessCoundown--;
				break; // if tab found asume tabs
			}
		}
		if ( guessCoundown == 0 )
			break;
	}
	if ( !guessTabs && !guessSpaces ) {
		return;
	}
	if ( guessTabs > guessSpaces ) {
		mIndentType = IndentType::IndentTabs;
	} else {
		mIndentType = IndentType::IndentSpaces;
		mIndentWidth = guessWidth.begin()->first;
	}

	if ( mIndentWidth == 0 )
		mIndentWidth = 4;
}

void TextDocument::resetSyntax() {
	String header( getText( {{0, 0}, positionOffset( {0, 0}, 128 )} ) );
	mSyntaxDefinition = SyntaxDefinitionManager::instance()->find( mFilePath, header );
}

bool TextDocument::getAutoDetectIndentType() const {
	return mAutoDetectIndentType;
}

void TextDocument::setAutoDetectIndentType( bool autodetect ) {
	mAutoDetectIndentType = autodetect;
}

const TextDocument::LineEnding& TextDocument::getLineEnding() const {
	return mLineEnding;
}

void TextDocument::setLineEnding( const LineEnding& lineEnding ) {
	mLineEnding = lineEnding;
}

bool TextDocument::getForceNewLineAtEndOfFile() const {
	return mForceNewLineAtEndOfFile;
}

void TextDocument::setForceNewLineAtEndOfFile( bool forceNewLineAtEndOfFile ) {
	mForceNewLineAtEndOfFile = forceNewLineAtEndOfFile;
}

bool TextDocument::getTrimTrailingWhitespaces() const {
	return mTrimTrailingWhitespaces;
}

void TextDocument::setTrimTrailingWhitespaces( bool trimTrailingWhitespaces ) {
	mTrimTrailingWhitespaces = trimTrailingWhitespaces;
}

TextDocument::Client* TextDocument::getActiveClient() const {
	return mActiveClient;
}

void TextDocument::setActiveClient( Client* activeClient ) {
	mActiveClient = activeClient;
}

void TextDocument::setBOM( bool active ) {
	mIsBOM = active;
}

bool TextDocument::getBOM() const {
	return mIsBOM;
}

bool TextDocument::loadFromFile( const std::string& path ) {
	if ( !FileSystem::fileExists( path ) && PackManager::instance()->isFallbackToPacksActive() ) {
		std::string pathFix( path );
		Pack* pack = PackManager::instance()->exists( pathFix );
		if ( NULL != pack ) {
			mFilePath = pathFix;
			return loadFromPack( pack, pathFix );
		}
	}

	IOStreamFile file( path, "rb" );
	bool ret = loadFromStream( file, path );
	mFilePath = path;
	resetSyntax();
	return ret;
}

bool TextDocument::loadFromMemory( const Uint8* data, const Uint32& size ) {
	IOStreamMemory stream( (const char*)data, size );
	return loadFromStream( stream, mFilePath );
}

bool TextDocument::loadFromPack( Pack* pack, std::string filePackPath ) {
	if ( NULL == pack )
		return false;
	bool ret = false;
	ScopedBuffer buffer;
	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, buffer ) ) {
		ret = loadFromMemory( buffer.get(), buffer.length() );
	}
	return ret;
}

bool TextDocument::save( const std::string& path ) {
	if ( path.empty() || mDefaultFileName == path )
		return false;
	if ( FileSystem::fileCanWrite( FileSystem::fileRemoveFileName( path ) ) ) {
		IOStreamFile file( path, "wb" );
		mFilePath = path;
		if ( save( file ) ) {
			file.close();
			notifyDocumentSaved();
			return true;
		} else {
			mFilePath.clear();
		}
	}
	return false;
}

bool TextDocument::save( IOStream& stream ) {
	if ( !stream.isOpen() || mLines.empty() )
		return false;
	const std::string whitespaces( " \t\f\v\n\r" );
	char nl = '\n';
	if ( mIsBOM ) {
		unsigned char bom[] = {0xEF, 0xBB, 0xBF};
		stream.write( (char*)bom, sizeof( bom ) );
	}
	size_t lastLine = mLines.size() - 1;
	for ( size_t i = 0; i <= lastLine; i++ ) {
		std::string text( mLines[i].toUtf8() );
		if ( mTrimTrailingWhitespaces && text.size() > 1 &&
			 whitespaces.find( text[text.size() - 2] ) != std::string::npos ) {
			size_t pos = text.find_last_not_of( whitespaces );
			if ( pos != std::string::npos ) {
				text.erase( pos + 1 );
				text += nl;
			} else {
				text = nl;
			}
			mLines[i].setText( text );
			notifyLineChanged( i );
			notifyTextChanged();
		}
		if ( i == lastLine ) {
			if ( !text.empty() && text[text.size() - 1] == '\n' ) {
				// Last \n is added by the document but it's not part of the document.
				text.pop_back();
				if ( text.empty() )
					continue;
			}
			if ( mForceNewLineAtEndOfFile && !text.empty() && text[text.size() - 1] != '\n' ) {
				text += "\n";
				mLines.emplace_back( TextDocumentLine( "\n" ) );
				notifyTextChanged();
				notifyLineChanged( i );
				notifyLineCountChanged( lastLine, lastLine + 1 );
			}
		}
		if ( mLineEnding == LineEnding::CRLF ) {
			text[text.size() - 1] = '\r';
			stream.write( text.c_str(), text.size() );
			stream.write( &nl, 1 );
		} else {
			stream.write( text.c_str(), text.size() );
		}
	}
	cleanChangeId();
	return true;
}

bool TextDocument::save() {
	return save( mFilePath );
}

std::string TextDocument::getFilename() const {
	return FileSystem::fileNameFromPath( mFilePath );
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
	TextRange nrange = sanitizeRange( range.normalized() );
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

TextPosition TextDocument::insert( TextPosition position, const String& text,
								   UndoStackContainer& undoStack, const Time& time ) {
	if ( text.empty() )
		return position;

	position = sanitizePosition( position );
	size_t lineCount = mLines.size();

	String before = mLines[position.line()].substr( 0, position.column() );
	String after = mLines[position.line()].substr( position.column() );
	std::vector<String> lines = text.split( '\n', true );
	Int64 linesAdd = eemax<Int64>( 0, static_cast<Int64>( lines.size() ) - 1 );
	for ( auto i = 0; i < linesAdd; i++ )
		lines[i] = lines[i] + "\n";
	lines[0] = before + lines[0];
	lines[lines.size() - 1] = lines[lines.size() - 1] + after;

	mLines[position.line()] = TextDocumentLine( lines[0] );
	notifyLineChanged( position.line() );

	for ( Int64 i = 1; i < (Int64)lines.size(); i++ ) {
		mLines.insert( mLines.begin() + position.line() + i, TextDocumentLine( lines[i] ) );
		notifyLineChanged( position.line() + i );
	}

	TextPosition cursor = positionOffset( position, text.size() );

	mUndoStack.pushSelection( undoStack, getSelection(), time );
	mUndoStack.pushRemove( undoStack, {position, cursor}, time );

	notifyTextChanged();

	if ( lineCount != mLines.size() ) {
		notifyLineCountChanged( lineCount, mLines.size() );
	}

	return cursor;
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
	if ( range.start().line() + 1 < range.end().line() ) {
		mLines.erase( mLines.begin() + range.start().line() + 1,
					  mLines.begin() + range.end().line() );
		range.end().setLine( range.start().line() + 1 );
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
		position.setColumn( position.column() - mLines[position.line()].size() );
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

TextRange TextDocument::getDocRange() const {
	return {startOfDoc(), endOfDoc()};
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

void TextDocument::registerClient( Client* client ) {
	mClients.insert( client );
	if ( mActiveClient == nullptr )
		setActiveClient( client );
}

void TextDocument::unregisterClient( Client* client ) {
	mClients.erase( client );
	if ( mActiveClient == client )
		setActiveClient( nullptr );
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
void TextDocument::selectLine() {
	if ( getSelection().start().line() + 1 < (Int64)linesCount() ) {
		setSelection(
			{{getSelection().start().line() + 1, 0}, {getSelection().start().line(), 0}} );
	} else {
		setSelection(
			{{getSelection().start().line(), (Int64)line( getSelection().start().line() ).size()},
			 {getSelection().start().line(), 0}} );
	}
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
	setSelection( endOfDoc(), startOfDoc() );
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
	Int64 startRemoved = 0;
	Int64 endRemoved = 0;
	for ( auto i = range.start().line(); i <= range.end().line(); i++ ) {
		const String& line = this->line( i ).getText();
		if ( !skipEmpty || line.length() != 1 ) {
			if ( line.substr( 0, text.length() ) == text ) {
				remove( {{i, 0}, {i, static_cast<Int64>( text.length() )}} );
				if ( i == range.start().line() ) {
					startRemoved = text.size();
				} else if ( i == range.end().line() ) {
					endRemoved = text.size();
				}
			}
		}
	}
	setSelection( TextPosition( range.start().line(), range.start().column() - startRemoved ),
				  TextPosition( range.end().line(), range.end().column() - endRemoved ), swap );
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
	if ( IndentType::IndentSpaces == mIndentType ) {
		return String( std::string( mIndentWidth, ' ' ) );
	}
	return String( "\t" );
}

const Uint32& TextDocument::getIndentWidth() const {
	return mIndentWidth;
}

void TextDocument::setIndentWidth( const Uint32& tabWidth ) {
	mIndentWidth = tabWidth;
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

TextRange TextDocument::sanitizeRange( const TextRange& range ) const {
	return {sanitizePosition( range.start() ), sanitizePosition( range.end() )};
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
	notifyUndoRedo( UndoRedo::Undo );
}

void TextDocument::redo() {
	mUndoStack.redo();
	notifyUndoRedo( UndoRedo::Redo );
}

const SyntaxDefinition& TextDocument::getSyntaxDefinition() const {
	return mSyntaxDefinition;
}

void TextDocument::setSyntaxDefinition( const SyntaxDefinition& definition ) {
	mSyntaxDefinition = definition;
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

TextPosition TextDocument::find( String text, TextPosition from, const bool& caseSensitive,
								 TextRange restrictRange ) {
	if ( text.empty() )
		return TextPosition();
	from = sanitizePosition( from );

	TextPosition to = endOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.end();
		if ( from < restrictRange.start() || from > restrictRange.end() )
			return TextPosition();
	}

	if ( !caseSensitive )
		text.toLower();

	for ( Int64 i = from.line(); i <= to.line(); i++ ) {
		size_t col;
		if ( i == from.line() ) {
			col = caseSensitive
					  ? line( i ).getText().substr( from.column() ).find( text )
					  : String::toLower( line( i ).getText() ).substr( from.column() ).find( text );
			if ( String::StringType::npos != col )
				col += from.column();
		} else if ( i == to.line() && to != endOfDoc() ) {
			col =
				caseSensitive
					? line( i ).getText().substr( 0, to.column() ).find( text )
					: String::toLower( line( i ).getText() ).substr( 0, to.column() ).find( text );
		} else {
			col = caseSensitive ? line( i ).getText().find( text )
								: String::toLower( line( i ).getText() ).find( text );
		}
		if ( String::StringType::npos != col ) {
			return {(Int64)i, (Int64)col};
		}
	}
	return TextPosition();
}

TextPosition TextDocument::findLast( String text, TextPosition from, const bool& caseSensitive,
									 TextRange restrictRange ) {
	if ( text.empty() )
		return TextPosition();
	from = sanitizePosition( from );

	TextPosition to = startOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.start();
		if ( from < restrictRange.start() || from > restrictRange.end() )
			return TextPosition();
	}

	if ( !caseSensitive )
		text.toLower();
	for ( Int64 i = from.line(); i >= to.line(); i-- ) {
		size_t col;
		if ( i == from.line() ) {
			col = caseSensitive ? line( i ).getText().substr( 0, from.column() ).rfind( text )
								: String::toLower( line( i ).getText() )
									  .substr( 0, from.column() )
									  .rfind( text );
		} else if ( i == to.line() ) {
			col = caseSensitive
					  ? line( i ).getText().substr( to.column() ).rfind( text )
					  : String::toLower( line( i ).getText() ).substr( to.column() ).rfind( text );
			if ( String::StringType::npos != col )
				col += to.column();
		} else {
			col = caseSensitive ? line( i ).getText().rfind( text )
								: String::toLower( line( i ).getText() ).rfind( text );
		}
		if ( String::StringType::npos != col ) {
			return {(Int64)i, (Int64)col};
		}
	}
	return TextPosition();
}

TextPosition TextDocument::replaceSelection( const String& replace ) {
	if ( hasSelection() ) {
		deleteTo( 0 );
		textInput( replace );
	}
	return getSelection( true ).end();
}

TextPosition TextDocument::replace( String search, const String& replace, TextPosition from,
									const bool& caseSensitive, TextRange restrictRange ) {
	TextPosition start( find( search, from, caseSensitive, restrictRange ) );
	if ( start.isValid() ) {
		TextPosition end = positionOffset( start, search.size() );
		if ( end.isValid() ) {
			setSelection( {start, end} );
			deleteTo( 0 );
			textInput( replace );
			return end;
		}
	}
	return TextPosition();
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

TextPosition TextDocument::findOpenBracket( TextPosition startPosition,
											const String::StringBaseType& openBracket,
											const String::StringBaseType& closeBracket ) const {
	int count = 0;
	Int64 startColumn;
	Int64 lineLength;
	for ( Int64 line = startPosition.line(); line >= 0; line-- ) {
		const String& string = mLines[line].getText();
		lineLength = string.size();
		startColumn = ( line == startPosition.line() ) ? startPosition.column() : lineLength - 1;
		for ( Int64 i = startColumn; i >= 0; i-- ) {
			if ( string[i] == closeBracket ) {
				count++;
			} else if ( string[i] == openBracket ) {
				count--;
				if ( 0 == count ) {
					return {line, i};
				}
			}
		}
	}
	return TextPosition();
}

TextPosition TextDocument::findCloseBracket( TextPosition startPosition,
											 const String::StringBaseType& openBracket,
											 const String::StringBaseType& closeBracket ) const {
	int count = 0;
	Int64 linesCount = mLines.size();
	Int64 startColumn;
	Int64 lineLength;
	for ( Int64 line = startPosition.line(); line < linesCount; line++ ) {
		const String& string = mLines[line].getText();
		startColumn = ( line == startPosition.line() ) ? startPosition.column() : 0;
		lineLength = string.size();
		for ( Int64 i = startColumn; i < lineLength; i++ ) {
			if ( string[i] == openBracket ) {
				count++;
			} else if ( string[i] == closeBracket ) {
				count--;
				if ( 0 == count ) {
					return {line, i};
				}
			}
		}
	}
	return TextPosition();
}

const String& TextDocument::getNonWordChars() const {
	return mNonWordChars;
}

void TextDocument::toggleLineComments() {
	std::string comment = mSyntaxDefinition.getComment();
	if ( comment.empty() )
		return;
	std::string commentText = comment + " ";
	TextRange selection = getSelection( true );
	bool uncomment = true;
	for ( Int64 i = selection.start().line(); i < selection.end().line(); i++ ) {
		const String& text = mLines[i].getText();
		if ( text.find_first_not_of( " \t\n" ) != std::string::npos &&
			 text.find( commentText ) == std::string::npos ) {
			uncomment = false;
		}
	}
	if ( uncomment ) {
		removeFromStartOfSelectedLines( commentText, true );
	} else {
		insertAtStartOfSelectedLines( commentText, true );
	}
}

void TextDocument::setNonWordChars( const String& nonWordChars ) {
	mNonWordChars = nonWordChars;
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

void TextDocument::notifyDocumentSaved() {
	for ( auto& client : mClients ) {
		client->onDocumentSaved();
	}
}

void TextDocument::notifyDocumentClosed() {
	for ( auto& client : mClients ) {
		client->onDocumentClosed( this );
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

void TextDocument::notifyUndoRedo( const TextDocument::UndoRedo& eventType ) {
	for ( auto& client : mClients ) {
		client->onDocumentUndoRedo( eventType );
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
	mCommands["select-line"] = [&] { selectLine(); };
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
	mCommands["toggle-line-comments"] = [&] { toggleLineComments(); };
}

TextDocument::Client::~Client() {}

}}} // namespace EE::UI::Doc
