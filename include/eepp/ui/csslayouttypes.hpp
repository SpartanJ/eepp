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

}} // namespace EE::UI

#endif
