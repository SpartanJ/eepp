#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packregistry.hpp>
#include <eepp/system/translator.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

#include <algorithm>
#include <array>
#include <deque>
#include <unordered_set>

namespace EE::System {

namespace {

constexpr std::array<std::string_view, 6> quantities{ "zero", "one",  "two",
													  "few",  "many", "other" };

std::string catalogLanguage( std::string_view language ) {
	return std::string( LocaleId( language ).getTag() );
}

std::string catalogLanguageFromPath( const std::string& path ) {
	return catalogLanguage(
		FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );
}

// Searches the normalized language and then each LocaleId::fallback() candidate. The exact
// catalog is tried before parsing, since most lookups end there. When coveredLanguage is given,
// coversLanguage reports whether the search visited that language and therefore its chain.
template <typename Dictionaries, typename Lookup>
const String* findInCatalogs( const Dictionaries& dictionaries, const std::string& language,
							  const Lookup& lookup, const std::string* coveredLanguage = nullptr,
							  bool* coversLanguage = nullptr ) {
	if ( language.empty() )
		return nullptr;
	const auto visit = [&]( const std::string& tag ) -> const String* {
		if ( coveredLanguage && tag == *coveredLanguage )
			*coversLanguage = true;
		const auto found = dictionaries.find( tag );
		return found != dictionaries.end() ? lookup( found->second ) : nullptr;
	};
	if ( const auto* value = visit( language ) )
		return value;
	std::string tag;
	for ( auto id = LocaleId( language ).fallback(); id.isValid(); id = id.fallback() ) {
		tag.assign( id.getTag() );
		if ( const auto* value = visit( tag ) )
			return value;
	}
	return nullptr;
}

const String* findString( const Translator::StringDictionary& dictionary, const std::string& key ) {
	auto found = dictionary.find( key );
	return found != dictionary.end() ? &found->second : nullptr;
}

bool isResources( pugi::xml_node node ) {
	return String::icompare( node.name(), "resources" ) == 0;
}

// An explicit language overrides the catalog. Otherwise the catalog's own declaration outranks
// its file name, which is only a fallback. Empty when no valid language is available.
std::string resourceLanguage( pugi::xml_node resources, const std::string& explicitId,
							  const std::string& fileLanguage ) {
	if ( !explicitId.empty() )
		return explicitId;
	auto declared = catalogLanguage( resources.attribute( "language" ).as_string() );
	return declared.empty() ? fileLanguage : declared;
}

void logParseError( const pugi::xml_parse_result& result ) {
	Log::error( "Error description: %s", result.description() );
	Log::error( "Error offset: %d", result.offset );
}

// Keys must survive saveToStream() and loading: XML attributes cannot hold NUL, and the loader
// skips or rejects an empty name.
bool isSerializableKey( const std::string& key ) {
	return !key.empty() && key.find( '\0' ) == std::string::npos;
}

} // namespace

Translator::Translator() :
	mDefaultLanguage( "en" ), mCurrentLanguage( LocaleId::systemUiLanguage().getTag() ) {}

bool Translator::loadFromDirectory( std::string dirPath, std::string ext ) {
	FileSystem::dirAddSlashAtEnd( dirPath );
	if ( !FileSystem::isDirectory( dirPath ) )
		return false;

	String::toLowerInPlace( ext );
	auto files = FileSystem::filesGetInPath( dirPath, true, true );
	files.erase( std::remove_if( files.begin(), files.end(),
								 [&ext]( const std::string& file ) {
									 return FileSystem::fileExtension( file ) != ext;
								 } ),
				 files.end() );

	// Parse every catalog first, so collisions between the languages the catalogs resolve to are
	// rejected before any of them changes the dictionaries.
	struct Catalog {
		std::string fileLanguage;
		pugi::xml_document document;
	};
	// A deque keeps each parsed document in place; moving an xml_document is avoidable work.
	std::deque<Catalog> catalogs;
	std::unordered_map<std::string, std::string> owners;
	bool someFailed = false;
	for ( const auto& file : files ) {
		auto& catalog = catalogs.emplace_back();
		const auto path = dirPath + file;
		if ( const auto result = catalog.document.load_file( path.c_str() ); !result ) {
			Log::error( "Couldn't load i18n file: %s", path.c_str() );
			logParseError( result );
			catalogs.pop_back();
			someFailed = true;
			continue;
		}
		catalog.fileLanguage = catalogLanguageFromPath( file );
		std::unordered_set<std::string> languages;
		for ( auto resources = catalog.document.first_child(); resources;
			  resources = resources.next_sibling() ) {
			if ( isResources( resources ) ) {
				if ( auto id = resourceLanguage( resources, {}, catalog.fileLanguage );
					 !id.empty() )
					languages.emplace( std::move( id ) );
			}
		}
		for ( const auto& language : languages ) {
			const auto [owner, added] = owners.try_emplace( language, file );
			if ( !added ) {
				Log::error( "Duplicate i18n catalog language %s in %s and %s in %s",
							language.c_str(), owner->second.c_str(), file.c_str(),
							dirPath.c_str() );
				return true;
			}
		}
	}

	for ( const auto& catalog : catalogs ) {
		if ( !loadNodes( catalog.document.first_child(), {}, catalog.fileLanguage ) )
			someFailed = true;
	}
	return someFailed;
}

bool Translator::loadNodes( pugi::xml_node node, const std::string& lang,
							const std::string& fileLanguage ) {
	const auto explicitId = catalogLanguage( lang );
	if ( !lang.empty() && explicitId.empty() ) {
		Log::error( "Invalid i18n language: %s", lang.c_str() );
		return false;
	}

	std::unordered_map<std::string, std::string_view> spellings;
	for ( auto resources = node; resources; resources = resources.next_sibling() ) {
		if ( !isResources( resources ) )
			continue;
		const auto id = resourceLanguage( resources, explicitId, fileLanguage );
		if ( id.empty() ) {
			Log::error( "Couldn't load i18n catalog: missing or invalid resources language." );
			return false;
		}
		if ( explicitId.empty() && !fileLanguage.empty() && id != fileLanguage ) {
			Log::warning( "i18n catalog declares language %s, not its file name language %s.",
						  id.c_str(), fileLanguage.c_str() );
		}
		if ( explicitId.empty() ) {
			const std::string_view spelling = resources.attribute( "language" ).as_string();
			auto [found, added] = spellings.try_emplace( id, spelling );
			if ( !added && found->second != spelling ) {
				Log::error( "Duplicate normalized i18n catalog language: %s", id.c_str() );
				return false;
			}
		}
		for ( auto group : resources.children( "plurals" ) ) {
			const char* key = group.attribute( "name" ).as_string();
			if ( !*key || !group.child( "item" ) ) {
				Log::error( "Invalid i18n plurals entry: expected a name and at least one item." );
				return false;
			}
			std::array<bool, quantities.size()> seen{};
			for ( auto item : group.children( "item" ) ) {
				const std::string_view quantity = item.attribute( "quantity" ).as_string();
				const auto index = std::find( quantities.begin(), quantities.end(), quantity ) -
								   quantities.begin();
				if ( index == static_cast<std::ptrdiff_t>( quantities.size() ) || seen[index] ) {
					Log::error( "Invalid or duplicate i18n plural quantity '%s' in %s.",
								quantity.data(), key );
					return false;
				}
				seen[index] = true;
			}
		}
	}

	for ( auto resources = node; resources; resources = resources.next_sibling() ) {
		if ( !isResources( resources ) )
			continue;
		const auto id = resourceLanguage( resources, explicitId, fileLanguage );
		for ( auto string : resources.children( "string" ) ) {
			const char* key = string.attribute( "name" ).as_string();
			if ( *key )
				mDictionary[id][key] = String( string.text().as_string() );
		}
		for ( auto group : resources.children( "plurals" ) ) {
			auto& forms = mPlurals[id][group.attribute( "name" ).as_string()];
			for ( auto item : group.children( "item" ) ) {
				const std::string_view quantity = item.attribute( "quantity" ).as_string();
				const auto index = std::find( quantities.begin(), quantities.end(), quantity ) -
								   quantities.begin();
				forms.set( static_cast<PluralCategory>( index ),
						   String( item.text().as_string() ) );
			}
		}
	}
	return true;
}

bool Translator::loadFromFile( const std::string& path, std::string lang ) {
	if ( FileSystem::fileExists( path ) ) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( path.c_str() );

		if ( result ) {
			return loadNodes( doc.first_child(), lang, catalogLanguageFromPath( path ) );
		} else {
			Log::error( "Couldn't load i18n file: %s", path.c_str() );
			logParseError( result );
		}
	} else if ( PackRegistry::instance()->isFallbackToPacksActive() ) {
		std::string packPath( path );
		Pack* pack = PackRegistry::instance()->exists( packPath );

		if ( NULL != pack ) {
			return loadFromPack( pack, packPath, std::move( lang ) );
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
		logParseError( result );
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
		logParseError( result );
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
		logParseError( result );
	}

	return false;
}

bool Translator::loadFromPack( Pack* pack, const std::string& FilePackPath, std::string lang ) {
	ScopedBuffer buffer;

	if ( pack->isOpen() && pack->extractFileToMemory( FilePackPath, buffer ) ) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_buffer( buffer.get(), buffer.length() );
		if ( result )
			return loadNodes( doc.first_child(), lang, catalogLanguageFromPath( FilePackPath ) );
		Log::error( "Couldn't load i18n file from pack: %s", FilePackPath.c_str() );
		logParseError( result );
	}

