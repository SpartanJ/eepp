#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/animationdefinition.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/timingfunction.hpp>

namespace EE { namespace UI { namespace CSS {

inline bool isTimingFunction( const std::string& str ) {
	return Ease::Interpolation::None != Ease::fromName( str, Ease::Interpolation::None );
}

UnorderedMap<std::string, AnimationDefinition> AnimationDefinition::parseAnimationProperties(
	const std::vector<const StyleSheetProperty*>& stylesheetProperties ) {
	AnimationsMap animations;
	std::vector<std::string> names;
	std::vector<Time> durations;
	std::vector<Time> delays;
	std::vector<Int32> iterations;
	std::vector<Ease::Interpolation> timingFunctions;
	std::vector<std::vector<double>> timingFunctionParameters;
	std::vector<AnimationDirection> directions;
	std::vector<AnimationFillMode> fillModes;
	std::vector<bool> pausedStates;

	for ( auto& prop : stylesheetProperties ) {
		if ( prop->getPropertyDefinition() == NULL )
			continue;

		const PropertyDefinition* propDef = prop->getPropertyDefinition();

		switch ( propDef->getPropertyId() ) {
			case PropertyId::Animation: {
				bool durationSet = false;

				for ( size_t i = 0; i < prop->getPropertyIndexCount(); i++ ) {
					const StyleSheetProperty& iProp = prop->getPropertyIndex( i );

					auto parts = String::split( iProp.getValue(), ' ' );

					if ( parts.size() >= 2 ) {
						AnimationDefinition animationDef;

						for ( auto& part : parts ) {
							std::string val( String::trim( String::toLower( part ) ) );

							if ( isDirectionString( val ) ) {
								animationDef.setDirection( directionFromString( val ) );
							} else if ( isAnimationFillModeString( val ) ) {
								animationDef.setFillMode( fillModeFromString( val ) );
							} else if ( "infinite" == val ) {
								animationDef.setIterations( -1 );
							} else if ( "paused" == val ) {
								animationDef.setPaused( true );
							} else if ( "running" == val ) {
								animationDef.setPaused( false );
							} else if ( isTimingFunction( val ) ) {
								TimingFunction tf( TimingFunction::parse( val ) );
								animationDef.setTimingFunction( tf.interpolation );
								animationDef.setTimingFunctionParameters( tf.parameters );
							} else if ( Time::isValid( val ) ) {
								if ( durationSet ) {
									animationDef.setDelay( Time::fromString( val ) );
								} else {
									animationDef.setDuration( Time::fromString( val ) );
									durationSet = true;
								}
							} else if ( String::isNumber( val, true ) ) {
								int iterations = 1;

								if ( String::fromString( iterations, val ) ) {
									animationDef.setIterations( iterations );
								}
							} else {
								animationDef.setName( part );
							}
						}

						animations[animationDef.getName()] = std::move( animationDef );
					}
				}
				return animations;
				break;
			}
			case PropertyId::AnimationName:
			case PropertyId::AnimationDelay:
			case PropertyId::AnimationDuration:
			case PropertyId::AnimationFillMode:
			case PropertyId::AnimationPlayState:
			case PropertyId::AnimationIterationCount:
			case PropertyId::AnimationTimingFunction: {
				for ( size_t i = 0; i < prop->getPropertyIndexCount(); i++ ) {
					const StyleSheetProperty& iProp = prop->getPropertyIndex( i );
					std::string val( String::trim( String::toLower( iProp.getValue() ) ) );
					switch ( propDef->getPropertyId() ) {
						case PropertyId::AnimationName:
							names.push_back( iProp.getValue() );
							break;
						case PropertyId::AnimationDelay:
							delays.push_back( Time::fromString( val ) );
							break;
						case PropertyId::AnimationDuration:
							durations.push_back( Time::fromString( val ) );
							break;
						case PropertyId::AnimationFillMode:
							fillModes.push_back( fillModeFromString( val ) );
							break;
						case PropertyId::AnimationPlayState:
							pausedStates.push_back( val == "paused" ? true : false );
							break;
						case PropertyId::AnimationIterationCount: {
							int iVal;
							if ( val == "infinite" ) {
								iterations.push_back( -1 );
							} else if ( String::fromString( iVal, val ) && iVal >= -1 ) {
								iterations.push_back( iVal );
							}
							break;
						}
						case PropertyId::AnimationTimingFunction: {
							TimingFunction tf( TimingFunction::parse( val ) );
							timingFunctions.emplace_back( tf.interpolation );
							timingFunctionParameters.emplace_back( tf.parameters );
							break;
						}
						default:
							break;
					}
				}
				break;
			}
			default:
				break;
		}
	}

	for ( size_t i = 0; i < names.size(); i++ ) {
		AnimationDefinition animationDef;
		animationDef.setName( names[i] );

		if ( !delays.empty() )
			animationDef.setDelay( delays[i % delays.size()] );

		if ( !durations.empty() )
			animationDef.setDuration( delays[i % durations.size()] );

		if ( !fillModes.empty() )
			animationDef.setFillMode( fillModes[i % fillModes.size()] );

		if ( !pausedStates.empty() )
			animationDef.setPaused( pausedStates[i % pausedStates.size()] );

		if ( !iterations.empty() )
			animationDef.setIterations( iterations[i % iterations.size()] );

		if ( !timingFunctions.empty() )
			animationDef.setTimingFunction( timingFunctions[i % timingFunctions.size()] );

		if ( !timingFunctionParameters.empty() )
			animationDef.setTimingFunctionParameters(
				timingFunctionParameters[i % timingFunctionParameters.size()] );

		animations[animationDef.getName()] = std::move( animationDef );
	}

	return animations;
}

bool AnimationDefinition::isDirectionString( const std::string str ) {
	String::HashType id = String::hash( str );
	return id == AlternateReverse || id == Alternate || id == Reverse || id == Normal;
}

AnimationDefinition::AnimationDirection
AnimationDefinition::directionFromString( std::string str ) {
	String::trimInPlace( str );
	String::toLowerInPlace( str );
	switch ( String::hash( str ) ) {
		case AlternateReverse:
			return AlternateReverse;
		case Alternate:
			return Alternate;
		case Reverse:
			return Reverse;
		case Normal:
		default:
			return Normal;
	}
}

bool AnimationDefinition::isAnimationFillModeString( const std::string& str ) {
	Uint32 id = String::hash( str );
	return id == None || id == Forwards || id == Backwards || id == Both;
}

AnimationDefinition::AnimationFillMode AnimationDefinition::fillModeFromString( std::string str ) {
	String::trimInPlace( str );
	String::toLowerInPlace( str );
	switch ( String::hash( str ) ) {
		case None:
			return None;
		case Forwards:
			return Forwards;
		case Backwards:
			return Backwards;
		case Both:
		default:
			return Both;
	}
}

AnimationDefinition::AnimationDefinition() {}

const AnimationDefinition::AnimationDirection& AnimationDefinition::getDirection() const {
	return mDirection;
}

const bool& AnimationDefinition::isPaused() const {
	return mPaused;
}

const Int32& AnimationDefinition::getIterations() const {
	return mIterations;
}

const std::string& AnimationDefinition::getName() const {
	return mName;
}

const Time& AnimationDefinition::getDelay() const {
	return mDelay;
}

const Time& AnimationDefinition::getDuration() const {
	return mDuration;
}

const Ease::Interpolation& AnimationDefinition::getTimingFunction() const {
	return mTimingFunction;
}

void AnimationDefinition::setName( const std::string& value ) {
	mName = value;
	mId = String::hash( mName );
}

void AnimationDefinition::setFillMode( const AnimationFillMode& value ) {
	mFillMode = value;
}

void AnimationDefinition::setPaused( bool value ) {
	mPaused = value;
}

const String::HashType& AnimationDefinition::getId() const {
	return mId;
}

const std::vector<double>& AnimationDefinition::getTimingFunctionParameters() const {
	return mTimingFunctionParameters;
}

void AnimationDefinition::setTimingFunctionParameters(
	const std::vector<double>& timingFunctionParameters ) {
	mTimingFunctionParameters = timingFunctionParameters;
}

const AnimationDefinition::AnimationFillMode& AnimationDefinition::getFillMode() const {
	return mFillMode;
}

void AnimationDefinition::setDirection( const AnimationDirection& value ) {
	mDirection = value;
}

void AnimationDefinition::setTimingFunction( const Ease::Interpolation& value ) {
	mTimingFunction = value;
}

void AnimationDefinition::setIterations( const Int32& value ) {
	mIterations = value;
}

void AnimationDefinition::setDuration( const Time& value ) {
	mDuration = value;
}

void AnimationDefinition::setDelay( const Time& value ) {
	mDelay = value;
}

}}} // namespace EE::UI::CSS
