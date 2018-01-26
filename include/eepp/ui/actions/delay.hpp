#ifndef EE_UI_ACTION_DELAY_HPP
#define EE_UI_ACTION_DELAY_HPP

#include <eepp/ui/uiaction.hpp>
#include <eepp/system/clock.hpp>
using namespace EE::System;

namespace EE { namespace UI { namespace Action {

class EE_API Delay : public UIAction {
	public:
		static Delay * New( const Time& time );
		
		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		UIAction * clone() const override;

		UIAction * reverse() const override;

	protected:
		Clock mClock;
		Time mTime;
		
		Delay( const Time& time );
		
};

}}} 

#endif
