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

enum class ContrastPreference { NoPreference, More, Less, Custom };

enum class ContrastExtPreference : std::underlying_type_t<ContrastPreference> {
	NoPreference =
		static_cast<std::underlying_type_t<ContrastPreference>>( ContrastPreference::NoPreference ),
	More = static_cast<std::underlying_type_t<ContrastPreference>>( ContrastPreference::More ),
	Less = static_cast<std::underlying_type_t<ContrastPreference>>( ContrastPreference::Less ),
	Custom = static_cast<std::underlying_type_t<ContrastPreference>>( ContrastPreference::Custom ),
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

struct EE_API ContrastPreferences {
	static ContrastPreference fromString( std::string_view str );

	static ContrastExtPreference fromStringExt( std::string_view str );

	static ContrastPreference fromExt( ContrastExtPreference pref );

	static ContrastExtPreference toExt( ContrastPreference pref );

	static std::string toString( ContrastPreference pref );

	static std::string toString( ContrastExtPreference pref );
};

} // namespace EE::UI
