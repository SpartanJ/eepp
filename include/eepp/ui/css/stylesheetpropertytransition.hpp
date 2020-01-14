#ifndef EE_UI_CSS_STYLESHEETLENGTHTRANSITION_HPP
#define EE_UI_CSS_STYLESHEETLENGTHTRANSITION_HPP

#include <eepp/math/ease.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <functional>

using namespace EE::Math;
using namespace EE::Scene;

namespace EE { namespace UI {
class UIWidget;
}} // namespace EE::UI

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetPropertyTransition : public Action {
  public:
	static bool transitionSupported( const PropertyType& type );

	static StyleSheetPropertyTransition* New( const PropertyDefinition* property,
											  const std::string& startValue,
											  const std::string& endValue, const Time& duration,
											  const Ease::Interpolation& type = Ease::Linear );

	void start() override;

	void stop() override;

	void update( const Time& time ) override;

	bool isDone() override;

	Float getCurrentProgress() override;

	Action* clone() const override;

	Action* reverse() const override;

	const std::string& getPropertyName() const;

	const std::string& getStartValue() const;

	const std::string& getEndValue() const;

	const Time& getDuration() const;

	const Ease::Interpolation& getType() const;

	const Time& getElapsed() const;

  protected:
	StyleSheetPropertyTransition( const PropertyDefinition* property, const std::string& startValue,
								  const std::string& endValue, const Time& duration,
								  const Ease::Interpolation& type = Ease::Linear );

	void onStart() override;

	void onUpdate( const Time& time ) override;

	const PropertyDefinition* mProperty;
	std::string mStartValue;
	std::string mEndValue;
	Time mDuration;
	Time mElapsed;
	Ease::Interpolation mType;

	Float getContainerLength( UIWidget* node );
};

}}} // namespace EE::UI::CSS

#endif
