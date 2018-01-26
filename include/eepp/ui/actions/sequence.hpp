#ifndef EE_UI_ACTION_SEQUENCE_HPP
#define EE_UI_ACTION_SEQUENCE_HPP

#include <eepp/ui/uiaction.hpp>

namespace EE { namespace UI { namespace Action {

class EE_API Sequence : public UIAction {
	public:
		static Sequence * New( const std::vector<UIAction*> sequence );
		static Sequence * New( UIAction * action, UIAction * action2 );
		static Sequence * New( UIAction * action, UIAction * action2, UIAction * action3 );
		static Sequence * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4 );
		static Sequence * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5 );
		static Sequence * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6 );
		static Sequence * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7 );
		static Sequence * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7, UIAction * action8 );
		static Sequence * New( UIAction * action, UIAction * action2, UIAction * action3, UIAction * action4, UIAction * action5, UIAction * action6, UIAction * action7, UIAction * action8, UIAction * action9 );

		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		UIAction * clone() const override;

		UIAction * reverse() const override;

		virtual ~Sequence();

	protected:
		std::vector<UIAction*> mSequence;
		Uint32 mCurPos;
		
		Sequence( const std::vector<UIAction*> sequence );
		
};

}}} 

#endif
