#include "linterplugin.hpp"
#include "../../scopedop.hpp"
#include <algorithm>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/process.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <nlohmann/json.hpp>
#include <random>

using json = nlohmann::json;

namespace ecode {

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define LINTER_THREADED 1
#else
#define LINTER_THREADED 0
#endif

UICodeEditorPlugin* LinterPlugin::New( const PluginManager* pluginManager ) {
	return eeNew( LinterPlugin, ( pluginManager ) );
}

LinterPlugin::LinterPlugin( const PluginManager* pluginManager ) :
	mManager( pluginManager ), mPool( pluginManager->getThreadPool() ) {
#if LINTER_THREADED
	mPool->run( [&, pluginManager] { load( pluginManager ); }, [] {} );
#else
	load( pluginManager );
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

bool LinterPlugin::hasFileConfig() {
	return !mConfigPath.empty();
}

std::string LinterPlugin::getFileConfigPath() {
	return mConfigPath;
}

size_t LinterPlugin::linterFilePatternPosition( const std::vector<std::string>& patterns ) {
	for ( size_t i = 0; i < mLinters.size(); ++i ) {
		for ( const std::string& filePattern : mLinters[i].files ) {
			for ( const std::string& pattern : patterns ) {
				if ( filePattern == pattern ) {
					return i;
				}
			}
		}
	}
	return std::string::npos;
}

void LinterPlugin::loadLinterConfig( const std::string& path ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "LinterPlugin::loadLinterConfig - Error parsing linter config from "
					"path %s, error: ",
					path.c_str(), e.what() );
		return;
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "delay_time" ) )
			setDelayTime( Time::fromString( config["delay_time"].get<std::string>() ) );
		if ( config.contains( "enable_lsp_diagnostics" ) &&
			 config["enable_lsp_diagnostics"].is_boolean() )
			setEnableLSPDiagnostics( config["enable_lsp_diagnostics"].get<bool>() );
		if ( config.contains( "disable_lsp_languages" ) &&
			 config["disable_lsp_languages"].is_array() ) {
			const auto& langs = config["disable_lsp_languages"];
			try {
				mLSPLanguagesDisabled.clear();
				for ( const auto& lang : langs ) {
					if ( lang.is_string() ) {
						std::string lg = lang.get<std::string>();
						if ( !lg.empty() )
							mLSPLanguagesDisabled.insert( lg );
					}
				}
			} catch ( const json::exception& e ) {
				Log::debug(
					"LinterPlugin::loadLinterConfig: Error parsing disable_lsp_languages: %s",
					e.what() );
			}
		}
	}

	if ( !j.contains( "linters" ) )
		return;

	auto& linters = j["linters"];
	for ( auto& obj : linters ) {
		if ( !obj.contains( "file_patterns" ) || !obj.contains( "warning_pattern" ) ||
			 !obj.contains( "command" ) )
			continue;

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

		// If the file pattern is repeated, we will overwrite the previous linter.
		// The previous linter should be the "default" linter that comes with ecode.
		size_t pos = linterFilePatternPosition( linter.files );
		if ( pos != std::string::npos ) {
			mLinters[pos] = linter;
		} else {
			mLinters.emplace_back( std::move( linter ) );
		}
	}
}

LinterType getLinterTypeFromSeverity( const LSPDiagnosticSeverity& severity ) {
	switch ( severity ) {
		case LSPDiagnosticSeverity::Error:
			return LinterType::Error;
		case LSPDiagnosticSeverity::Warning:
			return LinterType::Warning;
		default:
			return LinterType::Notice;
	}
}

void LinterPlugin::eraseMatchesFromOrigin( TextDocument* doc, const MatchOrigin& origin ) {
	Lock matchesLock( mMatchesMutex );
	auto& docMatches = mMatches[doc];

	std::vector<Int64> emptyMatchLines;
	for ( auto& matchLine : docMatches ) {
		bool found;
		do {
			found = false;
			auto it = matchLine.second.begin();
			for ( ; it != matchLine.second.end(); ++it ) {
				if ( it->origin == origin ) {
					found = true;
					break;
				}
			}
			if ( found ) {
				matchLine.second.erase( it );
				if ( matchLine.second.empty() )
					emptyMatchLines.push_back( matchLine.first );
			}
		} while ( found );
	}

	for ( const auto& line : emptyMatchLines )
		docMatches.erase( line );
}

