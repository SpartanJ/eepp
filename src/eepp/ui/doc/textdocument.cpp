#include <algorithm>
#include <cstdio>
#include <eepp/core/debug.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <sstream>
#include <string>

using namespace EE::Network;

namespace EE { namespace UI { namespace Doc {

// Text document is loosely based on the SerenityOS (https://github.com/SerenityOS/serenity)
// TextDocument and the lite editor (https://github.com/rxi/lite) implementations.

const char DEFAULT_NON_WORD_CHARS[] = " \t\n/\\()\"':,.;<>~!@#$%^&*|+=[]{}`?-";

bool TextDocument::isNonWord( String::StringBaseType ch ) const {
	return mNonWordChars.find_first_of( ch ) != String::InvalidPos;
}

TextDocument::TextDocument( bool verbose ) :
	mUndoStack( this ),
	mVerbose( verbose ),
	mAutoCloseBracketsPairs(
		{ { '(', ')' }, { '[', ']' }, { '{', '}' }, { '\'', '\'' }, { '"', '"' }, { '`', '`' } } ),
	mDefaultFileName( "untitled" ),
	mCleanChangeId( 0 ),
	mNonWordChars( DEFAULT_NON_WORD_CHARS ),
	mHighlighter( std::make_unique<SyntaxHighlighter>( this ) ) {
	initializeCommands();
	reset();
}

TextDocument::~TextDocument() {
	if ( mLoading ) {
		mLoading = false;
		Lock l( mLoadingMutex );
	}
	notifyDocumentClosed();
	if ( mDeleteOnClose )
		FileSystem::fileRemove( mFilePath );
}

bool TextDocument::hasFilepath() const {
	return mDefaultFileName != mFilePath;
}

bool TextDocument::isEmpty() const {
	return linesCount() == 1 && line( 0 ).size() == 1;
}

bool TextDocument::isUntitledEmpty() const {
	return isEmpty() && !hasFilepath();
}

void TextDocument::reset() {
	mMightBeBinary = false;
	mIsBOM = false;
	mDirtyOnFileSystem = false;
	mSaving = false;
	mFilePath = mDefaultFileName;
	mFileURI = URI( "file://" + mFilePath );
	mFileRealPath = FileInfo();
	mSelection.clear();
	mSelection.push_back( { { 0, 0 }, { 0, 0 } } );
	mLastSelection = 0;
	mLines.clear();
	mLines.emplace_back( String( "\n" ) );
	mSyntaxDefinition = SyntaxDefinitionManager::instance()->getPlainStyle();
	mUndoStack.clear();
	cleanChangeId();
	notifySyntaxDefinitionChange();
	notifyCursorChanged();
	notifySelectionChanged();
}

void TextDocument::resetCursor() {
	auto cursor = sanitizeRange( getSelection() );
	mSelection.clear();
	mSelection.push_back( cursor );
	mLastSelection = 0;
	notifyCursorChanged();
	notifySelectionChanged();
}

static String ptrGetLine( char* data, const size_t& size, size_t& position ) {
	position = 0;
	while ( position < size && data[position] != '\n' && data[position] != '\r' )
		position++;
	if ( position < size ) {
		if ( position + 1 < size && data[position] == '\r' && data[position + 1] == '\n' )
			position++;
		position++;
	}
	return String( data, position );
}

TextDocument::LoadStatus TextDocument::loadFromStream( IOStream& file ) {
	return loadFromStream( file, "untitled", true );
}

TextDocument::LoadStatus TextDocument::loadFromStream( IOStream& file, std::string path,
													   bool callReset ) {
	mLoading = true;
	Lock l( mLoadingMutex );
	Clock clock;
	if ( callReset )
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
		while ( pending && mLoading ) {
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

			while ( consume && mLoading ) {
				lineBuffer += ptrGetLine( bufferPtr, consume, position );
				bufferPtr += position;
				consume -= position;
				size_t lineBufferSize = lineBuffer.size();
				char lastChar = lineBuffer[lineBufferSize - 1];

				if ( lastChar == '\n' || lastChar == '\r' || !consume ) {
					if ( mLines.empty() ) {
						if ( lineBufferSize > 1 && lineBuffer[lineBufferSize - 2] == '\r' &&
							 lastChar == '\n' ) {
							mLineEnding = LineEnding::CRLF;
						} else if ( lastChar == '\r' ) {
							mLineEnding = LineEnding::CR;
						}

						mMightBeBinary = lineBuffer.find_first_of( (String::StringBaseType)'\0' ) !=
										 String::InvalidPos;
					}

					if ( mLineEnding == LineEnding::CRLF && lineBufferSize > 1 &&
						 lastChar == '\n' ) {
						lineBuffer[lineBuffer.size() - 2] = '\n';
						lineBuffer.resize( lineBufferSize - 1 );
					} else if ( mLineEnding == LineEnding::CR && lineBufferSize > 0 ) {
						lineBuffer[lineBuffer.size() - 1] = '\n';
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

	if ( mVerbose )
		Log::info( "Document \"%s\" loaded in %.2fms.", path.c_str(),
				   clock.getElapsedTime().asMilliseconds() );

	bool wasInterrupted = !mLoading;
	if ( wasInterrupted )
		reset();
	mLoading = false;
	return wasInterrupted ? LoadStatus::Interrupted
						  : ( file.isOpen() ? LoadStatus::Loaded : LoadStatus::Failed );
}

void TextDocument::guessIndentType() {
	int guessSpaces = 0;
	int guessTabs = 0;
	std::map<int, int> guessWidth;
	int guessCountdown = 10;
	size_t linesCount = eemin<size_t>( 100, mLines.size() );
	for ( size_t i = 0; i < linesCount; i++ ) {
		const String& text = mLines[i].getText();
		std::string match =
			LuaPattern::match( text.size() > 128 ? text.substr( 0, 12 ) : text, "^  +" );
		if ( !match.empty() ) {
			guessSpaces++;
			guessWidth[match.size()]++;
			guessCountdown--;
		} else {
			match = LuaPattern::match( mLines[i].getText(), "^\t+" );
			if ( !match.empty() ) {
				guessTabs++;
				guessCountdown--;
				break; // if tab found asume tabs
			}
		}
		if ( guessCountdown == 0 )
			break;
	}
	if ( !guessTabs && !guessSpaces ) {
		return;
	}
	if ( guessTabs >= guessSpaces ) {
		mIndentType = IndentType::IndentTabs;
	} else {
		mIndentType = IndentType::IndentSpaces;
		mIndentWidth = guessWidth.begin()->first;
	}

	if ( mIndentWidth == 0 )
		mIndentWidth = 4;
}

void TextDocument::mergeSelection() {
	mSelection.merge();
	if ( mLastSelection >= mSelection.size() )
		mLastSelection = mSelection.size() - 1;
}

bool TextDocument::hasSyntaxDefinition() const {
	return !mSyntaxDefinition.getPatterns().empty();
}

void TextDocument::resetSyntax() {
	String header( getText( { { 0, 0 }, positionOffset( { 0, 0 }, 128 ) } ) );
	std::string oldDef = mSyntaxDefinition.getLSPName();
	mSyntaxDefinition = SyntaxDefinitionManager::instance()->find( mFilePath, header, mHAsCpp );
	if ( mSyntaxDefinition.getLSPName() != oldDef )
		notifySyntaxDefinitionChange();
}

bool TextDocument::getAutoDetectIndentType() const {
	return mAutoDetectIndentType;
}

void TextDocument::setAutoDetectIndentType( bool autodetect ) {
	if ( !mAutoDetectIndentType && autodetect )
		guessIndentType();
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

void TextDocument::notifyDocumentMoved( const std::string& path ) {
	mFilePath = path;
	mFileURI = URI( "file://" + mFilePath );
	mFileRealPath = FileInfo::isLink( mFilePath ) ? FileInfo( FileInfo( mFilePath ).linksTo() )
												  : FileInfo( mFilePath );
	notifyDocumentMoved();
}

void TextDocument::toUpperSelection() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextRange selection( getSelectionIndex( i ) );
		String selectedText( getSelectedText( i ) );
		selectedText.toUpper();
		deleteSelection( i );
		insert( i, getSelectionIndex( i ).start(), selectedText );
		setSelection( i, selection );
	}
}

void TextDocument::toLowerSelection() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextRange selection( getSelectionIndex( i ) );
		String selectedText( getSelectedText( i ) );
		selectedText.toLower();
		deleteSelection( i );
		insert( i, getSelectionIndex( i ).start(), selectedText );
		setSelection( i, selection );
	}
}

const std::string& TextDocument::getLoadingFilePath() const {
	Lock l( mLoadingFilePathMutex );
	return mLoadingFilePath;
}

const URI& TextDocument::getLoadingFileURI() const {
	Lock l( mLoadingFilePathMutex );
	return mLoadingFileURI;
}

TextDocument::LoadStatus TextDocument::loadFromFile( const std::string& path ) {
	mLoading = true;
	if ( !FileSystem::fileExists( path ) && PackManager::instance()->isFallbackToPacksActive() ) {
		std::string pathFix( path );
		Pack* pack = PackManager::instance()->exists( pathFix );
		if ( NULL != pack ) {
			mFilePath = pathFix;
			mFileRealPath = FileInfo();
			mFileURI = URI( "file://" + mFilePath );
			return loadFromPack( pack, pathFix );
		}
	}

	IOStreamFile file( path, "rb" );
	auto ret = loadFromStream( file, path, true );
	mFilePath = path;
	mFileURI = URI( "file://" + mFilePath );
	mFileRealPath = FileInfo::isLink( mFilePath ) ? FileInfo( FileInfo( mFilePath ).linksTo() )
												  : FileInfo( mFilePath );
	resetSyntax();
	mLoading = false;
	if ( !mLoadingAsync )
		notifyDocumentLoaded();
	return ret;
}

bool TextDocument::loadAsyncFromFile( const std::string& path, std::shared_ptr<ThreadPool> pool,
									  std::function<void( TextDocument*, bool )> onLoaded ) {
	mLoading = true;
	mLoadingAsync = true;
	{
		Lock l( mLoadingFilePathMutex );
		mLoadingFilePath = path;
		mLoadingFileURI = URI( "file://" + mLoadingFilePath );
	}
	pool->run( [this, path, onLoaded] {
		auto loaded = loadFromFile( path );
		if ( loaded != LoadStatus::Interrupted && onLoaded ) {
			onLoaded( this, loaded == LoadStatus::Loaded );
		}
		{
			Lock l( mLoadingFilePathMutex );
			mLoadingFilePath.clear();
			mLoadingFileURI = URI();
		}
		mLoadingAsync = false;
		notifyDocumentLoaded();
	} );
	return true;
}

TextDocument::LoadStatus TextDocument::loadFromMemory( const Uint8* data, const Uint32& size ) {
	IOStreamMemory stream( (const char*)data, size );
	return loadFromStream( stream, mFilePath, true );
}

TextDocument::LoadStatus TextDocument::loadFromPack( Pack* pack, std::string filePackPath ) {
	if ( NULL == pack )
		return LoadStatus::Failed;
	LoadStatus ret = LoadStatus::Failed;
	ScopedBuffer buffer;
	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, buffer ) ) {
		ret = loadFromMemory( buffer.get(), buffer.length() );
	}
	return ret;
}

static std::string getTempPathFromURI( const URI& uri ) {
	std::string lastSegment( uri.getLastPathSegment() );
	std::string name( String::randString( 8 ) +
					  ( lastSegment.empty() ? ".txt" : "." + lastSegment ) );
	std::string tmpPath( Sys::getTempPath() + name );
	return tmpPath;
}

TextDocument::LoadStatus TextDocument::loadFromURL( const std::string& url,
													const Http::Request::FieldTable& headers ) {
	URI uri( url );

	if ( uri.getScheme().empty() )
		return LoadStatus::Failed;

	mLoading = true;

	Http::Response response =
		Http::get( uri, Seconds( 10 ), nullptr, headers, "", true, Http::getEnvProxyURI() );

	if ( response.getStatus() <= Http::Response::Ok ) {
		std::string path( getTempPathFromURI( uri ) );
		FileSystem::fileWrite( path, (const Uint8*)response.getBody().c_str(),
							   response.getBody().size() );
		auto ret = loadFromFile( path );
		setDeleteOnClose( true );
		return ret;
	}

	mLoading = false;
	notifyDocumentLoaded();
	return LoadStatus::Failed;
}

bool TextDocument::loadAsyncFromURL( const std::string& url,
									 const Http::Request::FieldTable& headers,
									 std::function<void( TextDocument*, bool success )> onLoaded,
									 const Http::Request::ProgressCallback& progressCallback ) {
	URI uri( url );

	if ( uri.getScheme().empty() || ( uri.getScheme() != "https" && uri.getScheme() != "http" ) )
		return false;

	mLoading = true;
	mLoadingAsync = true;

	Http::getAsync(
		[=]( const Http&, Http::Request&, Http::Response& response ) {
			if ( response.getStatus() <= Http::Response::Ok ) {
				std::string path( getTempPathFromURI( uri ) );
				FileSystem::fileWrite( path, (const Uint8*)response.getBody().c_str(),
									   response.getBody().size() );
				if ( loadFromFile( path ) == LoadStatus::Loaded ) {
					setDeleteOnClose( true );
					if ( onLoaded )
						onLoaded( this, true );
				}
			} else {
				onLoaded( this, false );
			}
			mLoading = false;
			mLoadingAsync = false;
			notifyDocumentLoaded();
		},
		uri, Seconds( 10 ), progressCallback, headers, "", true, Http::getEnvProxyURI() );
	return true;
}

TextDocument::LoadStatus TextDocument::reload() {
	TextDocument::LoadStatus ret = LoadStatus::Failed;
	std::string path( mFilePath );
	if ( mFileRealPath.exists() ) {
		auto selection = mSelection;
		mUndoStack.clear();
		cleanChangeId();
		IOStreamFile file( path, "rb" );
		ret = loadFromStream( file, path, false );
		mFileRealPath = FileInfo::isLink( mFilePath ) ? FileInfo( FileInfo( mFilePath ).linksTo() )
													  : FileInfo( mFilePath );
		resetSyntax();
		notifyDocumentReloaded();
		setSelection( sanitizeRange( selection ) );
	}
	return ret;
}

bool TextDocument::save( const std::string& path ) {
	if ( path.empty() || mDefaultFileName == path )
		return false;
	if ( FileSystem::fileCanWrite( FileSystem::fileRemoveFileName( path ) ) ) {
		IOStreamFile file( path, "wb" );
		std::string oldFilePath( mFilePath );
		URI oldFileURI( mFileURI );
		FileInfo oldFileInfo( mFileRealPath );
		mFilePath = path;
		mFileURI = URI( "file://" + mFilePath );
		mSaving = true;
		if ( save( file ) ) {
			file.close();
			mFileRealPath =
				FileInfo::isLink( mFilePath ) ? FileInfo( mFilePath ).linksTo() : mFilePath;
			mSaving = false;
			notifyDocumentSaved();
			return true;
		} else {
			mFilePath = std::move( oldFilePath );
			mFileURI = std::move( oldFileURI );
			mFileRealPath = std::move( oldFileInfo );
			mSaving = false;
		}
	}
	return false;
}

bool TextDocument::save( IOStream& stream, bool keepUndoRedoStatus ) {
	if ( !stream.isOpen() || mLines.empty() )
		return false;
	const std::string whitespaces( " \t\f\v\n\r" );
	if ( mIsBOM ) {
		unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
		stream.write( (char*)bom, sizeof( bom ) );
	}
	size_t lastLine = mLines.size() - 1;
	for ( size_t i = 0; i <= lastLine; i++ ) {
		std::string text( mLines[i].toUtf8() );
		if ( !keepUndoRedoStatus && mTrimTrailingWhitespaces && text.size() > 1 &&
			 whitespaces.find( text[text.size() - 2] ) != std::string::npos ) {
			size_t pos = text.find_last_not_of( whitespaces );
			Int64 curLine = i;
			if ( pos != std::string::npos ) {
				remove( 0, { { curLine, static_cast<Int64>( pos + 1 ) },
							 { curLine, static_cast<Int64>( mLines[i].getText().size() ) } } );
			} else {
				remove( 0, { startOfLine( { curLine, 0 } ), { endOfLine( { curLine, 0 } ) } } );
			}
			text = mLines[i].toUtf8();
		}
		if ( i == lastLine ) {
			if ( !text.empty() && text[text.size() - 1] == '\n' ) {
				// Last \n is added by the document but it's not part of the document.
				text.pop_back();
				if ( text.empty() )
					continue;
			}
			if ( !keepUndoRedoStatus && mForceNewLineAtEndOfFile && !text.empty() &&
				 text[text.size() - 1] != '\n' ) {
				text += "\n";
				insert( 0, endOfDoc(), "\n" );
			}
		}
		if ( mLineEnding == LineEnding::CRLF ) {
			if ( text[text.size() - 1] == '\n' ) {
				text[text.size() - 1] = '\r';
				text += "\n";
			}
			stream.write( text.c_str(), text.size() );
		} else if ( mLineEnding == LineEnding::CR ) {
			text[text.size() - 1] = '\r';
			stream.write( text.c_str(), text.size() );
		} else {
			stream.write( text.c_str(), text.size() );
		}
	}

	sanitizeCurrentSelection();

	if ( !keepUndoRedoStatus )
		cleanChangeId();

	return true;
}

bool TextDocument::save() {
	return save( mFilePath );
}

void TextDocument::sanitizeCurrentSelection() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		auto& selection = mSelection[i];
		auto newSelection = sanitizeRange( selection );
		if ( selection != newSelection )
			setSelection( i, newSelection );
	}
}

