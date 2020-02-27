#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI { namespace CSS {

SINGLETON_DECLARE_IMPLEMENTATION( StyleSheetSpecification )

StyleSheetSpecification::StyleSheetSpecification() {
	// TODO: Add correct "background" and "foreground" shorthand.
	// TODO: Support border-color top right bottom left.
	// TODO: Support border-radius top right bottom left.
	// TODO: Create a rule to set the border position against its box.
	//		 Something like: "border-box", with the following options:
	//			inside: The border is drawn inside the box.
	//			outside: The border is drawn outside the box.
	//			over: The border is drawn in the middle point of inside and outside.
	registerDefaultProperties();
	registerDefaultNodeSelectors();
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
	registerProperty( "border-color", "" ).setType( PropertyType::Color );
	registerProperty( "border-width", "" ).setType( PropertyType::NumberLength );
	registerProperty( "border-radius", "0px" ).setType( PropertyType::NumberLength );
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
	registerProperty( "selected-color", "" ).setType( PropertyType::Color );
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
	registerProperty( "icon-horizontal-margin", "" ).setType( PropertyType::NumberLength );
	registerProperty( "icon-auto-margin", "" ).setType( PropertyType::NumberLength );
	registerProperty( "src", "" ).setType( PropertyType::String );
	registerProperty( "scale-type", "" );
	registerProperty( "tint", "" ).setType( PropertyType::Color );
	registerProperty( "rotation-origin-point", "" ).setType( PropertyType::Vector2 );
	registerProperty( "max-text-length", "" ).setType( PropertyType::NumberInt );
	registerProperty( "min-tab-width", "" ).setType( PropertyType::NumberLength );
	registerProperty( "max-tab-width", "" ).setType( PropertyType::NumberLength );
	registerProperty( "tab-closable", "" ).setType( PropertyType::Bool );
	registerProperty( "special-border-tabs", "" ).setType( PropertyType::Bool );
	registerProperty( "line-below-tabs", "" ).setType( PropertyType::Bool );
	registerProperty( "line-below-tabs-color", "" ).setType( PropertyType::Color );
	registerProperty( "line-below-tabs-y-offset", "" ).setType( PropertyType::NumberLength );
	registerProperty( "tab-separation", "" ).setType( PropertyType::NumberLength );
	registerProperty( "selected", "" ).setType( PropertyType::Bool ).addAlias( "active" );
	registerProperty( "popup-to-main-control", "" ).setType( PropertyType::Bool );
	registerProperty( "max-visible-items", "" ).setType( PropertyType::NumberInt );
	registerProperty( "selected-index", "" );
	registerProperty( "selected-text", "" );
	registerProperty( "scrollbar-type", "" );
	registerProperty( "row-height", "" ).setType( PropertyType::NumberLength );
	registerProperty( "vscroll-mode", "" );
	registerProperty( "hscroll-mode", "" );

	registerProperty( "column-span", "" ).setType( PropertyType::NumberLength );
	registerProperty( "row-span", "" ).setType( PropertyType::NumberLength );
	registerProperty( "span", "" ).setType( PropertyType::NumberLength );
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
	registerProperty( "animation-speed", "" ).setType( PropertyType::Vector2 );
	registerProperty( "arc-start-angle", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "min-width", "" ).setType( PropertyType::NumberLength );
	registerProperty( "min-margin-right", "" ).setType( PropertyType::NumberLength );
	registerProperty( "min-icon-space", "" ).setType( PropertyType::NumberLength );

	registerProperty( "total-steps", "" ).setType( PropertyType::NumberInt );
	registerProperty( "vertical-expand", "" ).setType( PropertyType::Bool );
	registerProperty( "display-percent", "" ).setType( PropertyType::Bool );
	registerProperty( "filler-padding-left", "" ).setType( PropertyType::NumberLength );
	registerProperty( "filler-padding-top", "" ).setType( PropertyType::NumberLength );
	registerProperty( "filler-padding-right", "" ).setType( PropertyType::NumberLength );
	registerProperty( "filler-padding-bottom", "" ).setType( PropertyType::NumberLength );
	registerProperty( "movement-speed", "" ).setType( PropertyType::Vector2 );
	registerProperty( "min-value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "max-value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "value", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "click-step", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "page-step", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "background-expand", "" ).setType( PropertyType::Bool );
	registerProperty( "scrollbar-mode", "" );
	registerProperty( "half-slider", "" ).setType( PropertyType::Bool );
	registerProperty( "name", "" ).setType( PropertyType::String );
	registerProperty( "owns", "" ).setType( PropertyType::String );
	registerProperty( "allow-editing", "" ).setType( PropertyType::Bool );
	registerProperty( "max-length", "" ).setType( PropertyType::NumberInt );
	registerProperty( "free-editing", "" ).setType( PropertyType::Bool );
	registerProperty( "only-numbers", "" ).setType( PropertyType::Bool );
	registerProperty( "allow-dot", "" ).setType( PropertyType::Bool );
	registerProperty( "touch-drag", "" ).setType( PropertyType::Bool );
	registerProperty( "touch-drag-deceleration", "" ).setType( PropertyType::NumberFloat );

	registerProperty( "base-alpha", "" ).setType( PropertyType::NumberFloat );
	registerProperty( "buttons-position-offset", "" ).setType( PropertyType::NumberLength );
	registerProperty( "window-flags", "" ).addAlias( "winflags" );
	registerProperty( "decoration-size", "" ).setType( PropertyType::Vector2 );
	registerProperty( "border-size", "" ).setType( PropertyType::Vector2 );
	registerProperty( "min-window-size", "" ).setType( PropertyType::Vector2 );
	registerProperty( "buttons-separation", "" ).setType( PropertyType::NumberLength );
	registerProperty( "min-corner-distance", "" );
	registerProperty( "decoration-auto-size", "" ).setType( PropertyType::Bool );
	registerProperty( "border-auto-size", "" ).setType( PropertyType::Bool );

	registerProperty( "margin-between-buttons", "" ).setType( PropertyType::NumberLength );
	registerProperty( "button-margin", "" ).setType( PropertyType::NumberLength );
	registerProperty( "menu-height", "" ).setType( PropertyType::NumberLength );
	registerProperty( "first-button-margin-left", "" ).setType( PropertyType::NumberLength );

	registerProperty( "scale-origin-point", "" ).setType( PropertyType::Vector2 );

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
	registerProperty( "transition-duration", "" ).addAlias( "transitionduration" );
	registerProperty( "transition-delay", "0s" )
		.setType( PropertyType::Time )
		.addAlias( "transitiondelay" );
	registerProperty( "transition-timing-function", "linear" )
		.addAlias( "transitiontimingfunction" );
	registerProperty( "transition-property", "" ).addAlias( "transitionproperty" );

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


	// Shorthands
	registerShorthand( "margin", {"margin-top", "margin-right", "margin-bottom", "margin-left"},
					   ShorthandType::Box );
	registerShorthand( "layout-margin",
					   {"margin-top", "margin-right", "margin-bottom", "margin-left"},
					   ShorthandType::Box );
	registerShorthand( "layout_margin",
					   {"margin-top", "margin-right", "margin-bottom", "margin-left"},
					   ShorthandType::Box );
	registerShorthand( "padding",
					   {"padding-top", "padding-right", "padding-bottom", "padding-left"},
					   ShorthandType::Box );
	registerShorthand( "background", {"background-color", "background-image"},
					   ShorthandType::Background );
	registerShorthand( "foreground", {"foreground-color", "foreground-image"},
					   ShorthandType::Background );
	registerShorthand( "filler-padding",
					   {"filler-padding-top", "filler-padding-right", "filler-padding-bottom",
						"filler-padding-left"},
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

}}} // namespace EE::UI::CSS