void LinterPlugin::insertMatches( TextDocument* doc,
								  std::map<Int64, std::vector<LinterMatch>>& matches ) {
	Lock matchesLock( mMatchesMutex );
	auto& docMatches = mMatches[doc];
	for ( auto& match : matches ) {
		std::vector<LinterMatch>& vec = docMatches[match.first];
		vec.insert( vec.end(), match.second.begin(), match.second.end() );
	}
}

void LinterPlugin::setMatches( TextDocument* doc, const MatchOrigin& origin,
							   std::map<Int64, std::vector<LinterMatch>>& matches ) {
	if ( !mEnableLSPDiagnostics ) {
		Lock matchesLock( mMatchesMutex );
		mMatches[doc] = std::move( matches );
	} else {
		eraseMatchesFromOrigin( doc, origin );
		insertMatches( doc, matches );
	}

	invalidateEditors( doc );
}

PluginRequestHandle LinterPlugin::processMessage( const PluginMessage& notification ) {
	if ( !mEnableLSPDiagnostics || notification.type != PluginMessageType::Diagnostics ||
		 notification.format != PluginMessageFormat::Diagnostics )
		return PluginRequestHandle::empty();
	const auto& diags = notification.asDiagnostics();
	TextDocument* doc = getDocumentFromURI( diags.uri );
	if ( doc == nullptr ||
		 mLSPLanguagesDisabled.find( String::toLower( doc->getSyntaxDefinition().getLSPName() ) ) !=
			 mLSPLanguagesDisabled.end() )
		return PluginRequestHandle::empty();

	std::map<Int64, std::vector<LinterMatch>> matches;

	for ( const auto& diag : diags.diagnostics ) {
		LinterMatch match;
		match.range = diag.range;
		match.text = diag.message;
		match.type = getLinterTypeFromSeverity( diag.severity );
		match.lineCache = doc->line( match.range.start().line() ).getHash();
		match.origin = MatchOrigin::Diagnostics;
		matches[match.range.start().line()].emplace_back( std::move( match ) );
	}

	setMatches( doc, MatchOrigin::Diagnostics, matches );
	return PluginRequestHandle::empty();
}

TextDocument* LinterPlugin::getDocumentFromURI( const URI& uri ) {
	for ( TextDocument* doc : mDocs ) {
		if ( doc->getURI() == uri )
			return doc;
	}
	return nullptr;
}

void LinterPlugin::load( const PluginManager* pluginManager ) {
	pluginManager->subscribeMessages( this, [&]( const auto& notification ) -> PluginRequestHandle {
		return processMessage( notification );
	} );
	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/linters.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "linters.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite( path, "{\n\"config\":{},\n\"linters\":[]\n}\n" ) ) {
		mConfigPath = path;
		paths.emplace_back( path );
	}
	if ( paths.empty() )
		return;
	for ( const auto& path : paths ) {
		try {
			loadLinterConfig( path );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing linter \"%s\" failed:\n%s", path.c_str(), e.what() );
		}
	}
	mReady = !mLinters.empty();
	if ( mReady )
		fireReadyCbs();
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
#else
		lintDoc( doc );
#endif
	}
}

const Time& LinterPlugin::getDelayTime() const {
	return mDelayTime;
}

void LinterPlugin::setDelayTime( const Time& delayTime ) {
	mDelayTime = delayTime;
}

bool LinterPlugin::getEnableLSPDiagnostics() const {
	return mEnableLSPDiagnostics;
}

