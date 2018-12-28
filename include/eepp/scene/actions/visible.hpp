#ifndef EE_SCENE_ACTION_VISIBLE_HPP
#define EE_SCENE_ACTION_VISIBLE_HPP

#include <eepp/scene/actions/delay.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Visible : public Delay {
	public:
		static Visible * New( bool visible, const Time& time = Seconds(0) );

		void update( const Time& time ) override;

		Action * clone() const override;

		Action * reverse() const override;

	protected:
		bool mVisible;

		explicit Visible( bool visible, const Time& time );

		void onStart() override;

};

}}}

#endif