bool TextDocument::isLoading() const {
	return mLoading;
}

bool TextDocument::isDeleteOnClose() const {
	return mDeleteOnClose;
}

void TextDocument::setDeleteOnClose( bool deleteOnClose ) {
	mDeleteOnClose = deleteOnClose;
}

std::string TextDocument::getFilename() const {
	return FileSystem::fileNameFromPath( mFilePath );
}

void TextDocument::setSelection( const TextRanges& selection ) {
	if ( selection.empty() ) {
		mSelection.clear();
		mSelection.push_back( { { 0, 0 }, { 0, 0 } } );
		mLastSelection = 0;
		return;
	}

	auto prevSelection = mSelection;

	for ( size_t i = 0; i < selection.size(); ++i ) {
		if ( i >= mSelection.size() )
			mSelection.push_back( selection[i] );
		setSelection( i, selection[i] );
	}

	mSelection.sort();
	mLastSelection = selection.size() - 1;
}

void TextDocument::resetSelection( const TextRanges& selection ) {
	if ( mSelection != selection ) {
		mSelection = selection;
		mSelection.sort();
		notifySelectionChanged();
		notifyCursorChanged();
	}
}

void TextDocument::setSelection( const TextPosition& position ) {
	setSelection( position, position );
}

void TextDocument::setSelection( const size_t& cursorIdx, const TextPosition& position ) {
	setSelection( cursorIdx, position, position );
}

