#include <eepp/graphics/fontmanager.hpp>
#include <eepp/scene/actions/actions.hpp>
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

UIStyle::UIStyle( UIWidget* widget ) : UIState(), mWidget( widget ), mChangingState( false ) {
	load();
}

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

	UISceneNode* uiSceneNode = mWidget->getSceneNode()->isUISceneNode()
								   ? static_cast<UISceneNode*>( mWidget->getSceneNode() )
								   : NULL;

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

void UIStyle::subscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.insert( widget );
}

void UIStyle::unsubscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.erase( widget );
}

void UIStyle::tryApplyStyle( const StyleSheetStyle& style ) {
	if ( style.getSelector().select( mWidget ) ) {
		for ( const auto& prop : style.getProperties() ) {
			const StyleSheetProperty& property = prop.second;
			const auto& it = mProperties.find( property.getName() );

			if ( it == mProperties.end() ||
				 property.getSpecificity() >= it->second.getSpecificity() ) {
				mProperties[property.getName()] = property;

				if ( String::startsWith( property.getName(), "transition" ) )
					mTransitionAttributes.push_back( property );
			}
		}
	}
}

void UIStyle::onStateChange() {
	if ( NULL != mWidget ) {
		mChangingState = true;

		mProperties.clear();
		mTransitionAttributes.clear();

		tryApplyStyle( mElementStyle );

		for ( auto& style : mCacheableStyles ) {
			tryApplyStyle( style );
		}

		for ( auto& style : mNoncacheableStyles ) {
			tryApplyStyle( style );
		}

		mTransitions = TransitionDefinition::parseTransitionProperties( mTransitionAttributes );

		mWidget->beginAttributesTransaction();

		for ( const auto& prop : mProperties ) {
			const StyleSheetProperty& property = prop.second;
			const PropertyDefinition* propertyDefinition = property.getPropertyDefinition();

			// Save default value if possible and not available.
			if ( mCurrentState != UIState::StateFlagNormal ||
				 ( mCurrentState == UIState::StateFlagNormal && property.isVolatile() ) ) {
				StyleSheetProperty oldAttribute =
					getStatelessStyleSheetProperty( property.getName() );
				if ( oldAttribute.isEmpty() && getPreviousState() == UIState::StateFlagNormal ) {
					std::string value( mWidget->getPropertyString( propertyDefinition ) );
					if ( !value.empty() ) {
						setStyleSheetProperty( StyleSheetProperty( propertyDefinition, value ) );
					}
				}
			}

			if ( !mWidget->isSceneNodeLoading() &&
				 StyleSheetPropertyTransition::transitionSupported(
					 propertyDefinition->getType() ) &&
				 hasTransition( property.getName() ) ) {
				std::string startValue = mWidget->getPropertyString( propertyDefinition );

				if ( !startValue.empty() ) {
					TransitionDefinition transitionInfo( getTransition( property.getName() ) );

					std::vector<Action*> previousTransitions =
						mWidget->getActionsByTag( propertyDefinition->getId() );

					Time duration( transitionInfo.getDuration() );

					if ( !previousTransitions.empty() ) {
						StyleSheetPropertyTransition* prevTransition =
							dynamic_cast<StyleSheetPropertyTransition*>( previousTransitions[0] );

						if ( NULL != prevTransition ) {
							if ( prevTransition->getEndValue() == property.getValue() ) {
								Float currentProgress =
									prevTransition->getElapsed().asMilliseconds() /
									prevTransition->getDuration().asMilliseconds();
								currentProgress = eemin( 1.f, currentProgress );
								if ( 0.f != currentProgress ) {
									duration = Milliseconds(
										transitionInfo.getDuration().asMilliseconds() *
										currentProgress );
								}
							}
						}

						for ( auto& prev : previousTransitions ) {
							mWidget->removeAction( prev );
						}
					}

					Action* newTransition = StyleSheetPropertyTransition::New(
						propertyDefinition, startValue, property.getValue(), duration,
						transitionInfo.getTimingFunction() );

					if ( transitionInfo.getDelay().asMicroseconds() > 0 ) {
						newTransition = Actions::Sequence::New(
							Actions::Delay::New( transitionInfo.getDelay() ), newTransition );
					}
					newTransition->setTag( propertyDefinition->getId() );
					mWidget->runAction( newTransition );
				} else {
					mWidget->applyProperty( property, mCurrentState );
				}
			} else {
				mWidget->applyProperty( property, mCurrentState );
			}
		}

		mWidget->endAttributesTransaction();

		for ( auto& related : mRelatedWidgets ) {
			if ( NULL != related->getUIStyle() ) {
				related->getUIStyle()->onStateChange();
			}
		}

		mChangingState = false;
	}
}

StyleSheetProperty
UIStyle::getStatelessStyleSheetProperty( const std::string& propertyName ) const {
	if ( !propertyName.empty() ) {
		if ( !mElementStyle.getSelector().hasPseudoClasses() ) {
			StyleSheetProperty property = mElementStyle.getPropertyByName( propertyName );

			if ( !property.isEmpty() )
				return property;
		}

		for ( const StyleSheetStyle& style : mCacheableStyles ) {
			if ( !style.getSelector().hasPseudoClasses() ) {
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
		std::vector<CSS::StyleSheetElement*> elements =
			style.getSelector().getRelatedElements( mWidget, false );

		if ( !elements.empty() ) {
			for ( auto& element : elements ) {
				UIWidget* widget = dynamic_cast<UIWidget*>( element );

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
