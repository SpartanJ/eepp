#ifndef EE_UI_CSSLAYOUTTYPES_HPP
#define EE_UI_CSSLAYOUTTYPES_HPP

#include <eepp/config.hpp>
#include <string>
#include <string_view>

namespace EE { namespace UI {

enum class CSSDisplay {
	Inline,
	Block,
	InlineBlock,
	ListItem,
	Flex,
	InlineFlex,
	Grid,
	InlineGrid,
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
	UpperRoman,
	DisclosureClosed,
	DisclosureOpen
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

enum class CSSBoxSizing { ContentBox, BorderBox };

struct EE_API CSSBoxSizingHelper {
	static std::string toString( CSSBoxSizing val );

	static CSSBoxSizing fromString( std::string_view val );
};

enum class CSSFlexDirection { Row, RowReverse, Column, ColumnReverse };

struct EE_API CSSFlexDirectionHelper {
	static std::string toString( CSSFlexDirection val );
	static CSSFlexDirection fromString( std::string_view val );
};

enum class CSSFlexWrap { NoWrap, Wrap, WrapReverse };

struct EE_API CSSFlexWrapHelper {
	static std::string toString( CSSFlexWrap val );
	static CSSFlexWrap fromString( std::string_view val );
};

enum class CSSJustifyContent { FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, SpaceEvenly };

struct EE_API CSSJustifyContentHelper {
	static std::string toString( CSSJustifyContent val );
	static CSSJustifyContent fromString( std::string_view val );
};

enum class CSSAlignItems { FlexStart, FlexEnd, Center, Baseline, Stretch };

struct EE_API CSSAlignItemsHelper {
	static std::string toString( CSSAlignItems val );
	static CSSAlignItems fromString( std::string_view val );
};

enum class CSSAlignContent {
	FlexStart,
	FlexEnd,
	Center,
	SpaceBetween,
	SpaceAround,
	SpaceEvenly,
	Stretch
};

struct EE_API CSSAlignContentHelper {
	static std::string toString( CSSAlignContent val );
	static CSSAlignContent fromString( std::string_view val );
};

enum class CSSAlignSelf { Auto, FlexStart, FlexEnd, Center, Baseline, Stretch };

struct EE_API CSSAlignSelfHelper {
	static std::string toString( CSSAlignSelf val );
	static CSSAlignSelf fromString( std::string_view val );
};

enum class CSSBaselineAlignment {
	Baseline,
	Sub,
	Super,
	TextTop,
	TextBottom,
	Middle,
	Top,
	Bottom,
	Length,
	Percentage,
	Auto
};

struct EE_API CSSBaselineAlignValue {
	CSSBaselineAlignment type{ CSSBaselineAlignment::Baseline };
	Float value{ 0.f };

	bool operator==( const CSSBaselineAlignValue& other ) const {
		return type == other.type && value == other.value;
	}

	bool operator!=( const CSSBaselineAlignValue& other ) const { return !( *this == other ); }
};

struct EE_API CSSBaselineAlignmentHelper {
	static std::string_view toString( const CSSBaselineAlignValue& val );

	static CSSBaselineAlignValue fromKeyword( std::string_view val );
};

enum class CSSGridAutoFlow { Row, Column };

struct EE_API CSSGridAutoFlowHelper {
	static std::string toString( CSSGridAutoFlow val );
	static CSSGridAutoFlow fromString( std::string_view val );
};

enum class CSSJustifyItems { Normal, Stretch, Start, End, Center, FlexStart, FlexEnd };

struct EE_API CSSJustifyItemsHelper {
	static std::string toString( CSSJustifyItems val );
	static CSSJustifyItems fromString( std::string_view val );
};

enum class CSSJustifySelf { Auto, Normal, Stretch, Start, End, Center, FlexStart, FlexEnd };

struct EE_API CSSJustifySelfHelper {
	static std::string toString( CSSJustifySelf val );
	static CSSJustifySelf fromString( std::string_view val );
};

enum class CSSVisibility { Visible, Hidden, Collapse };

struct EE_API CSSVisibilityHelper {
	static std::string toString( CSSVisibility val );
	static CSSVisibility fromString( std::string_view val );
};

}} // namespace EE::UI

#endif