void TextDocument::setSelection( const size_t& cursorIdx, TextPosition start, TextPosition end,
								 bool swap ) {
	eeASSERT( cursorIdx < mSelection.size() );
	if ( cursorIdx >= mSelection.size() )
		return;

	if ( ( start == mSelection[cursorIdx].start() && end == mSelection[cursorIdx].end() &&
		   !swap ) ||
		 ( start == mSelection[cursorIdx].end() && end == mSelection[cursorIdx].start() && swap ) )
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

	if ( mSelection[cursorIdx] != TextRange( start, end ) ) {
		mSelection[cursorIdx].set( start, end );
		notifyCursorChanged( mSelection[cursorIdx].start() );
		notifySelectionChanged( mSelection[cursorIdx] );
	}
}

void TextDocument::setSelection( TextPosition start, TextPosition end, bool swap ) {
	setSelection( 0, start, end, swap );
}

void TextDocument::setSelection( const TextRange& range ) {
	setSelection( range.start(), range.end() );
}

void TextDocument::setSelection( const size_t& cursorIdx, const TextRange& range ) {
	setSelection( cursorIdx, range.start(), range.end() );
}

TextRange TextDocument::addSelection( const TextPosition& selection ) {
	return addSelection( { selection, selection } );
}

TextRange TextDocument::addSelection( TextRange selection ) {
	if ( mSelection.exists( selection ) )
		return {};
	selection = sanitizeRange( selection );
	if ( mSelection.exists( selection ) )
		return {};
	mSelection.push_back( selection );
	mSelection.sort();
	mergeSelection();
	notifyCursorChanged( selection.start() );
	notifySelectionChanged( selection );
	mLastSelection = mSelection.findIndex( selection );
	return selection;
}

void TextDocument::popSelection() {
	mSelection.pop_back();
	if ( mLastSelection >= mSelection.size() )
		mLastSelection = mSelection.size() - 1;
	notifyCursorChanged();
	notifySelectionChanged();
}

TextRange TextDocument::getSelection( bool sort ) const {
	if ( mLastSelection >= 0 && mLastSelection < mSelection.size() )
		return sort ? mSelection[mLastSelection].normalized() : mSelection[mLastSelection];
	return sort ? mSelection.front().normalized() : mSelection.front();
}

const TextRanges& TextDocument::getSelections() const {
	return mSelection;
}

std::vector<TextRange> TextDocument::getSelectionsSorted() const {
	auto selections( mSelection );
	for ( auto& selection : selections )
		selection.normalize();
	return selections;
}

const TextRange& TextDocument::getSelectionIndex( const size_t& index ) const {
	eeASSERT( index < mSelection.size() );
	return mSelection[index];
}

const TextRange& TextDocument::getSelection() const {
	if ( mLastSelection >= 0 && mLastSelection < mSelection.size() )
		return mSelection[mLastSelection];
	return mSelection.front();
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

const TextDocumentLine& TextDocument::getCurrentLine() const {
	return mLines[getSelection().start().line()];
}

std::vector<TextDocumentLine>& TextDocument::lines() {
	return mLines;
}

bool TextDocument::hasSelection() const {
	return mSelection.hasSelection();
}

String TextDocument::getText( const TextRange& range ) const {
	TextRange nrange = sanitizeRange( range.normalized() );
	if ( nrange.start().line() == nrange.end().line() ) {
		return mLines[nrange.start().line()].substr(
			nrange.start().column(), nrange.end().column() - nrange.start().column() );
	}
	std::vector<String> lines = { mLines[nrange.start().line()].substr( nrange.start().column() ) };
	for ( auto i = nrange.start().line() + 1; i <= nrange.end().line() - 1; i++ ) {
		lines.emplace_back( mLines[i].getText() );
	}
	lines.emplace_back( mLines[nrange.end().line()].substr( 0, nrange.end().column() ) );
	return String::join( lines, -1 );
}

String TextDocument::getText() const {
	return getText( getDocRange() );
}

String TextDocument::getAllSelectedText() const {
	String text;
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		String sel( getSelectedText( i ) );
		if ( !sel.empty() ) {
			text += std::move( sel );
			if ( i != mSelection.size() - 1 )
				text += "\n";
		}
	}
	return text;
}

std::vector<std::string> TextDocument::getCommandList() const {
	std::vector<std::string> cmds;
	for ( const auto& cmd : mCommands )
		cmds.push_back( cmd.first );
	return cmds;
}

bool TextDocument::isRunningTransaction() const {
	return mRunningTransaction;
}

void TextDocument::setRunningTransaction( const bool runningTransaction ) {
	mRunningTransaction = runningTransaction;
}

String TextDocument::getSelectedText() const {
	return getText( getSelection() );
}

String TextDocument::getSelectedText( const size_t& cursorIdx ) const {
	return getText( getSelectionIndex( cursorIdx ) );
}

size_t TextDocument::getLastSelection() const {
	return mLastSelection;
}

bool TextDocument::selectionExists( const TextRange& selection ) {
	return mSelection.exists( selection );
}

bool TextDocument::selectionExists( const TextPosition& selection ) {
	return mSelection.exists( { selection, selection } );
}

String::StringBaseType TextDocument::getPrevChar() const {
	return getChar( positionOffset( getSelection().start(), -1 ) );
}

String::StringBaseType TextDocument::getCurrentChar() const {
	return getChar( getSelection().start() );
}

String::StringBaseType TextDocument::getChar( const TextPosition& position ) const {
	auto pos = sanitizePosition( position );
	return mLines[pos.line()][pos.column()];
}

TextPosition TextDocument::insert( const size_t& cursorIdx, const TextPosition& position,
								   const String& text ) {
	mUndoStack.clearRedoStack();
	return insert( cursorIdx, position, text, mUndoStack.getUndoStackContainer(),
				   mTimer.getElapsedTime() );
}

TextPosition TextDocument::insert( const size_t& cursorIdx, TextPosition position,
								   const String& text, UndoStackContainer& undoStack,
								   const Time& time, bool fromUndoRedo ) {
	if ( text.empty() )
		return position;

	mModificationId++;

	if ( fromUndoRedo ) {
		if ( cursorIdx >= mSelection.size() ) {
			while ( cursorIdx >= mSelection.size() )
				mSelection.push_back( { position, position } );
			notifyCursorChanged();
			notifySelectionChanged();
		}
	} else {
		eeASSERT( cursorIdx < mSelection.size() );
	}

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

	mUndoStack.pushSelection( undoStack, cursorIdx, mSelection, time );
	mUndoStack.pushRemove( undoStack, cursorIdx, { position, cursor }, time );

	if ( linesAdd > 0 ) {
		mHighlighter->moveHighlight( position.line(), linesAdd );
		notifiyDocumenLineMove( position.line(), linesAdd );
	}

	notifyTextChanged( { { position, position }, text } );

	if ( lineCount != mLines.size() ) {
		notifyLineCountChanged( lineCount, mLines.size() );
	}

	if ( mSelection.size() > 1 ) {
		for ( auto& sel : mSelection ) {
			auto selNorm( sel.normalized() );
			if ( selNorm.start().line() < position.line() )
				continue;
			Int64 addLines = position.line() < selNorm.start().line() ||
									 position.column() < selNorm.start().column()
								 ? linesAdd
								 : 0;
			sel.start().setLine( sel.start().line() + addLines );
			sel.end().setLine( sel.end().line() + addLines );
			if ( selNorm.start().line() == position.line() &&
				 selNorm.start().column() >= position.column() ) {
				sel.start().setColumn( positionOffset( sel.start(), text.size() ).column() );
				sel.end().setColumn( positionOffset( sel.end(), text.size() ).column() );
			}
			sel = sanitizeRange( sel );
		}
	}

	return cursor;
}

size_t TextDocument::remove( const size_t& cursorIdx, TextRange range ) {
	size_t lineCount = mLines.size();
	mUndoStack.clearRedoStack();
	size_t linesRemoved = remove( cursorIdx, sanitizeRange( range.normalized() ),
								  mUndoStack.getUndoStackContainer(), mTimer.getElapsedTime() );
	if ( lineCount != mLines.size() ) {
		notifyLineCountChanged( lineCount, mLines.size() );
	}
	return linesRemoved;
}

