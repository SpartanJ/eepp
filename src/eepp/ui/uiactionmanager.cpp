#include <eepp/ui/uiactionmanager.hpp>
#include <eepp/ui/uiaction.hpp>
#include <eepp/core.hpp>
#include <algorithm>

namespace EE { namespace UI {

UIActionManager::UIActionManager() {
}

UIActionManager::~UIActionManager() {
	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		UIAction * action = (*it);

		eeSAFE_DELETE( action );
	}
}

void UIActionManager::addAction( UIAction * action ) {
	bool found = (std::find(mActions.begin(), mActions.end(), action) != mActions.end());

	if ( !found ) {
		mActions.push_back( action );
	}
}

UIAction * UIActionManager::getActionByTag( const Uint32& tag ) {
	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		UIAction * action = (*it);

		if ( action->getTag() == tag )
			return action;
	}

	return NULL;
}

void UIActionManager::removeActionByTag( const Uint32& tag ) {
	removeAction( getActionByTag( tag ) );
}

void UIActionManager::update( const Time& time ) {
	if ( isEmpty() )
		return;

	std::list<UIAction*> removeList;

	for ( auto it = mActions.begin(); it != mActions.end(); ++it ) {
		UIAction * action = (*it);

		action->update( time );

		if ( action->isDone() ) {
			action->sendEvent( UIAction::ActionType::OnDone );

			removeList.push_back( action );
		}
	}

	for ( auto it = removeList.begin(); it != removeList.end(); ++it )
		removeAction( (*it) );
}

std::size_t UIActionManager::count() const {
	return mActions.size();
}

bool UIActionManager::isEmpty() const {
	return mActions.empty();
}

void UIActionManager::removeAction( UIAction * action ) {
	if ( NULL != action ) {
		mActions.remove( action );

		eeSAFE_DELETE( action );
	}
}

}}
