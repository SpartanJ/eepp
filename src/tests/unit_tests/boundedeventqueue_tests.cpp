#include "../../tools/ecode/boundedeventqueue.hpp"
#include "utest.h"
#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

using namespace ecode;

UTEST( BoundedEventQueue, schedulesOnceAndDrainsInFifoOrder ) {
	BoundedEventQueue<int> queue;
	EXPECT_TRUE( queue.push( 1 ) );
	EXPECT_FALSE( queue.push( 2 ) );
	EXPECT_FALSE( queue.push( 3 ) );

	std::vector<int> firstDrain;
	queue.popUpTo( firstDrain, 2 );
	EXPECT_EQ( firstDrain.size(), 2u );
	EXPECT_EQ( firstDrain[0], 1 );
	EXPECT_EQ( firstDrain[1], 2 );
	EXPECT_TRUE( queue.finishDrain() );

	std::vector<int> secondDrain;
	queue.popUpTo( secondDrain, 2 );
	EXPECT_EQ( secondDrain.size(), 1u );
	EXPECT_EQ( secondDrain[0], 3 );
	EXPECT_FALSE( queue.finishDrain() );
	EXPECT_TRUE( queue.push( 4 ) );
}

UTEST( BoundedEventQueue, producerDuringDrainCannotLoseWakeup ) {
	BoundedEventQueue<int> queue;
	EXPECT_TRUE( queue.push( 1 ) );

	std::vector<int> drain;
	queue.popUpTo( drain, 1 );
	EXPECT_FALSE( queue.push( 2 ) );
	EXPECT_TRUE( queue.finishDrain() );

	drain.clear();
	queue.popUpTo( drain, 1 );
	EXPECT_EQ( drain.size(), 1u );
	EXPECT_EQ( drain[0], 2 );
	EXPECT_FALSE( queue.finishDrain() );
}

UTEST( BoundedEventQueue, replacesOnlyMatchingLastPendingEvent ) {
	BoundedEventQueue<int> queue;
	EXPECT_TRUE( queue.pushOrReplaceLast( 1, []( int, int ) { return false; } ) );
	EXPECT_FALSE( queue.pushOrReplaceLast( 2, []( int previous, int ) { return previous == 1; } ) );
	EXPECT_FALSE( queue.pushOrReplaceLast( 3, []( int, int ) { return false; } ) );

	std::vector<int> drain;
	queue.popUpTo( drain, 4 );
	EXPECT_EQ( drain.size(), 2u );
	EXPECT_EQ( drain[0], 2 );
	EXPECT_EQ( drain[1], 3 );
	EXPECT_FALSE( queue.finishDrain() );
}

UTEST( BoundedEventQueue, acceptsConcurrentProducersWithoutLoss ) {
	static constexpr int ProducerCount = 4;
	static constexpr int EventsPerProducer = 1000;
	BoundedEventQueue<int> queue;
	std::atomic<int> scheduleCount{ 0 };
	std::vector<std::thread> producers;

	for ( int producer = 0; producer < ProducerCount; ++producer ) {
		producers.emplace_back( [producer, &queue, &scheduleCount] {
			for ( int event = 0; event < EventsPerProducer; ++event ) {
				if ( queue.push( producer * EventsPerProducer + event ) )
					++scheduleCount;
			}
		} );
	}
	for ( auto& producer : producers )
		producer.join();

	EXPECT_EQ( scheduleCount.load(), 1 );
	std::vector<int> received;
	do {
		queue.popUpTo( received, received.size() + 37 );
	} while ( queue.finishDrain() );

	EXPECT_EQ( received.size(), static_cast<std::size_t>( ProducerCount * EventsPerProducer ) );
	int lastEvent[ProducerCount] = { -1, -1, -1, -1 };
	for ( int event : received ) {
		const int producer = event / EventsPerProducer;
		EXPECT_EQ( event, producer * EventsPerProducer + ++lastEvent[producer] );
	}
	std::sort( received.begin(), received.end() );
	for ( int event = 0; event < ProducerCount * EventsPerProducer; ++event )
		EXPECT_EQ( received[event], event );
}

UTEST( BoundedEventQueue, clearCancelsPendingDrainState ) {
	BoundedEventQueue<int> queue;
	EXPECT_TRUE( queue.push( 1 ) );
	EXPECT_FALSE( queue.push( 2 ) );
	queue.clear();
	EXPECT_TRUE( queue.push( 3 ) );

	std::vector<int> drain;
	queue.popUpTo( drain, 4 );
	EXPECT_EQ( drain.size(), 1u );
	EXPECT_EQ( drain[0], 3 );
	EXPECT_FALSE( queue.finishDrain() );
}
