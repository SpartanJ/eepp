#ifndef EE_UI_CSS_TRANSITIONDEFINITION_HPP
#define EE_UI_CSS_TRANSITIONDEFINITION_HPP

#include <eepp/math/ease.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <map>
#include <vector>

using namespace EE::Math;
using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

class EE_API TransitionDefinition {
  public:
	static UnorderedMap<std::string, TransitionDefinition>
	parseTransitionProperties( const std::vector<const StyleSheetProperty*>& styleSheetProperties );

	TransitionDefinition() : timingFunction( Ease::Linear ) {}

	const std::string& getProperty() const { return property; }

	Ease::Interpolation getTimingFunction() const { return timingFunction; }

	std::vector<double> getTimingFunctionParameters() const { return timingFunctionParameters; }

	const Time& getDelay() const { return delay; }

	const Time& getDuration() const { return duration; }

	std::string property;
	Ease::Interpolation timingFunction = Ease::Interpolation::Linear;
	std::vector<double> timingFunctionParameters{};
	Time delay = Time::Zero;
	Time duration = Time::Zero;
};

typedef UnorderedMap<std::string, TransitionDefinition> TransitionsMap;

}}} // namespace EE::UI::CSS

#endif
