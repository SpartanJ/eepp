#include "lintermodule.hpp"
#include "thirdparty/json.hpp"
#include "thirdparty/subprocess.h"
#include <algorithm>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <random>

using json = nlohmann::json;

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define LINTER_THREADED 1
#else
#define LINTER_THREADED 0
#endif

LinterModule::LinterModule( const std::string& lintersPath, std::shared_ptr<ThreadPool> pool ) :
	mPool( pool ) {
#if LINTER_THREADED
	mPool->run( [&, lintersPath] { load( lintersPath ); }, [] {} );
#else
	load( lintersPath );
#endif
}

LinterModule::~LinterModule() {
	mClosing = true;
	for ( const auto& editor : mEditors ) {
		for ( auto listener : editor.second )
			editor.first->removeEventListener( listener );
		editor.first->unregisterModule( this );
	}
}

void LinterModule::load( const std::string& lintersPath ) {
	if ( !FileSystem::fileExists( lintersPath ) )
		return;
	try {
		std::ifstream stream( lintersPath );
		json j;
		stream >> j;

		for ( auto& obj : j ) {
			Linter linter;
			auto fp = obj["file_patterns"];

			for ( auto& pattern : fp )
				linter.files.push_back( pattern.get<std::string>() );

			linter.warningPattern = obj["warning_pattern"].get<std::string>();
			linter.command = obj["command"].get<std::string>();

			if ( obj.contains( "warning_pattern_order" ) ) {
				auto& wpo = obj["warning_pattern_order"];
				if ( wpo.contains( "line" ) )
					linter.warningPatternOrder.line = wpo["line"].get<int>();
				if ( wpo.contains( "col" ) )
					linter.warningPatternOrder.col = wpo["col"].get<int>();
				if ( wpo.contains( "message" ) )
					linter.warningPatternOrder.message = wpo["message"].get<int>();
				if ( wpo.contains( "type" ) )
					linter.warningPatternOrder.type = wpo["type"].get<int>();
			}

			mLinters.emplace_back( std::move( linter ) );
		}

		mReady = true;
	} catch ( json::exception& e ) {
		mReady = false;
		Log::error( "Parsing linter failed:\n%s", e.what() );
	}
}

void LinterModule::onRegister( UICodeEditor* editor ) {
	Lock l( mDocMutex );

	std::vector<Uint32> listeners;

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [&]( const Event* event ) {
			Lock l( mDocMutex );
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			setDocDirty( docEvent->getDoc() );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentClosed, [&]( const Event* event ) {
			Lock l( mDocMutex );
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			TextDocument* doc = docEvent->getDoc();
			mDocs.erase( doc );
			mDirtyDoc.erase( doc );
			Lock matchesLock( mMatchesMutex );
			mMatches.erase( doc );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentChanged, [&, editor]( const Event* ) {
			TextDocument* oldDoc = mEditorDocs[editor];
			TextDocument* newDoc = editor->getDocumentRef().get();
			Lock l( mDocMutex );
			mDocs.erase( oldDoc );
			mDirtyDoc.erase( oldDoc );
			mEditorDocs[editor] = newDoc;
			Lock matchesLock( mMatchesMutex );
			mMatches.erase( oldDoc );
		} ) );

	listeners.push_back( editor->addEventListener(
		Event::OnTextChanged, [&, editor]( const Event* ) { setDocDirty( editor ); } ) );

	mEditors.insert( { editor, listeners } );
	mDocs.insert( editor->getDocumentRef().get() );
	mEditorDocs[editor] = editor->getDocumentRef().get();
}

void LinterModule::onUnregister( UICodeEditor* editor ) {
	if ( mClosing )
		return;
	Lock l( mDocMutex );
	TextDocument* doc = mEditorDocs[editor];
	auto cbs = mEditors[editor];
	for ( auto listener : cbs )
		editor->removeEventListener( listener );
	mEditors.erase( editor );
	mEditorDocs.erase( editor );
	for ( auto editor : mEditorDocs )
		if ( editor.second == doc )
			return;
	mDocs.erase( doc );
	mDirtyDoc.erase( doc );
	Lock matchesLock( mMatchesMutex );
	mMatches.erase( doc );
}

void LinterModule::update( UICodeEditor* editor ) {
	TextDocument* doc = editor->getDocumentRef().get();
	auto it = mDirtyDoc.find( doc );
	if ( it != mDirtyDoc.end() && it->second->getElapsedTime() >= mDelayTime ) {
		mDirtyDoc.erase( doc );
#if LINTER_THREADED
		mPool->run( [&, doc] { lintDoc( doc ); }, [] {} );
#endif
	}
}

