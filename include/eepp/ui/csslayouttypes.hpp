#ifndef EE_UI_CSSLAYOUTTYPES_HPP
#define EE_UI_CSSLAYOUTTYPES_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace UI {

enum class CSSDisplay {
	Inline,
	Block,
	InlineBlock,
	ListItem,
	Flex,
	None,
	Table,
	TableRow,
	TableCell,
	TableHead,
	TableBody,
	TableFooter
};

struct EE_API CSSDisplayHelper {
	static std::string toString( CSSDisplay display );

	static CSSDisplay fromString( std::string_view val );
};

enum class CSSPosition { Static, Relative, Absolute, Fixed, Sticky };

struct EE_API CSSPositionHelper {
	static std::string toString( CSSPosition position );

	static CSSPosition fromString( std::string_view val );
};

enum class CSSListStyleType {
	None,
	Disc,
	Circle,
	Square,
	Decimal,
	LowerAlpha,
	UpperAlpha,
	LowerRoman,
	UpperRoman
};

struct EE_API CSSListStyleTypeHelper {
	static std::string toString( CSSListStyleType type );

	static CSSListStyleType fromString( std::string_view val );
};

enum class CSSListStylePosition { Outside, Inside };

struct EE_API CSSListStylePositionHelper {
	static std::string toString( CSSListStylePosition pos );

	static CSSListStylePosition fromString( std::string_view val );
};

enum class CSSFloat { None, Left, Right };

struct EE_API CSSFloatHelper {
	static std::string toString( CSSFloat val );

	static CSSFloat fromString( std::string_view val );
};

enum class CSSClear { None, Left, Right, Both };

struct EE_API CSSClearHelper {
	static std::string toString( CSSClear val );

	static CSSClear fromString( std::string_view val );
};

}} // namespace EE::UI

#endif
