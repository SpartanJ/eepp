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

UIStyle::PropertyResolution::PropertyResolution( PropertyResolution&& other ) noexcept :
	mOwner( other.mOwner ), mProperty( other.mProperty ), mSlot( other.mSlot ) {
	other.mOwner = nullptr;
	other.mProperty = nullptr;
	other.mSlot = NoSlot;
}

UIStyle::PropertyResolution::~PropertyResolution() {
	release();
}

void UIStyle::PropertyResolution::release() {
	if ( nullptr == mOwner || NoSlot == mSlot )
		return;
	eeASSERT( mOwner->mPropertyResolutionDepth == mSlot + 1 );
	--mOwner->mPropertyResolutionDepth;
	mOwner = nullptr;
	mProperty = nullptr;
	mSlot = NoSlot;
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

void UIStyle::setStyleSheetVariable( const StyleSheetVariable& variable ) {
	StyleSheetVariable inlineVariable( variable );
	inlineVariable.setSpecificity( StyleSheetSelectorRule::SpecificityInline );
	mElementStyle->setVariable( inlineVariable, false );
}

void UIStyle::resetGlobalDefinition( bool force ) {
	const auto& stylesheet = mWidget->getUISceneNode()->getStyleSheet();
	if ( mWidget->getFlags() & UI_IGNORE_GLOBAL_CSS ) {
		mGlobalDefinition = nullptr;
		mLoadedStyleSheet = &stylesheet;
		mLoadedVersion = stylesheet.getVersion();
		return;
	}

	if ( !force && &stylesheet == mLoadedStyleSheet && stylesheet.getVersion() == mLoadedVersion )
		return;

	auto prefDef = mGlobalDefinition;

	mGlobalDefinition = stylesheet.getElementStyles( mWidget, false );

	mLoadedStyleSheet = &stylesheet;
	mLoadedVersion = stylesheet.getVersion();
}

void UIStyle::resetCachedProperties() {
	mElementStyle->clearCachedProperties();
}

void UIStyle::load() {
	removeStructurallyVolatileWidgetFromParent();

	resetGlobalDefinition( true );

	unsubscribeNonCacheableStyles();

	subscribeNonCacheableStyles();

	addStructurallyVolatileWidgetFromParent();

	// applyInheritedProperties();
}

void UIStyle::applyInheritedProperties() {
	const auto& props = StyleSheetSpecification::instance()->getInheritableProperties();
	mWidget->beginAttributesTransaction();
	for ( const auto& propId : props ) {
		if ( !hasLocalProperty( propId ) ) {
			UIStyle* inheritedStyle = nullptr;
			auto inheritedProp = getInheritedProperty( propId, &inheritedStyle );
			if ( inheritedProp ) {
				auto resolvedProperty = inheritedStyle->resolveProperty( inheritedProp );
				mWidget->applyProperty( *resolvedProperty.get() );
			}
		}
	}
	mWidget->endAttributesTransaction();
}

void UIStyle::setStyleSheetProperties( const CSS::StyleSheetProperties& properties ) {
	for ( const auto& it : properties ) {
		setStyleSheetProperty( it.second );
	}
}

bool UIStyle::hasTransition( const std::string& propertyName ) {
	return mDefinition && mDefinition->getTransitions().get( String::hashToLower(
							  propertyName.c_str(), static_cast<Int64>( propertyName.size() ) ) );
}

StyleSheetPropertyAnimation* UIStyle::getAnimation( const PropertyDefinition* propertyDef ) {
	SmallVector<Action*, 4> actions;
	mWidget->getActionsByTag( propertyDef->getId(), actions );
	for ( auto* action : actions ) {
		if ( action->getId() == StyleSheetPropertyAnimation::ID ) {
			StyleSheetPropertyAnimation* animation =
				static_cast<StyleSheetPropertyAnimation*>( action );
			if ( animation->getAnimationOrigin() == AnimationOrigin::Animation ) {
				return animation;
			}
		}
	}
	return NULL;
}

bool UIStyle::hasAnimation( const PropertyDefinition* propertyDef ) {
	return NULL != getAnimation( propertyDef );
}

TransitionDefinition UIStyle::getTransition( const std::string& propertyName ) {
	TransitionDefinition transition;
	if ( !mDefinition )
		return transition;
	const auto* computed = mDefinition->getTransitions().get(
		String::hashToLower( propertyName.c_str(), static_cast<Int64>( propertyName.size() ) ) );
	if ( !computed )
		return transition;
	transition.property = propertyName;
	transition.timingFunction = computed->timingFunction;
	transition.timingFunctionParameters.assign( computed->timingFunctionParameters.begin(),
												computed->timingFunctionParameters.end() );
	transition.delay = computed->delay;
	transition.duration = computed->duration;
	return transition;
}

const bool& UIStyle::isChangingState() const {
	return mChangingState;
}

StyleSheetVariable UIStyle::getVariable( const std::string& variable ) {
	const StyleSheetVariable* resolved = getVariableRef( variable );
	return resolved ? *resolved : StyleSheetVariable();
}

const StyleSheetVariable* UIStyle::getVariableRef( const std::string& variable ) {
	if ( NULL != mWidget && NULL != mWidget->getUISceneNode() )
		resetGlobalDefinition();

	const auto nameHash = String::hash( variable );
	const StyleSheetVariable* localVariable = nullptr;
	if ( mElementStyle ) {
		auto it = mElementStyle->getVariables().find( nameHash );
		if ( it != mElementStyle->getVariables().end() )
			localVariable = &it->second;
	}

	if ( NULL != mGlobalDefinition ) {
		auto it = mGlobalDefinition->getVariables().find( nameHash );

		if ( it != mGlobalDefinition->getVariables().end() ) {
			if ( nullptr == localVariable ||
				 it->second.getSpecificity() > localVariable->getSpecificity() )
				return &it->second;
		}
	}

	if ( nullptr != localVariable )
		return localVariable;

	Node* parentWidget = mWidget ? mWidget->getParentWidget() : nullptr;

	if ( NULL != parentWidget ) {
		UIStyle* style = parentWidget->asType<UIWidget>()->getUIStyle();

		if ( NULL != style ) {
			return style->getVariableRef( variable );
		}
	}

	return nullptr;
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
	if ( mDefinition && mDefinition->getPropertyIds().contains( PropertyId::FontFamily ) ) {
		StyleSheetProperty* fontProp = mDefinition->getProperty( PropertyId::FontFamily );
		if ( fontProp )
			applyStyleSheetProperty( *fontProp, nullptr );
	}
}

void UIStyle::addStructurallyVolatileChild( UIWidget* widget ) {
	if ( mStructurallyVolatileChildren.count( widget ) == 0 ) {
		mStructurallyVolatileChildren.insert( widget );
	}
}

void UIStyle::removeStructurallyVolatileChild( UIWidget* widget ) {
	mStructurallyVolatileChildren.erase( widget );
}

UnorderedSet<UIWidget*>& UIStyle::getStructurallyVolatileChildren() {
	return mStructurallyVolatileChildren;
}

const CSS::StyleSheetProperty* UIStyle::getProperty( const CSS::PropertyId& id ) const {
	const auto* gProp = mGlobalDefinition ? mGlobalDefinition->getProperty( id ) : nullptr;
	const auto* elProp = mElementStyle ? mElementStyle->getPropertyById( id ) : nullptr;
	if ( elProp && gProp )
		return elProp->getSpecificity() > gProp->getSpecificity() ? elProp : gProp;
	return elProp ? elProp : gProp;
}

bool UIStyle::hasProperty( const CSS::PropertyId& propertyId ) const {
	return ( mGlobalDefinition && mGlobalDefinition->getProperty( propertyId ) ) ||
		   ( mElementStyle && mElementStyle->getPropertyById( propertyId ) );
}

bool UIStyle::hasLocalProperty( PropertyId propId ) const {
	return ( mElementStyle && mElementStyle->getPropertyById( propId ) ) ||
		   ( mDefinition && mDefinition->getProperty( propId ) );
}

CSS::StyleSheetProperty* UIStyle::getInheritedProperty( CSS::PropertyId propId,
														UIStyle** ownerStyle ) const {
	if ( ownerStyle )
		*ownerStyle = nullptr;

	Node* parentNode = mWidget->getParent();
	while ( parentNode && parentNode->isWidget() ) {
		UIWidget* parent = parentNode->asType<UIWidget>();
		UIStyle* parentStyle = parent->getUIStyle();
		if ( parentStyle ) {
			auto prop = parentStyle->getLocalProperty( propId );
			if ( prop ) {
				if ( ownerStyle )
					*ownerStyle = parentStyle;
				return prop;
			}
		}
		parentNode = parent->getParent();
	}
	return nullptr;
}

void UIStyle::subscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.insert( widget );
}

