#ifndef EE_SCENEACTIONMANAGER_HPP
#define EE_SCENEACTIONMANAGER_HPP

#include <list>
#include <eepp/config.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;

namespace EE { namespace Scene {

class Action;

class EE_API ActionManager {
	public:
		ActionManager();

		~ActionManager();
		
		void addAction( Action * action );
		
		Action * getActionByTag( const Uint32& tag );

		void removeActionByTag( const Uint32& tag );

		void removeAction( Action * action );

		void update( const Time& time );

		std::size_t count() const;

		bool isEmpty() const;

		void clear();
	protected:
		std::list<Action*> mActions;
};

}}

#endif

