#include <eepp/graphics/fontmanager.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/stylesheetpropertyanimation.hpp>
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

UIStyle::UIStyle( UIWidget* widget ) :
	UIState(),
	mWidget( widget ),
	mElementStyle( std::make_shared<CSS::StyleSheetStyle>() ),
	mGlobalDefinition( nullptr ),
	mDefinition( nullptr ),
	mChangingState( false ),
	mForceReapplyProperties( false ),
	mDisableAnimations( false ),
	mFirstState( true ) {}

UIStyle::~UIStyle() {
	removeStructurallyVolatileWidgetFromParent();
	removeRelatedWidgets();
	unsubscribeNonCacheableStyles();
}

bool UIStyle::stateExists( const EE::Uint32& ) const {
	return true;
}

void UIStyle::setStyleSheetProperty( const StyleSheetProperty& property ) {
	if ( StyleSheetSpecification::instance()->isShorthand( property.getName() ) ) {
		std::vector<StyleSheetProperty> properties;

		properties = StyleSheetSpecification::instance()
						 ->getShorthand( property.getName() )
						 ->parse( property.getValue() );

		for ( auto& prop : properties ) {
			mElementStyle->setProperty( prop );
		}
	} else {
		mElementStyle->setProperty( property );
	}
}

void UIStyle::resetGlobalDefinition() {
	const auto& stylesheet = mWidget->getUISceneNode()->getStyleSheet();

	if ( &stylesheet == mLoadedStyleSheet && stylesheet.getVersion() == mLoadedVersion )
		return;

	auto prefDef = mGlobalDefinition;

	mGlobalDefinition = stylesheet.getElementStyles( mWidget, false );

	mLoadedStyleSheet = &stylesheet;
	mLoadedVersion = stylesheet.getVersion();
}

void UIStyle::load() {
	removeStructurallyVolatileWidgetFromParent();

	resetGlobalDefinition();

	unsubscribeNonCacheableStyles();

	subscribeNonCacheableStyles();

	addStructurallyVolatileWidgetFromParent();
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

StyleSheetPropertyAnimation* UIStyle::getAnimation( const PropertyDefinition* propertyDef ) {
	std::vector<Action*> actions = mWidget->getActionsByTag( propertyDef->getId() );
	if ( !actions.empty() ) {
		for ( auto& action : actions ) {
			if ( action->getId() == StyleSheetPropertyAnimation::ID ) {
				StyleSheetPropertyAnimation* animation =
					static_cast<StyleSheetPropertyAnimation*>( action );
				if ( animation->getAnimationOrigin() == AnimationOrigin::Animation ) {
					return animation;
				}
			}
		}
	}
	return NULL;
}

bool UIStyle::hasAnimation( const PropertyDefinition* propertyDef ) {
	return NULL != getAnimation( propertyDef );
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
	if ( NULL != mGlobalDefinition ) {
		auto it = mGlobalDefinition->getVariables().find( String::hash( variable ) );

		if ( it != mGlobalDefinition->getVariables().end() ) {
			return it->second;
		}
	}

	Node* parentWidget = mWidget->getParentWidget();

	if ( NULL != parentWidget ) {
		UIStyle* style = parentWidget->asType<UIWidget>()->getUIStyle();

		if ( NULL != style ) {
			return style->getVariable( variable );
		}
	}

	return StyleSheetVariable();
}

bool UIStyle::getForceReapplyProperties() const {
	return mForceReapplyProperties;
}

void UIStyle::setForceReapplyProperties( bool forceReapplyProperties ) {
	mForceReapplyProperties = forceReapplyProperties;
}

bool UIStyle::getDisableAnimations() const {
	return mDisableAnimations;
}

void UIStyle::setDisableAnimations( bool disableAnimations ) {
	mDisableAnimations = disableAnimations;
}

bool UIStyle::isStructurallyVolatile() const {
	return mGlobalDefinition && mGlobalDefinition->isStructurallyVolatile();
}

void UIStyle::reloadFontFamily() {
	if ( mDefinition && mDefinition->getPropertyIds().contains( (Uint32)PropertyId::FontFamily ) ) {
		auto propIt = mDefinition->getProperties().find( (Uint32)PropertyId::FontFamily );
		if ( propIt != mDefinition->getProperties().end() ) {
			applyStyleSheetProperty( propIt->second, nullptr );
		}
	}
}

void UIStyle::addStructurallyVolatileChild( UIWidget* widget ) {
	if ( mStructurallyVolatileChilds.count( widget ) == 0 ) {
		mStructurallyVolatileChilds.insert( widget );
	}
}

void UIStyle::removeStructurallyVolatileChild( UIWidget* widget ) {
	mStructurallyVolatileChilds.erase( widget );
}

UnorderedSet<UIWidget*>& UIStyle::getStructurallyVolatileChilds() {
	return mStructurallyVolatileChilds;
}

bool UIStyle::hasProperty( const CSS::PropertyId& propertyId ) const {
	return ( mGlobalDefinition && mGlobalDefinition->getProperty( (Uint32)propertyId ) ) ||
		   ( mElementStyle && mElementStyle->getPropertyById( propertyId ) );
}

void UIStyle::subscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.insert( widget );
}

