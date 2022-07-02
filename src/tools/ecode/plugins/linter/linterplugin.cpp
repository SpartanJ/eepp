#include "linterplugin.hpp"
#include "../../scopedop.hpp"
#include "../../thirdparty/json.hpp"
#include "../../thirdparty/subprocess.h"
#include <algorithm>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <random>

using json = nlohmann::json;

namespace ecode {

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define LINTER_THREADED 1
#else
#define LINTER_THREADED 0
#endif

LinterPlugin::LinterPlugin( const std::string& lintersPath, std::shared_ptr<ThreadPool> pool ) :
	mPool( pool ) {
#if LINTER_THREADED
	mPool->run( [&, lintersPath] { load( lintersPath ); }, [] {} );
#else
	load( lintersPath );
#endif
}

LinterPlugin::~LinterPlugin() {
	mShuttingDown = true;

	if ( mWorkersCount != 0 ) {
		std::unique_lock<std::mutex> lock( mWorkMutex );
		mWorkerCondition.wait( lock, [&]() { return mWorkersCount <= 0; } );
	}

	for ( const auto& editor : mEditors ) {
		for ( auto listener : editor.second )
			editor.first->removeEventListener( listener );
		editor.first->unregisterPlugin( this );
	}
}

void LinterPlugin::load( const std::string& lintersPath ) {
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

			auto wp = obj["warning_pattern"];

			if ( wp.is_array() ) {
				for ( auto& warningPattern : wp )
					linter.warningPattern.push_back( warningPattern.get<std::string>() );
			} else {
				linter.warningPattern = { wp.get<std::string>() };
			}

			linter.command = obj["command"].get<std::string>();

			if ( obj.contains( "expected_exitcodes" ) ) {
				auto ee = obj["expected_exitcodes"];
				if ( ee.is_array() ) {
					for ( auto& number : ee )
						if ( number.is_number() )
							linter.expectedExitCodes.push_back( number.get<Int64>() );
				} else if ( ee.is_number() ) {
					linter.expectedExitCodes.push_back( ee.get<Int64>() );
				}
			}

			if ( obj.contains( "warning_pattern_order" ) ) {
				auto& wpo = obj["warning_pattern_order"];
				if ( wpo.contains( "line" ) && wpo["line"].is_number() )
					linter.warningPatternOrder.line = wpo["line"].get<int>();
				if ( wpo.contains( "col" ) && wpo["col"].is_number() )
					linter.warningPatternOrder.col = wpo["col"].get<int>();
				if ( wpo.contains( "message" ) && wpo["message"].is_number() )
					linter.warningPatternOrder.message = wpo["message"].get<int>();
				if ( wpo.contains( "type" ) && wpo["type"].is_number() )
					linter.warningPatternOrder.type = wpo["type"].get<int>();
			}

			if ( obj.contains( "column_starts_at_zero" ) )
				linter.columnsStartAtZero = obj["column_starts_at_zero"].get<bool>();

			if ( obj.contains( "deduplicate" ) )
				linter.deduplicate = obj["deduplicate"].get<bool>();

			if ( obj.contains( "use_tmp_folder" ) )
				linter.useTmpFolder = obj["use_tmp_folder"].get<bool>();

			if ( obj.contains( "no_errors_exit_code" ) &&
				 obj["no_errors_exit_code"].is_number_integer() ) {
				linter.hasNoErrorsExitCode = true;
				linter.noErrorsExitCode = obj["no_errors_exit_code"].get<int>();
			}

			mLinters.emplace_back( std::move( linter ) );
		}

		mReady = true;
	} catch ( json::exception& e ) {
		mReady = false;
		Log::error( "Parsing linter failed:\n%s", e.what() );
	}
}