	return false;
}

String Translator::getString( const std::string& key, const String& defaultValue ) {
	if ( !mSetDefaultValues || defaultValue.empty() )
		return static_cast<const Translator&>( *this ).getString( key, defaultValue );

	if ( const auto* value = findInCatalogs( mDictionary, mCurrentLanguage,
											 [&key]( const StringDictionary& dictionary ) {
												 return findString( dictionary, key );
											 } ) )
		return *value;
	if ( !mCurrentLanguage.empty() )
		mDictionary[mCurrentLanguage][key] = defaultValue;
	return defaultValue;
}

String Translator::getString( const std::string& key, const String& defaultValue ) const {
	auto lookup = [&key]( const StringDictionary& dictionary ) {
		return findString( dictionary, key );
	};
	bool coveredDefault = false;
	if ( const auto* value = findInCatalogs( mDictionary, mCurrentLanguage, lookup,
											 &mDefaultLanguage, &coveredDefault ) )
		return *value;
	// The current chain already searched the default chain when it passed the default language.
	if ( !coveredDefault ) {
		if ( const auto* value = findInCatalogs( mDictionary, mDefaultLanguage, lookup ) )
			return *value;
	}
	return defaultValue;
}

String Translator::getPlural( const std::string& key, std::string_view count,
							  const String& defaultValue, LocaleId pluralLocale ) const {
	auto lookup = [&]( const std::string& language, LocaleId ruleLocale ) {
		const auto category = PluralRules::select( ruleLocale, count );
		return findInCatalogs(
			mPlurals, language, [&]( const PluralDictionary& dictionary ) -> const String* {
				const auto found = dictionary.find( key );
				if ( found == dictionary.end() )
					return nullptr;
				const auto* value = found->second.get( category );
				return value ? value : found->second.get( PluralCategory::Other );
			} );
	};

	if ( !pluralLocale.isValid() )
		pluralLocale = LocaleId( mCurrentLanguage );
	if ( const auto* value = lookup( mCurrentLanguage, pluralLocale ) )
		return *value;
	if ( const auto* value = lookup( mDefaultLanguage, LocaleId( mDefaultLanguage ) ) )
		return *value;
	return defaultValue;
}

