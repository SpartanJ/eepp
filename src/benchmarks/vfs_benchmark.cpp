#include "../tests/unit_tests/utest.hpp"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/vfs.hpp>
#include <mutex>
#include <thread>

using namespace EE;
using namespace EE::VFS;
using EE::System::Clock;
using EE::System::FileSystem;
using EE::System::Sys;
using EE::System::ThreadPool;

namespace {

template <typename T> VFSResult<T> wait( VFSTask<T>&& task ) {
	auto value = std::move( task ).get();
	if ( !value )
		return Err( VFSError( VFSErrorCode::Unknown ) );
	return std::move( *value );
}

double ms( const Clock& clock ) {
	return clock.getElapsedTime().asMicroseconds() / 1000.0;
}

std::string benchDir( const std::string& name ) {
	std::string dir = Sys::getTempPath();
	FileSystem::dirAddSlashAtEnd( dir );
	dir += "eepp-vfs-bench-" + name + "-" + String::toString( Sys::getProcessID() ) + "/";
	FileSystem::makeDir( dir, true );
	return dir;
}

void populate( const std::string& dir, int count ) {
	for ( int i = 0; i < count; ++i ) {
		FileSystem::fileWrite( dir + "file-" + String::toString( i ) + ".txt",
							   std::string_view( "x" ) );
	}
}

/** Executor drained by the calling thread, standing in for a UI loop. */
class LoopExecutor : public Executor {
  public:
	void execute( Function fn ) override {
		std::lock_guard<std::mutex> lock( mMutex );
		mQueue.emplace_back( std::move( fn ) );
	}

	size_t drain() {
		std::vector<Function> queue;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			queue.swap( mQueue );
		}
		for ( auto& fn : queue )
			fn();
		return queue.size();
	}

  private:
	std::mutex mMutex;
	std::vector<Function> mQueue;
};

} // namespace

UTEST( VFSBenchmark, taskAndContinuationOverhead ) {
	constexpr int Iterations = 1000000;
	Clock clock;
	long long sum = 0;
	for ( int i = 0; i < Iterations; ++i ) {
		auto task = makeReadyTask( i ).then( []( int v ) { return v + 1; } );
		sum += std::move( task ).takeValue();
	}
	double readyMs = ms( clock );
	printf( "  ready task + then():          %8.1f ns/op\n", readyMs * 1e6 / Iterations );

	clock.restart();
	for ( int i = 0; i < Iterations; ++i ) {
		TaskResolver<int> resolve;
		auto task = Task<int>::create( resolve ).then( []( int v ) { return v * 2; } );
		resolve( i );
		sum += std::move( task ).takeValue();
	}
	printf( "  pending task resolve + then(): %8.1f ns/op\n", ms( clock ) * 1e6 / Iterations );

	LoopExecutor loop;
	clock.restart();
	for ( int i = 0; i < Iterations; ++i ) {
		auto task = makeReadyTask( i ).then( loop, []( int v ) { return v; } );
		loop.drain();
		sum += std::move( task ).takeValue();
	}
	printf( "  then( executor ) hop:         %8.1f ns/op\n", ms( clock ) * 1e6 / Iterations );

	auto pool = ThreadPool::createUnique( 4 );
	constexpr int Submits = 200000;
	clock.restart();
	std::vector<Task<int>> tasks;
	tasks.reserve( Submits );
	for ( int i = 0; i < Submits; ++i )
		tasks.push_back( pool->submit( [i] { return i; } ) );
	for ( auto& task : tasks )
		sum += *std::move( task ).get();
	printf( "  ThreadPool::submit round trip: %8.1f ns/op (4 workers)\n",
			ms( clock ) * 1e6 / Submits );
	EXPECT_TRUE( sum != 0 );
}

UTEST( VFSBenchmark, localDirectoryListing ) {
	VirtualFileSystem vfs;
	constexpr int Repetitions = 5;
	for ( int count : { 100, 10000, 100000 } ) {
		std::string dir = benchDir( "list" + String::toString( count ) );
		populate( dir, count );
		URI uri = URI::fromFSPath( dir );
		DirectoryOpenOptions light;
		light.metadata = DirectoryMetadata::NamesAndTypes;
		light.resolveSymlinkTargets = false;

		// Best of several runs: the directory is warm in the page cache after the first one.
		double firstBatch = 1e9, total = 1e9, namesOnly = 1e9, sync = 1e9;
		for ( int run = 0; run < Repetitions; ++run ) {
			Clock clock;
			auto opened = wait( vfs.openDirectory( uri ) );
			ASSERT_TRUE( opened.hasValue() );
			DirectoryHandle handle( std::move( opened ).value() );
			auto first = wait( handle.read( 128 ) );
			firstBatch = std::min( firstBatch, ms( clock ) );
			ASSERT_TRUE( first.hasValue() );
			handle.close();

			clock.restart();
			auto all = wait( vfs.listAll( uri ) );
			total = std::min( total, ms( clock ) );
			ASSERT_TRUE( all.hasValue() );
			EXPECT_EQ( static_cast<size_t>( count ), all.value().size() );

			clock.restart();
			auto names = wait( vfs.listAll( uri, light ) );
			namesOnly = std::min( namesOnly, ms( clock ) );
			ASSERT_TRUE( names.hasValue() );

			clock.restart();
			auto baseline = FileSystem::filesInfoGetInPath( dir );
			sync = std::min( sync, ms( clock ) );
			EXPECT_EQ( baseline.size(), names.value().size() );
		}

		printf( "  list %6d entries: first batch %7.2f ms | full metadata %8.2f ms | names+types "
				"%8.2f ms | FileSystem::filesInfoGetInPath %8.2f ms\n",
				count, firstBatch, total, namesOnly, sync );
		FileSystem::dirRemoveAll( dir );
	}
	printf( "  sizeof(VFSFileInfo) = %zu bytes\n", sizeof( VFSFileInfo ) );
}

