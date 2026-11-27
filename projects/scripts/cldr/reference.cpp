// Optional conformance driver. Its ICU dependency is only for maintainer checks.
#include <array>
#include <eepp/system/locale.hpp>
#include <eepp/system/pluralrules.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <unicode/dcfmtsym.h>
#include <unicode/decimfmt.h>
#include <vector>

using namespace EE::System;

namespace {

std::string nativeNumber( const std::vector<std::string>& fields ) {
	using Symbols = icu::DecimalFormatSymbols;
	UErrorCode status = U_ZERO_ERROR;
	Symbols symbols( icu::Locale::getRoot(), status );
	constexpr std::array symbolIds{ Symbols::kDecimalSeparatorSymbol,
									Symbols::kGroupingSeparatorSymbol, Symbols::kPlusSignSymbol,
									Symbols::kMinusSignSymbol, Symbols::kPercentSymbol };
	for ( std::size_t i = 0; i < symbolIds.size(); ++i )
		symbols.setSymbol( symbolIds[i], icu::UnicodeString::fromUTF8( fields[7 + i] ) );

	const auto digits = icu::UnicodeString::fromUTF8( fields[12] );
	int32_t offset = 0;
	for ( int i = 0; i < 10; ++i ) {
		const UChar32 digit = digits.char32At( offset );
		const auto symbol =
			i == 0 ? Symbols::kZeroDigitSymbol
				   : static_cast<Symbols::ENumberFormatSymbol>( Symbols::kOneDigitSymbol + i - 1 );
		symbols.setSymbol( symbol, icu::UnicodeString( digit ), false );
		offset += U16_LENGTH( digit );
	}
	icu::DecimalFormat format( icu::UnicodeString::fromUTF8( fields[6] ), symbols, status );
	format.setMinimumGroupingDigits( std::stoi( fields[13] ) );
	format.setRoundingMode( icu::DecimalFormat::kRoundHalfEven );
	if ( fields[5] != "-" )
		format.setMaximumFractionDigits( std::stoi( fields[5] ) );

	icu::UnicodeString output;
	format.format( icu::StringPiece( fields[3] ), output, nullptr, status );
	if ( U_FAILURE( status ) )
		return std::string( "ICU error: " ) + u_errorName( status );
	std::string utf8;
	return output.toUTF8String( utf8 );
}

} // namespace

int main() {
	for ( std::string line; std::getline( std::cin, line ); ) {
		std::istringstream input( line );
		std::vector<std::string> fields;
		for ( std::string field; std::getline( input, field, '\t' ); )
			fields.push_back( std::move( field ) );
		if ( fields.size() == 3 && fields[0] == "plural" ) {
			std::cout << static_cast<int>( PluralRules::select( LocaleId( fields[1] ), fields[2] ) )
					  << '\n';
		} else if ( fields.size() == 14 && fields[0] == "number" ) {
			const bool percent = fields[4] == "percent";
			const Locale locale( fields[1], fields[2] == "latin" ? NumberingSystem::Latin
																 : NumberingSystem::LocaleDefault );
			NumberFormatOptions options;
			if ( fields[5] != "-" )
				options.maxFractionDigits = std::stoi( fields[5] );
			const auto formatted = percent ? locale.formatPercent( fields[3], options )
										   : locale.formatNumber( fields[3], options );
			std::cout << formatted.toUtf8() << '\t' << nativeNumber( fields ) << '\n';
		} else {
			return 1;
		}
	}
}
