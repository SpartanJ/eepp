#ifndef EE_SCENE_ACTIONS_RESIZEHEIGHT_HPP
#define EE_SCENE_ACTIONS_RESIZEHEIGHT_HPP

#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/actioninterpolation1d.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API ResizeHeight : public ActionInterpolation1d {
  public:
	static ResizeHeight* New( const Float& start, const Float& end, const Time& duration,
							  const Ease::Interpolation& type = Ease::Linear );

	Action* clone() const override;

	Action* reverse() const override;

  protected:
	ResizeHeight();

	ResizeHeight( const Float& start, const Float& end, const Time& duration,
				  const Ease::Interpolation& type );

	void onStart() override;

	void onUpdate( const Time& time ) override;
};

}}} // namespace EE::Scene::Actions

#endif
