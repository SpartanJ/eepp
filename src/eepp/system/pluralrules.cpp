#include <eepp/system/pluralrules.hpp>

#include "localeinternal.hpp"
#include <limits>

namespace EE::System {

namespace {

using namespace LocaleData;

std::uint64_t digitValue( std::string_view digits, std::uint32_t modulo ) {
	std::uint64_t result = 0;
	if ( modulo ) {
		for ( char digit : digits )
			result = ( result * 10 + ( digit - '0' ) ) % modulo;
	} else {
		for ( char digit : digits ) {
			result = result * 10 + ( digit - '0' );
			if ( result > std::numeric_limits<std::uint32_t>::max() )
				break; // Every generated range ends at or below UINT32_MAX.
		}
	}
	return result;
}

struct PluralOperands {
	std::string_view whole;
	std::string_view fraction;
	std::string_view trimmedFraction;

	std::uint64_t integerValue( Operand operand, std::uint32_t modulo ) const {
		switch ( operand ) {
			case Operand::N:
			case Operand::I:
				return digitValue( whole, modulo );
			case Operand::V:
				return modulo ? fraction.size() % modulo : fraction.size();
			case Operand::W:
				return modulo ? trimmedFraction.size() % modulo : trimmedFraction.size();
			case Operand::F:
				return digitValue( fraction, modulo );
			case Operand::T:
				return digitValue( trimmedFraction, modulo );
			case Operand::C:
			case Operand::E:
				return 0;
		}
		return 0;
	}

	bool matches( const Relation& relation ) const {
		// CLDR integer-range membership fails when n has a nonzero fraction, even with modulo.
		if ( relation.operand == Operand::N && !trimmedFraction.empty() )
			return relation.negate;
		const auto value = integerValue( relation.operand, relation.modulo );
		const auto first = ranges + relation.firstRange;
		const auto end = first + relation.rangeCount;
		const bool contains = std::any_of( first, end, [value]( const Range& range ) {
			return value >= range.low && value <= range.high;
		} );
		return contains != relation.negate;
	}
};

} // namespace

PluralCategory PluralRules::select( LocaleId locale, std::string_view value ) {
	using namespace LocaleData;
	const auto* entry = findLocaleData( locale, pluralLocales, pluralLocaleCount );
	if ( !entry )
		return PluralCategory::Other;
	const auto input = DecimalInput::parse( value );
	if ( !input )
		return PluralCategory::Other;

	const auto lastDigit = input->fraction.find_last_not_of( '0' );
	const PluralOperands operands{ input->whole, input->fraction,
								   lastDigit == std::string_view::npos
									   ? std::string_view{}
									   : input->fraction.substr( 0, lastDigit + 1 ) };
	const auto& set = ruleSets[entry->ruleSet];
	for ( auto rule = rules + set.firstRule; rule != rules + set.firstRule + set.ruleCount;
		  ++rule ) {
		const auto end = clauses + rule->firstClause + rule->clauseCount;
		for ( auto clause = clauses + rule->firstClause; clause != end; ++clause ) {
			const auto first = relations + clause->firstRelation;
			if ( std::all_of( first, first + clause->relationCount,
							  [&operands]( const Relation& relation ) {
								  return operands.matches( relation );
							  } ) )
				return static_cast<PluralCategory>( rule->category );
		}
	}
	return PluralCategory::Other;
}

} // namespace EE::System
