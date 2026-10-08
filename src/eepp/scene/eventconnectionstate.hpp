#ifndef EE_SCENE_EVENTCONNECTIONSTATE_HPP
#define EE_SCENE_EVENTCONNECTIONSTATE_HPP

#include <algorithm>
#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/scene/event.hpp>
#include <functional>

namespace EE { namespace Scene {

// Nodes own this registry strongly while EventConnection observes it weakly. Keeping callbacks in
// the shared state, rather than keeping a Node pointer in each connection, makes emitter-first
// destruction safe without relying on UIWidget::OnClose or extending the emitter's lifetime.
struct EventConnectionState {
	using EventCallback = std::function<void( const Event* )>;

	struct EventListener {
		Uint32 id;
		EventCallback callback;
	};

	// Listener IDs increase monotonically, so appending preserves registration order and keeps the
	// collection sorted for lookup while avoiding one allocation per listener.
	using EventListeners = SmallVector<EventListener, 4>;
	using EventsMap = UnorderedMap<Uint32, EventListeners>;

	Uint32 nextId{ 0 };
	EventsMap events;

	Uint32 add( Uint32 eventType, EventCallback callback ) {
		auto id = ++nextId;
		events[eventType].emplace_back( EventListener{ id, std::move( callback ) } );
		return id;
	}

	bool contains( Uint32 eventType, Uint32 callbackId ) const {
		auto event = events.find( eventType );
		if ( event == events.end() )
			return false;
		auto listener = lowerBound( event->second, callbackId );
		return listener != event->second.end() && listener->id == callbackId;
	}

	void remove( Uint32 eventType, Uint32 callbackId ) {
		auto event = events.find( eventType );
		if ( event != events.end() ) {
			auto listener = lowerBound( event->second, callbackId );
			if ( listener != event->second.end() && listener->id == callbackId )
				event->second.erase( listener );
		}
	}

	void remove( Uint32 callbackId ) {
		for ( auto& event : events ) {
			auto listener = lowerBound( event.second, callbackId );
			if ( listener != event.second.end() && listener->id == callbackId ) {
				event.second.erase( listener );
				break;
			}
		}
	}

  private:
	static EventListeners::iterator lowerBound( EventListeners& listeners, Uint32 callbackId ) {
		return std::lower_bound(
			listeners.begin(), listeners.end(), callbackId,
			[]( const EventListener& listener, Uint32 id ) { return listener.id < id; } );
	}

	static EventListeners::const_iterator lowerBound( const EventListeners& listeners,
													  Uint32 callbackId ) {
		return std::lower_bound(
			listeners.begin(), listeners.end(), callbackId,
			[]( const EventListener& listener, Uint32 id ) { return listener.id < id; } );
	}
};

}} // namespace EE::Scene

#endif
