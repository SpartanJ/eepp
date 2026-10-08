#include "../../tools/ecode/filesystemlistenercallbackstate.hpp"
#include "../../tools/ecode/filesystemlisteneroptions.hpp"
#include "utest.h"
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

using namespace ecode;

UTEST( FileSystemListenerOptions, filtersByEventTypeAndPathPrefix ) {
	FileSystemListenerOptions options;
	FileSystemListenerFilter filter;
	filter.eventTypes = fileEventTypeMask( FileSystemEventType::Add ) |
						fileEventTypeMask( FileSystemEventType::Modified );
	filter.path = "/tmp/ecode-ipc/";
	options.filters.emplace_back( std::move( filter ) );

	EXPECT_TRUE( options.matches( FileSystemEventType::Add, "/tmp/ecode-ipc/request" ) );
	EXPECT_TRUE( options.matches( FileSystemEventType::Modified, "/tmp/ecode-ipc/request" ) );
	EXPECT_FALSE( options.matches( FileSystemEventType::Delete, "/tmp/ecode-ipc/request" ) );
	EXPECT_FALSE( options.matches( FileSystemEventType::Add, "/tmp/other/request" ) );
}

UTEST( FileSystemListenerOptions, matchesAnyFilterWithoutDuplicateSemantics ) {
	FileSystemListenerOptions options;
	FileSystemListenerFilter config;
	config.eventTypes = fileEventTypeMask( FileSystemEventType::Modified );
	config.path = "/tmp/plugin.json";
	config.pathMatch = FileEventPathMatch::Exact;
	options.filters.emplace_back( std::move( config ) );
	FileSystemListenerFilter workspace;
	workspace.path = "/tmp/workspace/";
	options.filters.emplace_back( std::move( workspace ) );

	EXPECT_TRUE( options.matches( FileSystemEventType::Modified, "/tmp/plugin.json" ) );
	EXPECT_FALSE( options.matches( FileSystemEventType::Add, "/tmp/plugin.json" ) );
	EXPECT_TRUE( options.matches( FileSystemEventType::Add, "/tmp/workspace/file.cpp" ) );
	EXPECT_FALSE( options.matches( FileSystemEventType::Modified, "/tmp/plugin.json.backup" ) );
	EXPECT_TRUE(
		options.matchesJoinedPath( FileSystemEventType::Moved, "/tmp/workspace/", 0, "file.cpp" ) );
	EXPECT_TRUE( options.matchesJoinedPath( FileSystemEventType::Moved, "/tmp/workspace", '/',
											"file.cpp" ) );
}

UTEST( FileSystemListenerOptions, defaultsToAllEventsAndPathsOnMainThread ) {
	FileSystemListenerOptions options;
	EXPECT_EQ( options.affinity, FileEventThreadAffinity::Main );
	EXPECT_TRUE( options.matches( FileSystemEventType::Add, "/any/path" ) );
	EXPECT_TRUE( options.matches( FileSystemEventType::Delete, "/another/path" ) );
	EXPECT_TRUE( options.matches( FileSystemEventType::Modified, "relative/path" ) );
	EXPECT_TRUE( options.matches( FileSystemEventType::Moved, "" ) );
}

UTEST( FileSystemListener, removalWaitsForActiveCallback ) {
	FileSystemListenerCallbackState callbackState;
	std::mutex mutex;
	std::condition_variable condition;
	bool callbackBegan{ false };
	bool callbackStarted{ false };
	bool releaseCallback{ false };
	std::thread callbackThread( [&] {
		callbackBegan = callbackState.beginCallback();
		std::unique_lock<std::mutex> lock( mutex );
		callbackStarted = true;
		condition.notify_all();
		condition.wait( lock, [&] { return releaseCallback; } );
		lock.unlock();
		callbackState.endCallback();
	} );
	{
		std::unique_lock<std::mutex> lock( mutex );
		condition.wait( lock, [&] { return callbackStarted; } );
	}

	auto removal = std::async( std::launch::async, [&] { callbackState.removeAndWait( false ); } );
	EXPECT_EQ( removal.wait_for( std::chrono::milliseconds( 20 ) ), std::future_status::timeout );
	{
		std::lock_guard<std::mutex> lock( mutex );
		releaseCallback = true;
	}
	condition.notify_all();
	callbackThread.join();
	removal.get();
	EXPECT_TRUE( callbackBegan );
	EXPECT_FALSE( callbackState.beginCallback() );
}

UTEST( FileSystemListener, callbackCanRemoveItself ) {
	FileSystemListenerCallbackState callbackState;
	ASSERT_TRUE( callbackState.beginCallback() );
	callbackState.removeAndWait( true );
	callbackState.endCallback();
	EXPECT_FALSE( callbackState.beginCallback() );
}
