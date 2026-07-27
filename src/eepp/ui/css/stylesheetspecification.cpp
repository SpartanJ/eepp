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
	registerDefaultNodeSelectors();
}

StyleSheetSpecification::~StyleSheetSpecification() {
	PropertySpecification::destroySingleton();
}

PropertyDefinition& StyleSheetSpecification::registerProperty( const std::string& propertyVame,
															   const std::string& defaultValue,
															   bool inherited ) {
	return mPropertySpecification->registerProperty( propertyVame, defaultValue, inherited );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const PropertyId& id ) const {
	return mPropertySpecification->getProperty( id );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const Uint32& id ) const {
	return mPropertySpecification->getProperty( id );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const std::string& name ) const {
	return mPropertySpecification->getProperty( name );
}

const SmallVector<PropertyId>& StyleSheetSpecification::getInheritableProperties() const {
	return mPropertySpecification->getInheritableProperties();
}

ShorthandDefinition&
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

const ShorthandDefinition* StyleSheetSpecification::getShorthand( const Uint32& id ) const {
	return mPropertySpecification->getShorthand( id );
}

const ShorthandDefinition* StyleSheetSpecification::getShorthand( const std::string& name ) const {
	return mPropertySpecification->getShorthand( name );
}

bool StyleSheetSpecification::isShorthand( const Uint32& id ) const {
	return mPropertySpecification->isShorthand( id );
}

bool StyleSheetSpecification::isShorthand( const std::string& name ) const {
	return mPropertySpecification->isShorthand( name );
}

