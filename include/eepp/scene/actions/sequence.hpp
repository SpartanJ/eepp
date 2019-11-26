#ifndef EE_SCENE_ACTION_SEQUENCE_HPP
#define EE_SCENE_ACTION_SEQUENCE_HPP

#include <eepp/scene/action.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Sequence : public Action {
	public:
		static Sequence * New( const std::vector<Action*> sequence );
		static Sequence * New( Action * action, Action * action2 );
		static Sequence * New( Action * action, Action * action2, Action * action3 );
		static Sequence * New( Action * action, Action * action2, Action * action3, Action * action4 );
		static Sequence * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5 );
		static Sequence * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6 );
		static Sequence * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7 );
		static Sequence * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7, Action * action8 );
		static Sequence * New( Action * action, Action * action2, Action * action3, Action * action4, Action * action5, Action * action6, Action * action7, Action * action8, Action * action9 );

		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		Float getCurrentProgress();

		Action * clone() const override;

		Action * reverse() const override;

		virtual ~Sequence();

	protected:
		std::vector<Action*> mSequence;
		Uint32 mCurPos;

		Sequence( const std::vector<Action*> sequence );

};

}}}

#endif
