#ifndef EE_SYSTEM_LOCALE_DATA_HPP
#define EE_SYSTEM_LOCALE_DATA_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace EE::System::LocaleData {

struct Range {
	std::uint32_t low, high;
};

enum class Operand : std::uint8_t { N, I, V, W, F, T, C, E };

struct Relation {
	std::uint32_t modulo;
	std::uint16_t firstRange;
	std::uint8_t rangeCount;
	Operand operand;
	bool negate;
};

struct Clause {
	std::uint16_t firstRelation;
	std::uint8_t relationCount;
};

struct Rule {
	std::uint16_t firstClause;
	std::uint8_t clauseCount, category;
};

struct RuleSet {
	std::uint16_t firstRule;
	std::uint8_t ruleCount;
};

struct PluralLocale {
	std::uint16_t name;
	std::uint8_t ruleSet;
};

struct DigitSet {
	char32_t digits[10];
};

struct Affixes {
	std::uint16_t prefix, suffix;
};

struct Pattern {
	Affixes positive, negative, explicitPlus;
	std::uint8_t primaryGrouping, secondaryGrouping, minFraction, maxFraction;
};

struct NumberProfile {
	std::uint16_t decimal, group, nan, infinity;
	std::uint8_t digits, decimalPattern, percentPattern, minGrouping;
};

struct NumberLocale {
	std::uint16_t name;
	std::uint8_t profile, latinProfile;
};

/** A language-region ID whose likely script differs from its bare language's script. */
struct ScriptAlias {
	std::uint16_t name, script;
};

extern const char strings[];
extern const Range ranges[];
extern const Relation relations[];
extern const Clause clauses[];
extern const Rule rules[];
extern const RuleSet ruleSets[];
extern const PluralLocale pluralLocales[];
extern const DigitSet digitSets[];
extern const Pattern patterns[];
extern const NumberProfile numberProfiles[];
extern const NumberLocale numberLocales[];
extern const ScriptAlias scriptAliases[];
extern const std::size_t pluralLocaleCount, numberLocaleCount, scriptAliasCount;
extern const std::uint8_t defaultNumberProfile;

inline std::string_view text( std::uint16_t offset ) {
	return strings + offset;
}

} // namespace EE::System::LocaleData

#endif
