#include <eepp/ui/csslayouttypes.hpp>

namespace EE { namespace UI {

std::string CSSDisplayHelper::toString( CSSDisplay display ) {
	switch ( display ) {
		case CSSDisplay::Inline:
			return "inline";
		case CSSDisplay::InlineBlock:
			return "inline-block";
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

}} // namespace EE::UI
