#ifndef EE_VFS_TEST_UTILS_HPP
#define EE_VFS_TEST_UTILS_HPP

// Shared helpers for the eepp-vfs test suites (included by several translation units).

#include "utest.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/vfs.hpp>
#include <mutex>
#include <thread>

namespace vfstest {

using namespace EE;
using namespace EE::VFS;
using EE::System::FileSystem;
using EE::System::Sys;

template <typename T> VFSResult<T> waitFor( VFSTask<T>&& task ) {
	auto value = std::move( task ).get();
	if ( !value )
		return Err( VFSError( VFSErrorCode::Unknown, URI(), "abandoned" ) );
	return std::move( *value );
}

/** @return The error code of a result, or VFSErrorCode::None when it succeeded. */
template <typename T> VFSErrorCode codeOf( const VFSResult<T>& result ) {
	return result ? VFSErrorCode::None : result.error().code;
}

struct TempDir {
	std::string path;

	TempDir() {
		static std::atomic<int> counter{ 0 };
		path = Sys::getTempPath();
		FileSystem::dirAddSlashAtEnd( path );
		path += "eepp-vfs-test-" + String::toString( Sys::getProcessID() ) + "-" +
				String::toString( ++counter ) + "-" + String::toString( Sys::getTicks() );
		FileSystem::makeDir( path, true );
		FileSystem::dirAddSlashAtEnd( path );
	}

	~TempDir() {
		if ( !path.empty() )
			FileSystem::dirRemoveAll( path );
	}

	URI uri() const { return URI::fromFSPath( path ); }
};

/** Thread-safe queue executor drained explicitly by the test thread (stands in for a UI loop). */
class TestLoop : public Executor {
  public:
	void execute( Function function ) override {
		std::lock_guard<std::mutex> lock( mMutex );
		mQueue.emplace_back( std::move( function ) );
	}

	void drain() {
		std::vector<Function> queue;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			queue.swap( mQueue );
		}
		for ( auto& fn : queue )
			fn();
	}

	/** Drains until pred() holds or the timeout expires. */
	template <typename Pred> bool runUntil( Pred pred, int timeoutMs = 5000 ) {
		auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds( timeoutMs );
		while ( std::chrono::steady_clock::now() < end ) {
			drain();
			if ( pred() )
				return true;
			std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
		}
		drain();
		return pred();
	}

  private:
	std::mutex mMutex;
	std::vector<Function> mQueue;
};

inline std::string readString( VirtualFileSystem& vfs, const URI& uri ) {
	auto data = waitFor( vfs.readAll( uri ) );
	if ( !data )
		return "<error: " + data.error().toString() + ">";
	return std::string( data.value().begin(), data.value().end() );
}

inline VFSResult<void> writeString( VirtualFileSystem& vfs, const URI& uri,
									std::string_view content ) {
	auto written = waitFor( vfs.writeAll( uri, content ) );
	if ( !written )
		return Err( std::move( written ).error() );
	return Ok();
}

inline std::vector<std::string> sortedNames( const std::vector<FileInfo>& entries ) {
	std::vector<std::string> names;
	for ( const auto& entry : entries )
		names.push_back( entry.name );
	std::sort( names.begin(), names.end() );
	return names;
}

/** Provider conformance suite shared by every writable provider. base must be an empty,
 * writable directory. */
