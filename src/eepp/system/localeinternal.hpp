#ifndef EE_SYSTEM_LOCALE_INTERNAL_HPP
#define EE_SYSTEM_LOCALE_INTERNAL_HPP

#include "localedata.hpp"
#include <algorithm>
#include <eepp/system/locale.hpp>
#include <optional>

namespace EE::System {

struct DecimalInput {
	std::string_view whole, fraction;
	bool negative;

	static std::optional<DecimalInput> parse( std::string_view value ) {
		const bool negative = !value.empty() && value.front() == '-';
		if ( negative || ( !value.empty() && value.front() == '+' ) )
			value.remove_prefix( 1 );
		const auto dot = value.find( '.' );
		const auto whole = value.substr( 0, dot );
		const auto fraction = dot == value.npos ? std::string_view{} : value.substr( dot + 1 );
		if ( whole.empty() || whole.find_first_not_of( "0123456789" ) != value.npos ||
			 ( dot != value.npos &&
			   ( fraction.empty() || fraction.find_first_not_of( "0123456789" ) != value.npos ) ) )
			return std::nullopt;
		return DecimalInput{ whole, fraction, negative };
	}
};

template <typename Entry>
const Entry* findLocaleEntry( std::string_view tag, const Entry* entries, std::size_t count ) {
	const auto end = entries + count;
	const auto found =
		std::lower_bound( entries, end, tag, []( const Entry& entry, std::string_view key ) {
			return LocaleData::text( entry.name ) < key;
		} );
	return found != end && LocaleData::text( found->name ) == tag ? found : nullptr;
}

/** Searches the ID and then each LocaleId::fallback() candidate. */
template <typename Entry>
const Entry* findLocaleData( LocaleId id, const Entry* entries, std::size_t count ) {
	for ( ; id.isValid(); id = id.fallback() ) {
		if ( const auto* found = findLocaleEntry( id.getTag(), entries, count ) )
			return found;
	}
	return nullptr;
}

} // namespace EE::System

#endif
