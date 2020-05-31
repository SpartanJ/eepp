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
		eePRINTL( "Shorthand parser \"%s\" is already registered.", name.c_str() );
		return;
	}

	mShorthandParsers[name] = shorthandParserFunc;
}

ShorthandParserFunc StyleSheetSpecification::getShorthandParser( const std::string& name ) {
	if ( mShorthandParsers.find( name ) == mShorthandParsers.end() ) {
		eePRINTL( "Shorthand parser \"%s\" not found.", name.c_str() );
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
		.addAlias( "layout-margin-top" )
		.addAlias( "layout_margintop" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "margin-left", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-left" )
		.addAlias( "layout_marginleft" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-right", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-right" )
		.addAlias( "layout_marginright" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-bottom", "0px" )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-bottom" )
		.addAlias( "layout_marginbottom" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "tooltip", "" ).setType( PropertyType::String );
	registerProperty( "layout-weight", "" )
		.addAlias( "layout_weight" )
		.setType( PropertyType::NumberFloat );
	registerProperty( "layout-gravity", "" ).addAlias( "layout_gravity" );
	registerProperty( "layout-width", "" ).addAlias( "layout_width" );
	registerProperty( "layout-height", "" ).addAlias( "layout_height" );
	registerProperty( "layout-to-left-of", "" ).addAlias( "layout_to_left_of" );
	registerProperty( "layout-to-right-of", "" ).addAlias( "layout_to_right_of" );
	registerProperty( "layout-to-top-of", "" ).addAlias( "layout_to_top_of" );
	registerProperty( "layout-to-bottom-of", "" ).addAlias( "layout_to_bottom_of" );
	registerProperty( "clip", "" ).setType( PropertyType::Bool );
	registerProperty( "rotation", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "scale", "" ).setType( PropertyType::Vector2 );
	registerProperty( "rotation-origin-point-x", "50%" )
		.setRelativeTarget( PropertyRelativeTarget::LocalBlockWidth )
		.setType( PropertyType::NumberLength );
	registerProperty( "rotation-origin-point-y", "50%" )
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
	registerProperty( "color", "" )
		.setType( PropertyType::Color )
		.addAlias( "text-color" )
		.addAlias( "textcolor" );
	registerProperty( "shadow-color", "" ).setType( PropertyType::Color );
	registerProperty( "selection-color", "" ).setType( PropertyType::Color );
	registerProperty( "selection-back-color", "" ).setType( PropertyType::Color );
	registerProperty( "font-family", "" ).addAlias( "font-name" ).setType( PropertyType::String );
	registerProperty( "font-size", "" )
		.setType( PropertyType::NumberLength )
		.addAlias( "text-size" )
		.addAlias( "textsize" );
	registerProperty( "font-style", "" ).addAlias( "text-style" ).addAlias( "text-decoration" );
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
	registerProperty( "tabs-edges-diff-skin", "" ).setType( PropertyType::Bool );
	registerProperty( "tab-separation", "" ).setType( PropertyType::NumberLength );
	registerProperty( "tab-height", "" ).setType( PropertyType::NumberLength );
	registerProperty( "selected", "" ).setType( PropertyType::Bool ).addAlias( "active" );
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
	registerProperty( "free-editing", "" ).setType( PropertyType::Bool );
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
	registerProperty( "hint-font-size", "" ).setType( PropertyType::NumberLength );
	registerProperty( "hint-font-style", "" ).setType( PropertyType::String );
	registerProperty( "hint-stroke-width", "" )
		.setType( PropertyType::NumberLength )
		.addAlias( "hintoutlinethickness" );
	registerProperty( "hint-stroke-color", "" ).setType( PropertyType::Color );
	registerProperty( "hint-font-family", "" ).addAlias( "hint-font-name" );

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

	// Shorthands
	registerShorthand( "margin", {"margin-top", "margin-right", "margin-bottom", "margin-left"},
					   "box" );
	registerShorthand( "layout-margin",
					   {"margin-top", "margin-right", "margin-bottom", "margin-left"}, "box" );
	registerShorthand( "layout_margin",
					   {"margin-top", "margin-right", "margin-bottom", "margin-left"}, "box" );
	registerShorthand( "padding",
					   {"padding-top", "padding-right", "padding-bottom", "padding-left"}, "box" );
	registerShorthand(
		"background",
		{"background-color", "background-image", "background-repeat", "background-position"},
		"background" );
	registerShorthand(
		"foreground",
		{"foreground-color", "foreground-image", "foreground-repeat", "foreground-position"},
		"background" );
	registerShorthand( "box-margin", {"column-margin", "row-margin"}, "single-value-vector" );
	registerShorthand( "background-position", {"background-position-x", "background-position-y"},
					   "background-position" );
	registerShorthand( "foreground-position", {"foreground-position-x", "foreground-position-y"},
					   "background-position" );
	registerShorthand(
		"border-color",
		{"border-top-color", "border-right-color", "border-bottom-color", "border-left-color"},
		"border-box" );
	registerShorthand(
		"border-width",
		{"border-top-width", "border-right-width", "border-bottom-width", "border-left-width"},
		"border-box" );
	registerShorthand( "border-radius",
					   {"border-top-left-radius", "border-top-right-radius",
						"border-bottom-right-radius", "border-bottom-left-radius"},
					   "radius" );
	registerShorthand( "rotation-origin-point",
					   {"rotation-origin-point-x", "rotation-origin-point-y"}, "vector2" );
	registerShorthand( "scale-origin-point", {"scale-origin-point-x", "scale-origin-point-y"},
					   "vector2" );
}

void StyleSheetSpecification::registerNodeSelector( const std::string& name,
													StyleSheetNodeSelector nodeSelector ) {
	mNodeSelectors[String::toLower( name )] = nodeSelector;
}

static bool isNth( int a, int b, int count ) {
	int x = count;
	x -= b;
	if ( a != 0 )
		x /= a;
	return ( x >= 0 && x * a + b == count );
}

void StyleSheetSpecification::registerDefaultNodeSelectors() {
	mNodeSelectors["empty"] = []( const UIWidget* node, int a, int b,
								  FunctionString data ) -> bool {
		return node->getFirstChild() == NULL;
	};
	mNodeSelectors["first-child"] = []( const UIWidget* node, int a, int b,
										FunctionString data ) -> bool {
		return NULL != node->getParent() && node->getParent()->getFirstChild() == node;
	};
	mNodeSelectors["enabled"] = []( const UIWidget* node, int a, int b,
									FunctionString data ) -> bool { return node->isEnabled(); };
	mNodeSelectors["disabled"] = []( const UIWidget* node, int a, int b,
									 FunctionString data ) -> bool { return !node->isEnabled(); };
	mNodeSelectors["first-of-type"] = []( const UIWidget* node, int a, int b,
										  FunctionString data ) -> bool {
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
	mNodeSelectors["last-child"] = []( const UIWidget* node, int a, int b,
									   FunctionString data ) -> bool {
		return NULL != node->getParent() && node->getParent()->getLastChild() == node;
	};
	mNodeSelectors["last-of-type"] = []( const UIWidget* node, int a, int b,
										 FunctionString data ) -> bool {
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
	mNodeSelectors["only-child"] = []( const UIWidget* node, int a, int b,
									   FunctionString data ) -> bool {
		return NULL != node->getParent() && node->getParent()->getChildCount() == 1;
	};
	mNodeSelectors["only-of-type"] = []( const UIWidget* node, int a, int b,
										 FunctionString data ) -> bool {
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
									  FunctionString data ) -> bool {
		return isNth( a, b, node->getNodeIndex() + 1 );
	};
	mNodeSelectors["nth-last-child"] = []( const UIWidget* node, int a, int b,
										   FunctionString data ) -> bool {
		return isNth( a, b, node->getChildCount() - node->getNodeIndex() );
	};
	mNodeSelectors["nth-of-type"] = []( const UIWidget* node, int a, int b,
										FunctionString data ) -> bool {
		return isNth( a, b, node->getNodeOfTypeIndex() + 1 );
	};
	mNodeSelectors["nth-last-of-type"] = []( const UIWidget* node, int a, int b,
											 FunctionString data ) -> bool {
		return node->getParent() != NULL
				   ? isNth( a, b,
							node->getParent()->getChildOfTypeCount( node->getType() ) -
								node->getNodeOfTypeIndex() )
				   : false;
	};
	mNodeSelectors["checked"] = []( const UIWidget* node, int a, int b,
									FunctionString data ) -> bool {
		return 0 != ( node->getFlags() & UI_CHECKED );
	};
	mNodeSelectors["not"] = []( const UIWidget* node, int a, int b, FunctionString data ) -> bool {
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

void StyleSheetSpecification::registerDefaultShorthandParsers() {
	mShorthandParsers["empty"] = []( const ShorthandDefinition* shorthand,
									 std::string value ) -> std::vector<StyleSheetProperty> {
		return {};
	};

	mShorthandParsers["box"] = []( const ShorthandDefinition* shorthand,
								   std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
		if ( propNames.size() != 4 ) {
			eePRINTL( "ShorthandType::Box properties must be 4 for %s",
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
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
		if ( propNames.size() != 2 ) {
			eePRINTL( "ShorthandType::Vector2 properties must be 2 for %s",
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
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
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
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
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
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
		std::vector<std::string> values = String::split( value, ',' );
		std::map<std::string, std::vector<std::string>> tmpProperties;

		for ( auto& val : values ) {
			std::vector<std::string> pos = String::split( val, ' ' );

			if ( pos.size() == 1 )
				pos.push_back( "center" );

			if ( pos.size() == 2 ) {
				int xFloatIndex = 0;
				int yFloatIndex = 1;

				if ( "bottom" == pos[0] || "top" == pos[0] ) {
					xFloatIndex = 1;
					yFloatIndex = 0;
				}

				tmpProperties[propNames[0]].emplace_back( pos[xFloatIndex] );
				tmpProperties[propNames[1]].emplace_back( pos[yFloatIndex] );
			} else if ( pos.size() > 2 ) {
				if ( pos.size() == 3 ) {
					pos.push_back( "0dp" );
				}

				int xFloatIndex = 0;
				int yFloatIndex = 2;

				if ( "bottom" == pos[0] || "top" == pos[0] ) {
					xFloatIndex = 2;
					yFloatIndex = 0;
				}

				tmpProperties[propNames[0]].emplace_back( pos[xFloatIndex] + " " +
														  pos[xFloatIndex + 1] );
				tmpProperties[propNames[1]].emplace_back( pos[yFloatIndex] + " " +
														  pos[yFloatIndex + 1] );
			}
		}

		for ( auto& props : tmpProperties ) {
			properties.push_back(
				StyleSheetProperty( props.first, String::join( props.second, ',' ) ) );
		}
		return properties;
	};

	mShorthandParsers["background"] = [&]( const ShorthandDefinition* shorthand,
										   std::string value ) -> std::vector<StyleSheetProperty> {
		value = String::trim( value );
		if ( "none" == value )
			return {};
		std::vector<StyleSheetProperty> properties;
		const std::vector<std::string> propNames( shorthand->getProperties() );
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
				if ( NULL != shorthand )
					mShorthandParsers["background-position"]( shorthand, positionStr );
			}
		}

		return properties;
	};
}

}}} // namespace EE::UI::CSS