void UIStyle::unsubscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.erase( widget );
}

void UIStyle::applyLightDarkValue( std::string& value ) {
	std::string::size_type tokenStart = 0;
	std::string::size_type tokenEnd = 0;

	while ( true ) {
		tokenStart = value.find( "light-dark(", tokenStart );
		if ( tokenStart != std::string::npos ) {
			tokenEnd = String::findCloseBracket( value, tokenStart, '(', ')' );
			if ( tokenEnd != std::string::npos ) {
				auto fn( value.substr( tokenStart, tokenEnd - tokenStart + 1 ) );
				auto function( FunctionString::parse( fn ) );
				auto size = function.getParameters().size();
				if ( size > 0 ) {
					String::replaceAll(
						value, fn,
						function.getParameters()
							[size == 1 || mWidget->getUISceneNode()->getColorSchemePreference() ==
											  ColorSchemePreference::Light
								 ? 0
								 : 1] );
				}
				tokenStart = tokenEnd;
			} else {
				break;
			}
		} else {
			break;
		}
	};
}

void UIStyle::setVariableFromValue( StyleSheetProperty* property ) {
	if ( property->getVarCache().empty() && !property->isLightDarkValue() )
		return;

	std::string& value = property->mutableValue();
	static constexpr int maxDepth = 16;

	for ( int depth = 0; depth < maxDepth; depth++ ) {
		bool changed = false;
		if ( !property->getVarCache().empty() ) {
			const auto varCache = property->getVarCache();
			for ( const auto& var : varCache ) {
				for ( const auto& val : var.variableList ) {
					const StyleSheetVariable* variable = getVariableRef( val );
					if ( nullptr != variable ) {
						String::replaceAll( value, var.definition, variable->getValue() );
						changed = true;
						break;
					}
				}
			}
		}
		if ( property->isLightDarkValue() ) {
			applyLightDarkValue( value );
			changed = true;
		}
		if ( !changed )
			break;
		property->setValue( value );
		if ( property->getVarCache().empty() && !property->isLightDarkValue() )
			break;
	}
}

