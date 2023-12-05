#ifndef EE_UI_CSS_ANIMATIONDEFINITION_HPP
#define EE_UI_CSS_ANIMATIONDEFINITION_HPP

#include <eepp/math/ease.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <vector>

using namespace EE::Math;
using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

class EE_API AnimationDefinition {
  public:
	static UnorderedMap<std::string, AnimationDefinition>
	parseAnimationProperties( const std::vector<const StyleSheetProperty*>& stylesheetProperties );

	/* https://developer.mozilla.org/en-US/docs/Web/CSS/animation-direction */
	enum AnimationDirection {
		Normal = String::hash( "normal" ),
		Reverse = String::hash( "reverse" ),
		Alternate = String::hash( "alternate" ),
		AlternateReverse = String::hash( "alternate-reverse" )
	};

	static bool isDirectionString( const std::string str );

	static AnimationDirection directionFromString( std::string str );

	/* https://developer.mozilla.org/en-US/docs/Web/CSS/animation-fill-mode */
	enum AnimationFillMode {
		None = String::hash( "none" ),
		Forwards = String::hash( "forwards" ),
		Backwards = String::hash( "backwards" ),
		Both = String::hash( "both" )
	};

	static bool isAnimationFillModeString( const std::string& str );

	static AnimationFillMode fillModeFromString( std::string str );

	AnimationDefinition();

	const AnimationDirection& getDirection() const;

	const bool& isPaused() const;

	const Int32& getIterations() const;

	const std::string& getName() const;

	const Time& getDelay() const;

	const Time& getDuration() const;

	const Ease::Interpolation& getTimingFunction() const;

	const std::vector<double>& getTimingFunctionParameters() const;

	const AnimationFillMode& getFillMode() const;

	void setName( const std::string& value );

	void setDelay( const Time& value );

	void setDuration( const Time& value );

	void setIterations( const Int32& value );

	void setTimingFunction( const Ease::Interpolation& value );

	void setTimingFunctionParameters( const std::vector<double>& timingFunctionParameters );

	void setDirection( const AnimationDirection& value );

	void setFillMode( const AnimationFillMode& value );

	void setPaused( bool value );

	const String::HashType& getId() const;

  protected:
	String::HashType mId;
	std::string mName;
	Time mDelay = Time::Zero;
	Time mDuration = Time::Zero;
	Int32 mIterations = 1; /* -1 == "infinite" */
	Ease::Interpolation mTimingFunction = Ease::Interpolation::Linear;
	std::vector<double> mTimingFunctionParameters{};
	AnimationDirection mDirection = Normal;
	AnimationFillMode mFillMode = None;
	bool mPaused = false;
};

inline bool operator==( const AnimationDefinition& a, const AnimationDefinition& b ) {
	return a.getDuration() == b.getDuration() && a.getTimingFunction() == b.getTimingFunction() &&
		   a.getDelay() == b.getDelay() && a.getDirection() == b.getDirection() &&
		   a.isPaused() == b.isPaused() && a.getIterations() == b.getIterations() &&
		   a.getName() == b.getName() &&
		   a.getTimingFunctionParameters() == b.getTimingFunctionParameters();
}

inline bool operator!=( const AnimationDefinition& a, const AnimationDefinition& b ) {
	return !( a == b );
}

typedef UnorderedMap<std::string, AnimationDefinition> AnimationsMap;

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_ANIMATIONDEFINITION_HPP
