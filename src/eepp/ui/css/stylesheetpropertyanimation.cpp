#include <algorithm>
#include <eepp/math/easing.hpp>
#include <eepp/ui/border.hpp>
#include <eepp/ui/css/stylesheetpropertyanimation.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::Math::easing;

namespace EE { namespace UI { namespace CSS {

inline Float easingFn( const Ease::Interpolation& timingFunction,
					   const std::vector<double>& timingFunctionParameters, const double& t,
					   const double& b, const double& c, const double& d ) {
	if ( timingFunction != Ease::Interpolation::CubizBezier )
		return easingCb[timingFunction]( t, b, c, d );

	if ( timingFunctionParameters.size() == 4 )
		return cubicBezierInterpolation( timingFunctionParameters[0], timingFunctionParameters[1],
										 timingFunctionParameters[2], timingFunctionParameters[3],
										 t );

	return t;
}

void StyleSheetPropertyAnimation::tweenProperty( UIWidget* widget, const Float& normalizedProgress,
												 const PropertyDefinition* property,
												 const std::string& startValue,
												 const std::string& endValue,
												 const Ease::Interpolation& timingFunction,
												 const std::vector<double> timingFunctionParameters,
												 const Uint32& propertyIndex, const bool& isDone ) {
	switch ( property->getType() ) {
		case PropertyType::NumberFloat:
		case PropertyType::NumberInt: {
			Float start = widget->convertLength( startValue, 0 );
			Float end = widget->convertLength( endValue, 0 );
			Float value = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
									start, end - start, 1.f );
			if ( property->getType() == PropertyType::NumberFloat ) {
				widget->applyProperty(
					StyleSheetProperty( property, String::fromFloat( value ), propertyIndex ) );
			} else {
				widget->applyProperty( StyleSheetProperty(
					property, String::format( "%d", static_cast<int>( value ) ), propertyIndex ) );
			}
			break;
		}
		case PropertyType::Color: {
			Color startColor( startValue );
			Color endColor( endValue );
			if ( startColor.getValue() == 0 ) {
				startColor = endColor;
				startColor.a = 0;
			}
			if ( endColor.getValue() == 0 ) {
				endColor = startColor;
				endColor.a = 0;
			}
			Float progress =
				easingFn( timingFunction, timingFunctionParameters, normalizedProgress, 0, 1, 1.f );
			Color resColor( startColor );
			resColor.r = static_cast<Uint8>( eemin(
				static_cast<Int32>( startColor.r + ( endColor.r - startColor.r ) * progress ),
				255 ) );
			resColor.g = static_cast<Uint8>( eemin(
				static_cast<Int32>( startColor.g + ( endColor.g - startColor.g ) * progress ),
				255 ) );
			resColor.b = static_cast<Uint8>( eemin(
				static_cast<Int32>( startColor.b + ( endColor.b - startColor.b ) * progress ),
				255 ) );
			resColor.a = static_cast<Uint8>( eemin(
				static_cast<Int32>( startColor.a + ( endColor.a - startColor.a ) * progress ),
				255 ) );
			widget->applyProperty(
				StyleSheetProperty( property, resColor.toHexString(), propertyIndex ) );
			break;
		}
		case PropertyType::NumberLength: {
			Float containerLength = widget->getPropertyRelativeTargetContainerLength(
				property->getRelativeTarget(), 0.f, propertyIndex );
			Float start = widget->convertLength( startValue, containerLength );
			Float end = widget->convertLength( endValue, containerLength );
			Float value = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
									start, end - start, 1.f );
			widget->applyProperty(
				StyleSheetProperty( property, String::fromFloat( value, "px" ), propertyIndex ) );

			if ( isDone ) {
				widget->applyProperty( StyleSheetProperty( property, endValue, propertyIndex ) );
			}
			break;
		}
		case PropertyType::RadiusLength: {
			Sizef start( Borders::radiusFromString( widget, startValue ) );
			Sizef end( Borders::radiusFromString( widget, endValue ) );
			Float x = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
								start.x, end.x - start.x, 1.f );
			Float y = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
								start.y, end.y - start.y, 1.f );
			widget->applyProperty( StyleSheetProperty(
				property, String::fromFloat( x ) + " " + String::fromFloat( y ), propertyIndex ) );
			if ( isDone ) {
				widget->applyProperty( StyleSheetProperty( property, endValue, propertyIndex ) );
			}
			break;
		}
		case PropertyType::Vector2: {
			Vector2f start( StyleSheetProperty( property, startValue ).asVector2f() );
			Vector2f end( StyleSheetProperty( property, endValue ).asVector2f() );
			Float x = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
								start.x, end.x - start.x, 1.f );
			Float y = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
								start.y, end.y - start.y, 1.f );
			widget->applyProperty( StyleSheetProperty(
				property, String::fromFloat( x ) + " " + String::fromFloat( y ), propertyIndex ) );
			if ( isDone ) {
				widget->applyProperty( StyleSheetProperty( property, endValue, propertyIndex ) );
			}
			break;
		}
		case PropertyType::BackgroundSize: {
			Sizef start( widget->getBackground()
							 ->getLayer( propertyIndex )
							 ->calcDrawableSize( startValue ) );
			Sizef end(
				widget->getBackground()->getLayer( propertyIndex )->calcDrawableSize( endValue ) );
			Float x = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
								start.x, end.x - start.x, 1.f );
			Float y = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
								start.y, end.y - start.y, 1.f );
			widget->applyProperty( StyleSheetProperty(
				property, String::fromFloat( x, "px" ) + " " + String::fromFloat( y, "px" ),
				propertyIndex ) );
			if ( isDone ) {
				widget->applyProperty( StyleSheetProperty( property, endValue, propertyIndex ) );
			}
			break;
		}
		case PropertyType::ForegroundSize: {
			Sizef start( widget->getForeground()
							 ->getLayer( propertyIndex )
							 ->calcDrawableSize( startValue ) );
			Sizef end(
				widget->getForeground()->getLayer( propertyIndex )->calcDrawableSize( endValue ) );
			Float x = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
								start.x, end.x - start.x, 1.f );
			Float y = easingFn( timingFunction, timingFunctionParameters, normalizedProgress,
								start.y, end.y - start.y, 1.f );
			widget->applyProperty( StyleSheetProperty(
				property, String::fromFloat( x, "px" ) + " " + String::fromFloat( y, "px" ),
				propertyIndex ) );
			if ( isDone ) {
				widget->applyProperty( StyleSheetProperty( property, endValue, propertyIndex ) );
			}
			break;
		}
		default:
			break;
	}
}

