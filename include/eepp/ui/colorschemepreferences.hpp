#pragma once

#include <eepp/config.hpp>
#include <string>
#include <string_view>
#include <type_traits>

namespace EE::UI {

enum class ColorSchemePreference { Light, Dark };

enum class ColorSchemeExtPreference : std::underlying_type_t<ColorSchemePreference> {
	Light =
		static_cast<std::underlying_type_t<ColorSchemePreference>>( ColorSchemePreference::Light ),
	Dark =
		static_cast<std::underlying_type_t<ColorSchemePreference>>( ColorSchemePreference::Dark ),
	System
};

struct EE_API ColorSchemePreferences {
	static ColorSchemePreference fromString( std::string_view str );

	static ColorSchemeExtPreference fromStringExt( std::string_view str );

	static ColorSchemePreference fromExt( ColorSchemeExtPreference pref );

	static ColorSchemeExtPreference toExt( ColorSchemePreference pref );

	static std::string toString( ColorSchemePreference pref );

	static std::string toString( ColorSchemeExtPreference pref );
};

} // namespace EE::UI