const Time& LinterModule::getDelayTime() const {
	return mDelayTime;
}

void LinterModule::setDelayTime( const Time& delayTime ) {
	mDelayTime = delayTime;
}

static std::string randString( size_t len ) {
	std::string str( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" );
	std::random_device rd;
	std::mt19937 generator( rd() );
	std::shuffle( str.begin(), str.end(), generator );
	return str.substr( 0, len );
}

void LinterModule::lintDoc( TextDocument* doc ) {
	auto linter = supportsLinter( doc );
	if ( linter.command.empty() )
		return;
	IOStreamString fileString;
	if ( doc->isDirty() || !doc->hasFilepath() ) {
		std::string tmpPath;
		if ( !doc->hasFilepath() ) {
			tmpPath = Sys::getTempPath() + ".ecode-" + doc->getFilename() + "." + randString( 8 );
		} else {
			std::string fileDir( FileSystem::fileRemoveFileName( doc->getFilePath() ) );
			FileSystem::dirAddSlashAtEnd( fileDir );
			tmpPath = fileDir + "." + randString( 8 ) + "." + doc->getFilename();
		}

		doc->save( fileString, true );
		FileSystem::fileWrite( tmpPath, (Uint8*)fileString.getStreamPointer(),
							   fileString.getSize() );
		runLinter( doc, linter, tmpPath );
		FileSystem::fileRemove( tmpPath );
	} else {
		runLinter( doc, linter, doc->getFilePath() );
	}
}

void LinterModule::runLinter( TextDocument* doc, const Linter& linter, const std::string& path ) {
	Clock clock;
	std::string cmd( linter.command );
	String::replaceAll( cmd, "$FILENAME", path );
	std::vector<std::string> cmdArr = String::split( cmd, ' ' );
	std::vector<const char*> strings;
	for ( size_t i = 0; i < cmdArr.size(); ++i )
		strings.push_back( cmdArr[i].c_str() );
	strings.push_back( NULL );
	struct subprocess_s subprocess;
	int result = subprocess_create( strings.data(),
									subprocess_option_inherit_environment |
										subprocess_option_combined_stdout_stderr,
									&subprocess );
	if ( 0 == result ) {
		std::string buffer( 1024, '\0' );
		std::string data;
		unsigned index = 0;
		unsigned bytesRead = 0;
		do {
			bytesRead = subprocess_read_stdout( &subprocess, &buffer[0], buffer.size() );
			index += bytesRead;
			data += buffer.substr( 0, bytesRead );
		} while ( bytesRead != 0 );

		int ret;
		subprocess_join( &subprocess, &ret );
		subprocess_destroy( &subprocess );

		// Log::info( "Linter result:\n%s", data.c_str() );

		LuaPattern pattern( linter.warningPattern );
		std::map<Int64, LinterMatch> matches;
		for ( auto& match : pattern.gmatch( data ) ) {
			LinterMatch linterMatch;
			std::string lineStr = match.group( linter.warningPatternOrder.line );
			std::string colStr = linter.warningPatternOrder.col >= 0
									 ? match.group( linter.warningPatternOrder.col )
									 : "";
			linterMatch.text = match.group( linter.warningPatternOrder.message );

			if ( linter.warningPatternOrder.type >= 0 ) {
				std::string type( match.group( linter.warningPatternOrder.type ) );
				String::toLowerInPlace( type );
				if ( String::startsWith( type, "warn" ) ) {
					linterMatch.type = LinterType::Warning;
				} else if ( String::startsWith( type, "notice" ) ) {
					linterMatch.type = LinterType::Notice;
				}
			}

			Int64 line;
			Int64 col = 1;
			if ( !linterMatch.text.empty() && !lineStr.empty() &&
				 String::fromString( line, lineStr ) ) {
				if ( !colStr.empty() )
					String::fromString( col, colStr );
				linterMatch.pos = { line - 1, col > 0 ? col - 1 : 0 };
				linterMatch.lineCache = doc->line( line - 1 ).getHash();
				matches.insert( { line - 1, std::move( linterMatch ) } );
			}
		}

		{
			Lock matchesLock( mMatchesMutex );
			mMatches[doc] = matches;
		}

		invalidateEditors( doc );

		Log::info( "LinterModule::runLinter for %s took %.2fms", path.c_str(),
				   clock.getElapsedTime().asMilliseconds() );
	}
}

void LinterModule::drawAfterLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
									  const Float& /*fontSize*/, const Float& lineHeight ) {
	mMatchesMutex.lock();
	auto matchIt = mMatches.find( editor->getDocumentRef().get() );
	if ( matchIt == mMatches.end() ) {
		mMatchesMutex.unlock();
		return;
	}
	mMatchesMutex.unlock();

	std::map<Int64, LinterMatch>& map = matchIt->second;
	auto lineIt = map.find( index );
	if ( lineIt == map.end() )
		return;
	TextDocument* doc = matchIt->first;
	LinterMatch& match = lineIt->second;
	if ( match.lineCache != doc->line( index ).getHash() )
		return;
	Text line( "", editor->getFont(), editor->getFontSize() );
	line.setTabWidth( editor->getTabWidth() );
	line.setStyleConfig( editor->getFontStyleConfig() );
	line.setColor( editor->getColorScheme()
					   .getEditorSyntaxStyle( match.type == LinterType::Warning ||
													  match.type == LinterType::Notice
												  ? "warning"
												  : "error" )
					   .color );
	const String& text = doc->line( index ).getText();
	size_t minCol = text.find_first_not_of( " \t\f\v\n\r", match.pos.column() );
	if ( minCol == String::InvalidPos )
		minCol = match.pos.column();
	minCol = std::max( (Int64)minCol, match.pos.column() );
	if ( minCol >= text.size() )
		minCol = match.pos.column();
	if ( minCol >= text.size() )
		minCol = text.size() - 1;

	std::string str( text.substr( minCol ).size() - 1, '~' );
	String string( str );
	line.setString( string );

	Vector2f pos( position.x + editor->getXOffsetCol( { match.pos.line(), (Int64)minCol } ),
				  position.y );
	Rectf box( pos - editor->getScreenPos(), { editor->getTextWidth( string ), lineHeight } );
	match.box = box;
	line.draw( pos.x, pos.y + lineHeight * 0.5f );
}