StyleSheetPropertyAnimation* StyleSheetPropertyAnimation::fromAnimationKeyframes(
	const AnimationDefinition& animation, const KeyframesDefinition& keyframes,
	const PropertyDefinition* propertyDef, UIWidget* widget, const Uint32& propertyIndex,
	const AnimationOrigin& ) {
	std::vector<std::string> properties;
	std::vector<Float> times;

	for ( auto& blockIt : keyframes.getKeyframeBlocks() ) {
		const KeyframesDefinition::KeyframeBlock& block = blockIt.second;
		auto propIt = block.properties.find( propertyDef->getId() );

		if ( propIt != block.properties.end() ) {
			properties.push_back( propIt->second.getValue() );
		} else {
			if ( properties.empty() ) {
				// Get the start value from the widget.
				properties.push_back( widget->getPropertyString( propertyDef, propertyIndex ) );
			} else {
				// If already exists get the start value from the previous keyframe block.
				properties.push_back( properties[properties.size() - 1] );
			}
		}

		times.push_back( block.normalizedTime );
	}

	return New( animation, propertyDef, properties, times, propertyIndex,
				AnimationOrigin::Animation );
}

bool StyleSheetPropertyAnimation::animationSupported( const PropertyType& type ) {
	switch ( type ) {
		case PropertyType::NumberFloat:
		case PropertyType::NumberInt:
		case PropertyType::NumberLength:
		case PropertyType::Color:
		case PropertyType::Vector2:
		case PropertyType::BackgroundSize:
		case PropertyType::ForegroundSize:
		case PropertyType::RadiusLength:
			return true;
		default:
			return false;
	}
}

StyleSheetPropertyAnimation* StyleSheetPropertyAnimation::New(
	const AnimationDefinition& animation, const PropertyDefinition* propertyDef,
	std::vector<std::string> states, std::vector<Float> animationStepsTime,
	const Uint32& propertyIndex, const AnimationOrigin& animationOrigin ) {
	return eeNew( StyleSheetPropertyAnimation, ( animation, propertyDef, states, animationStepsTime,
												 propertyIndex, animationOrigin ) );
}

StyleSheetPropertyAnimation* StyleSheetPropertyAnimation::New(
	const PropertyDefinition* property, const std::string& startValue, const std::string& endValue,
	const Uint32& propertyIndex, const Time& duration, const Time& delay,
	const Ease::Interpolation& timingFunction, const std::vector<double>& timingFunctionParameters,
	const AnimationOrigin& animationOrigin ) {
	AnimationDefinition animation;
	animation.setDelay( delay );
	animation.setDuration( duration );
	animation.setTimingFunction( timingFunction );
	animation.setTimingFunctionParameters( timingFunctionParameters );
	return New( animation, property, { startValue, endValue }, { 0, 1 }, propertyIndex,
				animationOrigin );
}

