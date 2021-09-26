#include "formattermodule.hpp"
#include "thirdparty/json.hpp"
#include "thirdparty/subprocess.h"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <random>

using json = nlohmann::json;

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define FORMATTER_THREADED 1
#else
#define FORMATTER_THREADED 0
#endif

FormatterModule::FormatterModule( const std::string& formattersPath,
								  std::shared_ptr<ThreadPool> pool ) :
	mPool( pool ) {
#if FORMATTER_THREADED
	mPool->run( [&, formattersPath] { load( formattersPath ); }, [] {} );
#else
	load( formattersPath );
#endif
}

FormatterModule::~FormatterModule() {
	mClosing = true;
	for ( auto editor : mEditors )
		editor->unregisterModule( this );
}

void FormatterModule::onRegister( UICodeEditor* editor ) {
	mEditors.insert( editor );

	auto& doc = editor->getDocument();

	if ( doc.hasCommand( "format-doc" ) )
		return;

	doc.setCommand( "format-doc", [&, editor]() { formatDoc( editor ); } );
}

void FormatterModule::onUnregister( UICodeEditor* editor ) {
	if ( mClosing )
		return;
	mEditors.erase( editor );
}

void FormatterModule::load( const std::string& formatterPath ) {
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
				formatter.type =
					"inplace" == typeStr ? FormatterType::Inplace : FormatterType::Output;
			}

			mFormatters.emplace_back( std::move( formatter ) );
		}

		mReady = true;
	} catch ( json::exception& e ) {
		mReady = false;
		Log::error( "Parsing formatter failed:\n%s", e.what() );
	}
}
static std::string randString( size_t len ) {
	std::string str( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" );
	std::random_device rd;
	std::mt19937 generator( rd() );
	std::shuffle( str.begin(), str.end(), generator );
	return str.substr( 0, len );
}

void FormatterModule::formatDoc( UICodeEditor* editor ) {
	if ( !mReady )
		return;
	std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
	auto formatter = supportsFormatter( doc );
	if ( formatter.command.empty() && doc->getFilePath().empty() )
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
		runFormatter( editor, formatter, tmpPath );
		FileSystem::fileRemove( tmpPath );
	} else {
		runFormatter( editor, formatter, doc->getFilePath() );
	}
}

void FormatterModule::runFormatter( UICodeEditor* editor, const Formatter& formatter,
									const std::string& path ) {
	Clock clock;

	std::string cmd( formatter.command );
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

		// Log::info( "Formatter result:\n%s", data.c_str() );

		if ( formatter.type == FormatterType::Output ) {
			editor->runOnMainThread( [&, data, editor]() {
				std::shared_ptr<TextDocument> doc = editor->getDocumentRef();
				TextPosition pos = doc->getSelection().start();
				doc->selectAll();
				doc->textInput( data );
				doc->setSelection( pos );
			} );
		}

		Log::info( "FormatterModule::formatDoc for %s took %.2fms", path.c_str(),
				   clock.getElapsedTime().asMilliseconds() );
	}
}

FormatterModule::Formatter FormatterModule::supportsFormatter( std::shared_ptr<TextDocument> doc ) {
	std::string filePath( doc->getFilePath() );
	std::string extension( FileSystem::fileExtension( filePath ) );
	if ( extension.empty() )
		extension = FileSystem::fileNameFromPath( filePath );
	const auto& def = doc->getSyntaxDefinition();
	for ( auto& formatter : mFormatters ) {
		for ( auto& ext : formatter.files ) {
			auto& files = def.getFiles();
			if ( std::find( files.begin(), files.end(), ext ) != files.end() ) {
				return formatter;
			}
		}
	}
	return {};
}
