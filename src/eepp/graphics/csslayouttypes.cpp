#include <eepp/core/string.hpp>
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
		case CSSDisplay::InlineFlex:
			return "inline-flex";
		case CSSDisplay::Grid:
			return "grid";
		case CSSDisplay::InlineGrid:
			return "inline-grid";
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
	else if ( val == "inline-flex" )
		display = CSSDisplay::InlineFlex;
	else if ( val == "grid" )
		display = CSSDisplay::Grid;
	else if ( val == "inline-grid" )
		display = CSSDisplay::InlineGrid;
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

std::string CSSBoxSizingHelper::toString( CSSBoxSizing val ) {
	switch ( val ) {
		case CSSBoxSizing::BorderBox:
			return "border-box";
		case CSSBoxSizing::ContentBox:
		default:
			return "content-box";
	}
}

CSSBoxSizing CSSBoxSizingHelper::fromString( std::string_view val ) {
	if ( val == "border-box" )
		return CSSBoxSizing::BorderBox;
	return CSSBoxSizing::ContentBox;
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
		case CSSListStyleType::DisclosureClosed:
			return "disclosure-closed";
		case CSSListStyleType::DisclosureOpen:
			return "disclosure-open";
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
	if ( val == "disclosure-closed" )
		return CSSListStyleType::DisclosureClosed;
	if ( val == "disclosure-open" )
		return CSSListStyleType::DisclosureOpen;
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

std::string CSSFloatHelper::toString( CSSFloat val ) {
	switch ( val ) {
		case CSSFloat::Left:
			return "left";
		case CSSFloat::Right:
			return "right";
		case CSSFloat::None:
		default:
			return "none";
	}
}

CSSFloat CSSFloatHelper::fromString( std::string_view val ) {
	if ( val == "left" )
		return CSSFloat::Left;
	if ( val == "right" )
		return CSSFloat::Right;
	return CSSFloat::None;
}

std::string CSSClearHelper::toString( CSSClear val ) {
	switch ( val ) {
		case CSSClear::Left:
			return "left";
		case CSSClear::Right:
			return "right";
		case CSSClear::Both:
			return "both";
		case CSSClear::None:
		default:
			return "none";
	}
}

CSSClear CSSClearHelper::fromString( std::string_view val ) {
	if ( val == "left" )
		return CSSClear::Left;
	if ( val == "right" )
		return CSSClear::Right;
	if ( val == "both" )
		return CSSClear::Both;
	return CSSClear::None;
}

std::string_view CSSBaselineAlignmentHelper::toString( const CSSBaselineAlignValue& val ) {
	switch ( val.type ) {
		case CSSBaselineAlignment::Sub:
			return "sub";
		case CSSBaselineAlignment::Super:
			return "super";
		case CSSBaselineAlignment::TextTop:
			return "text-top";
		case CSSBaselineAlignment::TextBottom:
			return "text-bottom";
		case CSSBaselineAlignment::Middle:
			return "middle";
		case CSSBaselineAlignment::Top:
			return "top";
		case CSSBaselineAlignment::Bottom:
			return "bottom";
		case CSSBaselineAlignment::Length:
			return "length";
		case CSSBaselineAlignment::Percentage:
			return "percentage";
		case CSSBaselineAlignment::Auto:
			return "auto";
		case CSSBaselineAlignment::Baseline:
		default:
			return "baseline";
	}
}

CSSBaselineAlignValue CSSBaselineAlignmentHelper::fromKeyword( std::string_view val ) {
	CSSBaselineAlignValue out;

	switch ( String::hashToLower( val ) ) {
		case String::hash( "auto" ):
			out.type = CSSBaselineAlignment::Auto;
			break;
		case String::hash( "sub" ):
			out.type = CSSBaselineAlignment::Sub;
			break;
		case String::hash( "super" ):
			out.type = CSSBaselineAlignment::Super;
			break;
		case String::hash( "text-top" ):
		case String::hash( "text-before-edge" ):
		case String::hash( "before-edge" ):
		case String::hash( "hanging" ):
			out.type = CSSBaselineAlignment::TextTop;
			break;
		case String::hash( "text-bottom" ):
		case String::hash( "text-after-edge" ):
		case String::hash( "after-edge" ):
		case String::hash( "mathematical" ):
			out.type = CSSBaselineAlignment::TextBottom;
			break;
		case String::hash( "middle" ):
		case String::hash( "central" ):
			out.type = CSSBaselineAlignment::Middle;
			break;
		case String::hash( "top" ):
			out.type = CSSBaselineAlignment::Top;
			break;
		case String::hash( "bottom" ):
			out.type = CSSBaselineAlignment::Bottom;
			break;
		case String::hash( "baseline" ):
		default:
			out.type = CSSBaselineAlignment::Baseline;
			break;
	}

	return out;
}

std::string CSSFlexDirectionHelper::toString( CSSFlexDirection val ) {
	switch ( val ) {
		case CSSFlexDirection::RowReverse:
			return "row-reverse";
		case CSSFlexDirection::Column:
			return "column";
		case CSSFlexDirection::ColumnReverse:
			return "column-reverse";
		case CSSFlexDirection::Row:
		default:
			return "row";
	}
}

CSSFlexDirection CSSFlexDirectionHelper::fromString( std::string_view val ) {
	if ( val == "row-reverse" )
		return CSSFlexDirection::RowReverse;
	if ( val == "column" )
		return CSSFlexDirection::Column;
	if ( val == "column-reverse" )
		return CSSFlexDirection::ColumnReverse;
	return CSSFlexDirection::Row;
}

std::string CSSFlexWrapHelper::toString( CSSFlexWrap val ) {
	switch ( val ) {
		case CSSFlexWrap::Wrap:
			return "wrap";
		case CSSFlexWrap::WrapReverse:
			return "wrap-reverse";
		case CSSFlexWrap::NoWrap:
		default:
			return "nowrap";
	}
}

CSSFlexWrap CSSFlexWrapHelper::fromString( std::string_view val ) {
	if ( val == "wrap" )
		return CSSFlexWrap::Wrap;
	if ( val == "wrap-reverse" )
		return CSSFlexWrap::WrapReverse;
	return CSSFlexWrap::NoWrap;
}

std::string CSSJustifyContentHelper::toString( CSSJustifyContent val ) {
	switch ( val ) {
		case CSSJustifyContent::FlexEnd:
			return "flex-end";
		case CSSJustifyContent::Center:
			return "center";
		case CSSJustifyContent::SpaceBetween:
			return "space-between";
		case CSSJustifyContent::SpaceAround:
			return "space-around";
		case CSSJustifyContent::SpaceEvenly:
			return "space-evenly";
		case CSSJustifyContent::FlexStart:
		default:
			return "flex-start";
	}
}

CSSJustifyContent CSSJustifyContentHelper::fromString( std::string_view val ) {
	if ( val == "flex-end" )
		return CSSJustifyContent::FlexEnd;
	if ( val == "center" )
		return CSSJustifyContent::Center;
	if ( val == "space-between" )
		return CSSJustifyContent::SpaceBetween;
	if ( val == "space-around" )
		return CSSJustifyContent::SpaceAround;
	if ( val == "space-evenly" )
		return CSSJustifyContent::SpaceEvenly;
	return CSSJustifyContent::FlexStart;
}

std::string CSSAlignItemsHelper::toString( CSSAlignItems val ) {
	switch ( val ) {
		case CSSAlignItems::FlexEnd:
			return "flex-end";
		case CSSAlignItems::Center:
			return "center";
		case CSSAlignItems::Baseline:
			return "baseline";
		case CSSAlignItems::Stretch:
			return "stretch";
		case CSSAlignItems::FlexStart:
		default:
			return "flex-start";
	}
}

CSSAlignItems CSSAlignItemsHelper::fromString( std::string_view val ) {
	if ( val == "flex-end" )
		return CSSAlignItems::FlexEnd;
	if ( val == "center" )
		return CSSAlignItems::Center;
	if ( val == "baseline" )
		return CSSAlignItems::Baseline;
	if ( val == "stretch" )
		return CSSAlignItems::Stretch;
	return CSSAlignItems::FlexStart;
}

std::string CSSAlignContentHelper::toString( CSSAlignContent val ) {
	switch ( val ) {
		case CSSAlignContent::FlexEnd:
			return "flex-end";
		case CSSAlignContent::Center:
			return "center";
		case CSSAlignContent::SpaceBetween:
			return "space-between";
		case CSSAlignContent::SpaceAround:
			return "space-around";
		case CSSAlignContent::SpaceEvenly:
			return "space-evenly";
		case CSSAlignContent::Stretch:
			return "stretch";
		case CSSAlignContent::FlexStart:
		default:
			return "flex-start";
	}
}

CSSAlignContent CSSAlignContentHelper::fromString( std::string_view val ) {
	if ( val == "flex-end" )
		return CSSAlignContent::FlexEnd;
	if ( val == "center" )
		return CSSAlignContent::Center;
	if ( val == "space-between" )
		return CSSAlignContent::SpaceBetween;
	if ( val == "space-around" )
		return CSSAlignContent::SpaceAround;
	if ( val == "space-evenly" )
		return CSSAlignContent::SpaceEvenly;
	if ( val == "stretch" )
		return CSSAlignContent::Stretch;
	return CSSAlignContent::FlexStart;
}

std::string CSSAlignSelfHelper::toString( CSSAlignSelf val ) {
	switch ( val ) {
		case CSSAlignSelf::FlexStart:
			return "flex-start";
		case CSSAlignSelf::FlexEnd:
			return "flex-end";
		case CSSAlignSelf::Center:
			return "center";
		case CSSAlignSelf::Baseline:
			return "baseline";
		case CSSAlignSelf::Stretch:
			return "stretch";
		case CSSAlignSelf::Auto:
		default:
			return "auto";
	}
}

CSSAlignSelf CSSAlignSelfHelper::fromString( std::string_view val ) {
	if ( val == "flex-start" )
		return CSSAlignSelf::FlexStart;
	if ( val == "flex-end" )
		return CSSAlignSelf::FlexEnd;
	if ( val == "center" )
		return CSSAlignSelf::Center;
	if ( val == "baseline" )
		return CSSAlignSelf::Baseline;
	if ( val == "stretch" )
		return CSSAlignSelf::Stretch;
	return CSSAlignSelf::Auto;
}

std::string CSSGridAutoFlowHelper::toString( CSSGridAutoFlow val ) {
	switch ( val ) {
		case CSSGridAutoFlow::Column:
			return "column";
		case CSSGridAutoFlow::Row:
		default:
			return "row";
	}
}

CSSGridAutoFlow CSSGridAutoFlowHelper::fromString( std::string_view val ) {
	if ( val == "column" )
		return CSSGridAutoFlow::Column;
	return CSSGridAutoFlow::Row;
}

std::string CSSJustifyItemsHelper::toString( CSSJustifyItems val ) {
	switch ( val ) {
		case CSSJustifyItems::Stretch:
			return "stretch";
		case CSSJustifyItems::Start:
			return "start";
		case CSSJustifyItems::End:
			return "end";
		case CSSJustifyItems::Center:
			return "center";
		case CSSJustifyItems::FlexStart:
			return "flex-start";
		case CSSJustifyItems::FlexEnd:
			return "flex-end";
		case CSSJustifyItems::Normal:
		default:
			return "normal";
	}
}

CSSJustifyItems CSSJustifyItemsHelper::fromString( std::string_view val ) {
	if ( val == "stretch" )
		return CSSJustifyItems::Stretch;
	if ( val == "start" )
		return CSSJustifyItems::Start;
	if ( val == "end" )
		return CSSJustifyItems::End;
	if ( val == "center" )
		return CSSJustifyItems::Center;
	if ( val == "flex-start" )
		return CSSJustifyItems::FlexStart;
	if ( val == "flex-end" )
		return CSSJustifyItems::FlexEnd;
	return CSSJustifyItems::Normal;
}

std::string CSSJustifySelfHelper::toString( CSSJustifySelf val ) {
	switch ( val ) {
		case CSSJustifySelf::Stretch:
			return "stretch";
		case CSSJustifySelf::Start:
			return "start";
		case CSSJustifySelf::End:
			return "end";
		case CSSJustifySelf::Center:
			return "center";
		case CSSJustifySelf::FlexStart:
			return "flex-start";
		case CSSJustifySelf::FlexEnd:
			return "flex-end";
		case CSSJustifySelf::Normal:
			return "normal";
		case CSSJustifySelf::Auto:
		default:
			return "auto";
	}
}

CSSJustifySelf CSSJustifySelfHelper::fromString( std::string_view val ) {
	if ( val == "stretch" )
		return CSSJustifySelf::Stretch;
	if ( val == "start" )
		return CSSJustifySelf::Start;
	if ( val == "end" )
		return CSSJustifySelf::End;
	if ( val == "center" )
		return CSSJustifySelf::Center;
	if ( val == "flex-start" )
		return CSSJustifySelf::FlexStart;
	if ( val == "flex-end" )
		return CSSJustifySelf::FlexEnd;
	if ( val == "normal" )
		return CSSJustifySelf::Normal;
	return CSSJustifySelf::Auto;
}

std::string CSSVisibilityHelper::toString( CSSVisibility val ) {
	switch ( val ) {
		case CSSVisibility::Visible:
			return "visible";
		case CSSVisibility::Hidden:
			return "hidden";
		case CSSVisibility::Collapse:
			return "collapse";
	}
	return "visible";
}

CSSVisibility CSSVisibilityHelper::fromString( std::string_view val ) {
	if ( val == "hidden" )
		return CSSVisibility::Hidden;
	if ( val == "collapse" )
		return CSSVisibility::Collapse;
	return CSSVisibility::Visible;
}

}} // namespace EE::UI