size_t TextDocument::remove( const size_t& cursorIdx, TextRange range,
							 UndoStackContainer& undoStack, const Time& time, bool fromUndoRedo ) {
	if ( !range.isValid() )
		return 0;

	mModificationId++;

	if ( fromUndoRedo ) {
		if ( cursorIdx >= mSelection.size() ) {
			while ( cursorIdx >= mSelection.size() )
				mSelection.push_back( range );
			mSelection.sort();
			notifyCursorChanged();
			notifySelectionChanged();
		}
	} else {
		eeASSERT( cursorIdx < mSelection.size() );
	}

	TextRange originalRange = range;
	mUndoStack.pushSelection( undoStack, cursorIdx, mSelection, time );
	mUndoStack.pushInsert( undoStack, getText( range ), cursorIdx, range.start(), time );

	size_t linesRemoved = 0;

	// First delete all the lines in between the first and last one.
	if ( range.start().line() + 1 < range.end().line() ) {
		mLines.erase( mLines.begin() + range.start().line() + 1,
					  mLines.begin() + range.end().line() );
		linesRemoved = range.end().line() - ( range.start().line() + 1 );
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
				!line.empty() && range.end().column() < (Int64)line.size()
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
		auto afterSelection = !secondLine.empty() && range.end().column() < (Int64)secondLine.size()
								  ? secondLine.substr( range.end().column(),
													   secondLine.length() - range.end().column() )
								  : "";

		if ( !beforeSelection.empty() && beforeSelection[beforeSelection.size() - 1] == '\n' )
			beforeSelection = beforeSelection.substr( 0, beforeSelection.size() - 1 );
		if ( afterSelection.empty() || afterSelection[afterSelection.size() - 1] != '\n' )
			afterSelection += '\n';

		firstLine.setText( beforeSelection + afterSelection );
		mLines.erase( mLines.begin() + range.end().line() );
		linesRemoved += 1;
	}

	if ( lines().empty() )
		mLines.emplace_back( String( "\n" ) );

	if ( mSelection.size() > 1 ) {
		for ( auto& sel : mSelection ) {
			auto selNorm( sel.normalized() );
			auto ranNorm( originalRange.normalized() );

			if ( selNorm.start().line() < ranNorm.end().line() )
				continue;
			Int64 lineRem = ranNorm.end().line() - ranNorm.start().line();
			Int64 colRem = 0;
			if ( selNorm.end().line() == ranNorm.end().line() &&
				 ranNorm.end().column() < selNorm.start().column() ) {
				colRem = ranNorm.start().line() == ranNorm.end().line()
							 ? ranNorm.end().column() - ranNorm.start().column()
							 : ranNorm.end().column();
			}
			sel.start().setLine( sel.start().line() - lineRem );
			sel.start().setColumn( sel.start().column() - colRem );
			sel.end().setLine( sel.end().line() - lineRem );
			sel.end().setColumn( sel.end().column() - colRem );
			sel = sanitizeRange( sel );
		}
	}

	if ( linesRemoved > 0 ) {
		mHighlighter->moveHighlight( range.start().line(), -linesRemoved );
		notifiyDocumenLineMove( range.start().line(), -linesRemoved );
	}

	notifyTextChanged( { originalRange, "" } );
	notifyLineChanged( range.start().line() );

	return linesRemoved;
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

bool TextDocument::replaceLine( const Int64& lineNum, const String& text ) {
	if ( lineNum >= 0 && lineNum < (Int64)mLines.size() ) {
		TextRange oldSelection = getSelection();
		setSelection( { startOfLine( { lineNum, 0 } ), endOfLine( { lineNum, 0 } ) } );
		textInput( text );
		setSelection( oldSelection );
		return true;
	}
	return false;
}

bool TextDocument::replaceCurrentLine( const String& text ) {
	return replaceLine( getSelection().start().line(), text );
}

TextPosition TextDocument::nextChar( TextPosition position ) const {
	return positionOffset( position, TextPosition( 0, 1 ) );
}

TextPosition TextDocument::previousChar( TextPosition position ) const {
	return positionOffset( position, TextPosition( 0, -1 ) );
}

TextPosition TextDocument::previousWordBoundary( TextPosition position,
												 bool ignoreFirstNonWord ) const {
	auto ch = getChar( positionOffset( position, -1 ) );
	bool inWord = !isNonWord( ch );
	if ( !ignoreFirstNonWord && !inWord )
		return position;
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

TextPosition TextDocument::nextWordBoundary( TextPosition position,
											 bool ignoreFirstNonWord ) const {
	auto ch = getChar( position );
	bool inWord = !isNonWord( ch );
	if ( !ignoreFirstNonWord && !inWord )
		return position;
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

TextPosition TextDocument::previousSpaceBoundaryInLine( TextPosition position ) const {
	auto ch = getChar( positionOffset( position, -1 ) );
	bool inWord = ch != ' ';
	String::StringBaseType nextChar = 0;
	do {
		TextPosition curPos = position;
		position = positionOffset( position, -1 );
		if ( curPos == position )
			break;
		if ( curPos.line() != position.line() ) {
			position = curPos;
			break;
		}
		nextChar = getChar( positionOffset( position, -1 ) );
	} while ( ( inWord && nextChar != ' ' ) || ( !inWord && nextChar == ch ) );
	return position;
}

TextPosition TextDocument::nextSpaceBoundaryInLine( TextPosition position ) const {
	auto ch = getChar( position );
	bool inWord = ch != ' ';
	String::StringBaseType nextChar = 0;
	do {
		TextPosition curPos = position;
		position = positionOffset( position, 1 );
		if ( curPos == position )
			break;
		if ( curPos.line() != position.line() ) {
			position = curPos;
			break;
		}
		nextChar = getChar( position );
	} while ( ( inWord && nextChar != ' ' ) || ( !inWord && nextChar == ch ) );
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
	return { start.line(), indent };
}

TextPosition TextDocument::startOfDoc() const {
	return TextPosition( 0, 0 );
}

TextPosition TextDocument::endOfDoc() const {
	return TextPosition( mLines.size() - 1, mLines[mLines.size() - 1].size() - 1 );
}

TextRange TextDocument::getDocRange() const {
	return { startOfDoc(), endOfDoc() };
}

void TextDocument::deleteTo( const size_t& cursorIdx, int offset ) {
	eeASSERT( cursorIdx < mSelection.size() );
	TextPosition cursorPos = mSelection[cursorIdx].normalized().start();
	if ( mSelection[cursorIdx].hasSelection() ) {
		remove( cursorIdx, getSelectionIndex( cursorIdx ) );
	} else {
		TextPosition delPos = positionOffset( cursorPos, offset );
		TextRange range( cursorPos, delPos );
		remove( cursorIdx, range );
		range = range.normalized();
		cursorPos = range.start();
	}
	setSelection( cursorIdx, cursorPos );
}

void TextDocument::deleteSelection( const size_t& cursorIdx ) {
	TextPosition cursorPos = getSelectionIndex( cursorIdx ).normalized().start();
	remove( cursorIdx, getSelectionIndex( cursorIdx ) );
	setSelection( cursorIdx, cursorPos );
}

void TextDocument::deleteSelection() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		deleteSelection( i );
}

void TextDocument::selectTo( TextPosition position ) {
	setSelection( TextRange( sanitizePosition( position ), getSelection().end() ) );
}

void TextDocument::selectTo( int offset ) {
	const TextRange& range = getSelection();
	TextPosition posOffset = positionOffset( range.start(), offset );
	setSelection( TextRange( posOffset, range.end() ) );
}

void TextDocument::selectTo( const size_t& cursorIdx, TextPosition position ) {
	setSelection( cursorIdx,
				  TextRange( sanitizePosition( position ), getSelectionIndex( cursorIdx ).end() ) );
}

void TextDocument::selectTo( const size_t& cursorIdx, int offset ) {
	const TextRange& range = getSelectionIndex( cursorIdx );
	TextPosition posOffset = positionOffset( range.start(), offset );
	setSelection( cursorIdx, TextRange( posOffset, range.end() ) );
}

void TextDocument::moveTo( TextPosition offset ) {
	setSelection( offset );
}

void TextDocument::moveTo( int columnOffset ) {
	setSelection( positionOffset( getSelection().start(), columnOffset ) );
}

void TextDocument::moveTo( const size_t& cursorIdx, TextPosition offset ) {
	setSelection( cursorIdx, offset );
}

void TextDocument::moveTo( const size_t& cursorIdx, int columnOffset ) {
	setSelection( cursorIdx, positionOffset( getSelection().start(), columnOffset ) );
}

std::vector<bool> TextDocument::autoCloseBrackets( const String& text ) {
	if ( !mAutoCloseBrackets || 1 != text.size() )
		return {};

	size_t pos = 0xFFFFFFFF;
	for ( size_t i = 0; i < mAutoCloseBracketsPairs.size(); i++ ) {
		if ( text[0] == mAutoCloseBracketsPairs[i].first ) {
			pos = i;
			break;
		}
	}

	if ( pos == 0xFFFFFFFF )
		return {};

	std::vector<bool> inserted;
	inserted.reserve( mSelection.size() );
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		auto& sel = mSelection[i];
		if ( sel.hasSelection() ) {
			replaceSelection( i, mAutoCloseBracketsPairs[pos].first + getSelectedText() +
									 mAutoCloseBracketsPairs[pos].second );
			inserted.push_back( true );
		} else {
			auto closeChar = mAutoCloseBracketsPairs[pos].second;
			bool mustClose = true;

			if ( sel.start().column() < (Int64)line( sel.start().line() ).size() ) {
				auto ch = line( sel.start().line() ).getText()[sel.start().column()];
				if ( ch == closeChar )
					mustClose = false;
			}

			if ( mustClose ) {
				setSelection(
					i, positionOffset( insert( i, sel.start(), text + String( closeChar ) ), -1 ) );
				inserted.push_back( true );
			} else {
				inserted.push_back( false );
			}
		}
	}

	return inserted;
}

void TextDocument::textInput( const String& text ) {
	if ( mAutoCloseBrackets && 1 == text.size() ) {
		auto inserted = autoCloseBrackets( text );

		if ( !inserted.empty() ) {
			for ( size_t i = 0; i < mSelection.size(); ++i ) {
				if ( inserted[i] || i >= inserted.size() )
					continue;

				if ( mSelection[i].hasSelection() )
					deleteTo( i, 0 );
				setSelection( i, insert( i, getSelectionIndex( i ).start(), text ) );
			}
			return;
		}
	}

	auto crPOS = text.find_first_of( '\r' );
	if ( crPOS != String::InvalidPos ) {
		String textCpy( text );
		textCpy.replaceAll( "\r", "" );

		for ( size_t i = 0; i < mSelection.size(); ++i ) {
			if ( mSelection[i].hasSelection() )
				deleteTo( i, 0 );
			setSelection( i, insert( i, getSelectionIndex( i ).start(), textCpy ) );
		}
	} else {

		for ( size_t i = 0; i < mSelection.size(); ++i ) {
			if ( mSelection[i].hasSelection() )
				deleteTo( i, 0 );
			setSelection( i, insert( i, getSelectionIndex( i ).start(), text ) );
		}
	}
}

void TextDocument::registerClient( Client* client ) {
	Lock l( mClientsMutex );
	mClients.insert( client );
	if ( mActiveClient == nullptr )
		setActiveClient( client );
}

void TextDocument::unregisterClient( Client* client ) {
	Lock l( mClientsMutex );
	mClients.erase( client );
	if ( mActiveClient == client )
		setActiveClient( nullptr );
}

void TextDocument::moveToPreviousChar() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		if ( mSelection[i].hasSelection() ) {
			setSelection( i, mSelection[i].normalize().start() );
		} else {
			setSelection( i, positionOffset( mSelection[i].start(), -1 ) );
		}
	}
	mergeSelection();
}

