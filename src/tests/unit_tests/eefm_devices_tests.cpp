#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX

#include "../../tools/eefm/devicemonitor.hpp"
#include "../../tools/eefm/places.hpp"
#include "utest.hpp"
#include <chrono>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <mutex>
#include <thread>

using namespace eefm;
using namespace EE;
using namespace EE::System;

namespace {

Place device( const char* path ) {
	Place place;
	place.uri = URI::fromFSPath( path );
	place.section = PlaceSection::Devices;
	return place;
}

bool waitForCall( const char* marker ) {
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
	const auto path = Sys::getEnv( "EEFM_TEST_UDISKS_LOG" );
	do {
		std::string calls;
		if ( FileSystem::fileGet( path, calls ) && calls.find( marker ) != std::string::npos )
			return true;
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	} while ( std::chrono::steady_clock::now() < deadline );
	return false;
}

} // namespace

UTEST( EEFMDevices, RejectsRootAndUnknownVolumes ) {
	std::vector<Place> devices{ device( "/" ), device( "/nonexistent-eefm-device-fixture/" ) };
	DeviceMonitor::updateCapabilities( devices );
	EXPECT_FALSE( devices[0].ejectable );
	EXPECT_FALSE( devices[1].ejectable );
	EXPECT_FALSE( DeviceMonitor::eject( "/nonexistent-eefm-device-fixture/" ).empty() );
}

UTEST( EEFMDevices, UDisksIntegration ) {
	// The private D-Bus fixture owns every device below; never run removal operations against
	// a developer's real UDisks service. See projects/scripts/tests/test_eefm_devices_linux.py.
	if ( Sys::getEnv( "EEFM_TEST_UDISKS" ) != "1" )
		UTEST_SKIP( "Run with the private UDisks integration fixture" );
	std::vector<Place> devices{ device( "/fixture/usb/a/" ),
								device( "/fixture/usb/b" ),
								device( "/fixture/disc" ),
								device( "/fixture/busy" ),
								device( "/" ),
								device( "/fixture/protected" ),
								device( "/fixture/encrypted" ),
								device( "/fixture/usb/a-child" ) };
	DeviceMonitor::updateCapabilities( devices );
	EXPECT_TRUE( devices[0].ejectable );
	EXPECT_TRUE( devices[1].ejectable );
	EXPECT_TRUE( devices[2].ejectable );
	EXPECT_TRUE( devices[3].ejectable );
	EXPECT_FALSE( devices[4].ejectable );
	EXPECT_FALSE( devices[5].ejectable );
	EXPECT_TRUE( devices[6].ejectable );
	EXPECT_FALSE( devices[7].ejectable );
	EXPECT_TRUE( DeviceMonitor::eject( "/fixture/usb/a/" ).empty() );
	EXPECT_TRUE( DeviceMonitor::eject( "/fixture/disc" ).empty() );
	const auto error = DeviceMonitor::eject( "/fixture/busy" );
	EXPECT_TRUE( error.find( "busy" ) != std::string::npos );
	EXPECT_FALSE( DeviceMonitor::eject( "/fixture/protected" ).empty() );
	EXPECT_FALSE( DeviceMonitor::eject( "/" ).empty() );
	EXPECT_TRUE( DeviceMonitor::eject( "/fixture/encrypted" ).empty() );
}

UTEST( EEFMDevices, UDisksCleanupStaysOffTheGlobalContext ) {
	if ( Sys::getEnv( "EEFM_TEST_UDISKS" ) != "1" )
		UTEST_SKIP( "Run with the private UDisks integration fixture" );
	void* library = Sys::loadObject( "libudisks2.so.0", false );
	ASSERT_TRUE( library != nullptr );
	auto pending = reinterpret_cast<int ( * )( void* )>(
		Sys::loadFunction( library, "g_main_context_pending" ) );
	auto threadContext = reinterpret_cast<void* (*)()>(
		Sys::loadFunction( library, "g_main_context_get_thread_default" ) );
	ASSERT_TRUE( pending && threadContext );
	void* originalContext = threadContext();
	EXPECT_FALSE( pending( nullptr ) );
	std::vector<Place> devices{ device( "/fixture/busy" ) };
	for ( size_t i = 0; i < 300; ++i )
		DeviceMonitor::updateCapabilities( devices );
	EXPECT_TRUE( devices[0].ejectable );
	EXPECT_TRUE( threadContext() == originalContext );
	EXPECT_FALSE( pending( nullptr ) );
	Sys::unloadObject( library );
}