void LinterPlugin::onRegister( UICodeEditor* editor ) {
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

void LinterPlugin::onUnregister( UICodeEditor* editor ) {
	if ( mShuttingDown )
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

void LinterPlugin::update( UICodeEditor* editor ) {
	std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
	auto it = mDirtyDoc.find( doc.get() );
	if ( it != mDirtyDoc.end() && it->second->getElapsedTime() >= mDelayTime ) {
		mDirtyDoc.erase( doc.get() );
#if LINTER_THREADED
		mPool->run( [&, doc] { lintDoc( doc ); }, [] {} );
#endif
	}
}

const Time& LinterPlugin::getDelayTime() const {
	return mDelayTime;
}

void LinterPlugin::setDelayTime( const Time& delayTime ) {
	mDelayTime = delayTime;
}

void LinterPlugin::lintDoc( std::shared_ptr<TextDocument> doc ) {
	ScopedOp op(
		[&]() {
			mWorkMutex.lock();
			mWorkersCount++;
		},
		[&]() {
			mWorkersCount--;
			mWorkMutex.unlock();
			mWorkerCondition.notify_all();
		} );
	if ( !mReady )
		return;
	auto linter = supportsLinter( doc );
	if ( linter.command.empty() )
		return;

	IOStreamString fileString;
	if ( doc->isDirty() || !doc->hasFilepath() ) {
		std::string tmpPath;
		if ( !doc->hasFilepath() ) {
			tmpPath =
				Sys::getTempPath() + ".ecode-" + doc->getFilename() + "." + String::randString( 8 );
		} else if ( linter.useTmpFolder ) {
			tmpPath = Sys::getTempPath() + doc->getFilename();
			if ( FileSystem::fileExists( tmpPath ) ) {
				tmpPath = Sys::getTempPath() + ".ecode-" + doc->getFilename() + "." +
						  String::randString( 8 );
			}
		} else {
			std::string fileDir( FileSystem::fileRemoveFileName( doc->getFilePath() ) );
			FileSystem::dirAddSlashAtEnd( fileDir );
			tmpPath = fileDir + "." + String::randString( 8 ) + "." + doc->getFilename();
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

void LinterPlugin::runLinter( std::shared_ptr<TextDocument> doc, const Linter& linter,
							  const std::string& path ) {
	Clock clock;
	std::string cmd( linter.command );
	String::replaceAll( cmd, "$FILENAME", "\"" + path + "\"" );
	std::vector<std::string> cmdArr = String::split( cmd, " " );
	std::vector<const char*> strings;
	for ( size_t i = 0; i < cmdArr.size(); ++i )
		strings.push_back( cmdArr[i].c_str() );
	strings.push_back( NULL );
	struct subprocess_s subprocess;
	int result = subprocess_create(
		strings.data(),
		subprocess_option_search_user_path | subprocess_option_inherit_environment |
			subprocess_option_combined_stdout_stderr | subprocess_option_no_window,
		&subprocess );
	if ( 0 == result ) {
		std::string buffer( 1024, '\0' );
		std::string data;
		unsigned bytesRead = 0;
		int returnCode;
		do {
			bytesRead = subprocess_read_stdout( &subprocess, &buffer[0], buffer.size() );
			data += buffer.substr( 0, bytesRead );
		} while ( bytesRead != 0 && subprocess_alive( &subprocess ) && !mShuttingDown );

		if ( subprocess_alive( &subprocess ) && !mShuttingDown )
			subprocess_join( &subprocess, &returnCode );
		subprocess_destroy( &subprocess );

		if ( linter.hasNoErrorsExitCode && linter.noErrorsExitCode == returnCode ) {
			Lock matchesLock( mMatchesMutex );
			mMatches[doc.get()] = {};
			return;
		}

		if ( !linter.expectedExitCodes.empty() &&
			 std::find( linter.expectedExitCodes.begin(), linter.expectedExitCodes.end(),
						returnCode ) == linter.expectedExitCodes.end() )
			return;

		// Log::info( "Linter result:\n%s", data.c_str() );

		std::map<Int64, std::vector<LinterMatch>> matches;
		size_t totalMatches = 0;
		size_t totalErrors = 0;
		size_t totalWarns = 0;
		size_t totalNotice = 0;

		for ( auto warningPatterm : linter.warningPattern ) {
			String::replaceAll( warningPatterm, "$FILENAME", path );
			LuaPattern pattern( warningPatterm );
			for ( auto& match : pattern.gmatch( data ) ) {
				LinterMatch linterMatch;
				std::string lineStr = match.group( linter.warningPatternOrder.line );
				std::string colStr = linter.warningPatternOrder.col >= 0
										 ? match.group( linter.warningPatternOrder.col )
										 : "";
				linterMatch.text = match.group( linter.warningPatternOrder.message );
				String::trimInPlace( linterMatch.text );
				String::trimInPlace( linterMatch.text, '\n' );

				if ( linter.warningPatternOrder.type >= 0 ) {
					std::string type( match.group( linter.warningPatternOrder.type ) );
					String::toLowerInPlace( type );
					if ( String::startsWith( type, "warn" ) ) {
						linterMatch.type = LinterType::Warning;
					} else if ( String::startsWith( type, "notice" ) ||
								String::startsWith( type, "hint" ) ) {
						linterMatch.type = LinterType::Notice;
					}
				}

				Int64 line;
				Int64 col = 1;
				if ( !linterMatch.text.empty() && !lineStr.empty() &&
					 String::fromString( line, lineStr ) ) {
					if ( !colStr.empty() ) {
						String::fromString( col, colStr );
						if ( linter.columnsStartAtZero )
							col++;
					}
					linterMatch.pos = { line - 1, col > 0 ? col - 1 : 0 };
					linterMatch.lineCache = doc->line( line - 1 ).getHash();
					bool skip = false;

					if ( linter.deduplicate && matches.find( line - 1 ) != matches.end() ) {
						for ( auto& match : matches[line - 1] ) {
							if ( match.pos == linterMatch.pos ) {
								match.text += "\n" + linterMatch.text;
								skip = true;
								break;
							}
						}
					}

					if ( !skip )
						matches[line - 1].emplace_back( std::move( linterMatch ) );
				}
			}
		}

		totalMatches = matches.size();
		for ( const auto& matchLine : matches ) {
			for ( const auto& match : matchLine.second ) {
				switch ( match.type ) {
					case LinterType::Warning:
						++totalWarns;
						break;
					case LinterType::Notice:
						++totalNotice;
						break;
					case LinterType::Error:
					default:
						++totalErrors;
						break;
				}
			}
		}

		{
			Lock matchesLock( mMatchesMutex );
			mMatches[doc.get()] = std::move( matches );
		}

		invalidateEditors( doc.get() );

		Log::info( "LinterPlugin::runLinter for %s took %.2fms. Found: %d matches. Errors: %d, "
				   "Warnings: %d, Notices: %d.",
				   path.c_str(), clock.getElapsedTime().asMilliseconds(), totalMatches, totalErrors,
				   totalWarns, totalNotice );
	}
}

std::string LinterPlugin::getMatchString( const LinterType& type ) {
	switch ( type ) {
		case LinterType::Warning:
			return "warning";
		case LinterType::Notice:
			return "notice";
		default:
			break;
	}
	return "error";
}

void LinterPlugin::drawAfterLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
									  const Float& /*fontSize*/, const Float& lineHeight ) {
	Lock l( mMatchesMutex );
	auto matchIt = mMatches.find( editor->getDocumentRef().get() );
	if ( matchIt == mMatches.end() )
		return;

	std::map<Int64, std::vector<LinterMatch>>& map = matchIt->second;
	auto lineIt = map.find( index );
	if ( lineIt == map.end() )
		return;
	TextDocument* doc = matchIt->first;
	std::vector<LinterMatch>& matches = lineIt->second;
	for ( auto& match : matches ) {
		if ( match.lineCache != doc->line( index ).getHash() )
			return;
		Text line( "", editor->getFont(), editor->getFontSize() );
		line.setTabWidth( editor->getTabWidth() );
		line.setStyleConfig( editor->getFontStyleConfig() );
		line.setColor(
			editor->getColorScheme().getEditorSyntaxStyle( getMatchString( match.type ) ).color );
		const String& text = doc->line( index ).getText();
		size_t minCol = text.find_first_not_of( " \t\f\v\n\r", match.pos.column() );
		if ( minCol == String::InvalidPos )
			minCol = match.pos.column();
		minCol = std::max( (Int64)minCol, match.pos.column() );
		if ( minCol >= text.size() )
			minCol = match.pos.column();
		if ( minCol >= text.size() )
			minCol = text.size() - 1;

		Int64 strSize = 0;
		TextPosition endPos;
		Vector2f pos;

		if ( minCol < text.size() - 1 ) {
			endPos = doc->nextWordBoundary( { match.pos.line(), (Int64)minCol } );
			strSize = eemax( (Int64)0, static_cast<Int64>( endPos.column() - minCol ) );
			pos = { position.x + editor->getXOffsetCol( { match.pos.line(), (Int64)minCol } ),
					position.y };
		} else {
			endPos = doc->previousWordBoundary( { match.pos.line(), (Int64)minCol } );
			strSize = eemax( (Int64)0, static_cast<Int64>( minCol - endPos.column() ) );
			pos = { position.x +
						editor->getXOffsetCol( { match.pos.line(), (Int64)endPos.column() } ),
					position.y };
		}

		if ( strSize == 0 ) {
			strSize = 1;
			pos = { position.x, position.y };
		}

		std::string str( strSize, '~' );
		String string( str );
		line.setString( string );
		Rectf box( pos - editor->getScreenPos(), { editor->getTextWidth( string ), lineHeight } );
		match.box = box;
		line.draw( pos.x, pos.y + lineHeight * 0.5f );
	}
}

bool LinterPlugin::onMouseMove( UICodeEditor* editor, const Vector2i& pos, const Uint32& ) {
	Lock l( mMatchesMutex );
	auto it = mMatches.find( editor->getDocumentRef().get() );
	if ( it != mMatches.end() ) {
		Vector2f localPos( editor->convertToNodeSpace( pos.asFloat() ) );
		for ( const auto& matchIt : it->second ) {
			auto& matches = matchIt.second;
			for ( const auto& match : matches ) {
				if ( match.box.contains( localPos ) ) {
					editor->setTooltipText( match.text );
					editor->getTooltip()->setDontAutoHideOnMouseMove( true );
					editor->getTooltip()->setPixelsPosition( Vector2f( pos.x, pos.y ) );
					if ( !editor->getTooltip()->isVisible() )
						editor->runOnMainThread( [&, editor] { editor->getTooltip()->show(); } );
					return false;
				}
			}
		}
		if ( editor->getTooltip() && editor->getTooltip()->isVisible() ) {
			editor->setTooltipText( "" );
			editor->getTooltip()->hide();
		}
	}
	return false;
}

bool LinterPlugin::onMouseLeave( UICodeEditor* editor, const Vector2i&, const Uint32& ) {
	if ( editor->getTooltip() && editor->getTooltip()->isVisible() ) {
		editor->setTooltipText( "" );
		editor->getTooltip()->hide();
	}
	return false;
}

Linter LinterPlugin::supportsLinter( std::shared_ptr<TextDocument> doc ) {
	std::string fileName( FileSystem::fileNameFromPath( doc->getFilePath() ) );
	const auto& def = doc->getSyntaxDefinition();

	for ( auto& linter : mLinters ) {
		for ( auto& ext : linter.files ) {
			if ( LuaPattern::find( fileName, ext ).isValid() )
				return linter;
			auto& files = def.getFiles();
			if ( std::find( files.begin(), files.end(), ext ) != files.end() ) {
				return linter;
			}
		}
	}

	return {};
}

void LinterPlugin::setDocDirty( TextDocument* doc ) {
	mDirtyDoc[doc] = std::make_unique<Clock>();
}
void LinterPlugin::setDocDirty( UICodeEditor* editor ) {
	mDirtyDoc[editor->getDocumentRef().get()] = std::make_unique<Clock>();
}

void LinterPlugin::invalidateEditors( TextDocument* doc ) {
	Lock l( mDocMutex );
	for ( auto& it : mEditorDocs ) {
		if ( it.second == doc )
			it.first->invalidateDraw();
	}
}

} // namespace ecode