void UIStyle::applyLightDarkValues( CSS::StyleSheetProperty* style ) {}

void UIStyle::applyVarValues( StyleSheetProperty* property ) {
	if ( property->needsValueSubstitution() ) {
		if ( NULL != property->getPropertyDefinition() &&
			 property->getPropertyDefinition()->isIndexed() &&
			 property->getPropertyIndexCount() > 0 ) {
			for ( size_t i = 0; i < property->getPropertyIndexCount(); i++ ) {
				StyleSheetProperty* realProperty = property->getPropertyIndexRef( i );
				setVariableFromValue( realProperty );
			}
		} else {
			setVariableFromValue( property );
		}
	}
}

void UIStyle::onStateChange() {
	if ( NULL == mWidget || NULL == mWidget->getUISceneNode() )
		return;

	mChangingState = true;

	auto prevDefinition = mDefinition;
	auto newDefinition =
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

					for ( PropertyId id : propertiesInBothDefinitions ) {
						const StyleSheetProperty* p0 = mDefinition->getProperty( id );
						const StyleSheetProperty* p1 = newDefinition->getProperty( id );
						if ( nullptr != p0 && nullptr != p1 && *p0 == *p1 )
							changedProperties.erase( id );
					}
				}
			}
		}

		if ( mElementStyle ) {
			// Local raw values can stay unchanged while their resolved var()/light-dark() value
			// changes after a stylesheet update, so include them after definition pruning.
			for ( const auto& localProperty : mElementStyle->getProperties() ) {
				if ( localProperty.second.needsValueSubstitution() &&
					 localProperty.second.getPropertyDefinition() )
					changedProperties.insert(
						localProperty.second.getPropertyDefinition()->getPropertyId() );
			}
		}

		mDefinition = newDefinition;

		mForceReapplyProperties = false;

		mWidget->beginAttributesTransaction();

		for ( PropertyId prop : changedProperties ) {
			auto resolvedProperty = getResolvedLocalProperty( prop );
			const StyleSheetProperty* property = resolvedProperty.get();

			if ( nullptr == property || NULL == property->getPropertyDefinition() ) {
				const auto def = StyleSheetSpecification::instance()->getProperty( prop );
				if ( def && def->isInherited() ) {
					UIStyle* inheritedStyle = nullptr;
					StyleSheetProperty* inheritedProp =
						getInheritedProperty( prop, &inheritedStyle );
					if ( inheritedProp ) {
						auto resolvedInheritedProperty =
							inheritedStyle->resolveProperty( inheritedProp );
						mWidget->applyProperty( *resolvedInheritedProperty.get() );
						mWidget->propagateInheritedProperty( *resolvedInheritedProperty.get() );
					}
				}
				continue;
			}

			// Resolve "inherit" keyword to parent's value
			if ( property->getValue() == "inherit" &&
				 !property->getPropertyDefinition()->isIndexed() ) {
				UIStyle* inheritedStyle = nullptr;
				StyleSheetProperty* inheritedProp = getInheritedProperty( prop, &inheritedStyle );
				if ( inheritedProp ) {
					if ( property->getPropertyDefinition()->getPropertyId() ==
						 PropertyId::FontSize ) {
						Node* parentNode = mWidget->getParent();
						if ( parentNode && parentNode->isWidget() ) {
							Float parentPxSize =
								mWidget->getAbsoluteFontSize( parentNode->asType<UIWidget>() );
							StyleSheetProperty resolved(
								property->getPropertyDefinition(),
								String::fromFloat( PixelDensity::pxToDp( parentPxSize ), "dp" ),
								property->getIndex() );
							mWidget->applyProperty( resolved );
							mWidget->propagateInheritedProperty( resolved );
						}
					} else {
						auto resolvedInheritedProperty =
							inheritedStyle->resolveProperty( inheritedProp );
						mWidget->applyProperty( *resolvedInheritedProperty.get() );
						mWidget->propagateInheritedProperty( *resolvedInheritedProperty.get() );
					}
				}
			} else {
				if ( property->getPropertyDefinition()->isIndexed() ) {
					for ( size_t i = 0; i < property->getPropertyIndexCount(); i++ ) {
						applyStyleSheetProperty( property->getPropertyIndex( i ), prevDefinition );
					}
				} else {
					applyStyleSheetProperty( *property, prevDefinition );
				}

				if ( property->getPropertyDefinition()->isInherited() )
					mWidget->propagateInheritedProperty( *property );
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
UIStyle::getStatelessStyleSheetProperty( const PropertyId& propertyId ) const {
	if ( propertyId == PropertyId::Invalid )
		return nullptr;

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

void UIStyle::applyStyleSheetProperty( const StyleSheetProperty& originalProperty,
									   std::shared_ptr<ElementDefinition> prevDefinition ) {
	auto resolvedProperty = resolveProperty( &originalProperty );
	const StyleSheetProperty* property = resolvedProperty.get();

	const PropertyDefinition* propertyDefinition = property->getPropertyDefinition();

	// Save default value if possible and not available.
	if ( mCurrentState != UIState::StateFlagNormal ||
		 ( mCurrentState == UIState::StateFlagNormal && property->isVolatile() ) ) {
		const StyleSheetProperty* oldAttribute =
			getStatelessStyleSheetProperty( property->getPropertyId() );
		if ( nullptr == oldAttribute && getPreviousState() == UIState::StateFlagNormal ) {
			std::string value(
				mWidget->getPropertyString( propertyDefinition, property->getIndex() ) );
			if ( !value.empty() ) {
				setStyleSheetProperty( StyleSheetProperty( propertyDefinition, value,
														   property->getIndex(), true, true ) );
			}
		}
	}

	if ( !mDisableAnimations && !mFirstState && !mWidget->isSceneNodeLoading() &&
		 NULL != propertyDefinition &&
		 StyleSheetPropertyAnimation::animationSupported( propertyDefinition->getType() ) &&
		 hasTransition( property->getName() ) && !hasAnimation( propertyDefinition ) ) {
		std::string currentValue =
			mWidget->getPropertyString( propertyDefinition, property->getIndex() );
		std::string startValue( currentValue );

		if ( !startValue.empty() ) {
			// Get the real start value
			if ( nullptr != prevDefinition ) {
				auto prevProp = prevDefinition->getProperty( property->getPropertyId() );
				if ( nullptr != prevProp ) {
					auto resolvedPrevProperty = resolveProperty( prevProp );
					const StyleSheetProperty* curProperty = resolvedPrevProperty.get();
					if ( propertyDefinition->isIndexed() &&
						 property->getIndex() < curProperty->getPropertyIndexCount() ) {
						startValue =
							curProperty->getPropertyIndex( property->getIndex() ).getValue();
					} else {
						startValue = curProperty->getValue();
					}
				}
			}

			const ComputedTransitionDefinition* transitionInfo =
				mDefinition ? mDefinition->getTransitions().get( propertyDefinition->getId() )
							: nullptr;
			if ( nullptr == transitionInfo ) {
				mWidget->applyProperty( *property );
				return;
			}

			SmallVector<Action*, 4> previousTransitions;
			mWidget->getActionsByTag( propertyDefinition->getId(), previousTransitions );
			SmallVector<Action*, 4> removeTransitions;
			StyleSheetPropertyAnimation* prevTransition = NULL;

			if ( !previousTransitions.empty() ) {
				for ( auto& transition : previousTransitions ) {
					if ( transition->getId() == StyleSheetPropertyAnimation::ID ) {
						StyleSheetPropertyAnimation* tmpTransition =
							static_cast<StyleSheetPropertyAnimation*>( transition );
						if ( tmpTransition->getAnimationOrigin() == AnimationOrigin::Transition ) {
							if ( propertyDefinition->isIndexed() ) {
								if ( tmpTransition->getPropertyIndex() == property->getIndex() ) {
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
				if ( prevTransition->getEndValue() == property->getValue() ) {
					return;
				} else if ( prevTransition->getStartValue() == property->getValue() ) {
					Float currentProgress = prevTransition->getCurrentProgress();
					currentProgress = eemin( 1.f, currentProgress );
					if ( 0.f != currentProgress ) {
						elapsed = Milliseconds( ( 1.f - currentProgress ) *
												transitionInfo->duration.asMilliseconds() );
					} else {
						elapsed = transitionInfo->duration;
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
				propertyDefinition, startValue, property->getValue(), property->getIndex(),
				transitionInfo->duration, transitionInfo->delay, transitionInfo->timingFunction,
				transitionInfo->timingFunctionParameters, AnimationOrigin::Transition );
			newTransition->setElapsed( elapsed );
			newTransition->setTag( propertyDefinition->getId() );
			mWidget->runAction( newTransition );
		} else {
			mWidget->applyProperty( *property );
		}
	} else {
		mWidget->applyProperty( *property );
	}
}

void UIStyle::updateAnimations() {
	if ( nullptr == mDefinition )
		return;

	if ( mDefinition->getAnimationProperties().empty() ) {
		if ( !mAnimations.empty() ) {
			mAnimations.clear();
			removeAllAnimations();
		}
		return;
	}

	bool isDifferent = false;
	CSS::AnimationsMap animations =
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

	if ( isDifferent ) {
		mAnimations.clear();

		removeAllAnimations();

		startAnimations( animations );
	} else {
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
						StyleSheetProperty* prop =
							mDefinition->getProperty( propDef->getPropertyId() );
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

StyleSheetProperty* UIStyle::getLocalProperty( PropertyId propId ) {
	StyleSheetProperty* defProperty = mDefinition ? mDefinition->getProperty( propId ) : nullptr;
	StyleSheetProperty* elemProperty = nullptr;
	if ( mElementStyle ) {
		const auto* def = StyleSheetSpecification::instance()->getProperty( propId );
		if ( def )
			elemProperty = mElementStyle->getPropertyById( def->getPropertyId() );
	}
	if ( defProperty && elemProperty )
		return defProperty->getSpecificity() > elemProperty->getSpecificity() ? defProperty
																			  : elemProperty;
	return defProperty ? defProperty : elemProperty;
}

UIStyle::PropertyResolution UIStyle::resolveProperty( const StyleSheetProperty* property ) {
	if ( nullptr == property || !property->needsValueSubstitution() )
		return PropertyResolution{ nullptr, property };

	const Uint32 slotIndex = mPropertyResolutionDepth;
	StyleSheetProperty* slot;
	if ( 0 == slotIndex ) {
		if ( !mPropertyResolutionSlot )
			mPropertyResolutionSlot = std::make_unique<StyleSheetProperty>();
		slot = mPropertyResolutionSlot.get();
	} else {
		const Uint32 nestedSlotIndex = slotIndex - 1;
		if ( nestedSlotIndex >= mNestedPropertyResolutionSlots.size() )
			mNestedPropertyResolutionSlots.emplace_back( std::make_unique<StyleSheetProperty>() );
		slot = mNestedPropertyResolutionSlots[nestedSlotIndex].get();
	}

	++mPropertyResolutionDepth;
	PropertyResolution resolution{ this, slot, slotIndex };
	if ( !slot->hasSameResolutionSource( *property ) )
		*slot = *property;
	applyVarValues( slot );
	return resolution;
}

UIStyle::PropertyResolution UIStyle::getResolvedLocalProperty( PropertyId propId ) {
	return resolveProperty( getLocalProperty( propId ) );
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