void TextDocument::moveToNextChar() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		if ( mSelection[i].hasSelection() ) {
			setSelection( i, mSelection[i].normalize().end() );
		} else {
			setSelection( i, positionOffset( mSelection[i].start(), 1 ) );
		}
	}
	mergeSelection();
}

void TextDocument::moveToPreviousWord() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		if ( mSelection[i].hasSelection() ) {
			setSelection( i, mSelection[i].normalize().start() );
		} else {
			setSelection( i, previousWordBoundary( mSelection[i].start() ) );
		}
	}
	mergeSelection();
}

void TextDocument::moveToNextWord() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		if ( mSelection[i].hasSelection() ) {
			setSelection( i, mSelection[i].normalize().end() );
		} else {
			setSelection( i, nextWordBoundary( mSelection[i].start() ) );
		}
	}
	mergeSelection();
}

void TextDocument::moveToPreviousLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextPosition pos = mSelection[i].start();
		pos.setLine( pos.line() - 1 );
		setSelection( i, pos, pos );
	}
	mergeSelection();
}

void TextDocument::moveToNextLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextPosition pos = mSelection[i].start();
		pos.setLine( pos.line() + 1 );
		setSelection( i, pos, pos );
	}
	mergeSelection();
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
	resetCursor();
	setSelection( startOfDoc() );
}

void TextDocument::moveToEndOfDoc() {
	resetCursor();
	setSelection( endOfDoc() );
}

void TextDocument::moveToStartOfContent() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextPosition start = getSelectionIndex( i ).start();
		TextPosition indented = startOfContent( getSelectionIndex( i ).start() );
		setSelection( i, indented.column() == start.column() ? TextPosition( start.line(), 0 )
															 : indented );
	}
	mergeSelection();
}

void TextDocument::selectToStartOfContent() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextPosition start = getSelectionIndex( i ).start();
		TextPosition indented = startOfContent( getSelectionIndex( i ).start() );
		setSelection(
			i, { indented.column() == start.column() ? TextPosition( start.line(), 0 ) : indented,
				 getSelectionIndex( i ).end() } );
	}
	mergeSelection();
}

void TextDocument::moveToStartOfLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		setSelection( i, startOfLine( getSelectionIndex( i ).start() ) );
	mergeSelection();
}

void TextDocument::moveToEndOfLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		setSelection( i, endOfLine( getSelectionIndex( i ).start() ) );
	mergeSelection();
}

void TextDocument::deleteToPreviousChar() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		deleteTo( i, -1 );
	mergeSelection();
}

void TextDocument::deleteToNextChar() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		deleteTo( i, 1 );
	mergeSelection();
}

void TextDocument::deleteToPreviousWord() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		deleteTo( i, previousWordBoundary( getSelectionIndex( i ).start() ) );
	mergeSelection();
}

void TextDocument::deleteToNextWord() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		deleteTo( i, nextWordBoundary( getSelectionIndex( i ).start() ) );
	mergeSelection();
}

void TextDocument::deleteCurrentLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		if ( mSelection[i].hasSelection() ) {
			deleteSelection( i );
			continue;
		}
		auto start = startOfLine( getSelectionIndex( i ).start() );
		auto end = positionOffset( endOfLine( getSelectionIndex( i ).start() ), 1 );
		remove( i, { start, end } );
		setSelection( i, start );
	}
}

void TextDocument::selectToPreviousChar() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		selectTo( i, -1 );
	mergeSelection();
}

void TextDocument::selectToNextChar() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		selectTo( i, 1 );
	mergeSelection();
}

void TextDocument::selectToPreviousWord() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		setSelection( i, { previousWordBoundary( getSelectionIndex( i ).start() ),
						   getSelectionIndex( i ).end() } );
	}
	mergeSelection();
}

void TextDocument::selectToNextWord() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		setSelection( i, { nextWordBoundary( getSelectionIndex( i ).start() ),
						   getSelectionIndex( i ).end() } );
	}
	mergeSelection();
}

TextRange TextDocument::getWordRangeInPosition( const TextPosition& pos ) {
	if ( mHighlighter ) {
		auto type( mHighlighter->getTokenPositionAt( pos ) );
		return { { pos.line(), type.pos }, { pos.line(), type.pos + (Int64)type.len } };
	}

	return { nextWordBoundary( pos, false ), previousWordBoundary( pos, false ) };
}

TextRange TextDocument::getWordRangeInPosition() {
	return getWordRangeInPosition( getSelection().start() );
}

String TextDocument::getWordInPosition( const TextPosition& pos ) {
	return getText( getWordRangeInPosition( pos ) );
}

String TextDocument::getWordInPosition() {
	return getWordInPosition( getSelection().start() );
}

bool TextDocument::mightBeBinary() const {
	return mMightBeBinary;
}

void TextDocument::setMightBeBinary( bool mightBeBinary ) {
	mMightBeBinary = mightBeBinary;
}

TextRange TextDocument::getActiveClientVisibleRange() const {
	if ( mActiveClient )
		return mActiveClient->getVisibleRange();
	return {};
}

bool TextDocument::hAsCpp() const {
	return mHAsCpp;
}

void TextDocument::setHAsCpp( bool hAsCpp ) {
	mHAsCpp = hAsCpp;
}

const Uint64& TextDocument::getModificationId() const {
	return mModificationId;
}

void TextDocument::selectWord( bool withMulticursor ) {
	if ( !hasSelection() ) {
		setSelection( { nextWordBoundary( getSelection().start(), false ),
						previousWordBoundary( getSelection().start(), false ) } );
	} else if ( withMulticursor ) {
		String text( getSelectedText() );
		TextRange res( find( text, getBottomMostCursor().normalized().end() ) );
		if ( res.isValid() && !mSelection.exists( res.reversed() ) ) {
			addSelection( res.reversed() );
		} else {
			res = findLast( text, getTopMostCursor().normalized().start(), true, false,
							FindReplaceType::Normal,
							{ startOfDoc(), getTopMostCursor().normalized().start() } );
			if ( res.isValid() && !mSelection.exists( res ) ) {
				addSelection( res );
			}
		}
	}
}

void TextDocument::selectLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		auto sel = getSelectionIndex( i );
		if ( sel.start().line() + 1 < (Int64)linesCount() ) {
			setSelection( i, { { sel.start().line() + 1, 0 }, { sel.start().line(), 0 } } );
		} else {
			setSelection( i, { { sel.start().line(), (Int64)line( sel.start().line() ).size() },
							   { sel.start().line(), 0 } } );
		}
	}
	mergeSelection();
}

void TextDocument::selectToPreviousLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextPosition pos = getSelectionIndex( i ).start();
		pos.setLine( pos.line() - 1 );
		setSelection( i, TextRange( pos, getSelectionIndex( i ).end() ) );
	}
	mergeSelection();
}

void TextDocument::selectToNextLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextPosition pos = getSelectionIndex( i ).start();
		pos.setLine( pos.line() + 1 );
		setSelection( i, TextRange( pos, getSelectionIndex( i ).end() ) );
	}
	mergeSelection();
}

void TextDocument::selectToStartOfLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		selectTo( i, startOfLine( getSelectionIndex( i ).start() ) );
	mergeSelection();
}

void TextDocument::selectToEndOfLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i )
		selectTo( i, endOfLine( getSelectionIndex( i ).start() ) );
	mergeSelection();
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
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		String input( "\n" );
		TextPosition start = getSelectionIndex( i ).start();
		TextPosition indent = startOfContent( getSelectionIndex( i ).start() );
		if ( indent.column() != 0 )
			input.insert( 0, line( start.line() ).getText().substr( 0, indent.column() ) );
		insert( i, { start.line(), 0 }, input );
		setSelection( i, { start.line(), (Int64)input.size() } );
	}
}