inline void providerConformance( int* utest_result, VirtualFileSystem& vfs, const URI& base ) {
	ProviderCapabilities caps = vfs.capabilities( base );
	EXPECT_TRUE( caps.has( VFSCapability::Read ) );
	EXPECT_TRUE( caps.has( VFSCapability::Write ) );
	EXPECT_TRUE( caps.has( VFSCapability::List ) );

	// stat of the base directory.
	auto baseInfo = waitFor( vfs.stat( base ) );
	EXPECT_TRUE( baseInfo.hasValue() );
	if ( baseInfo )
		EXPECT_TRUE( baseInfo.value().isDirectory() );

	// Errors.
	URI missing = base.withAppendedSegment( "missing" );
	auto missingInfo = waitFor( vfs.stat( missing ) );
	EXPECT_TRUE( missingInfo.hasError() );
	if ( !missingInfo )
		EXPECT_EQ( VFSErrorCode::NotFound, missingInfo.error().code );

	// File creation, explicit-offset write and read.
	URI file = base.withAppendedSegment( "file #1%.txt" );
	{
		auto opened = waitFor(
			vfs.open( file, OpenFlags::ReadWrite | OpenFlags::Create | OpenFlags::Exclusive ) );
		EXPECT_TRUE( opened.hasValue() );
		if ( opened ) {
			FileHandle handle( std::move( opened ).value() );
			std::string a = "hello ", b = "world";
			auto w1 = waitFor(
				handle.write( 0, std::span<const Uint8>( (const Uint8*)a.data(), a.size() ) ) );
			auto w2 = waitFor( handle.write(
				a.size(), std::span<const Uint8>( (const Uint8*)b.data(), b.size() ) ) );
			EXPECT_TRUE( w1 && w1.value() == a.size() );
			EXPECT_TRUE( w2 && w2.value() == b.size() );
			std::vector<Uint8> buffer( 5 );
			auto r = waitFor( handle.read( 6, buffer ) );
			EXPECT_TRUE( r && r.value() == 5 );
			EXPECT_TRUE( std::string( buffer.begin(), buffer.end() ) == "world" );
			auto eof = waitFor( handle.read( 11, buffer ) );
			EXPECT_TRUE( eof && eof.value() == 0 );
			auto st = waitFor( handle.stat() );
			EXPECT_TRUE( st && st.value().size() && *st.value().size() == 11 );
			EXPECT_TRUE( waitFor( handle.truncate( 5 ) ).hasValue() );
			EXPECT_TRUE( waitFor( handle.close() ).hasValue() );
			auto afterClose = waitFor( handle.read( 0, buffer ) );
			EXPECT_TRUE( afterClose.hasError() );
		}
	}
	EXPECT_STDSTREQ( "hello", readString( vfs, file ) );

	// Exclusive create on an existing file.
	auto exclusive =
		waitFor( vfs.open( file, OpenFlags::Write | OpenFlags::Create | OpenFlags::Exclusive ) );
	EXPECT_TRUE( exclusive.hasError() );
	if ( !exclusive )
		EXPECT_EQ( VFSErrorCode::AlreadyExists, exclusive.error().code );

	// Opening a directory as a file.
	auto openDir = waitFor( vfs.open( base, OpenFlags::Read ) );
	EXPECT_TRUE( openDir.hasError() );

	// mkdir / NotDirectory / listing with batches and explicit EOF.
	URI dir = base.withAppendedSegment( "dir" );
	EXPECT_TRUE( waitFor( vfs.makeDir( dir ) ).hasValue() );
	auto again = waitFor( vfs.makeDir( dir ) );
	EXPECT_TRUE( again.hasError() );
	if ( !again )
		EXPECT_EQ( VFSErrorCode::AlreadyExists, again.error().code );
	auto notDir = waitFor( vfs.openDirectory( file ) );
	EXPECT_TRUE( notDir.hasError() );
	if ( !notDir )
		EXPECT_EQ( VFSErrorCode::NotDirectory, notDir.error().code );
	for ( int i = 0; i < 10; ++i )
		EXPECT_TRUE(
			writeString( vfs, dir.withAppendedSegment( "f" + String::toString( i ) ), "x" ) );
	{
		auto opened = waitFor( vfs.openDirectory( dir ) );
		EXPECT_TRUE( opened.hasValue() );
		if ( opened ) {
			DirectoryHandle handle( std::move( opened ).value() );
			size_t total = 0;
			int batches = 0;
			while ( true ) {
				auto batch = waitFor( handle.read( 3 ) );
				EXPECT_TRUE( batch.hasValue() );
				if ( !batch )
					break;
				EXPECT_LE( batch.value().entries.size(), 3u );
				total += batch.value().entries.size();
				++batches;
				for ( const auto& entry : batch.value().entries ) {
					EXPECT_TRUE( entry.isRegularFile() );
					EXPECT_TRUE( entry.uriIn( dir ).getLastPathSegment() == entry.name );
				}
				if ( batch.value().endOfDirectory )
					break;
				EXPECT_LT( batches, 20 );
			}
			EXPECT_EQ( 10u, total );
			handle.close();
		}
	}
	auto listed = waitFor( vfs.listAll( base ) );
	EXPECT_TRUE( listed.hasValue() );
	if ( listed ) {
		auto names = sortedNames( listed.value() );
		EXPECT_EQ( 2u, names.size() );
		if ( names.size() == 2 ) {
			EXPECT_STDSTREQ( "dir", names[0] );
			EXPECT_STDSTREQ( "file #1%.txt", names[1] );
		}
	}

	// Non-recursive remove of a non-empty directory fails; recursive succeeds.
	auto notEmpty = waitFor( vfs.remove( dir ) );
	EXPECT_TRUE( notEmpty.hasError() );
	if ( !notEmpty )
		EXPECT_EQ( VFSErrorCode::NotEmpty, notEmpty.error().code );

	// Rename: no-overwrite fails on existing destination, overwrite replaces.
	URI other = base.withAppendedSegment( "other.txt" );
	EXPECT_TRUE( writeString( vfs, other, "other" ) );
	auto noReplace = waitFor( vfs.rename( other, file ) );
	EXPECT_TRUE( noReplace.hasError() );
	if ( !noReplace )
		EXPECT_EQ( VFSErrorCode::AlreadyExists, noReplace.error().code );
	EXPECT_TRUE( waitFor( vfs.rename( other, file, RenameOptions{ true } ) ).hasValue() );
	EXPECT_STDSTREQ( "other", readString( vfs, file ) );
	EXPECT_TRUE( waitFor( vfs.stat( other ) ).hasError() );

	// Canonicalize resolves to an equivalent, stat-able URI.
	auto canonical = waitFor( vfs.canonicalize( file ) );
	EXPECT_TRUE( canonical.hasValue() );
	if ( canonical )
		EXPECT_STDSTREQ( "other", readString( vfs, canonical.value() ) );

	// Cancellation before the operation starts.
	CancellationSource cancel;
	cancel.cancel();
	auto cancelled = waitFor( vfs.stat( file, {}, cancel.token() ) );
	EXPECT_TRUE( cancelled.hasError() );
	if ( !cancelled )
		EXPECT_EQ( VFSErrorCode::Cancelled, cancelled.error().code );

	// Recursive remove.
	EXPECT_TRUE( waitFor( vfs.remove( dir, RemoveOptions{ true } ) ).hasValue() );
	EXPECT_TRUE( waitFor( vfs.stat( dir ) ).hasError() );
	EXPECT_TRUE( waitFor( vfs.remove( file ) ).hasValue() );
	auto empty = waitFor( vfs.listAll( base ) );
	EXPECT_TRUE( empty && empty.value().empty() );
}

struct EventCollector {
	std::mutex mutex;
	std::vector<WatchEvent> events;
	std::atomic<int> batches{ 0 };
	WatchStatus lastStatus{ WatchStatus::Active };

	WatchCallback callback() {
		return [this]( WatchEventBatch&& batch ) {
			std::lock_guard<std::mutex> lock( mutex );
			++batches;
			lastStatus = batch.status;
			for ( auto& event : batch.events )
				events.push_back( std::move( event ) );
		};
	}

	bool has( WatchEventType type, const std::string& path ) {
		std::lock_guard<std::mutex> lock( mutex );
		for ( const auto& event : events )
			if ( event.type == type && event.relativePath == path )
				return true;
		return false;
	}
};

} // namespace vfstest

#endif
