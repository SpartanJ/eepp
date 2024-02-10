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
	typedef std::unordered_map<std::string, String> StringDictionary;
	typedef std::unordered_map<std::string, StringDictionary> StringLocaleDictionary;

	Translator( const std::locale& locale = std::locale() );

	bool loadFromDirectory( std::string dirPath, std::string ext = "xml" );

	bool loadFromFile( const std::string& path, std::string lang = "" );

	bool loadFromString( const std::string& string, std::string lang = "" );

	bool loadFromMemory( const void* buffer, Int32 bufferSize, std::string lang = "" );

	bool loadFromStream( IOStream& stream, std::string lang = "" );

	bool loadFromPack( Pack* pack, const std::string& FilePackPath, std::string lang = "" );

	String getString( const std::string& key, const String& defaultValue = String() );

	void setString( const std::string& lang, const std::string& key, const String& val );

	template <typename... Args> String getStringf( const char* key, Args&&... args ) {
		return String::format(
			std::string_view{ getString( key ).toUtf8() },
			FormatArg<std::decay_t<Args>>::get( std::forward<Args>( args ) )... );
	}

	void setLanguageFromLocale( std::locale locale );

	std::string getDefaultLanguage() const;

	void setDefaultLanguage( const std::string& defaultLanguage );

	std::string getCurrentLanguage() const;

	void setCurrentLanguage( const std::string& currentLanguage );

	void setSaveDefaultValues( bool set ) { mSetDefaultValues = set; }

	bool isSetDefaultValues() const { return mSetDefaultValues; }

	void saveToStream( IOStream& stream, std::string lang = "" );

	const StringLocaleDictionary& getDictionary() const { return mDictionary; }

	StringLocaleDictionary& getDictionary() { return mDictionary; }

	void setLanguageName( const std::string& id, const std::string& name );

	std::unordered_map<std::string, std::string> getLanguageNames() const;

  protected:
	std::string mDefaultLanguage;
	std::string mCurrentLanguage;
	StringLocaleDictionary mDictionary;
	std::unordered_map<std::string, std::string> mLangNames;
	bool mSetDefaultValues{ false };

	bool loadNodes( pugi::xml_node node, std::string lang = "" );
};

}} // namespace EE::System

#endif
