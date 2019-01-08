#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/fontmanager.hpp>


namespace EE { namespace UI {

UIStyle * EE::UI::UIStyle::New( UIWidget * widget ) {
	return eeNew( UIStyle, ( widget ) );
}

UIStyle::UIStyle( UIWidget * widget ) :
	mWidget( widget )
{
	load();
}

UIStyle::~UIStyle()
{}

bool UIStyle::stateExists( const EE::Uint32 & state  ) const {
	return mStates.find( state ) != mStates.end();
}

void UIStyle::addAttribute( int state, NodeAttribute attribute ) {
	mStates[ state ][ attribute.getName() ] = attribute;

	if ( String::startsWith( attribute.getName(), "transition" ) ) {
		mTransitionAttributes[ state ].push_back( attribute );

		parseTransitions( state );
	}
}

void UIStyle::load() {
	mStates.clear();
	mTransitions.clear();
	mTransitionAttributes.clear();

	UISceneNode * uiSceneNode = mWidget->getSceneNode()->isUISceneNode() ? static_cast<UISceneNode*>( mWidget->getSceneNode() ) : NULL;

	if ( NULL != uiSceneNode ) {
		CSS::StyleSheet& styleSheet = uiSceneNode->getStyleSheet();

		if ( !styleSheet.isEmpty() ) {
			CSS::StyleSheet::StyleSheetPseudoClassProperties propertiesByPseudoClass = styleSheet.getElementProperties( mWidget );

			if ( !propertiesByPseudoClass.empty() ) {
				Uint32 stateFlag;

				for ( auto it = propertiesByPseudoClass.begin(); it != propertiesByPseudoClass.end(); ++it ) {
					stateFlag = getStateFlagFromName( it->first );

					if ( eeINDEX_NOT_FOUND != stateFlag )
						addStyleSheetProperties( stateFlag, it->second );
				}
			}
		}
	}
}

void UIStyle::addStyleSheetProperties(const Uint32 & state, const CSS::StyleSheetProperties& properties ) {
	if ( !properties.empty() ) {
		for ( auto it = properties.begin(); it != properties.end(); ++it ) {
			CSS::StyleSheetProperty property = it->second;

			addAttribute( state, NodeAttribute( property.getName(), property.getValue() ) );
		}
	}
}

void UIStyle::addStyleSheetProperty( const Uint32& state, const CSS::StyleSheetProperty& property ) {
	addAttribute( state, NodeAttribute( property.getName(), property.getValue() ) );
}

bool UIStyle::hasTransition( const Uint32& state, const std::string& propertyName ) {
	bool ret = mTransitions.find( state ) != mTransitions.end() &&
			( mTransitions[ state ].find( propertyName ) != mTransitions[ state ].end() ||
			  mTransitions[ state ].find( "all" ) != mTransitions[ state ].end()
			);

	// When transitions are declared without state are global
	if ( !ret && state != StateFlagNormal ) {
		ret = mTransitions.find( StateFlagNormal ) != mTransitions.end() && (
			  mTransitions[ StateFlagNormal ].find( propertyName ) != mTransitions[ StateFlagNormal ].end() ||
			  mTransitions[ StateFlagNormal ].find( "all" ) != mTransitions[ StateFlagNormal ].end()
		);
	}

	return ret;
}

UIStyle::TransitionInfo UIStyle::getTransition( const Uint32& state, const std::string& propertyName ) {
	if ( mTransitions.find( state ) != mTransitions.end() ) {
		auto propertyTransitionIt = mTransitions[ state ].find( propertyName );

		if ( propertyTransitionIt != mTransitions[ state ].end() ) {
			return propertyTransitionIt->second;
		} else if ( ( propertyTransitionIt = mTransitions[ state ].find( "all" ) ) != mTransitions[ state ].end() ) {
			return propertyTransitionIt->second;
		} else if ( mTransitions.find( StateFlagNormal ) != mTransitions.end() ) {
			propertyTransitionIt = mTransitions[ StateFlagNormal ].find( propertyName );

			if ( propertyTransitionIt != mTransitions[ StateFlagNormal ].end() ) {
				return propertyTransitionIt->second;
			} else if ( ( propertyTransitionIt = mTransitions[ StateFlagNormal ].find( "all" ) ) != mTransitions[ state ].end() ) {
				return propertyTransitionIt->second;
			}
		}
	} else if ( mTransitions.find( StateFlagNormal ) != mTransitions.end() ) {
		auto propertyTransitionIt = mTransitions[ StateFlagNormal ].find( propertyName );

		if ( propertyTransitionIt != mTransitions[ StateFlagNormal ].end() ) {
			return propertyTransitionIt->second;
		} else if ( ( propertyTransitionIt = mTransitions[ StateFlagNormal ].find( "all" ) ) != mTransitions[ state ].end() ) {
			return propertyTransitionIt->second;
		}
	}

	return TransitionInfo();
}

void UIStyle::onStateChange() {
	if ( NULL != mWidget && stateExists( mCurrentState ) ) {
		AttributesMap& attrs = mStates[ mCurrentState ];

		if ( !attrs.empty() ) {
			mWidget->beginAttributesTransaction();

			for ( auto it = attrs.begin(); it != attrs.end(); ++it ) {
				NodeAttribute& nodeAttr = it->second;

				mWidget->setAttribute( nodeAttr, mCurrentState );
			}

			mWidget->endAttributesTransaction();
		}
	}
}


NodeAttribute UIStyle::getAttribute( const Uint32& state, const std::string& attributeName ) const {
	if ( !attributeName.empty() && stateExists( state ) ) {
		const AttributesMap& attributesMap = mStates.at( state );

		auto attributeFound = attributesMap.find( attributeName );

		if ( attributeFound != attributesMap.end() ) {
			return attributeFound->second;
		}
	}

	return NodeAttribute();
}

bool UIStyle::hasAttribute(const Uint32 & state, const std::string & attributeName) const {
	if ( !attributeName.empty() && stateExists( state ) ) {
		const AttributesMap& attributesMap = mStates.at( state );
		return attributesMap.find( attributeName ) != attributesMap.end();
	}

	return false;
}

NodeAttribute UIStyle::getAttributeFromNames( const Uint32& state, const std::vector<std::string>& attributeNames ) const {
	if ( !attributeNames.empty() && stateExists( state ) ) {
		const AttributesMap& attributesMap = mStates.at( state );

		for ( size_t i = 0; i < attributeNames.size(); i++ ) {
			const std::string& name = attributeNames[i];
			auto attributeFound = attributesMap.find( name );

			if ( attributeFound != attributesMap.end() ) {
				return attributeFound->second;
			}

		}
	}

	return NodeAttribute();
}

Font * UIStyle::getFontFamily( const Uint32& state ) const {
	NodeAttribute attribute = getAttributeFromNames( state, { "fontfamily", "fontname" } );

	if ( !attribute.isEmpty() ) {
		return FontManager::instance()->getByName( attribute.asString() );
	}

	return UIThemeManager::instance()->getDefaultFont();
}

int UIStyle::getFontCharacterSize(const Uint32& state, const int& defaultValue ) const {
	NodeAttribute attribute = getAttributeFromNames( state, { "textsize", "fontsize", "charactersize" } );

	if ( !attribute.isEmpty() ) {
		return attribute.asDpDimensionI();
	}

	return defaultValue;
}

Color UIStyle::getTextColor( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, "textcolor" );

