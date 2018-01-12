#ifndef EE_UI_ACTIONINTERPOLATION2D_HPP
#define EE_UI_ACTIONINTERPOLATION2D_HPP

#include <eepp/ui/uiaction.hpp>
#include <eepp/math/interpolation2d.hpp>
using namespace EE::Math;

namespace EE { namespace UI { namespace Action {

class ActionInterpolation2d : public UIAction {
	public:
		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		Interpolation2d * getInterpolation();
	protected:
		Interpolation2d mInterpolation;

		ActionInterpolation2d();

		void setInterpolation( Interpolation2d interpolation );
};

}}}

#endif

