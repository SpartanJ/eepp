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

Action* ActionManager::getActionByTagFromTarget( Node* target, const Action::UniqueID& tag,
												 bool mustBePending ) {
	Lock l( mMutex );

	for ( Action* action : mActions ) {
		if ( action->getTarget() == target && action->getTag() == tag &&
			 ( !mustBePending || !action->isDone() ) )
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

void ActionManager::update( const Time& time, Action** actions, size_t count ) {
	std::vector<Action*> removeList;
	std::vector<Action*> deferredRemoveList;

	// Copy the deferred remove list under lock to avoid data races during the loop.
	{
		Lock l( mMutex );
		if ( !mActionsRemoveList.empty() )
			deferredRemoveList = mActionsRemoveList;
	}

	for ( size_t i = 0; i < count; i++ ) {
		Action* action = actions[i];

		if ( std::find( deferredRemoveList.begin(), deferredRemoveList.end(), action ) !=
			 deferredRemoveList.end() )
			continue;

		action->update( time );

		if ( action->isDone() ) {
			action->sendEvent( Action::ActionType::OnDone );

			removeList.emplace_back( action );
		}
	}

	mUpdating = false;

	// Atomically get the list of deferred removals and clear the shared list.
	{
		Lock l( mMutex );
		deferredRemoveList.clear(); // Reuse the vector
		deferredRemoveList.swap( mActionsRemoveList );
	}

	// Process actions that were queued for removal.
	for ( auto it = deferredRemoveList.begin(); it != deferredRemoveList.end(); ++it )
		removeAction( *it );

	// Process actions that finished during this update.
	for ( auto it = removeList.begin(); it != removeList.end(); ++it )
		removeAction( *it );
}

void ActionManager::update( const Time& time ) {
	if ( isEmpty() )
		return;

	mUpdating = true;
	size_t size;

	{
		Lock l( mMutex );
		size = mActions.size();
	}

	// Micro-optimization to avoid heap allocations during updates (which are done usually at 60 hz)
	if ( size <= 8 ) {
		Action* actions[8];
		{
			Lock l( mMutex );
			std::copy( mActions.begin(), mActions.end(), actions );
		}
		update( time, actions, size );
	} else if ( size <= 16 ) {
		Action* actions[16];
		{
			Lock l( mMutex );
			std::copy( mActions.begin(), mActions.end(), actions );
		}
		update( time, actions, size );
	} else {
		// Actions can be added during action updates, we need to only iterate the current actions
		std::vector<Action*> actions;
		{
			Lock l( mMutex );
			actions = mActions;
		}
		update( time, actions.data(), size );
	}
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
		Lock l( mMutex );

		if ( !mUpdating ) {
			auto actionIt = std::find( mActions.begin(), mActions.end(), action );

			if ( actionIt != mActions.end() ) {
				mActions.erase( actionIt );

				eeSAFE_DELETE( action );
			}
		} else if ( std::find( mActionsRemoveList.begin(), mActionsRemoveList.end(), action ) ==
					mActionsRemoveList.end() ) {
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
