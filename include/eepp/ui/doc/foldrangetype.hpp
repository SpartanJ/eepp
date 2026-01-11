#ifndef EE_UI_DOC_FOLDRANGETYPE_HPP
#define EE_UI_DOC_FOLDRANGETYPE_HPP

#include <eepp/core/string.hpp>
#include <string>

namespace EE { namespace UI { namespace Doc {

enum class FoldRangeType { Braces, Indentation, Tag, Markdown, Undefined };

struct FoldRangeTypeUtil {
	static FoldRangeType fromString( const std::string& str ) {
		if ( String::iequals( str, "braces" ) ) {
			return FoldRangeType::Braces;
		} else if ( String::iequals( str, "indentation" ) ) {
			return FoldRangeType::Indentation;
		} else if ( String::iequals( str, "tag" ) ) {
			return FoldRangeType::Tag;
		} else if ( String::iequals( str, "markdown" ) ) {
			return FoldRangeType::Markdown;
		}
		return FoldRangeType::Undefined;
	}

	static std::string toString( FoldRangeType type ) {
		switch ( type ) {
			case FoldRangeType::Braces:
				return "braces";
			case FoldRangeType::Indentation:
				return "indentation";
			case FoldRangeType::Tag:
				return "tag";
			case FoldRangeType::Markdown:
				return "markdown";
			case FoldRangeType::Undefined:
			default:
				break;
		}
		return "";
	}
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_FOLDRANGETYPE_HPP
