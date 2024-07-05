#include "linterplugin.hpp"
#include "../../stringhelper.hpp"
#include <algorithm>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/window.hpp>
#include <nlohmann/json.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using json = nlohmann::json;

namespace ecode {

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define LINTER_THREADED 1
#else
#define LINTER_THREADED 0
#endif

Plugin* LinterPlugin::New( PluginManager* pluginManager ) {
	return eeNew( LinterPlugin, ( pluginManager, false ) );
}

Plugin* LinterPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( LinterPlugin, ( pluginManager, true ) );
}

LinterPlugin::LinterPlugin( PluginManager* pluginManager, bool sync ) : Plugin( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
#if defined( LINTER_THREADED ) && LINTER_THREADED == 1
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
#else
		load( pluginManager );
#endif
	}
}

LinterPlugin::~LinterPlugin() {
	mShuttingDown = true;
	mManager->unsubscribeMessages( this );
	unsubscribeFileSystemListener();

	if ( mWorkersCount != 0 ) {
		std::unique_lock<std::mutex> lock( mWorkMutex );
		mWorkerCondition.wait( lock, [this]() { return mWorkersCount <= 0; } );
	}

	for ( const auto& editor : mEditors ) {
		for ( auto& kb : mKeyBindings ) {
			editor.first->getKeyBindings().removeCommandKeybind( kb.first );
			if ( editor.first->hasDocument() )
				editor.first->getDocument().removeCommand( kb.first );
		}
		for ( auto listener : editor.second )
			editor.first->removeEventListener( listener );
		editor.first->unregisterPlugin( this );
	}
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

void LinterPlugin::loadLinterConfig( const std::string& path, bool updateConfigFile ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "LinterPlugin::loadLinterConfig - Error parsing linter config from "
					"path %s, error: %s, config file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		if ( !updateConfigFile )
			return;
		// Recreate it
		j = json::parse( "{\n\"config\":{},\n  \"keybindings\":{},\n\"linters\":[]\n}\n", nullptr,
						 true, true );
	}

	if ( updateConfigFile ) {
		mConfigHash = String::hash( data );
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "delay_time" ) )
			setDelayTime( Time::fromString( config["delay_time"].get<std::string>() ) );
		else if ( updateConfigFile )
			config["delay_time"] = getDelayTime().toString();

		if ( config.contains( "enable_lsp_diagnostics" ) &&
			 config["enable_lsp_diagnostics"].is_boolean() )
			setEnableLSPDiagnostics( config["enable_lsp_diagnostics"].get<bool>() );
		else if ( updateConfigFile )
			config["enable_lsp_diagnostics"] = getEnableLSPDiagnostics();

		if ( config.contains( "enable_error_lens" ) && config["enable_error_lens"].is_boolean() )
			setErrorLens( config["enable_error_lens"].get<bool>() );
		else if ( updateConfigFile )
			config["enable_error_lens"] = getErrorLens();

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
		} else if ( updateConfigFile ) {
			config["disable_lsp_languages"] = json::array();
		}

		if ( config.contains( "disable_languages" ) && config["disable_languages"].is_array() ) {
			const auto& langs = config["disable_languages"];
			try {
				mLanguagesDisabled.clear();
				for ( const auto& lang : langs ) {
					if ( lang.is_string() ) {
						std::string lg = lang.get<std::string>();
						if ( !lg.empty() )
							mLanguagesDisabled.insert( lg );
					}
				}
			} catch ( const json::exception& e ) {
				Log::debug( "LinterPlugin::loadLinterConfig: Error parsing disable_languages: %s",
							e.what() );
			}
		} else if ( updateConfigFile ) {
			config["disable_languages"] = json::array();
		}

		if ( config.contains( "go-to-ignore-warnings" ) ) {
			config["goto_ignore_warnings"] = config["go-to-ignore-warnings"];
			config.erase( "go-to-ignore-warnings" );
		}

		if ( config.contains( "goto_ignore_warnings" ) &&
			 config["goto_ignore_warnings"].is_boolean() ) {
			mGoToIgnoreWarnings = config["goto_ignore_warnings"].get<bool>();
		} else if ( updateConfigFile ) {
			config["goto_ignore_warnings"] = false;
		}
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["linter-go-to-next-error"] = "mod+shift+n";
		mKeyBindings["linter-go-to-previous-error"] = "mod+shift+alt+n";
	}

	auto& kb = j["keybindings"];
	auto list = { "linter-go-to-next-error", "linter-go-to-previous-error" };
	for ( const auto& key : list ) {
		if ( kb.contains( key ) ) {
			if ( !kb[key].empty() )
				mKeyBindings[key] = kb[key];
		} else if ( updateConfigFile )
			kb[key] = mKeyBindings[key];
	}

	if ( updateConfigFile ) {
		std::string newData( j.dump( 2 ) );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
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

		if ( obj.contains( "language" ) ) {
			if ( obj["language"].is_array() ) {
				const auto& langs = obj["language"];
				for ( const auto& lang : langs ) {
					if ( lang.is_string() )
						linter.languages.push_back( lang.get<std::string>() );
				}
			} else if ( obj["language"].is_string() ) {
				linter.languages.push_back( obj["language"].get<std::string>() );
			}
		}

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
		linter.url = obj.value( "url", "" );

		if ( obj.contains( "type" ) ) {
			std::string typeStr( obj["type"].get<std::string>() );
			String::toLowerInPlace( typeStr );
			String::trimInPlace( typeStr );
			if ( "native" == typeStr ) {
				linter.isNative = true;
				if ( mNativeLinters.find( linter.command ) == mNativeLinters.end() ) {
					Log::error( "Requested native linter: '%s' does not exists.",
								linter.command.c_str() );
					continue;
				}
			}
		}

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

// TODO: Clean up this
static json toJson( const TextPosition& pos ) {
	return json{ { "line", pos.line() }, { "character", pos.column() } };
}

static json toJson( const TextRange& pos ) {
	return json{ { "start", toJson( pos.start() ) }, { "end", toJson( pos.end() ) } };
}

static json toJson( const LSPLocation& location ) {
	if ( !location.uri.empty() ) {
		return json{ { "uri", location.uri.toString() }, { "range", toJson( location.range ) } };
	}
	return json();
}

static json toJson( const LSPDiagnosticRelatedInformation& related ) {
	auto loc = toJson( related.location );
	if ( loc.is_object() ) {
		return json{ { "location", toJson( related.location ) }, { "message", related.message } };
	}
	return json();
}

static json toJson( const LSPDiagnostic& diagnostic ) {
	// required
	auto result = json();
	result["range"] = toJson( diagnostic.range );
	result["message"] = diagnostic.message;
	// optional
	if ( !diagnostic.code.empty() )
		result["code"] = diagnostic.code;
	if ( diagnostic.severity != LSPDiagnosticSeverity::Unknown )
		result["severity"] = static_cast<int>( diagnostic.severity );
	if ( !diagnostic.source.empty() )
		result["source"] = diagnostic.source;
	json relatedInfo;
	for ( const auto& vrelated : diagnostic.relatedInformation ) {
		auto related = toJson( vrelated );
		if ( related.is_object() ) {
			relatedInfo.push_back( related );
		}
	}
	if ( !relatedInfo.empty() )
		result["relatedInformation"] = relatedInfo;
	if ( !diagnostic.data.empty() )
		result["data"] = diagnostic.data;
	return result;
}

PluginRequestHandle LinterPlugin::processMessage( const PluginMessage& notification ) {
	if ( notification.type == PluginMessageType::FileSystemListenerReady ) {
		subscribeFileSystemListener();
		return {};
	}

	if ( notification.type == PluginMessageType::DiagnosticsCodeAction &&
		 notification.format == PluginMessageFormat::JSON && notification.isRequest() ) {
		PluginIDType id( std::numeric_limits<Int64>::max() );
		TextDocument* doc = getDocumentFromURI( notification.asJSON().value( "uri", "" ) );
		if ( doc ) {
			Lock l( mMatchesMutex );
			auto foundMatch = mMatches.find( doc );
			if ( foundMatch == mMatches.end() )
				return {};

			auto pos = TextPosition::fromString( notification.asJSON().value( "pos", "" ) );
			if ( !pos.isValid() )
				return {};

			auto foundLine = foundMatch->second.find( pos.line() );
			if ( foundLine == foundMatch->second.end() )
				return {};

			LSPDiagnosticsCodeAction quickFix;
			for ( const auto& match : foundLine->second ) {
				if ( !match.diagnostic.codeActions.empty() ) {
					for ( const auto& ca : match.diagnostic.codeActions ) {
						quickFix = ca;
						if ( quickFix.isPreferred )
							break;
					}
				}
			}

			mManager->sendResponse( this, PluginMessageType::DiagnosticsCodeAction,
									PluginMessageFormat::DiagnosticsCodeAction, &quickFix, id );
		}

		return {};
	}

	if ( notification.type == PluginMessageType::GetErrorOrWarning &&
		 notification.format == PluginMessageFormat::JSON && notification.isRequest() ) {
		const json& j = notification.asJSON();
		if ( !j.contains( "uri" ) || !j.contains( "line" ) || !j.contains( "character" ) )
			return {};
		URI uri( j["uri"].get<std::string>() );
		TextDocument* doc = getDocumentFromURI( uri );
		if ( nullptr == doc )
			return {};

		Lock l( mMatchesMutex );
		auto found = mMatches.find( doc );
		if ( found == mMatches.end() )
			return {};

		TextPosition pos( LSPConverter::parsePosition( j ) );

		const auto& docMatches = found->second;

		auto foundLine = docMatches.find( pos.line() );
		if ( foundLine == docMatches.end() )
			return {};

		const auto& matches = foundLine->second;

		for ( const auto& match : matches ) {
			if ( pos.column() >= match.range.start().column() &&
				 pos.column() <= match.range.end().column() ) {
				PluginInmediateResponse msg;
				msg.type = PluginMessageType::GetErrorOrWarning;
				json rj;
				rj["text"] = match.text;
				rj["type"] = match.type == LinterType::Error ? "error" : "warning";
				rj["range"] = match.range.toString();
				msg.data = std::move( rj );
				return PluginRequestHandle( msg );
			}
		}

		return {};
	}

	if ( notification.type == PluginMessageType::GetDiagnostics &&
		 notification.format == PluginMessageFormat::JSON && notification.isRequest() ) {
		const json& j = notification.asJSON();
		if ( !j.contains( "uri" ) || !j.contains( "line" ) || !j.contains( "character" ) )
			return {};
		URI uri( j["uri"].get<std::string>() );
		TextDocument* doc = getDocumentFromURI( uri );
		if ( nullptr == doc )
			return {};

		Lock l( mMatchesMutex );
		auto found = mMatches.find( doc );
		if ( found == mMatches.end() )
			return {};

		TextPosition pos( LSPConverter::parsePosition( j ) );

		const auto& docMatches = found->second;

		auto foundLine = docMatches.find( pos.line() );
		if ( foundLine == docMatches.end() )
			return {};

		const auto& matches = foundLine->second;

		for ( const auto& match : matches ) {
			if ( pos.column() >= match.range.start().column() &&
				 pos.column() <= match.range.end().column() ) {
				PluginInmediateResponse msg;
				msg.type = PluginMessageType::GetDiagnostics;
				json rj;
				rj["diagnostics"] = json::array( { toJson( match.diagnostic ) } );
				msg.data = std::move( rj );
				return PluginRequestHandle( msg );
			}
		}

		return {};
	}

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
		match.diagnostic = std::move( diag );
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

void LinterPlugin::load( PluginManager* pluginManager ) {
	AtomicBoolScopedOp loading( mLoading, true );
	pluginManager->subscribeMessages( this,
									  [this]( const auto& notification ) -> PluginRequestHandle {
										  return processMessage( notification );
									  } );
	registerNativeLinters();

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
	for ( const auto& tpath : paths ) {
		try {
			loadLinterConfig( tpath, mConfigPath == tpath );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing linter \"%s\" failed:\n%s", tpath.c_str(), e.what() );
		}
	}

	subscribeFileSystemListener();
	mReady = !mLinters.empty();
	if ( mReady ) {
		fireReadyCbs();
		setReady();
	}
}

void LinterPlugin::onRegister( UICodeEditor* editor ) {
	Lock l( mDocMutex );

	std::vector<Uint32> listeners;

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [this, editor]( const Event* event ) {
			Lock l( mDocMutex );
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			mDocs.insert( docEvent->getDoc() );
			mEditorDocs[editor] = docEvent->getDoc();
			setDocDirty( docEvent->getDoc() );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentClosed, [this]( const Event* event ) {
			Lock l( mDocMutex );
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			TextDocument* doc = docEvent->getDoc();
			mDocs.erase( doc );
			mDirtyDoc.erase( doc );
			Lock matchesLock( mMatchesMutex );
			mMatches.erase( doc );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentChanged, [this, editor]( const Event* ) {
			TextDocument* oldDoc = mEditorDocs[editor];
			TextDocument* newDoc = editor->getDocumentRef().get();
			Lock l( mDocMutex );
			mDocs.erase( oldDoc );
			mDirtyDoc.erase( oldDoc );
			mEditorDocs[editor] = newDoc;
			mDocs.insert( newDoc );
			Lock matchesLock( mMatchesMutex );
			mMatches.erase( oldDoc );
		} ) );

	listeners.push_back( editor->addEventListener(
		Event::OnTextChanged, [this, editor]( const Event* ) { setDocDirty( editor ); } ) );

	for ( auto& kb : mKeyBindings ) {
		if ( !kb.second.empty() )
			editor->getKeyBindings().addKeybindString( kb.second, kb.first );
	}

	if ( editor->hasDocument() ) {
		auto& doc = editor->getDocument();

		doc.setCommand( "linter-go-to-next-error", [this]( TextDocument::Client* client ) {
			goToNextError( static_cast<UICodeEditor*>( client ) );
		} );

		doc.setCommand( "linter-go-to-previous-error", [this]( TextDocument::Client* client ) {
			goToPrevError( static_cast<UICodeEditor*>( client ) );
		} );

		doc.setCommand( "linter-copy-error-message", [this]( TextDocument::Client* client ) {
			static_cast<UICodeEditor*>( client )
				->getUISceneNode()
				->getWindow()
				->getClipboard()
				->setText( mErrorMsg );
		} );
	}

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
	for ( auto editorIt : mEditorDocs )
		if ( editorIt.second == doc )
			return;

	for ( auto& kb : mKeyBindings ) {
		editor->getKeyBindings().removeCommandKeybind( kb.first );
		if ( editor->hasDocument() )
			editor->getDocument().removeCommand( kb.first );
	}

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
		mThreadPool->run( [this, doc] { lintDoc( doc ); } );
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

bool LinterPlugin::getErrorLens() const {
	return mErrorLens;
}

void LinterPlugin::setErrorLens( bool errorLens ) {
	mErrorLens = errorLens;
}

const std::vector<Linter>& LinterPlugin::getLinters() const {
	return mLinters;
}

Linter LinterPlugin::getLinterForLang( const std::string& lang ) {
	for ( const auto& linter : mLinters ) {
		for ( const auto& clang : linter.languages ) {
			if ( clang == lang ) {
				return linter;
			}
		}
	}
	return {};
}

void LinterPlugin::lintDoc( std::shared_ptr<TextDocument> doc ) {
	if ( !mLanguagesDisabled.empty() &&
		 mLanguagesDisabled.find( doc->getSyntaxDefinition().getLSPName() ) !=
			 mLanguagesDisabled.end() )
		return;

	ScopedOp op(
		[this, doc]() {
			std::lock_guard l( mWorkMutex );
			mWorkersCount++;
		},
		[this]() {
			{
				std::lock_guard l( mWorkMutex );
				mWorkersCount--;
			}
			mWorkerCondition.notify_all();
		} );
	if ( !mReady )
		return;
	auto linter = supportsLinter( doc );
	if ( linter.command.empty() )
		return;

	bool binaryFound = false;
	auto parts = String::split( linter.command, ' ' );
	if ( !parts.empty() ) {
		if ( parts[0].find_first_of( "\\/" ) != std::string::npos ) {
			binaryFound = FileSystem::fileExists( parts[0] );
		} else {
			binaryFound = !Sys::which( parts[0] ).empty();
		}
	}
	if ( !binaryFound )
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
		FileSystem::fileHide( tmpPath );
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
	std::string pathstr( "\"" + path + "\"" );
	String::replaceAll( cmd, "$FILENAME", pathstr );
	String::replaceAll( cmd, "${file_path}", pathstr );
	String::replaceAll( cmd, "$PROJECTPATH", mManager->getWorkspaceFolder() );
	String::replaceAll( cmd, "${project_root}", mManager->getWorkspaceFolder() );
	if ( linter.isNative && mNativeLinters.find( cmd ) != mNativeLinters.end() ) {
		mNativeLinters[cmd]( doc, path );
		return;
	}
	Process process;
	TextDocument* docPtr = doc.get();
	ScopedOp op(
		[this, &process, &docPtr] {
			std::lock_guard l( mRunningProcessesMutex );
			auto found = mRunningProcesses.find( docPtr );
			if ( found != mRunningProcesses.end() )
				found->second->kill();
			mRunningProcesses[docPtr] = &process;
		},
		[this, &docPtr] {
			std::lock_guard l( mRunningProcessesMutex );
			mRunningProcesses.erase( docPtr );
		} );
	;

	if ( process.create( cmd, Process::getDefaultOptions() | Process::CombinedStdoutStderr, {},
						 mManager->getWorkspaceFolder() ) ) {
		int returnCode;
		std::string data;
		process.readAllStdOut( data, Seconds( 30 ) );

		if ( mShuttingDown ) {
			process.kill();
			return;
		}

		process.join( &returnCode );
		process.destroy();

		if ( linter.hasNoErrorsExitCode && linter.noErrorsExitCode == returnCode ) {
			Lock matchesLock( mMatchesMutex );
			std::map<Int64, std::vector<LinterMatch>> empty;
			setMatches( doc.get(), MatchOrigin::Linter, empty );
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
						for ( auto& tmatch : matches[line - 1] ) {
							if ( tmatch.range == linterMatch.range ) {
								tmatch.text += "\n" + linterMatch.text;
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

		for ( const auto& matchLine : matches ) {
			totalMatches += matchLine.second.size();
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

SyntaxStyleType LinterPlugin::getMatchString( const LinterType& type ) {
	switch ( type ) {
		case LinterType::Warning:
			return "warning"_sst;
		case LinterType::Notice:
			return "notice"_sst;
		default:
			break;
	}
	return "error"_sst;
}

void LinterPlugin::preDraw( UICodeEditor*, const Vector2f& /*startScroll*/,
							const Float& /*lineHeight*/, const TextPosition& /*cursor*/ ) {
	mQuickFixRect = {};
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
	Float sepSpace = PixelDensity::dpToPx( 24.f );
	bool quickFixRendered = false;

	for ( size_t i = 0; i < matches.size(); ++i ) {
		auto& match = matches[i];

		bool isDuplicate = false;
		if ( i > 0 ) {
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
		Color color(
			editor->getColorScheme().getEditorSyntaxStyle( getMatchString( match.type ) ).color );
		line.setTabWidth( editor->getTabWidth() );
		line.setStyleConfig( editor->getFontStyleConfig() );
		line.setColor( color );

		Int64 strSize = match.range.end().column() - match.range.start().column();
		Vector2f pos = { static_cast<Float>(
							 position.x + editor->getTextPositionOffset( match.range.start() ).x ),
						 position.y };
		if ( strSize <= 0 ) {
			strSize = 1;
			pos = { position.x, position.y };
		}

		std::string str( strSize, '~' );
		String string( str );
		line.setString( string );
		Rectf box( pos - editor->getScreenPos(), { editor->getTextWidth( string ), lineHeight } );
		match.box[editor] = box;
		line.draw( pos.x, pos.y + lineHeight * 0.5f );

		Float rLineWidth = 0;

		if ( !quickFixRendered && doc->getSelection().start().line() == index ) {
			if ( !match.diagnostic.codeActions.empty() ) {
				rLineWidth = editor->getLineWidth( index ) + editor->getGlyphWidth();
				Color wcolor(
					editor->getColorScheme().getEditorSyntaxStyle( "warning"_sst ).color );
				if ( nullptr == mLightbulbIcon ) {
					mLightbulbIcon = editor->getUISceneNode()->getUIIconThemeManager()->findIcon(
						"lightbulb-autofix" );
				}
				if ( nullptr != mLightbulbIcon ) {
					Drawable* drawable = mLightbulbIcon->getSize( (int)eefloor( lineHeight ) );
					if ( drawable == nullptr )
						return;

					Color oldColor( drawable->getColor() );
					drawable->setColor( wcolor );
					Vector2f pos = { position.x + rLineWidth, position.y };
					drawable->draw( pos );
					mQuickFixRect = { pos, drawable->getPixelsSize() };
					drawable->setColor( oldColor );
					quickFixRendered = true;
				}
			} else {
				mQuickFixRect = {};
			}
		}

		if ( !mErrorLens || i != 0 )
			continue;

		if ( rLineWidth == 0 )
			rLineWidth = editor->getLineWidth( index );
		Float lineWidth = rLineWidth + sepSpace;
		Float realSpace = editor->getViewportWidth();
		Float spaceWidth = realSpace - lineWidth;
		if ( spaceWidth < sepSpace )
			continue;

		Primitives p;
		auto nlPos = match.text.find_first_of( '\n' );
		line.setString( nlPos != std::string::npos ? match.text.substr( 0, nlPos ) : match.text );
		Float txtWidth = line.getTextWidth();
		Float distWidth = realSpace - txtWidth;
		if ( txtWidth > spaceWidth )
			distWidth = lineWidth;

		Color blendedColor( Color( color, 50 ) );
		p.drawRectangle( Rectf( { position.x + distWidth, position.y }, { txtWidth, lineHeight } ),
						 Color::Transparent, Color::Transparent, blendedColor, blendedColor );
		line.setColor( Color( color, 180 ) );
		line.draw( position.x + distWidth, position.y );
	}
}

void LinterPlugin::minimapDrawBefore( UICodeEditor* editor, const DocumentLineRange& docLineRange,
									  const DocumentViewLineRange&, const Vector2f& /*linePos*/,
									  const Vector2f& /*lineSize*/, const Float& /*charWidth*/,
									  const Float& /*gutterWidth*/,
									  const DrawTextRangesFn& drawTextRanges ) {
	Lock l( mMatchesMutex );
	auto matchIt = mMatches.find( editor->getDocumentRef().get() );
	if ( matchIt == mMatches.end() )
		return;

	TextDocument* doc = matchIt->first;
	for ( const auto& matches : matchIt->second ) {
		for ( const auto& match : matches.second ) {
			if ( match.range.intersectsLineRange( docLineRange ) ) {
				if ( match.lineCache != doc->line( match.range.start().line() ).getHash() )
					return;
				Color col( editor->getColorScheme()
							   .getEditorSyntaxStyle( getMatchString( match.type ) )
							   .color );
				col.blendAlpha( 100 );
				drawTextRanges( match.range, col, true );
			}
		}
	}
}

void LinterPlugin::tryHideHoveringMatch( UICodeEditor* editor ) {
	if ( mHoveringMatch && editor->getTooltip() && editor->getTooltip()->isVisible() ) {
		editor->setTooltipText( "" );
		editor->getTooltip()->hide();
		mHoveringMatch = false;
	}
}

void LinterPlugin::goToNextError( UICodeEditor* editor ) {
	if ( nullptr == editor || !editor->hasDocument() )
		return;
	TextDocument* doc = &editor->getDocument();
	auto pos = doc->getSelection().start();

	Lock l( mMatchesMutex );
	auto fMatch = mMatches.find( doc );
	if ( fMatch == mMatches.end() )
		return;
	const auto& matches = fMatch->second;
	if ( matches.empty() )
		return;

	const LinterMatch* matched = nullptr;
	for ( const auto& match : matches ) {
		if ( match.first > pos.line() ) {
			if ( mGoToIgnoreWarnings ) {
				for ( const auto& m : match.second ) {
					if ( m.type == LinterType::Error ) {
						matched = &m;
						break;
					}
				}
			} else {
				matched = &match.second.front();
				break;
			}
		}
	}

	if ( matched != nullptr ) {
		editor->goToLine( matched->range.start() );
		mManager->getSplitter()->addCurrentPositionToNavigationHistory();
	} else {
		if ( mGoToIgnoreWarnings ) {
			for ( const auto& m : matches ) {
				if ( m.first < pos.line() ) {
					for ( const auto& lm : m.second ) {
						if ( lm.type == LinterType::Error ) {
							editor->goToLine( lm.range.start() );
							mManager->getSplitter()->addCurrentPositionToNavigationHistory();
							break;
						}
					}
				} else {
					break;
				}
			}
		} else if ( matches.begin()->second.front().range.start().line() != pos.line() ) {
			editor->goToLine( matches.begin()->second.front().range.start() );
			mManager->getSplitter()->addCurrentPositionToNavigationHistory();
		}
	}
}

void LinterPlugin::goToPrevError( UICodeEditor* editor ) {
	if ( nullptr == editor || !editor->hasDocument() )
		return;
	TextDocument* doc = &editor->getDocument();
	auto pos = doc->getSelection().start();

	Lock l( mMatchesMutex );
	auto fMatch = mMatches.find( doc );
	if ( fMatch == mMatches.end() )
		return;
	auto& matches = fMatch->second;
	if ( matches.empty() )
		return;

	const LinterMatch* matched = nullptr;
	for ( auto match = matches.rbegin(); match != matches.rend(); ++match ) {
		if ( match->first < pos.line() ) {
			if ( mGoToIgnoreWarnings ) {
				for ( const auto& m : match->second ) {
					if ( m.type == LinterType::Error ) {
						matched = &m;
						break;
					}
				}
			} else {
				matched = &match->second.front();
				break;
			}
		}
	}

	if ( matched != nullptr ) {
		editor->goToLine( matched->range.start() );
		mManager->getSplitter()->addCurrentPositionToNavigationHistory();
	} else {
		if ( mGoToIgnoreWarnings ) {
			for ( auto m = matches.rbegin(); m != matches.rend(); ++m ) {
				if ( m->first > pos.line() ) {
					for ( const auto& lm : m->second ) {
						if ( lm.type == LinterType::Error ) {
							editor->goToLine( lm.range.start() );
							mManager->getSplitter()->addCurrentPositionToNavigationHistory();
							break;
						}
					}
				} else {
					break;
				}
			}
		} else if ( matches.rbegin()->second.front().range.start().line() != pos.line() ) {
			editor->goToLine( matches.rbegin()->second.front().range.start() );
			mManager->getSplitter()->addCurrentPositionToNavigationHistory();
		}
	}
}

bool LinterPlugin::onMouseClick( UICodeEditor* editor, const Vector2i& pos, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) && mQuickFixRect.Right != 0 && mQuickFixRect.Bottom != 0 &&
		 mQuickFixRect.contains( pos.asFloat() ) ) {
		editor->getDocument().execute( "lsp-symbol-code-action", editor );
		return true;
	}
	return false;
}

bool LinterPlugin::onMouseMove( UICodeEditor* editor, const Vector2i& pos, const Uint32& flags ) {
	if ( mQuickFixRect.Right != 0 && mQuickFixRect.Bottom != 0 &&
		 mQuickFixRect.contains( pos.asFloat() ) ) {
		editor->getUISceneNode()->setCursor( Cursor::Hand );
		return true;
	}
	if ( flags != 0 ) {
		tryHideHoveringMatch( editor );
		return false;
	}
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
					if ( match.text.empty() )
						return false;
					mHoveringMatch = true;
					editor->setTooltipText( match.text );
					UITooltip* tooltip = editor->createTooltip();
					if ( tooltip == nullptr )
						return false;
					tooltip->setHorizontalAlign( UI_HALIGN_LEFT );
					tooltip->setDontAutoHideOnMouseMove( true );
					tooltip->setPixelsPosition( tooltip->getTooltipPosition( pos.asFloat() ) );
					if ( !tooltip->isVisible() )
						tooltip->show();
					return true;
				}
			}
		}
		tryHideHoveringMatch( editor );
	}
	return false;
}

bool LinterPlugin::onMouseLeave( UICodeEditor* editor, const Vector2i&, const Uint32& ) {
	tryHideHoveringMatch( editor );
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

bool LinterPlugin::onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
										const Vector2i& pos, const Uint32& /*flags*/ ) {
	Lock l( mMatchesMutex );
	auto it = mMatches.find( editor->getDocumentRef().get() );
	if ( it == mMatches.end() )
		return false;

	Vector2f localPos( editor->convertToNodeSpace( pos.asFloat() ) );
	TextPosition cursorPosition = editor->resolveScreenPosition( pos.asFloat() );
	auto matchIt = it->second.find( cursorPosition.line() );
	if ( matchIt == it->second.end() )
		return false;

	auto& matches = matchIt->second;
	for ( auto& match : matches ) {
		if ( match.box[editor].contains( localPos ) ) {
			menu->addSeparator();
			menu->add( editor->i18n( "linter_copy_error_message", "Copy Error Message" ),
					   mManager->getUISceneNode()->findIcon( "copy" )->getSize(
						   PixelDensity::dpToPxI( 12 ) ) )
				->setId( "linter-copy-error-message" );
			mErrorMsg = match.text;
			break;
		}
	}

	return false;
}

void LinterPlugin::registerNativeLinter(
	const std::string& cmd,
	const std::function<void( std::shared_ptr<TextDocument>, const std::string& )>& nativeLinter ) {
	mNativeLinters[cmd] = nativeLinter;
}

void LinterPlugin::unregisterNativeLinter( const std::string& cmd ) {
	mNativeLinters.erase( cmd );
}

void LinterPlugin::registerNativeLinters() {
	if ( !mNativeLinters.empty() )
		return;
	mNativeLinters["xml"] = [this]( std::shared_ptr<TextDocument> doc, const std::string& path ) {
		pugi::xml_document xmlDoc;
		pugi::xml_parse_result result = xmlDoc.load_file( path.c_str() );
		std::map<Int64, std::vector<LinterMatch>> matches;
		if ( !result ) {
			std::string file;
			FileSystem::fileGet( path, file );
			std::string_view filesv{ file };
			Int64 line = StringHelper::countLines( filesv.substr( 0, result.offset ) );
			Int64 offset = 0;
			auto lastNL = filesv.substr( 0, result.offset ).find_last_of( '\n' );
			if ( lastNL != std::string_view::npos )
				offset = result.offset - lastNL;
			LinterMatch match;
			match.range = { { line, offset }, { line, offset } };
			match.range = { doc->nextWordBoundary( match.range.start(), false ),
							doc->previousWordBoundary( match.range.start(), false ) };
			match.text = result.description();
			match.type = getLinterTypeFromSeverity( LSPDiagnosticSeverity::Error );
			match.lineCache = doc->line( match.range.start().line() ).getHash();
			match.origin = MatchOrigin::Linter;
			matches[line] = { match };
		}
		setMatches( doc.get(), MatchOrigin::Linter, matches );
	};
}

} // namespace ecode
