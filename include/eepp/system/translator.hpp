#ifndef EE_SYSTEM_STRINGLOCALERESOURCE_HPP
#define EE_SYSTEM_STRINGLOCALERESOURCE_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/pluralrules.hpp>

#include <array>
#include <functional>
#include <unordered_map>

namespace pugi {

class xml_node;

} // namespace pugi

namespace EE::System {

class IOStream;
class Pack;

class EE_API Translator {
  public:
	typedef std::unordered_map<std::string, String> StringDictionary;
	typedef std::unordered_map<std::string, StringDictionary> StringLocaleDictionary;

	/** The templates of one <plurals> entry, indexed by PluralCategory. */
	class PluralForms {
	  public:
		/** False for values outside the PluralCategory enumerators. */
		static bool isValid( PluralCategory category ) {
			return static_cast<Uint8>( category ) <= static_cast<Uint8>( PluralCategory::Other );
		}

		/** Null when the catalog does not provide the category, or the category is invalid. */
		const String* get( PluralCategory category ) const {
			const auto index = static_cast<Uint8>( category );
			return isValid( category ) && ( ( mPresent >> index ) & 1u ) ? &mForms[index] : nullptr;
		}

		/** An invalid category is ignored. */
		void set( PluralCategory category, String value ) {
			if ( !isValid( category ) )
				return;
			const auto index = static_cast<Uint8>( category );
			mForms[index] = std::move( value );
			mPresent |= static_cast<Uint8>( 1u << index );
		}

	  private:
		std::array<String, 6> mForms;
		Uint8 mPresent{ 0 };
	};

	typedef std::unordered_map<std::string, PluralForms> PluralDictionary;
	typedef std::unordered_map<std::string, PluralDictionary> PluralLocaleDictionary;

	Translator();

	/** Loads catalog files of the given extension. Returns true if any catalog failed. When two
	 * files resolve to the same language, as loadFromFile() resolves it, nothing is loaded. */
	bool loadFromDirectory( std::string dirPath, std::string ext = "xml" );

	/** A nonempty lang overrides the catalog. Otherwise each <resources language="…"> is used,
	 * and the file name supplies the language only where that attribute is absent or invalid. */
	bool loadFromFile( const std::string& path, std::string lang = "" );

	bool loadFromString( const std::string& string, std::string lang = "" );

	bool loadFromMemory( const void* buffer, Int32 bufferSize, std::string lang = "" );

	bool loadFromStream( IOStream& stream, std::string lang = "" );

	bool loadFromPack( Pack* pack, const std::string& FilePackPath, std::string lang = "" );

	String getString( const std::string& key, const String& defaultValue = String() );

	/** Read-only lookup, even when saving defaults is enabled. Concurrent const lookups are
	 * safe while the language and dictionaries remain unchanged. */
	String getString( const std::string& key, const String& defaultValue = String() ) const;

	/** Selects a template without substituting the count. Each catalog tries the selected category
	 * and then other before continuing to its parent. pluralLocale overrides the rule locale for
	 * the current language chain. The default language chain uses its own rules. */
	String getPlural( const std::string& key, std::string_view count,
					  const String& defaultValue = {}, LocaleId pluralLocale = {} ) const;

	template <NumericInteger Integer>
	String getPlural( const std::string& key, Integer count, const String& defaultValue = {},
					  LocaleId pluralLocale = {} ) const {
		return getPlural( key, String::toString( WideInteger<Integer>( count ) ), defaultValue,
						  pluralLocale );
	}

	/** An invalid language, or a key that is empty or contains NUL, is ignored, since the entry
	 * could not be saved and loaded again. */
	void setString( const std::string& lang, const std::string& key, const String& val );

	/** Adds or replaces one template of a plural entry. An invalid language, a key that is empty
	 * or contains NUL, or an invalid category is ignored, since the entry could not be saved and
	 * loaded again. */
	void setPlural( const std::string& lang, const std::string& key, PluralCategory category,
					const String& val );

	template <typename... Args> String getStringf( const char* key, Args&&... args ) {
		return String::format(
			std::string_view{ getString( key ).toUtf8() },
			FormatArg<std::decay_t<Args>>::get( std::forward<Args>( args ) )... );
	}

	std::string getDefaultLanguage() const;

	/** Empty disables the default language. An invalid nonempty ID is ignored. */
	void setDefaultLanguage( const std::string& defaultLanguage );

	std::string getCurrentLanguage() const;

	/** Empty clears the current language. An invalid nonempty ID is ignored. */
	void setCurrentLanguage( const std::string& currentLanguage );

	void setSaveDefaultValues( bool set ) { mSetDefaultValues = set; }

	bool isSetDefaultValues() const { return mSetDefaultValues; }

	void saveToStream( IOStream& stream, std::string lang = "" );

	/** The <string> entries of every catalog. Plural entries are in getPluralDictionary(). */
	const StringLocaleDictionary& getDictionary() const { return mDictionary; }

	StringLocaleDictionary& getDictionary() { return mDictionary; }

	const PluralLocaleDictionary& getPluralDictionary() const { return mPlurals; }

	void setLanguageName( const std::string& id, const std::string& name );

	std::unordered_map<std::string, std::string> getLanguageNames() const;

  protected:
	std::string mDefaultLanguage;
	std::string mCurrentLanguage;
	StringLocaleDictionary mDictionary;
	PluralLocaleDictionary mPlurals;
	std::unordered_map<std::string, std::string> mLangNames;
	bool mSetDefaultValues{ false };

	bool loadNodes( pugi::xml_node node, const std::string& lang,
					const std::string& fileLanguage = {} );
};

} // namespace EE::System

#endif