void Translator::setString( const std::string& lang, const std::string& key, const String& val ) {
	auto language = catalogLanguage( lang );
	if ( !language.empty() && isSerializableKey( key ) )
		mDictionary[std::move( language )][key] = val;
}

void Translator::setPlural( const std::string& lang, const std::string& key,
							PluralCategory category, const String& val ) {
	auto language = catalogLanguage( lang );
	if ( !language.empty() && isSerializableKey( key ) && PluralForms::isValid( category ) )
		mPlurals[std::move( language )][key].set( category, val );
}

std::string Translator::getDefaultLanguage() const {
	return mDefaultLanguage;
}

void Translator::setDefaultLanguage( const std::string& defaultLanguage ) {
	auto id = catalogLanguage( defaultLanguage );
	if ( !id.empty() || defaultLanguage.empty() )
		mDefaultLanguage = std::move( id );
}

std::string Translator::getCurrentLanguage() const {
	return mCurrentLanguage;
}

void Translator::setCurrentLanguage( const std::string& currentLanguage ) {
	auto id = catalogLanguage( currentLanguage );
	if ( !id.empty() || currentLanguage.empty() )
		mCurrentLanguage = std::move( id );
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
	lang = lang.empty() ? mCurrentLanguage : catalogLanguage( lang );
	if ( lang.empty() )
		return;

	pugi::xml_document doc;
	auto resources = doc.append_child( "resources" );
	resources.append_attribute( "language" ).set_value( lang.c_str() );
	if ( auto name = mLangNames.find( lang ); name != mLangNames.end() )
		resources.append_attribute( "title" ).set_value( name->second.c_str() );

	// Sorted output keeps saved catalogs stable across runs and hash-table implementations.
	const auto sorted = []( const auto& dictionary ) {
		std::vector<const typename std::decay_t<decltype( dictionary )>::value_type*> entries;
		entries.reserve( dictionary.size() );
		for ( const auto& entry : dictionary )
			entries.push_back( &entry );
		std::sort( entries.begin(), entries.end(),
				   []( const auto* lhs, const auto* rhs ) { return lhs->first < rhs->first; } );
		return entries;
	};
	const auto appendText = []( pugi::xml_node element, const String& value ) {
		element.append_child( pugi::node_pcdata ).set_value( value.toUtf8().c_str() );
	};

	if ( auto catalog = mDictionary.find( lang ); catalog != mDictionary.end() ) {
		for ( const auto* entry : sorted( catalog->second ) ) {
			auto element = resources.append_child( "string" );
			element.append_attribute( "name" ).set_value( entry->first.c_str() );
			appendText( element, entry->second );
		}
	}
	if ( auto catalog = mPlurals.find( lang ); catalog != mPlurals.end() ) {
		for ( const auto* entry : sorted( catalog->second ) ) {
			auto group = resources.append_child( "plurals" );
			group.append_attribute( "name" ).set_value( entry->first.c_str() );
			for ( std::size_t index = 0; index < quantities.size(); ++index ) {
				const auto* value = entry->second.get( static_cast<PluralCategory>( index ) );
				if ( !value )
					continue;
				auto item = group.append_child( "item" );
				item.append_attribute( "quantity" ).set_value( quantities[index].data() );
				appendText( item, *value );
			}
		}
	}

	IOStreamXmlWriter writer( stream );
	doc.save( writer );
}

void Translator::setLanguageName( const std::string& id, const std::string& name ) {
	auto language = catalogLanguage( id );
	if ( !language.empty() )
		mLangNames[std::move( language )] = name;
}

std::unordered_map<std::string, std::string> Translator::getLanguageNames() const {
	return mLangNames;
}

} // namespace EE::System
