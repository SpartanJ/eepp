#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/fontmanager.hpp>

using namespace EE::UI::CSS;

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

void UIStyle::addStyleSheetProperty( const Uint32& state, const StyleSheetProperty& attribute ) {
	if ( attribute.getName() == "padding" ) {
		Rectf rect(  NodeAttribute( attribute.getName(), attribute.getValue() ).asRectf() );
		mStates[ state ][ "paddingleft" ] = StyleSheetProperty( "paddingleft", String::toStr( rect.Left ), attribute.getSpecificity() );
		mStates[ state ][ "paddingright" ] = StyleSheetProperty( "paddingright", String::toStr( rect.Right ), attribute.getSpecificity() );
		mStates[ state ][ "paddingtop" ] = StyleSheetProperty( "paddingtop", String::toStr( rect.Top ), attribute.getSpecificity() );
		mStates[ state ][ "paddingbottom" ] = StyleSheetProperty( "paddingbottom", String::toStr( rect.Bottom ), attribute.getSpecificity() );
	} else if ( attribute.getName() == "layout_margin" ) {
		Rect rect(  NodeAttribute( attribute.getName(), attribute.getValue() ).asRect() );
		mStates[ state ][ "layout_marginleft" ] = StyleSheetProperty( "layout_marginleft", String::toStr( rect.Left ), attribute.getSpecificity() );
		mStates[ state ][ "layout_marginright" ] = StyleSheetProperty( "layout_marginright", String::toStr( rect.Right ), attribute.getSpecificity() );
		mStates[ state ][ "layout_margintop" ] = StyleSheetProperty( "layout_margintop", String::toStr( rect.Top ), attribute.getSpecificity() );
		mStates[ state ][ "layout_marginbottom" ] = StyleSheetProperty( "layout_marginbottom", String::toStr( rect.Bottom ), attribute.getSpecificity() );
	} else {
		mStates[ state ][ attribute.getName() ] = attribute;
	}

	if ( String::startsWith( attribute.getName(), "transition" ) ) {
		mTransitionAttributes[ state ].push_back( attribute );

		parseTransitions( state );
	}
}

void UIStyle::load() {
	mStates.clear();
	mNoncacheableStyles.clear();
	mTransitions.clear();
	mTransitionAttributes.clear();

	UISceneNode * uiSceneNode = mWidget->getSceneNode()->isUISceneNode() ? static_cast<UISceneNode*>( mWidget->getSceneNode() ) : NULL;

	if ( NULL != uiSceneNode ) {
		CSS::StyleSheet& styleSheet = uiSceneNode->getStyleSheet();

		if ( !styleSheet.isEmpty() ) {
			CSS::StyleSheet::StyleSheetPseudoClassProperties propertiesByPseudoClass = styleSheet.getElementPropertiesByState( mWidget );

			if ( !propertiesByPseudoClass.empty() ) {
				Uint32 stateFlag;

				for ( auto it = propertiesByPseudoClass.begin(); it != propertiesByPseudoClass.end(); ++it ) {
					stateFlag = getStateFlagFromName( it->first );

					if ( eeINDEX_NOT_FOUND != stateFlag )
						addStyleSheetProperties( stateFlag, it->second );
				}
			}

			mNoncacheableStyles = styleSheet.getNoncacheableElementStyles( mWidget );
		}
	}
}

void UIStyle::addStyleSheetProperties(const Uint32 & state, const CSS::StyleSheetProperties& properties ) {
	if ( !properties.empty() ) {
		for ( auto it = properties.begin(); it != properties.end(); ++it ) {
			CSS::StyleSheetProperty property = it->second;

			addStyleSheetProperty( state, property );
		}
	}
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
	if ( NULL != mWidget ) {
		StyleSheetProperties properties;

		auto& props = mStates[ mCurrentState ];

		for ( auto& prop : props ) {
			auto& property = prop.second;
			auto it = properties.find( property.getName() );

			if ( it == properties.end() || property.getSpecificity() >= it->second.getSpecificity() ) {
				properties[ property.getName() ] = property;
			}
		}

		for ( auto& style : mNoncacheableStyles ) {
			if ( style.getSelector().select( mWidget ) ) {
				for ( auto& prop : style.getProperties() ) {
					auto& property = prop.second;
					auto it = properties.find( property.getName() );

					if ( it == properties.end() || property.getSpecificity() >= it->second.getSpecificity() ) {
						properties[ property.getName() ] = property;
					}
				}
			}
		}

		mWidget->beginAttributesTransaction();

		for ( auto& prop : properties ) {
			auto& property = prop.second;

			mWidget->setAttribute( property.getName(), property.getValue(), mCurrentState );
		}

		mWidget->endAttributesTransaction();
	}
}

StyleSheetProperty UIStyle::getStyleSheetProperty( const Uint32& state, const std::string& attributeName ) const {
	if ( !attributeName.empty() && stateExists( state ) ) {
		auto& attributesMap = mStates.at( state );

		auto attributeFound = attributesMap.find( attributeName );

		if ( attributeFound != attributesMap.end() ) {
			return attributeFound->second;
		}
	}

	return StyleSheetProperty();
}

StyleSheetProperty UIStyle::getStyleSheetPropertyFromNames( const Uint32& state, const std::vector<std::string>& propertiesNames ) const {
	if ( !propertiesNames.empty() && stateExists( state ) ) {
		auto& attributesMap = mStates.at( state );

		for ( size_t i = 0; i < propertiesNames.size(); i++ ) {
			const std::string& name = propertiesNames[i];
			auto attributeFound = attributesMap.find( name );

			if ( attributeFound != attributesMap.end() ) {
				return attributeFound->second;
			}

		}
	}

	return StyleSheetProperty();
}

NodeAttribute UIStyle::getNodeAttribute( const Uint32& state, const std::string& attributeName ) const {
	StyleSheetProperty property( getStyleSheetProperty( state, attributeName ) );
	return NodeAttribute( property.getName(), property.getValue() );
}

bool UIStyle::hasStyleSheetProperty( const Uint32 & state, const std::string& propertyName ) const {
	if ( !propertyName.empty() && stateExists( state ) ) {
		auto& attributesMap = mStates.at( state );
		return attributesMap.find( propertyName ) != attributesMap.end();
	}

	return false;
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

	for ( auto& attr : transitionAttributes ) {
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