void TextDocument::insertAtStartOfSelectedLines( const String& text, bool skipEmpty ) {
	TextPosition prevStart = getSelection().start();
	TextRange range = getSelection( true );
	bool swap = prevStart != range.start();
	for ( auto i = range.start().line(); i <= range.end().line(); i++ ) {
		const String& line = this->line( i ).getText();
		if ( !skipEmpty || line.length() != 1 ) {
			insert( 0, { i, 0 }, text );
		}
	}
	setSelection( TextPosition( range.start().line(), range.start().column() + text.size() ),
				  TextPosition( range.end().line(), range.end().column() + text.size() ), swap );
}

void TextDocument::removeFromStartOfSelectedLines( const String& text, bool skipEmpty,
												   bool removeExtraSpaces ) {
	TextPosition prevStart = getSelection().start();
	TextRange range = getSelection( true );
	bool swap = prevStart != range.start();
	Int64 startRemoved = 0;
	Int64 endRemoved = 0;
	String indentSpaces( removeExtraSpaces ? std::string( mIndentWidth, ' ' ) : "" );
	for ( auto i = range.start().line(); i <= range.end().line(); i++ ) {
		const String& line = this->line( i ).getText();
		if ( !skipEmpty || line.length() > 1 ) {
			if ( line.substr( 0, text.length() ) == text ) {
				remove( 0, { { i, 0 }, { i, static_cast<Int64>( text.length() ) } } );
				if ( i == range.start().line() ) {
					startRemoved = text.size();
				} else if ( i == range.end().line() ) {
					endRemoved = text.size();
				}
			} else if ( removeExtraSpaces ) {
				if ( line.size() >= indentSpaces.size() &&
					 line.substr( 0, indentSpaces.size() ) == indentSpaces ) {
					remove( 0, { { i, 0 }, { i, static_cast<Int64>( indentSpaces.length() ) } } );
					if ( i == range.start().line() ) {
						startRemoved = indentSpaces.size();
					} else if ( i == range.end().line() ) {
						endRemoved = indentSpaces.size();
					}
				} else {
					size_t pos = line.find_first_not_of( ' ' );
					if ( pos != String::InvalidPos ) {
						remove( 0, { { i, 0 }, { i, static_cast<Int64>( pos ) } } );
						if ( i == range.start().line() ) {
							startRemoved = pos;
						} else if ( i == range.end().line() ) {
							endRemoved = pos;
						}
					}
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
	removeFromStartOfSelectedLines( getIndentString(), false, true );
}

void TextDocument::moveLinesUp() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextRange range = getSelectionIndex( i ).normalized();
		bool swap = getSelectionIndex( i ).normalized() != getSelection();
		appendLineIfLastLine( i, range.end().line() );
		if ( range.start().line() > 0 ) {
			auto& text = line( range.start().line() - 1 );
			insert( i, { range.end().line() + 1, 0 }, text.getText() );
			remove( i, { { range.start().line() - 1, 0 }, { range.start().line(), 0 } } );
			setSelection( i, { range.start().line() - 1, range.start().column() },
						  { range.end().line() - 1, range.end().column() }, swap );
		}
	}
}

void TextDocument::moveLinesDown() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		TextRange range = getSelectionIndex( i ).normalized();
		bool swap = getSelectionIndex( i ).normalized() != getSelection();
		appendLineIfLastLine( i, range.end().line() + 1 );
		if ( range.end().line() < (Int64)mLines.size() - 1 ) {
			auto text = line( range.end().line() + 1 );
			remove( i, { { range.end().line() + 1, 0 }, { range.end().line() + 2, 0 } } );
			insert( i, { range.start().line(), 0 }, text.getText() );
			setSelection( i, { range.start().line() + 1, range.start().column() },
						  { range.end().line() + 1, range.end().column() }, swap );
		}
	}
}

bool TextDocument::hasUndo() const {
	return mUndoStack.hasUndo();
}

bool TextDocument::hasRedo() const {
	return mUndoStack.hasRedo();
}

void TextDocument::appendLineIfLastLine( const size_t& cursorIdx, Int64 line ) {
	if ( line >= (Int64)mLines.size() - 1 ) {
		insert( cursorIdx, endOfDoc(), "\n" );
	}
}

String TextDocument::getIndentString() {
	if ( IndentType::IndentSpaces == mIndentType )
		return String( std::string( mIndentWidth, ' ' ) );
	return String( "\t" );
}

const Uint32& TextDocument::getIndentWidth() const {
	return mIndentWidth;
}

void TextDocument::setIndentWidth( const Uint32& tabWidth ) {
	mIndentWidth = tabWidth;
}

void TextDocument::deleteTo( const size_t& cursorIdx, TextPosition position ) {
	TextPosition cursorPos = getSelectionIndex( cursorIdx ).normalized().start();
	if ( getSelectionIndex( cursorIdx ).hasSelection() ) {
		remove( cursorIdx, getSelectionIndex( cursorIdx ) );
	} else {
		TextRange range( cursorPos, position );
		remove( cursorIdx, range );
		range = range.normalized();
		cursorPos = range.start();
	}
	setSelection( cursorIdx, cursorPos );
}

void TextDocument::print() const {
	for ( size_t i = 0; i < mLines.size(); i++ )
		printf( "%s", mLines[i].toUtf8().c_str() );
}

TextRange TextDocument::sanitizeRange( const TextRange& range ) const {
	if ( !range.isValid() )
		return range;
	return { sanitizePosition( range.start() ), sanitizePosition( range.end() ) };
}

TextRanges TextDocument::sanitizeRange( const TextRanges& ranges ) const {
	TextRanges sanitizedRanges;
	for ( const auto& range : ranges ) {
		if ( !range.isValid() )
			return sanitizedRanges;
		sanitizedRanges.push_back(
			{ sanitizePosition( range.start() ), sanitizePosition( range.end() ) } );
	}
	return sanitizedRanges;
}

bool TextDocument::isValidPosition( const TextPosition& position ) const {
	return !( position.line() < 0 || position.line() > (Int64)mLines.size() - 1 ||
			  position.column() < 0 ||
			  position.column() > (Int64)mLines[position.line()].size() - 1 );
}

bool TextDocument::isValidRange( const TextRange& range ) const {
	return isValidPosition( range.start() ) && isValidPosition( range.end() );
}

bool TextDocument::getAutoCloseBrackets() const {
	return mAutoCloseBrackets;
}

void TextDocument::setAutoCloseBrackets( bool autoCloseBrackets ) {
	mAutoCloseBrackets = autoCloseBrackets;
}

const std::vector<std::pair<String::StringBaseType, String::StringBaseType>>&
TextDocument::getAutoCloseBracketsPairs() const {
	return mAutoCloseBracketsPairs;
}

void TextDocument::setAutoCloseBracketsPairs(
	const std::vector<std::pair<String::StringBaseType, String::StringBaseType>>&
		autoCloseBracketsPairs ) {
	mAutoCloseBracketsPairs = autoCloseBracketsPairs;
}

bool TextDocument::isDirtyOnFileSystem() const {
	return mDirtyOnFileSystem;
}

void TextDocument::setDirtyOnFileSystem( bool dirtyOnFileSystem ) {
	mDirtyOnFileSystem = dirtyOnFileSystem;
	if ( mDirtyOnFileSystem )
		notifyDirtyOnFileSystem();
}

bool TextDocument::isSaving() const {
	return mSaving;
}

TextPosition TextDocument::sanitizePosition( const TextPosition& position ) const {
	Int64 line = eeclamp<Int64>( position.line(), 0UL, mLines.size() ? mLines.size() - 1 : 0 );
	Int64 col =
		eeclamp<Int64>( position.column(), 0UL, eemax<Int64>( 0, mLines[line].size() - 1 ) );
	return { line, col };
}

const TextDocument::IndentType& TextDocument::getIndentType() const {
	return mIndentType;
}

void TextDocument::setIndentType( const IndentType& indentType ) {
	mIndentType = indentType;
}

void TextDocument::undo() {
	setRunningTransaction( true );
	mUndoStack.undo();
	setRunningTransaction( false );
	notifyUndoRedo( UndoRedo::Undo );
}

void TextDocument::redo() {
	setRunningTransaction( true );
	mUndoStack.redo();
	setRunningTransaction( false );
	notifyUndoRedo( UndoRedo::Redo );
}

const SyntaxDefinition& TextDocument::getSyntaxDefinition() const {
	return mSyntaxDefinition;
}

