#ifndef EE_SCENE_ACTION_CLOSE_HPP
#define EE_SCENE_ACTION_CLOSE_HPP

#include <eepp/scene/actions/delay.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Close : public Delay {
	public:
		static Close * New( const Time& time );
		
		void update( const Time& time ) override;

		Action * clone() const override;

		Action * reverse() const override;

	protected:
		Close( const Time& time );
		
};

}}} 

#endif
