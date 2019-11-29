#ifndef EE_UI_CSS_PROPERTYDEFINITION_HPP
#define EE_UI_CSS_PROPERTYDEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace UI { namespace CSS {

/* Shorthands:
 *
 * background
 * foreground
 * margin
 * layout_margin
 * padding
 * background-position
 * rotation-origin-point
 * scale-origin-point
 * */

enum class PropertyId : Uint32 {
	Id = String::hash( "id" ),
	Class = String::hash( "class" ),
	X = String::hash( "x" ),
	Y = String::hash( "y" ),
	Width = String::hash( "width" ),
	Height = String::hash( "height" ),
	BackgroundColor = String::hash( "background-color" ),
	BackgroundImage = String::hash( "background-image" ),
	BackgroundPositionX = String::hash( "background-position-x" ),
	BackgroundPositionY = String::hash( "background-position-y" ),
	BackgroundRepeat = String::hash( "background-repeat" ),
	BackgroundSize = String::hash( "background-size" ),
	ForegroundColor = String::hash( "foreground-color" ),
	ForegroundImage = String::hash( "foreground-image" ),
	ForegroundPositionX = String::hash( "foreground-position-x" ),
	ForegroundPositionY = String::hash( "foreground-position-y" ),
	ForegroundRepeat = String::hash( "foreground-repeat" ),
	ForegroundSize = String::hash( "foreground-size" ),
	ForegroundRadius = String::hash( "foreground-radius" ),
	BorderColor = String::hash( "border-color" ),
	BorderWidth = String::hash( "border-width" ),
	BorderRadius = String::hash( "border-radius" ),
	Visible = String::hash( "visible" ),
	Enabled = String::hash( "enabled" ),
	Theme = String::hash( "theme" ),
	Skin = String::hash( "skin" ),
	SkinColor = String::hash( "skin-color" ),
	Gravity = String::hash( "gravity" ),
	Flags = String::hash( "flags" ),
	MarginLeft = String::hash( "margin-left" ),
	MarginRight = String::hash( "margin-right" ),
	MarginTop = String::hash( "margin-top" ),
	MarginBottom = String::hash( "margin-bottom" ),
	Tooltip = String::hash( "tooltip" ),
	LayoutWeight = String::hash( "layout-weight" ),
	LayoutGravity = String::hash( "layout-gravity" ),
	LayoutWidth = String::hash( "layout-width" ),
	LayoutHeight = String::hash( "layout-height" ),
	LayoutToLeftOf = String::hash( "layout-to-left-of" ),
	LayoutToRightOf = String::hash( "layout-to-right-of" ),
	LayoutToTopOf = String::hash( "layout-to-top-of" ),
	LayoutToBottomOf = String::hash( "layout-to-bottom-of" ),
	Clip = String::hash( "clip" ),
	Rotation = String::hash( "rotation" ),
	Scale = String::hash( "scale" ),
	RotationOriginPointX = String::hash( "rotation-origin-point-x" ),
	RotationOriginPointY = String::hash( "rotation-origin-point-y" ),
	ScaleOriginPointX = String::hash( "scale-origin-point-x" ),
	ScaleOriginPointY = String::hash( "scale-origin-point-y" ),
	BlendMode = String::hash( "blend-mode" ),
	PaddingLeft = String::hash( "padding-left" ),
	PaddingRight = String::hash( "padding-right" ),
	PaddingTop = String::hash( "padding-top" ),
	PaddingBottom = String::hash( "padding-bottom" ),
	Opacity = String::hash( "opacity" ),
	Cursor = String::hash( "cursor" ),
	Text = String::hash( "text" ),
	Color = String::hash( "color" ),
	ShadowColor = String::hash( "shadow-color" ),
	SelectedColor = String::hash( "selected-color" ),
	SelectionBackColor = String::hash( "selection-back-color" ),
	FontFamily = String::hash( "font-family" ),
	FontSize = String::hash( "font-size" ),
	FontStyle = String::hash( "font-style" ),
	TextDecoration = String::hash( "text-decoration" ),
	Wordwrap = String::hash( "wordwrap" ),
	TextStrokeWidth = String::hash( "text-stroke-width" ),
	TextStrokeColor = String::hash( "text-stroke-color" ),
	TextSelection = String::hash( "text-selection" ),
	TextAlign = String::hash( "text-align" ),
	Icon = String::hash( "icon" ),
	MinIconSize = String::hash( "min-icon-size" ),
	IconHorizontalMargin = String::hash( "icon-horizontal-margin" ),
	IconAutoMargin = String::hash( "icon-auto-margin" ),
	Src = String::hash( "src" ),
	ScaleType = String::hash( "scale-type" ),
	Tint = String::hash( "tint" ),
	BackgroundPosition = String::hash( "background-position" ),
	ForegroundPosition = String::hash( "foreground-position" ),
	RotationOriginPoint = String::hash( "rotation-origin-point" ),
	ScaleOriginPoint = String::hash( "scale-origin-point" ),
	MaxTextLength = String::hash( "max-text-length" ),
	MinTabWidth = String::hash( "min-tab-width" ),
	MaxTabWidth = String::hash( "max-tab-width" ),
	TabClosable = String::hash( "tab-closable" ),
	SpecialBorderTabs = String::hash( "special-border-tabs" ),
	LineBelowTabs = String::hash( "line-below-tabs" ),
	LineBelowTabsColor = String::hash( "line-below-tabs-color" ),
	LineBelowTabsYOffset = String::hash( "line-below-tabs-y-offset" ),
	TabSeparation = String::hash( "tax-separation" ),
	Selected = String::hash( "selected" ),
	PopUpToMainControl = String::hash( "popup-to-main-control" ),
	MaxVisibleItems = String::hash( "max-visible-items" ),
	SelectedIndex = String::hash( "selected-index" ),
	SelectedText = String::hash( "selected-text" ),
	ScrollBarType = String::hash( "scrollbar-type" ),
	RowHeight = String::hash( "row-height" ),
	VScrollMode = String::hash( "vscroll-mode" ),
	HScrollMode = String::hash( "hscroll-mode" ),
	ColumnSpan = String::hash( "column-span" ),
	RowSpan = String::hash( "row-span" ),
	Span = String::hash( "span" ),
	ColumnMode = String::hash( "column-mode" ),
	RowMode = String::hash( "row-mode" ),
	ColumnWeight = String::hash( "column-weight" ),
	ColumnHeight = String::hash( "column-height" ),
	ColumnWidth = String::hash( "column-width" ),
	RowWeight = String::hash( "row-weight" ),
	ReverseDraw = String::hash( "reverse-draw" ),
	Orientation = String::hash( "orientation" ),
	Indeterminate = String::hash( "indeterminate" ),
	MaxProgress = String::hash( "max-progress" ),
	Progress = String::hash( "progress" ),
	FillColor = String::hash( "fill-color" ),
	Radius = String::hash( "radius" ),
	OutlineThickness = String::hash( "outline-thickness" ),
	AnimationSpeed = String::hash( "animation-speed" ),
	ArcStartAngle = String::hash( "arc-start-angle" ),
	MinWidth = String::hash( "min-width" ),
	MinMarginRight = String::hash( "min-margin-right" ),
	MinIconSpace = String::hash( "min-icon-space" ),
	TotalSteps = String::hash( "total-steps" ),
	VerticalExpand = String::hash( "vertical-expand" ),
	DisplayPercent = String::hash( "display-percent" ),
	FillerPaddingLeft = String::hash( "filler-padding-left" ),
	FillerPaddingTop = String::hash( "filler-padding-top" ),
	FillerPaddingRight = String::hash( "filler-padding-right" ),
	FillerPaddingBottom = String::hash( "filler-padding-bottom" ),
	MovementSpeed = String::hash( "movement-speed" ),
	MinValue = String::hash( "min-value" ),
	MaxValue = String::hash( "max-value" ),
	Value = String::hash( "value" ),
	ClickStep = String::hash( "click-step" ),
	PageStep = String::hash( "page-step" ),
	BackgroundExpand = String::hash( "background-expand" ),
	ScrollBarMode = String::hash( "scrollbar-mode" ),
	HalfSlider = String::hash( "half-slider" ),
	Name = String::hash( "name" ),
	Owns = String::hash( "owns" ),
	AllowEditing = String::hash( "allow-editing" ),
	MaxLength = String::hash( "max-length" ),
	FreeEditing = String::hash( "free-editing" ),
	OnlyNumbers = String::hash( "only-numbers" ),
	AllowDot = String::hash( "allow-dot" ),
	TouchDrag = String::hash( "touch-drag" ),
	TouchDragDeceleration = String::hash( "touch-drag-deceleration" ),
	Title = String::hash( "title" ),
	BaseAlpha = String::hash( "base-alpha" ),
	ButtonsPositionOffset = String::hash( "buttons-position-offset" ),
	WindowFlags = String::hash( "window-flags" ),
	DecorationSize = String::hash( "decoration-size" ),
	BorderSize = String::hash( "border-size" ),
	MinWindowSize = String::hash( "min-window-size" ),
	ButtonsSeparation = String::hash( "buttons-separation" ),
	MinCornerDistance = String::hash( "min-corner-distance" ),
	DecorationAutoSize = String::hash( "decoration-auto-size" ),
	BorderAutoSize = String::hash( "border-auto-size" ),
	MarginBetweenButtons = String::hash( "margin-between-buttons" ),
	ButtonMargin = String::hash( "button-margin" ),
	MenuHeight = String::hash( "menu-height" ),
	FirstButtonMarginLeft = String::hash( "first-button-margin-left" )
};

