#include "formatterplugin.hpp"
#include "../../scopedop.hpp"
#include "../../thirdparty/json.hpp"
#include "../../thirdparty/subprocess.h"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
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

FormatterPlugin::FormatterPlugin( const std::string& formattersPath,
								  std::shared_ptr<ThreadPool> pool ) :
	mPool( pool ) {
#if FORMATTER_THREADED
	mPool->run( [&, formattersPath] { load( formattersPath ); }, [] {} );
#else
	load( formattersPath );
#endif
}

FormatterPlugin::~FormatterPlugin() {
	mShuttingDown = true;

	if ( mWorkersCount != 0 ) {
		std::unique_lock<std::mutex> lock( mWorkMutex );
		mWorkerCondition.wait( lock, [&]() { return mWorkersCount <= 0; } );
	}

	for ( auto editor : mEditors )
		editor->unregisterPlugin( this );
}

void FormatterPlugin::onRegister( UICodeEditor* editor ) {
	mEditors.insert( editor );

	auto& doc = editor->getDocument();

	if ( doc.hasCommand( "format-doc" ) )
		return;

	doc.setCommand( "format-doc", [&, editor]() { formatDoc( editor ); } );

	editor->addEventListener( Event::OnDocumentSave, [&]( const Event* event ) {
		if ( mAutoFormatOnSave && event->getNode()->isType( UI_TYPE_CODEEDITOR ) )
			formatDoc( event->getNode()->asType<UICodeEditor>() );
	} );
}

void FormatterPlugin::onUnregister( UICodeEditor* editor ) {
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

void FormatterPlugin::load( const std::string& formatterPath ) {
	registerNativeFormatters();

	if ( !FileSystem::fileExists( formatterPath ) )
		return;
	try {
		std::ifstream stream( formatterPath );
		json j;
		stream >> j;

		for ( auto& obj : j ) {
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

			mFormatters.emplace_back( std::move( formatter ) );
		}

		mReady = true;
	} catch ( json::exception& e ) {
		mReady = false;
		Log::error( "Parsing formatter failed:\n%s", e.what() );
	}
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

	Log::debug( "FormatterPlugin::formatDoc for %s took %.2fms", path.c_str(),
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
	std::vector<std::string> cmdArr = String::split( cmd, " ", "", "\"", true );
	std::vector<const char*> strings;
	for ( size_t i = 0; i < cmdArr.size(); ++i )
		strings.push_back( cmdArr[i].c_str() );
	strings.push_back( NULL );
	struct subprocess_s subprocess;
	int result =
		subprocess_create( strings.data(),
						   subprocess_option_search_user_path |
							   subprocess_option_inherit_environment | subprocess_option_no_window,
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

		if ( mShuttingDown ) {
			subprocess_terminate( &subprocess );
			return;
		}

		subprocess_join( &subprocess, &returnCode );
		subprocess_destroy( &subprocess );

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
}

} // namespace ecode
