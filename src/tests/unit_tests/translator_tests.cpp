#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/translator.hpp>

using namespace EE;
using namespace EE::System;

UTEST( Translator, ConstLookupUsesCurrentThenFallbackLanguageWithoutSavingDefaults ) {
	Translator translator;
	translator.setDefaultLanguage( "en" );
	translator.setCurrentLanguage( "fr" );
	translator.setString( "en", "hello", "Hello" );
	translator.setString( "en", "fallback", "English fallback" );
	translator.setString( "fr", "hello", "Bonjour" );
	translator.setSaveDefaultValues( true );
	const Translator& lookup = translator;
	EXPECT_TRUE( lookup.getString( "hello" ) == "Bonjour" );
	EXPECT_TRUE( lookup.getString( "fallback", "Inline fallback" ) == "English fallback" );
	EXPECT_TRUE( lookup.getString( "missing", "Inline fallback" ) == "Inline fallback" );
	EXPECT_TRUE( lookup.getString( "missing" ).empty() );
	EXPECT_EQ( 1u, translator.getDictionary().at( "fr" ).size() );
}

UTEST( Translator, MutableLookupPreservesDefaultRecordingBehavior ) {
	Translator translator;
	translator.setDefaultLanguage( "en" );
	translator.setCurrentLanguage( "fr" );
	translator.setString( "en", "fallback", "English fallback" );
	EXPECT_TRUE( translator.getString( "fallback", "Inline fallback" ) == "English fallback" );
	EXPECT_TRUE( translator.getDictionary().find( "fr" ) == translator.getDictionary().end() );
	translator.setSaveDefaultValues( true );
	EXPECT_TRUE( translator.getString( "fallback", "Inline fallback" ) == "Inline fallback" );
	EXPECT_TRUE( translator.getDictionary().at( "fr" ).at( "fallback" ) == "Inline fallback" );
	EXPECT_TRUE( translator.getString( "fallback", "Other fallback" ) == "Inline fallback" );
}

UTEST( Translator, NormalizesCatalogsAndFallsBackWithoutSwitchingScripts ) {
	Translator translator;
	EXPECT_TRUE( translator.loadFromString( R"(<resources language="ZH_hant">
		<string name="script">Traditional</string>
		<string name="first">Parent</string>
	</resources>)" ) );
	EXPECT_TRUE( translator.loadFromString( R"(<resources>
		<string name="first">Taiwan</string>
	</resources>)",
											"zh_HANT_tw" ) );
	translator.setString( "zh", "base", "Chinese" );
	translator.setString( "zh-Hans", "script-only", "Simplified" );
	translator.setString( "EN_us", "default-region", "Default region" );
	translator.setString( "en", "default-parent", "Default parent" );
	translator.setDefaultLanguage( "en_US.UTF-8" );
	translator.setCurrentLanguage( "ZH_hant_TW" );
	EXPECT_TRUE( translator.getCurrentLanguage() == "zh-Hant-TW" );
	EXPECT_TRUE( translator.getDefaultLanguage() == "en-US" );
	EXPECT_TRUE( translator.getString( "first" ) == "Taiwan" );
	EXPECT_TRUE( translator.getString( "script" ) == "Traditional" );
	EXPECT_TRUE( translator.getString( "base" ) == "Chinese" );
	EXPECT_TRUE( translator.getString( "default-region" ) == "Default region" );
	EXPECT_TRUE( translator.getString( "default-parent" ) == "Default parent" );
	EXPECT_TRUE( translator.getString( "script-only", "Absent" ) == "Absent" );
	translator.setCurrentLanguage( "zh_TW.UTF-8" );
	EXPECT_TRUE( translator.getCurrentLanguage() == "zh-TW" );
	EXPECT_TRUE( translator.getString( "script" ) == "Traditional" );
	EXPECT_TRUE( translator.getString( "first" ) == "Taiwan" );
	translator.setCurrentLanguage( "zh_CN" );
	EXPECT_TRUE( translator.getString( "script-only", "Absent" ) == "Absent" );
	EXPECT_TRUE( translator.getString( "base" ) == "Chinese" );
	translator.setCurrentLanguage( "ZH_hant_TW" );
	translator.setCurrentLanguage( "invalid!" );
	EXPECT_TRUE( translator.getCurrentLanguage() == "zh-Hant-TW" );
	translator.setSaveDefaultValues( true );
	EXPECT_TRUE( translator.getString( "script", "Inline" ) == "Traditional" );
	EXPECT_EQ( 1u, translator.getDictionary().at( "zh-Hant-TW" ).size() );
}

