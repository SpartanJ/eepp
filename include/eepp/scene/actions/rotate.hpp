#ifndef EE_SCENE_ACTION_ROTATE_HPP
#define EE_SCENE_ACTION_ROTATE_HPP

#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/actioninterpolation1d.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Rotate : public ActionInterpolation1d {
  public:
	static Rotate* New( const Float& start, const Float& end, const Time& duration,
						const Ease::Interpolation& type = Ease::Linear );

	Action* clone() const override;

	Action* reverse() const override;

  protected:
	Rotate( const Float& start, const Float& end, const Time& duration,
			const Ease::Interpolation& type );

	void onStart() override;

	void onUpdate( const Time& time ) override;

  private:
	Rotate();
};

}}} // namespace EE::Scene::Actions

#endif