void StyleSheetSpecification::registerDefaultProperties() {
	registerProperty( "id", "" ).setType( PropertyType::String );
	registerProperty( "class", "" ).setType( PropertyType::String );
	registerProperty( "x", "" ).setType( PropertyType::NumberLength );
	registerProperty( "y", "" ).setType( PropertyType::NumberLength );
	registerProperty( "width", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "height", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "background-color", "" ).setType( PropertyType::Color ).addAlias( "bgcolor" );
	registerProperty( "background-image", "none" ).setIndexed();
	registerProperty( "background-tint", "" ).setIndexed().setType( PropertyType::Color );
	registerProperty( "background-position-x", "center" )
		.setRelativeTarget( PropertyRelativeTarget::BackgroundWidth )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( "background-position-y", "center" )
		.setRelativeTarget( PropertyRelativeTarget::BackgroundHeight )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( "background-repeat", "no-repeat" ).setIndexed();
	registerProperty( "background-size", "auto" )
		.setType( PropertyType::BackgroundSize )
		.setIndexed();
	registerProperty( "background-origin", "padding-box" ).setIndexed();
	registerProperty( "background-clip", "border-box" ).setIndexed();
	registerProperty( "background-attachment", "scroll" ).setIndexed();
	registerProperty( "foreground-color", "" ).setType( PropertyType::Color );
	registerProperty( "foreground-image", "none" ).setIndexed();
	registerProperty( "foreground-tint", "" ).setIndexed().setType( PropertyType::Color );
	registerProperty( "foreground-position-x", "center" )
		.setRelativeTarget( PropertyRelativeTarget::ForegroundWidth )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( "foreground-position-y", "center" )
		.setRelativeTarget( PropertyRelativeTarget::ForegroundHeight )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( "foreground-repeat", "no-repeat" ).setIndexed();
	registerProperty( "foreground-size", "auto" )
		.setType( PropertyType::ForegroundSize )
		.setIndexed();
	registerProperty( "visible", "true" ).setType( PropertyType::Bool );
	registerProperty( "visibility", "visible" ).setType( PropertyType::String );
	registerProperty( "enabled", "true" ).setType( PropertyType::Bool );
	registerProperty( "theme", "" );
	registerProperty( "skin", "" );
	registerProperty( "skin-color", "" ).setType( PropertyType::Color );
	registerProperty( "gravity", "" );
	registerProperty( "flags", "" );
	registerProperty( "margin-top", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "margin_top" )
		.addAlias( "layout-margin-top" )
		.addAlias( "layout_margintop" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "margin-left", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "margin_left" )
		.addAlias( "layout-margin-left" )
		.addAlias( "layout_marginleft" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-right", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "margin_right" )
		.addAlias( "layout-margin-right" )
		.addAlias( "layout_marginright" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-bottom", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "margin_bottom" )
		.addAlias( "layout-margin-bottom" )
		.addAlias( "layout_marginbottom" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "tooltip", "" ).setType( PropertyType::String ).addAlias( "alt" );
	registerProperty( "layout-weight", "" )
		.addAlias( "layout_weight" )
		.addAlias( "lw8" )
		.setType( PropertyType::NumberFloat );
	registerProperty( "layout-gravity", "" ).addAlias( "layout_gravity" ).addAlias( "lg" );
	registerProperty( "layout-width", "" ).addAlias( "layout_width" ).addAlias( "lw" );
	registerProperty( "layout-height", "" ).addAlias( "layout_height" ).addAlias( "lh" );
	registerProperty( "layout-to-left-of", "" ).addAlias( "layout_to_left_of" );
	registerProperty( "layout-to-right-of", "" ).addAlias( "layout_to_right_of" );
	registerProperty( "layout-to-top-of", "" ).addAlias( "layout_to_top_of" );
	registerProperty( "layout-to-bottom-of", "" ).addAlias( "layout_to_bottom_of" );
	registerProperty( "clip", "" ).setType( PropertyType::String );
	// TODO: layer implement overflow-x and overflow-y properly
	registerProperty( "overflow", "visible" )
		.addAlias( "overflow-x" )
		.addAlias( "overflow-y" )
		.setType( PropertyType::String );
	registerProperty( "rotation", "" ).addAlias( "rotate" ).setType( PropertyType::NumberFloat );
	registerProperty( "scale", "" ).setType( PropertyType::Vector2 );
	registerProperty( "rotation-origin-point-x", "50%" )
		.addAlias( "rotate-origin-point-x" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( "rotation-origin-point-y", "50%" )
		.addAlias( "rotate-origin-point-y" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockHeight )
		.setType( PropertyType::NumberLength );
	registerProperty( "scale-origin-point-x", "50%" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( "scale-origin-point-y", "50%" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockHeight )
		.setType( PropertyType::NumberLength );
	registerProperty( "blend-mode", "" );
	registerProperty( "padding-left", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "padding-right", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "padding-top", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "padding-bottom", "" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "opacity", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "cursor", "arrow" );
	registerProperty( "text", "" ).setType( PropertyType::String );
	registerProperty( "text-transform", "", true ).setType( PropertyType::String );
	registerProperty( "color", "", true )
		.setType( PropertyType::Color )
		.addAlias( "text-color" )
		.addAlias( "textcolor" );
	registerProperty( "text-shadow-color", "", true ).setType( PropertyType::Color );
	registerProperty( "text-shadow-offset", "", true ).setType( PropertyType::Vector2 );
	registerProperty( "selection-color", "" ).setType( PropertyType::Color );
	registerProperty( "selection-back-color", "" ).setType( PropertyType::Color );
	registerProperty( "font-family", "", true )
		.addAlias( "font-name" )
		.setType( PropertyType::String );
	registerProperty( "font-size", "", true )
		.setType( PropertyType::NumberLength )
		.addAlias( "text-size" )
		.addAlias( "textsize" );
	registerProperty( "font-style", "", true );
	registerProperty( "font-weight", "", true );
	registerProperty( "text-decoration", "", true );
	registerProperty( "line-spacing", "", true ).setType( PropertyType::NumberLength );
	registerProperty( "line-height", "", true ).setType( PropertyType::NumberLength );
	registerProperty( "text-indent", "", true ).setType( PropertyType::NumberLength );
	registerProperty( "tab-size", "8", true ).setType( PropertyType::String );
	registerProperty( "text-stroke-width", "", true )
		.setType( PropertyType::NumberLength )
		.addAlias( "fontoutlinethickness" );
	registerProperty( "text-stroke-color", "", true )
		.setType( PropertyType::Color )
		.addAlias( "fontoutlinecolor" );
	registerProperty( "text-selection", "", true ).setType( PropertyType::Bool );
	registerProperty( "text-align", "", true ).addAlias( "align" );
	registerProperty( "icon", "" );
	registerProperty( "min-icon-size", "" ).setType( PropertyType::Vector2 );
	registerProperty( "src", "" ).setType( PropertyType::String );
	registerProperty( "scale-type", "" );
	registerProperty( "tint", "" ).setType( PropertyType::Color );
	registerProperty( "max-text-length", "" ).setType( PropertyType::NumberInt );
	registerProperty( "min-tab-width", "" ).setType( PropertyType::NumberLength );
	registerProperty( "max-tab-width", "" ).setType( PropertyType::NumberLength );
	registerProperty( "tab-closable", "" ).setType( PropertyType::Bool );
	registerProperty( "tab-close-button-visible", "" ).setType( PropertyType::Bool );
	registerProperty( "tabs-edges-diff-skin", "" ).setType( PropertyType::Bool );
	registerProperty( "tab-separation", "" ).setType( PropertyType::NumberLength );
	registerProperty( "tab-height", "" ).setType( PropertyType::NumberLength );
	registerProperty( "selected", "" )
		.setType( PropertyType::Bool )
		.addAlias( "active" )
		.addAlias( "checked" );
	registerProperty( "popup-to-root", "" ).setType( PropertyType::Bool );
	registerProperty( "max-visible-items", "" ).setType( PropertyType::NumberIntFixed );
	registerProperty( "selected-index", "" );
	registerProperty( "selected-text", "" );
	registerProperty( "scrollbar-style", "" );
	registerProperty( "row-height", "" ).setType( PropertyType::NumberLength );
	registerProperty( "vscroll-mode", "" );
	registerProperty( "hscroll-mode", "" );

	registerProperty( "column-margin", "" ).setType( PropertyType::NumberLength );
	registerProperty( "row-margin", "" ).setType( PropertyType::NumberLength );
	registerProperty( "column-mode", "" );
	registerProperty( "row-mode", "" );
	registerProperty( "column-weight", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "column-width", "" ).setType( PropertyType::NumberLength );
	registerProperty( "row-weight", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "reverse-draw", "" ).setType( PropertyType::Bool );

	registerProperty( "orientation", "" );
	registerProperty( "indeterminate", "" ).setType( PropertyType::Bool );
	registerProperty( "max-progress", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "progress", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "fill-color", "" ).setType( PropertyType::Color );
	registerProperty( "radius", "" ).setType( PropertyType::NumberLength );
	registerProperty( "outline-thickness", "" ).setType( PropertyType::NumberLength );
	registerProperty( "animation-speed", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "arc-start-angle", "" ).setType( PropertyType::NumberFloat );

	registerProperty( "min-width", "" )
		.setType( PropertyType::NumberLengthFixed )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "min-height", "" )
		.setType( PropertyType::NumberLengthFixed )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "max-width", "" )
		.setType( PropertyType::NumberLengthFixed )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "max-height", "" )
		.setType( PropertyType::NumberLengthFixed )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );

	registerProperty( "total-steps", "" ).setType( PropertyType::NumberInt );
	registerProperty( "vertical-expand", "" ).setType( PropertyType::Bool );
	registerProperty( "display-percent", "" ).setType( PropertyType::Bool );
	registerProperty( "movement-speed", "" ).setType( PropertyType::Vector2 );
	registerProperty( "min-value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "max-value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "click-step", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "page-step", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "background-expand", "" ).setType( PropertyType::Bool );
	registerProperty( "scrollbar-mode", "" );
	registerProperty( "half-slider", "" ).setType( PropertyType::Bool );
	registerProperty( "owns", "" ).setType( PropertyType::String );
	registerProperty( "allow-editing", "" ).setType( PropertyType::Bool );
	registerProperty( "locked", "" ).setType( PropertyType::Bool );
	registerProperty( "max-length", "" ).setType( PropertyType::NumberInt );
	registerProperty( "numeric", "" ).setType( PropertyType::Bool );
	registerProperty( "allow-float", "" ).setType( PropertyType::Bool );
	registerProperty( "touch-drag", "" ).setType( PropertyType::Bool );
	registerProperty( "touch-drag-deceleration", "" ).setType( PropertyType::NumberFloat );

	registerProperty( "window-title", "" ).setType( PropertyType::String );
	registerProperty( "window-opacity", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "window-buttons-offset", "" ).setType( PropertyType::NumberLength );
	registerProperty( "window-flags", "" ).addAlias( "winflags" ).setType( PropertyType::String );
	registerProperty( "window-titlebar-size", "" ).setType( PropertyType::Vector2 );
	registerProperty( "window-border-size", "" ).setType( PropertyType::Vector2 );
	registerProperty( "window-min-size", "" ).setType( PropertyType::Vector2 );
	registerProperty( "window-buttons-separation", "" ).setType( PropertyType::NumberLength );
	registerProperty( "window-corner-distance", "" ).setType( PropertyType::NumberLength );
	registerProperty( "window-decoration-auto-size", "" ).setType( PropertyType::Bool );
	registerProperty( "window-border-auto-size", "" ).setType( PropertyType::Bool );
	registerProperty( "window-shadow-color", "" ).setType( PropertyType::Color );
	registerProperty( "window-shadow-offset", "" ).setType( PropertyType::Vector2 );
	registerProperty( "window-shadow-size", "" ).setType( PropertyType::NumberLength );

	registerProperty( "word-wrap", "" ).setType( PropertyType::Bool );

	registerProperty( "white-space", "normal", true ).setType( PropertyType::String );
	registerProperty( "white-space-collapse", "collapse", true ).setType( PropertyType::String );

	registerProperty( "hint", "" ).setType( PropertyType::String ).addAlias( "placeholder" );
	registerProperty( "hint-color", "" ).setType( PropertyType::Color );
	registerProperty( "hint-shadow-color", "" ).setType( PropertyType::Color );
	registerProperty( "hint-shadow-offset", "" ).setType( PropertyType::Vector2 );
	registerProperty( "hint-font-size", "" ).setType( PropertyType::NumberLength );
	registerProperty( "hint-font-style", "" ).setType( PropertyType::String );
	registerProperty( "hint-stroke-width", "" )
		.setType( PropertyType::NumberLength )
		.addAlias( "hintoutlinethickness" );
	registerProperty( "hint-stroke-color", "" ).setType( PropertyType::Color );
	registerProperty( "hint-font-family", "" ).addAlias( "hint-font-name" );
	registerProperty( "hint-display", "" ).setType( PropertyType::String );

	registerProperty( "transition", "" ).setIndexed();
	registerProperty( "transition-duration", "" );
	registerProperty( "transition-delay", "0s" ).setType( PropertyType::Time );
	registerProperty( "transition-timing-function", "linear" );
	registerProperty( "transition-property", "" );

	registerProperty( "animation", "" ).setIndexed();
	registerProperty( "animation-delay", "0s" ).setType( PropertyType::Time ).setIndexed();
	registerProperty( "animation-direction", "normal" )
		.setType( PropertyType::String )
		.setIndexed();
	registerProperty( "animation-duration", "0s" ).setType( PropertyType::Time ).setIndexed();
	registerProperty( "animation-fill-mode", "none" ).setType( PropertyType::String ).setIndexed();
	registerProperty( "animation-iteration-count", "1" )
		.setType( PropertyType::NumberFloat )
		.setIndexed();
	registerProperty( "animation-name", "none" ).setType( PropertyType::String ).setIndexed();
	registerProperty( "animation-play-state", "running" )
		.setType( PropertyType::String )
		.setIndexed();
	registerProperty( "animation-timing-function", "linear" )
		.setType( PropertyType::String )
		.setIndexed();

	registerProperty( "drag-resistance", "8dp" ).setType( PropertyType::NumberLength );
	registerProperty( "change-page-percent", "0.33" ).setType( PropertyType::NumberFloat );
	registerProperty( "max-edge-resistance", "0" ).setType( PropertyType::NumberFloat );
	registerProperty( "timing-function", "linear" ).setType( PropertyType::String );

	registerProperty( "page-locked", "" ).setType( PropertyType::Bool );

	registerProperty( "border-type", "inside" ).setType( PropertyType::String );
	registerProperty( "border-left-color", "transparent" ).setType( PropertyType::Color );
	registerProperty( "border-right-color", "transparent" ).setType( PropertyType::Color );
	registerProperty( "border-top-color", "transparent" ).setType( PropertyType::Color );
	registerProperty( "border-bottom-color", "transparent" ).setType( PropertyType::Color );
	registerProperty( "border-left-width", "0" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockRadiusWidth );
	registerProperty( "border-right-width", "0" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockRadiusWidth );
	registerProperty( "border-top-width", "0" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockRadiusWidth );
	registerProperty( "border-bottom-width", "0" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockRadiusWidth );

	registerProperty( "border-top-left-radius", "0" ).setType( PropertyType::RadiusLength );
	registerProperty( "border-top-right-radius", "0" ).setType( PropertyType::RadiusLength );
	registerProperty( "border-bottom-left-radius", "0" ).setType( PropertyType::RadiusLength );
	registerProperty( "border-bottom-right-radius", "0" ).setType( PropertyType::RadiusLength );

	registerProperty( "border-smooth", "false" ).setType( PropertyType::Bool );
	registerProperty( "background-smooth", "false" ).setType( PropertyType::Bool );
	registerProperty( "foreground-smooth", "false" ).setType( PropertyType::Bool );

	registerProperty( "foreground-top-left-radius", "0" ).setType( PropertyType::RadiusLength );
	registerProperty( "foreground-top-right-radius", "0" ).setType( PropertyType::RadiusLength );
	registerProperty( "foreground-bottom-left-radius", "0" ).setType( PropertyType::RadiusLength );
	registerProperty( "foreground-bottom-right-radius", "0" ).setType( PropertyType::RadiusLength );

	registerProperty( "tabbar-hide-on-single-tab", "false" );
	registerProperty( "tabbar-allow-rearrange", "false" );
	registerProperty( "tabbar-allow-drag-and-drop-tabs", "false" );
	registerProperty( "tabbar-allow-switch-tabs-in-empty-spaces", "false" );

	registerProperty( "splitter-partition", "50%" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth );
	registerProperty( "splitter-always-show", "true" ).setType( PropertyType::Bool );

	registerProperty( "droppable-hovering-color", "#FFFFFF20" ).setType( PropertyType::Color );

	registerProperty( "text-as-fallback", "false" ).setType( PropertyType::Bool );
	registerProperty( "select-on-click", "false" ).setType( PropertyType::Bool );
	registerProperty( "gravity-owner", "false" ).setType( PropertyType::Bool );
	registerProperty( "href", "" ).setType( PropertyType::String );
	registerProperty( "focusable", "true" ).setType( PropertyType::Bool );
	registerProperty( "expand-text", "false" ).setType( PropertyType::Bool );
	registerProperty( "colspan", "1" ).setType( PropertyType::NumberInt );
	registerProperty( "table-layout", "auto" ).setType( PropertyType::String );
	registerProperty( "cellpadding", "0" ).setType( PropertyType::NumberLength );
	registerProperty( "cellspacing", "0" ).setType( PropertyType::NumberLength );
	registerProperty( "size", "20" ).setType( PropertyType::NumberInt );
	registerProperty( "type", "text" ).setType( PropertyType::String );
	registerProperty( "rows", "2" ).setType( PropertyType::NumberInt );
	registerProperty( "cols", "20" ).setType( PropertyType::NumberInt );
	registerProperty( "input-mode", "normal" ).setType( PropertyType::String );

	registerProperty( "hidden", "" ).setType( PropertyType::Bool );
	registerProperty( "open", "" ).setType( PropertyType::Bool );
	registerProperty( "display", "inline" ).setType( PropertyType::String );
	registerProperty( "position", "static" ).setType( PropertyType::String );
	registerProperty( "float", "none" ).setType( PropertyType::String );
	registerProperty( "clear", "none" ).setType( PropertyType::String );
	registerProperty( "list-style-type", "none", true ).setType( PropertyType::String );
	registerProperty( "list-style-position", "outside", true ).setType( PropertyType::String );
	registerProperty( "list-style-image", "none" ).setType( PropertyType::String );
	registerProperty( "box-sizing", "content-box" ).setType( PropertyType::String );
	registerProperty( "top", "auto" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "right", "auto" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "bottom", "auto" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "left", "auto" )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "z-index", "auto" ).setType( PropertyType::NumberInt );

	registerProperty( "inner-widget-orientation", "widgeticontextbox" )
		.setType( PropertyType::String );

	registerProperty( "glyph", "" ).setType( PropertyType::String );
	registerProperty( "name", "" ).setType( PropertyType::String );
	registerProperty( "for", "" ).setType( PropertyType::String );
	registerProperty( "row-valign", "" )
		.addAlias( "row-vertical-align" )
		.setType( PropertyType::String );

	registerProperty( "text-overflow", "clip" ).setType( PropertyType::String );

	registerProperty( "check-mode", "element" ).setType( PropertyType::String );

	registerProperty( "enable-editor-flags", "" ).setType( PropertyType::String );
	registerProperty( "disable-editor-flags", "" ).setType( PropertyType::String );

	registerProperty( "line-wrap-mode", "nowrap" ).setType( PropertyType::String );
	registerProperty( "line-wrap-type", "viewport" ).setType( PropertyType::String );

	registerProperty( "display-options", "" ).setType( PropertyType::String );
	registerProperty( "menu-width-mode", "" ).setType( PropertyType::String );

	registerProperty( "action", "" ).setType( PropertyType::String );
	registerProperty( "method", "GET" ).setType( PropertyType::String );
	registerProperty( "enctype", "application/x-www-form-urlencoded" )
		.setType( PropertyType::String );
	registerProperty( "target", "_self" ).setType( PropertyType::String );
	registerProperty( "unicode-range", "" ).setType( PropertyType::String );
	registerProperty( "alignment-baseline", "baseline" ).setType( PropertyType::String );
	registerProperty( "flex-direction", "row" ).setType( PropertyType::String );
	registerProperty( "flex-wrap", "nowrap" ).setType( PropertyType::String );
	registerProperty( "justify-content", "flex-start" ).setType( PropertyType::String );
	registerProperty( "align-items", "stretch" ).setType( PropertyType::String );
	registerProperty( "align-content", "stretch" ).setType( PropertyType::String );
	registerProperty( "align-self", "auto" ).setType( PropertyType::String );
	registerProperty( "flex-grow", "0" ).setType( PropertyType::NumberFloat );
	registerProperty( "flex-shrink", "1" ).setType( PropertyType::NumberFloat );
	registerProperty( "flex-basis", "auto" ).setType( PropertyType::NumberLength );
	registerProperty( "order", "0" ).setType( PropertyType::NumberInt );
	registerProperty( "row-gap", "0px" ).setType( PropertyType::NumberLength );
	registerProperty( "column-gap", "0px" ).setType( PropertyType::NumberLength );

	registerProperty( "grid-template-rows", "none" ).setType( PropertyType::String );
	registerProperty( "grid-template-columns", "none" ).setType( PropertyType::String );
	registerProperty( "grid-template-areas", "none" ).setType( PropertyType::String );
	registerProperty( "grid-auto-rows", "auto" ).setType( PropertyType::String );
	registerProperty( "grid-auto-columns", "auto" ).setType( PropertyType::String );
	registerProperty( "grid-auto-flow", "row" ).setType( PropertyType::String );
	registerProperty( "grid-row-start", "auto" ).setType( PropertyType::String );
	registerProperty( "grid-row-end", "auto" ).setType( PropertyType::String );
	registerProperty( "grid-column-start", "auto" ).setType( PropertyType::String );
	registerProperty( "grid-column-end", "auto" ).setType( PropertyType::String );
	registerProperty( "grid-row", "auto" ).setType( PropertyType::String );
	registerProperty( "grid-column", "auto" ).setType( PropertyType::String );
	registerProperty( "grid-area", "auto" ).setType( PropertyType::String );
	registerProperty( "justify-items", "normal" ).setType( PropertyType::String );
	registerProperty( "justify-self", "auto" ).setType( PropertyType::String );
	registerProperty( "defer", "0" ).setType( PropertyType::Bool );

	// Shorthands
	registerShorthand( "margin", { "margin-top", "margin-right", "margin-bottom", "margin-left" },
					   "box" );
	registerShorthand( "layout-margin",
					   { "margin-top", "margin-right", "margin-bottom", "margin-left" }, "box" );
	registerShorthand( "layout_margin",
					   { "margin-top", "margin-right", "margin-bottom", "margin-left" }, "box" );
	registerShorthand(
		"padding", { "padding-top", "padding-right", "padding-bottom", "padding-left" }, "box" );
	registerShorthand( "background",
					   { "background-color", "background-image", "background-position",
						 "background-size", "background-repeat", "background-attachment",
						 "background-origin", "background-clip" },
					   "background" );
	registerShorthand(
		"foreground",
		{ "foreground-color", "foreground-image", "foreground-repeat", "foreground-position" },
		"background" );
	registerShorthand( "box-margin", { "column-margin", "row-margin" }, "single-value-vector" );
	registerShorthand( "background-position", { "background-position-x", "background-position-y" },
					   "background-position" );
	registerShorthand( "foreground-position", { "foreground-position-x", "foreground-position-y" },
					   "background-position" );
	registerShorthand(
		"border-color",
		{ "border-top-color", "border-right-color", "border-bottom-color", "border-left-color" },
		"border-box" );
	registerShorthand(
		"border-width",
		{ "border-top-width", "border-right-width", "border-bottom-width", "border-left-width" },
		"border-box" );
	registerShorthand( "border-radius",
					   { "border-top-left-radius", "border-top-right-radius",
						 "border-bottom-right-radius", "border-bottom-left-radius" },
					   "radius" );
	registerShorthand( "foreground-radius",
					   { "foreground-top-left-radius", "foreground-top-right-radius",
						 "foreground-bottom-right-radius", "foreground-bottom-left-radius" },
					   "radius" );
	registerShorthand( "rotation-origin-point",
					   { "rotation-origin-point-x", "rotation-origin-point-y" }, "vector2" );
	registerShorthand( "rotate-origin-point",
					   { "rotation-origin-point-x", "rotation-origin-point-y" }, "vector2" );
	registerShorthand( "scale-origin-point", { "scale-origin-point-x", "scale-origin-point-y" },
					   "vector2" );
	registerShorthand( "min-size", { "min-width", "min-height" }, "vector2" );
	registerShorthand( "max-size", { "max-width", "max-height" }, "vector2" );
	registerShorthand( "border", { "border-width", "border-style", "border-color" }, "border" );
	registerShorthand( "text-shadow", { "text-shadow-color", "text-shadow-offset" },
					   "color-vector2" );
	registerShorthand( "hint-shadow", { "hint-shadow-color", "hint-shadow-offset" },
					   "color-vector2" );
	registerShorthand( "border-left",
					   { "border-left-width", "border-left-style", "border-left-color" },
					   "border-side" );
	registerShorthand( "border-right",
					   { "border-right-width", "border-right-style", "border-right-color" },
					   "border-side" );
	registerShorthand( "border-top", { "border-top-width", "border-top-style", "border-top-color" },
					   "border-side" );
	registerShorthand( "border-bottom",
					   { "border-bottom-width", "border-bottom-style", "border-bottom-color" },
					   "border-side" );
	registerShorthand( "list-style",
					   { "list-style-type", "list-style-position", "list-style-image" },
					   "list-style" );
	registerShorthand( "font",
					   { "font-style", "font-weight", "font-size", "line-height", "font-family" },
					   "font" );
	registerShorthand( "vertical-align", { "alignment-baseline" }, "vertical-align" );
	registerShorthand( "flex-flow", { "flex-direction", "flex-wrap" }, "single-value-vector" );
	registerShorthand( "flex", { "flex-grow", "flex-shrink", "flex-basis" }, "flex" );
	registerShorthand( "gap", { "row-gap", "column-gap" }, "vector2" );
	registerShorthand( "grid-template",
					   { "grid-template-rows", "grid-template-columns", "grid-template-areas" },
					   "grid-template" );
	registerShorthand( "grid",
					   { "grid-template-rows", "grid-template-columns", "grid-template-areas",
						 "grid-auto-rows", "grid-auto-columns", "grid-auto-flow" },
					   "grid" );
	registerShorthand( "place-items", { "align-items", "justify-items" }, "vector2" );
	registerShorthand( "place-self", { "align-self", "justify-self" }, "vector2" );
	registerShorthand( "place-content", { "align-content", "justify-content" }, "vector2" );
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

				// small hack to at least hide none borders until border-style is implemented
				if ( tok == "none" ) {
					int pos = getIndexEndingWith( propNames, "-width" );
					if ( pos != -1 ) {
						const ShorthandDefinition* shorthand = getShorthand( propNames[pos] );
						if ( NULL != shorthand ) {
							auto bbVec = mShorthandParsers["border-box"]( shorthand, "0" );
							for ( auto& bb : bbVec )
								properties.emplace_back( bb );
						}
					}
					continue;
				}

				int pos = getIndexEndingWith( propNames, "-style" );
				// boder-style is not implemented yet
				if ( pos != -1 )
					continue;

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

				// At least reset the border width if "none" was used
				if ( "none" == tok ) {
					int pos = getIndexEndingWith( propNames, "-width" );
					if ( pos != -1 )
						properties.emplace_back( StyleSheetProperty( propNames[pos], "0" ) );
				}

				// boder-style is not implemented yet
				int pos = getIndexEndingWith( propNames, "-style" );
				if ( pos != -1 )
					continue;
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
