#include "utest.hpp"
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
