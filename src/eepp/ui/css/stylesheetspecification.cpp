#include <cctype>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/propertyspecification.hpp>
#include <eepp/ui/css/stylesheetselectorrule.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI { namespace CSS {

SINGLETON_DECLARE_IMPLEMENTATION( StyleSheetSpecification )

StyleSheetSpecification::StyleSheetSpecification() :
	mPropertySpecification( PropertySpecification::createSingleton() ) {
	registerDefaultShorthandParsers();
	registerDefaultProperties();
	mPropertySpecification->finalizeBuiltins();
	registerDefaultNodeSelectors();
}

StyleSheetSpecification::~StyleSheetSpecification() {
	PropertySpecification::destroySingleton();
}

PropertyDefinition& StyleSheetSpecification::registerProperty( PropertyId id,
															   const std::string& propertyName,
															   const std::string& defaultValue,
															   bool inherited ) {
	return mPropertySpecification->registerProperty( id, propertyName, defaultValue, inherited );
}

PropertyDefinition* StyleSheetSpecification::registerProperty( const std::string& propertyName,
															   const std::string& defaultValue,
															   bool inherited ) {
	return mPropertySpecification->registerProperty( propertyName, defaultValue, inherited );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const PropertyId& id ) const {
	return mPropertySpecification->getProperty( id );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const char* name ) const {
	return mPropertySpecification->getProperty( name );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( std::string_view name ) const {
	return mPropertySpecification->getProperty( name );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const std::string& name ) const {
	return mPropertySpecification->getProperty( name );
}

const PropertyIdSet& StyleSheetSpecification::getInheritableProperties() const {
	return mPropertySpecification->getInheritableProperties();
}

ShorthandDefinition&
StyleSheetSpecification::registerShorthand( ShorthandId id, const std::string& name,
											const std::vector<std::string>& properties,
											const std::string& shorthandFuncName ) {
	return mPropertySpecification->registerShorthand( id, name, properties, shorthandFuncName );
}

ShorthandDefinition*
StyleSheetSpecification::registerShorthand( const std::string& name,
											const std::vector<std::string>& properties,
											const std::string& shorthandFuncName ) {
	return mPropertySpecification->registerShorthand( name, properties, shorthandFuncName );
}

void StyleSheetSpecification::registerShorthandParser( const std::string& name,
													   ShorthandParserFunc shorthandParserFunc ) {
	if ( mShorthandParsers.find( name ) != mShorthandParsers.end() ) {
		Log::error( "Shorthand parser \"%s\" is already registered.", name.c_str() );
		return;
	}

	mShorthandParsers[name] = shorthandParserFunc;
}

ShorthandParserFunc StyleSheetSpecification::getShorthandParser( const std::string& name ) {
	if ( mShorthandParsers.find( name ) == mShorthandParsers.end() ) {
		Log::error( "Shorthand parser \"%s\" not found.", name.c_str() );
		return mShorthandParsers["empty"];
	}
	return mShorthandParsers[name];
}

DrawableImageParser& StyleSheetSpecification::getDrawableImageParser() {
	return mDrawableImageParser;
}

const ShorthandDefinition* StyleSheetSpecification::getShorthand( const ShorthandId& id ) const {
	return mPropertySpecification->getShorthand( id );
}

const ShorthandDefinition* StyleSheetSpecification::getShorthand( const std::string& name ) const {
	return mPropertySpecification->getShorthand( name );
}

bool StyleSheetSpecification::isShorthand( const std::string& name ) const {
	return mPropertySpecification->isShorthand( name );
}

void StyleSheetSpecification::registerDefaultProperties() {
	registerProperty( PropertyId::Id, "id", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::Class, "class", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::X, "x", "" ).setType( PropertyType::NumberLength );
	registerProperty( PropertyId::Y, "y", "" ).setType( PropertyType::NumberLength );
	registerProperty( PropertyId::Width, "width", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::Height, "height", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( PropertyId::BackgroundColor, "background-color", "" )
		.setType( PropertyType::Color )
		.addAlias( "bgcolor" );
	registerProperty( PropertyId::BackgroundImage, "background-image", "none" ).setIndexed();
	registerProperty( PropertyId::BackgroundTint, "background-tint", "" )
		.setIndexed()
		.setType( PropertyType::Color );
	registerProperty( PropertyId::BackgroundPositionX, "background-position-x", "center" )
		.setRelativeTarget( PropertyRelativeTarget::BackgroundWidth )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( PropertyId::BackgroundPositionY, "background-position-y", "center" )
		.setRelativeTarget( PropertyRelativeTarget::BackgroundHeight )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( PropertyId::BackgroundRepeat, "background-repeat", "no-repeat" ).setIndexed();
	registerProperty( PropertyId::BackgroundSize, "background-size", "auto" )
		.setType( PropertyType::BackgroundSize )
		.setIndexed();
	registerProperty( PropertyId::BackgroundOrigin, "background-origin", "padding-box" )
		.setIndexed();
	registerProperty( PropertyId::BackgroundClip, "background-clip", "border-box" ).setIndexed();
	registerProperty( PropertyId::BackgroundAttachment, "background-attachment", "scroll" )
		.setIndexed();
	registerProperty( PropertyId::ForegroundColor, "foreground-color", "" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::ForegroundImage, "foreground-image", "none" ).setIndexed();
	registerProperty( PropertyId::ForegroundTint, "foreground-tint", "" )
		.setIndexed()
		.setType( PropertyType::Color );
	registerProperty( PropertyId::ForegroundPositionX, "foreground-position-x", "center" )
		.setRelativeTarget( PropertyRelativeTarget::ForegroundWidth )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( PropertyId::ForegroundPositionY, "foreground-position-y", "center" )
		.setRelativeTarget( PropertyRelativeTarget::ForegroundHeight )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( PropertyId::ForegroundRepeat, "foreground-repeat", "no-repeat" ).setIndexed();
	registerProperty( PropertyId::ForegroundSize, "foreground-size", "auto" )
		.setType( PropertyType::ForegroundSize )
		.setIndexed();
	registerProperty( PropertyId::Visible, "visible", "true" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::Visibility, "visibility", "visible" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::Enabled, "enabled", "true" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::Theme, "theme", "" );
	registerProperty( PropertyId::Skin, "skin", "" );
	registerProperty( PropertyId::SkinColor, "skin-color", "" ).setType( PropertyType::Color );
	registerProperty( PropertyId::Gravity, "gravity", "" );
	registerProperty( PropertyId::Flags, "flags", "" );
	registerProperty( PropertyId::MarginTop, "margin-top", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "margin_top" )
		.addAlias( "layout-margin-top" )
		.addAlias( "layout_margintop" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( PropertyId::MarginLeft, "margin-left", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "margin_left" )
		.addAlias( "layout-margin-left" )
		.addAlias( "layout_marginleft" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::MarginRight, "margin-right", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "margin_right" )
		.addAlias( "layout-margin-right" )
		.addAlias( "layout_marginright" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::MarginBottom, "margin-bottom", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "margin_bottom" )
		.addAlias( "layout-margin-bottom" )
		.addAlias( "layout_marginbottom" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( PropertyId::Tooltip, "tooltip", "" )
		.setType( PropertyType::String )
		.addAlias( "alt" );
	registerProperty( PropertyId::LayoutWeight, "layout-weight", "" )
		.addAlias( "layout_weight" )
		.addAlias( "lw8" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::LayoutGravity, "layout-gravity", "" )
		.addAlias( "layout_gravity" )
		.addAlias( "lg" );
	registerProperty( PropertyId::LayoutWidth, "layout-width", "" )
		.addAlias( "layout_width" )
		.addAlias( "lw" );
	registerProperty( PropertyId::LayoutHeight, "layout-height", "" )
		.addAlias( "layout_height" )
		.addAlias( "lh" );
	registerProperty( PropertyId::LayoutToLeftOf, "layout-to-left-of", "" )
		.addAlias( "layout_to_left_of" );
	registerProperty( PropertyId::LayoutToRightOf, "layout-to-right-of", "" )
		.addAlias( "layout_to_right_of" );
	registerProperty( PropertyId::LayoutToTopOf, "layout-to-top-of", "" )
		.addAlias( "layout_to_top_of" );
	registerProperty( PropertyId::LayoutToBottomOf, "layout-to-bottom-of", "" )
		.addAlias( "layout_to_bottom_of" );
	registerProperty( PropertyId::Clip, "clip", "" ).setType( PropertyType::String );
	// TODO: layer implement overflow-x and overflow-y properly
	registerProperty( PropertyId::Overflow, "overflow", "visible" )
		.addAlias( "overflow-x" )
		.addAlias( "overflow-y" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::Rotation, "rotation", "" )
		.addAlias( "rotate" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::Scale, "scale", "" ).setType( PropertyType::Vector2 );
	registerProperty( PropertyId::RotationOriginPointX, "rotation-origin-point-x", "50%" )
		.addAlias( "rotate-origin-point-x" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::RotationOriginPointY, "rotation-origin-point-y", "50%" )
		.addAlias( "rotate-origin-point-y" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockHeight )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::ScaleOriginPointX, "scale-origin-point-x", "50%" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::ScaleOriginPointY, "scale-origin-point-y", "50%" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockHeight )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::BlendMode, "blend-mode", "" );
	registerProperty( PropertyId::PaddingLeft, "padding-left", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::PaddingRight, "padding-right", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::PaddingTop, "padding-top", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( PropertyId::PaddingBottom, "padding-bottom", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( PropertyId::Opacity, "opacity", "" ).setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::Cursor, "cursor", "arrow", true );
	registerProperty( PropertyId::Text, "text", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::TextTransform, "text-transform", "", true )
		.setType( PropertyType::String );
	registerProperty( PropertyId::Color, "color", "", true )
		.setType( PropertyType::Color )
		.addAlias( "text-color" )
		.addAlias( "textcolor" );
	registerProperty( PropertyId::TextShadowColor, "text-shadow-color", "", true )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::TextShadowOffset, "text-shadow-offset", "", true )
		.setType( PropertyType::Vector2 );
	registerProperty( PropertyId::SelectionColor, "selection-color", "" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::SelectionBackColor, "selection-back-color", "" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::FontFamily, "font-family", "", true )
		.addAlias( "font-name" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::FontSize, "font-size", "", true )
		.setType( PropertyType::NumberLength )
		.addAlias( "text-size" )
		.addAlias( "textsize" );
	registerProperty( PropertyId::FontStyle, "font-style", "", true );
	registerProperty( PropertyId::FontWeight, "font-weight", "", true );
	registerProperty( PropertyId::TextDecoration, "text-decoration", "", true );
	registerProperty( PropertyId::LineSpacing, "line-spacing", "", true )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::LineHeight, "line-height", "", true )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::TextIndent, "text-indent", "", true )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::TabSize, "tab-size", "8", true ).setType( PropertyType::String );
	registerProperty( PropertyId::TextStrokeWidth, "text-stroke-width", "", true )
		.setType( PropertyType::NumberLength )
		.addAlias( "fontoutlinethickness" );
	registerProperty( PropertyId::TextStrokeColor, "text-stroke-color", "", true )
		.setType( PropertyType::Color )
		.addAlias( "fontoutlinecolor" );
	registerProperty( PropertyId::TextSelection, "text-selection", "", true )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::TextAlign, "text-align", "", true ).addAlias( "align" );
	registerProperty( PropertyId::Icon, "icon", "" );
	registerProperty( PropertyId::MinIconSize, "min-icon-size", "" )
		.setType( PropertyType::Vector2 );
	registerProperty( PropertyId::Src, "src", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::ScaleType, "scale-type", "" );
	registerProperty( PropertyId::Tint, "tint", "" ).setType( PropertyType::Color );
	registerProperty( PropertyId::MaxTextLength, "max-text-length", "" )
		.setType( PropertyType::NumberInt );
	registerProperty( PropertyId::MinTabWidth, "min-tab-width", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::MaxTabWidth, "max-tab-width", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::TabClosable, "tab-closable", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::TabCloseButtonVisible, "tab-close-button-visible", "" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::TabsEdgesDiffSkin, "tabs-edges-diff-skin", "" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::TabSeparation, "tab-separation", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::TabHeight, "tab-height", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::Selected, "selected", "" )
		.setType( PropertyType::Bool )
		.addAlias( "active" )
		.addAlias( "checked" );
	registerProperty( PropertyId::PopUpToRoot, "popup-to-root", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::MaxVisibleItems, "max-visible-items", "" )
		.setType( PropertyType::NumberIntFixed );
	registerProperty( PropertyId::SelectedIndex, "selected-index", "" );
	registerProperty( PropertyId::SelectedText, "selected-text", "" );
	registerProperty( PropertyId::ScrollBarStyle, "scrollbar-style", "" );
	registerProperty( PropertyId::RowHeight, "row-height", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::Editable, "editable", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::SelectionType, "selection-type", "row" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::SelectionKind, "selection-kind", "single" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::IconSize, "icon-size", "" ).setType( PropertyType::NumberLength );
	registerProperty( PropertyId::SortIconSize, "sort-icon-size", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::MainColumn, "main-column", "0" )
		.setType( PropertyType::NumberInt );
	registerProperty( PropertyId::RowHeaderWidth, "row-header-width", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::TableFlags, "table-flags", "" )
		.addAlias( "tableflags" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::TableModel, "table-model", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::IndentWidth, "indent-width", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::ExpanderIconSize, "expander-icon-size", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::VScrollMode, "vscroll-mode", "" );
	registerProperty( PropertyId::HScrollMode, "hscroll-mode", "" );

	registerProperty( PropertyId::ColumnMargin, "column-margin", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::RowMargin, "row-margin", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::ColumnMode, "column-mode", "" );
	registerProperty( PropertyId::RowMode, "row-mode", "" );
	registerProperty( PropertyId::ColumnWeight, "column-weight", "" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::ColumnWidth, "column-width", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::RowWeight, "row-weight", "" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::ReverseDraw, "reverse-draw", "" ).setType( PropertyType::Bool );

	registerProperty( PropertyId::Orientation, "orientation", "" );
	registerProperty( PropertyId::Indeterminate, "indeterminate", "" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::MaxProgress, "max-progress", "" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::Progress, "progress", "" ).setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::FillColor, "fill-color", "" ).setType( PropertyType::Color );
	registerProperty( PropertyId::Radius, "radius", "" ).setType( PropertyType::NumberLength );
	registerProperty( PropertyId::OutlineThickness, "outline-thickness", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::AnimationSpeed, "animation-speed", "" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::ArcStartAngle, "arc-start-angle", "" )
		.setType( PropertyType::NumberFloat );

	registerProperty( PropertyId::MinWidth, "min-width", "" )
		.setType( PropertyType::NumberLengthFixed )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::MinHeight, "min-height", "" )
		.setType( PropertyType::NumberLengthFixed )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( PropertyId::MaxWidth, "max-width", "" )
		.setType( PropertyType::NumberLengthFixed )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::MaxHeight, "max-height", "" )
		.setType( PropertyType::NumberLengthFixed )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );

	registerProperty( PropertyId::TotalSteps, "total-steps", "" )
		.setType( PropertyType::NumberInt );
	registerProperty( PropertyId::VerticalExpand, "vertical-expand", "" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::DisplayPercent, "display-percent", "" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::MovementSpeed, "movement-speed", "" )
		.setType( PropertyType::Vector2 );
	registerProperty( PropertyId::MinValue, "min-value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::MaxValue, "max-value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::Value, "value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::ClickStep, "click-step", "" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::PageStep, "page-step", "" ).setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::BackgroundExpand, "background-expand", "" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::ScrollBarMode, "scrollbar-mode", "" );
	registerProperty( PropertyId::HalfSlider, "half-slider", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::Owns, "owns", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::AllowEditing, "allow-editing", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::Locked, "locked", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::MaxLength, "max-length", "" ).setType( PropertyType::NumberInt );
	registerProperty( PropertyId::Numeric, "numeric", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::AllowFloat, "allow-float", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::TouchDrag, "touch-drag", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::TouchDragDeceleration, "touch-drag-deceleration", "" )
		.setType( PropertyType::NumberFloat );

	registerProperty( PropertyId::WindowTitle, "window-title", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::WindowOpacity, "window-opacity", "" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::WindowButtonsOffset, "window-buttons-offset", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::WindowFlags, "window-flags", "" )
		.addAlias( "winflags" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::WindowTitlebarSize, "window-titlebar-size", "" )
		.setType( PropertyType::Vector2 );
	registerProperty( PropertyId::WindowBorderSize, "window-border-size", "" )
		.setType( PropertyType::Vector2 );
	registerProperty( PropertyId::WindowMinSize, "window-min-size", "" )
		.setType( PropertyType::Vector2 );
	registerProperty( PropertyId::WindowButtonsSeparation, "window-buttons-separation", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::WindowCornerDistance, "window-corner-distance", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::WindowTitlebarAutoSize, "window-decoration-auto-size", "" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::WindowBorderAutoSize, "window-border-auto-size", "" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::WindowShadowColor, "window-shadow-color", "" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::WindowShadowOffset, "window-shadow-offset", "" )
		.setType( PropertyType::Vector2 );
	registerProperty( PropertyId::WindowShadowSize, "window-shadow-size", "" )
		.setType( PropertyType::NumberLength );

	registerProperty( PropertyId::Wordwrap, "word-wrap", "" ).setType( PropertyType::Bool );

	registerProperty( PropertyId::WhiteSpace, "white-space", "normal", true )
		.setType( PropertyType::String );
	registerProperty( PropertyId::WhiteSpaceCollapse, "white-space-collapse", "collapse", true )
		.setType( PropertyType::String );

	registerProperty( PropertyId::Hint, "hint", "" )
		.setType( PropertyType::String )
		.addAlias( "placeholder" );
	registerProperty( PropertyId::HintColor, "hint-color", "" ).setType( PropertyType::Color );
	registerProperty( PropertyId::HintShadowColor, "hint-shadow-color", "" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::HintShadowOffset, "hint-shadow-offset", "" )
		.setType( PropertyType::Vector2 );
	registerProperty( PropertyId::HintFontSize, "hint-font-size", "" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::HintFontStyle, "hint-font-style", "" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::HintStrokeWidth, "hint-stroke-width", "" )
		.setType( PropertyType::NumberLength )
		.addAlias( "hintoutlinethickness" );
	registerProperty( PropertyId::HintStrokeColor, "hint-stroke-color", "" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::HintFontFamily, "hint-font-family", "" )
		.addAlias( "hint-font-name" );
	registerProperty( PropertyId::HintDisplay, "hint-display", "" ).setType( PropertyType::String );

	registerProperty( PropertyId::Transition, "transition", "" ).setIndexed();
	registerProperty( PropertyId::TransitionDuration, "transition-duration", "" );
	registerProperty( PropertyId::TransitionDelay, "transition-delay", "0s" )
		.setType( PropertyType::Time );
	registerProperty( PropertyId::TransitionTimingFunction, "transition-timing-function",
					  "linear" );
	registerProperty( PropertyId::TransitionProperty, "transition-property", "" );

	registerProperty( PropertyId::Animation, "animation", "" ).setIndexed();
	registerProperty( PropertyId::AnimationDelay, "animation-delay", "0s" )
		.setType( PropertyType::Time )
		.setIndexed();
	registerProperty( PropertyId::AnimationDirection, "animation-direction", "normal" )
		.setType( PropertyType::String )
		.setIndexed();
	registerProperty( PropertyId::AnimationDuration, "animation-duration", "0s" )
		.setType( PropertyType::Time )
		.setIndexed();
	registerProperty( PropertyId::AnimationFillMode, "animation-fill-mode", "none" )
		.setType( PropertyType::String )
		.setIndexed();
	registerProperty( PropertyId::AnimationIterationCount, "animation-iteration-count", "1" )
		.setType( PropertyType::NumberFloat )
		.setIndexed();
	registerProperty( PropertyId::AnimationName, "animation-name", "none" )
		.setType( PropertyType::String )
		.setIndexed();
	registerProperty( PropertyId::AnimationPlayState, "animation-play-state", "running" )
		.setType( PropertyType::String )
		.setIndexed();
	registerProperty( PropertyId::AnimationTimingFunction, "animation-timing-function", "linear" )
		.setType( PropertyType::String )
		.setIndexed();

	registerProperty( PropertyId::DragResistance, "drag-resistance", "8dp" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::ChangePagePercent, "change-page-percent", "0.33" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::MaxEdgeResistance, "max-edge-resistance", "0" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::TimingFunction, "timing-function", "linear" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::PageLocked, "page-locked", "" ).setType( PropertyType::Bool );

	registerProperty( PropertyId::BorderType, "border-type", "inside" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::BorderLeftColor, "border-left-color", "transparent" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::BorderRightColor, "border-right-color", "transparent" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::BorderTopColor, "border-top-color", "transparent" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::BorderBottomColor, "border-bottom-color", "transparent" )
		.setType( PropertyType::Color );
	registerProperty( PropertyId::BorderLeftWidth, "border-left-width", "0" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockRadiusWidth );
	registerProperty( PropertyId::BorderRightWidth, "border-right-width", "0" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockRadiusWidth );
	registerProperty( PropertyId::BorderTopWidth, "border-top-width", "0" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockRadiusWidth );
	registerProperty( PropertyId::BorderBottomWidth, "border-bottom-width", "0" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockRadiusWidth );
	registerProperty( PropertyId::BorderLeftStyle, "border-left-style", "none" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::BorderRightStyle, "border-right-style", "none" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::BorderTopStyle, "border-top-style", "none" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::BorderBottomStyle, "border-bottom-style", "none" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::BorderTopLeftRadius, "border-top-left-radius", "0" )
		.setType( PropertyType::RadiusLength );
	registerProperty( PropertyId::BorderTopRightRadius, "border-top-right-radius", "0" )
		.setType( PropertyType::RadiusLength );
	registerProperty( PropertyId::BorderBottomLeftRadius, "border-bottom-left-radius", "0" )
		.setType( PropertyType::RadiusLength );
	registerProperty( PropertyId::BorderBottomRightRadius, "border-bottom-right-radius", "0" )
		.setType( PropertyType::RadiusLength );

	registerProperty( PropertyId::BorderSmooth, "border-smooth", "false" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::BackgroundSmooth, "background-smooth", "false" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::ForegroundSmooth, "foreground-smooth", "false" )
		.setType( PropertyType::Bool );

	registerProperty( PropertyId::ForegroundTopLeftRadius, "foreground-top-left-radius", "0" )
		.setType( PropertyType::RadiusLength );
	registerProperty( PropertyId::ForegroundTopRightRadius, "foreground-top-right-radius", "0" )
		.setType( PropertyType::RadiusLength );
	registerProperty( PropertyId::ForegroundBottomLeftRadius, "foreground-bottom-left-radius", "0" )
		.setType( PropertyType::RadiusLength );
	registerProperty( PropertyId::ForegroundBottomRightRadius, "foreground-bottom-right-radius",
					  "0" )
		.setType( PropertyType::RadiusLength );

	registerProperty( PropertyId::TabBarHideOnSingleTab, "tabbar-hide-on-single-tab", "false" );
	registerProperty( PropertyId::TabBarAllowRearrange, "tabbar-allow-rearrange", "false" );
	registerProperty( PropertyId::TabBarAllowDragAndDrop, "tabbar-allow-drag-and-drop-tabs",
					  "false" );
	registerProperty( PropertyId::TabAllowSwitchTabsInEmptySpaces,
					  "tabbar-allow-switch-tabs-in-empty-spaces", "false" );

	registerProperty( PropertyId::SplitterPartition, "splitter-partition", "50%" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth );
	registerProperty( PropertyId::SplitterAlwaysShow, "splitter-always-show", "true" )
		.setType( PropertyType::Bool );

	registerProperty( PropertyId::DroppableHoveringColor, "droppable-hovering-color", "#FFFFFF20" )
		.setType( PropertyType::Color );

	registerProperty( PropertyId::TextAsFallback, "text-as-fallback", "false" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::SelectOnClick, "select-on-click", "false" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::GravityOwner, "gravity-owner", "false" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::Href, "href", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::Focusable, "focusable", "true" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::ExpandText, "expand-text", "false" )
		.setType( PropertyType::Bool );
	registerProperty( PropertyId::ColSpan, "colspan", "1" ).setType( PropertyType::NumberInt );
	registerProperty( PropertyId::TableLayout, "table-layout", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::CellPadding, "cellpadding", "0" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::CellSpacing, "cellspacing", "0" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::Size, "size", "20" ).setType( PropertyType::NumberInt );
	registerProperty( PropertyId::Type, "type", "text" ).setType( PropertyType::String );
	registerProperty( PropertyId::Rows, "rows", "2" ).setType( PropertyType::NumberInt );
	registerProperty( PropertyId::Cols, "cols", "20" ).setType( PropertyType::NumberInt );
	registerProperty( PropertyId::InputMode, "input-mode", "normal" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::Hidden, "hidden", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::Open, "open", "" ).setType( PropertyType::Bool );
	registerProperty( PropertyId::Display, "display", "inline" ).setType( PropertyType::String );
	registerProperty( PropertyId::Position, "position", "static" ).setType( PropertyType::String );
	registerProperty( PropertyId::Float, "float", "none" ).setType( PropertyType::String );
	registerProperty( PropertyId::Clear, "clear", "none" ).setType( PropertyType::String );
	registerProperty( PropertyId::ListStyleType, "list-style-type", "none", true )
		.setType( PropertyType::String );
	registerProperty( PropertyId::ListStylePosition, "list-style-position", "outside", true )
		.setType( PropertyType::String );
	registerProperty( PropertyId::ListStyleImage, "list-style-image", "none" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::BoxSizing, "box-sizing", "content-box" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::Top, "top", "auto" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( PropertyId::Right, "right", "auto" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::Bottom, "bottom", "auto" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( PropertyId::Left, "left", "auto" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( PropertyId::ZIndex, "z-index", "auto" ).setType( PropertyType::NumberInt );

	registerProperty( PropertyId::InnerWidgetOrientation, "inner-widget-orientation",
					  "widgeticontextbox" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::Glyph, "glyph", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::Name, "name", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::For, "for", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::RowValign, "row-valign", "" )
		.addAlias( "row-vertical-align" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::TextOverflow, "text-overflow", "clip" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::CheckMode, "check-mode", "element" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::EnableCodeEditorFlags, "enable-editor-flags", "" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::DisableCodeEditorFlags, "disable-editor-flags", "" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::LineWrapMode, "line-wrap-mode", "nowrap" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::LineWrapType, "line-wrap-type", "viewport" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::DisplayOptions, "display-options", "" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::MenuWidthMode, "menu-width-mode", "" )
		.setType( PropertyType::String );

	registerProperty( PropertyId::Action, "action", "" ).setType( PropertyType::String );
	registerProperty( PropertyId::Method, "method", "GET" ).setType( PropertyType::String );
	registerProperty( PropertyId::Enctype, "enctype", "application/x-www-form-urlencoded" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::Target, "target", "_self" ).setType( PropertyType::String );
	registerProperty( PropertyId::UnicodeRange, "unicode-range", "" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::AlignmentBaseline, "alignment-baseline", "baseline" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::FlexDirection, "flex-direction", "row" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::FlexWrap, "flex-wrap", "nowrap" ).setType( PropertyType::String );
	registerProperty( PropertyId::JustifyContent, "justify-content", "flex-start" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::AlignItems, "align-items", "stretch" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::AlignContent, "align-content", "stretch" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::AlignSelf, "align-self", "auto" ).setType( PropertyType::String );
	registerProperty( PropertyId::FlexGrow, "flex-grow", "0" ).setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::FlexShrink, "flex-shrink", "1" )
		.setType( PropertyType::NumberFloat );
	registerProperty( PropertyId::FlexBasis, "flex-basis", "auto" )
		.setType( PropertyType::NumberLength );
	registerProperty( PropertyId::Order, "order", "0" ).setType( PropertyType::NumberInt );
	registerProperty( PropertyId::RowGap, "row-gap", "0px" ).setType( PropertyType::NumberLength );
	registerProperty( PropertyId::ColumnGap, "column-gap", "0px" )
		.setType( PropertyType::NumberLength );

	registerProperty( PropertyId::GridTemplateRows, "grid-template-rows", "none" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridTemplateColumns, "grid-template-columns", "none" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridTemplateAreas, "grid-template-areas", "none" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridAutoRows, "grid-auto-rows", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridAutoColumns, "grid-auto-columns", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridAutoFlow, "grid-auto-flow", "row" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridRowStart, "grid-row-start", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridRowEnd, "grid-row-end", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridColumnStart, "grid-column-start", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridColumnEnd, "grid-column-end", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridRow, "grid-row", "auto" ).setType( PropertyType::String );
	registerProperty( PropertyId::GridColumn, "grid-column", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::GridArea, "grid-area", "auto" ).setType( PropertyType::String );
	registerProperty( PropertyId::JustifyItems, "justify-items", "normal" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::JustifySelf, "justify-self", "auto" )
		.setType( PropertyType::String );
	registerProperty( PropertyId::Defer, "defer", "0" ).setType( PropertyType::Bool );

	// Shorthands
	registerShorthand( ShorthandId::Margin, "margin",
					   { "margin-top", "margin-right", "margin-bottom", "margin-left" }, "box" );
	registerShorthand( ShorthandId::LayoutMargin, "layout-margin",
					   { "margin-top", "margin-right", "margin-bottom", "margin-left" }, "box" );
	registerShorthand( ShorthandId::LayoutMarginUnderscore, "layout_margin",
					   { "margin-top", "margin-right", "margin-bottom", "margin-left" }, "box" );
	registerShorthand( ShorthandId::Padding, "padding",
					   { "padding-top", "padding-right", "padding-bottom", "padding-left" },
					   "box" );
	registerShorthand( ShorthandId::Background, "background",
					   { "background-color", "background-image", "background-position",
						 "background-size", "background-repeat", "background-attachment",
						 "background-origin", "background-clip" },
					   "background" );
	registerShorthand(
		ShorthandId::Foreground, "foreground",
		{ "foreground-color", "foreground-image", "foreground-repeat", "foreground-position" },
		"background" );
	registerShorthand( ShorthandId::BoxMargin, "box-margin", { "column-margin", "row-margin" },
					   "single-value-vector" );
	registerShorthand( ShorthandId::BackgroundPosition, "background-position",
					   { "background-position-x", "background-position-y" },
					   "background-position" );
	registerShorthand( ShorthandId::ForegroundPosition, "foreground-position",
					   { "foreground-position-x", "foreground-position-y" },
					   "background-position" );
	registerShorthand(
		ShorthandId::BorderColor, "border-color",
		{ "border-top-color", "border-right-color", "border-bottom-color", "border-left-color" },
		"border-box" );
	registerShorthand(
		ShorthandId::BorderWidth, "border-width",
		{ "border-top-width", "border-right-width", "border-bottom-width", "border-left-width" },
		"border-box" );
	registerShorthand(
		ShorthandId::BorderStyle, "border-style",
		{ "border-top-style", "border-right-style", "border-bottom-style", "border-left-style" },
		"border-box" );
	registerShorthand( ShorthandId::BorderRadius, "border-radius",
					   { "border-top-left-radius", "border-top-right-radius",
						 "border-bottom-right-radius", "border-bottom-left-radius" },
					   "radius" );
	registerShorthand( ShorthandId::ForegroundRadius, "foreground-radius",
					   { "foreground-top-left-radius", "foreground-top-right-radius",
						 "foreground-bottom-right-radius", "foreground-bottom-left-radius" },
					   "radius" );
	registerShorthand( ShorthandId::RotationOriginPoint, "rotation-origin-point",
					   { "rotation-origin-point-x", "rotation-origin-point-y" }, "vector2" );
	registerShorthand( ShorthandId::RotateOriginPoint, "rotate-origin-point",
					   { "rotation-origin-point-x", "rotation-origin-point-y" }, "vector2" );
	registerShorthand( ShorthandId::ScaleOriginPoint, "scale-origin-point",
					   { "scale-origin-point-x", "scale-origin-point-y" }, "vector2" );
	registerShorthand( ShorthandId::MinSize, "min-size", { "min-width", "min-height" }, "vector2" );
	registerShorthand( ShorthandId::MaxSize, "max-size", { "max-width", "max-height" }, "vector2" );
	registerShorthand( ShorthandId::Border, "border",
					   { "border-width", "border-style", "border-color" }, "border" );
	registerShorthand( ShorthandId::TextShadow, "text-shadow",
					   { "text-shadow-color", "text-shadow-offset" }, "color-vector2" );
	registerShorthand( ShorthandId::HintShadow, "hint-shadow",
					   { "hint-shadow-color", "hint-shadow-offset" }, "color-vector2" );
	registerShorthand( ShorthandId::BorderLeft, "border-left",
					   { "border-left-width", "border-left-style", "border-left-color" },
					   "border-side" );
	registerShorthand( ShorthandId::BorderRight, "border-right",
					   { "border-right-width", "border-right-style", "border-right-color" },
					   "border-side" );
	registerShorthand( ShorthandId::BorderTop, "border-top",
					   { "border-top-width", "border-top-style", "border-top-color" },
					   "border-side" );
	registerShorthand( ShorthandId::BorderBottom, "border-bottom",
					   { "border-bottom-width", "border-bottom-style", "border-bottom-color" },
					   "border-side" );
	registerShorthand( ShorthandId::ListStyle, "list-style",
					   { "list-style-type", "list-style-position", "list-style-image" },
					   "list-style" );
	registerShorthand( ShorthandId::Font, "font",
					   { "font-style", "font-weight", "font-size", "line-height", "font-family" },
					   "font" );
	registerShorthand( ShorthandId::VerticalAlign, "vertical-align", { "alignment-baseline" },
					   "vertical-align" );
	registerShorthand( ShorthandId::FlexFlow, "flex-flow", { "flex-direction", "flex-wrap" },
					   "single-value-vector" );
	registerShorthand( ShorthandId::Flex, "flex", { "flex-grow", "flex-shrink", "flex-basis" },
					   "flex" );
	registerShorthand( ShorthandId::Gap, "gap", { "row-gap", "column-gap" }, "vector2" );
	registerShorthand( ShorthandId::GridTemplate, "grid-template",
					   { "grid-template-rows", "grid-template-columns", "grid-template-areas" },
					   "grid-template" );
	registerShorthand( ShorthandId::Grid, "grid",
					   { "grid-template-rows", "grid-template-columns", "grid-template-areas",
						 "grid-auto-rows", "grid-auto-columns", "grid-auto-flow" },
					   "grid" );
	registerShorthand( ShorthandId::PlaceItems, "place-items", { "align-items", "justify-items" },
					   "vector2" );
	registerShorthand( ShorthandId::PlaceSelf, "place-self", { "align-self", "justify-self" },
					   "vector2" );
	registerShorthand( ShorthandId::PlaceContent, "place-content",
					   { "align-content", "justify-content" }, "vector2" );

	validateBuiltinRegistrations();
}

void StyleSheetSpecification::validateBuiltinRegistrations() {
#if EE_DEBUG
	// Ensure every built-in slot is populated and every dense ID round-trips
	// through its canonical name. A newly added enum member or registration that
	// leaves a gap or duplicates a slot would otherwise silently shift custom
	// IDs or leave an unregistered bit.
	int propertyCount = 0;
	for ( Uint16 i = 1; i < static_cast<Uint16>( PropertyId::NumDefinedIds ); ++i ) {
		const PropertyDefinition* def =
			mPropertySpecification->getProperty( static_cast<PropertyId>( i ) );
		if ( nullptr == def ) {
			Log::error( "PropertySpecification: built-in property slot %d is unregistered.", i );
			eeASSERT( false );
			continue;
		}
		propertyCount++;
		if ( def->getPropertyId() != static_cast<PropertyId>( i ) ) {
			Log::error( "PropertySpecification: property \"%s\" reports dense ID %d, expected %d.",
						def->getName().c_str(), static_cast<int>( def->getPropertyId() ), i );
			eeASSERT( false );
		}
		const std::string name = def->getName();
		const PropertyDefinition* byName = mPropertySpecification->getProperty( name );
		if ( byName != def ) {
			Log::error( "PropertySpecification: property \"%s\" does not round-trip to slot %d.",
						name.c_str(), i );
			eeASSERT( false );
		}
	}
	if ( propertyCount != static_cast<int>( PropertyId::NumDefinedIds ) - 1 ) {
		Log::error( "PropertySpecification: registered %d built-in properties, expected %d.",
					propertyCount, static_cast<int>( PropertyId::NumDefinedIds ) - 1 );
		eeASSERT( false );
	}

	int shorthandCount = 0;
	for ( Uint8 i = 1; i < static_cast<Uint8>( ShorthandId::NumDefinedIds ); ++i ) {
		const ShorthandDefinition* def =
			mPropertySpecification->getShorthand( static_cast<ShorthandId>( i ) );
		if ( nullptr == def ) {
			Log::error( "PropertySpecification: built-in shorthand slot %d is unregistered.", i );
			eeASSERT( false );
			continue;
		}
		shorthandCount++;
		if ( def->getShorthandId() != static_cast<ShorthandId>( i ) ) {
			Log::error( "PropertySpecification: shorthand \"%s\" reports dense ID %d, expected %d.",
						def->getName().c_str(), static_cast<int>( def->getShorthandId() ), i );
			eeASSERT( false );
		}
		const ShorthandDefinition* byName = mPropertySpecification->getShorthand( def->getName() );
		if ( byName != def ) {
			Log::error( "PropertySpecification: shorthand \"%s\" does not round-trip to slot %d.",
						def->getName().c_str(), i );
			eeASSERT( false );
		}
	}
	if ( shorthandCount != static_cast<int>( ShorthandId::NumDefinedIds ) - 1 ) {
		Log::error( "PropertySpecification: registered %d built-in shorthands, expected %d.",
					shorthandCount, static_cast<int>( ShorthandId::NumDefinedIds ) - 1 );
		eeASSERT( false );
	}
#endif
}

void StyleSheetSpecification::registerNodeSelector( const std::string& name,
													StyleSheetNodeSelector nodeSelector ) {
	mNodeSelectors[String::toLower( name )] = std::move( nodeSelector );
}

static bool isNth( int a, int b, int count ) {
	int x = count;
	x -= b;
	if ( a != 0 )
		x /= a;
	return ( x >= 0 && x * a + b == count );
}

static bool whereIsMatch( const UIWidget* node, const std::string& param ) {
	StyleSheetSelectorRule rule( param, StyleSheetSelectorRule::PatternMatch::ANY );
	return rule.matches( const_cast<UIWidget*>( node ) );
}

void StyleSheetSpecification::registerDefaultNodeSelectors() {
	mNodeSelectors["empty"] = []( const UIWidget* node, int, int, const FunctionString& ) -> bool {
		return node->getFirstChild() == NULL;
	};
	mNodeSelectors["first-child"] = []( const UIWidget* node, int, int,
										const FunctionString& ) -> bool {
		return NULL != node->getParent() && node->getElementIndex() == 0;
	};
	mNodeSelectors["enabled"] = []( const UIWidget* node, int, int,
									const FunctionString& ) -> bool { return node->isEnabled(); };
	mNodeSelectors["disabled"] = []( const UIWidget* node, int, int,
									 const FunctionString& ) -> bool { return !node->isEnabled(); };
	mNodeSelectors["first-of-type"] = []( const UIWidget* node, int, int,
										  const FunctionString& ) -> bool {
		return NULL != node->getParent() && node->getElementOfTypeIndex() == 0;
	};
	mNodeSelectors["last-child"] = []( const UIWidget* node, int, int,
									   const FunctionString& ) -> bool {
		if ( NULL == node->getParent() || !node->getParent()->isWidget() )
			return false;
		Node* child = node->getParent()->getLastChild();
		while ( NULL != child ) {
			if ( child->isWidget() && !static_cast<UIWidget*>( child )->isTextNode() )
				return child == node;
			child = child->getPrevNode();
		}
		return false;
	};
	mNodeSelectors["last-of-type"] = []( const UIWidget* node, int, int,
										 const FunctionString& ) -> bool {
		if ( NULL == node->getParent() || !node->getParent()->isWidget() )
			return false;
		Uint32 type = node->getType();
		Node* child = node->getParent()->getLastChild();
		while ( NULL != child ) {
			if ( child->getType() == type && child->isWidget() &&
				 !static_cast<UIWidget*>( child )->isTextNode() )
				return child == node;
			child = child->getPrevNode();
		}
		return false;
	};
	mNodeSelectors["only-child"] = []( const UIWidget* node, int, int,
									   const FunctionString& ) -> bool {
		return NULL != node->getParent() && node->getParent()->isWidget() &&
			   static_cast<const UIWidget*>( node->getParent() )->getChildElementCount() == 1;
	};
	mNodeSelectors["only-of-type"] = []( const UIWidget* node, int, int,
										 const FunctionString& ) -> bool {
		return NULL != node->getParent() && node->getParent()->isWidget() &&
			   static_cast<const UIWidget*>( node->getParent() )
					   ->getChildElementOfTypeCount( node->getType() ) == 1;
	};
	mNodeSelectors["nth-child"] = []( const UIWidget* node, int a, int b,
									  const FunctionString& ) -> bool {
		return isNth( a, b, node->getElementIndex() + 1 );
	};
	mNodeSelectors["nth-last-child"] = []( const UIWidget* node, int a, int b,
										   const FunctionString& ) -> bool {
		return node->getParent() != NULL && node->getParent()->isWidget()
				   ? isNth(
						 a, b,
						 static_cast<const UIWidget*>( node->getParent() )->getChildElementCount() -
							 node->getElementIndex() )
				   : false;
	};
	mNodeSelectors["nth-of-type"] = []( const UIWidget* node, int a, int b,
										const FunctionString& ) -> bool {
		return isNth( a, b, node->getElementOfTypeIndex() + 1 );
	};
	mNodeSelectors["nth-last-of-type"] = []( const UIWidget* node, int a, int b,
											 const FunctionString& ) -> bool {
		return node->getParent() != NULL && node->getParent()->isWidget()
				   ? isNth( a, b,
							static_cast<const UIWidget*>( node->getParent() )
									->getChildElementOfTypeCount( node->getType() ) -
								node->getElementOfTypeIndex() )
				   : false;
	};
	mNodeSelectors["checked"] = []( const UIWidget* node, int, int,
									const FunctionString& ) -> bool {
		return 0 != ( node->getFlags() & UI_CHECKED );
	};
	mNodeSelectors["not"] = []( const UIWidget* node, int, int,
								const FunctionString& data ) -> bool {
		if ( !data.isEmpty() && !data.getParameters().empty() && data.getName() == "not" ) {
			for ( const auto& param : data.getParameters() ) {
				if ( !param.empty() ) {
					if ( param[0] == '.' ) {
						if ( node->hasClass( param.substr( 1 ) ) ) {
							return false;
						}
					} else if ( param[0] == '#' ) {
						if ( node->getId() == param.substr( 1 ) ) {
							return false;
						}
					} else if ( param[0] == ':' ) {
						if ( node->hasPseudoClass( param.substr( 1 ) ) ) {
							return false;
						}
					} else {
						if ( node->getElementTag() == String::toLower( param ) ) {
							return false;
						}
					}
				} else {
					return false;
				}
			}
			return true;
		}
		return false;
	};

	mNodeSelectors["where"] = []( const UIWidget* node, int, int,
								  const FunctionString& data ) -> bool {
		if ( data.isEmpty() || data.getParameters().empty() ||
			 ( data.getName() != "where" && data.getName() != "is" ) )
			return false;

		for ( const auto& param : data.getParameters() ) {
			if ( !param.empty() && whereIsMatch( node, param ) )
				return true;
		}
		return false;
	};

	auto whereFn = mNodeSelectors["where"];
	mNodeSelectors["is"] = whereFn;
}

StructuralSelector StyleSheetSpecification::getStructuralSelector( const std::string& name ) {
	size_t index = name.find( '(' );
	if ( index == std::string::npos ) {
		auto it = mNodeSelectors.find( name );
		if ( it == mNodeSelectors.end() )
			return StructuralSelector( nullptr );
		// Selector without any function call "()"
		return StructuralSelector( it->second );
	}
	auto it = mNodeSelectors.find( name.substr( 0, index ) );
	if ( it == mNodeSelectors.end() )
		return StructuralSelector( nullptr );

	// Parse the 'a' and 'b' values.
	int a = 1;
	int b = 0;
	int t = 0;

	size_t parameterStart = name.find( '(' );
	size_t parameterEnd = name.find( ')' );
	if ( parameterStart != std::string::npos && parameterEnd != std::string::npos ) {
		std::string parameters = String::toLower( String::trim(
			name.substr( parameterStart + 1, parameterEnd - ( parameterStart + 1 ) ) ) );

		// Check for 'even' or 'odd' first.
		if ( parameters == "even" ) {
			a = 2;
			b = 0;
		} else if ( parameters == "odd" ) {
			a = 2;
			b = 1;
		} else {
			size_t nIndex = parameters.find( 'n' );
			if ( nIndex == std::string::npos ) {
				// The equation is 0n + b. So a = 0, and we only have to parse b.
				a = 0;
				if ( String::isNumber( parameters ) && String::fromString( t, parameters ) ) {
					b = t;
				} else {
					return StructuralSelector( it->second, 0, 0, FunctionString::parse( name ) );
				}
			} else {
				if ( nIndex == 0 ) {
					a = 1;
				} else {
					std::string aParameter = parameters.substr( 0, nIndex );
					if ( String::trim( aParameter ) == "-" ) {
						a = -1;
					} else {
						if ( String::fromString( t, aParameter ) ) {
							a = t;
						} else {
							return StructuralSelector( nullptr );
						}
					}
				}

				size_t pmIndex = parameters.find( '+', nIndex + 1 );
				if ( pmIndex != std::string::npos ) {
					b = 1;
				} else {
					pmIndex = parameters.find( '-', nIndex + 1 );
					if ( pmIndex != std::string::npos ) {
						b = -1;
					}
				}

				if ( nIndex == parameters.size() - 1 || pmIndex == std::string::npos ) {
					b = 0;
				} else {
					if ( String::fromString( t, parameters.substr( pmIndex + 1 ) ) ) {
						b = b * t;
					} else {
						return StructuralSelector( nullptr );
					}
				}
			}
		}
	}

	return StructuralSelector( it->second, a, b );
}

static int getIndexEndingWith( const std::vector<std::string>& vec, const std::string& endWidth ) {
	for ( size_t i = 0; i < vec.size(); i++ ) {
		if ( String::endsWith( vec[i], endWidth ) ) {
			return i;
		}
	}

	return -1;
}

static bool isKeywordPosition( const std::string& str ) {
	return str == "center" || str == "top" || str == "bottom" || str == "left" || str == "right";
}

void StyleSheetSpecification::registerDefaultShorthandParsers() {
	mShorthandParsers["empty"] = []( const ShorthandDefinition*,
									 std::string ) -> std::vector<StyleSheetProperty> {
		return {};
	};

	mShorthandParsers["box"] = []( const ShorthandDefinition* shorthand,
								   std::string value ) -> std::vector<StyleSheetProperty> {
		String::removeExtraSpaces( value );
		if ( value.empty() )
			return {};

		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );

		if ( propNames.size() != 4 ) {
			Log::error( "ShorthandType::Box properties must be 4 for %s", shorthand->getName() );
			return properties;
		}

		auto ltrbSplit = String::split( value, ' ', true );
		if ( ltrbSplit.empty() )
			return properties;

		// Apply CSS shorthand rules (Top, Right, Bottom, Left)
		std::string top = ltrbSplit[0];
		std::string right = ltrbSplit.size() > 1 ? ltrbSplit[1] : top;
		std::string bottom = ltrbSplit.size() > 2 ? ltrbSplit[2] : top;
		std::string left = ltrbSplit.size() > 3 ? ltrbSplit[3] : right;

		// propNames order is Top, Right, Bottom, Left
		properties.emplace_back( StyleSheetProperty( propNames[0], top ) );
		properties.emplace_back( StyleSheetProperty( propNames[1], right ) );
		properties.emplace_back( StyleSheetProperty( propNames[2], bottom ) );
		properties.emplace_back( StyleSheetProperty( propNames[3], left ) );

		return properties;
	};

	mShorthandParsers["single-value-vector"] =
		[]( const ShorthandDefinition* shorthand,
			std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
		for ( auto& prop : propNames ) {
			properties.emplace_back( StyleSheetProperty( prop, value ) );
		}
		return properties;
	};

	mShorthandParsers["vertical-align"] =
		[]( const ShorthandDefinition* shorthand,
			std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};
		const std::vector<std::string>& propNames = shorthand->getProperties();
		if ( propNames.empty() )
			return {};
		return { StyleSheetProperty( propNames[0], value ) };
	};

	mShorthandParsers["vector2"] = []( const ShorthandDefinition* shorthand,
									   std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
		if ( propNames.size() != 2 ) {
			Log::error( "ShorthandType::Vector2 properties must be 2 for %s",
						shorthand->getName().c_str() );
			return properties;
		}

		auto values = String::split( value, ' ' );

		if ( !values.empty() ) {
			for ( size_t i = 0; i < propNames.size(); i++ ) {
				properties.emplace_back(
					StyleSheetProperty( propNames[i], values[i % values.size()] ) );
			}
		}
		return properties;
	};

	mShorthandParsers["border-box"] = []( const ShorthandDefinition* shorthand,
										  std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		auto ltrbSplit = String::split( value, " ", "", "(\"" );
		if ( !ltrbSplit.empty() ) {
			for ( size_t i = 0; i < propNames.size(); i++ ) {
				properties.emplace_back(
					StyleSheetProperty( propNames[i], ltrbSplit[i % ltrbSplit.size()] ) );
			}
		}
		return properties;
	};

	mShorthandParsers["radius"] = []( const ShorthandDefinition* shorthand,
									  std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		auto splits = String::split( value, '/' );
		auto widths = String::split( splits[0], ' ' );
		std::vector<std::string> heights;
		if ( splits.size() >= 2 ) {
			heights = String::split( splits[1], ' ' );
		}
		if ( !widths.empty() ) {
			for ( size_t i = 0; i < propNames.size(); i++ ) {
				std::string val = widths[i % widths.size()];
				if ( !heights.empty() ) {
					val += " " + heights[i % heights.size()];
				}
				properties.emplace_back( StyleSheetProperty( propNames[i], val ) );
			}
		}
		return properties;
	};

	mShorthandParsers["background-position"] =
		[]( const ShorthandDefinition* shorthand,
			std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};

		bool isImportant = false;
		if ( String::icontains( value, "!important" ) ) {
			std::string lowerVal = String::toLower( value );
			size_t impPos = lowerVal.rfind( "!important" );
			if ( impPos != std::string::npos ) {
				isImportant = true;
				value.erase( impPos );
				value = String::trim( value );
			}
		}

		const std::vector<std::string>& propNames = shorthand->getProperties();
		std::vector<std::string> values = String::split( value, ',' );

		std::vector<std::string> xValues;
		std::vector<std::string> yValues;

		// Helper to identify keywords that can actually take an offset
		auto isDirectionalKeyword = []( const std::string& s ) {
			return s == "left" || s == "right" || s == "top" || s == "bottom";
		};

		// Helper to identify explicit axis direction
		auto isYAxis = []( const std::string& s ) {
			return String::startsWith( s, "top" ) || String::startsWith( s, "bottom" );
		};

		auto isXAxis = []( const std::string& s ) {
			return String::startsWith( s, "left" ) || String::startsWith( s, "right" );
		};

		for ( auto& val : values ) {
			// 1. Tokenize
			std::vector<std::string> rawTokens = String::split( val, ' ' );
			std::vector<std::string> tokens;
			for ( const auto& t : rawTokens ) {
				std::string clean = String::trim( t );
				if ( !clean.empty() )
					tokens.push_back( clean );
			}

			if ( tokens.empty() )
				continue;

			// 2. Group Components correctly
			std::vector<std::string> components;
			size_t idx = 0;
			while ( idx < tokens.size() ) {
				std::string current = tokens[idx];

				// Check if we should merge with next token.
				// Rule: We only merge if 'current' is a directional keyword AND
				// 'next' is NOT a keyword (implies it's a length/percent).
				// Example: "right 10px" -> merge. "3px 3px" -> don't merge.
				if ( isDirectionalKeyword( current ) ) {
					size_t nextIdx = idx + 1;
					if ( nextIdx < tokens.size() && !isKeywordPosition( tokens[nextIdx] ) ) {
						current += " " + tokens[nextIdx];
						idx++; // Skip the next token since we consumed it
					}
				}

				components.push_back( current );
				idx++;
			}

			// 3. Assign Axes
			std::string xAxis = "center";
			std::string yAxis = "center";

			if ( components.size() == 1 ) {
				// Case: "bottom" -> Y=bottom, X=center
				// Case: "10px"   -> X=10px,   Y=center
				if ( isYAxis( components[0] ) )
					yAxis = components[0];
				else
					xAxis = components[0];
			} else if ( components.size() >= 2 ) {
				// Case: "10px 20px" -> X=10px, Y=20px
				// Case: "top right" -> Y=top, X=right (Swap)
				// Case: "right 10px top" -> X=right 10px, Y=top

				std::string c1 = components[0];
				std::string c2 = components[1];

				// By default, first is X, second is Y.
				// We swap ONLY if the first is clearly Vertical OR the second is clearly
				// Horizontal.
				if ( isYAxis( c1 ) || isXAxis( c2 ) ) {
					yAxis = c1;
					xAxis = c2;
				} else {
					xAxis = c1;
					yAxis = c2;
				}
			}

			xValues.push_back( xAxis );
			yValues.push_back( yAxis );
		}

		std::vector<StyleSheetProperty> properties;
		std::string impStr = isImportant ? " !important" : "";

		if ( !propNames.empty() )
			properties.emplace_back( propNames[0], String::join( xValues, ',' ) + impStr );
		if ( propNames.size() > 1 )
			properties.emplace_back( propNames[1], String::join( yValues, ',' ) + impStr );

		return properties;
	};

	mShorthandParsers["background"] =
		[this]( const ShorthandDefinition* shorthand,
				std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() || "none" == value )
			return {};

		// Extract !important early so it doesn't get captured as a color token
		bool isImportant = false;
		if ( String::icontains( value, "!important" ) ) {
			isImportant = true;
			String::replaceAll( value, "!important", "" );
			String::replaceAll( value, "! important", "" );
		}

		// Ensure functional notations (like url) are separated by a space
		// so that standard token splitting works correctly for minified CSS.
		String::replaceAll( value, ")", ") " );
		String::removeExtraSpaces( value );
		value = String::trim( value );

		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();

		auto isRepeatKeyword = []( const std::string& s ) {
			return -1 != String::valueIndex( s, "repeat;repeat-x;repeat-y;no-repeat;space;round" );
		};

		auto isBoxKeyword = []( const std::string& s ) {
			return s == "border-box" || s == "padding-box" || s == "content-box";
		};

		auto isAttachmentKeyword = []( const std::string& s ) {
			return s == "scroll" || s == "fixed" || s == "local";
		};

		auto isPositionKeyword = []( const std::string& s ) {
			return s == "left" || s == "right" || s == "top" || s == "bottom" || s == "center";
		};

		// Split by comma for multi-layer support, while strictly ignoring
		// commas inside parentheses (required for data: URIs and functions)
		std::vector<std::string> layers = String::split( value, ",", "", "(" );

		std::vector<std::string> imageValues;
		std::vector<std::string> repeatValues;
		std::vector<std::string> attachmentValues;
		std::vector<std::string> originValues;
		std::vector<std::string> clipValues;
		std::vector<std::string> positionValues;
		std::vector<std::string> sizeValues;
		std::string colorValue;

		for ( size_t layerIdx = 0; layerIdx < layers.size(); ++layerIdx ) {
			std::string layerVal = String::trim( layers[layerIdx] );

			// Whitespace around the background position/size slash is optional. Tokenize it
			// without splitting slashes inside functional notation such as url() or var().
			std::vector<std::string> tokens;
			std::string token;
			int parenthesisDepth = 0;
			for ( const char ch : layerVal ) {
				if ( ch == '(' )
					++parenthesisDepth;
				else if ( ch == ')' && parenthesisDepth > 0 )
					--parenthesisDepth;

				if ( parenthesisDepth == 0 &&
					 ( std::isspace( static_cast<unsigned char>( ch ) ) || ch == '/' ) ) {
					if ( !token.empty() ) {
						tokens.emplace_back( std::move( token ) );
						token.clear();
					}
					if ( ch == '/' )
						tokens.emplace_back( "/" );
				} else {
					token += ch;
				}
			}
			if ( !token.empty() )
				tokens.emplace_back( std::move( token ) );
			std::string positionStr;
			std::string sizeStr;
			bool hasSlash{ false };
			std::string firstBox;
			std::string secondBox;

			for ( size_t ti = 0; ti < tokens.size(); ++ti ) {
				auto& tok = tokens[ti];
				if ( tok.empty() )
					continue; // Safeguard empty tokens

				auto open = tok.find_first_of( '(' );

				if ( open != std::string::npos &&
					 mDrawableImageParser.exists( tok.substr( 0, open ) ) ) {
					imageValues.push_back( tok );
				} else if ( isRepeatKeyword( tok ) ) {
					repeatValues.push_back( tok );
				} else if ( isAttachmentKeyword( tok ) ) {
					attachmentValues.push_back( tok );
				} else if ( isBoxKeyword( tok ) ) {
					if ( firstBox.empty() )
						firstBox = tok;
					else
						secondBox = tok;
				} else if ( tok == "/" ) {
					hasSlash = true;
				} else if ( hasSlash && String::startsWith( tok, "var(" ) && !sizeStr.empty() ) {
					// eepp resolves custom properties after expanding shorthands. Saved browser
					// pages commonly put an image var after an explicit size, for example
					// `center / 100% var(--image)`.
					imageValues.push_back( tok );
				} else if ( hasSlash && !tok.empty() && tok != "/" ) {
					sizeStr += tok + " ";
				} else if ( isPositionKeyword( tok ) || String::isNumber( tok[0] ) ||
							tok[0] == '-' || tok[0] == '.' || tok[0] == '+' ) {
					positionStr += tok + " ";
				} else {
					if ( colorValue.empty() )
						colorValue = tok;
				}
			}

			originValues.push_back( firstBox.empty() ? "padding-box" : firstBox );
			clipValues.push_back( secondBox.empty() ? "border-box" : secondBox );

			if ( !positionStr.empty() ) {
				String::trimInPlace( positionStr );
				positionValues.push_back( positionStr );
			} else {
				positionValues.push_back( "0% 0%" );
			}

			if ( !sizeStr.empty() ) {
				String::trimInPlace( sizeStr );
				sizeValues.push_back( sizeStr );
			} else {
				sizeValues.push_back( "auto" );
			}
		}

		// Re-apply the !important flag to the mapped individual longhands
		std::string impStr = isImportant ? " !important" : "";

		for ( auto& propName : propNames ) {
			if ( String::endsWith( propName, "-color" ) && !colorValue.empty() ) {
				properties.emplace_back( StyleSheetProperty( propName, colorValue + impStr ) );
			} else if ( String::endsWith( propName, "-image" ) && !imageValues.empty() ) {
				properties.emplace_back(
					StyleSheetProperty( propName, String::join( imageValues, ',' ) + impStr ) );
			} else if ( String::endsWith( propName, "-repeat" ) && !repeatValues.empty() ) {
				properties.emplace_back(
					StyleSheetProperty( propName, String::join( repeatValues, ',' ) + impStr ) );
			} else if ( String::endsWith( propName, "-attachment" ) && !attachmentValues.empty() ) {
				properties.emplace_back( StyleSheetProperty(
					propName, String::join( attachmentValues, ',' ) + impStr ) );
			} else if ( String::endsWith( propName, "-origin" ) ) {
				properties.emplace_back(
					StyleSheetProperty( propName, String::join( originValues, ',' ) + impStr ) );
			} else if ( String::endsWith( propName, "-clip" ) ) {
				properties.emplace_back(
					StyleSheetProperty( propName, String::join( clipValues, ',' ) + impStr ) );
			} else if ( String::endsWith( propName, "-position" ) ) {
				// Let background-position sub-parser handle this
				const ShorthandDefinition* posShorthand = getShorthand( propName );
				if ( NULL != posShorthand ) {
					auto bpVec = mShorthandParsers["background-position"](
						posShorthand, String::join( positionValues, ',' ) + impStr );
					for ( auto& bp : bpVec )
						properties.emplace_back( bp );
				}
			} else if ( String::endsWith( propName, "-size" ) ) {
				properties.emplace_back(
					StyleSheetProperty( propName, String::join( sizeValues, ',' ) + impStr ) );
			}
		}

		return properties;
	};

	mShorthandParsers["border"] = [this]( const ShorthandDefinition* shorthand,
										  std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};

		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		std::vector<std::string> tokens = String::split( value, " ", "", "(" );

		for ( auto& tok : tokens ) {
			if ( -1 !=
				 String::valueIndex(
					 tok, "none;hidden;dotted;dashed;solid;double;groove;ridge;inset;outset" ) ) {

				int pos = getIndexEndingWith( propNames, "-style" );
				if ( pos != -1 ) {
					const ShorthandDefinition* styleShorthand = getShorthand( propNames[pos] );
					if ( styleShorthand ) {
						auto styleVec = mShorthandParsers["border-box"]( styleShorthand, tok );
						for ( auto& styleProp : styleVec )
							properties.emplace_back( styleProp );
					}
				}
				if ( tok == "none" || tok == "hidden" ) {
					int widthPos = getIndexEndingWith( propNames, "-width" );
					if ( widthPos != -1 ) {
						const ShorthandDefinition* widthShorthand =
							getShorthand( propNames[widthPos] );
						if ( widthShorthand ) {
							auto widthVec = mShorthandParsers["border-box"]( widthShorthand, "0" );
							for ( auto& widthProp : widthVec )
								properties.emplace_back( widthProp );
						}
					}
				}

			} else if ( Color::isColorString( tok ) || String::startsWith( tok, "var(" ) ) {
				int pos = getIndexEndingWith( propNames, "-color" );
				if ( pos != -1 ) {
					const ShorthandDefinition* shorthand = getShorthand( propNames[pos] );
					if ( NULL != shorthand ) {
						auto bbVec = mShorthandParsers["border-box"]( shorthand, tok );
						for ( auto& bb : bbVec )
							properties.emplace_back( bb );
					}
				}
			} else {
				int pos = getIndexEndingWith( propNames, "-width" );
				if ( pos != -1 ) {
					const ShorthandDefinition* shorthand = getShorthand( propNames[pos] );
					if ( NULL != shorthand ) {
						auto bbVec = mShorthandParsers["border-box"]( shorthand, tok );
						for ( auto& bb : bbVec )
							properties.emplace_back( bb );
					}
				}
			}
		}

		return properties;
	};

	mShorthandParsers["border-side"] = []( const ShorthandDefinition* shorthand,
										   std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};

		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		std::vector<std::string> tokens = String::split( value, " ", "", "(" );

		for ( auto& tok : tokens ) {
			if ( -1 !=
				 String::valueIndex(
					 tok, "none;hidden;dotted;dashed;solid;double;groove;ridge;inset;outset" ) ) {

				int pos = getIndexEndingWith( propNames, "-style" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], tok ) );
				if ( tok == "none" || tok == "hidden" ) {
					int widthPos = getIndexEndingWith( propNames, "-width" );
					if ( widthPos != -1 )
						properties.emplace_back( StyleSheetProperty( propNames[widthPos], "0" ) );
				}
			} else if ( Color::isColorString( tok ) || String::startsWith( tok, "var(" ) ) {
				int pos = getIndexEndingWith( propNames, "-color" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], tok ) );
			} else {
				int pos = getIndexEndingWith( propNames, "-width" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], tok ) );
			}
		}

		return properties;
	};

	mShorthandParsers["color-vector2"] =
		[]( const ShorthandDefinition* shorthand,
			std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() || "none" == value )
			return {};

		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		std::vector<std::string> tokens = String::split( value, " ", "", "(" );
		std::vector<std::string> vec;

		for ( auto& tok : tokens ) {
			String::trimInPlace( tok );
			String::toLowerInPlace( tok );

			if ( Color::isColorString( tok ) ) {
				int pos = getIndexEndingWith( propNames, "-color" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], tok ) );
			} else {
				int pos = getIndexEndingWith( propNames, "-offset" );
				if ( pos != -1 )
					vec.emplace_back( tok );
			}
		}

		if ( !vec.empty() ) {
			int pos = getIndexEndingWith( propNames, "-offset" );
			if ( pos != -1 )
				properties.emplace_back(
					StyleSheetProperty( propNames[pos], String::join( vec, ' ' ) ) );
		}

		return properties;
	};

	mShorthandParsers["list-style"] = []( const ShorthandDefinition* shorthand,
										  std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		if ( propNames.empty() )
			return {};
		auto tokens = String::split( value, " ", "", "(" );
		int typePos = getIndexEndingWith( propNames, "-type" );
		int posPos = getIndexEndingWith( propNames, "-position" );
		int imagePos = getIndexEndingWith( propNames, "-image" );
		for ( auto& tok : tokens ) {
			String::trimInPlace( tok );
			if ( tok == "inside" || tok == "outside" ) {
				if ( posPos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[posPos], tok ) );
			} else if ( String::startsWith( tok, "url(" ) ) {
				if ( imagePos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[imagePos], tok ) );
			} else if ( tok == "none" ) {
				if ( typePos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[typePos], tok ) );
				if ( imagePos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[imagePos], tok ) );
			} else {
				if ( typePos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[typePos], tok ) );
			}
		}
		return properties;
	};

	mShorthandParsers["font"] = []( const ShorthandDefinition* shorthand,
									std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};

		std::string lowerVal = String::toLower( value );
		static const std::string systemFonts[] = { "caption",	  "icon",		   "menu",
												   "message-box", "small-caption", "status-bar" };
		for ( const auto& sysFont : systemFonts ) {
			if ( lowerVal == sysFont ) {
				std::vector<StyleSheetProperty> properties;
				const std::vector<std::string>& propNames = shorthand->getProperties();
				int familyPos = getIndexEndingWith( propNames, "-family" );
				int stylePos = getIndexEndingWith( propNames, "-style" );
				if ( familyPos != -1 && Graphics::SystemFontResolver::isEnabled() ) {
					Graphics::FontDesc desc =
						Graphics::SystemFontResolver::instance()->resolveGeneric(
							Graphics::GenericFamily::SystemUi, Graphics::FontWeight::Normal,
							false );
					if ( !desc.family.empty() ) {
						properties.emplace_back(
							StyleSheetProperty( propNames[familyPos], desc.family ) );
						if ( stylePos != -1 )
							properties.emplace_back( StyleSheetProperty(
								propNames[stylePos], desc.italic ? "italic" : "normal" ) );
					}
				}
				return properties;
			}
		}

		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();

		int stylePos = getIndexEndingWith( propNames, "-style" );
		int sizePos = getIndexEndingWith( propNames, "-size" );
		int linePos = getIndexEndingWith( propNames, "-height" );
		int familyPos = getIndexEndingWith( propNames, "-family" );
		int weightPos = getIndexEndingWith( propNames, "-weight" );

		static const std::string sizeKeywords[] = {
			"xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large", "xxx-large" };

		auto isSizeKeyword = []( const std::string& t ) {
			std::string lt = String::toLower( t );
			for ( const auto& kw : sizeKeywords ) {
				if ( lt == kw )
					return true;
			}
			return false;
		};

		auto isStyleWord = []( const std::string& t ) {
			std::string lt = String::toLower( t );
			return lt == "italic" || lt == "oblique" || lt == "normal";
		};

		auto isWeightWord = []( const std::string& t ) {
			std::string lt = String::toLower( t );
			return lt == "bold" || lt == "bolder" || lt == "lighter" || lt == "100" ||
				   lt == "200" || lt == "300" || lt == "400" || lt == "500" || lt == "600" ||
				   lt == "700" || lt == "800" || lt == "900";
		};

		auto isNumberOrLength = []( const std::string& t ) {
			if ( t.empty() )
				return false;
			return ( t[0] >= '0' && t[0] <= '9' ) || t[0] == '.' || t[0] == '-';
		};

		std::vector<std::string> tokens = String::split( value, " ", "", "(", "\"" );
		std::string styleStr;
		std::string sizeStr;
		std::string lineStr;
		std::string familyStr;
		std::string weightStr;
		bool inLineHeight = false;

		for ( size_t i = 0; i < tokens.size(); i++ ) {
			std::string tok = tokens[i];
			String::trimInPlace( tok );
			if ( tok.empty() )
				continue;

			if ( tok == "/" ) {
				inLineHeight = true;
				continue;
			}

			if ( !inLineHeight ) {
				size_t slashPos = tok.find( '/' );
				if ( slashPos != std::string::npos ) {
					if ( slashPos == 0 ) {
						lineStr = tok.substr( 1 );
						String::trimInPlace( lineStr );
						continue;
					}
					sizeStr = tok.substr( 0, slashPos );
					lineStr = tok.substr( slashPos + 1 );
					String::trimInPlace( lineStr );
					continue;
				}
			}

			if ( inLineHeight ) {
				lineStr += ( lineStr.empty() ? "" : " " ) + tok;
				inLineHeight = false;
				continue;
			}

			if ( !sizeStr.empty() && familyStr.empty() && !isStyleWord( tok ) &&
				 !isWeightWord( tok ) ) {
				familyStr += ( familyStr.empty() ? "" : " " ) + tok;
				continue;
			}

			if ( isStyleWord( tok ) ) {
				std::string lt = String::toLower( tok );
				if ( lt != "normal" ) {
					if ( !styleStr.empty() )
						styleStr += "|";
					styleStr += lt;
				}
				continue;
			}

			if ( isWeightWord( tok ) ) {
				std::string lt = String::toLower( tok );
				if ( lt != "normal" ) {
					if ( lt == "bolder" || lt == "lighter" )
						weightStr = "bold";
					else
						weightStr = lt;
				}
				continue;
			}

			if ( sizeStr.empty() && ( isNumberOrLength( tok ) || isSizeKeyword( tok ) ) ) {
				sizeStr = tok;
				continue;
			}

			familyStr += ( familyStr.empty() ? "" : " " ) + tok;
		}

		if ( !sizeStr.empty() ) {
			if ( stylePos != -1 && !styleStr.empty() )
				properties.emplace_back( StyleSheetProperty( propNames[stylePos], styleStr ) );
			if ( weightPos != -1 && !weightStr.empty() )
				properties.emplace_back( StyleSheetProperty( propNames[weightPos], weightStr ) );
			if ( sizePos != -1 )
				properties.emplace_back( StyleSheetProperty( propNames[sizePos], sizeStr ) );
			if ( linePos != -1 )
				properties.emplace_back( StyleSheetProperty(
					propNames[linePos], lineStr.empty() ? "normal" : lineStr ) );
			if ( familyPos != -1 && !familyStr.empty() ) {
				String::trimInPlace( familyStr );
				if ( familyStr.size() >= 2 &&
					 ( ( familyStr[0] == '"' && familyStr.back() == '"' ) ||
					   ( familyStr[0] == '\'' && familyStr.back() == '\'' ) ) ) {
					familyStr = familyStr.substr( 1, familyStr.size() - 2 );
				}
				properties.emplace_back( StyleSheetProperty( propNames[familyPos], familyStr ) );
			}
		}

		return properties;
	};

	mShorthandParsers["flex-flow"] = []( const ShorthandDefinition* shorthand,
										 std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};

		const std::vector<std::string>& propNames = shorthand->getProperties();
		if ( propNames.size() != 2 )
			return {};

		std::vector<std::string> tokens = String::split( value, ' ' );
		if ( tokens.empty() )
			return {};

		std::vector<StyleSheetProperty> properties;
		properties.emplace_back( StyleSheetProperty( propNames[0], tokens[0] ) );
		if ( tokens.size() > 1 )
			properties.emplace_back( StyleSheetProperty( propNames[1], tokens[1] ) );
		return properties;
	};

	mShorthandParsers["flex"] = []( const ShorthandDefinition* shorthand,
									std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() )
			return {};

		const std::vector<std::string>& propNames = shorthand->getProperties();
		if ( propNames.size() != 3 )
			return {};

		std::string lowerVal = String::toLower( value );
		String::removeExtraSpaces( lowerVal );

		std::vector<StyleSheetProperty> properties;

		if ( lowerVal == "auto" ) {
			properties.emplace_back( StyleSheetProperty( propNames[0], "1" ) );
			properties.emplace_back( StyleSheetProperty( propNames[1], "1" ) );
			properties.emplace_back( StyleSheetProperty( propNames[2], "auto" ) );
			return properties;
		}

		if ( lowerVal == "none" ) {
			properties.emplace_back( StyleSheetProperty( propNames[0], "0" ) );
			properties.emplace_back( StyleSheetProperty( propNames[1], "0" ) );
			properties.emplace_back( StyleSheetProperty( propNames[2], "auto" ) );
			return properties;
		}

		std::vector<std::string> tokens = String::split( lowerVal, ' ' );
		if ( tokens.empty() )
			return {};

		auto isNumber = []( const std::string& s ) -> bool {
			if ( s.empty() )
				return false;
			char c = s[0];
			return ( c >= '0' && c <= '9' ) || c == '.' || c == '-' || c == '+';
		};

		auto isLength = []( const std::string& s ) -> bool {
			if ( s.empty() )
				return false;
			if ( s == "auto" || s == "content" )
				return true;
			char c = s[0];
			return ( c >= '0' && c <= '9' ) || c == '.' || c == '-' || c == '+';
		};

		if ( tokens.size() == 1 ) {
			// flex: <number>  =>  <number> 1 0%
			if ( isNumber( tokens[0] ) ) {
				properties.emplace_back( StyleSheetProperty( propNames[0], tokens[0] ) );
				properties.emplace_back( StyleSheetProperty( propNames[1], "1" ) );
				properties.emplace_back( StyleSheetProperty( propNames[2], "0%" ) );
			}
		} else if ( tokens.size() == 2 ) {
			if ( isNumber( tokens[0] ) && isNumber( tokens[1] ) ) {
				// flex: <grow> <shrink>  =>  <grow> <shrink> 0%
				properties.emplace_back( StyleSheetProperty( propNames[0], tokens[0] ) );
				properties.emplace_back( StyleSheetProperty( propNames[1], tokens[1] ) );
				properties.emplace_back( StyleSheetProperty( propNames[2], "0%" ) );
			} else if ( isNumber( tokens[0] ) && isLength( tokens[1] ) ) {
				// flex: <grow> <basis>  =>  <grow> 1 <basis>
				properties.emplace_back( StyleSheetProperty( propNames[0], tokens[0] ) );
				properties.emplace_back( StyleSheetProperty( propNames[1], "1" ) );
				properties.emplace_back( StyleSheetProperty( propNames[2], tokens[1] ) );
			}
		} else if ( tokens.size() >= 3 ) {
			// flex: <grow> <shrink> <basis>
			if ( isNumber( tokens[0] ) && isNumber( tokens[1] ) && isLength( tokens[2] ) ) {
				properties.emplace_back( StyleSheetProperty( propNames[0], tokens[0] ) );
				properties.emplace_back( StyleSheetProperty( propNames[1], tokens[1] ) );
				properties.emplace_back( StyleSheetProperty( propNames[2], tokens[2] ) );
			}
		}

		return properties;
	};

	mShorthandParsers["grid-template"] =
		[]( const ShorthandDefinition* shorthand,
			std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		const auto& props = shorthand->getProperties();
		if ( value == "none" )
			return { StyleSheetProperty( props[0], "none" ), StyleSheetProperty( props[1], "none" ),
					 StyleSheetProperty( props[2], "none" ) };

		// Split on '/' for rows vs columns
		size_t slashPos = value.find( '/' );
		std::string rowsAndAreas = String::trim( value.substr( 0, slashPos ) );
		std::string cols = ( slashPos != std::string::npos )
							   ? String::trim( value.substr( slashPos + 1 ) )
							   : "none";
		return { StyleSheetProperty( props[0], rowsAndAreas ), StyleSheetProperty( props[1], cols ),
				 StyleSheetProperty( props[2], value ) };
	};

	mShorthandParsers["grid"] = []( const ShorthandDefinition* shorthand,
									std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value == "none" )
			return { StyleSheetProperty( "grid-template-rows", "none" ) };
		return { StyleSheetProperty( "grid-template-rows", value ) };
	};
}

}}} // namespace EE::UI::CSS
