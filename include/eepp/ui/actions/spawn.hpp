#ifndef EE_UI_ACTION_SPAWN_HPP
#define EE_UI_ACTION_SPAWN_HPP

#include <eepp/ui/uiaction.hpp>

namespace EE { namespace UI { namespace Action {

class EE_API Spawn : public UIAction {
	public:
		static Spawn * New( const std::vector<UIAction*> spawn );
		static Spawn * New( UIAction * action, UIAction * action2 );
		static Spawn * New( UIAction * action, UIAction * action2, UIAction * action3 );
		static Spawn * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4 );
		static Spawn * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5 );
		static Spawn * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6 );
		static Spawn * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7 );
		static Spawn * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7, UIAction * action8 );
		static Spawn * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7, UIAction * action8, UIAction * action9 );

		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		UIAction * clone() const override;

		UIAction * reverse() const override;

		virtual ~Spawn();
	protected:
		std::vector<UIAction*> mSpawn;
		bool mAllDone;
		
		Spawn( const std::vector<UIAction*> spawn );
		
};

}}} 

#endif
