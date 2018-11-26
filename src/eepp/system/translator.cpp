#include <cctype>
#include <algorithm>
#include <cstdarg>
#include <eepp/system/translator.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/iostream.hpp>
#include <pugixml/pugixml.hpp>

namespace  EE { namespace System {

Translator::Translator( const std::locale& locale ) :
	mDefaultLanguage( "en" )
{
	setLanguageFromLocale( locale );
}

void Translator::loadFromDirectory( std::string dirPath, std::string ext ) {
	FileSystem::dirPathAddSlashAtEnd( dirPath );

	if ( FileSystem::isDirectory( dirPath ) ) {
		std::vector<std::string> files = FileSystem::filesGetInPath( dirPath, true, true );
		String::toLowerInPlace( ext );

		for ( size_t i = 0; i < files.size(); i++ ) {
			std::string path = dirPath + files[i];

			if ( FileSystem::fileExtension( path ) == ext ) {
				std::string lang = FileSystem::fileRemoveExtension( files[i] );

				loadFromFile( path, lang );
			}
		}
	}
}

void Translator::loadNodes( pugi::xml_node node, std::string lang ) {
	for ( pugi::xml_node resources = node; resources; resources = resources.next_sibling() ) {
		std::string name = String::toLower( resources.name() );

		if ( name == "resources" ) {
			lang = lang.size() == 2 ? lang : resources.attribute("language").as_string();

			if ( lang.empty() || lang.size() != 2 ) {
				eePRINTL( "Error: Couldn't load i18n language strings: language not specified in file name or resources attribute \"language\"." );
				return;
			}

			for ( pugi::xml_node string = resources.child("string"); string; string = string.next_sibling("string") ) {
				std::string key = string.attribute("name").as_string();

				if ( !key.empty() ) {
					String txt( string.text().as_string() );

					mDictionary[ lang ][ key ] = txt;
				}
			}
		}
	}
}

void Translator::loadFromFile( const std::string& path, std::string lang ) {
	if ( FileSystem::fileExists( path ) ) {
		lang = lang.size() == 2 ? lang : FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) );

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( path.c_str() );

		if ( result ) {
			loadNodes( doc.first_child(), lang );
		} else {
			eePRINTL( "Error: Couldn't load i18n file: %s", path.c_str() );
			eePRINTL( "Error description: %s", result.description() );
			eePRINTL( "Error offset: %d", result.offset );
		}
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string packPath( path );
		Pack * pack = PackManager::instance()->exists( packPath );

		if ( NULL != pack ) {
			loadFromPack( pack, packPath, lang );
		}
	}
}
void Translator::loadFromString( const std::string& string, std::string lang ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string( string.c_str() );

	if ( result ) {
		loadNodes( doc.first_child(), lang );
	} else {
		eePRINTL( "Error: Couldn't load i18n file from string: %s", string.c_str() );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}
}

void Translator::loadFromMemory( const void * buffer, Int32 bufferSize, std::string lang ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( buffer, bufferSize );

	if ( result ) {
		loadNodes( doc.first_child(), lang );
	} else {
		eePRINTL( "Error: Couldn't load i18n file from buffer" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}
}

void Translator::loadFromStream( IOStream& stream, std::string lang ) {
	if ( !stream.isOpen() )
		return;

	ios_size bufferSize = stream.getSize();
	SafeDataPointer safeDataPointer( bufferSize );
	stream.read( reinterpret_cast<char*>( safeDataPointer.data ), safeDataPointer.size );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( safeDataPointer.data, safeDataPointer.size );

	if ( result ) {
		loadNodes( doc.first_child(), lang );
	} else {
		eePRINTL( "Error: Couldn't load i18n file from stream" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}
}

void Translator::loadFromPack( Pack * pack, const std::string& FilePackPath, std::string lang ) {
	SafeDataPointer PData;

	if ( pack->isOpen() && pack->extractFileToMemory( FilePackPath, PData ) ) {
		lang = lang.size() == 2 ? lang : FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( FilePackPath ) );

		loadFromMemory( PData.data, PData.size, lang );
	}
}

String Translator::getString( const std::string& key ) {
	StringLocaleDictionary::iterator lang = mDictionary.find( mCurrentLanguage );

	if ( lang != mDictionary.end() ) {
		StringDictionary& dictionary = lang->second;
		StringDictionary::iterator string = dictionary.find( key );

		if ( string != dictionary.end() ) {
			return string->second;
		}
	}

	lang = mDictionary.find( mDefaultLanguage );

	if ( lang != mDictionary.end() ) {
		StringDictionary& dictionary = lang->second;
		StringDictionary::iterator string = dictionary.find( key );

		if ( string != dictionary.end() ) {
			return string->second;
		}
	}

	return String();
}

String Translator::getStringf( const char * key, ... ) {
	std::string str( getString( key ).toUtf8() );

	if ( str.empty() )
		return String();

	const char * format = str.c_str();

	int size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while (1) {
		va_start( args, key );

		int n = vsnprintf( &tstr[0], size, format, args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );

			va_end( args );

			return tstr;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		tstr.resize( size );
	}

	return String( tstr );
}

void Translator::setLanguageFromLocale( std::locale locale ) {
	std::string name = locale.name();

	if ( "C" == name ) {
		#if defined( EE_SUPPORT_EXCEPTIONS ) && EE_PLATFORM != EE_PLATFORM_WIN
		try {
			const char * loc = setlocale( LC_ALL, "" );
			locale = std::locale( loc );
			name = locale.name();
		} catch( ... ) {}
		#endif

		if ( "C" == name ) {
			mCurrentLanguage = "en";
		} else {
			mCurrentLanguage = name.substr(0,2);
		}
	} else {
		mCurrentLanguage = name.substr(0,2);
	}
}

std::string Translator::getDefaultLanguage() const {
	return mDefaultLanguage;
}

void Translator::setDefaultLanguage(const std::string & defaultLanguage) {
	mDefaultLanguage = defaultLanguage;
}

std::string Translator::getCurrentLanguage() const {
	return mCurrentLanguage;
}

void Translator::setCurrentLanguage(const std::string & currentLanguage) {
	mCurrentLanguage = currentLanguage;
}

}}
