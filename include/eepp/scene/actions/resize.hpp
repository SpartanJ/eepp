#ifndef EE_SCENE_ACTIONS_RESIZE_HPP
#define EE_SCENE_ACTIONS_RESIZE_HPP

#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/actioninterpolation2d.hpp>
#include <eepp/math/size.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Resize : public ActionInterpolation2d {
	public:
		static Resize * New( const Sizef& start, const Sizef& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		Action * clone() const override;

		Action * reverse() const override;
	protected:
		Resize( const Sizef& start, const Sizef& end, const Time& duration, const Ease::Interpolation& type );

		void onStart() override;

		void onUpdate( const Time& time ) override;
	private:
		Resize();
};

}}}

#endif