bool LinterModule::onMouseMove( UICodeEditor* editor, const Vector2i& pos, const Uint32& ) {
	auto it = mMatches.find( editor->getDocumentRef().get() );
	if ( it != mMatches.end() ) {
		Vector2f localPos( editor->convertToNodeSpace( pos.asFloat() ) );
		for ( const auto& matchIt : it->second ) {
			auto& match = matchIt.second;
			if ( match.box.contains( localPos ) ) {
				editor->setTooltipText( match.text );
				editor->getTooltip()->setPixelsPosition( Vector2f( pos.x, pos.y ) );
				editor->runOnMainThread( [&, editor] { editor->getTooltip()->show(); } );
				return false;
			} else if ( editor->getTooltip() && editor->getTooltip()->isVisible() ) {
				editor->setTooltipText( "" );
				editor->getTooltip()->hide();
			}
		}
	}
	return false;
}

bool LinterModule::onMouseLeave( UICodeEditor* editor, const Vector2i&, const Uint32& ) {
	if ( editor->getTooltip() && editor->getTooltip()->isVisible() ) {
		editor->setTooltipText( "" );
		editor->getTooltip()->hide();
	}
	return false;
}

Linter LinterModule::supportsLinter( TextDocument* doc ) {
	std::string filePath( doc->getFilePath() );
	std::string extension( FileSystem::fileExtension( filePath ) );
	if ( extension.empty() ) {
		extension = FileSystem::fileNameFromPath( filePath );
	}
	const auto& def = doc->getSyntaxDefinition();

	for ( auto& linter : mLinters ) {
		for ( auto& ext : linter.files ) {
			auto& files = def.getFiles();
			if ( std::find( files.begin(), files.end(), ext ) != files.end() ) {
				return linter;
			}
		}
	}

	return {};
}

void LinterModule::setDocDirty( TextDocument* doc ) {
	mDirtyDoc[doc] = std::make_unique<Clock>();
}
void LinterModule::setDocDirty( UICodeEditor* editor ) {
	mDirtyDoc[editor->getDocumentRef().get()] = std::make_unique<Clock>();
}

void LinterModule::invalidateEditors( TextDocument* doc ) {
	Lock l( mDocMutex );
	for ( auto& it : mEditorDocs ) {
		if ( it.second == doc )
			it.first->invalidateDraw();
	}
}
