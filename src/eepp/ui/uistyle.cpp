#include <eepp/graphics/fontmanager.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/stylesheetpropertytransition.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::UI::CSS;
using namespace EE::Scene;

namespace EE { namespace UI {

UIStyle* UIStyle::New( UIWidget* widget ) {
	return eeNew( UIStyle, ( widget ) );
}

UIStyle::UIStyle( UIWidget* widget ) : UIState(), mWidget( widget ), mChangingState( false ) {}

UIStyle::~UIStyle() {
	removeRelatedWidgets();
	unsubscribeNonCacheableStyles();
}

bool UIStyle::stateExists( const EE::Uint32& ) const {
	return true;
}

void UIStyle::setStyleSheetProperty( const StyleSheetProperty& property ) {
	std::vector<StyleSheetProperty> properties;

	if ( StyleSheetSpecification::instance()->isShorthand( property.getName() ) ) {
		properties = ShorthandDefinition::parseShorthand(
			StyleSheetSpecification::instance()->getShorthand( property.getName() ),
			property.getValue() );
	} else {
		properties.emplace_back( property );
	}

	for ( auto& prop : properties ) {
		mElementStyle.setProperty( prop );
	}
}

void UIStyle::load() {
	unsubscribeNonCacheableStyles();

	mCacheableStyles.clear();
	mNoncacheableStyles.clear();
	mElementStyle.clearProperties();
	mProperties.clear();
	mVariables.clear();

	UISceneNode* uiSceneNode = mWidget->getUISceneNode();

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

				findVariables( style );
			}

			for ( auto& style : mCacheableStyles ) {
				applyVarValues( style );
			}

			for ( auto& style : mNoncacheableStyles ) {
				applyVarValues( style );
			}

			subscribeNonCacheableStyles();
		}
	}
}

void UIStyle::setStyleSheetProperties( const CSS::StyleSheetProperties& properties ) {
	for ( const auto& it : properties ) {
		setStyleSheetProperty( it.second );
	}
}

bool UIStyle::hasTransition( const std::string& propertyName ) {
	return mTransitions.find( propertyName ) != mTransitions.end() ||
		   mTransitions.find( "all" ) != mTransitions.end();
}

TransitionDefinition UIStyle::getTransition( const std::string& propertyName ) {
	auto propertyTransitionIt = mTransitions.find( propertyName );

	if ( propertyTransitionIt != mTransitions.end() ) {
		return propertyTransitionIt->second;
	} else if ( ( propertyTransitionIt = mTransitions.find( "all" ) ) != mTransitions.end() ) {
		return propertyTransitionIt->second;
	}

	return TransitionDefinition();
}

const bool& UIStyle::isChangingState() const {
	return mChangingState;
}

StyleSheetVariable UIStyle::getVariable( const std::string& variable ) {
	auto it = mVariables.find( variable );

	if ( it != mVariables.end() ) {
		return it->second;
	} else {
		Node* parentWidget = mWidget->getParentWidget();

		if ( NULL != parentWidget ) {
			UIStyle* style = parentWidget->asType<UIWidget>()->getUIStyle();

			if ( NULL != style ) {
				return style->getVariable( variable );
			}
		}
	}

	return StyleSheetVariable();
}

void UIStyle::subscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.insert( widget );
}

void UIStyle::unsubscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.erase( widget );
}

void UIStyle::tryApplyStyle( const StyleSheetStyle& style ) {
	if ( style.isMediaValid() && style.getSelector().select( mWidget ) ) {
		for ( const auto& prop : style.getProperties() ) {
			const StyleSheetProperty& property = prop.second;
			const auto& it = mProperties.find( property.getId() );

			if ( it == mProperties.end() ||
				 property.getSpecificity() >= it->second.getSpecificity() ) {
				mProperties[property.getId()] = property;

				if ( String::startsWith( property.getName(), "transition" ) )
					mTransitionAttributes.push_back( property );
				else if ( String::startsWith( property.getName(), "animation" ) )
					mAnimationAttributes.push_back( property );
			}
		}
	}
}

void UIStyle::findVariables( const StyleSheetStyle& style ) {
	if ( style.getSelector().select( mWidget, false ) ) {
		for ( const auto& vars : style.getVariables() ) {
			const StyleSheetVariable& variable = vars.second;
			const auto& it = mVariables.find( variable.getName() );

			if ( it == mVariables.end() ||
				 variable.getSpecificity() >= it->second.getSpecificity() ) {
				mVariables[variable.getName()] = variable;
			}
		}
	}
}

