#ifndef EE_SCENE_ACTION_DELAY_HPP
#define EE_SCENE_ACTION_DELAY_HPP

#include <eepp/scene/action.hpp>
#include <eepp/system/clock.hpp>
using namespace EE::System;

namespace EE { namespace Scene { namespace Actions {

class EE_API Delay : public Action {
	public:
		static Delay * New( const Time& time );

		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		Float getCurrentProgress() override;

		Action * clone() const override;

		Action * reverse() const override;

	protected:
		Clock mClock;
		Time mTime;

		Delay( const Time& time );

};

}}}

#endif
