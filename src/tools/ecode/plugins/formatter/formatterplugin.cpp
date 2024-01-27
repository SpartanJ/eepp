#include "formatterplugin.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <nlohmann/json.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using json = nlohmann::json;

namespace ecode {

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define FORMATTER_THREADED 1
#else
#define FORMATTER_THREADED 0
#endif

Plugin* FormatterPlugin::New( PluginManager* pluginManager ) {
	return eeNew( FormatterPlugin, ( pluginManager, false ) );
}

Plugin* FormatterPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( FormatterPlugin, ( pluginManager, true ) );
}

FormatterPlugin::FormatterPlugin( PluginManager* pluginManager, bool sync ) :
	Plugin( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
#if defined( FORMATTER_THREADED ) && FORMATTER_THREADED == 1
		mThreadPool->run( [&, pluginManager] { load( pluginManager ); } );
#else
		load( pluginManager );
#endif
	}
}

FormatterPlugin::~FormatterPlugin() {
	mShuttingDown = true;
	unsubscribeFileSystemListener();

	if ( mWorkersCount != 0 ) {
		std::unique_lock<std::mutex> lock( mWorkMutex );
		mWorkerCondition.wait( lock, [this]() { return mWorkersCount <= 0; } );
	}

	for ( auto& editor : mEditors ) {
		for ( auto& kb : mKeyBindings ) {
			editor.first->getKeyBindings().removeCommandKeybind( kb.first );
			if ( editor.first->hasDocument() )
				editor.first->getDocument().removeCommand( kb.first );
		}

		editor.first->unregisterPlugin( this );
	}
}

void FormatterPlugin::onRegister( UICodeEditor* editor ) {
	std::vector<Uint32> listeners;

	for ( auto& kb : mKeyBindings ) {
		if ( !kb.first.empty() )
			editor->getKeyBindings().addKeybindString( kb.second, kb.first );
	}

	if ( editor->hasDocument() ) {
		editor->getDocument().setCommand( "format-doc", [this]( TextDocument::Client* client ) {
			formatDocAsync( static_cast<UICodeEditor*>( client ) );
		} );
	}

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [this, editor]( const Event* ) {
			tryRequestCapabilities( editor->getDocumentRef() );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentChanged, [this, editor]( const Event* ) {
			TextDocument* newDoc = editor->getDocumentRef().get();
			mEditorDocs[editor] = newDoc;
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentSave, [this]( const Event* event ) {
			if ( mAutoFormatOnSave && event->getNode()->isType( UI_TYPE_CODEEDITOR ) ) {
				UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
				auto isAutoFormatting = mIsAutoFormatting.find( &editor->getDocument() );
				if ( ( isAutoFormatting == mIsAutoFormatting.end() ||
					   isAutoFormatting->second == false ) &&
					 mPluginManager &&
					 !String::startsWith( editor->getDocument().getFilePath(),
										  mPluginManager->getPluginsPath() ) )
					formatDocAsync( editor );
			}
		} ) );

	mEditors.insert( { editor, listeners } );
	mEditorDocs[editor] = editor->getDocumentRef().get();
}

void FormatterPlugin::onUnregister( UICodeEditor* editor ) {
	auto afIt = mIsAutoFormatting.find( &editor->getDocument() );
	if ( afIt != mIsAutoFormatting.end() )
		mIsAutoFormatting.erase( afIt );

	auto cbs = mEditors[editor];
	for ( auto listener : cbs )
		editor->removeEventListener( listener );

	if ( mShuttingDown )
		return;
	mEditors.erase( editor );
	mEditorDocs.erase( editor );

	TextDocument* doc = &editor->getDocument();
	for ( auto editorIt : mEditorDocs )
		if ( editorIt.second == doc )
			return;

	for ( auto& kb : mKeyBindings ) {
		editor->getKeyBindings().removeCommandKeybind( kb.first );
		if ( editor->hasDocument() )
			editor->getDocument().removeCommand( kb.first );
	}
}

