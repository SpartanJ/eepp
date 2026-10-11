#include <eepp/core/string.hpp>
#include <eepp/ui/uidirection.hpp>

namespace EE { namespace UI {

UIDirectionValue UIDirectionHelper::fromString( std::string_view str ) {
	if ( String::iequals( str, "rtl" ) )
		return UIDirectionValue::RightToLeft;
	if ( String::iequals( str, "ltr" ) )
		return UIDirectionValue::LeftToRight;
	// "inherit", "auto" (HTML dir=auto is not yet modeled), "unset" and unknown values inherit.
	return UIDirectionValue::Inherit;
}

std::string UIDirectionHelper::toString( UIDirectionValue direction ) {
	switch ( direction ) {
		case UIDirectionValue::LeftToRight:
			return "ltr";
		case UIDirectionValue::RightToLeft:
			return "rtl";
		case UIDirectionValue::Inherit:
		default:
			return "inherit";
	}
}

std::string UIDirectionHelper::toString( UIDirection direction ) {
	return direction == UIDirection::RightToLeft ? "rtl" : "ltr";
}

UIDirection UIDirectionHelper::fromLocale( std::string_view locale ) {
	// Only the language subtag decides the conventional direction. Script subtags such as
	// "az-Arab" are rare enough that they are not modeled here.
	size_t end = 0;
	while ( end < locale.size() && locale[end] != '_' && locale[end] != '-' && locale[end] != '.' &&
			locale[end] != '@' )
		end++;
	const std::string_view lang = locale.substr( 0, end );
	// Languages conventionally written in a right-to-left script. Languages that are mostly
	// written in Latin script today (Hausa, Kurmanji) are intentionally not listed.
	static constexpr std::string_view sRTLLanguages[] = { "ar", "arc", "ckb", "dv", "fa", "he",
														  "iw", "khw", "ks",  "ps", "sd", "syr",
														  "ug", "ur",  "yi",  "ji", "nqo" };
	for ( const auto& rtl : sRTLLanguages ) {
		if ( String::iequals( lang, rtl ) )
			return UIDirection::RightToLeft;
	}
	return UIDirection::LeftToRight;
}

bool UIHorizontalAlignmentHelper::fromString( std::string_view str,
											  UIHorizontalAlignment& alignment ) {
	if ( String::iequals( str, "left" ) ) {
		alignment = UIHorizontalAlignment::Left;
	} else if ( String::iequals( str, "right" ) ) {
		alignment = UIHorizontalAlignment::Right;
	} else if ( String::iequals( str, "center" ) || String::iequals( str, "center_horizontal" ) ) {
		alignment = UIHorizontalAlignment::Center;
	} else if ( String::iequals( str, "start" ) ) {
		alignment = UIHorizontalAlignment::Start;
	} else if ( String::iequals( str, "end" ) ) {
		alignment = UIHorizontalAlignment::End;
	} else {
		return false;
	}
	return true;
}

std::string UIHorizontalAlignmentHelper::toString( UIHorizontalAlignment alignment ) {
	switch ( alignment ) {
		case UIHorizontalAlignment::Center:
			return "center";
		case UIHorizontalAlignment::Right:
			return "right";
		case UIHorizontalAlignment::Start:
			return "start";
		case UIHorizontalAlignment::End:
			return "end";
		case UIHorizontalAlignment::Left:
		default:
			return "left";
	}
}

}} // namespace EE::UI