	return attribute.isEmpty() ? Color::White : attribute.asColor();
}

Color UIStyle::getTextShadowColor( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, "textshadowcolor" );

	return attribute.isEmpty() ? Color::Black : attribute.asColor();
}

Uint32 UIStyle::getTextStyle( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, "textstyle" );

	return attribute.isEmpty() ? 0 : attribute.asFontStyle();
}

Float UIStyle::getFontOutlineThickness( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, "fontoutlinethickness" );

	return attribute.isEmpty() ? 0.f : attribute.asFloat();
}

Color UIStyle::getFontOutlineColor( const Uint32& state ) const {
	NodeAttribute attribute = getAttribute( state, "fontoutlinecolor" );

	return attribute.isEmpty() ? Color::White : attribute.asColor();
}

FontStyleConfig UIStyle::getFontStyleConfig( const Uint32& state ) const {
	FontStyleConfig fontStyleConfig;
	fontStyleConfig.Font = getFontFamily( state );
	fontStyleConfig.CharacterSize = getFontCharacterSize( state );
	fontStyleConfig.Style = getTextStyle( state );
	fontStyleConfig.FontColor = getTextColor( state );
	fontStyleConfig.ShadowColor = getTextShadowColor( state );
	fontStyleConfig.OutlineColor = getFontOutlineColor( state );
	fontStyleConfig.OutlineThickness = getFontOutlineThickness( state );
	return fontStyleConfig;
}

void UIStyle::updateState() {
	for ( int i = StateFlagCount - 1; i >= 0; i-- ) {
		if ( ( mState & getStateFlag(i) ) == getStateFlag(i) ) {
			if ( stateExists( getStateFlag(i) ) ) {
				if ( mCurrentState != getStateFlag(i) ) {
					mPreviousState = mCurrentState;
					mCurrentState = getStateFlag(i);
					onStateChange();
				}

				return;
			}
		}
	}

	Uint32 currentState = mCurrentState;

	mCurrentState = StateFlagNormal;

	if ( currentState != StateFlagNormal ) {
		onStateChange();
	}
}

