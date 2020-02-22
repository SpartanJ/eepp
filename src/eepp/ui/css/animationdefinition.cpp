#include <eepp/ui/css/animationdefinition.hpp>
#include <eepp/ui/css/propertydefinition.hpp>

namespace EE { namespace UI { namespace CSS {

bool isTimingFunction( const std::string& str ) {
	return Ease::Interpolation::None != Ease::fromName( str, Ease::Interpolation::None );
}

std::map<std::string, AnimationDefinition> AnimationDefinition::parseAnimationProperties(
	const std::vector<StyleSheetProperty>& stylesheetProperties ) {
	AnimationsMap animations;
	std::vector<std::string> names;
	std::vector<Time> durations;
	std::vector<Time> delays;
	std::vector<Int32> iterations;
	std::vector<Ease::Interpolation> timingFunctions;
	std::vector<AnimationDirection> directions;
	std::vector<AnimationFillMode> fillModes;
	std::vector<bool> pausedStates;

	for ( auto& prop : stylesheetProperties ) {
		if ( prop.getPropertyDefinition() == NULL )
			continue;

		const PropertyDefinition* propDef = prop.getPropertyDefinition();

		switch ( propDef->getPropertyId() ) {
			case PropertyId::Animation: {
				bool durationSet = false;

				for ( size_t i = 0; i < prop.getPropertyIndexCount(); i++ ) {
					const StyleSheetProperty& iProp = prop.getPropertyIndex( i );

					auto parts = String::split( iProp.getValue(), ' ' );

					if ( parts.size() >= 2 ) {
						AnimationDefinition animationDef;

						for ( auto& part : parts ) {
							std::string val( String::trim( String::toLower( part ) ) );

							if ( isDirectionString( val ) ) {
								animationDef.direction = directionFromString( val );
							} else if ( isAnimationFillModeString( val ) ) {
								animationDef.fillMode = fillModeFromString( val );
							} else if ( "infinite" == val ) {
								animationDef.iterations = -1;
							} else if ( "paused" == val ) {
								animationDef.paused = true;
							} else if ( "running" == val ) {
								animationDef.paused = false;
							} else if ( isTimingFunction( val ) ) {
								animationDef.timingFunction = Ease::fromName( val );
							} else if ( Time::isValid( val ) ) {
								if ( durationSet ) {
									animationDef.delay =
										StyleSheetProperty( "animation-delay", val ).asTime();
								} else {
									animationDef.duration =
										StyleSheetProperty( "animation-duration", val ).asTime();
									durationSet = true;
								}
							} else if ( String::isNumber( val, true ) ) {
								int iterations = 1;

								if ( String::fromString( iterations, val ) ) {
									animationDef.iterations = iterations;
								}
							} else {
								animationDef.name = part;
							}
						}

						animations[animationDef.name] = std::move( animationDef );
					}
				}
				break;
			}
			case PropertyId::AnimationName:
			case PropertyId::AnimationDelay:
			case PropertyId::AnimationDuration:
			case PropertyId::AnimationFillMode:
			case PropertyId::AnimationPlayState:
			case PropertyId::AnimationIterationCount:
			case PropertyId::AnimationTimingFunction: {
				for ( size_t i = 0; i < prop.getPropertyIndexCount(); i++ ) {
					const StyleSheetProperty& iProp = prop.getPropertyIndex( i );
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
							Ease::Interpolation interpolation =
								Ease::fromName( val, Ease::Interpolation::None );
							if ( Ease::Interpolation::None != interpolation ) {
								timingFunctions.push_back( interpolation );
							}
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
		animationDef.name = names[i];

		if ( !delays.empty() )
			animationDef.delay = delays[i % delays.size()];

		if ( !durations.empty() )
			animationDef.duration = delays[i % durations.size()];

		if ( !fillModes.empty() )
			animationDef.fillMode = fillModes[i % fillModes.size()];

		if ( !pausedStates.empty() )
			animationDef.paused = pausedStates[i % pausedStates.size()];

		if ( !iterations.empty() )
			animationDef.iterations = iterations[i % iterations.size()];

		if ( !timingFunctions.empty() )
			animationDef.timingFunction = timingFunctions[i % timingFunctions.size()];

		animations[animationDef.name] = std::move( animationDef );
	}

	return animations;
}

bool AnimationDefinition::isDirectionString( const std::string str ) {
	Uint32 id = String::hash( str );
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
	return direction;
}

const bool& AnimationDefinition::isPaused() const {
	return paused;
}

const Int32& AnimationDefinition::getIterations() const {
	return iterations;
}

const std::string& AnimationDefinition::getName() const {
	return name;
}

const Time& AnimationDefinition::getDelay() const {
	return delay;
}

const Time& AnimationDefinition::getDuration() const {
	return duration;
}

const Ease::Interpolation& AnimationDefinition::getTimingFunction() const {
	return timingFunction;
}

}}} // namespace EE::UI::CSS
