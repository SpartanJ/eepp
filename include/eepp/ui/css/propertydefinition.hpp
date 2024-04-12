#ifndef EE_UI_CSS_PROPERTYDEFINITION_HPP
#define EE_UI_CSS_PROPERTYDEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace UI { namespace CSS {

enum class PropertyId : Uint32 {
	Id = String::hash( "id" ),
	Class = String::hash( "class" ),
	X = String::hash( "x" ),
	Y = String::hash( "y" ),
	Width = String::hash( "width" ),
	Height = String::hash( "height" ),
	BackgroundColor = String::hash( "background-color" ),
	BackgroundImage = String::hash( "background-image" ),
	BackgroundTint = String::hash( "background-tint" ),
	BackgroundPositionX = String::hash( "background-position-x" ),
	BackgroundPositionY = String::hash( "background-position-y" ),
	BackgroundRepeat = String::hash( "background-repeat" ),
	BackgroundSize = String::hash( "background-size" ),
	ForegroundColor = String::hash( "foreground-color" ),
	ForegroundTint = String::hash( "foreground-tint" ),
	ForegroundImage = String::hash( "foreground-image" ),
	ForegroundPositionX = String::hash( "foreground-position-x" ),
	ForegroundPositionY = String::hash( "foreground-position-y" ),
	ForegroundRepeat = String::hash( "foreground-repeat" ),
	ForegroundSize = String::hash( "foreground-size" ),
	ForegroundRadius = String::hash( "foreground-radius" ),
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
	TextTransform = String::hash( "text-transform" ),
	Color = String::hash( "color" ),
	TextShadowColor = String::hash( "text-shadow-color" ),
	TextShadowOffset = String::hash( "text-shadow-offset" ),
	SelectionColor = String::hash( "selection-color" ),
	SelectionBackColor = String::hash( "selection-back-color" ),
	FontFamily = String::hash( "font-family" ),
	FontSize = String::hash( "font-size" ),
	FontStyle = String::hash( "font-style" ),
	TextDecoration = String::hash( "text-decoration" ),
	Wordwrap = String::hash( "word-wrap" ),
	TextStrokeWidth = String::hash( "text-stroke-width" ),
	TextStrokeColor = String::hash( "text-stroke-color" ),
	TextSelection = String::hash( "text-selection" ),
	TextAlign = String::hash( "text-align" ),
	Icon = String::hash( "icon" ),
	MinIconSize = String::hash( "min-icon-size" ),
	MinSize = String::hash( "min-size" ),
	MaxSize = String::hash( "max-size" ),
	Src = String::hash( "src" ),
	ScaleType = String::hash( "scale-type" ),
	Tint = String::hash( "tint" ),
	MaxTextLength = String::hash( "max-text-length" ),
	MinTabWidth = String::hash( "min-tab-width" ),
	MaxTabWidth = String::hash( "max-tab-width" ),
	TabClosable = String::hash( "tab-closable" ),
	TabsEdgesDiffSkin = String::hash( "tabs-edges-diff-skin" ),
	TabSeparation = String::hash( "tab-separation" ),
	TabHeight = String::hash( "tab-height" ),
	TabCloseButtonVisible = String::hash( "tab-close-button-visible" ),
	Selected = String::hash( "selected" ),
	Checked = String::hash( "checked" ),
	PopUpToRoot = String::hash( "popup-to-root" ),
	MaxVisibleItems = String::hash( "max-visible-items" ),
	SelectedIndex = String::hash( "selected-index" ),
	SelectedText = String::hash( "selected-text" ),
	ScrollBarStyle = String::hash( "scrollbar-style" ),
	RowHeight = String::hash( "row-height" ),
	VScrollMode = String::hash( "vscroll-mode" ),
	HScrollMode = String::hash( "hscroll-mode" ),
	ColumnMargin = String::hash( "column-margin" ),
	RowMargin = String::hash( "row-margin" ),
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
	MinHeight = String::hash( "min-height" ),
	MaxWidth = String::hash( "max-width" ),
	MaxHeight = String::hash( "max-height" ),
	TotalSteps = String::hash( "total-steps" ),
	VerticalExpand = String::hash( "vertical-expand" ),
	DisplayPercent = String::hash( "display-percent" ),
	MovementSpeed = String::hash( "movement-speed" ),
	MinValue = String::hash( "min-value" ),
	MaxValue = String::hash( "max-value" ),
	Value = String::hash( "value" ),
	ClickStep = String::hash( "click-step" ),
	PageStep = String::hash( "page-step" ),
	BackgroundExpand = String::hash( "background-expand" ),
	ScrollBarMode = String::hash( "scrollbar-mode" ),
	HalfSlider = String::hash( "half-slider" ),
	Owns = String::hash( "owns" ),
	AllowEditing = String::hash( "allow-editing" ),
	Locked = String::hash( "locked" ),
	MaxLength = String::hash( "max-length" ),
	Numeric = String::hash( "numeric" ),
	AllowFloat = String::hash( "allow-float" ),
	TouchDrag = String::hash( "touch-drag" ),
	TouchDragDeceleration = String::hash( "touch-drag-deceleration" ),
	WindowTitle = String::hash( "window-title" ),
	WindowOpacity = String::hash( "window-opacity" ),
	WindowButtonsOffset = String::hash( "window-buttons-offset" ),
	WindowFlags = String::hash( "window-flags" ),
	WindowTitlebarSize = String::hash( "window-titlebar-size" ),
	WindowBorderSize = String::hash( "window-border-size" ),
	WindowMinSize = String::hash( "window-min-size" ),
	WindowButtonsSeparation = String::hash( "window-buttons-separation" ),
	WindowCornerDistance = String::hash( "window-corner-distance" ),
	WindowTitlebarAutoSize = String::hash( "window-decoration-auto-size" ),
	WindowBorderAutoSize = String::hash( "window-border-auto-size" ),
	Hint = String::hash( "hint" ),
	HintColor = String::hash( "hint-color" ),
	HintShadowColor = String::hash( "hint-shadow-color" ),
	HintShadowOffset = String::hash( "hint-shadow-offset" ),
	HintFontFamily = String::hash( "hint-font-family" ),
	HintFontSize = String::hash( "hint-font-size" ),
	HintFontStyle = String::hash( "hint-font-style" ),
	HintStrokeWidth = String::hash( "hint-stroke-width" ),
	HintStrokeColor = String::hash( "hint-stroke-color" ),
	HintDisplay = String::hash( "hint-display" ),
	Transition = String::hash( "transition" ),
	TransitionDelay = String::hash( "transition-delay" ),
	TransitionDuration = String::hash( "transition-duration" ),
	TransitionProperty = String::hash( "transition-property" ),
	TransitionTimingFunction = String::hash( "transition-timing-function" ),
	Animation = String::hash( "animation" ),
	AnimationName = String::hash( "animation-name" ),
	AnimationDelay = String::hash( "animation-delay" ),
	AnimationDuration = String::hash( "animation-duration" ),
	AnimationFillMode = String::hash( "animation-fill-mode" ),
	AnimationIterationCount = String::hash( "animation-iteration-count" ),
	AnimationPlayState = String::hash( "animation-play-state" ),
	AnimationTimingFunction = String::hash( "animation-timing-function" ),
	DragResistance = String::hash( "drag-resistance" ),
	ChangePagePercent = String::hash( "change-page-percent" ),
	MaxEdgeResistance = String::hash( "max-edge-resistance" ),
	PageTransitionDuration = String::hash( "page-transition-duration" ),
	TimingFunction = String::hash( "timing-function" ),
	PageLocked = String::hash( "page-locked" ),
	BorderType = String::hash( "border-type" ),
	BorderLeftColor = String::hash( "border-left-color" ),
	BorderRightColor = String::hash( "border-right-color" ),
	BorderTopColor = String::hash( "border-top-color" ),
	BorderBottomColor = String::hash( "border-bottom-color" ),
	BorderLeftWidth = String::hash( "border-left-width" ),
	BorderRightWidth = String::hash( "border-right-width" ),
	BorderTopWidth = String::hash( "border-top-width" ),
	BorderBottomWidth = String::hash( "border-bottom-width" ),
	BorderTopLeftRadius = String::hash( "border-top-left-radius" ),
	BorderTopRightRadius = String::hash( "border-top-right-radius" ),
	BorderBottomLeftRadius = String::hash( "border-bottom-left-radius" ),
	BorderBottomRightRadius = String::hash( "border-bottom-right-radius" ),
	BorderSmooth = String::hash( "border-smooth" ),
	BackgroundSmooth = String::hash( "background-smooth" ),
	ForegroundSmooth = String::hash( "foreground-smooth" ),
	TabBarHideOnSingleTab = String::hash( "tabbar-hide-on-single-tab" ),
	TabBarAllowRearrange = String::hash( "tabbar-allow-rearrange" ),
	TabBarAllowDragAndDrop = String::hash( "tabbar-allow-drag-and-drop-tabs" ),
	TabAllowSwitchTabsInEmptySpaces = String::hash( "tabbar-allow-switch-tabs-in-empty-spaces" ),
	SplitterPartition = String::hash( "splitter-partition" ),
	SplitterAlwaysShow = String::hash( "splitter-always-show" ),
	DroppableHoveringColor = String::hash( "droppable-hovering-color" ),
	TextAsFallback = String::hash( "text-as-fallback" ),
	SelectOnClick = String::hash( "select-on-click" ),
	LineSpacing = String::hash( "line-spacing" ),
	GravityOwner = String::hash( "gravity-owner" ),
	Href = String::hash( "href" ),
	Focusable = String::hash( "focusable" ),
	InnerWidgetOrientation = String::hash( "inner-widget-orientation" ),
	Glyph = String::hash( "glyph" ),
	Name = String::hash( "name" ),
	RowValign = String::hash( "row-valign" ),
	TextOverflow = String::hash( "text-overflow" ),
	CheckMode = String::hash( "check-mode" ),
};

