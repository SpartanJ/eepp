#ifndef EE_SYSTEM_PLURALRULES_HPP
#define EE_SYSTEM_PLURALRULES_HPP

#include <eepp/system/locale.hpp>

namespace EE::System {

enum class PluralCategory : Uint8 { Zero, One, Two, Few, Many, Other };

class EE_API PluralRules {
  public:
	/** Selects a CLDR cardinal category from an exact ASCII decimal, retaining visible zeros.
	 * The sign is ignored. Unknown locales and invalid inputs return Other. Compact exponents
	 * are unsupported, so the CLDR c and e operands are always zero. */
	static PluralCategory select( LocaleId locale, std::string_view value );

	template <NumericInteger Integer>
	static PluralCategory select( LocaleId locale, Integer value ) {
		return select( locale, String::toString( WideInteger<Integer>( value ) ) );
	}
};

} // namespace EE::System

#endif
