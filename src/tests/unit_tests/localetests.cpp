#include "scopedenvironmentvariable.hpp"
#include "utest.hpp"
#include <eepp/system/locale.hpp>
#include <limits>

using namespace EE;
using namespace EE::System;

namespace {

template <typename T>
concept FormatsAsNumber = requires( const Locale& locale, T value ) {
	locale.formatNumber( value );
	locale.formatPercent( value );
};

static_assert( FormatsAsNumber<int> && FormatsAsNumber<Uint64> && FormatsAsNumber<Int8> &&
			   FormatsAsNumber<double> );
static_assert( !FormatsAsNumber<bool> && !FormatsAsNumber<char> && !FormatsAsNumber<char32_t> );
static_assert( FormatsAsNumber<long> && FormatsAsNumber<unsigned long> &&
			   FormatsAsNumber<long long> );

} // namespace

UTEST( Locale, NormalizesLanguageScriptRegionAndPosixNames ) {
	struct Sample {
		const char* input;
		const char* normalized;
	};
	for ( const Sample sample :
		  { Sample{ "en", "en" }, Sample{ "ES_ar", "es-AR" }, Sample{ "PT-br", "pt-BR" },
			Sample{ "pt-PT", "pt-PT" }, Sample{ "zh-hANS_cn", "zh-Hans-CN" },
			Sample{ "zh-HANT_TW", "zh-Hant-TW" }, Sample{ "sr-CYRL", "sr-Cyrl" },
			Sample{ "sr_RS.UTF-8@latin", "sr-Latn-RS" }, Sample{ "en_US.UTF-8", "en-US" },
			Sample{ "es_419", "es-419" }, Sample{ "ca_ES@VALENCIA", "ca-ES-valencia" },
			Sample{ "be-TARASK", "be-tarask" }, Sample{ "en-US-1996", "en-US-1996" },
			Sample{ "ABC_LATN_001_ABCDEFGH", "abc-Latn-001-abcdefgh" } } ) {
		EXPECT_TRUE( LocaleId( sample.input ).getTag() == sample.normalized );
	}
	const LocaleId taiwan( "zh-Hant-TW" );
	EXPECT_TRUE( LocaleId( "zh-Hant-TW-1996" ).parent() == taiwan );
	EXPECT_TRUE( taiwan.parent().getTag() == "zh-Hant" );
	EXPECT_TRUE( taiwan.parent().parent().getTag() == "zh" );
	EXPECT_FALSE( taiwan.parent().parent().parent().isValid() );

	for ( auto invalid : { "",
						   "C",
						   "POSIX",
						   "en-",
						   "-en",
						   "en--GB",
						   "en_GB_Latn",
						   "en-u-nu-latn",
						   "no-bok-abc",
						   "es-49901-1234",
						   "en-1234-valencia",
						   "en.UTF 8",
						   "en.",
						   "en@euro",
						   "sr-Cyrl@latin",
						   "en_US!",
						   "e4",
						   " en",
						   "en-GB.extra@",
						   "abcd",
						   "strings",
						   "myapp_es",
						   "english" } ) {
		EXPECT_FALSE( LocaleId( invalid ).isValid() );
	}
	EXPECT_TRUE( LocaleId( "zh-TW" ).fallback().getTag() == "zh-Hant-TW" );
	EXPECT_TRUE( LocaleId( "zh-TW-1996" ).fallback().getTag() == "zh-Hant-TW-1996" );
	EXPECT_TRUE( LocaleId( "sr_ME" ).fallback().getTag() == "sr-Latn-ME" );
	EXPECT_TRUE( LocaleId( "sr_RS" ).fallback().getTag() == "sr" );
	EXPECT_TRUE( LocaleId( "zh-CN" ).fallback().getTag() == "zh" );
	EXPECT_TRUE( LocaleId( "zh-Hans-TW" ).fallback().getTag() == "zh-Hans" );
	EXPECT_TRUE( LocaleId( "pa-PK" ).fallback().getTag() == "pa-Arab-PK" );
	EXPECT_TRUE( LocaleId( "ca-valencia" ).fallback().getTag() == "ca" );
	EXPECT_FALSE( LocaleId( "zh" ).fallback().isValid() );
	EXPECT_TRUE( LocaleId::systemUiLanguage().isValid() );
	EXPECT_TRUE( LocaleId::systemNumberLocale().isValid() );
}

