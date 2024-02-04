#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/translator.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace System {

Translator::Translator( const std::locale& locale ) : mDefaultLanguage( "en" ) {
	setLanguageFromLocale( locale );
}

bool Translator::loadFromDirectory( std::string dirPath, std::string ext ) {
	FileSystem::dirAddSlashAtEnd( dirPath );
	bool someFailed = false;

	if ( FileSystem::isDirectory( dirPath ) ) {
		std::vector<std::string> files = FileSystem::filesGetInPath( dirPath, true, true );
		String::toLowerInPlace( ext );

		for ( size_t i = 0; i < files.size(); i++ ) {
			std::string path = dirPath + files[i];

			if ( FileSystem::fileExtension( path ) == ext ) {
				std::string lang = FileSystem::fileRemoveExtension( files[i] );

				if ( !loadFromFile( path, lang ) )
					someFailed = true;
			}
		}
	}

	return someFailed;
}

bool Translator::loadNodes( pugi::xml_node node, std::string lang ) {
	for ( pugi::xml_node resources = node; resources; resources = resources.next_sibling() ) {
		std::string name = String::toLower( std::string( resources.name() ) );

		if ( name == "resources" ) {
			lang = lang.size() == 2 ? lang : resources.attribute( "language" ).as_string();

			if ( lang.empty() || lang.size() != 2 ) {
				Log::error( "Error: Couldn't load i18n language strings: language not specified in "
							"file name or resources attribute \"language\"." );
				return false;
			}

			for ( pugi::xml_node string = resources.child( "string" ); string;
				  string = string.next_sibling( "string" ) ) {
				std::string key = string.attribute( "name" ).as_string();

				if ( !key.empty() ) {
					String txt( string.text().as_string() );

					mDictionary[lang][key] = txt;
				}
			}
		}
	}
	return true;
}

bool Translator::loadFromFile( const std::string& path, std::string lang ) {
	if ( FileSystem::fileExists( path ) ) {
		lang = lang.size() == 2
				   ? lang
				   : FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) );

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( path.c_str() );

		if ( result ) {
			return loadNodes( doc.first_child(), lang );
		} else {
			Log::error( "Couldn't load i18n file: %s", path.c_str() );
			Log::error( "Error description: %s", result.description() );
			Log::error( "Error offset: %d", result.offset );
		}
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string packPath( path );
		Pack* pack = PackManager::instance()->exists( packPath );

		if ( NULL != pack ) {
			return loadFromPack( pack, packPath, lang );
		}
	}
	return false;
}
bool Translator::loadFromString( const std::string& string, std::string lang ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string( string.c_str() );

	if ( result ) {
		return loadNodes( doc.first_child(), lang );
	} else {
		Log::error( "Couldn't load i18n file from string: %s", string.c_str() );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
	}
	return false;
}

bool Translator::loadFromMemory( const void* buffer, Int32 bufferSize, std::string lang ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( buffer, bufferSize );

	if ( result ) {
		return loadNodes( doc.first_child(), lang );
	} else {
		Log::error( "Couldn't load i18n file from buffer" );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
	}
	return false;
}

bool Translator::loadFromStream( IOStream& stream, std::string lang ) {
	if ( !stream.isOpen() )
		return false;

	ios_size bufferSize = stream.getSize();
	TScopedBuffer<char> scopedBuffer( bufferSize );
	stream.read( scopedBuffer.get(), scopedBuffer.length() );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( scopedBuffer.get(), scopedBuffer.length() );

	if ( result ) {
		return loadNodes( doc.first_child(), lang );
	} else {
		Log::error( "Couldn't load i18n file from stream" );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
	}

	return false;
}

bool Translator::loadFromPack( Pack* pack, const std::string& FilePackPath, std::string lang ) {
	ScopedBuffer buffer;

	if ( pack->isOpen() && pack->extractFileToMemory( FilePackPath, buffer ) ) {
		lang =
			lang.size() == 2
				? lang
				: FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( FilePackPath ) );

		return loadFromMemory( buffer.get(), buffer.length(), lang );
	}

	return false;
}

String Translator::getString( const std::string& key, const String& defaultValue ) {
	StringLocaleDictionary::iterator lang = mDictionary.find( mCurrentLanguage );

	if ( lang != mDictionary.end() || mSetDefaultValues ) {
		if ( lang == mDictionary.end() ) {
			mDictionary[mCurrentLanguage] = StringDictionary{};
			lang = mDictionary.find( mCurrentLanguage );
		}

		StringDictionary& dictionary = lang->second;
		StringDictionary::iterator string = dictionary.find( key );
		if ( string != dictionary.end() )
			return string->second;

		if ( mSetDefaultValues && !defaultValue.empty() ) {
			dictionary[key] = defaultValue;
			return defaultValue;
		}
	}

	lang = mDictionary.find( mDefaultLanguage );

	if ( lang != mDictionary.end() ) {
		StringDictionary& dictionary = lang->second;
		StringDictionary::iterator string = dictionary.find( key );

		if ( string != dictionary.end() )
			return string->second;
	}

	return defaultValue;
}

void Translator::setLanguageFromLocale( std::locale locale ) {
	std::string name = locale.name();

	if ( "C" == name ) {
#if defined( EE_SUPPORT_EXCEPTIONS ) && EE_PLATFORM != EE_PLATFORM_WIN
		try {
			const char* loc = setlocale( LC_CTYPE, "" );
			locale = std::locale( loc );
			name = locale.name();
		} catch ( ... ) {
		}
#endif

		if ( "C" == name ) {
			mCurrentLanguage = "en";
		} else {
			mCurrentLanguage = name.substr( 0, 2 );
		}
	} else {
		mCurrentLanguage = name.substr( 0, 2 );
	}
}

std::string Translator::getDefaultLanguage() const {
	return mDefaultLanguage;
}

void Translator::setDefaultLanguage( const std::string& defaultLanguage ) {
	mDefaultLanguage = defaultLanguage;
}

std::string Translator::getCurrentLanguage() const {
	return mCurrentLanguage;
}

void Translator::setCurrentLanguage( const std::string& currentLanguage ) {
	mCurrentLanguage = currentLanguage;
}

class IOStreamXmlWriter : public pugi::xml_writer {
  public:
	IOStreamXmlWriter( IOStream& stream ) : mIOStream( stream ) {}

	virtual void write( const void* data, size_t size ) {
		mIOStream.write( (const char*)data, size );
	}

  private:
	IOStream& mIOStream;
};

void Translator::saveToStream( IOStream& stream, std::string lang ) {
	pugi::xml_document doc;

	auto resources = doc.append_child( "resources" );
	resources.append_attribute( "language" ).set_value( lang.c_str() );

	const auto& langStrs = mDictionary[lang];

	for ( const auto& str : langStrs ) {
		auto r = resources.append_child( "string" );
		auto d = r.append_child( pugi::node_cdata );
		r.append_attribute( "name" ).set_value( str.first.c_str() );
		d.set_value( str.second.toUtf8().c_str() );
	}

	IOStreamXmlWriter writer( stream );
	doc.save( writer );
}

}} // namespace EE::System