bool FormatterPlugin::getAutoFormatOnSave() const {
	return mAutoFormatOnSave;
}

void FormatterPlugin::setAutoFormatOnSave( bool autoFormatOnSave ) {
	mAutoFormatOnSave = autoFormatOnSave;
}

void FormatterPlugin::registerNativeFormatter(
	const std::string& cmd,
	const std::function<NativeFormatterResult( const std::string& )>& nativeFormatter ) {
	mNativeFormatters[cmd] = nativeFormatter;
}

void FormatterPlugin::unregisterNativeFormatter( const std::string& cmd ) {
	mNativeFormatters.erase( cmd );
}

size_t FormatterPlugin::formatterFilePatternPosition( const std::vector<std::string>& patterns ) {
	for ( size_t i = 0; i < mFormatters.size(); ++i ) {
		for ( const std::string& filePattern : mFormatters[i].files ) {
			for ( const std::string& pattern : patterns ) {
				if ( filePattern == pattern ) {
					return i;
				}
			}
		}
	}
	return std::string::npos;
}

void FormatterPlugin::loadFormatterConfig( const std::string& path, bool updateConfigFile ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	json j;

	if ( updateConfigFile ) {
		mConfigHash = String::hash( data );
	}

	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "FormatterPlugin::loadFormatterConfig - Error parsing formatter config from "
					"path %s, error: %s, config file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		if ( !updateConfigFile )
			return;
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"keybindings\":{},\n  \"formatters\":[]\n}\n",
						 nullptr, true, true );
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "auto_format_on_save" ) )
			setAutoFormatOnSave( config["auto_format_on_save"].get<bool>() );
		else if ( updateConfigFile )
			config["auto_format_on_save"] = getAutoFormatOnSave();
	}

	if ( mKeyBindings.empty() )
		mKeyBindings["format-doc"] = "alt+f";
	if ( j.contains( "keybindings" ) && j["keybindings"].contains( "format-doc" ) )
		mKeyBindings["format-doc"] = j["keybindings"]["format-doc"];
	else if ( updateConfigFile )
		j["keybindings"]["format-doc"] = mKeyBindings["format-doc"];

	if ( updateConfigFile ) {
		std::string newData = j.dump( 2 );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( data );
		}
	}

	if ( !j.contains( "formatters" ) )
		return;

	auto& formatters = j["formatters"];
	for ( auto& obj : formatters ) {
		Formatter formatter;
		auto fp = obj["file_patterns"];

		if ( obj.contains( "language" ) ) {
			if ( obj["language"].is_array() ) {
				const auto& langs = obj["language"];
				for ( const auto& lang : langs ) {
					if ( lang.is_string() )
						formatter.languages.push_back( lang.get<std::string>() );
				}
			} else if ( obj["language"].is_string() ) {
				formatter.languages.push_back( obj["language"].get<std::string>() );
			}
		}

		for ( auto& pattern : fp )
			formatter.files.push_back( pattern.get<std::string>() );

		formatter.command = obj["command"].get<std::string>();
		formatter.url = obj.value( "url", "" );

		if ( obj.contains( "type" ) ) {
			std::string typeStr( obj["type"].get<std::string>() );
			String::toLowerInPlace( typeStr );
			String::trimInPlace( typeStr );
			if ( "native" == typeStr ) {
				formatter.type = FormatterType::Native;
				if ( mNativeFormatters.find( formatter.command ) == mNativeFormatters.end() ) {
					Log::error( "Requested native formatter: '%s' does not exists.",
								formatter.command.c_str() );
					continue;
				}
			} else {
				formatter.type =
					"inplace" == typeStr ? FormatterType::Inplace : FormatterType::Output;
			}
		}

		// If the file pattern is repeated, we will overwrite the previous linter.
		// The previous linter should be the "default" linter that comes with ecode.
		size_t pos = formatterFilePatternPosition( formatter.files );
		if ( pos != std::string::npos ) {
			mFormatters[pos] = formatter;
		} else {
			mFormatters.emplace_back( std::move( formatter ) );
		}
	}
}

