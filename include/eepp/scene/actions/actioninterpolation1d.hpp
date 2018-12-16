#ifndef EE_SCENE_ACTIONINTERPOLATION1D_HPP
#define EE_SCENE_ACTIONINTERPOLATION1D_HPP

#include <eepp/scene/action.hpp>
#include <eepp/math/interpolation1d.hpp>
using namespace EE::Math;

namespace EE { namespace Scene { namespace Actions {

class EE_API ActionInterpolation1d : public Action {
	public:
		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		Interpolation1d * getInterpolation();
	protected:
		mutable Interpolation1d mInterpolation;

		ActionInterpolation1d();

		void setInterpolation( Interpolation1d interpolation );
};

}}}

#endif