UTEST( Locale, UsesRegionalGroupingAndUnicodeSeparators ) {
	EXPECT_TRUE( Locale( "es-AR" ).formatNumber( "12345.67" ) == U"12.345,67" );
	EXPECT_TRUE( Locale( "be-tarask" ).formatNumber( "12345.67" ) == U"12\u00A0345,67" );
	EXPECT_TRUE( Locale( "es-MX" ).formatNumber( "12345.67" ) == U"12,345.67" );
	EXPECT_TRUE( Locale( "en-IN" ).formatNumber( "1234567.89" ) == U"12,34,567.89" );
	EXPECT_TRUE( Locale( "pt-PT" ).formatNumber( "1234.5" ) == U"1234,5" );
	EXPECT_TRUE( Locale( "pt-PT" ).formatNumber( "12345.5" ) == U"12\u00A0345,5" );
	EXPECT_TRUE( Locale( "fr" ).formatNumber( "12345.5" ) == U"12\u202F345,5" );
	EXPECT_TRUE( Locale( "fr-CH" ).formatNumber( "12345.5" ) == U"12'345,5" );
	EXPECT_TRUE( Locale( "fr-Latn-XX" ).formatNumber( "12345.5" ) == U"12\u202F345,5" );
	EXPECT_TRUE( Locale( "xx-Latn-XX" ).formatNumber( "12345.5" ) == U"12,345.5" );
	EXPECT_TRUE( Locale( "zh-HK" ).formatNumber( "12345.5" ) ==
				 Locale( "zh-Hant-HK" ).formatNumber( "12345.5" ) );
	EXPECT_TRUE( Locale( "zh-TW" ).formatPercent( "-0.5" ) ==
				 Locale( "zh-Hant" ).formatPercent( "-0.5" ) );
}

UTEST( Locale, SubstitutesDefaultDigitsAndSupportsLatinOverride ) {
	EXPECT_TRUE( Locale( "ar-EG" ).formatNumber( "1234.5" ) ==
				 U"\u0661\u066C\u0662\u0663\u0664\u066B\u0665" );
	EXPECT_TRUE( Locale( "ar-EG", NumberingSystem::Latin ).formatNumber( "1234.5" ) == U"1,234.5" );
	EXPECT_TRUE( Locale( "fa" ).formatNumber( "12.3" ) == U"\u06F1\u06F2\u066B\u06F3" );
	EXPECT_TRUE( Locale( "as" ).formatNumber( "12.3" ) == U"\u09E7\u09E8.\u09E9" );
	EXPECT_TRUE( Locale( "dz" ).formatNumber( 12 ) == U"\u0F21\u0F22" );
	EXPECT_TRUE( Locale( "sat" ).formatNumber( 12 ) == U"\u1C51\u1C52" );
	EXPECT_TRUE( Locale( "ff-Adlm-SN" ).formatNumber( 12 ) == U"\U0001E951\U0001E952" );
	EXPECT_TRUE( Locale( "ff-Latn-SN" ).formatNumber( 12 ) == U"12" );
}

UTEST( Locale, ShortestDoubleTextRoundTrips ) {
	EXPECT_TRUE( String::toShortestString( 0.1 ) == "0.1" );
	EXPECT_TRUE( String::toShortestString( 0.1 + 0.2 ) == "0.30000000000000004" );
	EXPECT_TRUE( String::toShortestString( -2.5 ) == "-2.5" );
	EXPECT_TRUE( String::toShortestString( 1e21 ) == "1e+21" );
	EXPECT_TRUE( String::toShortestString( 1.25e-7 ) == "1.25e-07" );
	EXPECT_TRUE( String::toShortestString( std::numeric_limits<double>::max() ) ==
				 "1.7976931348623157e+308" );
	EXPECT_TRUE( Locale( "en" ).formatNumber( static_cast<long>( -42 ) ) == U"-42" );
	EXPECT_TRUE( Locale( "en" ).formatNumber( 12345678901234567ull ) == U"12,345,678,901,234,567" );
}

