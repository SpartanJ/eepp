#include <algorithm>
#include <eepp/core.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/scene/actionmanager.hpp>
#include <eepp/system/lock.hpp>

namespace EE { namespace Scene {

ActionManager* ActionManager::New() {
	return eeNew( ActionManager, () );
}

ActionManager::ActionManager() : mUpdating( false ) {}

ActionManager::~ActionManager() {
	clear();
}

void ActionManager::addAction( Action* action ) {
	Lock l( mMutex );

	bool found = ( std::find( mActions.begin(), mActions.end(), action ) != mActions.end() );

	if ( !found )
		mActions.emplace_back( action );
}

Action* ActionManager::getActionByTag( const Action::UniqueID& tag ) {
	Lock l( mMutex );

	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		Action* action = *it;

		if ( action->getTag() == tag )
			return action;
	}

	return NULL;
}

std::vector<Action*> ActionManager::getActionsFromTarget( Node* target ) {
	Lock l( mMutex );

	std::vector<Action*> actions;

	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		Action* action = *it;

		if ( action->getTarget() == target )
			actions.emplace_back( action );
	}

	return actions;
}

std::vector<Action*> ActionManager::getActionsByTagFromTarget( Node* target,
															   const Action::UniqueID& tag ) {
	Lock l( mMutex );
	std::vector<Action*> actions;

	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		Action* action = *it;

		if ( action->getTarget() == target && action->getTag() == tag )
			actions.emplace_back( action );
	}

	return actions;
}

bool ActionManager::removeActionByTag( const Action::UniqueID& tag ) {
	return removeAction( getActionByTag( tag ) );
}

bool ActionManager::removeActionsByTagFromTarget( Node* target, const Action::UniqueID& tag ) {
	std::vector<Action*> removeList;

	{
		Lock l( mMutex );
		for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
			Action* action = *it;

			if ( action->getTarget() == target && action->getTag() == tag ) {
				removeList.emplace_back( *it );
			}
		}
	}

	for ( auto it = removeList.begin(); it != removeList.end(); ++it )
		removeAction( *it );

	return !removeList.empty();
}

void ActionManager::update( const Time& time ) {
	if ( isEmpty() )
		return;

	std::vector<Action*> removeList;

	mUpdating = true;

	// Actions can be added during action updates, we need to only iterate the current actions
	std::vector<Action*> actions;
	{
		Lock l( mMutex );
		actions = mActions;
	}

	for ( auto it = actions.begin(); it != actions.end(); ++it ) {
		Action* action = *it;

		action->update( time );

		if ( action->isDone() ) {
			action->sendEvent( Action::ActionType::OnDone );

			removeList.emplace_back( action );
		}
	}

	mUpdating = false;

	for ( auto it = mActionsRemoveList.begin(); it != mActionsRemoveList.end(); ++it )
		removeAction( *it );

	mActionsRemoveList.clear();

	for ( auto it = removeList.begin(); it != removeList.end(); ++it )
		removeAction( *it );
}

std::size_t ActionManager::count() const {
	Lock l( mMutex );
	return mActions.size();
}

bool ActionManager::isEmpty() const {
	Lock l( mMutex );
	return mActions.empty();
}

void ActionManager::clear() {
	Lock l( mMutex );

	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		Action* action = *it;

		eeSAFE_DELETE( action );
	}

	mActions.clear();
}

bool ActionManager::removeAction( Action* action ) {
	if ( NULL != action ) {
		if ( !mUpdating ) {
			Lock l( mMutex );

			auto actionIt = std::find( mActions.begin(), mActions.end(), action );

			if ( actionIt != mActions.end() ) {
				mActions.erase( actionIt );

				eeSAFE_DELETE( action );
			}
		} else {
			mActionsRemoveList.emplace_back( action );
		}

		return true;
	}

	return false;
}

bool ActionManager::removeActions( const std::vector<Action*>& actions ) {
	size_t removed = 0;
	for ( auto& action : actions ) {
		if ( removeAction( action ) )
			removed++;
	}
	return removed == actions.size();
}

bool ActionManager::removeAllActionsFromTarget( Node* target ) {
	std::vector<Action*> removeList;

	{
		Lock l( mMutex );
		for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
			Action* action = *it;

			if ( action->getTarget() == target ) {
				removeList.emplace_back( *it );
			}
		}
	}

	for ( auto it = removeList.begin(); it != removeList.end(); ++it )
		removeAction( *it );

	return !removeList.empty();
}

}} // namespace EE::Scene