void FormatterPlugin::load( PluginManager* pluginManager ) {
	AtomicBoolScopedOp loading( mLoading, true );
	mPluginManager = pluginManager;
	pluginManager->subscribeMessages( this,
									  [this]( const auto& notification ) -> PluginRequestHandle {
										  return processMessage( notification );
									  } );
	registerNativeFormatters();

	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/formatters.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "formatters.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite(
			 path, "{\n  \"config\":{},\n  \"keybindings\":{},\n  \"formatters\":[]\n}\n" ) ) {
		mConfigPath = path;
		paths.emplace_back( path );
	}
	if ( paths.empty() )
		return;
	for ( const auto& fpath : paths ) {
		try {
			loadFormatterConfig( fpath, mConfigPath == fpath );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing formatter \"%s\" failed:\n%s", fpath.c_str(), e.what() );
		}
	}

	subscribeFileSystemListener();
	mReady = !mFormatters.empty();
	if ( mReady ) {
		fireReadyCbs();
		setReady();
	}
}

bool FormatterPlugin::onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu, const Vector2i&,
										   const Uint32& ) {
	tryRequestCapabilities( editor->getDocumentRef() );
	if ( supportsFormatter( editor->getDocumentRef() ).command.empty() &&
		 !supportsLSPFormatter( editor->getDocumentRef() ) )
		return false;

	menu->addSeparator();
	menu->add( editor->getUISceneNode()->i18n( "formatter-format-document", "Format Document" ),
			   nullptr, KeyBindings::keybindFormat( mKeyBindings["format-doc"] ) )
		->setId( "format-doc" );

	return false;
}

const std::vector<FormatterPlugin::Formatter>& FormatterPlugin::getFormatters() const {
	return mFormatters;
}

FormatterPlugin::Formatter FormatterPlugin::getFormatterForLang( const std::string& lang ) {
	for ( const auto& formatter : mFormatters ) {
		for ( const auto& clang : formatter.languages ) {
			if ( clang == lang ) {
				return formatter;
			}
		}
	}
	return {};
}

void FormatterPlugin::formatDocAsync( UICodeEditor* editor ) {
	mThreadPool->run( [editor, this] { formatDoc( editor ); } );
}

