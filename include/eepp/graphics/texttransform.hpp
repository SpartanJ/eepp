#ifndef EE_GRAPHICS_TEXTTRANSFORM_HPP
#define EE_GRAPHICS_TEXTTRANSFORM_HPP

#include <eepp/core/string.hpp>

namespace EE { namespace Graphics {

class TextTransform {
  public:
	enum Value { LowerCase, UpperCase, Capitalize, None };

	static TextTransform::Value fromString( std::string str ) {
		String::toLowerInPlace( str );
		if ( str == "lowercase" )
			return LowerCase;
		else if ( str == "uppercase" )
			return UpperCase;
		else if ( str == "capitalize" )
			return Capitalize;
		return None;
	}

	static std::string toString( const TextTransform::Value& val ) {
		switch ( val ) {
			case LowerCase:
				return "lowercase";
			case UpperCase:
				return "uppercase";
			case Capitalize:
				return "capitalize";
			case None:
				return "none";
		}
		return "none";
	}
};

}} // namespace EE::Graphics

#endif // EE_GRAPHICS_TEXTTRANSFORM_HPP
