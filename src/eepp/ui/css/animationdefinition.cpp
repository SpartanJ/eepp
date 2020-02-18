#include <eepp/ui/css/animationdefinition.hpp>

namespace EE { namespace UI { namespace CSS {

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
