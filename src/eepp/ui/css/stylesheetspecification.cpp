#include <eepp/system/log.hpp>
#include <eepp/ui/css/propertyspecification.hpp>
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

const PropertyDefinition* StyleSheetSpecification::getProperty( const Uint32& id ) const {
	return mPropertySpecification->getProperty( id );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const std::string& name ) const {
	return mPropertySpecification->getProperty( name );
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
	registerProperty( "background-color", "" ).setType( PropertyType::Color );
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
	registerProperty( "foreground-radius", "0px" ).setType( PropertyType::NumberLength );
	registerProperty( "visible", "true" ).setType( PropertyType::Bool );
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
	registerProperty( "tooltip", "" ).setType( PropertyType::String );
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
	registerProperty( "text-transform", "" ).setType( PropertyType::String );
	registerProperty( "color", "" )
		.setType( PropertyType::Color )
		.addAlias( "text-color" )
		.addAlias( "textcolor" );
	registerProperty( "text-shadow-color", "" ).setType( PropertyType::Color );
	registerProperty( "text-shadow-offset", "" ).setType( PropertyType::Vector2 );
	registerProperty( "selection-color", "" ).setType( PropertyType::Color );
	registerProperty( "selection-back-color", "" ).setType( PropertyType::Color );
	registerProperty( "font-family", "" ).addAlias( "font-name" ).setType( PropertyType::String );
	registerProperty( "font-size", "" )
		.setType( PropertyType::NumberLength )
		.addAlias( "text-size" )
		.addAlias( "textsize" );
	registerProperty( "font-style", "" ).addAlias( "text-style" ).addAlias( "text-decoration" );
	registerProperty( "line-spacing", "" ).setType( PropertyType::NumberLength );
	registerProperty( "text-stroke-width", "" )
		.setType( PropertyType::NumberLength )
		.addAlias( "fontoutlinethickness" );
	registerProperty( "text-stroke-color", "" )
		.setType( PropertyType::Color )
		.addAlias( "fontoutlinecolor" );
	registerProperty( "text-selection", "" ).setType( PropertyType::Bool );
	registerProperty( "text-align", "" );
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

	registerProperty( "word-wrap", "" ).setType( PropertyType::Bool );

	registerProperty( "hint", "" ).setType( PropertyType::String );
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

	registerProperty( "inner-widget-orientation", "widgeticontextbox" )
		.setType( PropertyType::String );

	registerProperty( "glyph", "" ).setType( PropertyType::String );
	registerProperty( "name", "" ).setType( PropertyType::String );
	registerProperty( "row-valign", "" )
		.addAlias( "row-vertical-align" )
		.setType( PropertyType::String );

	registerProperty( "text-overflow", "clip" ).setType( PropertyType::String );

	registerProperty( "check-mode", "element" ).setType( PropertyType::String );

	// Shorthands
	registerShorthand( "margin", { "margin-top", "margin-right", "margin-bottom", "margin-left" },
					   "box" );
	registerShorthand( "layout-margin",
					   { "margin-top", "margin-right", "margin-bottom", "margin-left" }, "box" );
	registerShorthand( "layout_margin",
					   { "margin-top", "margin-right", "margin-bottom", "margin-left" }, "box" );
	registerShorthand(
		"padding", { "padding-top", "padding-right", "padding-bottom", "padding-left" }, "box" );
	registerShorthand(
		"background",
		{ "background-color", "background-image", "background-repeat", "background-position" },
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

void StyleSheetSpecification::registerDefaultNodeSelectors() {
	mNodeSelectors["empty"] = []( const UIWidget* node, int, int, const FunctionString& ) -> bool {
		return node->getFirstChild() == NULL;
	};
	mNodeSelectors["first-child"] = []( const UIWidget* node, int, int,
										const FunctionString& ) -> bool {
		return NULL != node->getParent() && node->getParent()->getFirstChild() == node;
	};
	mNodeSelectors["enabled"] = []( const UIWidget* node, int, int,
									const FunctionString& ) -> bool { return node->isEnabled(); };
	mNodeSelectors["disabled"] = []( const UIWidget* node, int, int,
									 const FunctionString& ) -> bool { return !node->isEnabled(); };
	mNodeSelectors["first-of-type"] = []( const UIWidget* node, int, int,
										  const FunctionString& ) -> bool {
		Node* child = NULL != node->getParent() ? node->getParent()->getFirstChild() : NULL;
		Uint32 type = node->getType();
		while ( NULL != child ) {
			if ( type == child->getType() ) {
				return child == node;
			}
			child = child->getNextNode();
		};
		return false;
	};
	mNodeSelectors["last-child"] = []( const UIWidget* node, int, int,
									   const FunctionString& ) -> bool {
		return NULL != node->getParent() && node->getParent()->getLastChild() == node;
	};
	mNodeSelectors["last-of-type"] = []( const UIWidget* node, int, int,
										 const FunctionString& ) -> bool {
		Node* child = NULL != node->getParent() ? node->getParent()->getLastChild() : NULL;
		Uint32 type = node->getType();
		while ( NULL != child ) {
			if ( type == child->getType() ) {
				return child == node;
			}
			child = child->getPrevNode();
		};
		return false;
	};
	mNodeSelectors["only-child"] = []( const UIWidget* node, int, int,
									   const FunctionString& ) -> bool {
		return NULL != node->getParent() && node->getParent()->getChildCount() == 1;
	};
	mNodeSelectors["only-of-type"] = []( const UIWidget* node, int, int,
										 const FunctionString& ) -> bool {
		Node* child = NULL != node->getParent() ? node->getParent()->getFirstChild() : NULL;
		Uint32 type = node->getType();
		Uint32 typeCount = 0;
		while ( NULL != child ) {
			if ( child->getType() == type ) {
				typeCount++;
			}
			if ( typeCount > 1 )
				return false;
			child = child->getNextNode();
		};
		return typeCount == 1;
	};
	mNodeSelectors["nth-child"] = []( const UIWidget* node, int a, int b,
									  const FunctionString& ) -> bool {
		return isNth( a, b, node->getNodeIndex() + 1 );
	};
	mNodeSelectors["nth-last-child"] = []( const UIWidget* node, int a, int b,
										   const FunctionString& ) -> bool {
		return isNth( a, b, node->getChildCount() - node->getNodeIndex() );
	};
	mNodeSelectors["nth-of-type"] = []( const UIWidget* node, int a, int b,
										const FunctionString& ) -> bool {
		return isNth( a, b, node->getNodeOfTypeIndex() + 1 );
	};
	mNodeSelectors["nth-last-of-type"] = []( const UIWidget* node, int a, int b,
											 const FunctionString& ) -> bool {
		return node->getParent() != NULL
				   ? isNth( a, b,
							node->getParent()->getChildOfTypeCount( node->getType() ) -
								node->getNodeOfTypeIndex() )
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
				if ( String::fromString( t, parameters ) ) {
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
		value = String::trim( value );
		if ( value.empty() )
			return {};
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
		if ( propNames.size() != 4 ) {
			Log::error( "ShorthandType::Box properties must be 4 for %s",
						shorthand->getName().c_str() );
			return properties;
		}

		auto ltrbSplit = String::split( value, ' ', true );

		if ( ltrbSplit.size() >= 2 ) {
			for ( size_t i = 0; i < ltrbSplit.size(); i++ )
				properties.emplace_back( StyleSheetProperty( propNames[i], ltrbSplit[i] ) );
		} else if ( ltrbSplit.size() == 1 ) {
			for ( size_t i = 0; i < propNames.size(); i++ )
				properties.emplace_back( StyleSheetProperty( propNames[i], ltrbSplit[0] ) );
		}
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
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		std::vector<std::string> values = String::split( value, ',' );
		std::unordered_map<std::string, std::vector<std::string>> tmpProperties;

		for ( auto& val : values ) {
			std::vector<std::string> pos = String::split( val, ' ' );
			bool lastWasKeyword = false;
			bool isXAxis = true;
			std::string xAxis = "";
			std::string yAxis = "";

			for ( const auto& data : pos ) {
				bool isKeyword = isKeywordPosition( data );
				if ( isXAxis && ( isKeyword && lastWasKeyword ) )
					isXAxis = false;

				if ( isXAxis )
					xAxis += data + " ";
				else
					yAxis += data + " ";

				if ( isXAxis && ( !isKeyword && !lastWasKeyword ) )
					isXAxis = false;

				lastWasKeyword = isKeyword;
			}

			if ( xAxis.empty() )
				xAxis = "center";

			if ( yAxis.empty() )
				yAxis = "center";

			if ( String::startsWith( xAxis, "top" ) || String::startsWith( xAxis, "bottom" ) )
				std::swap( xAxis, yAxis );

			String::trimInPlace( xAxis );
			String::trimInPlace( yAxis );

			tmpProperties[propNames[0]].emplace_back( xAxis );
			tmpProperties[propNames[1]].emplace_back( yAxis );
		}

		for ( auto& props : tmpProperties ) {
			properties.push_back(
				StyleSheetProperty( props.first, String::join( props.second, ',' ) ) );
		}
		return properties;
	};

	mShorthandParsers["background"] =
		[this]( const ShorthandDefinition* shorthand,
				std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() || "none" == value )
			return {};
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		std::vector<std::string> tokens = String::split( value, " ", "", "(" );
		std::string positionStr;

		for ( auto& tok : tokens ) {
			if ( mDrawableImageParser.exists( tok ) ) {
				int pos = getIndexEndingWith( propNames, "-image" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], tok ) );
			} else if ( -1 != String::valueIndex( tok, "repeat;repeat-x;repeat-y;no-repeat" ) ) {
				int pos = getIndexEndingWith( propNames, "-repeat" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], value ) );
			} else if ( -1 != String::valueIndex( tok, "left;right;top;bottom;center" ) ||
						String::isNumber( tok[0] ) || tok[0] == '-' || tok[0] == '.' ||
						tok[0] == '+' ) {
				positionStr += tok + " ";
			} else if ( Color::isColorString( tok ) ) {
				int pos = getIndexEndingWith( propNames, "-color" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], value ) );
			}
		}

		if ( !positionStr.empty() ) {
			String::trimInPlace( positionStr );
			int pos = getIndexEndingWith( propNames, "-position" );
			if ( pos != -1 ) {
				const ShorthandDefinition* shorthand = getShorthand( propNames[pos] );
				if ( NULL != shorthand ) {
					auto bpVec = mShorthandParsers["background-position"]( shorthand, positionStr );
					for ( auto& bp : bpVec )
						properties.emplace_back( bp );
				}
			}
		}

		return properties;
	};

	mShorthandParsers["border"] = [this]( const ShorthandDefinition* shorthand,
										  std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( value.empty() || "none" == value )
			return {};

		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		std::vector<std::string> tokens = String::split( value, " ", "", "(" );

		for ( auto& tok : tokens ) {
			if ( -1 !=
				 String::valueIndex(
					 tok, "none;hidden;dotted;dashed;solid;double;groove;ridge;inset;outset" ) ) {
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
		if ( value.empty() || "none" == value )
			return {};

		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string>& propNames = shorthand->getProperties();
		std::vector<std::string> tokens = String::split( value, " ", "", "(" );

		for ( auto& tok : tokens ) {
			if ( -1 !=
				 String::valueIndex(
					 tok, "none;hidden;dotted;dashed;solid;double;groove;ridge;inset;outset" ) ) {
				int pos = getIndexEndingWith( propNames, "-style" );
				// boder-style is not implemented yet
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
}

}}} // namespace EE::UI::CSS
