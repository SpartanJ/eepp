#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/fontmanager.hpp>

using namespace EE::UI::CSS;

namespace EE { namespace UI {

UIStyle * UIStyle::New( UIWidget * widget ) {
	return eeNew( UIStyle, ( widget ) );
}

UIStyle::UIStyle( UIWidget * widget ) :
	mWidget( widget )
{
	load();
}

UIStyle::~UIStyle() {
	removeRelatedWidgets();
	unsubscribeNonCacheableStyles();
}

bool UIStyle::stateExists( const EE::Uint32&  ) const {
	return true;
}

void UIStyle::addStyleSheetProperty( const StyleSheetProperty& attribute ) {
	if ( attribute.getName() == "padding" ) {
		Rectf rect(  NodeAttribute( attribute.getName(), attribute.getValue() ).asRectf() );
		mElementStyle.setProperty( StyleSheetProperty( "paddingleft", String::toStr( rect.Left ), attribute.getSpecificity() ) );
		mElementStyle.setProperty( StyleSheetProperty( "paddingright", String::toStr( rect.Right ), attribute.getSpecificity() ) );
		mElementStyle.setProperty( StyleSheetProperty( "paddingtop", String::toStr( rect.Top ), attribute.getSpecificity() ) );
		mElementStyle.setProperty( StyleSheetProperty( "paddingbottom", String::toStr( rect.Bottom ), attribute.getSpecificity() ) );
	} else if ( attribute.getName() == "layout_margin" ) {
		Rect rect(  NodeAttribute( attribute.getName(), attribute.getValue() ).asRect() );
		mElementStyle.setProperty( StyleSheetProperty( "layout_marginleft", String::toStr( rect.Left ), attribute.getSpecificity() ) );
		mElementStyle.setProperty( StyleSheetProperty( "layout_marginright", String::toStr( rect.Right ), attribute.getSpecificity() ) );
		mElementStyle.setProperty( StyleSheetProperty( "layout_margintop", String::toStr( rect.Top ), attribute.getSpecificity() ) );
		mElementStyle.setProperty( StyleSheetProperty( "layout_marginbottom", String::toStr( rect.Bottom ), attribute.getSpecificity() ) );
	} else {
		mElementStyle.setProperty( attribute );
	}
}

void UIStyle::load() {
	unsubscribeNonCacheableStyles();

	mCacheableStyles.clear();
	mNoncacheableStyles.clear();

	UISceneNode * uiSceneNode = mWidget->getSceneNode()->isUISceneNode() ? static_cast<UISceneNode*>( mWidget->getSceneNode() ) : NULL;

	if ( NULL != uiSceneNode ) {
		CSS::StyleSheet& styleSheet = uiSceneNode->getStyleSheet();

		if ( !styleSheet.isEmpty() ) {
			StyleSheetStyleVector styles = styleSheet.getElementStyles( mWidget );

			for ( auto& style : styles ) {
				const StyleSheetSelector& selector = style.getSelector();

				if ( selector.isCacheable() ) {
					mCacheableStyles.push_back( style );
				} else {
					mNoncacheableStyles.push_back( style );
				}
			}

			subscribeNonCacheableStyles();
		}
	}
}

void UIStyle::addStyleSheetProperties( const CSS::StyleSheetProperties& properties ) {
	if ( !properties.empty() ) {
		for ( const auto& it : properties ) {
			addStyleSheetProperty( it.second );
		}
	}
}

bool UIStyle::hasTransition( const std::string& propertyName ) {
	return mTransitions.find( propertyName ) != mTransitions.end() || mTransitions.find( "all" ) != mTransitions.end();
}

UIStyle::TransitionInfo UIStyle::getTransition( const std::string& propertyName ) {
	auto propertyTransitionIt = mTransitions.find( propertyName );

	if ( propertyTransitionIt != mTransitions.end() ) {
		return propertyTransitionIt->second;
	} else if ( ( propertyTransitionIt = mTransitions.find( "all" ) ) != mTransitions.end() ) {
		return propertyTransitionIt->second;
	}

	return TransitionInfo();
}

void UIStyle::subscribeRelated( UIWidget * widget ) {
	mRelatedWidgets.insert( widget );
}

void UIStyle::unsubscribeRelated( UIWidget * widget ) {
	mRelatedWidgets.erase( widget );
}

void UIStyle::tryApplyStyle( const StyleSheetStyle& style ) {
	if ( style.getSelector().select( mWidget ) ) {
		for ( const auto& prop : style.getProperties() ) {
			const StyleSheetProperty& property = prop.second;
			const auto& it = mProperties.find( property.getName() );

			if ( it == mProperties.end() || property.getSpecificity() >= it->second.getSpecificity() ) {
				mProperties[ property.getName() ] = property;

				if ( String::startsWith( property.getName(), "transition" ) )
					mTransitionAttributes.push_back( property );
			}
		}
	}
}

void UIStyle::onStateChange() {
	if ( NULL != mWidget ) {
		mProperties.clear();
		mTransitionAttributes.clear();

		tryApplyStyle( mElementStyle );

		for ( auto& style : mCacheableStyles ) {
			tryApplyStyle( style );
		}

		for ( auto& style : mNoncacheableStyles ) {
			tryApplyStyle( style );
		}

		parseTransitions();

		mWidget->beginAttributesTransaction();

		for ( const auto& prop : mProperties ) {
			const StyleSheetProperty& property = prop.second;

			mWidget->setAttribute( property.getName(), property.getValue(), mCurrentState );
		}

		mWidget->endAttributesTransaction();

		for ( auto& related : mRelatedWidgets ) {
			if ( NULL != related->getUIStyle() ) {
				related->getUIStyle()->onStateChange();
			}
		}
	}
}

StyleSheetProperty UIStyle::getStatelessStyleSheetProperty( const std::string& propertyName ) const {
	if ( !propertyName.empty() ) {
		if  ( !mElementStyle.getSelector().hasPseudoClasses() ) {
			StyleSheetProperty property = mElementStyle.getPropertyByName( propertyName );

			if ( !property.isEmpty() )
				return property;
		}

		for ( const StyleSheetStyle& style : mCacheableStyles ) {
			if  ( !style.getSelector().hasPseudoClasses() ) {
				StyleSheetProperty property = style.getPropertyByName( propertyName );

				if ( !property.isEmpty() )
					return property;
			}
		}
	}

	return StyleSheetProperty();
}

StyleSheetProperty UIStyle::getStyleSheetProperty( const std::string& propertyName ) const {
	auto propertyIt = mProperties.find( propertyName );

	if ( propertyIt != mProperties.end() )
		return propertyIt->second;

	return StyleSheetProperty();
}

NodeAttribute UIStyle::getNodeAttribute( const std::string& attributeName ) const {
	StyleSheetProperty property( getStyleSheetProperty( attributeName ) );
	return NodeAttribute( property.getName(), property.getValue() );
}

void UIStyle::updateState() {
	for ( int i = StateFlagCount - 1; i >= 0; i-- ) {
		if ( ( mState & getStateFlag(i) ) == getStateFlag(i) ) {
			if ( stateExists( getStateFlag(i) ) ) {
				if ( mCurrentState != getStateFlag(i) ) {
					mPreviousState = mCurrentState;
					mCurrentState = getStateFlag(i);
					break;
				}
			}
		}
	}

	onStateChange();
}

void UIStyle::subscribeNonCacheableStyles() {
	for ( auto& style : mNoncacheableStyles ) {
		std::vector<CSS::StyleSheetElement*> elements = style.getSelector().getRelatedElements( mWidget, false );

		if ( !elements.empty() ) {
			for ( auto& element : elements ) {
				UIWidget * widget = dynamic_cast<UIWidget*>( element );

				if ( NULL != widget && NULL != widget->getUIStyle() ) {
					widget->getUIStyle()->subscribeRelated( mWidget );

					mSubscribedWidgets.insert( widget );
				}
			}
		}
	}
}

void UIStyle::unsubscribeNonCacheableStyles() {
	for ( auto& widget : mSubscribedWidgets ) {
		if ( NULL != widget->getUIStyle() ) {
			widget->getUIStyle()->unsubscribeRelated( mWidget );
		}
	}

	mSubscribedWidgets.clear();
}

void UIStyle::removeFromSubscribedWidgets( UIWidget * widget ) {
	mSubscribedWidgets.erase( widget );
}

void UIStyle::removeRelatedWidgets() {
	for ( auto& widget : mRelatedWidgets ) {
		if ( NULL != widget->getUIStyle() ) {
			widget->getUIStyle()->removeFromSubscribedWidgets( mWidget );
		}
	}

	mRelatedWidgets.clear();
}

void UIStyle::parseTransitions() {
	std::vector<std::string> properties;
	std::vector<Time> durations;
	std::vector<Time> delays;
	std::vector<Ease::Interpolation> timingFunctions;
	TransitionsMap transitions;

	for ( auto& attr : mTransitionAttributes ) {
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
			mTransitions = transitions;

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

		mTransitions[ property ] = transitionInfo;
	}
}

}}
