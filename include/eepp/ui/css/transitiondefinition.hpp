#ifndef EE_UI_CSS_TRANSITIONDEFINITION_HPP
#define EE_UI_CSS_TRANSITIONDEFINITION_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/math/ease.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/timingfunction.hpp>
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

/** Compact transition data used by computed element definitions. It owns no strings and keeps
 * the complete CSS timing-function payload inline. */
struct ComputedTransitionDefinition {
	String::HashType propertyNameHash{ 0 };
	Ease::Interpolation timingFunction{ Ease::Linear };
	TimingFunction::Parameters timingFunctionParameters;
	Time delay{ Time::Zero };
	Time duration{ Time::Zero };
};

class EE_API ComputedTransitions {
  public:
	static ComputedTransitions
	parse( const std::vector<const StyleSheetProperty*>& styleSheetProperties );

	const ComputedTransitionDefinition* get( String::HashType propertyNameHash ) const;

	bool empty() const { return mTransitions.empty(); }

	std::size_t size() const { return mTransitions.size(); }

  private:
	SmallVector<ComputedTransitionDefinition, 4> mTransitions;

	void set( const ComputedTransitionDefinition& transition );
};

}}} // namespace EE::UI::CSS

#endif