void TextDocument::setSyntaxDefinition( const SyntaxDefinition& definition ) {
	mSyntaxDefinition = definition;
	notifySyntaxDefinitionChange();
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

const URI& TextDocument::getURI() const {
	return mFileURI;
}

const FileInfo& TextDocument::getFileInfo() const {
	return mFileRealPath;
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

void TextDocument::setCommands( const std::map<std::string, DocumentCommand>& cmds ) {
	mCommands.insert( cmds.begin(), cmds.end() );
}

void TextDocument::setCommand( const std::string& command,
							   const TextDocument::DocumentCommand& func ) {
	mCommands[command] = func;
}

bool TextDocument::hasCommand( const std::string& command ) {
	return mCommands.find( command ) != mCommands.end();
}

bool TextDocument::removeCommand( const std::string& command ) {
	return mCommands.erase( command ) > 0;
}

static std::pair<size_t, size_t> findType( const String& str, const String& findStr,
										   const TextDocument::FindReplaceType& type ) {
	switch ( type ) {
		case TextDocument::FindReplaceType::LuaPattern: {
			LuaPattern words( findStr );
			int start, end = 0;
			words.find( str, start, end );
			if ( start < 0 )
				return { String::StringType::npos, String::StringType::npos };
			else
				return { start, end };
		}
		case TextDocument::FindReplaceType::Normal:
		default: {
			size_t res = str.find( findStr );
			return { res, String::InvalidPos == res ? res : res + findStr.size() };
		}
	}
}

static std::pair<size_t, size_t> findLastType( const String& str, const String& findStr,
											   const TextDocument::FindReplaceType& type ) {
	switch ( type ) {
		case TextDocument::FindReplaceType::LuaPattern: {
			// TODO: Implement findLastType for Lua patterns
			LuaPattern words( findStr );
			int start, end = 0;
			words.find( str, start, end );
			if ( start < 0 )
				return { String::StringType::npos, String::StringType::npos };
			else
				return { end, start };
		}
		case TextDocument::FindReplaceType::Normal:
		default: {
			size_t res = str.rfind( findStr );
			return { res, String::InvalidPos == res ? res : res + findStr.size() };
		}
	}
}

TextRange TextDocument::findText( String text, TextPosition from, bool caseSensitive,
								  bool wholeWord, const FindReplaceType& type,
								  TextRange restrictRange ) {
	if ( text.empty() )
		return TextRange();
	from = sanitizePosition( from );

	TextPosition to = endOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.end();
		if ( from < restrictRange.start() || from > restrictRange.end() )
			return TextRange();
	}

	if ( type == FindReplaceType::LuaPattern && caseSensitive == false )
		caseSensitive =
			true; // Ignore case insensitive request since this is managed at pattern level

	if ( !caseSensitive )
		text.toLower();

	for ( Int64 i = from.line(); i <= to.line(); i++ ) {
		std::pair<size_t, size_t> col;
		if ( i == from.line() ) {
			col = caseSensitive
					  ? findType( line( i ).getText().substr( from.column() ), text, type )
					  : findType( String::toLower( line( i ).getText() ).substr( from.column() ),
								  text, type );
			if ( String::StringType::npos != col.first ) {
				col.first += from.column();
				col.second += from.column();
			}
		} else if ( i == to.line() && to != endOfDoc() ) {
			col = caseSensitive
					  ? findType( line( i ).getText().substr( 0, to.column() ), text, type )
					  : findType( String::toLower( line( i ).getText() ).substr( 0, to.column() ),
								  text, type );
		} else {
			col = caseSensitive ? findType( line( i ).getText(), text, type )
								: findType( String::toLower( line( i ).getText() ), text, type );
		}
		if ( String::StringType::npos != col.first &&
			 ( !wholeWord || String::isWholeWord( line( i ).getText(), text, col.first ) ) ) {
			TextRange pos( { { (Int64)i, (Int64)col.first }, { (Int64)i, (Int64)col.second } } );
			if ( pos.end().column() == (Int64)mLines[pos.end().line()].size() )
				pos.setEnd( positionOffset( pos.end(), 1 ) );
			return pos;
		}
	}
	return TextRange();
}

TextRange TextDocument::findTextLast( String text, TextPosition from, bool caseSensitive,
									  bool wholeWord, const FindReplaceType& type,
									  TextRange restrictRange ) {
	if ( text.empty() )
		return TextRange();
	from = sanitizePosition( from );

	TextPosition to = startOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.start();
		if ( from < restrictRange.start() || from > restrictRange.end() )
			return TextRange();
	}

	if ( type == FindReplaceType::LuaPattern && caseSensitive == false )
		caseSensitive =
			true; // Ignore case insensitive request since this is managed at pattern level

	if ( !caseSensitive )
		text.toLower();

	for ( Int64 i = from.line(); i >= to.line(); i-- ) {
		std::pair<size_t, size_t> col;
		if ( i == from.line() ) {
			col = caseSensitive
					  ? findLastType( line( i ).getText().substr( 0, from.column() ), text, type )
					  : findLastType(
							String::toLower( line( i ).getText().substr( 0, from.column() ) ), text,
							type );
		} else if ( i == to.line() ) {
			col = caseSensitive
					  ? findLastType( line( i ).getText().substr( to.column() ), text, type )
					  : findLastType( String::toLower( line( i ).getText().substr( to.column() ) ),
									  text, type );
			if ( String::StringType::npos != col.first ) {
				col.first += to.column();
				col.second += to.column();
			}
		} else {
			col = caseSensitive
					  ? findLastType( line( i ).getText(), text, type )
					  : findLastType( String::toLower( line( i ).getText() ), text, type );
		}
		if ( String::StringType::npos != col.first &&
			 ( !wholeWord || String::isWholeWord( line( i ).getText(), text, col.first ) ) ) {
			TextRange pos( { { (Int64)i, (Int64)col.second }, { (Int64)i, (Int64)col.first } } );
			if ( pos.start().column() == (Int64)mLines[pos.start().line()].size() )
				pos.setStart( positionOffset( pos.start(), 1 ) );
			return pos;
		}
	}
	return TextRange();
}