void FormatterPlugin::formatDoc( UICodeEditor* editor ) {
	ScopedOp op(
		[this]() {
			mWorkMutex.lock();
			mWorkersCount++;
		},
		[this]() {
			mWorkersCount--;
			mWorkMutex.unlock();
			mWorkerCondition.notify_all();
		} );
	if ( !mReady )
		return;

	Clock clock;
	std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
	auto formatter = supportsFormatter( doc );
	if ( formatter.command.empty() ) {
		if ( supportsLSPFormatter( doc ) )
			formatDocWithLSP( doc );
		return;
	} else if ( doc->getFilePath().empty() ) {
		return;
	}
	std::string path;
	if ( doc->isDirty() || !doc->hasFilepath() || formatter.type == FormatterType::Inplace ) {
		std::string tmpPath;
		if ( !doc->hasFilepath() ) {
			tmpPath =
				Sys::getTempPath() + ".ecode-" + doc->getFilename() + "." + String::randString( 8 );
		} else {
			std::string fileDir( FileSystem::fileRemoveFileName( doc->getFilePath() ) );
			FileSystem::dirAddSlashAtEnd( fileDir );
			tmpPath = fileDir + "." + String::randString( 8 ) + "." + doc->getFilename();
		}

		IOStreamString fileString;
		doc->save( fileString, true );
		FileSystem::fileWrite( tmpPath, (Uint8*)fileString.getStreamPointer(),
							   fileString.getSize() );
		FileSystem::fileHide( tmpPath );
		runFormatter( editor, formatter, tmpPath );

		if ( formatter.type == FormatterType::Inplace ) {
			std::string data;
			FileSystem::fileGet( tmpPath, data );

			auto oldFile = MD5::fromStream( fileString );
			auto newFile = MD5::fromString( data );

			if ( oldFile != newFile ) {
				editor->runOnMainThread( [&, data, editor]() {
					std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
					auto pos = doc->getSelection();
					auto scroll = editor->getScroll();
					doc->resetCursor();
					doc->selectAll();
					doc->setRunningTransaction( true );
					doc->textInput( data, false );
					doc->setSelection( pos );
					editor->setScroll( scroll );
					if ( mAutoFormatOnSave && mPluginManager &&
						 !String::startsWith( doc->getFilePath(),
											  mPluginManager->getPluginsPath() ) ) {
						mIsAutoFormatting[doc.get()] = true;
						doc->save();
						mIsAutoFormatting[doc.get()] = false;
					}
					doc->setRunningTransaction( false );
				} );
			}
		}

		FileSystem::fileRemove( tmpPath );
		path = tmpPath;
	} else {
		runFormatter( editor, formatter, doc->getFilePath() );
		path = doc->getFilePath();
	}

	Log::info( "FormatterPlugin::formatDoc for %s took %.2fms", path.c_str(),
			   clock.getElapsedTime().asMilliseconds() );
}

void FormatterPlugin::runFormatter( UICodeEditor* editor, const Formatter& formatter,
									const std::string& path ) {
	if ( formatter.type == FormatterType::Native ) {
		NativeFormatterResult res = mNativeFormatters[formatter.command]( path );
		if ( !res.success )
			return;
		editor->runOnMainThread( [&, res, editor]() {
			std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
			TextPosition pos = doc->getSelection().start();
			auto scroll = editor->getScroll();
			doc->resetCursor();
			doc->selectAll();
			doc->setRunningTransaction( true );
			doc->textInput( res.result, false );
			doc->setSelection( pos );
			editor->setScroll( scroll );
			doc->setRunningTransaction( false );
		} );
		return;
	}

	std::string cmd( formatter.command );
	String::replaceAll( cmd, "$FILENAME", "\"" + path + "\"" );
	Process process;
	if ( process.create( cmd ) ) {
		std::string data;
		process.readAllStdOut( data, Seconds( 30 ) );

		if ( mShuttingDown ) {
			process.kill();
			return;
		}

		process.destroy();

		// Log::info( "Formatter result:\n%s", data.c_str() );

		if ( formatter.type == FormatterType::Output ) {
			if ( data.empty() )
				return;

			IOStreamString fileString;
			editor->getDocumentRef()->save( fileString, true );

			auto oldFile = MD5::fromStream( fileString );
			auto newFile = MD5::fromString( data );

			if ( oldFile != newFile ) {
				editor->runOnMainThread( [&, data, editor]() {
					std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
					TextPosition pos = doc->getSelection().start();
					auto scroll = editor->getScroll();
					doc->resetCursor();
					doc->selectAll();
					doc->setRunningTransaction( true );
					doc->textInput( data, false );
					doc->setSelection( pos );
					editor->setScroll( scroll );
					doc->setRunningTransaction( false );
				} );
			}
		}
	}
}

FormatterPlugin::Formatter FormatterPlugin::supportsFormatter( std::shared_ptr<TextDocument> doc ) {
	std::string fileName( FileSystem::fileNameFromPath( doc->getFilePath() ) );
	const auto& def = doc->getSyntaxDefinition();

	for ( const auto& formatter : mFormatters ) {
		for ( const auto& ext : formatter.files ) {
			if ( LuaPattern::matches( fileName, ext ) )
				return formatter;
			auto& files = def.getFiles();
			if ( std::find( files.begin(), files.end(), ext ) != files.end() )
				return formatter;
		}
	}
	return {};
}

