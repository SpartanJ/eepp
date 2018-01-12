#ifndef EE_UIActionManager_HPP
#define EE_UIActionManager_HPP

#include <list>
#include <eepp/config.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;

namespace EE { namespace UI {

class UIAction;

class UIActionManager {
	public:
		UIActionManager();

		~UIActionManager();
		
		void addAction( UIAction * action );
		
		UIAction * getActionByTag( const Uint32& tag );

		void removeActionByTag( const Uint32& tag );

		void removeAction( UIAction * action );

		void update( const Time& time );

		std::size_t count() const;

		bool isEmpty() const;
	protected:
		std::list<UIAction*> mActions;
};

}}

#endif