void LinterPlugin::setEnableLSPDiagnostics( bool enableLSPDiagnostics ) {
	mEnableLSPDiagnostics = enableLSPDiagnostics;
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
	Process process;
	if ( process.create( cmd, Process::getDefaultOptions() | Process::CombinedStdoutStderr, {},
						 mManager->getWorkspaceFolder() ) ) {
		std::string buffer( 1024, '\0' );
		std::string data;
		unsigned bytesRead = 0;
		int returnCode;
		do {
			bytesRead = process.readStdOut( buffer );
			data += buffer.substr( 0, bytesRead );
		} while ( bytesRead != 0 && process.isAlive() && !mShuttingDown );

		if ( mShuttingDown ) {
			process.kill();
			return;
		}

		process.join( &returnCode );
		process.destroy();

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

					linterMatch.range.setStart(
						{ line > 0 ? line - 1 : 0, col > 0 ? col - 1 : 0 } );

					const String& text = doc->line( linterMatch.range.start().line() ).getText();
					size_t minCol =
						text.find_first_not_of( " \t\f\v\n\r", linterMatch.range.start().column() );
					if ( minCol == String::InvalidPos )
						minCol = linterMatch.range.start().column();
					minCol = std::max( (Int64)minCol, linterMatch.range.start().column() );
					if ( minCol >= text.size() )
						minCol = linterMatch.range.start().column();
					if ( minCol >= text.size() )
						minCol = text.size() - 1;
					linterMatch.range.setStart(
						{ linterMatch.range.start().line(), (Int64)minCol } );
					TextPosition endPos;
					endPos = ( minCol < text.size() - 1 )
								 ? doc->nextWordBoundary(
									   { linterMatch.range.start().line(), (Int64)minCol } )
								 : doc->previousWordBoundary(
									   { linterMatch.range.start().line(), (Int64)minCol } );

					linterMatch.range.setEnd( endPos );
					linterMatch.range = linterMatch.range.normalized();
					linterMatch.lineCache = doc->line( linterMatch.range.start().line() ).getHash();
					bool skip = false;

					if ( linter.deduplicate && matches.find( line - 1 ) != matches.end() ) {
						for ( auto& match : matches[line - 1] ) {
							if ( match.range == linterMatch.range ) {
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

		setMatches( doc.get(), MatchOrigin::Linter, matches );

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

// TODO: Implement error lens
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

	for ( size_t i = 0; i < matches.size(); ++i ) {
		auto& match = matches[i];

		bool isDuplicate = false;
		if ( i > 1 ) {
			for ( size_t p = 0; p < i; ++p ) {
				if ( matches[p].range == match.range ) {
					isDuplicate = true;
					break;
				}
			}
		}

		if ( isDuplicate )
			continue;

		if ( match.lineCache != doc->line( index ).getHash() )
			return;

		Text line( "", editor->getFont(), editor->getFontSize() );
		line.setTabWidth( editor->getTabWidth() );
		line.setStyleConfig( editor->getFontStyleConfig() );
		line.setColor(
			editor->getColorScheme().getEditorSyntaxStyle( getMatchString( match.type ) ).color );

		Int64 strSize = match.range.end().column() - match.range.start().column();
		Vector2f pos = { position.x + editor->getXOffsetCol( match.range.start() ), position.y };
		if ( strSize == 0 ) {
			strSize = 1;
			pos = { position.x, position.y };
		}

		std::string str( strSize, '~' );
		String string( str );
		line.setString( string );
		Rectf box( pos - editor->getScreenPos(), { editor->getTextWidth( string ), lineHeight } );
		match.box[editor] = box;
		line.draw( pos.x, pos.y + lineHeight * 0.5f );
	}
}

void LinterPlugin::minimapDrawBeforeLineText( UICodeEditor* editor, const Int64& index,
											  const Vector2f& pos, const Vector2f& size,
											  const Float&, const Float& ) {
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
	Primitives p;
	for ( auto& match : matches ) {
		if ( match.lineCache != doc->line( index ).getHash() )
			return;
		Color col(
			editor->getColorScheme().getEditorSyntaxStyle( getMatchString( match.type ) ).color );
		col.blendAlpha( 100 );
		p.setColor( col );
		p.drawRectangle( Rectf( pos, size ) );
		break;
	}
}

bool LinterPlugin::onMouseMove( UICodeEditor* editor, const Vector2i& pos, const Uint32& ) {
	Lock l( mMatchesMutex );
	auto it = mMatches.find( editor->getDocumentRef().get() );
	if ( it != mMatches.end() ) {
		Vector2f localPos( editor->convertToNodeSpace( pos.asFloat() ) );
		TextPosition cursorPosition = editor->resolveScreenPosition( pos.asFloat() );
		auto matchIt = it->second.find( cursorPosition.line() );
		if ( matchIt != it->second.end() ) {
			auto& matches = matchIt->second;
			for ( auto& match : matches ) {
				if ( match.box[editor].contains( localPos ) ) {
					mHoveringMatch = true;
					editor->runOnMainThread( [&, editor] {
						editor->setTooltipText( match.text );
						editor->getTooltip()->setDontAutoHideOnMouseMove( true );
						editor->getTooltip()->setPixelsPosition( Vector2f( pos.x, pos.y ) );
						if ( !editor->getTooltip()->isVisible() )
							editor->getTooltip()->show();
					} );
					return true;
				}
			}
		}
		if ( mHoveringMatch && editor->getTooltip() && editor->getTooltip()->isVisible() ) {
			editor->setTooltipText( "" );
			editor->getTooltip()->hide();
			mHoveringMatch = false;
		}
	}
	return false;
}

bool LinterPlugin::onMouseLeave( UICodeEditor* editor, const Vector2i&, const Uint32& ) {
	if ( mHoveringMatch && editor->getTooltip() && editor->getTooltip()->isVisible() ) {
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
