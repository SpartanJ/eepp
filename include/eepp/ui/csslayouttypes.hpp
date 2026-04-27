#ifndef EE_UI_CSSLAYOUTTYPES_HPP
#define EE_UI_CSSLAYOUTTYPES_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace UI {

enum class CSSDisplay {
	Inline,
	Block,
	InlineBlock,
	Flex,
	None,
	Table,
	TableRow,
	TableCell,
	TableHead,
	TableBody,
	TableFooter
};

struct CSSDisplayHelper {
	static std::string toString( CSSDisplay display );

	static CSSDisplay fromString( std::string_view val );
};

enum class CSSPosition { Static, Relative, Absolute, Fixed, Sticky };

struct CSSPositionHelper {
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

struct CSSListStyleTypeHelper {
	static std::string toString( CSSListStyleType type );

	static CSSListStyleType fromString( std::string_view val );
};

enum class CSSListStylePosition { Outside, Inside };

struct CSSListStylePositionHelper {
	static std::string toString( CSSListStylePosition pos );

	static CSSListStylePosition fromString( std::string_view val );
};

}} // namespace EE::UI

#endif