void UIStyle::setVariableFromValue( StyleSheetProperty& property, const std::string& value ) {
	FunctionString functionType = FunctionString::parse( value );

	if ( !functionType.getParameters().empty() ) {
		for ( auto& val : functionType.getParameters() ) {
			if ( String::startsWith( val, "--" ) ) {
				StyleSheetVariable variable( getVariable( val ) );

				if ( !variable.isEmpty() ) {
					property.setValue( variable.getValue() );
					break;
				}
			} else if ( String::startsWith( val, "var(" ) ) {
				return setVariableFromValue( property, val );
			} else {
				property.setValue( val );
				break;
			}
		}
	}
}

void UIStyle::applyVarValues( StyleSheetStyle& style ) {
	for ( auto& prop : style.getPropertiesRef() ) {
		StyleSheetProperty& property = prop.second;

		if ( property.isVarValue() ) {
			if ( NULL != property.getPropertyDefinition() &&
				 property.getPropertyDefinition()->isIndexed() ) {
				for ( size_t i = 0; i < property.getPropertyIndexCount(); i++ ) {
					StyleSheetProperty& realProperty = property.getPropertyIndexRef( i );
					setVariableFromValue( realProperty, realProperty.getValue() );
				}
			} else {
				setVariableFromValue( property, property.getValue() );
			}
		}
	}
}

template <typename Map> bool mapEquals( Map const& lhs, Map const& rhs ) {
	// No predicate needed because there is operator== for pairs already.
	return lhs.size() == rhs.size() && std::equal( lhs.begin(), lhs.end(), rhs.begin() );
}

void UIStyle::onStateChange() {
	if ( NULL != mWidget ) {
		mChangingState = true;

		CSS::StyleSheetProperties prevProperties( mProperties );

		mProperties.clear();
		mTransitionAttributes.clear();
		mAnimationAttributes.clear();

		tryApplyStyle( mElementStyle );

		for ( auto& style : mCacheableStyles ) {
			tryApplyStyle( style );
		}

		for ( auto& style : mNoncacheableStyles ) {
			tryApplyStyle( style );
		}

		if ( !mapEquals( mProperties, prevProperties ) ) {
			if ( !mAnimationAttributes.empty() ) {
				mAnimations = AnimationDefinition::parseAnimationProperties( mAnimationAttributes );
			}

			if ( !mTransitionAttributes.empty() ) {
				mTransitions =
					TransitionDefinition::parseTransitionProperties( mTransitionAttributes );
			}

			mWidget->beginAttributesTransaction();

			for ( auto& prop : mProperties ) {
				StyleSheetProperty& property = prop.second;

				if ( property.getPropertyDefinition()->isIndexed() ) {
					for ( size_t i = 0; i < property.getPropertyIndexCount(); i++ ) {
						applyStyleSheetProperty( property.getPropertyIndex( i ), prevProperties );
					}
				} else {
					applyStyleSheetProperty( property, prevProperties );
				}
			}

			mWidget->endAttributesTransaction();
		}

		for ( auto& related : mRelatedWidgets ) {
			if ( NULL != related->getUIStyle() ) {
				related->getUIStyle()->onStateChange();
			}
		}

		mChangingState = false;
	}
}

StyleSheetProperty UIStyle::getStatelessStyleSheetProperty( const Uint32& propertyId ) const {
	if ( 0 != propertyId ) {
		if ( !mElementStyle.getSelector().hasPseudoClasses() ) {
			StyleSheetProperty property = mElementStyle.getPropertyById( propertyId );

			if ( !property.isEmpty() )
				return property;
		}

		for ( const StyleSheetStyle& style : mCacheableStyles ) {
			if ( !style.getSelector().hasPseudoClasses() ) {
				StyleSheetProperty property = style.getPropertyById( propertyId );

				if ( !property.isEmpty() )
					return property;
			}
		}
	}

	return StyleSheetProperty();
}

void UIStyle::updateState() {
	for ( int i = StateFlagCount - 1; i >= 0; i-- ) {
		if ( ( mState & getStateFlag( i ) ) == getStateFlag( i ) ) {
			if ( stateExists( getStateFlag( i ) ) ) {
				if ( mCurrentState != getStateFlag( i ) ) {
					mPreviousState = mCurrentState;
					mCurrentState = getStateFlag( i );
					break;
				}
			}
		}
	}

	onStateChange();
}

