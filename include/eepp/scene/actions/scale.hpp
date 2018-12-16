#ifndef EE_SCENE_ACTION_SCALE_HPP
#define EE_SCENE_ACTION_SCALE_HPP

#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/actioninterpolation2d.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Scale : public ActionInterpolation2d {
	public:
		static Scale * New( const Vector2f& start, const Vector2f& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		Action * clone() const override;

		Action * reverse() const override;
	protected:
		Scale( const Vector2f& start, const Vector2f& end, const Time& duration, const Ease::Interpolation& type );

		void onStart() override;

		void onUpdate( const Time& time ) override;
	private:
		Scale();
};

}}}

#endif