enum class PropertyType : Uint32 {
	Undefined,
	String,
	Bool,
	NumberInt,
	NumberFloat,
	NumberLength,
	Color,
	Vector2,
	BackgroundSize
};

enum class PropertyRelativeTarget : Uint32 {
	None,
	ContainingBlockWidth,
	ContainingBlockHeight,
	FontSize,
	ParentFontSize,
	LineHeight,
	LocalBlockWidth,
	LocalBlockHeight
};

class EE_API PropertyDefinition {
  public:
	static PropertyDefinition* New( const std::string& name, const std::string& defaultValue,
									const bool& inherited = false );

	PropertyDefinition( const std::string& name, const std::string& defaultValue,
						const bool& inherited = false );

	const std::string& getName() const;

	const Uint32& getId() const;

	PropertyId getPropertyId() const;

	const std::string& getDefaultValue() const;

	bool getInherited() const;

	const PropertyRelativeTarget& getRelativeTarget() const;

	PropertyDefinition& setRelativeTarget( const PropertyRelativeTarget& relativeTarget );

	PropertyDefinition& setType( const PropertyType& propertyType );

	const PropertyType& getType() const;

	PropertyDefinition& addAlias( const std::string& alias );

	bool isAlias( const std::string& alias ) const;

	bool isAlias( const Uint32& id ) const;

	bool isDefinition( const std::string& name ) const;

	bool isDefinition( const Uint32& id ) const;

  protected:
	std::string mName;
	Uint32 mId;
	std::vector<std::string> mAliases;
	std::vector<Uint32> mAliasesHash;
	std::string mDefaultValue;
	bool mInherited;
	PropertyRelativeTarget mRelativeTarget;
	PropertyType mPropertyType;
};

}}} // namespace EE::UI::CSS

#endif
