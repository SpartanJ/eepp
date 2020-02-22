#ifndef EE_SCENE_ACTIONINTERPOLATION2D_HPP
#define EE_SCENE_ACTIONINTERPOLATION2D_HPP

#include <eepp/math/interpolation2d.hpp>
#include <eepp/scene/action.hpp>
using namespace EE::Math;

namespace EE { namespace Scene { namespace Actions {

class EE_API ActionInterpolation2d : public Action {
  public:
	void start() override;

	void stop() override;

	void update( const Time& time ) override;

	bool isDone() override;

	Float getCurrentProgress() override;

	Time getTotalTime() override;

	Interpolation2d* getInterpolation();

  protected:
	mutable Interpolation2d mInterpolation;

	ActionInterpolation2d();

	void setInterpolation( Interpolation2d interpolation );
};

}}} // namespace EE::Scene::Actions

#endif
