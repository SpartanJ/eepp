#ifndef EE_SCENE_ACTION_MARGINMOVE_HPP
#define EE_SCENE_ACTION_MARGINMOVE_HPP

#include <eepp/scene/action.hpp>

#include <eepp/math/interpolation1d.hpp>
#include <eepp/math/rect.hpp>
using namespace EE::Math;

namespace EE { namespace Scene { namespace Actions {

class EE_API MarginMove : public Action {
	public:
		static MarginMove * New( const Rect& start, const Rect& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		virtual Action * clone() const override;

		Action * reverse() const override;

		Interpolation1d getInterpolationLeft() const;

		void setInterpolationLeft(const Interpolation1d & getInterpolationLeft);

		Interpolation1d getInterpolationRight() const;

		void setInterpolationRight(const Interpolation1d & getInterpolationRight);

		Interpolation1d getInterpolationTop() const;

		void setInterpolationTop(const Interpolation1d & getInterpolationTop);

		Interpolation1d getInterpolationBottom() const;

		void setInterpolationBottom(const Interpolation1d & getInterpolationBottom);

	protected:
		MarginMove( const Rect & start, const Rect & end, const Time & duration, const Ease::Interpolation & type );

		void onStart() override;

		virtual void onUpdate( const Time& time ) override;

		MarginMove();

		Interpolation1d mInterpolationLeft;
		Interpolation1d mInterpolationRight;
		Interpolation1d mInterpolationTop;
		Interpolation1d mInterpolationBottom;
};

}}}

#endif

