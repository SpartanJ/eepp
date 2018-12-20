#ifndef EE_SCENE_ACTION_ENABLE_HPP
#define EE_SCENE_ACTION_ENABLE_HPP

#include <eepp/scene/actions/delay.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Enable : public Delay {
	public:
		static Enable * New( const Time& time = Seconds(0) );

		static Enable * New( bool enable, const Time& time = Seconds(0) );

		void update( const Time& time ) override;

		Action * clone() const override;

		Action * reverse() const override;

	protected:
		bool mEnable;

		explicit Enable( bool enable, const Time& time );

		void onStart() override;

};

}}}

#endif
