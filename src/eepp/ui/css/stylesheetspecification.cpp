#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI { namespace CSS {

SINGLETON_DECLARE_IMPLEMENTATION( StyleSheetSpecification )

StyleSheetSpecification::StyleSheetSpecification() {
	// TODO: Support border-color top right bottom left.
	// TODO: Support border-radius top right bottom left.
	// TODO: Add support to animations (@keyframes).
	// TODO: Support box-sizing or something similar.
	// TODO: Add correct "background" and "foreground" shorthand.
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
	registerProperty( "id", "", false ).setType( PropertyType::String );
	registerProperty( "class", "", false ).setType( PropertyType::String );
	registerProperty( "x", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "y", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "width", "", false )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "height", "", false )
		.setType( PropertyType::NumberLength )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "background-color", "", false ).setType( PropertyType::Color );
	registerProperty( "background-image", "none", false ).setIndexed();
	registerProperty( "background-position-x", "center", false )
		.setRelativeTarget( PropertyRelativeTarget::BackgroundWidth )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( "background-position-y", "center", false )
		.setRelativeTarget( PropertyRelativeTarget::BackgroundHeight )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( "background-repeat", "no-repeat", false ).setIndexed();
	registerProperty( "background-size", "auto", false )
		.setType( PropertyType::BackgroundSize )
		.setIndexed();
	registerProperty( "foreground-color", "", false ).setType( PropertyType::Color );
	registerProperty( "foreground-image", "none", false ).setIndexed();
	registerProperty( "foreground-position-x", "center", false )
		.setRelativeTarget( PropertyRelativeTarget::ForegroundWidth )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( "foreground-position-y", "center", false )
		.setRelativeTarget( PropertyRelativeTarget::ForegroundHeight )
		.setType( PropertyType::NumberLength )
		.setIndexed();
	registerProperty( "foreground-repeat", "no-repeat", false ).setIndexed();
	registerProperty( "foreground-size", "auto", false )
		.setType( PropertyType::ForegroundSize )
		.setIndexed();
	registerProperty( "foreground-radius", "0px", false ).setType( PropertyType::NumberLength );
	registerProperty( "border-color", "", false ).setType( PropertyType::Color );
	registerProperty( "border-width", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "border-radius", "0px", false ).setType( PropertyType::NumberLength );
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
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockHeight );
	registerProperty( "margin-left", "0px", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-left" )
		.addAlias( "layout_marginleft" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-right", "0px", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-right" )
		.addAlias( "layout_marginright" )
		.setRelativeTarget( PropertyRelativeTarget::ContainingBlockWidth );
	registerProperty( "margin-bottom", "0px", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "layout-margin-bottom" )
		.addAlias( "layout_marginbottom" )
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
	registerProperty( "font-family", "", false )
		.addAlias( "font-name" )
		.setType( PropertyType::String );
	registerProperty( "font-size", "", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "text-size" )
		.addAlias( "textsize" );
	registerProperty( "font-style", "", false )
		.addAlias( "text-style" )
		.addAlias( "text-decoration" );
	registerProperty( "text-stroke-width", "", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "fontoutlinethickness" );
	registerProperty( "text-stroke-color", "", false )
		.setType( PropertyType::Color )
		.addAlias( "fontoutlinecolor" );
	registerProperty( "text-selection", "", false ).setType( PropertyType::Bool );
	registerProperty( "text-align", "", false );
	registerProperty( "icon", "", false );
	registerProperty( "min-icon-size", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "icon-horizontal-margin", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "icon-auto-margin", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "src", "", false ).setType( PropertyType::String );
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
	registerProperty( "line-below-tabs-y-offset", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "tab-separation", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "selected", "", false ).setType( PropertyType::Bool ).addAlias( "active" );
	registerProperty( "popup-to-main-control", "", false ).setType( PropertyType::Bool );
	registerProperty( "max-visible-items", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "selected-index", "", false );
	registerProperty( "selected-text", "", false );
	registerProperty( "scrollbar-type", "", false );
	registerProperty( "row-height", "", false ).setType( PropertyType::NumberLength );
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
	registerProperty( "radius", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "outline-thickness", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "animation-speed", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "arc-start-angle", "", false ).setType( PropertyType::NumberFloat );
	registerProperty( "min-width", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "min-margin-right", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "min-icon-space", "", false ).setType( PropertyType::NumberLength );

	registerProperty( "total-steps", "", false ).setType( PropertyType::NumberInt );
	registerProperty( "vertical-expand", "", false ).setType( PropertyType::Bool );
	registerProperty( "display-percent", "", false ).setType( PropertyType::Bool );
	registerProperty( "filler-padding-left", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "filler-padding-top", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "filler-padding-right", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "filler-padding-bottom", "", false ).setType( PropertyType::NumberLength );
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
	registerProperty( "buttons-position-offset", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "window-flags", "", false ).addAlias( "winflags" );
	registerProperty( "decoration-size", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "border-size", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "min-window-size", "", false ).setType( PropertyType::Vector2 );
	registerProperty( "buttons-separation", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "min-corner-distance", "", false );
	registerProperty( "decoration-auto-size", "", false ).setType( PropertyType::Bool );
	registerProperty( "border-auto-size", "", false ).setType( PropertyType::Bool );

	registerProperty( "margin-between-buttons", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "button-margin", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "menu-height", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "first-button-margin-left", "", false ).setType( PropertyType::NumberLength );

	registerProperty( "scale-origin-point", "", false ).setType( PropertyType::Vector2 );

	registerProperty( "word-wrap", "", false ).setType( PropertyType::Bool );

	registerProperty( "hint", "", false ).setType( PropertyType::String );
	registerProperty( "hint-color", "", false ).setType( PropertyType::Color );
	registerProperty( "hint-shadow-color", "", false ).setType( PropertyType::Color );
	registerProperty( "hint-font-size", "", false ).setType( PropertyType::NumberLength );
	registerProperty( "hint-font-style", "", false ).setType( PropertyType::String );
	registerProperty( "hint-stroke-width", "", false )
		.setType( PropertyType::NumberLength )
		.addAlias( "hintoutlinethickness" );
	registerProperty( "hint-stroke-color", "", false ).setType( PropertyType::Color );
	registerProperty( "hint-font-family", "", false ).addAlias( "hint-font-name" );

	registerProperty( "transition", "", false );
	registerProperty( "transition-duration", "", false ).addAlias( "transitionduration" );
	registerProperty( "transition-delay", "", false ).addAlias( "transitiondelay" );
	registerProperty( "transition-timing-function", "", false )
		.addAlias( "transitiontimingfunction" );
	registerProperty( "transition-property", "", false ).addAlias( "transitionproperty" );

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
