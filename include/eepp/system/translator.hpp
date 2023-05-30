#ifndef EE_SYSTEM_STRINGLOCALERESOURCE_HPP
#define EE_SYSTEM_STRINGLOCALERESOURCE_HPP

#include <eepp/core.hpp>

namespace pugi {
class xml_node;
}

namespace EE { namespace System {

class IOStream;
class Pack;

class EE_API Translator {
  public:
	Translator( const std::locale& locale = std::locale() );

	bool loadFromDirectory( std::string dirPath, std::string ext = "xml" );

	bool loadFromFile( const std::string& path, std::string lang = "" );

	bool loadFromString( const std::string& string, std::string lang = "" );

	bool loadFromMemory( const void* buffer, Int32 bufferSize, std::string lang = "" );

	bool loadFromStream( IOStream& stream, std::string lang = "" );

	bool loadFromPack( Pack* pack, const std::string& FilePackPath, std::string lang = "" );

	String getString( const std::string& key, const String& defaultValue = String() );

	String getStringf( const char* key, ... );

	void setLanguageFromLocale( std::locale locale );

	std::string getDefaultLanguage() const;

	void setDefaultLanguage( const std::string& defaultLanguage );

	std::string getCurrentLanguage() const;

	void setCurrentLanguage( const std::string& currentLanguage );

  protected:
	typedef std::unordered_map<std::string, String> StringDictionary;
	typedef std::unordered_map<std::string, StringDictionary> StringLocaleDictionary;

	std::string mDefaultLanguage;
	std::string mCurrentLanguage;
	StringLocaleDictionary mDictionary;

	bool loadNodes( pugi::xml_node node, std::string lang = "" );
};

}} // namespace EE::System

#endif
