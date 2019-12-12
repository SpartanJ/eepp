#include <eepp/scene/action.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

Action::Action() :
	mNode( NULL ),
	mFlags( 0 ),
	mTag( 0 ),
	mNumCallBacks( 0 )
{
}

Action::~Action() {
	sendEvent( ActionType::OnDelete );
}

Uint32 Action::getFlags() const {
	return mFlags;
}

void Action::setFlags( const Uint32 & flags ) {
	mFlags = flags;
}

Uint32 Action::getTag() const {
	return mTag;
}

void Action::setTag( const Uint32 & tag ) {
	mTag = tag;
}

void Action::setTarget( Node * target ) {
	mNode = target;
}

Node * Action::getTarget() const {
	return mNode;
}

Action * Action::clone() const {
	return NULL;
}

Action * Action::reverse() const {
	return NULL;
}

Uint32 Action::addEventListener( const ActionType& actionType, const ActionCallback& callback ) {
	mNumCallBacks++;

	mCallbacks[ actionType ][ mNumCallBacks ] = callback;

	return mNumCallBacks;
}

Action * Action::on(const Action::ActionType& actionType, const Action::ActionCallback& callback) {
	addEventListener( actionType, callback );
	return this;
}

void Action::removeEventListener( const Uint32& callbackId ) {
	for ( auto it = mCallbacks.begin(); it != mCallbacks.end(); ++it ) {
		std::map<Uint32, ActionCallback>& event = it->second;

		if ( event.erase( callbackId ) )
			break;
	}
}

void Action::sendEvent( const ActionType& actionType ) {
	if ( 0 != mCallbacks.count( actionType ) ) {
		auto event = mCallbacks[ actionType ];

		if ( !event.empty() ) {
			for ( auto it = event.begin(); it != event.end(); ++it )
				it->second( this, actionType );
		}
	}
}

void Action::onStart() {}

void Action::onStop() {}

void Action::onUpdate( const Time& ) {}

}}
