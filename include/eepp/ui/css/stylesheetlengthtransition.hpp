#ifndef EE_UI_CSS_STYLESHEETLENGTHTRANSITION_HPP
#define EE_UI_CSS_STYLESHEETLENGTHTRANSITION_HPP

#include <eepp/scene/action.hpp>
#include <eepp/math/ease.hpp>
#include <functional>

using namespace EE::Math;
using namespace EE::Scene;

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetLengthTransition : public Action {
	public:
		typedef std::function<Float()> ContainerLengthProvider;

		static StyleSheetLengthTransition * New( const std::string& propertyName, const std::string& startValue, const std::string& endValue, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		Float getCurrentProgress();

		Action * clone() const override;

		Action * reverse() const override;

		const std::string& getPropertyName() const;

		const std::string& getStartValue() const;

		const std::string& getEndValue() const;

		const Time& getDuration() const;

		const Ease::Interpolation& getType() const;

		const Time& getElapsed() const;

		const Float& getContainerLength() const;

		StyleSheetLengthTransition& setContainerLength(const Float& containerLength);

		StyleSheetLengthTransition& setContainerLengthFunction(const ContainerLengthProvider& containerLengthProvider);
	protected:
		StyleSheetLengthTransition( const std::string& propertyName, const std::string& startValue, const std::string& endValue, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		void onStart() override;

		void onUpdate( const Time& time ) override;

		std::string mPropertyName;
		std::string mStartValue;
		std::string mEndValue;
		Time mDuration;
		Time mElapsed;
		Ease::Interpolation mType;
		Float mContainerLength;
		ContainerLengthProvider mContainerLengthFunction;
};

}}}

#endif