UTEST( Translator, RejectsCollidingCatalogIdsAndStillAcceptsLegacyFiles ) {
	const auto directory = FileSystem::createTempDirectory( "eepp-locale-" );
	ASSERT_FALSE( directory.empty() );
	const auto base = directory + FileSystem::getOSSlash();
	const auto common = base + "de.xml";
	const auto region = base + "es-AR.xml";
	const auto alias = base + "ES_ar.xml";
	const std::string catalog = R"(<resources><string name="value">Catalog</string></resources>)";
	EXPECT_TRUE( FileSystem::fileWrite( common, catalog ) );
	EXPECT_TRUE( FileSystem::fileWrite( region, catalog ) );
	EXPECT_TRUE( FileSystem::fileWrite( alias, catalog ) );
	Translator translator;
	EXPECT_TRUE( translator.loadFromDirectory( directory ) );
	EXPECT_TRUE( translator.getDictionary().empty() );

	FileSystem::fileRemove( alias );
	EXPECT_FALSE( translator.loadFromDirectory( directory ) );
	translator.setCurrentLanguage( "de-DE" );
	EXPECT_TRUE( translator.getString( "value" ) == "Catalog" );
	translator.setCurrentLanguage( "es_AR" );
	EXPECT_TRUE( translator.getString( "value" ) == "Catalog" );

	// The declared language outranks the file name, which can be any word.
	const auto words = base + "strings.xml";
	const auto renamed = base + "pt.xml";
	EXPECT_TRUE( FileSystem::fileWrite(
		words, std::string(
				   R"(<resources language="fr"><string name="word">Mot</string></resources>)" ) ) );
	EXPECT_TRUE( FileSystem::fileWrite(
		renamed,
		std::string(
			R"(<resources language="it"><string name="word">Parola</string></resources>)" ) ) );
	Translator declared;
	EXPECT_TRUE( declared.loadFromFile( words ) );
	EXPECT_TRUE( declared.loadFromFile( renamed ) );
	EXPECT_TRUE( declared.getDictionary().at( "fr" ).at( "word" ) == "Mot" );
	EXPECT_TRUE( declared.getDictionary().at( "it" ).at( "word" ) == "Parola" );
	EXPECT_EQ( 2u, declared.getDictionary().size() );
	EXPECT_TRUE( declared.loadFromFile( renamed, "pt" ) );
	EXPECT_TRUE( declared.getDictionary().at( "pt" ).at( "word" ) == "Parola" );

	FileSystem::fileRemove( words );
	FileSystem::fileRemove( renamed );
	FileSystem::fileRemove( common );
	FileSystem::fileRemove( region );
	FileSystem::dirRemoveAll( directory );
}