void UIStyle::unsubscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.erase( widget );
}

void UIStyle::setVariableFromValue( StyleSheetProperty* property, const std::string& value ) {
	if ( !property->getVarCache().empty() ) {
		std::string newValue( value );
		for ( auto& var : property->getVarCache() ) {
			for ( auto& val : var.variableList ) {
				StyleSheetVariable variable( getVariable( val ) );
				if ( !variable.isEmpty() ) {
					String::replaceAll( newValue, var.definition, variable.getValue() );
					break;
				}
			}
		}
		property->setValue( newValue );
	}
}

void UIStyle::applyVarValues( StyleSheetProperty* property ) {
	if ( property->isVarValue() ) {
		if ( NULL != property->getPropertyDefinition() &&
			 property->getPropertyDefinition()->isIndexed() ) {
			for ( size_t i = 0; i < property->getPropertyIndexCount(); i++ ) {
				StyleSheetProperty* realProperty = property->getPropertyIndexRef( i );
				setVariableFromValue( realProperty, realProperty->getValue() );
			}
		} else {
			setVariableFromValue( property, property->getValue() );
		}
	}
}

void UIStyle::onStateChange() {
	if ( NULL == mWidget || NULL == mWidget->getUISceneNode() )
		return;

	mChangingState = true;

	std::shared_ptr<ElementDefinition> prevDefinition = mDefinition;
	std::shared_ptr<ElementDefinition> newDefinition =
		mWidget->getUISceneNode()->getStyleSheet().getElementStyles( mWidget, true );

	if ( newDefinition != mDefinition || mForceReapplyProperties ) {
		PropertyIdSet changedProperties;

		if ( mDefinition )
			changedProperties = mDefinition->getPropertyIds();

		if ( newDefinition )
			changedProperties |= newDefinition->getPropertyIds();

		if ( !mForceReapplyProperties ) {
			if ( nullptr != newDefinition && !newDefinition->getPropertyIds().empty() ) {
				if ( nullptr != mDefinition ) {
					const PropertyIdSet propertiesInBothDefinitions =
						( mDefinition->getPropertyIds() & newDefinition->getPropertyIds() );

					for ( Uint32 id : propertiesInBothDefinitions ) {
						const StyleSheetProperty* p0 = mDefinition->getProperty( id );
						const StyleSheetProperty* p1 = newDefinition->getProperty( id );
						if ( nullptr != p0 && nullptr != p1 && *p0 == *p1 )
							changedProperties.erase( id );
					}
				}
			}
		}

		mDefinition = newDefinition;

		mForceReapplyProperties = false;

		mWidget->beginAttributesTransaction();

		if ( nullptr != mDefinition && !mDefinition->getTransitionProperties().empty() ) {
			mTransitions = TransitionDefinition::parseTransitionProperties(
				mDefinition->getTransitionProperties() );
		}

		for ( auto prop : changedProperties ) {
			StyleSheetProperty* property = getLocalProperty( prop );

			if ( nullptr == property || NULL == property->getPropertyDefinition() )
				continue;

			applyVarValues( property );

			if ( property->getPropertyDefinition()->isIndexed() ) {
				for ( size_t i = 0; i < property->getPropertyIndexCount(); i++ ) {
					applyVarValues( property->getPropertyIndexRef( i ) );

					applyStyleSheetProperty( property->getPropertyIndex( i ), prevDefinition );
				}
			} else {
				applyStyleSheetProperty( *property, prevDefinition );
			}
		}

		updateAnimations();

		mWidget->endAttributesTransaction();
	}

	mStateDepthCounter++;

	if ( mStateDepthCounter <= 1 ) {
		for ( auto& related : mRelatedWidgets ) {
			if ( NULL != related->getUIStyle() ) {
				related->getUIStyle()->onStateChange();
			}
		}
	}

	mStateDepthCounter--;

	mChangingState = false;
	mFirstState = false;
}

