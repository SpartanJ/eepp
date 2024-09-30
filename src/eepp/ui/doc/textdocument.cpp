#include <eepp/core/debug.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/window/engine.hpp>
#include <string>

using namespace std::literals;

using namespace EE::Network;

namespace EE { namespace UI { namespace Doc {

// Text document is loosely based on the SerenityOS (https://github.com/SerenityOS/serenity)
// TextDocument and the lite editor (https://github.com/rxi/lite) implementations.

static constexpr char DEFAULT_NON_WORD_CHARS[] = " \t\n/\\()\"':,.;<>~!@#$%^&*|+=[]{}`?-";

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
	mHighlighter( std::make_unique<SyntaxHighlighter>( this ) ),
	mFoldRangeService( this ) {
	initializeCommands();
	reset();
}

TextDocument::~TextDocument() {
	stopActiveFindAll();

	if ( mHighlighter->isTokenizingAsync() )
		mHighlighter->setStopTokenizingAsync();

	// TODO: Use a condition variable to wait the thread pool to finish
	while ( !mStopFlags.empty() )
		Sys::sleep( Milliseconds( 0.1 ) );

	if ( mLoading ) {
		mLoading = false;
		Lock l( mLoadingMutex );
	}

	{ Lock l( mClientsMutex ); }

	// Loading has been stopped
	while ( mLoadingAsync ) {
		mLoading = false;
		Sys::sleep( Milliseconds( 0.1 ) );
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
	mHash = {};
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
	mSyntaxDefinition = SyntaxDefinitionManager::instance()->getPlainDefinition();
	mUndoStack.clear();
	cleanChangeId();
	notifySyntaxDefinitionChange();
	notifyCursorChanged();
	notifySelectionChanged();
	notifyDocumentReset();
}

void TextDocument::resetCursor() {
	auto cursor = sanitizeRange( getSelection() );
	mSelection.clear();
	mSelection.push_back( cursor );
	mLastSelection = 0;
	notifyCursorChanged();
	notifySelectionChanged();
}

String shiftJISToUTF32( const std::string_view& shiftJISString ) {
	String string;
	auto* ret = Window::Engine::instance()->getPlatformHelper()->iconv(
		"UTF-32LE", "SHIFT-JIS", shiftJISString.data(), shiftJISString.size() );
	if ( ret ) {
		string = String( reinterpret_cast<String::StringBaseType*>( ret ) );
		Window::Engine::instance()->getPlatformHelper()->iconvFree( ret );
	}
	return string;
}

static constexpr int codepointSize( TextFormat::Encoding enc ) {
	switch ( enc ) {
		case TextFormat::Encoding::UTF16LE:
		case TextFormat::Encoding::UTF16BE:
			return 2;
		case TextFormat::Encoding::UTF8:
		default:
			break;
	}
	return 1;
}

static inline void searchSubstr( char* data, const size_t& size, size_t& position,
								 const std::string_view& substr, const std::string_view& substrfb,
								 int codepointSize ) {
	position = 0;
	const char* found =
		std::search( data, data + size, substr.data(), substr.data() + substr.size() );
	if ( found != data + size ) {
		position = ( found - data );
		position += codepointSize;
	} else {
		found =
			std::search( data, data + size, substrfb.data(), substrfb.data() + substrfb.size() );
		if ( found != data + size ) {
			position = ( found - data );
			position += codepointSize;
		} else {
			position = size;
		}
	}
}

static String ptrGetLine( char* data, const size_t& size, size_t& position,
						  TextFormat::Encoding enc ) {
	static constexpr auto LE_END_LF = "\n\0"sv;
	static constexpr auto LE_END_CR = "\r\0"sv;
	static constexpr auto BE_END_LF = "\0\n"sv;
	static constexpr auto BE_END_CR = "\0\r"sv;
	position = 0;
	switch ( enc ) {
		case TextFormat::Encoding::UTF16LE: {
			searchSubstr( data, size, position, LE_END_LF, LE_END_CR, codepointSize( enc ) );
			return String::fromUtf16( data, position, false );
		}
		case TextFormat::Encoding::UTF16BE: {
			searchSubstr( data, size, position, BE_END_LF, BE_END_CR, codepointSize( enc ) );
			return String::fromUtf16( data, position, true );
		}
		case TextFormat::Encoding::UTF8:
		case TextFormat::Encoding::Latin1:
		default:
			break;
	}

	while ( position < size && data[position] != '\n' && data[position] != '\r' )
		position++;
	if ( position < size ) {
		if ( position + 1 < size && data[position] == '\r' && data[position + 1] == '\n' )
			position++;
		position++;
	}

	if ( enc == TextFormat::Encoding::Shift_JIS )
		return shiftJISToUTF32( std::string_view{ data, position } );
	else if ( enc == TextFormat::Encoding::Latin1 )
		return String::fromLatin1( data, position );

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
	MD5::Context md5Ctx;
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
		MD5::init( md5Ctx );

		while ( pending && mLoading ) {
			read = file.read( data.get(), blockSize );
			bufferPtr = data.get();
			consume = read;

			MD5::update( md5Ctx, data.get(), read );

			if ( pending == total ) {
				// Check UTF-8 BOM header
				if ( (char)0xef == data.get()[0] && (char)0xbb == data.get()[1] &&
					 (char)0xbf == data.get()[2] ) {
					bufferPtr += 3;
					consume -= 3;
					mIsBOM = true;
					mEncoding = TextFormat::Encoding::UTF8;
				}
				// Check UTF-16 LE BOM header
				else if ( (char)0xFF == data.get()[0] && (char)0xFE == data.get()[1] ) {
					bufferPtr += 2;
					consume -= 2;
					mIsBOM = true;
					mEncoding = TextFormat::Encoding::UTF16LE;
				}
				// Check UTF-16 BE BOM header
				else if ( (char)0xFE == data.get()[0] && (char)0xFF == data.get()[1] ) {
					bufferPtr += 2;
					consume -= 2;
					mIsBOM = true;
					mEncoding = TextFormat::Encoding::UTF16BE;
				}
				// Try to guess
				else {
					mIsBOM = false;
					IOStreamMemory iomem( bufferPtr, read );
					mEncoding = TextFormat::autodetect( iomem ).encoding;
				}
			}

			while ( consume && mLoading ) {
				lineBuffer += ptrGetLine( bufferPtr, consume, position, mEncoding );
				bufferPtr += position;
				consume -= position;
				size_t lineBufferSize = lineBuffer.size();
				char lastChar = lineBuffer[lineBufferSize - 1];

				if ( lastChar == '\n' || lastChar == '\r' ) {
					if ( mLines.empty() ) {
						if ( lineBufferSize > 1 && lineBuffer[lineBufferSize - 2] == '\r' &&
							 lastChar == '\n' ) {
							mLineEnding = TextFormat::LineEnding::CRLF;
						} else if ( lastChar == '\r' ) {
							mLineEnding = TextFormat::LineEnding::CR;
						}

						static constexpr auto BINARY_STR = "\0\0\0\0"sv;
						mMightBeBinary =
							std::search( lineBuffer.begin(), lineBuffer.end(), BINARY_STR.data(),
										 BINARY_STR.data() + BINARY_STR.size() ) !=
							lineBuffer.end();

						if ( mMightBeBinary && mEncoding == TextFormat::Encoding::UTF16BE ) {
							mEncoding = TextFormat::Encoding::UTF8;
						}
					}

					if ( mLineEnding == TextFormat::LineEnding::CRLF && lineBufferSize > 1 &&
						 lastChar == '\n' ) {
						lineBuffer[lineBuffer.size() - 2] = '\n';
						lineBuffer.resize( lineBufferSize - 1 );
					} else if ( mLineEnding == TextFormat::LineEnding::CR && lineBufferSize > 0 ) {
						lineBuffer[lineBuffer.size() - 1] = '\n';
					}

					mLines.push_back( lineBuffer );
					lineBuffer.resize( 0 );
				} else if ( consume <= 0 && pending - read == 0 ) {
					mLines.push_back( lineBuffer );
				}

				if ( consume < 0 ) {
					eeASSERT( !consume );
					break;
				}
			}

			if ( !read )
				break;
			pending -= read;
			blockSize = eemin( pending, BLOCK_SIZE );
		};
	}

	if ( !mLines.empty() ) {
		const String& lastLine = mLines[mLines.size() - 1].getText();
		if ( lastLine[lastLine.size() - 1] == '\n' ) {
			mLines.push_back( String( "\n" ) );
		} else {
			mLines[mLines.size() - 1].append( "\n" );
		}
	} else {
		mLines.push_back( String( "\n" ) );
	}

	if ( mAutoDetectIndentType )
		guessIndentType();

	if ( mVerbose )
		Log::info( "Document \"%s\" loaded in %.2fms.", path.c_str(),
				   clock.getElapsedTime().asMilliseconds() );

	bool wasInterrupted = !mLoading;
	if ( wasInterrupted )
		reset();

	mHash = MD5::result( md5Ctx ).digest;
	mLoading = false;

	return wasInterrupted ? LoadStatus::Interrupted
						  : ( file.isOpen() ? LoadStatus::Loaded : LoadStatus::Failed );
}

void TextDocument::guessIndentType() {
	int guessSpaces = 0;
	int guessTabs = 0;
	std::map<int, int> guessWidth;

	const auto guessTabsFn = [&]( size_t start, size_t end ) {
		int guessCountdown = 10;
		for ( size_t i = start; i < end; i++ ) {
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
	};

	size_t start = eemin<size_t>( 100, mLines.size() );
	guessTabsFn( 0, start );

	if ( !guessTabs && !guessSpaces ) {
		if ( start == 100 )
			guessTabsFn( start, eemin<size_t>( start + 100, mLines.size() ) );
		if ( !guessTabs && !guessSpaces )
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

const SyntaxDefinition& TextDocument::guessSyntax() const {
	String header( getText( { { 0, 0 }, positionOffset( { 0, 0 }, 128 ) } ) );
	return SyntaxDefinitionManager::instance()->find( mFilePath, header, mHAsCpp );
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

const TextFormat::LineEnding& TextDocument::getLineEnding() const {
	return mLineEnding;
}

void TextDocument::setLineEnding( const TextFormat::LineEnding& lineEnding ) {
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

bool TextDocument::isBOM() const {
	return mIsBOM;
}

void TextDocument::notifyDocumentMoved( const std::string& path ) {
	changeFilePath( path, false );
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

std::string TextDocument::getLoadingFilePath() const {
	Lock l( mLoadingFilePathMutex );
	return mLoadingFilePath;
}

URI TextDocument::getLoadingFileURI() const {
	Lock l( mLoadingFilePathMutex );
	return mLoadingFileURI;
}

TextDocument::LoadStatus TextDocument::loadFromFile( const std::string& path ) {
	mLoading = true;
	if ( !FileSystem::fileExists( path ) && PackManager::instance()->isFallbackToPacksActive() ) {
		std::string pathFix( path );
		Pack* pack = PackManager::instance()->exists( pathFix );
		if ( NULL != pack ) {
			changeFilePath( pathFix, false );
			return loadFromPack( pack, pathFix );
		}
	}

	IOStreamFile file( path, "rb" );
	auto ret = loadFromStream( file, path, true );
	changeFilePath( path, false );
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
		notifyDocumentLoaded();
		mLoadingAsync = false;
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
	mLoading = true;
	URI uri( url );

	if ( uri.getScheme().empty() ) {
		mLoading = false;
		return LoadStatus::Failed;
	}

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
	mLoading = true;
	URI uri( url );

	if ( uri.getScheme().empty() || ( uri.getScheme() != "https" && uri.getScheme() != "http" ) ) {
		mLoading = false;
		return false;
	}

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
			notifyDocumentLoaded();
			mLoadingAsync = false;
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
	if ( path.empty() || mDefaultFileName == path || mSaving )
		return false;
	mSaving = true;
	if ( FileSystem::fileWrite( path, "" ) ) {
		IOStreamFile file( path, "wb" );
		std::string oldFilePath( mFilePath );
		URI oldFileURI( mFileURI );
		FileInfo oldFileInfo( mFileRealPath );
		mFilePath = path;
		mFileURI = URI( "file://" + mFilePath );
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
	BoolScopedOp op( mDoingTextInput, true );
	const std::string whitespaces( " \t\f\v\n\r" );
	MD5::Context md5Ctx;
	MD5::init( md5Ctx );

	if ( mIsBOM ) {
		switch ( mEncoding ) {
			case TextFormat::Encoding::UTF16LE: {
				unsigned char bom[] = { 0xFF, 0xFE };
				stream.write( (char*)bom, sizeof( bom ) );
				MD5::update( md5Ctx, bom, sizeof( bom ) );
				break;
			}
			case TextFormat::Encoding::UTF16BE: {
				unsigned char bom[] = { 0xFE, 0xFF };
				stream.write( (char*)bom, sizeof( bom ) );
				MD5::update( md5Ctx, bom, sizeof( bom ) );
				break;
			}
			case TextFormat::Encoding::UTF8: {
				unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
				stream.write( (char*)bom, sizeof( bom ) );
				MD5::update( md5Ctx, bom, sizeof( bom ) );
				break;
			}
			case TextFormat::Encoding::Shift_JIS:
			case TextFormat::Encoding::Latin1:
				break;
		}
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

		switch ( mLineEnding ) {
			case TextFormat::LineEnding::CRLF: {
				if ( text[text.size() - 1] == '\n' ) {
					text[text.size() - 1] = '\r';
					text += "\n";
				}
				break;
			}
			case TextFormat::LineEnding::CR: {
				text[text.size() - 1] = '\r';
				break;
			}
			case TextFormat::LineEnding::LF: {
				break;
			}
		}

		switch ( mEncoding ) {
			case TextFormat::Encoding::UTF16LE:
			case TextFormat::Encoding::UTF16BE: {
				std::u16string utf16String;
				Utf8::toUtf16( text.begin(), text.end(), std::back_inserter( utf16String ) );
				if ( mEncoding == TextFormat::Encoding::UTF16BE ) {
					for ( char16_t& c : utf16String )
						c = ( ( c >> 8 ) & 0xFF ) | ( ( c << 8 ) & 0xFF00 );
				}
				stream.write( (const char*)utf16String.data(), utf16String.size() * 2 );
				MD5::update( md5Ctx, (const char*)utf16String.data(), utf16String.size() * 2 );
				break;
			}
			case TextFormat::Encoding::Latin1: {
				std::string latin1;
				String utf32( text ); // TODO: Do direct conversion
				latin1.reserve( utf32.size() );
				for ( size_t i = 0; i < utf32.size(); i++ )
					if ( utf32[i] < 0xFF )
						latin1.push_back( utf32[i] );
				stream.write( latin1.c_str(), latin1.size() );
				MD5::update( md5Ctx, latin1.data(), latin1.size() );
				break;
			}
			case TextFormat::Encoding::Shift_JIS: {
				auto* ret = Window::Engine::instance()->getPlatformHelper()->iconv(
					"SHIFT-JIS", "UTF-8", text.c_str(), text.size() );
				auto len = strlen( ret );
				stream.write( ret, len );
				MD5::update( md5Ctx, ret, len );
				Window::Engine::instance()->getPlatformHelper()->iconvFree( ret );
				break;
			}
			case TextFormat::Encoding::UTF8: {
				stream.write( text.c_str(), text.size() );
				MD5::update( md5Ctx, text.data(), text.size() );
				break;
			}
		}
	}

	sanitizeCurrentSelection();

	if ( !keepUndoRedoStatus )
		cleanChangeId();

	mHash = MD5::result( md5Ctx ).digest;
	mDirtyOnFileSystem = false;

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

		if ( !mDoingTextInput && mLastCursorChangeWasInteresting ) {
			notifyInterstingCursorChange( mSelection[cursorIdx].start() );
			mLastCursorChangeWasInteresting = false;
		}
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

TextRange TextDocument::addSelections( TextRanges&& selections ) {
	mSelection.reserve( mSelection.size() + selections.size() );
	for ( auto& selection : selections ) {
		if ( mSelection.exists( selection ) )
			return {};
		selection = sanitizeRange( selection );
		if ( mSelection.exists( selection ) )
			return {};
		mSelection.push_back( selection );
	}
	mSelection.sort();
	mergeSelection();
	notifyCursorChanged( selections.back().start() );
	notifySelectionChanged( selections.back() );
	mLastSelection = mSelection.findIndex( selections.back() );
	return selections.back();
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

TextRanges TextDocument::getSelectionsSorted() const {
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
	static TextDocumentLine safeLine = TextDocumentLine( "" );
	eeASSERT( index < mLines.size() );
	return index >= mLines.size() ? safeLine : mLines[index];
}

const TextDocumentLine& TextDocument::line( const size_t& index ) const {
	static TextDocumentLine safeLine = TextDocumentLine( "" );
	eeASSERT( index < mLines.size() );
	return index >= mLines.size() ? safeLine : mLines[index];
}

size_t TextDocument::linesCount() const {
	return mLines.size();
}

const TextDocumentLine& TextDocument::getCurrentLine() const {
	return mLines[getSelection().start().line()];
}

bool TextDocument::hasSelection() const {
	return mSelection.hasSelection();
}

const std::array<Uint8, 16>& TextDocument::getHash() const {
	return mHash;
}

std::string TextDocument::getHashHexString() const {
	return MD5::Result{ mHash }.toHexString();
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

String::StringBaseType
TextDocument::getCharFromUnsanitizedPosition( const TextPosition& position ) const {
	return mLines[position.line()][position.column()];
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
		mHighlighter->moveHighlight( position.line(), position.line(), linesAdd );
		notifiyDocumenLineMove( position.line(), position.line(), linesAdd );
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

	Int64 linesRemoved = 0;
	bool deletedAcrossNewLine = false;

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
		deletedAcrossNewLine = true;
	}

	if ( mLines.empty() )
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
		mHighlighter->moveHighlight( deletedAcrossNewLine ? range.start().line()
														  : range.end().line(),
									 range.end().line(), -linesRemoved );
		notifiyDocumenLineMove( originalRange.start().line(), originalRange.end().line(),
								-linesRemoved );
	}

	notifyTextChanged( { originalRange, "" } );
	notifyLineChanged( range.start().line() );

	return linesRemoved;
}

TextPosition TextDocument::positionOffset( TextPosition position, int columnOffset,
										   bool sanitizeInput ) const {
	if ( sanitizeInput )
		position = sanitizePosition( position );
	position.setColumn( position.column() + columnOffset );
	while ( position.line() > 0 && position.column() < 0 ) {
		position.setLine( position.line() - 1 );
		position.setColumn(
			eemax<Int64>( 0, position.column() + (Int64)mLines[position.line()].size() ) );
	}
	while ( position.line() < (Int64)mLines.size() - 1 &&
			position.column() >
				(Int64)eemax<Int64>( 0, (Int64)mLines[position.line()].size() - 1 ) ) {
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
		textInput( text, false );
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

TextPosition TextDocument::previousWordBoundary( TextPosition position, bool ignoreFirstNonWord,
												 std::size_t maxSeekChars,
												 bool returnInvalidOnMaxSeek ) const {
	auto ch = getChar( positionOffset( position, -1 ) );
	bool inWord = !isNonWord( ch );
	if ( !ignoreFirstNonWord && !inWord )
		return position;
	String::StringBaseType nextChar = 0;
	Int64 seekedChars = static_cast<Int64>( maxSeekChars );
	do {
		TextPosition curPos = position;
		position = positionOffset( position, -1 );
		if ( curPos == position ) {
			break;
		}
		nextChar = getChar( positionOffset( position, -1 ) );
	} while ( ( ( inWord && !isNonWord( nextChar ) ) || ( !inWord && nextChar == ch ) ) &&
			  --seekedChars );
	return returnInvalidOnMaxSeek && seekedChars == 0 ? TextPosition() : position;
}

TextPosition TextDocument::nextWordBoundary( TextPosition position, bool ignoreFirstNonWord,
											 std::size_t maxSeekChars,
											 bool returnInvalidOnMaxSeek ) const {
	auto ch = getChar( position );
	bool inWord = !isNonWord( ch );
	if ( !ignoreFirstNonWord && !inWord )
		return position;
	String::StringBaseType nextChar = 0;
	Int64 seekedChars = static_cast<Int64>( maxSeekChars );
	do {
		TextPosition curPos = position;
		position = positionOffset( position, 1 );
		if ( curPos == position ) {
			break;
		}
		nextChar = getChar( position );
	} while ( ( ( inWord && !isNonWord( nextChar ) ) || ( !inWord && nextChar == ch ) ) &&
			  --seekedChars );
	return returnInvalidOnMaxSeek && seekedChars == 0 ? TextPosition() : position;
}

TextPosition TextDocument::previousSpaceBoundaryInLine( TextPosition position,
														std::size_t maxSeekChars,
														bool returnInvalidOnMaxSeek ) const {
	auto ch = getChar( positionOffset( position, -1 ) );
	bool inWord = ch != ' ';
	String::StringBaseType nextChar = 0;
	Int64 seekedChars = static_cast<Int64>( maxSeekChars );
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
	} while ( ( ( inWord && nextChar != ' ' ) || ( !inWord && nextChar == ch ) ) && --seekedChars );
	return returnInvalidOnMaxSeek && seekedChars == 0 ? TextPosition() : position;
}

TextPosition TextDocument::nextSpaceBoundaryInLine( TextPosition position, std::size_t maxSeekChars,
													bool returnInvalidOnMaxSeek ) const {
	auto ch = getChar( position );
	bool inWord = ch != ' ';
	String::StringBaseType nextChar = 0;
	Int64 seekedChars = static_cast<Int64>( maxSeekChars );
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
	} while ( ( ( inWord && nextChar != ' ' ) || ( !inWord && nextChar == ch ) ) && --seekedChars );
	return returnInvalidOnMaxSeek && seekedChars == 0 ? TextPosition() : position;
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

TextRange TextDocument::getLineRange( Int64 line ) const {
	return { startOfLine( { line, 0 } ), endOfLine( { line, 0 } ) };
}

void TextDocument::deleteTo( const size_t& cursorIdx, int offset ) {
	eeASSERT( cursorIdx < mSelection.size() );
	BoolScopedOpOptional op( !mDoingTextInput, mDoingTextInput, true );
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
	BoolScopedOpOptional op( !mDoingTextInput, mDoingTextInput, true );
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
	static std::vector<std::pair<String::StringBaseType, String::StringBaseType>>
		sAutoCloseBracketsPairs = { { '(', ')' }, { '{', '}' },	  { '[', ']' },
									{ '"', '"' }, { '\'', '\'' }, { '`', '`' } };

	if ( 1 != text.size() )
		return {};

	size_t pos = std::numeric_limits<size_t>::max();
	bool isClose = false;
	bool isSame = false;
	for ( size_t i = 0; i < sAutoCloseBracketsPairs.size(); i++ ) {
		if ( text[0] == sAutoCloseBracketsPairs[i].first ||
			 text[0] == sAutoCloseBracketsPairs[i].second ) {
			pos = i;
			isClose = text[0] == sAutoCloseBracketsPairs[i].second;
			isSame = sAutoCloseBracketsPairs[i].first == sAutoCloseBracketsPairs[i].second;
			break;
		}
	}

	if ( pos == std::numeric_limits<size_t>::max() )
		return {};

	std::vector<bool> inserted;
	inserted.reserve( mSelection.size() );
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		auto& sel = mSelection[i];
		if ( sel.hasSelection() ) {
			replaceSelection( i, sAutoCloseBracketsPairs[pos].first + getSelectedText() +
									 sAutoCloseBracketsPairs[pos].second );
			inserted.push_back( true );
		} else if ( mAutoCloseBrackets ) {
			// Confirm it's enabled
			auto closeChar = sAutoCloseBracketsPairs[pos].second;
			if ( std::find_if( mAutoCloseBracketsPairs.begin(), mAutoCloseBracketsPairs.end(),
							   [closeChar]( auto bracket ) {
								   return bracket.second == closeChar;
							   } ) == mAutoCloseBracketsPairs.end() )
				continue;
			bool mustClose = true;

			if ( sel.start().column() < (Int64)line( sel.start().line() ).size() ) {
				auto ch = line( sel.start().line() ).getText()[sel.start().column()];

				if ( isClose && ch == closeChar &&
					 ( !isSame ||
					   ( sel.start().column() - 1 >= 0 &&
						 line( sel.start().line() ).getText()[sel.start().column() - 1] ==
							 text[0] ) ) ) {
					deleteTo( i, 1 );
					inserted.push_back( false );
					continue;
				}

				if ( isClose && !isSame )
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

void TextDocument::textInput( const String& text, bool mightBeInteresting ) {
	BoolScopedOp op( mDoingTextInput, true );
	BoolScopedOp op2( mInsertingText, true );

	if ( 1 == text.size() && mAutoCloseBrackets ) {
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

	if ( mightBeInteresting )
		mLastCursorChangeWasInteresting = true;
}

void TextDocument::pasteText( String&& text ) {
	if ( text.find_first_of( '\r' ) != String::InvalidPos )
		String::replaceAll( text, "\r", "" );

	if ( std::count( text.begin(), text.end(), '\n' ) ==
			 static_cast<Int64>( mSelection.size() ) - 1 &&
		 text.back() != '\n' ) {
		std::vector<String> textLines = text.split( '\n', true, false );
		if ( textLines.size() == mSelection.size() ) {
			BoolScopedOp op( mDoingTextInput, true );
			BoolScopedOp op2( mInsertingText, true );
			for ( size_t i = 0; i < mSelection.size(); ++i ) {
				if ( mSelection[i].hasSelection() )
					deleteTo( i, 0 );
				setSelection( i, insert( i, getSelectionIndex( i ).start(), textLines[i] ) );
			}
		} else {
			textInput( text );
		}
	} else {
		textInput( text );
	}

	mLastCursorChangeWasInteresting = true;
}

void TextDocument::imeTextEditing( const String& text ) {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		if ( mSelection[i].hasSelection() )
			deleteTo( i, 0 );
		TextPosition tp( insert( i, getSelectionIndex( i ).start(), text ) );
		setSelection( i, { positionOffset( tp, -text.size() ), tp } );
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
		TextPosition indented = startOfContent( start );
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

void TextDocument::deleteWord() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		moveTo( i, previousWordBoundary( getSelectionIndex( i ).start() ) );
		deleteTo( i, nextWordBoundary( getSelectionIndex( i ).start() ) );
	}
	mergeSelection();
}

void TextDocument::deleteCurrentLine() {
	BoolScopedOpOptional op( !mDoingTextInput, mDoingTextInput, true );
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

TextRange TextDocument::getWordRangeInPosition( const TextPosition& pos, bool basedOnHighlighter ) {
	if ( mHighlighter && basedOnHighlighter ) {
		auto type( mHighlighter->getTokenPositionAt( pos ) );
		return { { pos.line(), type.pos }, { pos.line(), type.pos + (Int64)type.len } };
	}

	return { nextWordBoundary( pos, false ), previousWordBoundary( pos, false ) };
}

TextRange TextDocument::getWordRangeInPosition( bool basedOnHighlighter ) {
	return getWordRangeInPosition( getSelection().start(), basedOnHighlighter );
}

String TextDocument::getWordInPosition( const TextPosition& pos, bool basedOnHighlighter ) {
	return getText( getWordRangeInPosition( pos, basedOnHighlighter ) );
}

String TextDocument::getWordInPosition( bool basedOnHighlighter ) {
	return getWordInPosition( getSelection().start(), basedOnHighlighter );
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
		setSelection( { nextWordBoundary( getSelection().start(), false,
										  std::numeric_limits<std::size_t>::max() ),
						previousWordBoundary( getSelection().start(), false,
											  std::numeric_limits<std::size_t>::max() ) } );
	} else if ( withMulticursor ) {
		String text( getSelectedText() );
		auto res( find( text, getBottomMostCursor().normalized().end() ) );
		if ( res.isValid() && !mSelection.exists( res.result.reversed() ) ) {
			addSelection( res.result.reversed() );
		} else {
			res = findLast( text, getTopMostCursor().normalized().start(), true, false,
							FindReplaceType::Normal,
							{ startOfDoc(), getTopMostCursor().normalized().start() } );
			if ( res.isValid() && !mSelection.exists( res.result ) ) {
				addSelection( res.result );
			}
		}
	}
}

void TextDocument::selectAllWords() {
	if ( !hasSelection() )
		selectWord( false );
	String text( getSelectedText() );
	auto res( findAll( text, true, false, FindReplaceType::Normal,
					   { getBottomMostCursor().normalized().end(), endOfDoc() } ) );
	if ( !res.empty() ) {
		for ( auto& selection : res )
			selection.result.reverse();
		addSelections( res.ranges() );
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

void TextDocument::selectSingleLine() {
	for ( size_t i = 0; i < mSelection.size(); ++i ) {
		auto sel = getSelectionIndex( i );
		setSelection( i, { { sel.start().line(), 0 },
						   { sel.start().line(),
							 eemax( (Int64)line( sel.start().line() ).size() - 1, (Int64)0 ) } } );
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
	BoolScopedOpOptional op( !mDoingTextInput, mDoingTextInput, true );
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
	BoolScopedOpOptional op( !mDoingTextInput, mDoingTextInput, true );
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
	BoolScopedOp op( mDoingTextInput, true );
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
	BoolScopedOp op( mDoingTextInput, true );
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
	BoolScopedOpOptional op( !mDoingTextInput, mDoingTextInput, true );
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
	if ( mSyntaxDefinition.getLSPName() != definition.getLSPName() ) {
		mSyntaxDefinition = definition;
		notifySyntaxDefinitionChange();
	}
}

Uint64 TextDocument::getCurrentChangeId() const {
	return mUndoStack.getCurrentChangeId();
}

const std::string& TextDocument::getDefaultFileName() const {
	return mDefaultFileName;
}

void TextDocument::setDefaultFileName( const std::string& defaultFileName ) {
	mFilePath = mDefaultFileName = defaultFileName;
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
	if ( cmdIt != mCommands.end() )
		cmdIt->second();
}

void TextDocument::execute( const std::string& command, Client* client ) {
	auto cmdRefIt = mRefCommands.find( command );
	if ( cmdRefIt != mRefCommands.end() )
		return cmdRefIt->second( client );
	auto cmdIt = mCommands.find( command );
	if ( cmdIt != mCommands.end() )
		return cmdIt->second();
}

void TextDocument::setCommands( const UnorderedMap<std::string, DocumentCommand>& cmds ) {
	mCommands.insert( cmds.begin(), cmds.end() );
}

void TextDocument::setCommand( const std::string& command,
							   const TextDocument::DocumentCommand& func ) {
	mCommands[command] = func;
}

void TextDocument::setCommand( const std::string& command,
							   const TextDocument::DocumentRefCommand& func ) {
	mRefCommands[command] = func;
}

bool TextDocument::hasCommand( const std::string& command ) {
	return mCommands.find( command ) != mCommands.end() ||
		   mRefCommands.find( command ) != mRefCommands.end();
}

bool TextDocument::removeCommand( const std::string& command ) {
	return mCommands.erase( command ) > 0 || mRefCommands.erase( command ) > 0;
}

static constexpr auto MAX_CAPTURES = 12;

struct FindTypeResult {
	size_t start{ String::StringType::npos };
	size_t end{ String::StringType::npos };
	std::vector<PatternMatcher::Range> captures{};
};

static FindTypeResult findType( const String& str, const String& findStr,
								const TextDocument::FindReplaceType& type, int colOffset,
								bool caseSensitive ) {
	switch ( type ) {
		case TextDocument::FindReplaceType::RegEx: {
			RegEx words( findStr.toUtf8(),
						 static_cast<RegEx::Options>( RegEx::Options::Utf |
													  ( !caseSensitive ? RegEx::Options::Caseless
																	   : RegEx::Options::None ) ) );
			if ( !words.isValid() )
				return { String::StringType::npos, String::StringType::npos };
			RegEx::Range matches[MAX_CAPTURES];
			if ( words.matches( str, matches ) ) {
				FindTypeResult result{ static_cast<size_t>( matches[0].start ),
									   static_cast<size_t>( matches[0].end ) };
				if ( words.getNumMatches() > 1 ) {
					std::vector<TextPosition> captures;
					captures.reserve( words.getNumMatches() - 1 );
					for ( size_t i = 1; i < words.getNumMatches(); i++ ) {
						result.captures.emplace_back( PatternMatcher::Range{
							colOffset + matches[i].start, colOffset + matches[i].end } );
					}
				}
				return result;
			} else {
				return { String::StringType::npos, String::StringType::npos };
			}
		}
		case TextDocument::FindReplaceType::LuaPattern: {
			LuaPatternStorage words( findStr.toUtf8() );
			PatternMatcher::Range matches[MAX_CAPTURES];
			if ( words.matches( str, matches ) ) {
				FindTypeResult result{ static_cast<size_t>( matches[0].start ),
									   static_cast<size_t>( matches[0].end ) };
				if ( words.getNumMatches() > 1 ) {
					std::vector<TextPosition> captures;
					captures.reserve( words.getNumMatches() - 1 );
					for ( size_t i = 1; i < words.getNumMatches(); i++ ) {
						result.captures.emplace_back( PatternMatcher::Range{
							colOffset + matches[i].start, colOffset + matches[i].end } );
					}
				}
				return result;
			} else {
				return { String::StringType::npos, String::StringType::npos };
			}
		}
		case TextDocument::FindReplaceType::Normal:
		default: {
			size_t res = str.find( findStr );
			return { res, String::InvalidPos == res ? res : res + findStr.size() };
		}
	}
}

static FindTypeResult findLastType( const String& str, const String& findStr,
									const TextDocument::FindReplaceType& type,
									bool caseSensitive ) {
	switch ( type ) {
		case TextDocument::FindReplaceType::RegEx: {
			// TODO: Implement findLastType for Lua patterns
			RegEx words( findStr.toUtf8(),
						 static_cast<RegEx::Options>( RegEx::Options::Utf |
													  ( !caseSensitive ? RegEx::Options::Caseless
																	   : RegEx::Options::None ) ) );
			if ( !words.isValid() )
				return { String::StringType::npos, String::StringType::npos };
			RegEx::Range matches[MAX_CAPTURES];
			if ( words.matches( str, matches ) ) {
				FindTypeResult result{ static_cast<size_t>( matches[0].start ),
									   static_cast<size_t>( matches[0].end ) };
				if ( words.getNumMatches() > 1 ) {
					std::vector<TextPosition> captures;
					captures.reserve( words.getNumMatches() - 1 );
					for ( size_t i = 1; i < words.getNumMatches(); i++ ) {
						result.captures.emplace_back(
							PatternMatcher::Range{ matches[i].start, matches[i].end } );
					}
				}
				return result;
			} else {
				return { String::StringType::npos, String::StringType::npos };
			}
		}
		case TextDocument::FindReplaceType::LuaPattern: {
			// TODO: Implement findLastType for Lua patterns
			LuaPatternStorage words( findStr.toUtf8() );
			PatternMatcher::Range matches[MAX_CAPTURES];
			if ( words.matches( str, matches ) ) {
				FindTypeResult result{ static_cast<size_t>( matches[0].start ),
									   static_cast<size_t>( matches[0].end ) };
				if ( words.getNumMatches() > 1 ) {
					std::vector<TextPosition> captures;
					captures.reserve( words.getNumMatches() - 1 );
					for ( size_t i = 1; i < words.getNumMatches(); i++ ) {
						result.captures.emplace_back(
							PatternMatcher::Range{ matches[i].start, matches[i].end } );
					}
				}
				return result;
			} else {
				return { String::StringType::npos, String::StringType::npos };
			}
		}
		case TextDocument::FindReplaceType::Normal:
		default: {
			size_t res = str.rfind( findStr );
			return { res, String::InvalidPos == res ? res : res + findStr.size() };
		}
	}
}

TextDocument::SearchResult toSearchResult( TextDocument* doc, const Int64 line,
										   const FindTypeResult& res ) {
	TextDocument::SearchResult ret;
	TextRange pos(
		{ { line, static_cast<Int64>( res.start ) }, { line, static_cast<Int64>( res.end ) } } );
	if ( pos.end().column() == (Int64)doc->line( pos.end().line() ).size() )
		pos.setEnd( doc->positionOffset( pos.end(), 1 ) );
	ret.result = std::move( pos );
	ret.captures.reserve( res.captures.size() );
	for ( const auto& capture : res.captures ) {
		ret.captures.push_back(
			TextRange( TextPosition( line, capture.start ), TextPosition( line, capture.end ) ) );
	}
	return ret;
}

TextDocument::SearchResult TextDocument::findText( String text, TextPosition from,
												   bool caseSensitive, bool wholeWord,
												   FindReplaceType type, TextRange restrictRange ) {
	if ( text.empty() )
		return TextDocument::SearchResult{};
	from = sanitizePosition( from );

	TextPosition to = endOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.end();
		if ( from < restrictRange.start() || from > restrictRange.end() )
			return TextDocument::SearchResult{};
	}

	bool realCaseSensitive = caseSensitive;
	if ( ( type == FindReplaceType::LuaPattern || type == FindReplaceType::RegEx ) &&
		 caseSensitive == false ) {
		caseSensitive =
			true; // Ignore case insensitive request since this is managed at pattern level
	}

	if ( !caseSensitive )
		text.toLower();

	for ( Int64 i = from.line(); i <= to.line(); i++ ) {
		FindTypeResult col;
		if ( i == from.line() ) {
			col = caseSensitive
					  ? findType( line( i ).getText().substr( from.column(),
															  from.line() == to.line()
																  ? to.column() - from.column()
																  : String::InvalidPos ),
								  text, type, from.column(), realCaseSensitive )
					  : findType( String::toLower( line( i ).getText() )
									  .substr( from.column(), from.line() == to.line()
																  ? to.column() - from.column()
																  : String::InvalidPos ),
								  text, type, from.column(), realCaseSensitive );
			if ( String::StringType::npos != col.start ) {
				col.start += from.column();
				col.end += from.column();
			}
		} else if ( i == to.line() && to != endOfDoc() ) {
			col = caseSensitive
					  ? findType( line( i ).getText().substr( 0, to.column() ), text, type, 0,
								  realCaseSensitive )
					  : findType( String::toLower( line( i ).getText() ).substr( 0, to.column() ),
								  text, type, 0, realCaseSensitive );
		} else {
			col = caseSensitive ? findType( line( i ).getText(), text, type, 0, realCaseSensitive )
								: findType( String::toLower( line( i ).getText() ), text, type, 0,
											realCaseSensitive );
		}
		if ( String::StringType::npos != col.start &&
			 ( !wholeWord || String::isWholeWord( line( i ).getText(), text, col.start ) ) ) {
			return toSearchResult( this, i, col );
		}
	}
	return TextDocument::SearchResult{};
}

TextDocument::SearchResult TextDocument::findTextLast( String text, TextPosition from,
													   bool caseSensitive, bool wholeWord,
													   FindReplaceType type,
													   TextRange restrictRange ) {
	if ( text.empty() )
		return TextDocument::SearchResult{};
	from = sanitizePosition( from );

	TextPosition to = startOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.start();
		if ( from < restrictRange.start() || from > restrictRange.end() )
			return TextDocument::SearchResult{};
	}

	bool realCaseSensitive = caseSensitive;
	if ( ( type == FindReplaceType::LuaPattern || type == FindReplaceType::RegEx ) &&
		 caseSensitive == false ) {
		caseSensitive =
			true; // Ignore case insensitive request since this is managed at pattern level
	}

	if ( !caseSensitive )
		text.toLower();

	for ( Int64 i = from.line(); i >= to.line(); i-- ) {
		FindTypeResult res;
		if ( i == from.line() ) {
			res =
				caseSensitive
					? findLastType( line( i ).getText().substr(
										from.line() == to.line() ? to.column() : 0, from.column() ),
									text, type, realCaseSensitive )
					: findLastType(
						  String::toLower( line( i ).getText().substr(
							  from.line() == to.line() ? to.column() : 0, from.column() ) ),
						  text, type, realCaseSensitive );
		} else if ( i == to.line() ) {
			res = caseSensitive
					  ? findLastType( line( i ).getText().substr( to.column() ), text, type,
									  realCaseSensitive )
					  : findLastType( String::toLower( line( i ).getText().substr( to.column() ) ),
									  text, type, realCaseSensitive );
			if ( String::StringType::npos != res.start ) {
				res.start += to.column();
				res.end += to.column();
			}
		} else {
			res = caseSensitive ? findLastType( line( i ).getText(), text, type, realCaseSensitive )
								: findLastType( String::toLower( line( i ).getText() ), text, type,
												realCaseSensitive );
		}
		if ( String::StringType::npos != res.start &&
			 ( !wholeWord || String::isWholeWord( line( i ).getText(), text, res.start ) ) ) {
			return toSearchResult( this, i, res );
		}
	}
	return TextDocument::SearchResult{};
}

TextDocument::SearchResult TextDocument::find( const String& text, TextPosition from,
											   bool caseSensitive, bool wholeWord,
											   FindReplaceType type, TextRange restrictRange ) {
	std::vector<String> textLines = text.split( '\n', true, true );

	if ( textLines.empty() || textLines.size() > mLines.size() )
		return {};

	from = sanitizePosition( from );

	TextPosition to = endOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.end();
		if ( from < restrictRange.start() || from >= restrictRange.end() )
			return {};
	}

	if ( from == to )
		return {};

	if ( textLines.size() == 1 )
		return findText( text, from, caseSensitive, wholeWord, type, restrictRange );

	auto range = findText( textLines[0], from, caseSensitive, false, type, restrictRange );

	if ( !range.isValid() )
		return {};

	TextPosition initPos( range.result.end().line(), 0 );

	for ( size_t i = 1; i < textLines.size() - 1; i++ ) {
		if ( initPos < from || initPos > to )
			return find( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );

		String currentLine( mLines[initPos.line()].getText() );

		if ( TextPosition( initPos.line(), (Int64)currentLine.size() - 1 ) > to )
			return find( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );

		if ( !caseSensitive ) {
			currentLine.toLower();
			textLines[i].toLower();
		}

		if ( currentLine == textLines[i] ) {
			initPos = TextPosition( initPos.line() + 1, 0 );

			if ( initPos >= restrictRange.end() )
				return {};
		} else {
			return find( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );
		}
	}

	if ( initPos < from || initPos > to )
		return find( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );

	const String& lastLine = mLines[initPos.line()].getText();
	const String& curSearch = textLines[textLines.size() - 1];

	if ( TextPosition( initPos.line(), (Int64)curSearch.size() - 1 ) > to )
		return find( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );

	if ( lastLine.size() < curSearch.size() )
		return find( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );

	if ( ( caseSensitive && String::startsWith( lastLine, curSearch ) ) ||
		 ( !caseSensitive &&
		   String::startsWith( String( lastLine ).toLower(), String( curSearch ).toLower() ) ) ) {
		TextRange foundRange( range.result.start(),
							  TextPosition( initPos.line(), curSearch.size() ) );
		if ( foundRange.end().column() == (Int64)mLines[foundRange.end().line()].size() )
			foundRange.setEnd( positionOffset( foundRange.end(), 1 ) );
		return range;
	} else {
		return find( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );
	}

	return {};
}

TextDocument::SearchResult TextDocument::findLast( const String& text, TextPosition from,
												   bool caseSensitive, bool wholeWord,
												   FindReplaceType type, TextRange restrictRange ) {
	std::vector<String> textLines = text.split( '\n', true, true );

	if ( textLines.empty() || textLines.size() > mLines.size() )
		return {};

	from = sanitizePosition( from );

	TextPosition to = startOfDoc();
	if ( restrictRange.isValid() ) {
		restrictRange = sanitizeRange( restrictRange.normalized() );
		to = restrictRange.start();
		if ( from < restrictRange.start() || from > restrictRange.end() )
			return {};
	}

	if ( from == to )
		return {};

	if ( textLines.size() == 1 )
		return findTextLast( text, from, caseSensitive, wholeWord, type, restrictRange );

	auto range = findTextLast( textLines[0], from, caseSensitive, false, type, restrictRange );

	if ( !range.isValid() )
		return {};

	TextPosition initPos( range.result.end().line(), 0 );

	for ( size_t i = 1; i < textLines.size() - 1; i++ ) {
		if ( initPos < from || initPos > to )
			return findLast( text, range.result.end(), caseSensitive, wholeWord, type,
							 restrictRange );

		String currentLine( mLines[initPos.line()].getText() );

		if ( TextPosition( initPos.line(), (Int64)currentLine.size() - 1 ) > to )
			return findLast( text, range.result.end(), caseSensitive, wholeWord, type,
							 restrictRange );

		if ( !caseSensitive ) {
			currentLine.toLower();
			textLines[i].toLower();
		}

		if ( currentLine == textLines[i] ) {
			initPos = TextPosition( i + 1, 0 );
		} else {
			return findLast( text, range.result.end(), caseSensitive, wholeWord, type,
							 restrictRange );
		}
	}

	if ( initPos < from || initPos > to )
		return findLast( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );

	const String& lastLine = mLines[initPos.line()].getText();
	const String& curSearch = textLines[textLines.size() - 1];

	if ( TextPosition( initPos.line(), (Int64)curSearch.size() - 1 ) > to )
		return findLast( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );

	if ( lastLine.size() < curSearch.size() )
		return findLast( text, range.result.end(), caseSensitive, wholeWord, type, restrictRange );

	if ( ( caseSensitive && String::startsWith( lastLine, curSearch ) ) ||
		 ( !caseSensitive &&
		   String::startsWith( String( lastLine ).toLower(), String( curSearch ).toLower() ) ) ) {
		return range;
	}

	return {};
}

void TextDocument::stopActiveFindAll() {
	Lock l( mStopFlagsMutex );
	for ( const auto& stopFlag : mStopFlags )
		*stopFlag.second.get() = true;
}

bool TextDocument::isDoingTextInput() const {
	return mDoingTextInput;
}

bool TextDocument::isInsertingText() const {
	return mInsertingText;
}

TextDocument::SearchResults TextDocument::findAll( const String& text, bool caseSensitive,
												   bool wholeWord, FindReplaceType type,
												   TextRange restrictRange, size_t maxResults ) {
	SearchResults all;
	TextDocument::SearchResult found;
	TextPosition from = startOfDoc();
	auto stopFlagUP = std::make_unique<bool>( false );
	bool* stopFlag = stopFlagUP.get();
	{
		Lock l( mStopFlagsMutex );
		mStopFlags.insert( { stopFlag, std::move( stopFlagUP ) } );
	}

	if ( restrictRange.isValid() )
		from = restrictRange.normalized().start();
	do {
		found = find( text, from, caseSensitive, wholeWord, type, restrictRange );
		if ( found.isValid() ) {
			if ( !all.empty() && all.back() == found )
				break;
			from = found.result.end();
			all.push_back( found );
			if ( ( maxResults != 0 && all.size() >= maxResults ) || *stopFlag )
				break;
		}
	} while ( found.isValid() );
	if ( !all.empty() )
		all.setSorted();

	{
		Lock l( mStopFlagsMutex );
		mStopFlags.erase( stopFlag );
	}
	return all;
}

int TextDocument::replaceAll( const String& text, const String& replace, const bool& caseSensitive,
							  const bool& wholeWord, FindReplaceType type,
							  TextRange restrictRange ) {
	if ( text.empty() )
		return 0;
	bool wasRunningTransaction = isRunningTransaction();
	if ( !wasRunningTransaction )
		setRunningTransaction( true );
	int count = 0;
	TextDocument::SearchResult found;
	TextPosition startedPosition = getSelection().start();
	TextPosition from = startOfDoc();
	if ( restrictRange.isValid() )
		from = restrictRange.normalized().start();

	size_t numCaptures = 0;
	PatternMatcher::Range matchList[MAX_CAPTURES];

	if ( type == FindReplaceType::LuaPattern || type == FindReplaceType::RegEx ) {
		std::string replaceUtf8( replace.toUtf8() );
		LuaPattern ptrn( "$%d+"sv );
		while ( numCaptures < MAX_CAPTURES &&
				ptrn.matches( replaceUtf8, &matchList[numCaptures],
							  numCaptures > 0 ? matchList[numCaptures - 1].end : 0 ) ) {
			numCaptures++;
		}
	}

	do {
		found = find( text, from, caseSensitive, wholeWord, type, restrictRange );
		if ( found.isValid() ) {
			if ( numCaptures && numCaptures <= found.captures.size() ) {
				String finalReplace( replace );
				std::string l( line( found.captures[0].start().line() ).toUtf8() );
				for ( size_t i = 0; i < numCaptures; i++ ) {
					String matchSubStr( replace.substr(
						matchList[i].start, matchList[i].end - matchList[i].start ) ); // $1 $2 ...
					std::string matchNum( matchSubStr.substr( 1 ) );				   // 1 2 ...
					int num;
					if ( String::fromString( num, matchNum ) && num > 0 &&
						 num - 1 < static_cast<int>( found.captures.size() ) ) {
						auto start = found.captures[num - 1].start().column();
						auto end = found.captures[num - 1].end().column();
						finalReplace.replaceAll(
							matchSubStr, String::fromUtf8( l.substr( start, end - start ) ) );
					}
				}
				setSelection( found.result );
				from = replaceSelection( finalReplace );
			} else {
				setSelection( found.result );
				from = replaceSelection( replace );
			}
			count++;
		}
	} while ( found.isValid() && endOfDoc() != found.result.end() );
	if ( !wasRunningTransaction )
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
	auto ranges = findAll( getSelectedText(), true, false );
	for ( const auto& range : ranges ) {
		if ( sel == range.result || sel.normalized() == range.result )
			continue;
		addSelection( range.result.reversed() );
	}
}

TextPosition TextDocument::replace( String search, const String& replace, TextPosition from,
									const bool& caseSensitive, const bool& wholeWord,
									FindReplaceType type, TextRange restrictRange ) {
	auto found( findText( search, from, caseSensitive, wholeWord, type, restrictRange ) );
	size_t numCaptures = 0;
	PatternMatcher::Range matchList[MAX_CAPTURES];

	if ( type == FindReplaceType::LuaPattern || type == FindReplaceType::RegEx ) {
		std::string replaceUtf8( replace.toUtf8() );
		LuaPattern ptrn( "$%d+"sv );
		while ( numCaptures < MAX_CAPTURES &&
				ptrn.matches( replaceUtf8, &matchList[numCaptures],
							  numCaptures > 0 ? matchList[numCaptures - 1].end : 0 ) ) {
			numCaptures++;
		}
	}

	if ( found.isValid() ) {
		if ( numCaptures && numCaptures == found.captures.size() ) {
			String finalReplace( replace );
			std::string l( line( found.captures[0].start().line() ).toUtf8() );
			for ( size_t i = 0; i < numCaptures; i++ ) {
				String matchSubStr( replace.substr(
					matchList[i].start, matchList[i].end - matchList[i].start ) ); // $1 $2 ...
				std::string matchNum( matchSubStr.substr( 1 ) );				   // 1 2 ...
				int num;
				if ( String::fromString( num, matchNum ) && num > 0 &&
					 num - 1 < static_cast<int>( found.captures.size() ) ) {
					auto start =
						restrictRange.start().column() + found.captures[num - 1].start().column();
					auto end =
						restrictRange.start().column() + found.captures[num - 1].end().column();
					if ( start < static_cast<Int64>( l.size() ) &&
						 end < static_cast<Int64>( l.size() ) ) {
						finalReplace.replaceAll(
							matchSubStr, String::fromUtf8( l.substr( start, end - start ) ) );
					}
				}
			}
			setSelection( found.result );
			replaceSelection( finalReplace );
		} else {
			setSelection( found.result );
			replaceSelection( replace );
		}
		return found.result.end();
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

void TextDocument::resetUndoRedo() {
	mUndoStack.clear();
	cleanChangeId();
	notifyCursorChanged();
	notifySelectionChanged();
}

TextFormat::Encoding TextDocument::getEncoding() const {
	return mEncoding;
}

void TextDocument::setEncoding( TextFormat::Encoding encoding ) {
	mEncoding = encoding;
}

const FoldRangeServive& TextDocument::getFoldRangeService() const {
	return mFoldRangeService;
}

FoldRangeServive& TextDocument::getFoldRangeService() {
	return mFoldRangeService;
}

std::vector<TextDocumentLine> TextDocument::getLines() const {
	return mLines;
}

void TextDocument::setLines( std::vector<TextDocumentLine>&& lines ) {
	mLines = std::move( lines );
}

std::string TextDocument::serializeUndoRedo( bool inverted ) {
	return mUndoStack.toJSON( inverted );
}

void TextDocument::unserializeUndoRedo( const std::string& jsonString ) {
	return mUndoStack.fromJSON( jsonString );
}

void TextDocument::changeFilePath( const std::string& filePath ) {
	changeFilePath( filePath, true );
}

void TextDocument::setDirtyUntilSave() {
	mCleanChangeId = std::numeric_limits<Uint64>::max();
	notifySelectionChanged();
}

static size_t guessFileSize( const TextDocument* doc ) {
	Int64 maxLines = std::min( (Int64)101, (Int64)doc->linesCount() );
	Int64 totalSize = 0;
	for ( Int64 i = 0; i < maxLines; i++ )
		totalSize += doc->line( i ).size();
	return totalSize;
}

bool TextDocument::isHuge() const {
	return linesCount() > 50000 || guessFileSize( this ) > EE_1MB * 10;
}

void TextDocument::changeFilePath( const std::string& filePath, bool notify ) {
	mFilePath = filePath;
	mFileURI = URI( "file://" + mFilePath );
	mFileRealPath = FileInfo::isLink( mFilePath ) ? FileInfo( FileInfo( mFilePath ).linksTo() )
												  : FileInfo( mFilePath );
	if ( notify )
		notifyDocumentMoved();
}

static inline void changeDepth( SyntaxHighlighter* highlighter, int& depth, const TextPosition& pos,
								int dir ) {
	if ( highlighter ) {
		auto type = highlighter->getTokenTypeAt( pos );
		if ( type != "comment"_sst && type != "string"_sst )
			depth += dir;
	} else {
		depth += dir;
	}
}

TextPosition TextDocument::getMatchingBracket( TextPosition sp,
											   const String::StringBaseType& openBracket,
											   const String::StringBaseType& closeBracket,
											   MatchDirection dir, bool allowDepth ) {
	SyntaxHighlighter* highlighter = getHighlighter();
	int depth = 0;
	while ( sp.isValid() ) {
		auto byte = getCharFromUnsanitizedPosition( sp );
		if ( byte == openBracket ) {
			changeDepth( highlighter, depth, sp, 1 );
			if ( depth == 0 )
				return sp;
			if ( !allowDepth && depth > 1 )
				return {};
		} else if ( byte == closeBracket ) {
			changeDepth( highlighter, depth, sp, -1 );
			if ( depth == 0 )
				return sp;
			if ( !allowDepth && depth > 1 )
				return {};
		}

		auto prevPos = sp;
		sp = positionOffset( sp, dir == MatchDirection::Forward ? 1 : -1, false );
		if ( sp == prevPos )
			return {};
	}
	return {};
}

TextRange TextDocument::getMatchingBracket( TextPosition start, const String& openBracket,
											const String& closeBracket, MatchDirection dir,
											bool matchingXMLTags ) {
	if ( !start.isValid() )
		return {};
	SyntaxHighlighter* highlighter = getHighlighter();
	if ( dir == MatchDirection::Forward ) {
		{
			TextPosition end( positionOffset( start, openBracket.size() ) );
			// Skip the open string if the start position is from there. Always start with depth 1
			if ( end.isValid() ) {
				String text = getText( { start, end } );
				if ( text == openBracket )
					start = end;
			}
		}

		// Ensure there's a close bracket
		auto foundClose = find( closeBracket, start );
		if ( !foundClose.isValid() )
			return {}; // Not found, exit

		TextRange foundOpen = { start, start };
		int depth = 1;

		do {
			// Find all the open brackets between the first open bracket and the first close bracket
			do {
				if ( matchingXMLTags ) {
					// Ignore closed XML tags
					do {
						foundOpen = find( openBracket, start, true, false,
										  TextDocument::FindReplaceType::Normal,
										  { start, foundClose.result.start() } )
										.result;

						if ( foundOpen.isValid() ) {
							TextPosition closePosition =
								getMatchingBracket( foundOpen.start(), openBracket[0], '>',
													MatchDirection::Forward, false );
							if ( closePosition.isValid() ) {
								if ( getChar( positionOffset( closePosition, -1 ) ) != '/' ) {
									break;
								} else {
									start = closePosition;
								}
							} else {
								break;
							}
						}
					} while ( foundOpen.isValid() );
				} else {
					foundOpen = find( openBracket, start, true, false,
									  TextDocument::FindReplaceType::Normal,
									  { start, foundClose.result.start() } )
									.result;
				}

				if ( foundOpen.isValid() ) {
					start = foundOpen.end();
					changeDepth( highlighter, depth, start, 1 );
				} else {
					start = foundClose.result.end();
					changeDepth( highlighter, depth, start, -1 );
				}
			} while ( foundOpen.isValid() );

			if ( depth > 0 ) {
				// Find the next close bracket from the last close bracket
				foundClose = find( closeBracket, start );
				if ( !foundClose.isValid() )
					break;
			}
		} while ( depth > 0 );

		return foundClose.result;
	} else {
		{
			TextPosition end( positionOffset( start, -closeBracket.size() ) );
			// Skip the close string if the start position is from there. Always start with depth 1
			if ( end.isValid() ) {
				String text = getText( { end, start } );
				if ( text == closeBracket )
					start = end;
			}
		}

		// Ensure there's an open bracket
		TextRange foundOpen;
		if ( matchingXMLTags ) {
			do {
				foundOpen = findLast( openBracket, start ).result;
				if ( foundOpen.isValid() ) {
					TextPosition closePosition =
						getMatchingBracket( foundOpen.normalized().start(), openBracket[0], '>',
											MatchDirection::Forward, false );
					if ( closePosition.isValid() ) {
						if ( getChar( positionOffset( closePosition, -1 ) ) != '/' ) {
							break;
						} else {
							start = foundOpen.normalized().start();
						}
					} else {
						break;
					}
				}
			} while ( foundOpen.isValid() );
		} else {
			foundOpen = findLast( openBracket, start ).result;
		}
		if ( !foundOpen.isValid() )
			return {}; // Not found, exit

		TextRange foundClose = { start, start };
		int depth = 1;

		do {
			// Find all the close brackets between the first close bracket and the first open
			// bracket
			TextRange lastFoundClose;
			do {
				lastFoundClose = foundClose;
				foundClose =
					findLast( closeBracket, start, true, false,
							  TextDocument::FindReplaceType::Normal, { start, foundOpen.start() } )
						.result;
				if ( foundClose.isValid() ) {
					start = foundClose.end();
					changeDepth( highlighter, depth, start, 1 );
				} else {
					start = foundOpen.end();
					changeDepth( highlighter, depth, start, -1 );
				}
			} while ( foundClose.isValid() && lastFoundClose != foundClose );

			if ( depth > 0 ) {
				// Find the next open bracket from the last open bracket
				auto prevFoundOpen = foundOpen;
				if ( matchingXMLTags ) {
					do {
						foundOpen = findLast( openBracket, start ).result;
						if ( foundOpen.isValid() ) {
							TextPosition closePosition =
								getMatchingBracket( foundOpen.normalized().start(), openBracket[0],
													'>', MatchDirection::Forward, false );
							if ( closePosition.isValid() ) {
								if ( getChar( positionOffset( closePosition, -1 ) ) != '/' ) {
									break;
								} else {
									start = foundOpen.normalized().start();
								}
							} else {
								break;
							}
						}
					} while ( foundOpen.isValid() );
				} else {
					foundOpen = findLast( openBracket, start ).result;
				}

				if ( !foundOpen.isValid() || prevFoundOpen == foundOpen )
					break;
			}
		} while ( depth > 0 );

		return foundOpen;
	}
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
	for ( Int64 i = selection.start().line(); i <= selection.end().line(); i++ ) {
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

void TextDocument::notifyInterstingCursorChange( TextPosition selection ) {
	if ( !selection.isValid() )
		selection = getSelection().start();
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentInterestingCursorChange( selection );
	}
}

void TextDocument::notifyFoldRegionsUpdated( size_t oldCount, size_t newCount ) {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onFoldRegionsUpdated( oldCount, newCount );
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

void TextDocument::notifyDocumentReset() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentReset( this );
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

void TextDocument::notifiyDocumenLineMove( const Int64& fromLine, const Int64& toLine,
										   const Int64& numLines ) {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		client->onDocumentLineMove( fromLine, toLine, numLines );
	}
}

void TextDocument::initializeCommands() {
	mCommands["reset"] = [this] { reset(); };
	mCommands["save"] = [this] { save(); };
	mCommands["delete-to-previous-word"] = [this] { deleteToPreviousWord(); };
	mCommands["delete-to-previous-char"] = [this] { deleteToPreviousChar(); };
	mCommands["delete-to-next-word"] = [this] { deleteToNextWord(); };
	mCommands["delete-to-next-char"] = [this] { deleteToNextChar(); };
	mCommands["delete-current-line"] = [this] { deleteCurrentLine(); };
	mCommands["delete-selection"] = [this] { deleteSelection(); };
	mCommands["delete-word"] = [this] { deleteWord(); };
	mCommands["move-to-previous-char"] = [this] { moveToPreviousChar(); };
	mCommands["move-to-previous-word"] = [this] { moveToPreviousWord(); };
	mCommands["move-to-next-char"] = [this] { moveToNextChar(); };
	mCommands["move-to-next-word"] = [this] { moveToNextWord(); };
	mCommands["move-to-previous-line"] = [this] { moveToPreviousLine(); };
	mCommands["move-to-next-line"] = [this] { moveToNextLine(); };
	mCommands["move-to-previous-page"] = [this] { moveToPreviousPage( mPageSize ); };
	mCommands["move-to-next-page"] = [this] { moveToNextPage( mPageSize ); };
	mCommands["move-to-start-of-doc"] = [this] { moveToStartOfDoc(); };
	mCommands["move-to-end-of-doc"] = [this] { moveToEndOfDoc(); };
	mCommands["move-to-start-of-line"] = [this] { moveToStartOfLine(); };
	mCommands["move-to-end-of-line"] = [this] { moveToEndOfLine(); };
	mCommands["move-to-start-of-content"] = [this] { moveToStartOfContent(); };
	mCommands["move-lines-up"] = [this] { moveLinesUp(); };
	mCommands["move-lines-down"] = [this] { moveLinesDown(); };
	mCommands["select-to-previous-char"] = [this] { selectToPreviousChar(); };
	mCommands["select-to-previous-word"] = [this] { selectToPreviousWord(); };
	mCommands["select-to-previous-line"] = [this] { selectToPreviousLine(); };
	mCommands["select-to-next-char"] = [this] { selectToNextChar(); };
	mCommands["select-to-next-word"] = [this] { selectToNextWord(); };
	mCommands["select-to-next-line"] = [this] { selectToNextLine(); };
	mCommands["select-word"] = [this] { selectWord(); };
	mCommands["select-all-words"] = [this] { selectAllWords(); };
	mCommands["select-line"] = [this] { selectLine(); };
	mCommands["select-single-line"] = [this] { selectSingleLine(); };
	mCommands["select-to-start-of-line"] = [this] { selectToStartOfLine(); };
	mCommands["select-to-end-of-line"] = [this] { selectToEndOfLine(); };
	mCommands["select-to-start-of-doc"] = [this] { selectToStartOfDoc(); };
	mCommands["select-to-start-of-content"] = [this] { selectToStartOfContent(); };
	mCommands["select-to-end-of-doc"] = [this] { selectToEndOfDoc(); };
	mCommands["select-to-previous-page"] = [this] { selectToPreviousPage( mPageSize ); };
	mCommands["select-to-next-page"] = [this] { selectToNextPage( mPageSize ); };
	mCommands["select-all"] = [this] { selectAll(); };
	mCommands["new-line"] = [this] { newLine(); };
	mCommands["new-line-above"] = [this] { newLineAbove(); };
	mCommands["indent"] = [this] { indent(); };
	mCommands["unindent"] = [this] { unindent(); };
	mCommands["undo"] = [this] { undo(); };
	mCommands["redo"] = [this] { redo(); };
	mCommands["toggle-line-comments"] = [this] { toggleLineComments(); };
	mCommands["selection-to-upper"] = [this] { toUpperSelection(); };
	mCommands["selection-to-lower"] = [this] { toLowerSelection(); };
	mCommands["reset-cursor"] = [this] { resetCursor(); };
	mCommands["add-cursor-above"] = [this] { addCursorAbove(); };
	mCommands["add-cursor-below"] = [this] { addCursorBelow(); };
	mCommands["cursor-undo"] = [this] { cursorUndo(); };
	mCommands["select-all-matches"] = [this] { selectAllMatches(); };
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

bool TextSearchParams::isEmpty() {
	return text.empty();
}

}}} // namespace EE::UI::Doc
