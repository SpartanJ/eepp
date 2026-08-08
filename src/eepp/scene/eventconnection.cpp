#include <eepp/scene/eventconnection.hpp>
#include <eepp/scene/eventconnectionstate.hpp>

namespace EE { namespace Scene {

EventConnection::EventConnection( std::weak_ptr<EventConnectionState> state, Uint32 eventType,
								  Uint32 callbackId ) :
	mState( std::move( state ) ), mEventType( eventType ), mCallbackId( callbackId ) {}

EventConnection::~EventConnection() {
	disconnect();
}

EventConnection::EventConnection( EventConnection&& other ) noexcept :
	mState( std::move( other.mState ) ),
	mEventType( other.mEventType ),
	mCallbackId( other.mCallbackId ) {
	other.mEventType = 0;
	other.mCallbackId = 0;
}

EventConnection& EventConnection::operator=( EventConnection&& other ) noexcept {
	if ( this != &other ) {
		disconnect();
		mState = std::move( other.mState );
		mEventType = other.mEventType;
		mCallbackId = other.mCallbackId;
		other.mEventType = 0;
		other.mCallbackId = 0;
	}
	return *this;
}

void EventConnection::disconnect() {
	if ( mCallbackId != 0 ) {
		if ( auto state = mState.lock() )
			state->remove( mEventType, mCallbackId );
	}
	mState.reset();
	mEventType = 0;
	mCallbackId = 0;
}

EventConnection::operator bool() const {
	if ( mCallbackId != 0 ) {
		if ( auto state = mState.lock() )
			return state->contains( mEventType, mCallbackId );
	}
	return false;
}

EventConnectionList& EventConnectionList::add( EventConnection connection ) {
	mConnections.emplace_back( std::move( connection ) );
	return *this;
}

EventConnectionList& EventConnectionList::operator+=( EventConnection connection ) {
	add( std::move( connection ) );
	return *this;
}

void EventConnectionList::clear() {
	mConnections.clear();
}

bool EventConnectionList::empty() const {
	return mConnections.empty();
}

size_t EventConnectionList::size() const {
	return mConnections.size();
}

}} // namespace EE::Scene
