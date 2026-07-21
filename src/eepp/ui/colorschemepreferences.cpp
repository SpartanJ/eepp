#include <eepp/system/sys.hpp>
#include <eepp/ui/colorschemepreferences.hpp>

using namespace EE::System;

using namespace std::literals;

namespace EE::UI {

ColorSchemePreference ColorSchemePreferences::fromString( std::string_view str ) {
	if ( str == "light"sv )
		return ColorSchemePreference::Light;
	return ColorSchemePreference::Dark;
}

ColorSchemeExtPreference ColorSchemePreferences::fromStringExt( std::string_view str ) {
	if ( str == "system"sv )
		return ColorSchemeExtPreference::System;
	if ( str == "light"sv )
		return ColorSchemeExtPreference::Light;
	return ColorSchemeExtPreference::Dark;
}

ColorSchemePreference ColorSchemePreferences::fromExt( ColorSchemeExtPreference pref ) {
	switch ( pref ) {
		case ColorSchemeExtPreference::Light:
			return ColorSchemePreference::Light;
		case ColorSchemeExtPreference::Dark:
			return ColorSchemePreference::Dark;
		case ColorSchemeExtPreference::System:
			break;
	}
	return Sys::isOSUsingDarkColorScheme() ? ColorSchemePreference::Dark
										   : ColorSchemePreference::Light;
}

ColorSchemeExtPreference ColorSchemePreferences::toExt( ColorSchemePreference pref ) {
	if ( pref == ColorSchemePreference::Light )
		return ColorSchemeExtPreference::Light;
	if ( pref == ColorSchemePreference::Dark )
		return ColorSchemeExtPreference::Dark;
	return ColorSchemeExtPreference::System;
}

std::string ColorSchemePreferences::toString( ColorSchemePreference pref ) {
	if ( pref == ColorSchemePreference::Light )
		return "light";
	return "dark";
}

std::string ColorSchemePreferences::toString( ColorSchemeExtPreference pref ) {
	if ( pref == ColorSchemeExtPreference::Light )
		return "light";
	if ( pref == ColorSchemeExtPreference::Dark )
		return "dark";
	return "system";
}

ContrastPreference ContrastPreferences::fromString( std::string_view str ) {
	if ( str == "more"sv )
		return ContrastPreference::More;
	if ( str == "less"sv )
		return ContrastPreference::Less;
	if ( str == "custom"sv )
		return ContrastPreference::Custom;
	return ContrastPreference::NoPreference;
}

ContrastExtPreference ContrastPreferences::fromStringExt( std::string_view str ) {
	if ( str == "system"sv )
		return ContrastExtPreference::System;
	if ( str == "more"sv )
		return ContrastExtPreference::More;
	if ( str == "less"sv )
		return ContrastExtPreference::Less;
	if ( str == "custom"sv )
		return ContrastExtPreference::Custom;
	return ContrastExtPreference::NoPreference;
}

ContrastPreference ContrastPreferences::fromExt( ContrastExtPreference pref ) {
	if ( pref == ContrastExtPreference::System )
		return ContrastPreference::NoPreference;
	return static_cast<ContrastPreference>( pref );
}

ContrastExtPreference ContrastPreferences::toExt( ContrastPreference pref ) {
	return static_cast<ContrastExtPreference>( pref );
}

std::string ContrastPreferences::toString( ContrastPreference pref ) {
	switch ( pref ) {
		case ContrastPreference::More:
			return "more";
		case ContrastPreference::Less:
			return "less";
		case ContrastPreference::Custom:
			return "custom";
		default:
			break;
	}
	return "no-preference";
}

std::string ContrastPreferences::toString( ContrastExtPreference pref ) {
	if ( pref == ContrastExtPreference::System )
		return "system";
	return toString( static_cast<ContrastPreference>( pref ) );
}

} // namespace EE::UI