enum class PropertyType : Uint32 {
	Undefined,
	String,
	Bool,
	NumberInt,
	NumberIntFixed,
	NumberFloat,
	NumberFloatFixed,
	NumberLength,
	NumberLengthFixed,
	RadiusLength,
	Color,
	Vector2,
	BackgroundSize,
	ForegroundSize,
	Time
};

enum class PropertyRelativeTarget : Uint32 {
	None,
	ContainingBlockWidth,
	ContainingBlockHeight,
	FontSize,
	ParentFontSize,
	LineHeight,
	LocalBlockWidth,
	LocalBlockHeight,
	BackgroundWidth,
	BackgroundHeight,
	ForegroundWidth,
	ForegroundHeight,
	LocalBlockRadiusWidth,
	LocalBlockRadiusHeight
};

class EE_API PropertyDefinition {
  public:
	static PropertyDefinition* New( const std::string& name, const std::string& defaultValue,
									const bool& inherited = false );

	PropertyDefinition( const std::string& name, const std::string& defaultValue,
						const bool& inherited = false );

	const std::string& getName() const;

	const String::HashType& getId() const;

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

	PropertyDefinition& setIndexed();

	const bool& isIndexed() const;

  protected:
	std::string mName;
	String::HashType mId;
	std::vector<std::string> mAliases;
	std::vector<Uint32> mAliasesHash;
	std::string mDefaultValue;
	bool mInherited;
	bool mIndexed;
	PropertyRelativeTarget mRelativeTarget;
	PropertyType mPropertyType;
};

}}} // namespace EE::UI::CSS

#endif