TextRange TextDocument::find( const String& text, TextPosition from, bool caseSensitive,
							  bool wholeWord, const FindReplaceType& type,
							  TextRange restrictRange ) {
	std::vector<String> textLines = text.split( '\n', true, true );

	if ( textLines.empty() || textLines.size() > mLines.size() )
		return TextRange();

	from = sanitizePosition( from );

	TextPosition to = endOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.end();
		if ( from < restrictRange.start() || from >= restrictRange.end() )
			return TextRange();
	}

	if ( from == to )
		return TextRange();

	if ( textLines.size() == 1 )
		return findText( text, from, caseSensitive, wholeWord, type, restrictRange );

	TextRange range = findText( textLines[0], from, caseSensitive, false, type, restrictRange );

	if ( !range.isValid() )
		return TextRange();

	TextPosition initPos( range.end().line(), 0 );

	for ( size_t i = 1; i < textLines.size() - 1; i++ ) {
		if ( initPos < from || initPos > to )
			return find( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

		String currentLine( mLines[initPos.line()].getText() );

		if ( TextPosition( initPos.line(), (Int64)currentLine.size() - 1 ) > to )
			return find( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

		if ( !caseSensitive ) {
			currentLine.toLower();
			textLines[i].toLower();
		}

		if ( currentLine == textLines[i] ) {
			initPos = TextPosition( initPos.line() + 1, 0 );

			if ( initPos >= restrictRange.end() )
				return TextRange();
		} else {
			return find( text, range.end(), caseSensitive, wholeWord, type, restrictRange );
		}
	}

	if ( initPos < from || initPos > to )
		return find( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

	const String& lastLine = mLines[initPos.line()].getText();
	const String& curSearch = textLines[textLines.size() - 1];

	if ( TextPosition( initPos.line(), (Int64)curSearch.size() - 1 ) > to )
		return find( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

	if ( lastLine.size() < curSearch.size() )
		return find( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

	if ( ( caseSensitive && String::startsWith( lastLine, curSearch ) ) ||
		 ( !caseSensitive &&
		   String::startsWith( String( lastLine ).toLower(), String( curSearch ).toLower() ) ) ) {
		TextRange foundRange( range.start(), TextPosition( initPos.line(), curSearch.size() ) );
		if ( foundRange.end().column() == (Int64)mLines[foundRange.end().line()].size() )
			foundRange.setEnd( positionOffset( foundRange.end(), 1 ) );
		return foundRange;
	} else {
		return find( text, range.end(), caseSensitive, wholeWord, type, restrictRange );
	}

	return TextRange();
}

TextRange TextDocument::findLast( const String& text, TextPosition from, bool caseSensitive,
								  bool wholeWord, const FindReplaceType& type,
								  TextRange restrictRange ) {
	std::vector<String> textLines = text.split( '\n', true, true );

	if ( textLines.empty() || textLines.size() > mLines.size() )
		return TextRange();

	from = sanitizePosition( from );

	TextPosition to = startOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.start();
		if ( from < restrictRange.start() || from > restrictRange.end() )
			return TextRange();
	}

	if ( from == to )
		return TextRange();

	if ( textLines.size() == 1 )
		return findTextLast( text, from, caseSensitive, wholeWord, type, restrictRange );

	TextRange range = findTextLast( textLines[0], from, caseSensitive, false, type, restrictRange );

	if ( !range.isValid() )
		return TextRange();

	TextPosition initPos( range.end().line(), 0 );

	for ( size_t i = 1; i < textLines.size() - 1; i++ ) {
		if ( initPos < from || initPos > to )
			return findLast( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

		String currentLine( mLines[initPos.line()].getText() );

		if ( TextPosition( initPos.line(), (Int64)currentLine.size() - 1 ) > to )
			return findLast( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

		if ( !caseSensitive ) {
			currentLine.toLower();
			textLines[i].toLower();
		}

		if ( currentLine == textLines[i] ) {
			initPos = TextPosition( i + 1, 0 );
		} else {
			return findLast( text, range.end(), caseSensitive, wholeWord, type, restrictRange );
		}
	}

	if ( initPos < from || initPos > to )
		return findLast( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

	const String& lastLine = mLines[initPos.line()].getText();
	const String& curSearch = textLines[textLines.size() - 1];

	if ( TextPosition( initPos.line(), (Int64)curSearch.size() - 1 ) > to )
		return findLast( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

	if ( lastLine.size() < curSearch.size() )
		return findLast( text, range.end(), caseSensitive, wholeWord, type, restrictRange );

	if ( ( caseSensitive && String::startsWith( lastLine, curSearch ) ) ||
		 ( !caseSensitive &&
		   String::startsWith( String( lastLine ).toLower(), String( curSearch ).toLower() ) ) ) {
		TextRange foundRange( range.start(), TextPosition( initPos.line(), curSearch.size() ) );
		if ( foundRange.end().column() == (Int64)mLines[foundRange.end().line()].size() )
			foundRange.setEnd( positionOffset( foundRange.end(), 1 ) );
		return foundRange;
	}

	return TextRange();
}

TextRanges TextDocument::findAll( const String& text, bool caseSensitive, bool wholeWord,
								  const FindReplaceType& type, TextRange restrictRange ) {
	TextRanges all;
	TextRange found;
	TextPosition from = startOfDoc();
	if ( restrictRange.isValid() )
		from = restrictRange.normalized().start();
	do {
		found = find( text, from, caseSensitive, wholeWord, type, restrictRange );
		if ( found.isValid() ) {
			if ( !all.empty() && all.back() == found )
				break;
			from = found.end();
			all.push_back( found );
		}
	} while ( found.isValid() );
	if ( !all.empty() )
		all.setSorted();
	return all;
}

int TextDocument::replaceAll( const String& text, const String& replace, const bool& caseSensitive,
							  const bool& wholeWord, const FindReplaceType& type,
							  TextRange restrictRange ) {
	if ( text.empty() )
		return 0;
	bool wasRunningTransaction = isRunningTransaction();
	if ( wasRunningTransaction )
		setRunningTransaction( true );
	int count = 0;
	TextRange found;
	TextPosition startedPosition = getSelection().start();
	TextPosition from = startOfDoc();
	if ( restrictRange.isValid() )
		from = restrictRange.normalized().start();
	do {
		found = find( text, from, caseSensitive, wholeWord, type, restrictRange );
		if ( found.isValid() ) {
			setSelection( found );
			from = replaceSelection( replace );
			count++;
		}
	} while ( found.isValid() && endOfDoc() != found.end() );
	if ( wasRunningTransaction )
		setRunningTransaction( false );
	setSelection( startedPosition );
	return count;
}

TextPosition TextDocument::replaceSelection( const String& replace ) {
	return replaceSelection( 0, replace );
}

TextPosition TextDocument::replaceSelection( const size_t& cursorIdx, const String& replace ) {
	eeASSERT( cursorIdx < mSelection.size() );
	if ( getSelectionIndex( cursorIdx ).hasSelection() ) {
		deleteTo( cursorIdx, 0 );
		setSelection( cursorIdx,
					  insert( cursorIdx, getSelectionIndex( cursorIdx ).start(), replace ) );
	}
	return getSelectionIndex( cursorIdx ).normalized().end();
}

void TextDocument::cursorUndo() {
	mSelection.erase( mSelection.begin() + mLastSelection );
	mLastSelection = mSelection.size() - 1;
	notifySelectionChanged();
	notifyCursorChanged();
}

void TextDocument::selectAllMatches() {
	if ( !hasSelection() )
		return;
	auto sel = getSelection();
	TextRanges ranges = findAll( getSelectedText(), true, false );
	for ( const auto& range : ranges ) {
		if ( sel == range || sel.normalized() == range )
			continue;
		addSelection( range.reversed() );
	}
}

TextPosition TextDocument::replace( String search, const String& replace, TextPosition from,
									const bool& caseSensitive, const bool& wholeWord,
									const FindReplaceType& type, TextRange restrictRange ) {
	TextRange found( findText( search, from, caseSensitive, wholeWord, type, restrictRange ) );
	if ( found.isValid() ) {
		setSelection( found );
		deleteTo( 0, 0 );
		setSelection( 0, insert( 0, getSelectionIndex( 0 ).start(), replace ) );
		return found.end();
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

TextPosition TextDocument::getMatchingBracket( TextPosition sp,
											   const String::StringBaseType& openBracket,
											   const String::StringBaseType& closeBracket,
											   int dir ) {
	SyntaxHighlighter* highlighter = mHighlighter.get();
	int depth = 0;
	while ( sp.isValid() ) {
		auto byte = getChar( sp );
		if ( byte == openBracket ) {
			if ( highlighter ) {
				auto type = highlighter->getTokenTypeAt( sp );
				if ( type != "comment" && type != "string" )
					depth++;
			} else {
				depth++;
			}

			if ( depth == 0 )
				return sp;
		} else if ( byte == closeBracket ) {
			if ( highlighter ) {
				auto type = highlighter->getTokenTypeAt( sp );
				if ( type != "comment" && type != "string" )
					depth--;
			} else {
				depth--;
			}

			if ( depth == 0 )
				return sp;
		}

		auto prevPos = sp;
		sp = positionOffset( sp, dir );
		if ( sp == prevPos )
			return {};
	}
	return {};
}

SyntaxHighlighter* TextDocument::getHighlighter() const {
	return mHighlighter.get();
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

void TextDocument::notifyTextChanged( const DocumentContentChange& change ) {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentTextChanged( change );
	}
}

void TextDocument::notifyCursorChanged( TextPosition selection ) {
	if ( !selection.isValid() )
		selection = getSelection().start();
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentCursorChange( selection );
	}
}

void TextDocument::notifySelectionChanged( TextRange selection ) {
	if ( !selection.isValid() )
		selection = getSelection();
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentSelectionChange( selection );
	}
}

void TextDocument::notifyDocumentLoaded() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentLoaded( this );
	}
}

void TextDocument::notifyDocumentReloaded() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentReloaded( this );
	}
}

void TextDocument::notifyDocumentSaved() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentSaved( this );
	}
}

void TextDocument::notifyDocumentClosed() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentClosed( this );
	}
}

void TextDocument::notifyLineCountChanged( const size_t& lastCount, const size_t& newCount ) {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentLineCountChange( lastCount, newCount );
	}
}

void TextDocument::notifyLineChanged( const Int64& lineIndex ) {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentLineChanged( lineIndex );
	}
}

void TextDocument::notifyUndoRedo( const TextDocument::UndoRedo& eventType ) {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentUndoRedo( eventType );
	}
}

void TextDocument::notifyDirtyOnFileSystem() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentDirtyOnFileSystem( this );
	}
}

void TextDocument::notifyDocumentMoved() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentMoved( this );
	}
}

void TextDocument::notifySyntaxDefinitionChange() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentSyntaxDefinitionChange( mSyntaxDefinition );
	}
}

void TextDocument::notifiyDocumenLineMove( const Int64& fromLine, const Int64& numLines ) {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentLineMove( fromLine, numLines );
	}
}

void TextDocument::initializeCommands() {
	mCommands["reset"] = [&] { reset(); };
	mCommands["save"] = [&] { save(); };
	mCommands["delete-to-previous-word"] = [&] { deleteToPreviousWord(); };
	mCommands["delete-to-previous-char"] = [&] { deleteToPreviousChar(); };
	mCommands["delete-to-next-word"] = [&] { deleteToNextWord(); };
	mCommands["delete-to-next-char"] = [&] { deleteToNextChar(); };
	mCommands["delete-current-line"] = [&] { deleteCurrentLine(); };
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
	mCommands["selection-to-upper"] = [&] { toUpperSelection(); };
	mCommands["selection-to-lower"] = [&] { toLowerSelection(); };
	mCommands["reset-cursor"] = [&] { resetCursor(); };
	mCommands["add-cursor-above"] = [&] { addCursorAbove(); };
	mCommands["add-cursor-below"] = [&] { addCursorBelow(); };
	mCommands["cursor-undo"] = [&] { cursorUndo(); };
	mCommands["select-all-matches"] = [&] { selectAllMatches(); };
}

TextRange TextDocument::getTopMostCursor() {
	if ( mSelection.size() == 1 )
		return mSelection.front();
	TextRange topMost( mSelection[0] );
	for ( size_t i = 1; i < mSelection.size(); ++i ) {
		if ( mSelection[i] < topMost )
			topMost = mSelection[i];
	}
	return topMost;
}

TextRange TextDocument::getBottomMostCursor() {
	if ( mSelection.size() == 1 )
		return mSelection.front();
	TextRange bottomMost( mSelection[0] );
	for ( size_t i = 1; i < mSelection.size(); ++i ) {
		if ( mSelection[i] > bottomMost )
			bottomMost = mSelection[i];
	}
	return bottomMost;
}

void TextDocument::addCursorAbove() {
	auto curPos( getTopMostCursor().normalize().start() );
	if ( curPos.line() == 0 )
		return;
	curPos.setLine( curPos.line() - 1 );
	curPos = sanitizePosition( curPos );
	addSelection( { curPos, curPos } );
	notifyCursorChanged( curPos );
}

void TextDocument::addCursorBelow() {
	auto curPos( getBottomMostCursor().normalize().start() );
	if ( curPos.line() >= (Int64)linesCount() - 1 )
		return;
	curPos.setLine( curPos.line() + 1 );
	curPos = sanitizePosition( curPos );
	addSelection( { curPos, curPos } );
	notifyCursorChanged( curPos );
}

TextDocument::Client::~Client() {}

}}} // namespace EE::UI::Doc
