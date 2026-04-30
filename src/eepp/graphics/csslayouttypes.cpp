#include <eepp/ui/csslayouttypes.hpp>

namespace EE { namespace UI {

std::string CSSDisplayHelper::toString( CSSDisplay display ) {
	switch ( display ) {
		case CSSDisplay::Inline:
			return "inline";
		case CSSDisplay::InlineBlock:
			return "inline-block";
		case CSSDisplay::ListItem:
			return "list-item";
		case CSSDisplay::Flex:
			return "flex";
		case CSSDisplay::None:
			return "none";
		case CSSDisplay::Table:
			return "table";
		case CSSDisplay::TableRow:
			return "table-row";
		case CSSDisplay::TableCell:
			return "table-cell";
		case CSSDisplay::TableHead:
			return "table-header-group";
		case CSSDisplay::TableBody:
			return "table-row-group";
		case CSSDisplay::TableFooter:
			return "table-footer-group";
		case CSSDisplay::Block:
		default:
			return "block";
	}
};

CSSDisplay CSSDisplayHelper::fromString( std::string_view val ) {
	CSSDisplay display = CSSDisplay::Block;
	if ( val == "inline" )
		display = CSSDisplay::Inline;
	else if ( val == "inline-block" )
		display = CSSDisplay::InlineBlock;
	else if ( val == "list-item" )
		display = CSSDisplay::ListItem;
	else if ( val == "none" )
		display = CSSDisplay::None;
	else if ( val == "table" )
		display = CSSDisplay::Table;
	else if ( val == "table-row" )
		display = CSSDisplay::TableRow;
	else if ( val == "table-cell" )
		display = CSSDisplay::TableCell;
	else if ( val == "table-header-group" )
		display = CSSDisplay::TableHead;
	else if ( val == "table-row-group" )
		display = CSSDisplay::TableBody;
	else if ( val == "table-footer-group" )
		display = CSSDisplay::TableFooter;
	else if ( val == "flex" )
		display = CSSDisplay::Flex;
	return display;
}

std::string CSSPositionHelper::toString( CSSPosition position ) {
	switch ( position ) {
		case CSSPosition::Relative:
			return "relative";
		case CSSPosition::Absolute:
			return "absolute";
		case CSSPosition::Fixed:
			return "fixed";
		case CSSPosition::Sticky:
			return "sticky";
		case CSSPosition::Static:
		default: {
		}
	}
	return "static";
}

CSSPosition CSSPositionHelper::fromString( std::string_view val ) {
	CSSPosition position = CSSPosition::Static;
	if ( val == "relative" )
		position = CSSPosition::Relative;
	else if ( val == "absolute" )
		position = CSSPosition::Absolute;
	else if ( val == "fixed" )
		position = CSSPosition::Fixed;
	else if ( val == "sticky" )
		position = CSSPosition::Sticky;
	return position;
}

std::string CSSListStyleTypeHelper::toString( CSSListStyleType type ) {
	switch ( type ) {
		case CSSListStyleType::Disc:
			return "disc";
		case CSSListStyleType::Circle:
			return "circle";
		case CSSListStyleType::Square:
			return "square";
		case CSSListStyleType::Decimal:
			return "decimal";
		case CSSListStyleType::LowerAlpha:
			return "lower-alpha";
		case CSSListStyleType::UpperAlpha:
			return "upper-alpha";
		case CSSListStyleType::LowerRoman:
			return "lower-roman";
		case CSSListStyleType::UpperRoman:
			return "upper-roman";
		case CSSListStyleType::None:
		default:
			return "none";
	}
}

CSSListStyleType CSSListStyleTypeHelper::fromString( std::string_view val ) {
	if ( val == "disc" )
		return CSSListStyleType::Disc;
	if ( val == "circle" )
		return CSSListStyleType::Circle;
	if ( val == "square" )
		return CSSListStyleType::Square;
	if ( val == "decimal" )
		return CSSListStyleType::Decimal;
	if ( val == "lower-alpha" )
		return CSSListStyleType::LowerAlpha;
	if ( val == "upper-alpha" )
		return CSSListStyleType::UpperAlpha;
	if ( val == "lower-roman" )
		return CSSListStyleType::LowerRoman;
	if ( val == "upper-roman" )
		return CSSListStyleType::UpperRoman;
	return CSSListStyleType::None;
}

std::string CSSListStylePositionHelper::toString( CSSListStylePosition pos ) {
	return pos == CSSListStylePosition::Inside ? "inside" : "outside";
}

CSSListStylePosition CSSListStylePositionHelper::fromString( std::string_view val ) {
	if ( val == "inside" )
		return CSSListStylePosition::Inside;
	return CSSListStylePosition::Outside;
}

}} // namespace EE::UI
