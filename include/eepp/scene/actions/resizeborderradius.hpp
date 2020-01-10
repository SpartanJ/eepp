#ifndef EE_SCENE_RESIZEBORDERRADIUS_HPP
#define EE_SCENE_RESIZEBORDERRADIUS_HPP

#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/actioninterpolation1d.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API ResizeBorderRadius : public ActionInterpolation1d {
  public:
	static ResizeBorderRadius* New( const Float& start, const Float& end, const Time& duration,
									const Ease::Interpolation& type = Ease::Linear );

	Action* clone() const override;

	Action* reverse() const override;

  protected:
	ResizeBorderRadius();

	ResizeBorderRadius( const Float& start, const Float& end, const Time& duration,
						const Ease::Interpolation& type );

	void onStart() override;

	void onUpdate( const Time& time ) override;
};

}}} // namespace EE::Scene::Actions

#endif