void UIStyle::subscribeNonCacheableStyles() {
	for ( auto& style : mNoncacheableStyles ) {
		std::vector<UIWidget*> elements = style.getSelector().getRelatedElements( mWidget, false );

		if ( !elements.empty() ) {
			for ( auto& element : elements ) {
				UIWidget* widget = element->asType<UIWidget>();

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

void UIStyle::removeFromSubscribedWidgets( UIWidget* widget ) {
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

void UIStyle::applyStyleSheetProperty( const StyleSheetProperty& property,
									   StyleSheetProperties& prevProperties ) {
	const PropertyDefinition* propertyDefinition = property.getPropertyDefinition();

	// Save default value if possible and not available.
	if ( mCurrentState != UIState::StateFlagNormal ||
		 ( mCurrentState == UIState::StateFlagNormal && property.isVolatile() ) ) {
		StyleSheetProperty oldAttribute = getStatelessStyleSheetProperty( property.getId() );
		if ( oldAttribute.isEmpty() && getPreviousState() == UIState::StateFlagNormal ) {
			std::string value(
				mWidget->getPropertyString( propertyDefinition, property.getIndex() ) );
			if ( !value.empty() ) {
				setStyleSheetProperty(
					StyleSheetProperty( propertyDefinition, value, property.getIndex() ) );
			}
		}
	}

	if ( !mWidget->isSceneNodeLoading() && NULL != propertyDefinition &&
		 StyleSheetPropertyTransition::transitionSupported( propertyDefinition->getType() ) &&
		 hasTransition( property.getName() ) ) {
		std::string currentValue =
			mWidget->getPropertyString( propertyDefinition, property.getIndex() );
		std::string startValue( currentValue );

		if ( !startValue.empty() ) {
			// Get the real start value
			auto prevPropIt = prevProperties.find( property.getId() );
			if ( prevPropIt != prevProperties.end() ) {
				StyleSheetProperty& curProperty = prevPropIt->second;
				if ( propertyDefinition->isIndexed() &&
					 property.getIndex() < curProperty.getPropertyIndexCount() ) {
					startValue = curProperty.getPropertyIndex( property.getIndex() ).getValue();
				} else {
					startValue = curProperty.getValue();
				}
			}

			TransitionDefinition transitionInfo( getTransition( property.getName() ) );

			std::vector<Action*> previousTransitions =
				mWidget->getActionsByTag( propertyDefinition->getId() );
			std::vector<Action*> removeTransitions;
			StyleSheetPropertyTransition* prevTransition = NULL;

			if ( !previousTransitions.empty() ) {
				for ( auto& transition : previousTransitions ) {
					if ( transition->getId() == StyleSheetPropertyTransition::ID ) {
						StyleSheetPropertyTransition* tmpTransition =
							static_cast<StyleSheetPropertyTransition*>( transition );
						if ( propertyDefinition->isIndexed() ) {
							if ( tmpTransition->getPropertyIndex() == property.getIndex() ) {
								prevTransition = tmpTransition;
								removeTransitions.push_back( prevTransition );
							}
						} else {
							prevTransition = tmpTransition;
							removeTransitions.push_back( prevTransition );
							break;
						}
					}
				}
			}

			Time duration( transitionInfo.getDuration() );
			Time elapsed( Time::Zero );

			if ( NULL != prevTransition ) {
				if ( prevTransition->getEndValue() == property.getValue() ) {
					return;
				} else if ( prevTransition->getStartValue() == property.getValue() ) {
					Float currentProgress = prevTransition->getCurrentProgress();
					currentProgress = eemin( 1.f, currentProgress );
					if ( 0.f != currentProgress ) {
						elapsed = Milliseconds( ( 1.f - currentProgress ) *
												transitionInfo.getDuration().asMilliseconds() );
					}
				} else if ( startValue == prevTransition->getEndValue() ) {
					startValue = currentValue;
				} /*else if ( startValue == prevTransition->getStartValue() ) {
				}*/

				for ( auto& rem : removeTransitions ) {
					mWidget->removeAction( rem );
				}
			}

			Action* newTransition = StyleSheetPropertyTransition::New(
				propertyDefinition, startValue, property.getValue(), property.getIndex(), duration,
				transitionInfo.getTimingFunction() );

			static_cast<StyleSheetPropertyTransition*>( newTransition )->setElapsed( elapsed );

			if ( transitionInfo.getDelay().asMicroseconds() > 0 ) {
				newTransition = Actions::Sequence::New(
					Actions::Delay::New( transitionInfo.getDelay() ), newTransition );
			}
			newTransition->setTag( propertyDefinition->getId() );
			mWidget->runAction( newTransition );
		} else {
			mWidget->applyProperty( property );
		}
	} else {
		mWidget->applyProperty( property );
	}
}

}} // namespace EE::UI
