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

} // namespace EE::UI