UTEST( VFSBenchmark, localThroughput ) {
	VirtualFileSystem vfs;
	std::string dir = benchDir( "io" );
	URI file = URI::fromFSPath( dir + "big.bin" );
	std::vector<Uint8> data( 64 * 1024 * 1024 );
	for ( size_t i = 0; i < data.size(); ++i )
		data[i] = static_cast<Uint8>( i * 31 );
	const double mb = data.size() / ( 1024.0 * 1024.0 );

	Clock clock;
	WriteOptions fast;
	fast.fsync = false;
	auto written = wait( vfs.writeAll( file, data, fast ) );
	double writeMs = ms( clock );
	ASSERT_TRUE( written.hasValue() );

	clock.restart();
	auto read = wait( vfs.readAll( file ) );
	double readMs = ms( clock );
	ASSERT_TRUE( read.hasValue() );
	EXPECT_TRUE( read.value() == data );

	clock.restart();
	auto copied = wait( vfs.copy( file, URI::fromFSPath( dir + "copy.bin" ) ).completion() );
	double copyMs = ms( clock );
	ASSERT_TRUE( copied.hasValue() );

	printf( "  writeAll (atomic, no fsync) %.0f MiB: %7.1f ms (%6.0f MiB/s)\n", mb, writeMs,
			mb / ( writeMs / 1000 ) );
	printf( "  readAll               %.0f MiB: %7.1f ms (%6.0f MiB/s)\n", mb, readMs,
			mb / ( readMs / 1000 ) );
	printf( "  copy (256 KiB chunks) %.0f MiB: %7.1f ms (%6.0f MiB/s)\n", mb, copyMs,
			mb / ( copyMs / 1000 ) );
	FileSystem::dirRemoveAll( dir );
}

UTEST( VFSBenchmark, watchBurstDelivery ) {
	VirtualFileSystem vfs;
	std::string dir = benchDir( "watch" );
	auto opened = wait( vfs.watch( URI::fromFSPath( dir ) ) );
	ASSERT_TRUE( opened.hasValue() );
	WatchHandle watch( std::move( opened ).value() );
	LoopExecutor loop;
	std::atomic<size_t> events{ 0 };
	std::atomic<size_t> batches{ 0 };
	watch.onEvents( loop, [&]( WatchEventBatch&& batch ) {
		events += batch.events.size();
		++batches;
	} );
	constexpr int Count = 2000;
	Clock clock;
	populate( dir, Count );
	double createMs = ms( clock );
	while ( events.load() < static_cast<size_t>( Count ) &&
			clock.getElapsedTime().asSeconds() < 10 ) {
		loop.drain();
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	double deliveredMs = ms( clock );
	printf( "  %d file creations: created in %.1f ms, %zu events delivered in %.1f ms in %zu "
			"batches\n",
			Count, createMs, events.load(), deliveredMs, batches.load() );
	EXPECT_GE( events.load(), static_cast<size_t>( Count ) );
	watch.reset();
	FileSystem::dirRemoveAll( dir );
}

/** Optional: set EEPP_BENCH_SFTP_URI=sftp://host/writable/dir (key or agent authentication). */
UTEST( VFSBenchmark, sftpRoundTrips ) {
	const char* env = std::getenv( "EEPP_BENCH_SFTP_URI" );
	if ( !env || !*env ) {
		printf( "  EEPP_BENCH_SFTP_URI not set, SFTP benchmark skipped\n" );
		return;
	}
	VirtualFileSystem vfs;
	if ( !vfs.provider( "sftp" ) ) {
		printf( "  built without --with-vfs-sftp, SFTP benchmark skipped\n" );
		return;
	}
	URI base( env );
	Clock clock;
	auto first = wait( vfs.stat( base ) );
	double connectMs = ms( clock );
	ASSERT_TRUE( first.hasValue() );
	clock.restart();
	for ( int i = 0; i < 100; ++i )
		wait( vfs.stat( base ) );
	double statMs = ms( clock );
	clock.restart();
	auto listed = wait( vfs.listAll( base ) );
	double listMs = ms( clock );
	std::vector<Uint8> data( 32 * 1024 * 1024, 'z' );
	URI file = base.withAppendedSegment( ".eepp-vfs-bench.bin" );
	clock.restart();
	auto written = wait( vfs.writeAll( file, data ) );
	double writeMs = ms( clock );
	clock.restart();
	auto read = wait( vfs.readAll( file ) );
	double readMs = ms( clock );
	wait( vfs.remove( file ) );
	printf( "  connect + auth + first stat: %8.1f ms\n", connectMs );
	printf( "  100 sequential stats:        %8.1f ms (%.2f ms each)\n", statMs, statMs / 100 );
	printf( "  list %zu entries:            %8.1f ms\n", listed ? listed.value().size() : 0,
			listMs );
	printf( "  write 32 MiB: %s %8.1f ms | read 32 MiB: %s %8.1f ms\n", written ? "ok" : "FAILED",
			writeMs, read ? "ok" : "FAILED", readMs );
}