StyleSheetPropertyAnimation::StyleSheetPropertyAnimation( const AnimationDefinition& animation,
														  const PropertyDefinition* propertyDef,
														  std::vector<std::string> states,
														  std::vector<Float> animationStepsTime,
														  const Uint32& propertyIndex,
														  const AnimationOrigin& animationOrigin ) :
	mAnimation( animation ),
	mPropertyDef( propertyDef ),
	mStates( states ),
	mAnimationStepsTime( animationStepsTime ),
	mPendingIterations( animation.getIterations() ),
	mPropertyIndex( propertyIndex ),
	mAnimationOrigin( animationOrigin ),
	mPaused( mAnimation.isPaused() ) {
	mId = ID;
}

void StyleSheetPropertyAnimation::start() {
	onStart();

	sendEvent( ActionType::OnStart );
}

void StyleSheetPropertyAnimation::stop() {
	onStop();

	sendEvent( ActionType::OnStop );
}

void StyleSheetPropertyAnimation::update( const Time& time ) {
	if ( mPaused )
		return;

	mRealElapsed += time;

	bool wasDone = false;

	if ( mRealElapsed >= mAnimation.getDelay() ) {
		mElapsed += time;

		if ( mPendingIterations > 0 ) {
			while ( mElapsed > mAnimation.getDuration() ) {
				if ( mPendingIterations > 0 ) {
					mPendingIterations--;

					if ( mPendingIterations > 0 ) {
						wasDone = true;
						mElapsed -= mAnimation.getDuration();
					} else {
						mElapsed = mAnimation.getDuration();
					}
				} else {
					break;
				}
			}
		} else if ( mPendingIterations == -1 ) {
			while ( mElapsed > mAnimation.getDuration() ) {
				mElapsed -= mAnimation.getDuration();
				wasDone = true;
			}
		}

		if ( wasDone && ( mPendingIterations > 0 || mPendingIterations == -1 ) ) {
			if ( mAnimation.getDirection() == AnimationDefinition::AnimationDirection::Alternate ||
				 mAnimation.getDirection() ==
					 AnimationDefinition::AnimationDirection::AlternateReverse ) {
				reverseAnimation();
			}
		}

		onUpdate( time );

		if ( isDone() ) {
			notifyClose();
		}
	}
}

bool StyleSheetPropertyAnimation::isDone() {
	return mElapsed.asMicroseconds() >= mAnimation.getDuration().asMicroseconds() &&
		   ( mPendingIterations == 0 || mPendingIterations != -1 );
}

Float StyleSheetPropertyAnimation::getCurrentProgress() {
	return eemin( mElapsed.asMilliseconds() / mAnimation.getDuration().asMilliseconds(), 1. );
}

Time StyleSheetPropertyAnimation::getTotalTime() {
	return mAnimation.getDuration();
}

Action* StyleSheetPropertyAnimation::clone() const {
	return New( mAnimation, mPropertyDef, mStates, mAnimationStepsTime, mPropertyIndex,
				mAnimationOrigin );
}

Action* StyleSheetPropertyAnimation::reverse() const {
	std::vector<std::string> vcopy( mStates );
	std::reverse( vcopy.begin(), vcopy.end() );
	return New( mAnimation, mPropertyDef, vcopy, mAnimationStepsTime, mPropertyIndex,
				mAnimationOrigin );
}

const Uint32& StyleSheetPropertyAnimation::getPropertyIndex() const {
	return mPropertyIndex;
}

const std::string& StyleSheetPropertyAnimation::getStartValue() const {
	return mStates[0];
}

const std::string& StyleSheetPropertyAnimation::getEndValue() const {
	return mStates[mStates.size() - 1];
}

void StyleSheetPropertyAnimation::onStart() {
	if ( mRealElapsed >= mAnimation.getDelay() ) {
		onUpdate( Time::Zero );
	}
}

void StyleSheetPropertyAnimation::onUpdate( const Time& ) {
	if ( NULL != mNode && mNode->isWidget() ) {
		UIWidget* widget = mNode->asType<UIWidget>();

		Int32 curPos = 1;
		Float normalizedProgress = getCurrentProgress();

		for ( size_t i = 1; i < mAnimationStepsTime.size(); i++ ) {
			if ( normalizedProgress >= mAnimationStepsTime[i - 1] &&
				 normalizedProgress <= mAnimationStepsTime[i] ) {
				curPos = i;
				break;
			}
		}

		if ( curPos - 1 >= 0 && curPos < static_cast<Int32>( mStates.size() ) ) {
			Float relTime = mAnimationStepsTime[curPos] - mAnimationStepsTime[curPos - 1];
			Float curTime = normalizedProgress - mAnimationStepsTime[curPos - 1];
			Float relativeProgress = curTime / relTime;
			tweenProperty( widget, relativeProgress, mPropertyDef, mStates[curPos - 1],
						   mStates[curPos], mAnimation.getTimingFunction(),
						   mAnimation.getTimingFunctionParameters(), mPropertyIndex, isDone() );
		}
	}
}