const StyleSheetProperty*
UIStyle::getStatelessStyleSheetProperty( const Uint32& propertyId ) const {
	if ( 0 != propertyId ) {
		if ( !mElementStyle->getSelector().hasPseudoClasses() ) {
			const StyleSheetProperty* property = mElementStyle->getPropertyById( propertyId );

			if ( property )
				return property;
		}

		if ( nullptr == mDefinition )
			return nullptr;

		for ( auto style : mDefinition->getStyles() ) {
			if ( style->getSelector().isCacheable() && !style->getSelector().hasPseudoClasses() ) {
				const StyleSheetProperty* property = style->getPropertyById( propertyId );

				if ( property )
					return property;
			}
		}
	}

	return nullptr;
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
	if ( nullptr == mGlobalDefinition )
		return;
	for ( auto& style : mGlobalDefinition->getStyles() ) {
		if ( !style->getSelector().isCacheable() ) {
			std::vector<UIWidget*> elements =
				style->getSelector().getRelatedElements( mWidget, false );

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
									   std::shared_ptr<ElementDefinition> prevDefinition ) {
	const PropertyDefinition* propertyDefinition = property.getPropertyDefinition();

	// Save default value if possible and not available.
	if ( mCurrentState != UIState::StateFlagNormal ||
		 ( mCurrentState == UIState::StateFlagNormal && property.isVolatile() ) ) {
		const StyleSheetProperty* oldAttribute = getStatelessStyleSheetProperty( property.getId() );
		if ( nullptr == oldAttribute && getPreviousState() == UIState::StateFlagNormal ) {
			std::string value(
				mWidget->getPropertyString( propertyDefinition, property.getIndex() ) );
			if ( !value.empty() ) {
				setStyleSheetProperty(
					StyleSheetProperty( propertyDefinition, value, property.getIndex() ) );
			}
		}
	}

	if ( !mDisableAnimations && !mFirstState && !mWidget->isSceneNodeLoading() &&
		 NULL != propertyDefinition &&
		 StyleSheetPropertyAnimation::animationSupported( propertyDefinition->getType() ) &&
		 hasTransition( property.getName() ) &&
		 !hasAnimation( property.getPropertyDefinition() ) ) {
		std::string currentValue =
			mWidget->getPropertyString( propertyDefinition, property.getIndex() );
		std::string startValue( currentValue );

		if ( !startValue.empty() ) {
			// Get the real start value
			if ( nullptr != prevDefinition ) {
				auto prevProp = prevDefinition->getProperty( property.getId() );
				if ( nullptr != prevProp ) {
					StyleSheetProperty* curProperty = prevProp;
					if ( propertyDefinition->isIndexed() &&
						 property.getIndex() < curProperty->getPropertyIndexCount() ) {
						applyVarValues( curProperty->getPropertyIndexRef( property.getIndex() ) );
						startValue =
							curProperty->getPropertyIndex( property.getIndex() ).getValue();
					} else {
						applyVarValues( curProperty );
						startValue = curProperty->getValue();
					}
				}
			}

			TransitionDefinition transitionInfo( getTransition( property.getName() ) );

			std::vector<Action*> previousTransitions =
				mWidget->getActionsByTag( propertyDefinition->getId() );
			std::vector<Action*> removeTransitions;
			StyleSheetPropertyAnimation* prevTransition = NULL;

			if ( !previousTransitions.empty() ) {
				for ( auto& transition : previousTransitions ) {
					if ( transition->getId() == StyleSheetPropertyAnimation::ID ) {
						StyleSheetPropertyAnimation* tmpTransition =
							static_cast<StyleSheetPropertyAnimation*>( transition );
						if ( tmpTransition->getAnimationOrigin() == AnimationOrigin::Transition ) {
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
			}

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
					} else {
						elapsed = transitionInfo.getDuration();
					}
					startValue = prevTransition->getEndValue();
				} else if ( startValue == prevTransition->getEndValue() ) {
					startValue = currentValue;
				}

				for ( auto& rem : removeTransitions ) {
					mWidget->removeAction( rem );
				}
			}

			StyleSheetPropertyAnimation* newTransition = StyleSheetPropertyAnimation::New(
				propertyDefinition, startValue, property.getValue(), property.getIndex(),
				transitionInfo.getDuration(), transitionInfo.getDelay(),
				transitionInfo.getTimingFunction(), transitionInfo.getTimingFunctionParameters(),
				AnimationOrigin::Transition );
			newTransition->setElapsed( elapsed );
			newTransition->setTag( propertyDefinition->getId() );
			mWidget->runAction( newTransition );
		} else {
			mWidget->applyProperty( property );
		}
	} else {
		mWidget->applyProperty( property );
	}
}

void UIStyle::updateAnimations() {
	if ( nullptr == mDefinition )
		return;

	bool isDifferent = false;
	CSS::AnimationsMap animations;

	if ( !mDefinition->getAnimationProperties().empty() ) {
		animations =
			AnimationDefinition::parseAnimationProperties( mDefinition->getAnimationProperties() );
		if ( animations.size() == mAnimations.size() ) {
			for ( auto& animation : animations ) {
				auto animIt = mAnimations.find( animation.second.getName() );
				if ( animIt == mAnimations.end() || animIt->second != animation.second ) {
					isDifferent = true;
					break;
				}
			}
		} else {
			isDifferent = true;
		}
	} else if ( !mAnimations.empty() && mDefinition->getAnimationProperties().empty() ) {
		isDifferent = true;
	}

	if ( isDifferent ) {
		mAnimations.clear();

		removeAllAnimations();

		startAnimations( animations );
	} else if ( !mDefinition->getAnimationProperties().empty() ) {
		updateAnimationsPlayState();
	}
}

void UIStyle::updateAnimationsPlayState() {
	if ( mAnimations.empty() || nullptr == mDefinition )
		return;
	std::vector<Action*> actions = mWidget->getActions();
	for ( auto& action : actions ) {
		if ( action->getId() == StyleSheetPropertyAnimation::ID ) {
			StyleSheetPropertyAnimation* animation =
				static_cast<StyleSheetPropertyAnimation*>( action );
			if ( animation->getAnimationOrigin() == AnimationOrigin::Animation ) {
				// Check all the active animations.
				size_t animPos = 0;
				for ( auto anim = mAnimations.begin(); anim != mAnimations.end(); anim++ ) {
					// Find the animation index by iterating over them...
					if ( anim->first == animation->getAnimation().getName() ) {
						// Once found the iteration index of the corresponding keyframe animation
						// First check if in the current animation properties is there any
						// "animation-play-state" definition.
						bool isSet = false;
						for ( auto& animProp : mDefinition->getAnimationProperties() ) {
							if ( NULL != animProp->getPropertyDefinition() &&
								 animProp->getPropertyDefinition()->getPropertyId() ==
									 PropertyId::AnimationPlayState ) {
								// If found, get the pause/running state of the property, using the
								// index of the current animation, and set the animation.play-state.
								size_t animPropCount = animProp->getPropertyIndexCount();
								bool paused = animProp->getPropertyIndex( animPos % animPropCount )
														  .getValue() == "paused"
												  ? true
												  : false;
								animation->setPaused( paused );
								isSet = true;
								break;
							}
						}
						// If animation-play-state if set, continue with the next action.
						if ( isSet )
							break;
						// Otherwise set the animation-play-state defined on the animation.
						animation->setPaused( animation->getAnimation().isPaused() );
					}
					animPos++;
				}
			}
		}
	}
}

void UIStyle::startAnimations( const CSS::AnimationsMap& animations ) {
	UISceneNode* uiSceneNode = mWidget->getUISceneNode();

	if ( NULL == uiSceneNode )
		return;

	mAnimations = animations;

	CSS::StyleSheet& styleSheet = uiSceneNode->getStyleSheet();
	for ( auto& anim : animations ) {
		if ( styleSheet.isKeyframesDefined( anim.first ) ) {
			const AnimationDefinition& animation = anim.second;
			const KeyframesDefinition& keyframes = styleSheet.getKeyframesDefinition( anim.first );
			auto propDefMap = keyframes.getPropertyDefinitionList();

			for ( auto& propertyDef : propDefMap ) {
				const PropertyDefinition* propDef = propertyDef.second;

				if ( StyleSheetPropertyAnimation::animationSupported( propDef->getType() ) ) {
					if ( propDef->isIndexed() ) {
						StyleSheetProperty* prop = mDefinition->getProperty( propDef->getId() );
						if ( nullptr != prop ) {
							for ( size_t i = 0; i < prop->getPropertyIndexCount(); i++ ) {
								removeAnimation( propDef, i );
								StyleSheetPropertyAnimation* newAnimation =
									StyleSheetPropertyAnimation::fromAnimationKeyframes(
										animation, keyframes, propDef, mWidget, i );
								newAnimation->setFlags( animation.getId() );
								newAnimation->setTag( propDef->getId() );
								mWidget->runAction( newAnimation );
							}
						} else {
							removeAnimation( propDef, 0 );
							StyleSheetPropertyAnimation* newAnimation =
								StyleSheetPropertyAnimation::fromAnimationKeyframes(
									animation, keyframes, propDef, mWidget, 0 );
							newAnimation->setFlags( animation.getId() );
							newAnimation->setTag( propDef->getId() );
							mWidget->runAction( newAnimation );
						}
					} else {
						removeAnimation( propDef, 0 );
						StyleSheetPropertyAnimation* newAnimation =
							StyleSheetPropertyAnimation::fromAnimationKeyframes(
								animation, keyframes, propDef, mWidget, 0 );
						newAnimation->setFlags( animation.getId() );
						newAnimation->setTag( propDef->getId() );
						mWidget->runAction( newAnimation );
					}
				}
			}
		}
	}
}

void UIStyle::removeAllAnimations() {
	std::vector<Action*> actions = mWidget->getActions();
	std::vector<Action*> removeList;
	for ( auto& action : actions ) {
		if ( action->getId() == StyleSheetPropertyAnimation::ID ) {
			StyleSheetPropertyAnimation* animation =
				static_cast<StyleSheetPropertyAnimation*>( action );
			if ( animation->getAnimationOrigin() == AnimationOrigin::Animation ) {
				removeList.push_back( action );
			}
		}
	}
	if ( !removeList.empty() ) {
		for ( auto& action : removeList ) {
			static_cast<StyleSheetPropertyAnimation*>( action )->notifyClose();
		}
		mWidget->removeActions( removeList );
	}
}

void UIStyle::removeAnimation( const PropertyDefinition* propertyDefinition,
							   const Uint32& propertyIndex ) {
	std::vector<Action*> previousTransitions =
		mWidget->getActionsByTag( propertyDefinition->getId() );
	std::vector<Action*> removeTransitions;
	StyleSheetPropertyAnimation* prevTransition = NULL;

	if ( !previousTransitions.empty() ) {
		for ( auto& transition : previousTransitions ) {
			if ( transition->getId() == StyleSheetPropertyAnimation::ID ) {
				StyleSheetPropertyAnimation* tmpTransition =
					static_cast<StyleSheetPropertyAnimation*>( transition );
				if ( propertyDefinition->isIndexed() ) {
					if ( tmpTransition->getPropertyIndex() == propertyIndex ) {
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

		mWidget->removeActions( removeTransitions );
	}
}

StyleSheetProperty* UIStyle::getLocalProperty( Uint32 propId ) {
	StyleSheetProperty* defProperty = mDefinition ? mDefinition->getProperty( propId ) : nullptr;
	StyleSheetProperty* elemProperty =
		mElementStyle ? mElementStyle->getPropertyById( propId ) : nullptr;
	if ( defProperty && elemProperty )
		return defProperty->getSpecificity() > elemProperty->getSpecificity() ? defProperty
																			  : elemProperty;
	return defProperty ? defProperty : elemProperty;
}

void UIStyle::addStructurallyVolatileWidgetFromParent() {
	if ( mGlobalDefinition && mGlobalDefinition->isStructurallyVolatile() && mWidget->getParent() &&
		 mWidget->getParent()->isWidget() &&
		 mWidget->getParent()->asType<UIWidget>()->getUIStyle() ) {
		mWidget->getParent()->asType<UIWidget>()->getUIStyle()->addStructurallyVolatileChild(
			mWidget );
	}
}

void UIStyle::removeStructurallyVolatileWidgetFromParent() {
	if ( mGlobalDefinition && mGlobalDefinition->isStructurallyVolatile() && mWidget->getParent() &&
		 mWidget->getParent()->isWidget() &&
		 mWidget->getParent()->asType<UIWidget>()->getUIStyle() ) {
		mWidget->getParent()->asType<UIWidget>()->getUIStyle()->removeStructurallyVolatileChild(
			mWidget );
	}
}

}} // namespace EE::UI