UTEST( EEFMDevices, UDisksRemovesChildrenBeforeParents ) {
	if ( Sys::getEnv( "EEFM_TEST_UDISKS" ) != "1" )
		UTEST_SKIP( "Run with the private UDisks integration fixture" );
	EXPECT_TRUE( DeviceMonitor::eject( "/fixture/nested" ).empty() );
	EXPECT_TRUE( DeviceMonitor::eject( "/fixture/layered" ).empty() );
}

UTEST( EEFMDevices, UDisksCancellationReleasesTheWorkerPool ) {
	if ( Sys::getEnv( "EEFM_TEST_UDISKS" ) != "1" )
		UTEST_SKIP( "Run with the private UDisks integration fixture" );
	struct Operation {
		const char* path;
		const char* marker;
	};
	for ( const auto& operation :
		  { Operation{ "/fixture/stall-unmount", "block_devices/stall_unmount" },
			Operation{ "/fixture/stall-lock", "block_devices/stall_lock" },
			Operation{ "/fixture/stall-eject", "drives/stall_eject" },
			Operation{ "/fixture/stall-poweroff", "drives/stall_poweroff" } } ) {
		CancellationSource cancellation;
		auto pool = ThreadPool::createUnique( 1 );
		auto task = pool->submit( [path = operation.path, token = cancellation.token()] {
			return DeviceMonitor::eject( path, token );
		} );
		// Cancel only after the service accepted the blocking call, not during discovery.
		const bool reached = waitForCall( operation.marker );
		const auto start = std::chrono::steady_clock::now();
		cancellation.cancel();
		pool.reset();
		EXPECT_TRUE( reached );
		EXPECT_TRUE( std::chrono::steady_clock::now() - start < std::chrono::seconds( 5 ) );
		auto error = std::move( task ).get();
		ASSERT_TRUE( error.has_value() );
		EXPECT_FALSE( error->empty() );
	}
	CancellationSource cancelled;
	cancelled.cancel();
	EXPECT_FALSE( DeviceMonitor::eject( "/fixture/busy", cancelled.token() ).empty() );
}

UTEST( EEFMDevices, UDisksDiscoveryCancellationReleasesTheWorkerPool ) {
	if ( Sys::getEnv( "EEFM_TEST_UDISKS_DISCOVERY" ) != "1" )
		UTEST_SKIP( "Run with the stalled private UDisks discovery fixture" );
	CancellationSource cancellation;
	auto pool = ThreadPool::createUnique( 1 );
	auto task = pool->submit( [token = cancellation.token()] {
		std::vector<Place> devices{ device( "/fixture/busy" ) };
		DeviceMonitor::updateCapabilities( devices, token );
		return devices[0].ejectable;
	} );
	const bool reached = waitForCall( "GetManagedObjects" );
	const auto start = std::chrono::steady_clock::now();
	cancellation.cancel();
	pool.reset();
	EXPECT_TRUE( reached );
	EXPECT_TRUE( std::chrono::steady_clock::now() - start < std::chrono::seconds( 5 ) );
	auto capability = std::move( task ).get();
	ASSERT_TRUE( capability.has_value() );
	EXPECT_FALSE( *capability );
}

UTEST( EEFMDevices, PollingWorkerCanFinishAfterStop ) {
	if ( Sys::getEnv( "EEFM_TEST_UDISKS" ) != "1" )
		UTEST_SKIP( "Run in the private fixture's stable mount namespace" );
	DeviceMonitor monitor;
	auto pool = ThreadPool::createUnique( 1 );
	// Consume any initial notification, then hold a queued check until after UI shutdown.
	pool->submit( [&monitor] { return monitor.changed(); } ).get();
	std::mutex mutex;
	std::unique_lock lock( mutex );
	auto check = pool->submit( [&monitor, &mutex] {
		std::lock_guard guard( mutex );
		return monitor.changed();
	} );
	monitor.stop();
	lock.unlock();
	pool.reset();
	auto changed = std::move( check ).get();
	ASSERT_TRUE( changed.has_value() );
	EXPECT_FALSE( *changed );
}

#endif