void StyleSheetPropertyAnimation::onTargetChange() {
	if ( NULL != mNode && mNode->isWidget() && mAnimationOrigin == AnimationOrigin::Animation ) {
		if ( mAnimation.getFillMode() == AnimationDefinition::AnimationFillMode::None ) {
			UIWidget* widget = mNode->asType<UIWidget>();
			mFillModeValue = widget->getPropertyString( mPropertyDef, mPropertyIndex );
		} else if ( mAnimation.getFillMode() == AnimationDefinition::AnimationFillMode::Forwards ) {
			if ( mStates.empty() )
				return;
			switch ( mAnimation.getDirection() ) {
				case AnimationDefinition::AnimationDirection::Normal: {
					mFillModeValue = mStates[mStates.size() - 1];
					break;
				}
				case AnimationDefinition::AnimationDirection::Reverse: {
					mFillModeValue = mStates[0];
					break;
				}
				case AnimationDefinition::AnimationDirection::Alternate: {
					if ( mAnimation.getIterations() % 2 == 0 ) {
						mFillModeValue = mStates[0];
					} else {
						mFillModeValue = mStates[mStates.size() - 1];
					}
					break;
				}
				case AnimationDefinition::AnimationDirection::AlternateReverse: {
					if ( mAnimation.getIterations() % 2 == 0 ) {
						mFillModeValue = mStates[mStates.size() - 1];
					} else {

						mFillModeValue = mStates[0];
					}
					break;
				}
			}
		} else if ( mAnimation.getFillMode() ==
					AnimationDefinition::AnimationFillMode::Backwards ) {
			if ( mStates.empty() )
				return;
			if ( mAnimation.getDirection() == AnimationDefinition::AnimationDirection::Normal ||
				 mAnimation.getDirection() == AnimationDefinition::AnimationDirection::Alternate ) {
				mFillModeValue = mStates[0];
			} else {
				mFillModeValue = mStates[mStates.size() - 1];
			}
		}
	}
}

void StyleSheetPropertyAnimation::setElapsed( const Time& elapsed ) {
	mElapsed = elapsed;

	if ( mPendingIterations > 0 ) {
		while ( mElapsed > mAnimation.getDuration() ) {
			if ( mPendingIterations > 0 ) {
				mPendingIterations--;
				mElapsed = mAnimation.getDuration() - mElapsed;
			} else {
				break;
			}
		}
	}
}

const AnimationOrigin& StyleSheetPropertyAnimation::getAnimationOrigin() const {
	return mAnimationOrigin;
}

void StyleSheetPropertyAnimation::setRunning( const bool& running ) {
	mPaused = !running;
}

void StyleSheetPropertyAnimation::setPaused( const bool& paused ) {
	mPaused = paused;
	if ( !mPaused )
		onUpdate( Time::Zero );
}

void StyleSheetPropertyAnimation::notifyClose() {
	if ( mAnimationOrigin == AnimationOrigin::Animation && NULL != mNode && mNode->isWidget() ) {
		if ( mAnimation.getFillMode() != AnimationDefinition::AnimationFillMode::Both ) {
			UIWidget* widget = mNode->asType<UIWidget>();
			widget->applyProperty(
				StyleSheetProperty( mPropertyDef, mFillModeValue, mPropertyIndex ) );
		}
	}
}

const AnimationDefinition& StyleSheetPropertyAnimation::getAnimation() const {
	return mAnimation;
}

const Time& StyleSheetPropertyAnimation::getElapsed() const {
	return mElapsed;
}

void StyleSheetPropertyAnimation::prepareDirection() {
	if ( mAnimation.getDirection() == AnimationDefinition::AnimationDirection::Reverse ||
		 mAnimation.getDirection() == AnimationDefinition::AnimationDirection::AlternateReverse ) {
		reverseAnimation();
	}
}

void StyleSheetPropertyAnimation::reverseAnimation() {
	std::vector<std::string> reverseCopy( mStates );
	std::reverse( reverseCopy.begin(), reverseCopy.end() );
	mStates = reverseCopy;

	std::vector<Float> reverseTimes( mAnimationStepsTime );
	std::reverse( reverseTimes.begin(), reverseTimes.end() );
	for ( size_t i = 0; i < reverseTimes.size(); i++ ) {
		reverseTimes[i] = 1.f - reverseTimes[i];
	}
	mAnimationStepsTime = reverseTimes;
}

}}} // namespace EE::UI::CSS
