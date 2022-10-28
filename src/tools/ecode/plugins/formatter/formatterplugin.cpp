#include "formatterplugin.hpp"
#include "../../scopedop.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/process.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <nlohmann/json.hpp>
#include <random>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using json = nlohmann::json;

namespace ecode {

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define FORMATTER_THREADED 1
#else
#define FORMATTER_THREADED 0
#endif

UICodeEditorPlugin* FormatterPlugin::New( const PluginManager* pluginManager ) {
	return eeNew( FormatterPlugin, ( pluginManager ) );
}

FormatterPlugin::FormatterPlugin( const PluginManager* pluginManager ) :
	mPool( pluginManager->getThreadPool() ) {
#if FORMATTER_THREADED
	mPool->run( [&, pluginManager] { load( pluginManager ); }, [] {} );
#else
	load( pluginManager );
#endif
}

FormatterPlugin::~FormatterPlugin() {
	mShuttingDown = true;

	if ( mWorkersCount != 0 ) {
		std::unique_lock<std::mutex> lock( mWorkMutex );
		mWorkerCondition.wait( lock, [&]() { return mWorkersCount <= 0; } );
	}

	for ( auto editor : mEditors ) {
		for ( auto& kb : mKeyBindings ) {
			editor->getKeyBindings().removeCommandKeybind( kb.first );
			if ( editor->hasDocument() )
				editor->getDocument().removeCommand( kb.first );
		}

		editor->unregisterPlugin( this );
	}
}

void FormatterPlugin::onRegister( UICodeEditor* editor ) {
	mEditors.insert( editor );

	for ( auto& kb : mKeyBindings ) {
		editor->getKeyBindings().addKeybindString( kb.second, kb.first );
	}

	if ( editor->hasDocument() )
		editor->getDocument().setCommand( "format-doc", [&, editor]() { formatDoc( editor ); } );

	mOnDocumentSaveCb = editor->addEventListener( Event::OnDocumentSave, [&]( const Event* event ) {
		if ( mAutoFormatOnSave && event->getNode()->isType( UI_TYPE_CODEEDITOR ) )
			formatDoc( event->getNode()->asType<UICodeEditor>() );
	} );
}

void FormatterPlugin::onUnregister( UICodeEditor* editor ) {
	for ( auto& kb : mKeyBindings ) {
		editor->getKeyBindings().removeCommandKeybind( kb.first );
		if ( editor->hasDocument() )
			editor->getDocument().removeCommand( kb.first );
	}

	if ( mOnDocumentSaveCb != 0 )
		editor->removeEventListener( mOnDocumentSaveCb );

	if ( mShuttingDown )
		return;
	mEditors.erase( editor );
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

void FormatterPlugin::loadFormatterConfig( const std::string& path ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	json j;

	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( ... ) {
		return;
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "auto_format_on_save" ) )
			setAutoFormatOnSave( config["auto_format_on_save"].get<bool>() );
	}

	if ( mKeyBindings.empty() )
		mKeyBindings["format-doc"] = "alt+f";
	if ( j.contains( "keybindings" ) && j["keybindings"].contains( "format-doc" ) )
		mKeyBindings["format-doc"] = j["keybindings"]["format-doc"];

	if ( !j.contains( "formatters" ) )
		return;

	auto& formatters = j["formatters"];
	for ( auto& obj : formatters ) {
		Formatter formatter;
		auto fp = obj["file_patterns"];

		for ( auto& pattern : fp )
			formatter.files.push_back( pattern.get<std::string>() );

		formatter.command = obj["command"].get<std::string>();

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

void FormatterPlugin::load( const PluginManager* pluginManager ) {
	registerNativeFormatters();

	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/formatters.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "formatters.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite( path, "{\n\"config\":{},\n\"formatters\":[]\n}\n" ) ) {
		mConfigPath = path;
		paths.emplace_back( path );
	}
	if ( paths.empty() )
		return;
	for ( const auto& path : paths ) {
		try {
			loadFormatterConfig( path );
		} catch ( json::exception& e ) {
			Log::error( "Parsing formatter \"%s\" failed:\n%s", path.c_str(), e.what() );
		}
	}
	mReady = !mFormatters.empty();
	if ( mReady )
		fireReadyCbs();
}

bool FormatterPlugin::hasFileConfig() {
	return !mConfigPath.empty();
}

std::string FormatterPlugin::getFileConfigPath() {
	return mConfigPath;
}

void FormatterPlugin::formatDoc( UICodeEditor* editor ) {
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

	Clock clock;
	std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
	auto formatter = supportsFormatter( doc );
	if ( formatter.command.empty() || doc->getFilePath().empty() )
		return;
	IOStreamString fileString;
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

		doc->save( fileString, true );
		FileSystem::fileWrite( tmpPath, (Uint8*)fileString.getStreamPointer(),
							   fileString.getSize() );
		runFormatter( editor, formatter, tmpPath );

		if ( formatter.type == FormatterType::Inplace ) {
			std::string data;
			FileSystem::fileGet( tmpPath, data );

			editor->runOnMainThread( [&, data, editor]() {
				std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
				TextPosition pos = doc->getSelection().start();
				doc->selectAll();
				doc->textInput( data );
				doc->setSelection( pos );
			} );
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
			doc->selectAll();
			doc->textInput( res.result );
			doc->setSelection( pos );
		} );
		return;
	}

	std::string cmd( formatter.command );
	String::replaceAll( cmd, "$FILENAME", "\"" + path + "\"" );
	Process process;
	if ( process.create( cmd ) ) {
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

		// Log::info( "Formatter result:\n%s", data.c_str() );

		if ( formatter.type == FormatterType::Output ) {
			if ( data.empty() )
				return;

			editor->runOnMainThread( [&, data, editor]() {
				std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
				TextPosition pos = doc->getSelection().start();
				doc->selectAll();
				doc->textInput( data );
				doc->setSelection( pos );
			} );
		}
	}
}

FormatterPlugin::Formatter FormatterPlugin::supportsFormatter( std::shared_ptr<TextDocument> doc ) {
	std::string fileName( FileSystem::fileNameFromPath( doc->getFilePath() ) );
	const auto& def = doc->getSyntaxDefinition();

	for ( auto& formatter : mFormatters ) {
		for ( auto& ext : formatter.files ) {
			if ( LuaPattern::find( fileName, ext ).isValid() )
				return formatter;
			auto& files = def.getFiles();
			if ( std::find( files.begin(), files.end(), ext ) != files.end() )
				return formatter;
		}
	}
	return {};
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

} // namespace ecode