bool FormatterPlugin::supportsLSPFormatter( std::shared_ptr<TextDocument> doc ) {
	auto lang = doc->getSyntaxDefinition().getLSPName();
	auto cap = mCapabilities.find( lang );
	if ( cap != mCapabilities.end() )
		return cap->second.documentFormattingProvider;
	return false;
}

static json getURIJSON( std::shared_ptr<TextDocument> doc ) {
	json data;
	json docUri;
	json options;
	docUri["uri"] = doc->getURI().toString();
	data["textDocument"] = docUri;
	options["tabSize"] = doc->getIndentWidth();
	options["insertSpaces"] = doc->getIndentType() == TextDocument::IndentType::IndentSpaces;
	data["options"] = options;
	return data;
}

bool FormatterPlugin::formatDocWithLSP( std::shared_ptr<TextDocument> doc ) {
	json data = getURIJSON( doc );
	mManager->sendBroadcast( this, PluginMessageType::DocumentFormatting, PluginMessageFormat::JSON,
							 &data );
	return false;
}

void FormatterPlugin::registerNativeFormatters() {
	if ( !mNativeFormatters.empty() )
		return;
	mNativeFormatters["xml"] = []( const std::string& file ) -> NativeFormatterResult {
		struct xml_string_writer : pugi::xml_writer {
			std::string result;

			virtual void write( const void* data, size_t size ) {
				result.append( static_cast<const char*>( data ), size );
			}
		};
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( file.c_str() );

		if ( result ) {
			xml_string_writer str;
			doc.save( str );
			return { true, str.result, "" };
		} else {
			std::string err(
				String::format( "Couldn't load: %s\nError description: %s\nError offset: %td\n",
								file.c_str(), result.description(), result.offset ) );
			Log::error( err );
			return { false, "", err };
		}
	};

	mNativeFormatters["css"] = []( const std::string& file ) -> NativeFormatterResult {
		CSS::StyleSheetParser parser;
		if ( parser.loadFromFile( file ) ) {
			return { true, parser.getStyleSheet().print(), "" };
		} else {
			return { false, "", "Couldn't parse CSS file." };
		}
	};

	mNativeFormatters["json"] = []( const std::string& file ) -> NativeFormatterResult {
		std::string data;
		if ( !FileSystem::fileGet( file, data ) )
			return { false, "", "Couldn't access JSON file." };
		json j;
		try {
			j = json::parse( data, nullptr, true, true );
		} catch ( ... ) {
			return { false, "", "Error parsing JSON file." };
		}
		std::string res( j.dump( 2 ) );
		return { !res.empty(), res, "" };
	};
}

bool FormatterPlugin::tryRequestCapabilities( const std::shared_ptr<TextDocument>& doc ) {
	const auto& language = doc->getSyntaxDefinition().getLSPName();
	auto it = mCapabilities.find( language );
	if ( it != mCapabilities.end() )
		return true;
	json data;
	data["language"] = language;
	mManager->sendRequest( this, PluginMessageType::LanguageServerCapabilities,
						   PluginMessageFormat::JSON, &data );
	return false;
}

PluginRequestHandle FormatterPlugin::processMessage( const PluginMessage& msg ) {
	if ( msg.type == PluginMessageType::FileSystemListenerReady ) {
		subscribeFileSystemListener();
	} else if ( msg.isBroadcast() && msg.type == PluginMessageType::LanguageServerCapabilities ) {
		if ( msg.asLanguageServerCapabilities().ready ) {
			LSPServerCapabilities cap = msg.asLanguageServerCapabilities();
			Lock l( mCapabilitiesMutex );
			for ( const auto& lang : cap.languages )
				mCapabilities[lang] = cap;
		}
	}
	return {};
}

} // namespace ecode
