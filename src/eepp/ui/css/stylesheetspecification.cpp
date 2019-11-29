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

ShorthandDefinition& StyleSheetSpecification::registerShorthand(
	const std::string& name, const std::vector<std::string>& properties,
	const ShorthandDefinition::ShorthandType& shorthandType ) {
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
	registerProperty( "id", "", false );
	registerProperty( "class", "", false );
	registerProperty( "x", "", false );
	registerProperty( "y", "", false );
	registerProperty( "width", "", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockWidth );
	registerProperty( "height", "", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockHeight );
	registerProperty( "background-color", "", false );
	registerProperty( "background-image", "", false );
	registerProperty( "background-position-x", "center", false );
	registerProperty( "background-position-y", "center", false );
	registerProperty( "background-repeat", "no-repeat", false );
	registerProperty( "background-size", "auto", false );
	registerProperty( "foreground-color", "", false );
	registerProperty( "foreground-image", "", false );
	registerProperty( "foreground-position-x", "center", false );
	registerProperty( "foreground-position-y", "center", false );
	registerProperty( "foreground-repeat", "no-repeat", false );
	registerProperty( "foreground-size", "", false );
	registerProperty( "foreground-radius", "0px", false );
	registerProperty( "border-color", "", false );
	registerProperty( "border-width", "", false );
	registerProperty( "border-radius", "0px", false );
	registerProperty( "visible", "true", false );
	registerProperty( "enabled", "true", false );
	registerProperty( "theme", "", false );
	registerProperty( "skin", "", false );
	registerProperty( "skin-color", "", false );
	registerProperty( "gravity", "", false );
	registerProperty( "flags", "", false );
	registerProperty( "margin-top", "0px", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockHeight );
	registerProperty( "margin-left", "0px", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-right", "0px", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-bottom", "0px", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockHeight );
	registerProperty( "tooltip", "", false );
	registerProperty( "layout-weight", "", false ).addAlias( "layout_weight" );
	registerProperty( "layout-gravity", "", false ).addAlias( "layout_gravity" );
	registerProperty( "layout-width", "", false ).addAlias( "layout_width" );
	registerProperty( "layout-height", "", false ).addAlias( "layout_height" );
	registerProperty( "layout-to-left-of", "", false ).addAlias( "layout_to_left_of" );
	registerProperty( "layout-to-right-of", "", false ).addAlias( "layout_to_right_of" );
	registerProperty( "layout-to-top-of", "", false ).addAlias( "layout_to_top_of" );
	registerProperty( "layout-to-bottom-of", "", false ).addAlias( "layout_to_bottom_of" );
	registerProperty( "clip", "", false );
	registerProperty( "rotation", "", false );
	registerProperty( "scale", "", false );
	registerProperty( "rotation-origin-point-x", "", false );
	registerProperty( "rotation-origin-point-y", "", false );
	registerProperty( "scale-origin-point-x", "", false );
	registerProperty( "scale-origin-point-y", "", false );
	registerProperty( "blend-mode", "", false );
	registerProperty( "padding-left", "", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockWidth );
	registerProperty( "padding-right", "", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockWidth );
	registerProperty( "padding-top", "", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockHeight );
	registerProperty( "padding-bottom", "", false )
		.setRelativeTarget( PropertyDefinition::RelativeTarget::ContainingBlockHeight );
	registerProperty( "opacity", "", false );
	registerProperty( "cursor", "arrow", false );
	registerProperty( "text", "", false );
	registerProperty( "color", "", false ).addAlias( "text-color" );
	registerProperty( "shadow-color", "", false );
	registerProperty( "selected-color", "", false );
	registerProperty( "selection-back-color", "", false );
	registerProperty( "font-family", "", false ).addAlias( "font-name" );
	registerProperty( "font-size", "", false ).addAlias( "text-size" );
	registerProperty( "font-style", "", false )
		.addAlias( "text-style" )
		.addAlias( "text-decoration" );
	registerProperty( "text-stroke-width", "", false ).addAlias( "fontoutlinethickness" );
	registerProperty( "text-stroke-color", "", false ).addAlias( "fontoutlinecolor" );
	registerProperty( "text-selection", "", false );
	registerProperty( "text-align", "", false );
	registerProperty( "icon", "", false );
	registerProperty( "min-icon-size", "", false );
	registerProperty( "icon-horizontal-margin", "", false );
	registerProperty( "icon-auto-margin", "", false );
	registerProperty( "src", "", false );
	registerProperty( "scale-type", "", false );
	registerProperty( "tint", "", false );
	registerProperty( "rotation-origin-point", "", false );
	registerProperty( "max-text-length", "", false );
	registerProperty( "min-tab-width", "", false );
	registerProperty( "max-tab-width", "", false );
	registerProperty( "tab-closable", "", false );
	registerProperty( "special-border-tabs", "", false );
	registerProperty( "line-below-tabs", "", false );
	registerProperty( "line-below-tabs-color", "", false );
	registerProperty( "line-below-tabs-y-offset", "", false );
	registerProperty( "tab-separation", "", false );
	registerProperty( "selected", "", false ).addAlias( "active" );
	registerProperty( "popup-to-main-control", "", false );
	registerProperty( "max-visible-items", "", false );
	registerProperty( "selected-index", "", false );
	registerProperty( "selected-text", "", false );
	registerProperty( "scrollbar-type", "", false );
	registerProperty( "row-height", "", false );
	registerProperty( "vscroll-mode", "", false );
	registerProperty( "hscroll-mode", "", false );

	registerProperty( "column-span", "", false );
	registerProperty( "row-span", "", false );
	registerProperty( "span", "", false );
	registerProperty( "column-mode", "", false );
	registerProperty( "row-mode", "", false );
	registerProperty( "column-weight", "", false );
	registerProperty( "column-width", "", false );
	registerProperty( "row-weight", "", false );
	registerProperty( "reverse-draw", "", false );

	registerProperty( "orientation", "", false );
	registerProperty( "indeterminate", "", false );
	registerProperty( "max-progress", "", false );
	registerProperty( "progress", "", false );
	registerProperty( "fill-color", "", false );
	registerProperty( "radius", "", false );
	registerProperty( "outline-thickness", "", false );
	registerProperty( "animation-speed", "", false );
	registerProperty( "arc-start-angle", "", false );
	registerProperty( "min-width", "", false );
	registerProperty( "min-margin-right", "", false );
	registerProperty( "min-icon-space", "", false );

	registerProperty( "total-steps", "", false );
	registerProperty( "vertical-expand", "", false );
	registerProperty( "display-percent", "", false );
	registerProperty( "filler-padding-left", "", false );
	registerProperty( "filler-padding-top", "", false );
	registerProperty( "filler-padding-right", "", false );
	registerProperty( "filler-padding-bottom", "", false );
	registerProperty( "movement-speed", "", false );
	registerProperty( "min-value", "", false );
	registerProperty( "max-value", "", false );
	registerProperty( "value", "", false );
	registerProperty( "click-step", "", false );
	registerProperty( "page-step", "", false );
	registerProperty( "background-expand", "", false );
	registerProperty( "scrollbar-mode", "", false );
	registerProperty( "half-slider", "", false );
	registerProperty( "name", "", false );
	registerProperty( "owns", "", false );
	registerProperty( "allow-editing", "", false );
	registerProperty( "max-length", "", false );
	registerProperty( "free-editing", "", false );
	registerProperty( "only-numbers", "", false );
	registerProperty( "allow-dot", "", false );
	registerProperty( "touch-drag", "", false );
	registerProperty( "touch-drag-deceleration", "", false );

	registerProperty( "base-alpha", "", false );
	registerProperty( "buttons-position-offset", "", false );
	registerProperty( "window-flags", "", false );
	registerProperty( "decoration-size", "", false );
	registerProperty( "border-size", "", false );
	registerProperty( "min-window-size", "", false );
	registerProperty( "buttons-separation", "", false );
	registerProperty( "min-corner-distance", "", false );
	registerProperty( "decoration-auto-size", "", false );
	registerProperty( "border-auto-size", "", false );

	registerProperty( "margin-between-buttons", "", false );
	registerProperty( "button-margin", "", false );
	registerProperty( "menu-height", "", false );
	registerProperty( "first-button-margin-left", "", false );

	registerProperty( "scale-origin-point", "", false );

	// Shorthands
	registerProperty( "background-position", "", false );
	registerProperty( "transition", "", false );

	registerShorthand( "margin", {"margin-left", "margin-top", "margin-right", "margin-bottom"},
					   ShorthandDefinition::ShorthandType::Box );
	registerShorthand( "padding",
					   {"padding-left", "padding-top", "padding-right", "padding-bottom"},
					   ShorthandDefinition::ShorthandType::Box );
	registerShorthand( "background", {"background-color", "background-image"},
					   ShorthandDefinition::ShorthandType::Background );
	registerShorthand( "foreground", {"foreground-color", "foreground-image"},
					   ShorthandDefinition::ShorthandType::Background );
	registerShorthand( "filler-padding",
					   {"filler-padding-left", "filler-padding-top", "filler-padding-right",
						"filler-padding-bottom"},
					   ShorthandDefinition::ShorthandType::Box );
	/*registerShorthand( "rotation-origin-point", {"rotation-origin-point-x",
	"rotation-origin-point-y"}, ShorthandDefinition::ShorthandType::Vector2 ); registerShorthand(
	"scale-origin-point", {"scale-origin-point-x", "scale-origin-point-y"},
					   ShorthandDefinition::ShorthandType::Vector2 );*/
}

}}} // namespace EE::UI::CSS
