#ifndef EE_UI_CSS_ANIMATIONDEFINITION_HPP
#define EE_UI_CSS_ANIMATIONDEFINITION_HPP

#include <eepp/math/ease.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <map>
#include <vector>

using namespace EE::Math;
using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

class EE_API AnimationDefinition {
  public:
	/* https://developer.mozilla.org/en-US/docs/Web/CSS/animation-direction */
	enum AnimationDirection {
		Normal = String::hash( "normal" ),
		Reverse = String::hash( "reverse" ),
		Alternate = String::hash( "alternate" ),
		AlternateReverse = String::hash( "alternate-reverse" )
	};

	static AnimationDirection directionFromString( std::string str );

	/* https://developer.mozilla.org/en-US/docs/Web/CSS/animation-fill-mode */
	enum AnimationFillMode {
		None = String::hash( "none" ),
		Forwards = String::hash( "forwards" ),
		Backwards = String::hash( "backwards" ),
		Both = String::hash( "both" )
	};

	static AnimationFillMode fillModeFromString( std::string str );

	AnimationDefinition();

	const AnimationDirection& getDirection() const;

	const bool& isPaused() const;

	const Int32& getIterations() const;

	const std::string& getName() const;

	const Time& getDelay() const;

	const Time& getDuration() const;

	const Ease::Interpolation& getTimingFunction() const;

	std::string name;
	Time delay = Time::Zero;
	Time duration = Time::Zero;
	Int32 iterations = 1; /* -1 == "infinite" */
	Ease::Interpolation timingFunction = Ease::Interpolation::Linear;
	AnimationDirection direction = Normal;
	bool paused = false;
};

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_ANIMATIONDEFINITION_HPP
