#include "utest.h"
#include <eepp/scene/node.hpp>

using namespace EE;
using namespace EE::Scene;

namespace {

class EventEmitter : public Node {
  public:
	void emit( Uint32 eventType ) { sendCommonEvent( eventType ); }
};

constexpr Uint32 TestEvent = Event::UserEvent;

} // namespace

UTEST( EventConnection, disconnectsWhenDestroyed ) {
	EventEmitter emitter;
	int callbackCount = 0;
	{
		auto connection = emitter.connect( TestEvent, [&]( const Event* ) { ++callbackCount; } );
		EXPECT_TRUE( static_cast<bool>( connection ) );
		emitter.emit( TestEvent );
	}

	emitter.emit( TestEvent );
	EXPECT_EQ( callbackCount, 1 );
}

UTEST( EventConnection, expiresWhenEmitterIsDestroyed ) {
	auto emitter = eeNew( EventEmitter, () );
	auto connection = emitter->connect( TestEvent, []( const Event* ) {} );
	EXPECT_TRUE( static_cast<bool>( connection ) );

	eeDelete( emitter );
	EXPECT_FALSE( static_cast<bool>( connection ) );
	connection.disconnect();
}

UTEST( EventConnection, reflectsListenerRemovalByEmitter ) {
	EventEmitter emitter;
	auto connection = emitter.connect( TestEvent, []( const Event* ) {} );
	EXPECT_TRUE( static_cast<bool>( connection ) );

	emitter.removeEventsOfType( TestEvent );
	EXPECT_FALSE( static_cast<bool>( connection ) );
}

UTEST( EventConnection, moveAssignmentDisconnectsPreviousListener ) {
	EventEmitter firstEmitter;
	EventEmitter secondEmitter;
	int firstCallbackCount = 0;
	int secondCallbackCount = 0;
	EventConnection connection =
		firstEmitter.connect( TestEvent, [&]( const Event* ) { ++firstCallbackCount; } );

	connection = secondEmitter.connect( TestEvent, [&]( const Event* ) { ++secondCallbackCount; } );
	firstEmitter.emit( TestEvent );
	secondEmitter.emit( TestEvent );

	EXPECT_EQ( firstCallbackCount, 0 );
	EXPECT_EQ( secondCallbackCount, 1 );
}

UTEST( EventConnectionList, disconnectsAllListeners ) {
	EventEmitter emitter;
	int callbackCount = 0;
	EventConnectionList connections;
	connections += emitter.connect( TestEvent, [&]( const Event* ) { ++callbackCount; } );
	connections += emitter.connect( TestEvent, [&]( const Event* ) { ++callbackCount; } );
	EXPECT_EQ( connections.size(), 2u );

	emitter.emit( TestEvent );
	connections.clear();
	emitter.emit( TestEvent );

	EXPECT_EQ( callbackCount, 2 );
	EXPECT_TRUE( connections.empty() );
}

UTEST( EventConnection, callbackCanDisconnectItselfDuringDispatch ) {
	EventEmitter emitter;
	int callbackCount = 0;
	EventConnection connection;
	connection = emitter.connect( TestEvent, [&]( const Event* ) {
		++callbackCount;
		connection.disconnect();
	} );

	emitter.emit( TestEvent );
	emitter.emit( TestEvent );

	EXPECT_EQ( callbackCount, 1 );
	EXPECT_FALSE( static_cast<bool>( connection ) );
}

UTEST( EventConnection, callbackCanDisconnectAnotherDuringDispatch ) {
	EventEmitter emitter;
	int disconnectedCallbackCount = 0;
	EventConnection disconnectedConnection;
	auto disconnectingConnection =
		emitter.connect( TestEvent, [&]( const Event* ) { disconnectedConnection.disconnect(); } );
	disconnectedConnection =
		emitter.connect( TestEvent, [&]( const Event* ) { ++disconnectedCallbackCount; } );

	emitter.emit( TestEvent );
	emitter.emit( TestEvent );

	// Dispatch uses a snapshot, so removal takes effect on the following dispatch.
	EXPECT_EQ( disconnectedCallbackCount, 1 );
}

UTEST( EventConnection, coexistsWithLegacyNumericListener ) {
	EventEmitter emitter;
	int scopedCallbackCount = 0;
	int legacyCallbackCount = 0;
	auto legacyId = emitter.on( TestEvent, [&]( const Event* ) { ++legacyCallbackCount; } );
	auto connection = emitter.connect( TestEvent, [&]( const Event* ) { ++scopedCallbackCount; } );

	connection.disconnect();
	emitter.emit( TestEvent );
	EXPECT_EQ( scopedCallbackCount, 0 );
	EXPECT_EQ( legacyCallbackCount, 1 );

	emitter.removeEventListener( legacyId );
	emitter.emit( TestEvent );
	EXPECT_EQ( legacyCallbackCount, 1 );
}

UTEST( EventConnection, clearEventListenerInvalidatesConnection ) {
	EventEmitter emitter;
	auto connection = emitter.connect( TestEvent, []( const Event* ) {} );

	emitter.clearEventListener();

	EXPECT_FALSE( static_cast<bool>( connection ) );
}

UTEST( EventConnection, emitterCanBeDestroyedDuringCallback ) {
	auto emitter = eeNew( EventEmitter, () );
	int callbackCount = 0;
	auto connection = emitter->connect( TestEvent, [&]( const Event* ) {
		++callbackCount;
		eeDelete( emitter );
		emitter = nullptr;
	} );

	emitter->emit( TestEvent );

	EXPECT_EQ( callbackCount, 1 );
	EXPECT_TRUE( emitter == nullptr );
	EXPECT_FALSE( static_cast<bool>( connection ) );
}

UTEST( EventConnection, dispatchesListenersInRegistrationOrder ) {
	EventEmitter emitter;
	std::vector<int> order;
	auto third = emitter.connect( TestEvent, [&]( const Event* ) { order.emplace_back( 3 ); } );
	auto first = emitter.connect( TestEvent, [&]( const Event* ) { order.emplace_back( 1 ); } );
	auto second = emitter.connect( TestEvent, [&]( const Event* ) { order.emplace_back( 2 ); } );

	emitter.emit( TestEvent );

	EXPECT_EQ( order.size(), 3u );
	EXPECT_EQ( order[0], 3 );
	EXPECT_EQ( order[1], 1 );
	EXPECT_EQ( order[2], 2 );
}
