#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include "../../tools/eproc/platform/macos/process_icon_resolver_macos.hpp"
#include "../../tools/eproc/process_collector.hpp"
#include "utest.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sys/resource.h>
#include <unistd.h>

using namespace eproc;

UTEST( EProcMacOSCollector, CollectsCurrentProcessAndUsesMicrosecondCpuTime ) {
	auto collector = ProcessCollector::create();
	ASSERT_TRUE( collector != nullptr );
	std::vector<ProcessInfo> processes;
	SystemInfo system;
	ASSERT_TRUE( collector->collect( processes, system ) );
	const auto current =
		std::find_if( processes.begin(), processes.end(),
					  []( const ProcessInfo& proc ) { return proc.pid == getpid(); } );
	ASSERT_TRUE( current != processes.end() );
	EXPECT_EQ( system.clockTicksPerSecond, 1'000'000 );
	EXPECT_TRUE( system.cpuCount > 0 );
	ASSERT_EQ( system.coreCpuUsage.size(), static_cast<size_t>( system.cpuCount ) );
	EXPECT_TRUE( system.totalMemory > 0 );
	EXPECT_TRUE( current->ownedByCurrentUser );
	EXPECT_TRUE( current->vmRSS > 0 );
	EXPECT_TRUE( current->startTime > 0 );

	rusage usage{};
	ASSERT_EQ( getrusage( RUSAGE_SELF, &usage ), 0 );
	const Int64 userUs =
		static_cast<Int64>( usage.ru_utime.tv_sec ) * 1'000'000 + usage.ru_utime.tv_usec;
	// A broad bound tolerates the work between the two samples but catches treating Mach ticks as
	// nanoseconds, which reports about 1/40 of actual CPU time on Apple Silicon.
	if ( userUs > 10'000 ) {
		EXPECT_TRUE( current->userTime > userUs / 2 );
		EXPECT_TRUE( current->userTime < userUs * 2 + 1'000'000 );
	}
	const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds( 100 );
	while ( std::chrono::steady_clock::now() < end ) {
		// Keep the process busy across a second CPU sample.
	}
	ASSERT_TRUE( collector->collect( processes, system ) );
	const auto second =
		std::find_if( processes.begin(), processes.end(),
					  []( const ProcessInfo& proc ) { return proc.pid == getpid(); } );
	ASSERT_TRUE( second != processes.end() );
	EXPECT_TRUE( second->getCpuForSort() > 0 );
	EXPECT_TRUE( system.cpuUsage >= 0.f && system.cpuUsage <= 100.f );
	ASSERT_EQ( system.coreCpuUsage.size(), static_cast<size_t>( system.cpuCount ) );
	for ( float usage : system.coreCpuUsage )
		EXPECT_TRUE( usage >= 0.f && usage <= 100.f );
	EXPECT_TRUE( std::any_of( system.coreCpuUsage.begin(), system.coreCpuUsage.end(),
							  []( float usage ) { return usage > 0.f; } ) );
}

UTEST( EProcMacOSCollector, ResolvesDeclaredBundleIcon ) {
	char directory[] = "/private/tmp/eproc-icon-test-XXXXXX";
	ASSERT_TRUE( mkdtemp( directory ) != nullptr );
	const std::filesystem::path root( directory );
	const auto contents = root / "Example.app/Contents";
	std::filesystem::create_directories( contents / "Resources" );
	std::filesystem::create_directories( contents / "MacOS" );
	{
		std::ofstream plist( contents / "Info.plist" );
		plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				 "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
				 "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
				 "<plist version=\"1.0\"><dict>"
				 "<key>CFBundleExecutable</key><string>Example</string>"
				 "<key>CFBundleIdentifier</key><string>org.eepp.eproc.icon-test</string>"
				 "<key>CFBundleIconFile</key><string>ExampleIcon</string>"
				 "</dict></plist>";
	}
	std::ofstream( contents / "Resources/ExampleIcon.icns" ).put( '\0' );
	std::ofstream( contents / "MacOS/Example" ).put( '\0' );
	ProcessIconResolverMacOS resolver;
	const std::string resolved = resolver.resolve( ( contents / "MacOS/Example" ).string() );
	EXPECT_TRUE( resolved == ( contents / "Resources/ExampleIcon.icns" ).string() );
	std::filesystem::remove_all( root );
}
#endif
