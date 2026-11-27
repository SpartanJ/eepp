#include <eepp/system/locale.hpp>

#include "localeinternal.hpp"
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <eepp/core/utf.hpp>
#include <limits>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_IOS
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace EE::System {

namespace {

bool isAsciiLetter( char c ) {
	return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
}

bool isAsciiDigit( char c ) {
	return c >= '0' && c <= '9';
}

char lowerAscii( char c ) {
	return c >= 'A' && c <= 'Z' ? c + ( 'a' - 'A' ) : c;
}

char upperAscii( char c ) {
	return c >= 'a' && c <= 'z' ? c - ( 'a' - 'A' ) : c;
}

bool isScript( std::string_view part ) {
	return part.size() == 4 && std::all_of( part.begin(), part.end(), isAsciiLetter );
}

bool isRegion( std::string_view part ) {
	return ( part.size() == 2 && std::all_of( part.begin(), part.end(), isAsciiLetter ) ) ||
		   ( part.size() == 3 && std::all_of( part.begin(), part.end(), isAsciiDigit ) );
}

bool isVariant( std::string_view part ) {
	return ( ( part.size() >= 5 && part.size() <= 8 ) ||
			 ( part.size() == 4 && isAsciiDigit( part.front() ) ) ) &&
		   std::all_of( part.begin(), part.end(),
						[]( char c ) { return isAsciiLetter( c ) || isAsciiDigit( c ); } );
}

bool asciiEqual( std::string_view lhs, std::string_view rhs ) {
	return std::equal( lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), []( char left, char right ) {
		return lowerAscii( left ) == lowerAscii( right );
	} );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
LocaleId systemLocaleId( std::string_view name ) {
	LocaleId id( name );
	if ( !id.isValid() && name.find( '@' ) != name.npos )
		id = LocaleId( name.substr( 0, name.find( '@' ) ) );
	return id;
}
#endif

#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_IOS
LocaleId appleLocaleId( CFStringRef name ) {
	char utf8[128];
	return name && CFStringGetCString( name, utf8, sizeof( utf8 ), kCFStringEncodingUTF8 )
			   ? systemLocaleId( utf8 )
			   : LocaleId();
}
#elif EE_PLATFORM != EE_PLATFORM_WIN
LocaleId environmentLocaleId( const char* category ) {
	for ( auto variable : { "LC_ALL", category, "LANG" } ) {
		if ( const char* value = std::getenv( variable ); value && *value )
			return systemLocaleId( value );
	}
	return {};
}

// GNU gettext gives a nonempty LANGUAGE priority list precedence over the locale categories,
// but ignores it when the messages locale is C or POSIX. Neither of those is a valid LocaleId.
LocaleId environmentUiLanguage() {
	const auto messages = environmentLocaleId( "LC_MESSAGES" );
	if ( !messages.isValid() )
		return messages;
	if ( const char* value = std::getenv( "LANGUAGE" ); value ) {
		for ( std::string_view list( value ); !list.empty(); ) {
			const auto colon = list.find( ':' );
			if ( const auto id = systemLocaleId( list.substr( 0, colon ) ); id.isValid() )
				return id;
			list.remove_prefix( colon == list.npos ? list.size() : colon + 1 );
		}
	}
	return messages;
}
#endif

void appendUtf8( String& output, std::string_view utf8 ) {
	auto next = utf8.begin();
	while ( next != utf8.end() ) {
		Uint32 codepoint;
		next = Utf8::decode( next, utf8.end(), codepoint );
		output.push_back( static_cast<char32_t>( codepoint ) );
	}
}

const LocaleData::Affixes& affixesFor( const LocaleData::Pattern& pattern, bool negative,
									   bool showPlus ) {
	if ( negative )
		return pattern.negative;
	return showPlus ? pattern.explicitPlus : pattern.positive;
}

bool roundUp( std::string_view digits, std::size_t keep, NumberRounding rounding ) {
	if ( rounding == NumberRounding::TowardZero || digits[keep] < '5' )
		return false;
	if ( digits[keep] > '5' || rounding == NumberRounding::HalfAwayFromZero )
		return true;
	return ( digits[keep - 1] - '0' ) % 2 != 0 ||
		   digits.find_first_not_of( '0', keep + 1 ) != digits.npos;
}

bool validFractionLimits( const NumberFormatOptions& options ) {
	return options.minFractionDigits >= 0 &&
		   options.minFractionDigits <= NumberFormatOptions::MaxFractionDigits &&
		   ( !options.maxFractionDigits ||
			 ( *options.maxFractionDigits <= NumberFormatOptions::MaxFractionDigits &&
			   *options.maxFractionDigits >= options.minFractionDigits ) );
}

std::string_view likelyScript( std::string_view languageRegion ) {
	const auto* alias =
		findLocaleEntry( languageRegion, LocaleData::scriptAliases, LocaleData::scriptAliasCount );
	return alias ? LocaleData::text( alias->script ) : std::string_view{};
}

} // namespace

LocaleId::LocaleId( std::string_view tag ) {
	std::string_view scriptModifier, variantModifier;
	if ( const auto at = tag.find( '@' ); at != tag.npos ) {
		const auto modifier = tag.substr( at + 1 );
		tag = tag.substr( 0, at );
		if ( asciiEqual( modifier, "latin" ) )
			scriptModifier = "Latn";
		else if ( asciiEqual( modifier, "cyrillic" ) )
			scriptModifier = "Cyrl";
		else if ( isVariant( modifier ) )
			variantModifier = modifier;
		else
			return;
	}
	if ( const auto dot = tag.find( '.' ); dot != tag.npos ) {
		const auto encoding = tag.substr( dot + 1 );
		if ( encoding.empty() || !std::all_of( encoding.begin(), encoding.end(), []( char c ) {
				 return isAsciiLetter( c ) || isAsciiDigit( c ) || c == '-' || c == '_';
			 } ) )
			return;
		tag = tag.substr( 0, dot );
	}

	const auto firstSeparator = tag.find_first_of( "-_" );
	const auto language = tag.substr( 0, firstSeparator );
	if ( language.size() < 2 || language.size() > 3 ||
		 !std::all_of( language.begin(), language.end(), isAsciiLetter ) )
		return;

	std::array<std::string_view, 3> subtags;
	std::size_t count = 0;
	for ( auto separator = firstSeparator; separator != tag.npos; ) {
		if ( count == subtags.size() )
			return;
		const auto start = separator + 1;
		separator = tag.find_first_of( "-_", start );
		subtags[count++] = tag.substr( start, separator - start );
	}

	std::string_view script, region, variant;
	std::size_t next = 0;
	if ( next < count && isScript( subtags[next] ) )
		script = subtags[next++];
	if ( next < count && isRegion( subtags[next] ) )
		region = subtags[next++];
	if ( next < count && isVariant( subtags[next] ) )
		variant = subtags[next++];
	if ( next != count ||
		 ( !script.empty() && !scriptModifier.empty() && !asciiEqual( script, scriptModifier ) ) ||
		 ( !variant.empty() && !variantModifier.empty() &&
		   !asciiEqual( variant, variantModifier ) ) )
		return;
	if ( script.empty() )
		script = scriptModifier;
	if ( variant.empty() )
		variant = variantModifier;

	for ( char c : language )
		mTag[mSize++] = lowerAscii( c );
	if ( !script.empty() ) {
		mTag[mSize++] = '-';
		mTag[mSize++] = upperAscii( script.front() );
		for ( char c : script.substr( 1 ) )
			mTag[mSize++] = lowerAscii( c );
	}
	if ( !region.empty() ) {
		mTag[mSize++] = '-';
		for ( char c : region )
			mTag[mSize++] = upperAscii( c );
	}
	if ( !variant.empty() ) {
		mTag[mSize++] = '-';
		for ( char c : variant )
			mTag[mSize++] = lowerAscii( c );
	}
}

LocaleId LocaleId::parent() const {
	const auto separator = getTag().rfind( '-' );
	if ( separator == std::string_view::npos )
		return {};
	auto result = *this;
	result.mSize = static_cast<Uint8>( separator );
	return result;
}

LocaleId LocaleId::fallback() const {
	const auto tag = getTag();
	const auto languageEnd = tag.find( '-' );
	if ( languageEnd != tag.npos ) {
		const auto regionEnd = tag.find( '-', languageEnd + 1 );
		const auto second = tag.substr( languageEnd + 1, regionEnd - languageEnd - 1 );
		if ( isRegion( second ) ) {
			if ( const auto script = likelyScript( tag.substr( 0, regionEnd ) ); !script.empty() ) {
				// The inserted subtag fits: a tag without a script is at most 16 characters.
				LocaleId result;
				const auto copy = [&result]( std::string_view part ) {
					std::copy( part.begin(), part.end(), result.mTag.begin() + result.mSize );
					result.mSize += static_cast<Uint8>( part.size() );
				};
				copy( tag.substr( 0, languageEnd + 1 ) );
				copy( script );
				copy( tag.substr( languageEnd ) );
				return result;
			}
		}
	}
	return parent();
}

LocaleId LocaleId::systemUiLanguage() {
	LocaleId id;
#if EE_PLATFORM == EE_PLATFORM_WIN
	wchar_t languages[512];
	ULONG languageCount = 0, size = sizeof( languages ) / sizeof( *languages );
	if ( GetUserPreferredUILanguages( MUI_LANGUAGE_NAME, &languageCount, languages, &size ) &&
		 languageCount > 0 ) {
		id = LocaleId( String::fromWide( languages ).toUtf8() );
	} else {
		wchar_t name[LOCALE_NAME_MAX_LENGTH];
		if ( LCIDToLocaleName( MAKELCID( GetUserDefaultUILanguage(), SORT_DEFAULT ), name,
							   LOCALE_NAME_MAX_LENGTH, 0 ) )
			id = LocaleId( String::fromWide( name ).toUtf8() );
	}
#elif EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_IOS
	if ( const auto languages = CFLocaleCopyPreferredLanguages() ) {
		if ( CFArrayGetCount( languages ) > 0 ) {
			id =
				appleLocaleId( static_cast<CFStringRef>( CFArrayGetValueAtIndex( languages, 0 ) ) );
		}
		CFRelease( languages );
	}
#else
	id = environmentUiLanguage();
#endif
	return id.isValid() ? id : LocaleId( "en" );
}

LocaleId LocaleId::systemNumberLocale() {
	LocaleId id;
#if EE_PLATFORM == EE_PLATFORM_WIN
	wchar_t name[LOCALE_NAME_MAX_LENGTH];
	if ( GetUserDefaultLocaleName( name, LOCALE_NAME_MAX_LENGTH ) )
		id = LocaleId( String::fromWide( name ).toUtf8() );
#elif EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_IOS
	if ( const auto locale = CFLocaleCopyCurrent() ) {
		id = appleLocaleId( CFLocaleGetIdentifier( locale ) );
		CFRelease( locale );
	}
#else
	id = environmentLocaleId( "LC_NUMERIC" );
#endif
	return id.isValid() ? id : LocaleId( "en-US" );
}

Locale::Locale( LocaleId id, NumberingSystem digits ) : mId( id ) {
	using namespace LocaleData;
	const auto* entry = findLocaleData( id, numberLocales, numberLocaleCount );
	mProfile = entry ? ( digits == NumberingSystem::Latin ? entry->latinProfile : entry->profile )
					 : defaultNumberProfile;
}

String Locale::formatNumber( double value, NumberFormatOptions options ) const {
	return formatFloating( value, options, false );
}

String Locale::formatNumber( std::string_view value, NumberFormatOptions options ) const {
	return formatDecimal( value, options, false );
}

String Locale::formatPercent( double value, NumberFormatOptions options ) const {
	return formatFloating( value, options, true );
}

String Locale::formatPercent( std::string_view value, NumberFormatOptions options ) const {
	return formatDecimal( value, options, true );
}

String Locale::formatFloating( double value, NumberFormatOptions options, bool percent ) const {
	using namespace LocaleData;
	if ( !validFractionLimits( options ) )
		return {};
	if ( !std::isfinite( value ) ) {
		String output;
		const auto& profile = numberProfiles[mProfile];
		if ( std::isnan( value ) ) {
			appendUtf8( output, text( profile.nan ) );
		} else {
			const auto& pattern =
				patterns[percent ? profile.percentPattern : profile.decimalPattern];
			const auto& affixes =
				affixesFor( pattern, std::signbit( value ), options.showPlusSign );
			appendUtf8( output, text( affixes.prefix ) );
			appendUtf8( output, text( profile.infinity ) );
			appendUtf8( output, text( affixes.suffix ) );
		}
		return output;
	}

	const auto text = String::toShortestString( value );
	std::string_view decimal = text;
	int exponent = 0;
	if ( const auto e = decimal.find( 'e' ); e != decimal.npos ) {
		auto power = decimal.substr( e + 1 );
		if ( power.front() == '+' )
			power.remove_prefix( 1 );
		std::from_chars( power.data(), power.data() + power.size(), exponent );
		decimal = decimal.substr( 0, e );
	}
	return formatDecimal( decimal, options, percent, exponent );
}

String Locale::formatDecimal( std::string_view value, NumberFormatOptions options, bool percent,
							  int exponent ) const {
	using namespace LocaleData;
	const auto input = DecimalInput::parse( value );
	if ( !input || !validFractionLimits( options ) )
		return {};

	const auto& profile = numberProfiles[mProfile];
	const auto& pattern = patterns[percent ? profile.percentPattern : profile.decimalPattern];
	const auto minFraction =
		std::max<std::size_t>( options.minFractionDigits, pattern.minFraction );
	const auto maxFraction = options.maxFractionDigits
								 ? static_cast<std::size_t>( *options.maxFractionDigits )
								 : std::max<std::size_t>( minFraction, pattern.maxFraction );
	if ( maxFraction < minFraction )
		return {};

	std::string digits( input->whole );
	digits += input->fraction;

	std::size_t whole = input->whole.size();
	const int shift = exponent + ( percent ? 2 : 0 );
	if ( shift < 0 && whole <= static_cast<unsigned>( -shift ) ) {
		digits.insert( 0, 1 + static_cast<unsigned>( -shift ) - whole, '0' );
		whole = 1;
	} else if ( shift < 0 ) {
		whole -= static_cast<unsigned>( -shift );
	} else {
		whole += static_cast<unsigned>( shift );
		if ( whole > digits.size() )
			digits.resize( whole, '0' );
	}

	const auto nonZero = digits.find_first_not_of( '0' );
	const auto skip = std::min( whole - 1, nonZero );
	if ( skip ) {
		digits.erase( 0, skip );
		whole -= skip;
	}
	if ( digits.size() - whole > maxFraction ) {
		const auto keep = whole + maxFraction;
		const bool increment = roundUp( digits, keep, options.rounding );
		digits.resize( keep );
		if ( increment ) {
			auto carry = digits.size();
			while ( carry > 0 && digits[carry - 1] == '9' )
				digits[--carry] = '0';
			if ( carry > 0 ) {
				++digits[carry - 1];
			} else {
				digits.insert( digits.begin(), '1' );
				++whole;
			}
		}
	}
	while ( digits.size() - whole > minFraction && digits.back() == '0' )
		digits.pop_back();
	if ( digits.size() - whole < minFraction )
		digits.resize( whole + minFraction, '0' );

	const auto& glyphs = digitSets[profile.digits].digits;
	const auto& affixes = affixesFor( pattern, input->negative, options.showPlusSign );
	const bool group =
		options.grouping && pattern.primaryGrouping &&
		whole >= static_cast<unsigned>( pattern.primaryGrouping + profile.minGrouping );
	const auto groupedDigits = group ? whole - pattern.primaryGrouping : 0;
	auto nextGroup = group ? ( groupedDigits - 1 ) % pattern.secondaryGrouping + 1 : whole;
	const auto groupCount = group ? 1 + ( groupedDigits - 1 ) / pattern.secondaryGrouping : 0;
	const auto prefix = text( affixes.prefix );
	const auto suffix = text( affixes.suffix );
	const auto groupSymbol = group ? text( profile.group ) : std::string_view{};
	const auto decimalSymbol =
		whole != digits.size() ? text( profile.decimal ) : std::string_view{};
	String output;
	output.reserve( digits.size() + prefix.size() + suffix.size() +
					groupCount * groupSymbol.size() + decimalSymbol.size() );
	appendUtf8( output, prefix );
	for ( std::size_t i = 0; i < whole; ++i ) {
		if ( i == nextGroup ) {
			appendUtf8( output, groupSymbol );
			nextGroup = i == groupedDigits ? whole : i + pattern.secondaryGrouping;
		}
		output.push_back( glyphs[digits[i] - '0'] );
	}
	if ( whole != digits.size() ) {
		appendUtf8( output, decimalSymbol );
		for ( std::size_t i = whole; i < digits.size(); ++i )
			output.push_back( glyphs[digits[i] - '0'] );
	}
	appendUtf8( output, suffix );
	return output;
}

} // namespace EE::System
