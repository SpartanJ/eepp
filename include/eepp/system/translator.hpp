#ifndef EE_SYSTEM_STRINGLOCALERESOURCE_HPP
#define EE_SYSTEM_STRINGLOCALERESOURCE_HPP

#include <eepp/core.hpp>

namespace pugi {
class xml_node;
}

namespace  EE { namespace System {

class IOStream;
class Pack;

class Translator {
	public:
		typedef std::map<std::string, String> StringDictionary;
		typedef std::map<std::string, StringDictionary> StringLocaleDictionary;

		Translator( const std::locale& locale = std::locale() );

		void loadFromDirectory( std::string dirPath, std::string ext = "xml" );

		void loadFromFile( const std::string& path, std::string lang = "" );

		void loadFromString( const std::string& string, std::string lang = "" );

		void loadFromMemory( const void * buffer, Int32 bufferSize, std::string lang = "" );

		void loadFromStream( IOStream& stream, std::string lang = "" );

		void loadFromPack( Pack * pack, const std::string& FilePackPath, std::string lang = "" );

		String getString( const std::string& key );

		String getStringf( const char * key, ... );

		void setLanguageFromLocale( std::locale locale );

		std::string getDefaultLanguage() const;

		void setDefaultLanguage( const std::string& defaultLanguage );

		std::string getCurrentLanguage() const;

		void setCurrentLanguage( const std::string& currentLanguage );
	protected:
		std::string mDefaultLanguage;
		std::string mCurrentLanguage;
		StringLocaleDictionary mDictionary;

		void loadNodes( pugi::xml_node node, std::string lang = "" );
};

}}

#endif

