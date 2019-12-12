#include <eepp/ui/css/stylesheetspecification.hpp>

namespace EE { namespace UI { namespace CSS {

SINGLETON_DECLARE_IMPLEMENTATION( StyleSheetSpecification )

StyleSheetSpecification::StyleSheetSpecification() {
	registerDefaultProperties();
}

StyleSheetSpecification::~StyleSheetSpecification() {}

PropertyDefinition& StyleSheetSpecification::registerProperty( const std::string& propertyVame,
															   const std::string& defaultValue,
															   bool inherited ) {
	return mPropertySpecification.registerProperty( propertyVame, defaultValue, inherited );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const Uint32& id ) const {
	return mPropertySpecification.getProperty( id );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const std::string& name ) const {
	return mPropertySpecification.getProperty( name );
}

ShorthandDefinition&
StyleSheetSpecification::registerShorthand( const std::string& name,
											const std::vector<std::string>& properties,
											const ShorthandType& shorthandType ) {
	return mPropertySpecification.registerShorthand( name, properties, shorthandType );
}

const ShorthandDefinition* StyleSheetSpecification::getShorthand( const Uint32& id ) const {
	return mPropertySpecification.getShorthand( id );
}

const ShorthandDefinition* StyleSheetSpecification::getShorthand( const std::string& name ) const {
	return mPropertySpecification.getShorthand( name );
}

bool StyleSheetSpecification::isShorthand( const Uint32& id ) const {
	return mPropertySpecification.isShorthand( id );
}

bool StyleSheetSpecification::isShorthand( const std::string& name ) const {
	return mPropertySpecification.isShorthand( name );
}

void StyleSheetSpecification::registerDefaultProperties() {
	registerProperty( "id", "", false ).setType( PropertyType::String );
	registerProperty( "class", "", false ).setType( PropertyType::String );
	registerProperty( "x", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "y", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "width", "", false )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "height", "", false )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "background-color", "", false ).setType( PropertyType::Color );
	registerProperty( "background-image", "", false );
	registerProperty( "background-position-x", "center", false )
		.setRelativeTarget( PropertyRelativeTarget::BackgroundWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( "background-position-y", "center", false )
		.setRelativeTarget( PropertyRelativeTarget::BackgroundHeight )
		.setType( PropertyType::NumberLength );
	registerProperty( "background-repeat", "no-repeat", false );
	registerProperty( "background-size", "auto", false ).setType( PropertyType::BackgroundSize );
	registerProperty( "foreground-color", "", false ).setType( PropertyType::Color );
	registerProperty( "foreground-image", "", false );
	registerProperty( "foreground-position-x", "center", false )
		.setRelativeTarget( PropertyRelativeTarget::ForegroundWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( "foreground-position-y", "center", false )
		.setRelativeTarget( PropertyRelativeTarget::ForegroundHeight )
		.setType( PropertyType::NumberLength );
	registerProperty( "foreground-repeat", "no-repeat", false );
	registerProperty( "foreground-size", "", false ).setType( PropertyType::ForegroundSize );
	registerProperty( "foreground-radius", "0px", false ).setType( PropertyType::NumberInt );
	registerProperty( "border-color", "", false ).setType( PropertyType::Color );
	registerProperty( "border-width", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "border-radius", "0px", false ).setType( PropertyType::NumberInt );
	registerProperty( "visible", "true", false ).setType( PropertyType::Bool );
	registerProperty( "enabled", "true", false ).setType( PropertyType::Bool );
	registerProperty( "theme", "", false );
	registerProperty( "skin", "", false );
	registerProperty( "skin-color", "", false ).setType( PropertyType::Color );
	registerProperty( "gravity", "", false );
	registerProperty( "flags", "", false );
	registerProperty( "margin-top", "0px", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-top" )
		.addAlias( "layout_margintop" )
		.addAlias( "margintop" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "margin-left", "0px", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-left" )
		.addAlias( "layout_marginleft" )
		.addAlias( "marginleft" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-right", "0px", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-right" )
		.addAlias( "layout_marginright" )
		.addAlias( "marginright" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-bottom", "0px", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-bottom" )
		.addAlias( "layout_marginbottom" )
		.addAlias( "marginbottom" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "tooltip", "", false ).setType( PropertyType::String );
	registerProperty( "layout-weight", "", false )
		.addAlias( "layout_weight" )
		.setType( PropertyType::NumberFloat );
	registerProperty( "layout-gravity", "", false ).addAlias( "layout_gravity" );
	registerProperty( "layout-width", "", false ).addAlias( "layout_width" );
	registerProperty( "layout-height", "", false ).addAlias( "layout_height" );
	registerProperty( "layout-to-left-of", "", false ).addAlias( "layout_to_left_of" );
	registerProperty( "layout-to-right-of", "", false ).addAlias( "layout_to_right_of" );
	registerProperty( "layout-to-top-of", "", false ).addAlias( "layout_to_top_of" );
	registerProperty( "layout-to-bottom-of", "", false ).addAlias( "layout_to_bottom_of" );
	registerProperty( "clip", "", false ).setType( PropertyType::Bool );
	registerProperty( "rotation", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "scale", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "rotation-origin-point-x", "50%", false )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( "rotation-origin-point-y", "50%", false )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockHeight )
		.setType( PropertyType::NumberLength );
	registerProperty( "scale-origin-point-x", "50%", false )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( "scale-origin-point-y", "50%", false )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockHeight )
		.setType( PropertyType::NumberLength );
	registerProperty( "blend-mode", "", false );
	registerProperty( "padding-left", "", false )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "padding-right", "", false )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "padding-top", "", false )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "padding-bottom", "", false )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "opacity", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "cursor", "arrow", false );
	registerProperty( "text", "", false ).setType( PropertyType::String );
	registerProperty( "color", "", false )
		.setType( PropertyType::Color )
		.addAlias( "text-color" )
		.addAlias( "textcolor" );
	registerProperty( "shadow-color", "", false ).setType( PropertyType::Color );
	registerProperty( "selected-color", "", false ).setType( PropertyType::Color );
	registerProperty( "selection-back-color", "", false ).setType( PropertyType::Color );
	registerProperty( "font-family", "", false ).addAlias( "font-name" );
	registerProperty( "font-size", "", false )
		.setType( PropertyType::NumberFloat )
		.addAlias( "text-size" )
		.addAlias( "textsize" );
	registerProperty( "font-style", "", false )
		.addAlias( "text-style" )
		.addAlias( "text-decoration" );
	registerProperty( "text-stroke-width", "", false )
		.setType( PropertyType::NumberFloat )
		.addAlias( "fontoutlinethickness" );
	registerProperty( "text-stroke-color", "", false )
		.setType( PropertyType::Color )
		.addAlias( "fontoutlinecolor" );
	registerProperty( "text-selection", "", false ).setType( PropertyType::Bool );
	registerProperty( "text-align", "", false );
	registerProperty( "icon", "", false );
	registerProperty( "min-icon-size", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "icon-horizontal-margin", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "icon-auto-margin", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "src", "", false );
	registerProperty( "scale-type", "", false );
	registerProperty( "tint", "", false ).setType( PropertyType::Color );
	registerProperty( "rotation-origin-point", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "max-text-length", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "min-tab-width", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "max-tab-width", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "tab-closable", "", false ).setType( PropertyType::Bool );
	registerProperty( "special-border-tabs", "", false ).setType( PropertyType::Bool );
	registerProperty( "line-below-tabs", "", false ).setType( PropertyType::Bool );
	registerProperty( "line-below-tabs-color", "", false ).setType( PropertyType::Color );
	registerProperty( "line-below-tabs-y-offset", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "tab-separation", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "selected", "", false ).setType( PropertyType::Bool ).addAlias( "active" );
	registerProperty( "popup-to-main-control", "", false ).setType( PropertyType::Bool );
	registerProperty( "max-visible-items", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "selected-index", "", false );
	registerProperty( "selected-text", "", false );
	registerProperty( "scrollbar-type", "", false );
	registerProperty( "row-height", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "vscroll-mode", "", false );
	registerProperty( "hscroll-mode", "", false );

	registerProperty( "column-span", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "row-span", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "span", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "column-mode", "", false );
	registerProperty( "row-mode", "", false );
	registerProperty( "column-weight", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "column-width", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "row-weight", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "reverse-draw", "", false ).setType( PropertyType::Bool );

	registerProperty( "orientation", "", false );
	registerProperty( "indeterminate", "", false ).setType( PropertyType::Bool );
	registerProperty( "max-progress", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "progress", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "fill-color", "", false ).setType( PropertyType::Color );
	registerProperty( "radius", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "outline-thickness", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "animation-speed", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "arc-start-angle", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "min-width", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "min-margin-right", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "min-icon-space", "", false ).setType( PropertyType::NumberInt );

	registerProperty( "total-steps", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "vertical-expand", "", false ).setType( PropertyType::Bool );
	registerProperty( "display-percent", "", false ).setType( PropertyType::Bool );
	registerProperty( "filler-padding-left", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "filler-padding-top", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "filler-padding-right", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "filler-padding-bottom", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "movement-speed", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "min-value", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "max-value", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "value", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "click-step", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "page-step", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "background-expand", "", false ).setType( PropertyType::Bool );
	registerProperty( "scrollbar-mode", "", false );
	registerProperty( "half-slider", "", false ).setType( PropertyType::Bool );
	registerProperty( "name", "", false ).setType( PropertyType::String );
	registerProperty( "owns", "", false ).setType( PropertyType::String );
	registerProperty( "allow-editing", "", false ).setType( PropertyType::Bool );
	registerProperty( "max-length", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "free-editing", "", false ).setType( PropertyType::Bool );
	registerProperty( "only-numbers", "", false ).setType( PropertyType::Bool );
	registerProperty( "allow-dot", "", false ).setType( PropertyType::Bool );
	registerProperty( "touch-drag", "", false ).setType( PropertyType::Bool );
	registerProperty( "touch-drag-deceleration", "", false ).setType( PropertyType::NumberFloat );

	registerProperty( "base-alpha", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "buttons-position-offset", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "window-flags", "", false ).addAlias( "winflags" );
	registerProperty( "decoration-size", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "border-size", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "min-window-size", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "buttons-separation", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "min-corner-distance", "", false );
	registerProperty( "decoration-auto-size", "", false ).setType( PropertyType::Bool );
	registerProperty( "border-auto-size", "", false ).setType( PropertyType::Bool );

	registerProperty( "margin-between-buttons", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "button-margin", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "menu-height", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "first-button-margin-left", "", false ).setType( PropertyType::NumberInt );

	registerProperty( "scale-origin-point", "", false ).setType( PropertyType::Vector2 );

	registerProperty( "word-wrap", "", false ).setType( PropertyType::Bool );

	registerProperty( "hint", "", false ).setType( PropertyType::String );
	registerProperty( "hint-color", "", false ).setType( PropertyType::Color );
	registerProperty( "hint-shadow-color", "", false ).setType( PropertyType::Color );
	registerProperty( "hint-font-size", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "hint-font-style", "", false ).setType( PropertyType::String );
	registerProperty( "hint-stroke-width", "", false )
		.setType( PropertyType::NumberFloat )
		.addAlias( "hintoutlinethickness" );
	registerProperty( "hint-stroke-color", "", false )
		.setType( PropertyType::Color );
	registerProperty( "hint-font-family", "", false ).addAlias( "hint-font-name" );

	// Shorthands
	registerProperty( "background-position", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "transition", "", false );

	registerShorthand( "margin", {"margin-left", "margin-top", "margin-right", "margin-bottom"},
					   ShorthandType::Box );
	registerShorthand( "layout-margin",
					   {"margin-left", "margin-top", "margin-right", "margin-bottom"},
					   ShorthandType::Box );
	registerShorthand( "layout_margin",
					   {"margin-left", "margin-top", "margin-right", "margin-bottom"},
					   ShorthandType::Box );
	registerShorthand( "padding",
					   {"padding-left", "padding-top", "padding-right", "padding-bottom"},
					   ShorthandType::Box );
	registerShorthand( "background", {"background-color", "background-image"},
					   ShorthandType::Background );
	registerShorthand( "foreground", {"foreground-color", "foreground-image"},
					   ShorthandType::Background );
	registerShorthand( "filler-padding",
					   {"filler-padding-left", "filler-padding-top", "filler-padding-right",
						"filler-padding-bottom"},
					   ShorthandType::Box );
	registerShorthand( "span", {"column-span", "row-span"}, ShorthandType::SingleValueVector );
	registerShorthand( "background-position", {"background-position-x", "background-position-y"},
					   ShorthandType::BackgroundPosition );
	registerShorthand( "foreground-position", {"foreground-position-x", "foreground-position-y"},
					   ShorthandType::BackgroundPosition );
	/*registerShorthand( "rotation-origin-point", {"rotation-origin-point-x",
	"rotation-origin-point-y"}, ShorthandType::Vector2 ); registerShorthand(
	"scale-origin-point", {"scale-origin-point-x", "scale-origin-point-y"},
					   ShorthandType::Vector2 );*/
}

}}} // namespace EE::UI::CSS
