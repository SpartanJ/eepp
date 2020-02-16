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

void UIStyle::setStyleSheetProperty( const StyleSheetProperty& attribute ) {
	if ( StyleSheetSpecification::instance()->isShorthand( attribute.getName() ) ) {
		std::vector<StyleSheetProperty> properties = ShorthandDefinition::parseShorthand(
			StyleSheetSpecification::instance()->getShorthand( attribute.getName() ),
			attribute.getValue() );

		for ( auto& property : properties )
			mElementStyle.setProperty( property );
	} else {
		mElementStyle.setProperty( attribute );
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
	if ( !properties.empty() ) {
		for ( const auto& it : properties ) {
			setStyleSheetProperty( it.second );
		}
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
			setVariableFromValue( property, property.getValue() );
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

		tryApplyStyle( mElementStyle );

		for ( auto& style : mCacheableStyles ) {
			tryApplyStyle( style );
		}

		for ( auto& style : mNoncacheableStyles ) {
			tryApplyStyle( style );
		}

		if ( !mapEquals( mProperties, prevProperties ) ) {
			mTransitions = TransitionDefinition::parseTransitionProperties( mTransitionAttributes );

			mWidget->beginAttributesTransaction();

			for ( const auto& prop : mProperties ) {
				const StyleSheetProperty& property = prop.second;
				const PropertyDefinition* propertyDefinition = property.getPropertyDefinition();

				// Save default value if possible and not available.
				if ( mCurrentState != UIState::StateFlagNormal ||
					 ( mCurrentState == UIState::StateFlagNormal && property.isVolatile() ) ) {
					StyleSheetProperty oldAttribute =
						getStatelessStyleSheetProperty( property.getId() );
					if ( oldAttribute.isEmpty() &&
						 getPreviousState() == UIState::StateFlagNormal ) {
						std::string value( mWidget->getPropertyString( propertyDefinition ) );
						if ( !value.empty() ) {
							setStyleSheetProperty(
								StyleSheetProperty( propertyDefinition, value ) );
						}
					}
				}

				if ( !mWidget->isSceneNodeLoading() && NULL != propertyDefinition &&
					 StyleSheetPropertyTransition::transitionSupported(
						 propertyDefinition->getType() ) &&
					 hasTransition( property.getName() ) ) {
					std::string currentValue = mWidget->getPropertyString( propertyDefinition );
					std::string startValue( currentValue );

					if ( !startValue.empty() ) {
						// Get the real start value
						auto prevPropIt = prevProperties.find( property.getId() );
						if ( prevPropIt != prevProperties.end() ) {
							startValue = prevPropIt->second.getValue();
						}

						TransitionDefinition transitionInfo( getTransition( property.getName() ) );

						std::vector<Action*> previousTransitions =
							mWidget->getActionsByTag( propertyDefinition->getId() );

						Time duration( transitionInfo.getDuration() );
						Time elapsed( Time::Zero );

						if ( !previousTransitions.empty() &&
							 previousTransitions[0]->getId() == StyleSheetPropertyTransition::ID ) {
							StyleSheetPropertyTransition* prevTransition =
								static_cast<StyleSheetPropertyTransition*>(
									previousTransitions[0] );

							if ( prevTransition->getEndValue() == property.getValue() ) {
								continue;
							} else if ( prevTransition->getStartValue() == property.getValue() ) {
								Float currentProgress =
									prevTransition->getElapsed().asMilliseconds() /
									prevTransition->getDuration().asMilliseconds();
								currentProgress = eemin( 1.f, currentProgress );
								if ( 0.f != currentProgress ) {
									elapsed = Milliseconds(
										( 1.f - currentProgress ) *
										transitionInfo.getDuration().asMilliseconds() );
								}
							} else if ( startValue == prevTransition->getEndValue() ) {
								startValue = currentValue;
							} /*else if ( startValue == prevTransition->getStartValue() ) {
							}*/

							for ( auto& prev : previousTransitions ) {
								mWidget->removeAction( prev );
							}
						}

						Action* newTransition = StyleSheetPropertyTransition::New(
							propertyDefinition, startValue, property.getValue(), duration,
							transitionInfo.getTimingFunction() );

						static_cast<StyleSheetPropertyTransition*>( newTransition )
							->setElapsed( elapsed );

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

}} // namespace EE::UI