UTEST( Locale, RoundsExactDecimalsAndPreservesLargeIntegers ) {
	const Locale locale( "en-US" );
	NumberFormatOptions options;
	options.maxFractionDigits = options.minFractionDigits = 2;
	EXPECT_TRUE( locale.formatNumber( "1.245", options ) == U"1.24" );
	EXPECT_TRUE( locale.formatNumber( 1.245, options ) == U"1.24" );
	EXPECT_TRUE( locale.formatNumber( "-0001.255", options ) == U"-1.26" );
	EXPECT_TRUE( locale.formatNumber( "1.245000000000000000000000001", options ) == U"1.25" );
	EXPECT_TRUE( locale.formatNumber( "999.995", options ) == U"1,000.00" );
	options.rounding = NumberRounding::HalfAwayFromZero;
	EXPECT_TRUE( locale.formatNumber( "-1.245", options ) == U"-1.25" );
	options.rounding = NumberRounding::TowardZero;
	EXPECT_TRUE( locale.formatNumber( "-999.999", options ) == U"-999.99" );
	EXPECT_TRUE( locale.formatNumber( std::numeric_limits<Uint64>::max() ) ==
				 U"18,446,744,073,709,551,615" );
	EXPECT_TRUE( locale.formatNumber( std::numeric_limits<Int64>::min() ) ==
				 U"-9,223,372,036,854,775,808" );
	EXPECT_TRUE( locale.formatNumber( "1234567890123456789012345" ) ==
				 U"1,234,567,890,123,456,789,012,345" );
	EXPECT_TRUE( locale.formatPercent( 2u ) == U"200%" );
	EXPECT_TRUE( locale.formatPercent( std::numeric_limits<Uint64>::max() ) ==
				 U"1,844,674,407,370,955,161,500%" );
	EXPECT_TRUE( locale.formatNumber( 1.25e-7 ) == U"0" );
	EXPECT_TRUE( locale.formatNumber( 1.25e20 ) == U"125,000,000,000,000,000,000" );
}

UTEST( Locale, HandlesPrecisionOptionsAndSpecialValues ) {
	const Locale locale( "en" );
	NumberFormatOptions options;
	options.grouping = false;
	options.showPlusSign = true;
	options.minFractionDigits = 4;
	EXPECT_TRUE( locale.formatNumber( "+1000.1", options ) == U"+1000.1000" );
	EXPECT_TRUE( locale.formatNumber( -0.0, options ) == U"-0.0000" );
	EXPECT_TRUE( locale.formatNumber( "-0.00001", options ) == U"-0.0000" );
	EXPECT_TRUE( locale.formatNumber( std::numeric_limits<double>::denorm_min() ) == U"0" );
	EXPECT_TRUE( locale.formatNumber( std::numeric_limits<double>::quiet_NaN(), options ) ==
				 U"NaN" );
	EXPECT_TRUE( locale.formatPercent( -std::numeric_limits<double>::infinity() ) == U"-\u221E%" );
	EXPECT_TRUE( locale.formatNumber( std::numeric_limits<double>::infinity(), options ) ==
				 U"+\u221E" );
	options.maxFractionDigits = 2;
	EXPECT_TRUE( locale.formatNumber( 1.0, options ).empty() );
	options.minFractionDigits = 0;
	options.maxFractionDigits = NumberFormatOptions::MaxFractionDigits;
	EXPECT_TRUE( locale.formatNumber( "0.5", options ) == U"+0.5" );
	options.maxFractionDigits = NumberFormatOptions::MaxFractionDigits + 1;
	EXPECT_TRUE( locale.formatNumber( "0.5", options ).empty() );
	EXPECT_TRUE( locale.formatNumber( 0.5, options ).empty() );
	options.maxFractionDigits.reset();
	options.minFractionDigits = NumberFormatOptions::MaxFractionDigits + 1;
	EXPECT_TRUE( locale.formatNumber( 5, options ).empty() );
	// Limits that a narrower field would wrap into the accepted range stay rejected.
	options.minFractionDigits = 0;
	options.maxFractionDigits = 256;
	EXPECT_TRUE( locale.formatNumber( "0.5", options ).empty() );
	options.maxFractionDigits = 65536 + 2;
	EXPECT_TRUE( locale.formatNumber( "0.5", options ).empty() );
	options.maxFractionDigits.reset();
	options.minFractionDigits = -1;
	EXPECT_TRUE( locale.formatNumber( "0.5", options ).empty() );
	for ( auto invalid : { "", "+", ".2", "2.", "1,23", " 1.0", "1e5", "1.2.3", "1x", "NaN" } )
		EXPECT_TRUE( locale.formatNumber( invalid ).empty() );
}