void UIStyle::parseTransitions( const Uint32& state ) {
	std::vector<std::string> properties;
	std::vector<Time> durations;
	std::vector<Time> delays;
	std::vector<Ease::Interpolation> timingFunctions;
	TransitionsMap transitions;

	if ( mTransitionAttributes.find( state ) == mTransitionAttributes.end() )
		return;

	auto transitionAttributes = mTransitionAttributes[ state ];

	for ( auto it = transitionAttributes.begin(); it != transitionAttributes.end(); ++it ) {
		NodeAttribute& attr = *it;
		if ( attr.getName() == "transition" ) {
			auto strTransitions = String::split( attr.getValue(), ',' );

			for ( auto tit = strTransitions.begin(); tit != strTransitions.end(); ++tit ) {
				auto strTransition = String::trim( *tit );
				auto splitTransition = String::split( strTransition, ' ' );

				if ( !splitTransition.empty() ) {
					TransitionInfo transitionInfo;

					if ( splitTransition.size() >= 2 ) {
						std::string property  = String::trim( splitTransition[0] );
						String::toLowerInPlace( property );

						Time duration = NodeAttribute( attr.getName(), String::toLower( splitTransition[1] ) ).asTime();

						transitionInfo.property = property;
						transitionInfo.duration = duration;

						if ( splitTransition.size() >= 3 ) {
							transitionInfo.timingFunction = Ease::fromName( String::toLower( splitTransition[2] ) );

							if (  transitionInfo.timingFunction == Ease::Linear && splitTransition[2] != "linear" && splitTransition.size() == 3 ) {
								transitionInfo.delay = NodeAttribute( attr.getName(), String::toLower( splitTransition[2] ) ).asTime();
							} else if ( splitTransition.size() >= 4 ) {
								transitionInfo.delay = NodeAttribute( attr.getName(), String::toLower( splitTransition[3] ) ).asTime();
							}
						}

						transitions[ transitionInfo.getProperty() ] = transitionInfo;
					}
				}
			}
		} else if ( attr.getName() == "transitionduration" || attr.getName() == "transition-duration" ) {
			auto strDurations = String::split( attr.getValue(), ',' );

			for ( auto dit = strDurations.begin(); dit != strDurations.end(); ++dit ) {
				std::string duration( String::trim( *dit ) );
				String::toLowerInPlace( duration );
				durations.push_back( NodeAttribute( attr.getName(), duration ).asTime() );
			}
		} else if ( attr.getName() == "transitiondelay" || attr.getName() == "transition-delay" ) {
			auto strDelays = String::split( attr.getValue(), ',' );

			for ( auto dit = strDelays.begin(); dit != strDelays.end(); ++dit ) {
				std::string delay( String::trim( *dit ) );
				String::toLowerInPlace( delay );
				delays.push_back( NodeAttribute( attr.getName(), delay ).asTime() );
			}
		} else if ( attr.getName() == "transitiontimingfunction" || attr.getName() == "transition-timing-function" ) {
			auto strTimingFuncs = String::split( attr.getValue(), ',' );

			for ( auto dit = strTimingFuncs.begin(); dit != strTimingFuncs.end(); ++dit ) {
				std::string timingFunction( String::trim( *dit ) );
				String::toLowerInPlace( timingFunction );
				timingFunctions.push_back( Ease::fromName( timingFunction ) );
			}
		} else if ( attr.getName() == "transitionproperty" || attr.getName() == "transition-property" ) {
			auto strProperties = String::split( attr.getValue(), ',' );

			for ( auto dit = strProperties.begin(); dit != strProperties.end(); ++dit ) {
				std::string property( String::trim( *dit ) );
				String::toLowerInPlace( property );
				properties.push_back( property );
			}
		}
	}

	if ( properties.empty() ) {
		if ( !transitions.empty() )
			mTransitions[ state ] = transitions;

		return;
	}

	for ( size_t i = 0; i < properties.size(); i++ ) {
		const std::string& property = properties.at( i );
		TransitionInfo transitionInfo;

		transitionInfo.property = property;

		if ( durations.size() < i ) {
			transitionInfo.duration = durations[i];
		} else if ( !durations.empty() ) {
			transitionInfo.duration = durations[0];
		}

		if ( delays.size() < i ) {
			transitionInfo.delay = delays[i];
		} else if ( !delays.empty() ) {
			transitionInfo.delay = delays[0];
		}

		if ( timingFunctions.size() < i ) {
			transitionInfo.timingFunction = timingFunctions[i];
		} else if ( !delays.empty() ) {
			transitionInfo.timingFunction = timingFunctions[0];
		}

		mTransitions[ state ][ property ] = transitionInfo;
	}
}

}}
