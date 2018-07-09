#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/core.hpp>
#include <algorithm>

namespace EE { namespace Scene {

ActionManager::ActionManager() {
}

ActionManager::~ActionManager() {
	clear();
}

void ActionManager::addAction( Action * action ) {
	bool found = (std::find(mActions.begin(), mActions.end(), action) != mActions.end());

	if ( !found ) {
		mActions.push_back( action );
	}
}

Action * ActionManager::getActionByTag( const Uint32& tag ) {
	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		Action * action = (*it);

		if ( action->getTag() == tag )
			return action;
	}

	return NULL;
}

void ActionManager::removeActionByTag( const Uint32& tag ) {
	removeAction( getActionByTag( tag ) );
}

void ActionManager::update( const Time& time ) {
	if ( isEmpty() )
		return;

	std::list<Action*> removeList;

	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		Action * action = (*it);

		action->update( time );

		if ( action->isDone() ) {
			action->sendEvent( Action::ActionType::OnDone );

			removeList.push_back( action );
		}
	}

	for ( auto it = removeList.begin(); it != removeList.end(); ++it )
		removeAction( (*it) );
}

std::size_t ActionManager::count() const {
	return mActions.size();
}

bool ActionManager::isEmpty() const {
	return mActions.empty();
}

void ActionManager::clear() {
	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		Action * action = (*it);

		eeSAFE_DELETE( action );
	}

	mActions.clear();
}

void ActionManager::removeAction( Action * action ) {
	if ( NULL != action ) {
		mActions.remove( action );

		eeSAFE_DELETE( action );
	}
}

}}