UTEST( Locale, AppliesPercentPatternsGroupingAndBidiAffixes ) {
	NumberFormatOptions options;
	options.maxFractionDigits = 1;
	EXPECT_TRUE( Locale( "en" ).formatPercent( "0.125" ) == U"12%" );
	EXPECT_TRUE( Locale( "en" ).formatPercent( "0.135" ) == U"14%" );
	EXPECT_TRUE( Locale( "fr" ).formatPercent( "0.125", options ) == U"12,5\u00A0%" );
	EXPECT_TRUE( Locale( "tr" ).formatPercent( "-0.125", options ) == U"-%12,5" );
	EXPECT_TRUE( Locale( "blo" ).formatPercent( "-123.456", options ) ==
				 U"%\u00A0-1\u202F23\u202F45,6" );
	options.showPlusSign = true;
	EXPECT_TRUE( Locale( "blo" ).formatPercent( "0.125", options ) == U"%\u00A0+12,5" );
	EXPECT_TRUE( Locale( "ar-EG" ).formatPercent( "-0.125", options ) ==
				 U"\u061C-\u0661\u0662\u066B\u0665\u066A\u061C" );
	EXPECT_TRUE( Locale( "fa" ).formatPercent( "0.125", options ) ==
				 U"\u200E+\u06F1\u06F2\u066B\u06F5\u066A" );
}

#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_MACOS && \
	EE_PLATFORM != EE_PLATFORM_IOS
UTEST( Locale, SystemUiLanguageFollowsGettextEnvironmentPrecedence ) {
	// Empty locale variables count as unset, as they do in POSIX.
	ScopedEnvironmentVariable lang( "LANG", "en_US.UTF-8" );
	ScopedEnvironmentVariable language( "LANGUAGE", "" );
	{
		ScopedEnvironmentVariable messages( "LC_MESSAGES", "de_DE.UTF-8" );
		{
			ScopedEnvironmentVariable all( "LC_ALL", "en_GB.UTF-8" );
			EXPECT_TRUE( LocaleId::systemUiLanguage().getTag() == "en-GB" );
		}
		ScopedEnvironmentVariable all( "LC_ALL", "" );
		EXPECT_TRUE( LocaleId::systemUiLanguage().getTag() == "de-DE" );
	}
	ScopedEnvironmentVariable all( "LC_ALL", "" );
	ScopedEnvironmentVariable messages( "LC_MESSAGES", "" );
	EXPECT_TRUE( LocaleId::systemUiLanguage().getTag() == "en-US" );
	{
		ScopedEnvironmentVariable list( "LANGUAGE", "bad!::pt_BR:es" );
		EXPECT_TRUE( LocaleId::systemUiLanguage().getTag() == "pt-BR" );
		ScopedEnvironmentVariable posix( "LC_ALL", "C" );
		EXPECT_TRUE( LocaleId::systemUiLanguage().getTag() == "en" );
	}
}
#endif