UTEST( Translator, SelectsPluralTemplatesWithIndependentRulesAndCatalogFallback ) {
	Translator translator;
	translator.setCurrentLanguage( "pt-PT" );
	EXPECT_TRUE( translator.loadFromString( R"(<resources language="pt-PT">
		<plurals name="files"><item quantity="other">Portugal other</item></plurals>
	</resources>)" ) );
	EXPECT_TRUE( translator.loadFromString( R"(<resources language="pt">
		<plurals name="files"><item quantity="one">Brazil one</item></plurals>
		<plurals name="folders"><item quantity="one">Portuguese one</item>
		<item quantity="other">Portuguese other</item></plurals>
	</resources>)" ) );
	EXPECT_TRUE( translator.loadFromString( R"(<resources language="en">
		<plurals name="remaining"><item quantity="one">%lld item</item>
		<item quantity="other">%lld items</item></plurals>
	</resources>)" ) );

	EXPECT_TRUE( translator.getPlural( "files", 1 ) == "Portugal other" );
	EXPECT_TRUE( translator.getPlural( "folders", 0 ) == "Portuguese other" );
	EXPECT_TRUE( translator.getPlural( "folders", 0, {}, LocaleId( "pt-BR" ) ) ==
				 "Portuguese one" );
	EXPECT_TRUE( translator.getPlural( "remaining", 1 ) == "%lld item" );
	EXPECT_TRUE( translator.getPlural( "remaining", "1.0" ) == "%lld items" );
	EXPECT_TRUE( translator.getPlural( "remaining", 1, {}, LocaleId( "ja" ) ) == "%lld item" );
	EXPECT_TRUE( translator.getPlural( "missing", 1, "Missing" ) == "Missing" );
	EXPECT_TRUE( translator.getString( "remaining" ).empty() );
}

UTEST( Translator, SavesPluralTemplatesAndKeepsInvalidLoadsAtomic ) {
	Translator translator;
	translator.setCurrentLanguage( "fr_CA" );
	EXPECT_FALSE( translator.loadFromString( R"(<resources language="fr-CA">
		<string name="leak">Invalid catalog</string>
		<plurals name="files"><item quantity="one">A</item><item quantity="one">B</item></plurals>
	</resources>)" ) );
	EXPECT_TRUE( translator.getString( "leak" ).empty() );
	EXPECT_FALSE( translator.loadFromString( R"(<resources language="fr-CA">
		<plurals name="bad"><item quantity="invalid">A</item></plurals>
	</resources>)" ) );
	EXPECT_TRUE( translator.loadFromString( R"(<resources language="fr_CA">
		<string name="files">Ordinary string</string>
		<plurals name="files"><item quantity="one">One</item><item quantity="other">Other</item></plurals>
		<plurals name="files.ext"><item quantity="other">Extended key</item></plurals>
	</resources>)" ) );
	EXPECT_EQ( 1u, translator.getDictionary().at( "fr-CA" ).size() );
	EXPECT_EQ( 2u, translator.getPluralDictionary().at( "fr-CA" ).size() );
	const auto& forms = translator.getPluralDictionary().at( "fr-CA" ).at( "files" );
	EXPECT_TRUE( forms.get( PluralCategory::One ) && *forms.get( PluralCategory::One ) == "One" );
	EXPECT_TRUE( forms.get( PluralCategory::Many ) == nullptr );
	translator.setPlural( "fr_CA", "files", PluralCategory::Many, "Many" );
	translator.setPlural( "invalid!", "files", PluralCategory::Many, "Ignored" );
	translator.setPlural( "fr_CA", "", PluralCategory::One, "Ignored" );
	translator.setPlural( "fr_CA", std::string( "files\0hidden", 12 ), PluralCategory::One,
						  "Ignored" );
	translator.setPlural( "fr_CA", std::string( "\0files", 6 ), PluralCategory::One, "Ignored" );
	translator.setString( "fr_CA", std::string( "files\0hidden", 12 ), "Ignored" );
	translator.setString( "fr_CA", "", "Ignored" );
	EXPECT_EQ( 1u, translator.getDictionary().at( "fr-CA" ).size() );
	translator.setPlural( "fr_CA", "files", static_cast<PluralCategory>( 6 ), "Ignored" );
	translator.setPlural( "fr_CA", "files", static_cast<PluralCategory>( 255 ), "Ignored" );
	EXPECT_EQ( 2u, translator.getPluralDictionary().at( "fr-CA" ).size() );
	EXPECT_TRUE( forms.get( static_cast<PluralCategory>( 6 ) ) == nullptr );
	EXPECT_TRUE( forms.get( static_cast<PluralCategory>( 255 ) ) == nullptr );
	EXPECT_EQ( 1u, translator.getPluralDictionary().size() );
	EXPECT_TRUE( translator.getPlural( "files", 1000000 ) == "Many" );
	IOStreamString stream;
	translator.saveToStream( stream, "FR_ca" );

	Translator reloaded;
	reloaded.setCurrentLanguage( "fr-CA" );
	EXPECT_TRUE( reloaded.loadFromString( stream.getStream() ) );
	EXPECT_TRUE( reloaded.getString( "files" ) == "Ordinary string" );
	EXPECT_TRUE( reloaded.getPlural( "files", 1 ) == "One" );
	EXPECT_TRUE( reloaded.getPlural( "files", 2 ) == "Other" );
	EXPECT_TRUE( reloaded.getPlural( "files.ext", 1 ) == "Extended key" );
	EXPECT_TRUE( reloaded.getPlural( "files", 1000000 ) == "Many" );
	EXPECT_EQ( 1u, reloaded.getDictionary().at( "fr-CA" ).size() );
}

UTEST( Translator, DirectoryCollisionsUseResolvedCatalogLanguages ) {
	const auto directory = FileSystem::createTempDirectory( "eepp-locale-" );
	ASSERT_FALSE( directory.empty() );
	const auto base = directory + FileSystem::getOSSlash();
	const auto catalog = []( const char* language, const char* value ) {
		return std::string( "<resources language=\"" ) + language + "\"><string name=\"value\">" +
			   value + "</string></resources>";
	};

	// Different file names that both declare fr would overwrite each other's keys.
	const auto words = base + "strings.xml";
	const auto french = base + "fr.xml";
	EXPECT_TRUE( FileSystem::fileWrite( words, catalog( "fr", "Mots" ) ) );
	EXPECT_TRUE( FileSystem::fileWrite( french, catalog( "fr", "Français" ) ) );
	Translator colliding;
	EXPECT_TRUE( colliding.loadFromDirectory( directory ) );
	EXPECT_TRUE( colliding.getDictionary().empty() );
	FileSystem::fileRemove( words );
	FileSystem::fileRemove( french );

	// Colliding file names are fine when the catalogs declare different languages.
	const auto region = base + "es-AR.xml";
	const auto alias = base + "ES_ar.xml";
	EXPECT_TRUE( FileSystem::fileWrite( region, catalog( "es-AR", "Argentina" ) ) );
	EXPECT_TRUE( FileSystem::fileWrite( alias, catalog( "pt", "Português" ) ) );
	Translator distinct;
	EXPECT_FALSE( distinct.loadFromDirectory( directory ) );
	EXPECT_TRUE( distinct.getDictionary().at( "es-AR" ).at( "value" ) == "Argentina" );
	EXPECT_TRUE( distinct.getDictionary().at( "pt" ).at( "value" ) == "Português" );

	// A file that fails to parse is reported without blocking the others.
	const auto broken = base + "de.xml";
	EXPECT_TRUE( FileSystem::fileWrite( broken, std::string( "<resources" ) ) );
	Translator partial;
	EXPECT_TRUE( partial.loadFromDirectory( directory ) );
	EXPECT_EQ( 2u, partial.getDictionary().size() );

	FileSystem::fileRemove( broken );
	FileSystem::fileRemove( region );
	FileSystem::fileRemove( alias );
	FileSystem::dirRemoveAll( directory );
}
