#ifndef EE_SCENE_ACTION_SPAWN_HPP
#define EE_SCENE_ACTION_SPAWN_HPP

#include <eepp/scene/action.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Spawn : public Action {
	public:
		static Spawn * New( const std::vector<Action*> spawn );
		static Spawn * New( Action * action, Action * action2 );
		static Spawn * New( Action * action, Action * action2, Action * action3 );
		static Spawn * New( Action * action, Action * action2, Action * action3, Action * action4 );
		static Spawn * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5 );
		static Spawn * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6 );
		static Spawn * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7 );
		static Spawn * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7, Action * action8 );
		static Spawn * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7, Action * action8, Action * action9 );

		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		Action * clone() const override;

		Action * reverse() const override;

		virtual ~Spawn();
	protected:
		std::vector<Action*> mSpawn;
		bool mAllDone;
		
		Spawn( const std::vector<Action*> spawn );
		
};

}}} 

#endif
