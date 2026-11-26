#ifndef EE_WINDOW_TERMINAL_PRIVATE_KITTYUISCALE_HPP
#define EE_WINDOW_TERMINAL_PRIVATE_KITTYUISCALE_HPP

#include <charconv>
#include <cmath>
#include <eepp/config.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace EE::Window::Private {

struct KittyUIScale {
	// A valid 2x1 RGB query with the reserved ID and the version marker "EEPPS1".
	static constexpr std::string_view Query =
		"\033_Ga=q,i=1162170449,t=d,f=24,s=2,v=1;RUVQUFMx\033\\";

	static bool isQuery( std::string_view command ) {
		return command == Query.substr( 3, Query.size() - 5 );
	}

	static std::string reply( Float scale ) {
		if ( !std::isfinite( scale ) || scale < 0.25f || scale > 16.f )
			return {};
		return "\033_Gi=1162170449,eepp-scale=" +
			   std::to_string( std::lround( static_cast<double>( scale ) * 1000000.0 ) ) +
			   ";OK\033\\";
	}

	// nullopt is an unrelated reply. Zero acknowledges the query without a usable scale.
	static std::optional<Float> parseReply( std::string_view command ) {
		if ( command.empty() || command.front() != 'G' )
			return std::nullopt;
		const auto separator = command.find( ';' );
		if ( separator == std::string_view::npos )
			return std::nullopt;
		std::string_view fields = command.substr( 1, separator - 1 );
		bool matched = false;
		bool malformed = false;
		std::optional<unsigned> scale;
		while ( !fields.empty() ) {
			const auto comma = fields.find( ',' );
			const auto field = fields.substr( 0, comma );
			fields =
				comma == std::string_view::npos ? std::string_view{} : fields.substr( comma + 1 );
			const auto equals = field.find( '=' );
			if ( equals == std::string_view::npos || equals == 0 || equals + 1 == field.size() ||
				 ( comma != std::string_view::npos && fields.empty() ) )
				malformed = true;
			if ( field.starts_with( "i=" ) ) {
				if ( matched || field != "i=1162170449" )
					return std::nullopt;
				matched = true;
			} else if ( field.starts_with( "eepp-scale=" ) ) {
				if ( scale ) {
					malformed = true;
					continue;
				}
				const auto value = field.substr( 11 );
				unsigned parsed = 0;
				const auto end = value.data() + value.size();
				const auto result = std::from_chars( value.data(), end, parsed );
				scale = result.ec == std::errc{} && result.ptr == end ? parsed : 0;
			}
		}
		if ( !matched )
			return std::nullopt;
		if ( malformed || command.substr( separator + 1 ) != "OK" || !scale || *scale < 250000 ||
			 *scale > 16000000 )
			return 0.f;
		return static_cast<Float>( *scale ) / 1000000.f;
	}
};

} // namespace EE::Window::Private

#endif
