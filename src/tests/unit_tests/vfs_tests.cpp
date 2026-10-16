#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <eepp/system/directorypack.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/packfileindex.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/zip.hpp>
#include <eepp/vfs.hpp>
#include <eepp/vfs/watchers/localwatchbackend.hpp>
#include <mutex>
#include <thread>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <sys/stat.h> // umask
#include <unistd.h>	  // symlink, link
#endif

using namespace EE;
using namespace EE::VFS;
using EE::System::FileSystem;
using EE::System::Sys;
using EE::System::ThreadPool;
using namespace vfstest;

// ---------------------------------------------------------------------------------------------
// Value types
// ---------------------------------------------------------------------------------------------

UTEST( VFSFileInfo, unknownMetadataIsDistinguishableFromZero ) {
	FileInfo info;
	EXPECT_FALSE( info.size().has_value() );
	EXPECT_FALSE( info.modifiedTime().has_value() );
	info.setSize( 0 ).setModifiedTime( 0 ).setPermissions( 0 );
	EXPECT_TRUE( info.size().has_value() );
	EXPECT_EQ( 0u, *info.size() );
	EXPECT_TRUE( info.permissions().has_value() );
	EXPECT_FALSE( info.uid().has_value() );
}

UTEST( VFSFileInfo, revisionsCompare ) {
	FileRevision empty;
	EXPECT_FALSE( empty.matches( empty ) );
	auto a = FileRevision::fromValues( { 1, 2, 3 } );
	auto b = FileRevision::fromValues( { 1, 2, 3 } );
	auto c = FileRevision::fromValues( { 1, 2, 4 } );
	EXPECT_TRUE( a.matches( b ) );
	EXPECT_FALSE( a.matches( c ) );
	EXPECT_EQ( 24u, a.size() );
	EXPECT_EQ( 48u, a.toHexString().size() );
}

UTEST( VFSFileInfo, symlinkTargetResolution ) {
	FileInfo info;
	URI link( "sftp://host/a/b/link" );
	info.symlinkTarget = "../c/target #x";
	auto resolved = info.symlinkTargetURI( link );
	EXPECT_TRUE( resolved.has_value() );
	EXPECT_STDSTREQ( "/a/c/target #x", resolved->getPath() );
	EXPECT_STDSTREQ( "host", resolved->getHost() );
	info.symlinkTarget = "/abs/./t";
	EXPECT_STDSTREQ( "/abs/t", info.symlinkTargetURI( link )->getPath() );
}

UTEST( VFSFileInfo, layout ) {
	// Keep the hot listing value compact: 4 strings + revision + scalars, no per-entry URI.
	EXPECT_LE( sizeof( FileInfo ), 4 * sizeof( std::string ) + 40 + 56 );
}

// ---------------------------------------------------------------------------------------------
// Providers
// ---------------------------------------------------------------------------------------------

UTEST( VFSProvider, localConformance ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	providerConformance( utest_result, vfs, tmp.uri() );
}

UTEST( VFSProvider, memoryConformanceSynchronous ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	mem->putDirectory( URI( "mem://t/base" ) );
	providerConformance( utest_result, vfs, URI( "mem://t/base" ) );
}

UTEST( VFSProvider, memoryConformanceAsynchronous ) {
	auto pool = ThreadPool::createShared( 1 );
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>( "mem", pool.get() );
	vfs.registerProvider( mem );
	mem->putDirectory( URI( "mem://t/base" ) );
	providerConformance( utest_result, vfs, URI( "mem://t/base" ) );
	vfs.unregisterProvider( "mem" );
}

UTEST( VFSProvider, unknownSchemeIsUnsupported ) {
	VirtualFileSystem vfs;
	auto result = waitFor( vfs.stat( URI( "nope://x/y" ) ) );
	EXPECT_TRUE( result.hasError() );
	EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( result ) );
	auto relative = waitFor( vfs.stat( URI( "relative/path" ) ) );
	EXPECT_EQ( VFSErrorCode::InvalidURI, codeOf( relative ) );
}

UTEST( VFSProvider, localMetadataAndRevisions ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	URI file = tmp.uri().withAppendedSegment( "rev.txt" );
	EXPECT_TRUE( writeString( vfs, file, "one" ) );
	auto first = waitFor( vfs.stat( file ) );
	EXPECT_TRUE( first.hasValue() );
	EXPECT_TRUE( first.value().size().has_value() );
	EXPECT_EQ( 3u, *first.value().size() );
	EXPECT_TRUE( first.value().modifiedTime().has_value() );
	EXPECT_FALSE( first.value().revision.empty() );
	EXPECT_STDSTREQ( "rev.txt", first.value().name );
	// Atomic replacement changes the revision even with identical size.
	EXPECT_TRUE( writeString( vfs, file, "two" ) );
	auto second = waitFor( vfs.stat( file ) );
	EXPECT_FALSE( first.value().revision.matches( second.value().revision ) );
	// Hidden files.
	EXPECT_TRUE( writeString( vfs, tmp.uri().withAppendedSegment( ".hidden" ), "" ) );
	auto hidden = waitFor( vfs.stat( tmp.uri().withAppendedSegment( ".hidden" ) ) );
	EXPECT_TRUE( hidden && hidden.value().hidden );
	// UTF-8 names round-trip.
	URI utf8 = tmp.uri().withAppendedSegment( "\xc3\xa1rbol-\xe6\x97\xa5.txt" );
	EXPECT_TRUE( writeString( vfs, utf8, "u" ) );
	auto listed = waitFor( vfs.listAll( tmp.uri() ) );
	bool found = false;
	for ( const auto& entry : listed.value() )
		found = found || entry.name == "\xc3\xa1rbol-\xe6\x97\xa5.txt";
	EXPECT_TRUE( found );
	auto space = waitFor( vfs.spaceInfo( tmp.uri() ) );
	EXPECT_TRUE( space && space.value().capacity > 0 );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( VFSProvider, localSymlinks ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	URI target = tmp.uri().withAppendedSegment( "target" );
	EXPECT_TRUE( waitFor( vfs.makeDir( target ) ).hasValue() );
	EXPECT_TRUE( writeString( vfs, target.withAppendedSegment( "inner.txt" ), "inner" ) );
	std::string linkPath = tmp.path + "link";
	EXPECT_EQ( 0, symlink( "target", linkPath.c_str() ) );
	URI link = URI::fromFSPath( linkPath );

	auto followed = waitFor( vfs.stat( link ) );
	EXPECT_TRUE( followed && followed.value().type == FileType::Directory );
	auto lstat = waitFor( vfs.stat( link, StatOptions{ false } ) );
	EXPECT_TRUE( lstat && lstat.value().type == FileType::Symlink );
	EXPECT_EQ( FileType::Directory, lstat.value().targetType );
	EXPECT_STDSTREQ( "target", lstat.value().symlinkTarget );
	EXPECT_TRUE( lstat.value().isDirectory() );

	auto listed = waitFor( vfs.listAll( tmp.uri() ) );
	for ( const auto& entry : listed.value() ) {
		if ( entry.name == "link" ) {
			EXPECT_EQ( FileType::Symlink, entry.type );
			EXPECT_EQ( FileType::Directory, entry.targetType );
		}
	}

	auto canonical = waitFor( vfs.canonicalize( link ) );
	EXPECT_TRUE( canonical.hasValue() );
	EXPECT_STDSTREQ( "target", canonical.value().getLastPathSegment() );

	// Recursive remove deletes the link, never the target's content.
	EXPECT_TRUE( waitFor( vfs.remove( link, RemoveOptions{ true } ) ).hasValue() );
	EXPECT_STDSTREQ( "inner", readString( vfs, target.withAppendedSegment( "inner.txt" ) ) );

	// Saving through a file symlink keeps the link and writes the target.
	URI fileTarget = tmp.uri().withAppendedSegment( "real.txt" );
	EXPECT_TRUE( writeString( vfs, fileTarget, "v1" ) );
	std::string fileLink = tmp.path + "alias.txt";
	EXPECT_EQ( 0, symlink( "real.txt", fileLink.c_str() ) );
	EXPECT_TRUE( writeString( vfs, URI::fromFSPath( fileLink ), "v2" ) );
	auto aliasInfo = waitFor( vfs.stat( URI::fromFSPath( fileLink ), StatOptions{ false } ) );
	EXPECT_TRUE( aliasInfo && aliasInfo.value().isSymlink() );
	EXPECT_STDSTREQ( "v2", readString( vfs, fileTarget ) );
}

UTEST( VFSProvider, localPermissionsArePreserved ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	URI script = tmp.uri().withAppendedSegment( "run.sh" );
	EXPECT_TRUE( writeString( vfs, script, "#!/bin/sh" ) );
	EXPECT_TRUE( waitFor( vfs.setPermissions( script, 0750 ) ).hasValue() );
	// Atomic rewrite keeps the mode of the replaced file.
	EXPECT_TRUE( writeString( vfs, script, "#!/bin/sh\necho" ) );
	auto info = waitFor( vfs.stat( script ) );
	EXPECT_EQ( 0750u, *info.value().permissions() );
}
#endif

// ---------------------------------------------------------------------------------------------
// Orchestration
// ---------------------------------------------------------------------------------------------

UTEST( VFSOrchestration, writeAllDetectsExternalChanges ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	URI file = tmp.uri().withAppendedSegment( "doc.txt" );
	auto saved = waitFor( vfs.writeAll( file, std::string_view( "v1" ) ) );
	EXPECT_TRUE( saved.hasValue() );
	FileRevision revision = saved.value().revision;

	// A save with the current revision succeeds and yields a new revision.
	WriteOptions options;
	options.expectedRevision = revision;
	auto resaved = waitFor( vfs.writeAll( file, std::string_view( "v2" ), options ) );
	EXPECT_TRUE( resaved.hasValue() );
	EXPECT_FALSE( resaved.value().revision.matches( revision ) );

	// Someone else changes the file; saving with the stale revision is refused.
	EXPECT_TRUE( FileSystem::fileWrite( tmp.path + "doc.txt", std::string_view( "external!" ) ) );
	options.expectedRevision = resaved.value().revision;
	auto conflict = waitFor( vfs.writeAll( file, std::string_view( "v3" ), options ) );
	EXPECT_TRUE( conflict.hasError() );
	EXPECT_EQ( VFSErrorCode::ChangedExternally, codeOf( conflict ) );
	EXPECT_STDSTREQ( "external!", readString( vfs, file ) );

	// No temporary files are left behind.
	auto listed = waitFor( vfs.listAll( tmp.uri() ) );
	EXPECT_EQ( 1u, listed.value().size() );

	// Expected revision on a missing file is a conflict too.
	options.expectedRevision = revision;
	auto removed = waitFor( vfs.writeAll( tmp.uri().withAppendedSegment( "gone.txt" ),
										  std::string_view( "x" ), options ) );
	EXPECT_EQ( VFSErrorCode::ChangedExternally, codeOf( removed ) );
}

UTEST( VFSOrchestration, writeAllCreateParentsAndMakeDirs ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	URI deep =
		tmp.uri().withAppendedSegment( "a" ).withAppendedSegment( "b" ).withAppendedSegment( "c" );
	EXPECT_TRUE( waitFor( vfs.makeDirs( deep ) ).hasValue() );
	EXPECT_TRUE( waitFor( vfs.makeDirs( deep ) ).hasValue() );
	auto info = waitFor( vfs.stat( deep ) );
	EXPECT_TRUE( info && info.value().isDirectory() );

	WriteOptions options;
	options.createParents = true;
	URI nested = tmp.uri().withAppendedSegment( "x" ).withAppendedSegment( "y.txt" );
	EXPECT_TRUE( waitFor( vfs.writeAll( nested, std::string_view( "ok" ), options ) ).hasValue() );
	EXPECT_STDSTREQ( "ok", readString( vfs, nested ) );

	// makeDirs through a file fails with NotDirectory.
	auto through = waitFor( vfs.makeDirs( nested.withAppendedSegment( "z" ) ) );
	EXPECT_EQ( VFSErrorCode::NotDirectory, codeOf( through ) );
}

UTEST( VFSOrchestration, readAllLimits ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	mem->putFile( URI( "mem://r/big" ), std::string( 1000, 'x' ) );
	auto ok = waitFor( vfs.readAll( URI( "mem://r/big" ) ) );
	EXPECT_EQ( 1000u, ok.value().size() );
	auto limited = waitFor( vfs.readAll( URI( "mem://r/big" ), {}, 999 ) );
	EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( limited ) );
	auto missing = waitFor( vfs.readAll( URI( "mem://r/none" ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( missing ) );
}

UTEST( VFSOrchestration, listAllLargeSynchronousDirectoryDoesNotRecurse ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	mem->putDirectory( URI( "mem://l/d" ) );
	for ( int i = 0; i < 20000; ++i )
		mem->putFile( URI( "mem://l/d/f" + String::toString( i ) ), "" );
	auto listed = waitFor( vfs.listAll( URI( "mem://l/d" ) ) );
	EXPECT_EQ( 20000u, listed.value().size() );
}

UTEST( VFSTransfer, crossProviderCopyAndProgress ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	mem->putFile( URI( "mem://s/tree/a.txt" ), "aaaa" );
	mem->putFile( URI( "mem://s/tree/sub/b.txt" ), std::string( 600 * 1024, 'b' ) );
	mem->putDirectory( URI( "mem://s/tree/empty" ) );

	TestLoop loop;
	URI destination = tmp.uri().withAppendedSegment( "copy" );
	CopyOptions options;
	options.chunkSize = 64 * 1024;
	Transfer transfer = vfs.copy( URI( "mem://s/tree" ), destination, options );
	std::atomic<int> updates{ 0 };
	TransferProgress last;
	auto subscription = transfer.onProgress(
		[&]( const TransferProgress& progress ) {
			++updates;
			last = progress;
		},
		&loop );
	auto completion = transfer.completion();
	EXPECT_TRUE( loop.runUntil( [&] { return !completion.isPending(); } ) );
	auto result = std::move( completion ).takeValue();
	EXPECT_TRUE( result.hasValue() );
	EXPECT_EQ( 2u, result.value().filesTransferred );
	EXPECT_EQ( 3u, result.value().directoriesCreated );
	EXPECT_EQ( 4u + 600 * 1024, result.value().bytesTransferred );
	loop.drain();
	EXPECT_GT( updates.load(), 0 );
	EXPECT_EQ( 2u, last.filesDone );
	EXPECT_TRUE( last.filesTotal && *last.filesTotal == 2 );
	EXPECT_TRUE( last.bytesTotal && *last.bytesTotal == 4u + 600 * 1024 );
	EXPECT_EQ( 4u + 600 * 1024, last.bytesDone );
	EXPECT_STDSTREQ( "aaaa", readString( vfs, destination.withAppendedSegment( "a.txt" ) ) );
	auto big = waitFor(
		vfs.readAll( destination.withAppendedSegment( "sub" ).withAppendedSegment( "b.txt" ) ) );
	EXPECT_EQ( 600u * 1024, big.value().size() );
	auto emptyDir = waitFor( vfs.stat( destination.withAppendedSegment( "empty" ) ) );
	EXPECT_TRUE( emptyDir && emptyDir.value().isDirectory() );
	// Sources untouched by a copy.
	EXPECT_TRUE( mem->fileContent( URI( "mem://s/tree/a.txt" ) ).has_value() );

	// Copy again: Fail policy refuses, Skip skips, Overwrite replaces.
	auto failAgain = waitFor( vfs.copy( URI( "mem://s/tree" ), destination ).completion() );
	EXPECT_EQ( VFSErrorCode::AlreadyExists, codeOf( failAgain ) );
	options.overwrite = OverwritePolicy::Skip;
	auto skipped = waitFor( vfs.copy( URI( "mem://s/tree" ), destination, options ).completion() );
	EXPECT_EQ( 2u, skipped.value().filesSkipped );
	mem->putFile( URI( "mem://s/tree/a.txt" ), "new" );
	options.overwrite = OverwritePolicy::Overwrite;
	auto replaced = waitFor( vfs.copy( URI( "mem://s/tree" ), destination, options ).completion() );
	EXPECT_TRUE( replaced.hasValue() );
	EXPECT_STDSTREQ( "new", readString( vfs, destination.withAppendedSegment( "a.txt" ) ) );
}

UTEST( VFSTransfer, moveNeverRemovesUncopiedSources ) {
	VirtualFileSystem vfs;
	auto memA = std::make_shared<MemoryProvider>( "mema" );
	auto memB = std::make_shared<MemoryProvider>( "memb" );
	vfs.registerProvider( memA );
	vfs.registerProvider( memB );
	memA->putFile( URI( "mema://x/src/keep.txt" ), "source-keep" );
	memA->putFile( URI( "mema://x/src/move.txt" ), "source-move" );
	memB->putFile( URI( "memb://x/dst/keep.txt" ), "destination" );

	CopyOptions options;
	options.overwrite = OverwritePolicy::Skip;
	auto result =
		waitFor( vfs.move( URI( "mema://x/src" ), URI( "memb://x/dst" ), options ).completion() );
	EXPECT_TRUE( result.hasValue() );
	EXPECT_FALSE( result.value().renamed );
	EXPECT_EQ( 1u, result.value().filesTransferred );
	EXPECT_EQ( 1u, result.value().filesSkipped );
	// The moved file left the source; the skipped one (and its directory) stayed.
	EXPECT_FALSE( memA->fileContent( URI( "mema://x/src/move.txt" ) ).has_value() );
	EXPECT_STDSTREQ( "source-keep", *memA->fileContent( URI( "mema://x/src/keep.txt" ) ) );
	EXPECT_STDSTREQ( "destination", *memB->fileContent( URI( "memb://x/dst/keep.txt" ) ) );
	EXPECT_STDSTREQ( "source-move", *memB->fileContent( URI( "memb://x/dst/move.txt" ) ) );
}

UTEST( VFSTransfer, sameProviderMoveRenames ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	mem->putFile( URI( "mem://m/a/f.txt" ), "f" );
	Uint64 before = mem->operationCount();
	auto result = waitFor( vfs.move( URI( "mem://m/a" ), URI( "mem://m/b" ) ).completion() );
	EXPECT_TRUE( result.hasValue() && result.value().renamed );
	EXPECT_EQ( before + 1, mem->operationCount() );
	EXPECT_STDSTREQ( "f", *mem->fileContent( URI( "mem://m/b/f.txt" ) ) );
	// A directory cannot be copied into itself.
	auto self = waitFor( vfs.copy( URI( "mem://m/b" ), URI( "mem://m/b/inner" ) ).completion() );
	EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( self ) );
}

UTEST( VFSTransfer, cancellation ) {
	VirtualFileSystem vfs;
	auto pool = ThreadPool::createShared( 1 );
	// Asynchronous provider so the transfer is still running when cancelled.
	auto mem = std::make_shared<MemoryProvider>( "mem", pool.get() );
	vfs.registerProvider( mem );
	mem->putFile( URI( "mem://c/big" ), std::string( 8 * 1024 * 1024, 'z' ) );
	CopyOptions options;
	options.chunkSize = 4096;
	Transfer transfer = vfs.copy( URI( "mem://c/big" ), URI( "mem://c/copy" ), options );
	auto completion = transfer.completion();
	transfer.cancel();
	auto result = waitFor( std::move( completion ) );
	EXPECT_TRUE( result.hasError() );
	EXPECT_EQ( VFSErrorCode::Cancelled, codeOf( result ) );
	// The partial destination is removed.
	EXPECT_FALSE( mem->fileContent( URI( "mem://c/copy" ) ).has_value() );
	vfs.unregisterProvider( "mem" );
}

// ---------------------------------------------------------------------------------------------
// res:// provider
// ---------------------------------------------------------------------------------------------

UTEST( VFSResource, matchesPackFileIndex ) {
	TempDir tmp;
	// A directory pack feeds the same index as zip packs (see zipPackWithSeveralFiles).
	FileSystem::makeDir( tmp.path + "pack/vfs-test/fonts", true );
	FileSystem::makeDir( tmp.path + "pack/vfs-test/themes", true );
	FileSystem::fileWrite( tmp.path + "pack/vfs-test/fonts/a.ttf",
						   std::string_view( "font-data" ) );
	FileSystem::fileWrite( tmp.path + "pack/vfs-test/themes/x.css", std::string_view( "css" ) );
	System::DirectoryPack pack;
	EXPECT_TRUE( pack.open( tmp.path + "pack" ) );
	auto* index = System::PackFileIndex::instance();
	EXPECT_TRUE( index->fileExists( "vfs-test/fonts/a.ttf" ) );
	EXPECT_TRUE( index->directoryExists( "vfs-test/fonts" ) );

	VirtualFileSystem vfs;
	auto info = waitFor( vfs.stat( URI( "res:///vfs-test/fonts/a.ttf" ) ) );
	EXPECT_TRUE( info && info.value().isRegularFile() );
	auto dirInfo = waitFor( vfs.stat( URI( "res:///vfs-test" ) ) );
	EXPECT_TRUE( dirInfo && dirInfo.value().isDirectory() );
	auto missing = waitFor( vfs.stat( URI( "res:///vfs-test/nope" ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( missing ) );

	auto listed = waitFor( vfs.listAll( URI( "res:///vfs-test" ) ) );
	auto names = sortedNames( listed.value() );
	EXPECT_EQ( 2u, names.size() );
	if ( names.size() != 2 )
		return;
	EXPECT_STDSTREQ( "fonts", names[0] );
	EXPECT_STDSTREQ( "themes", names[1] );
	// Same files as the legacy index API.
	auto legacy = index->filesGetInPath( "vfs-test/fonts" );
	auto fonts = waitFor( vfs.listAll( URI( "res:///vfs-test/fonts" ) ) );
	EXPECT_EQ( legacy.size(), fonts.value().size() );

	EXPECT_STDSTREQ( "font-data", readString( vfs, URI( "res:///vfs-test/fonts/a.ttf" ) ) );
	auto write = waitFor( vfs.writeAll( URI( "res:///vfs-test/new" ), std::string_view( "x" ) ) );
	EXPECT_TRUE( write.hasError() );
	auto mk = waitFor( vfs.makeDir( URI( "res:///vfs-test/dir" ) ) );
	EXPECT_EQ( VFSErrorCode::ReadOnly, codeOf( mk ) );

	pack.close();
	EXPECT_FALSE( index->directoryExists( "vfs-test" ) );
	auto gone = waitFor( vfs.stat( URI( "res:///vfs-test/fonts/a.ttf" ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( gone ) );
}

UTEST( VFSResource, zipPackWithSeveralFiles ) {
	TempDir tmp;
	std::string zipPath = tmp.path + "pack.zip";
	{
		// Every addFile() rewrites the archive; the later ones replace an existing file, which
		// used to fail on Windows and silently drop the file.
		System::Zip zip;
		ASSERT_TRUE( zip.create( zipPath ) );
		std::vector<Uint8> a{ 'a', '1' }, b{ 'b', '2' }, c{ 'c', '3' };
		EXPECT_TRUE( zip.addFile( a, "vfs-zip/a.txt" ) );
		EXPECT_TRUE( zip.addFile( b, "vfs-zip/sub/b.txt" ) );
		EXPECT_TRUE( zip.addFile( c, "vfs-zip/sub/c.txt" ) );
		EXPECT_TRUE( zip.close() );
	}

	System::Zip zip;
	ASSERT_TRUE( zip.open( zipPath ) );
	EXPECT_EQ( 3u, zip.getFileList().size() );
	std::vector<Uint8> data;
	EXPECT_TRUE( zip.extractFileToMemory( "vfs-zip/sub/c.txt", data ) );
	EXPECT_STDSTREQ( "c3", std::string( data.begin(), data.end() ) );

	VirtualFileSystem vfs;
	EXPECT_STDSTREQ( "a1", readString( vfs, URI( "res:///vfs-zip/a.txt" ) ) );
	EXPECT_STDSTREQ( "b2", readString( vfs, URI( "res:///vfs-zip/sub/b.txt" ) ) );
	auto listed = waitFor( vfs.listAll( URI( "res:///vfs-zip/sub" ) ) );
	ASSERT_TRUE( listed.hasValue() );
	EXPECT_EQ( 2u, listed.value().size() );
	zip.close();
}

// ---------------------------------------------------------------------------------------------
// Watching
// ---------------------------------------------------------------------------------------------

UTEST( VFSWatch, localEventsAreDeliveredThroughExecutor ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	auto opened = waitFor( vfs.watch( tmp.uri(), WatchOptions{ true } ) );
	EXPECT_TRUE( opened.hasValue() );
	if ( !opened )
		return;
	WatchHandle watch( std::move( opened ).value() );
	EXPECT_TRUE( watch.status() == WatchStatus::Active || watch.status() == WatchStatus::Polling );
	TestLoop loop;
	EventCollector collector;
	watch.onEvents( loop, collector.callback() );

	EXPECT_TRUE( FileSystem::fileWrite( tmp.path + "a.txt", std::string_view( "1" ) ) );
	EXPECT_TRUE( loop.runUntil( [&] { return collector.has( WatchEventType::Added, "a.txt" ); } ) );

	FileSystem::makeDir( tmp.path + "sub" );
	EXPECT_TRUE( loop.runUntil( [&] { return collector.has( WatchEventType::Added, "sub" ); } ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	EXPECT_TRUE( FileSystem::fileWrite( tmp.path + "sub/b.txt", std::string_view( "2" ) ) );
	EXPECT_TRUE(
		loop.runUntil( [&] { return collector.has( WatchEventType::Added, "sub/b.txt" ); } ) );

	EXPECT_TRUE( FileSystem::fileRemove( tmp.path + "a.txt" ) );
	EXPECT_TRUE(
		loop.runUntil( [&] { return collector.has( WatchEventType::Removed, "a.txt" ); } ) );

	// A burst is coalesced into few batches.
	int before = collector.batches.load();
	for ( int i = 0; i < 50; ++i ) {
		FileSystem::fileWrite( tmp.path + "burst" + String::toString( i ),
							   std::string_view( "x" ) );
	}
	EXPECT_TRUE(
		loop.runUntil( [&] { return collector.has( WatchEventType::Added, "burst49" ); } ) );
	EXPECT_LT( collector.batches.load() - before, 50 );

	// After reset no callback runs anymore.
	watch.reset();
	int after = collector.batches.load();
	FileSystem::fileWrite( tmp.path + "late.txt", std::string_view( "x" ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
	loop.drain();
	EXPECT_EQ( after, collector.batches.load() );
}

UTEST( VFSWatch, twoWatchesOnTheSameDirectoryShareNotifications ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	auto a = waitFor( vfs.watch( tmp.uri() ) );
	auto b = waitFor( vfs.watch( tmp.uri() ) );
	EXPECT_TRUE( a.hasValue() && b.hasValue() );
	if ( !a || !b )
		return;
	WatchHandle wa( std::move( a ).value() );
	WatchHandle wb( std::move( b ).value() );
	TestLoop loop;
	EventCollector ca, cb;
	wa.onEvents( loop, ca.callback() );
	wb.onEvents( loop, cb.callback() );
	FileSystem::fileWrite( tmp.path + "shared.txt", std::string_view( "x" ) );
	EXPECT_TRUE( loop.runUntil( [&] {
		return ca.has( WatchEventType::Added, "shared.txt" ) &&
			   cb.has( WatchEventType::Added, "shared.txt" );
	} ) );
	// Dropping one keeps the other alive.
	wa.reset();
	FileSystem::fileWrite( tmp.path + "second.txt", std::string_view( "x" ) );
	EXPECT_TRUE( loop.runUntil( [&] { return cb.has( WatchEventType::Added, "second.txt" ); } ) );
}

UTEST( VFSWatch, pollingBackendForProvidersWithoutNotifications ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	mem->putFile( URI( "mem://w/dir/existing.txt" ), "1" );
	WatchOptions options;
	options.pollingInterval = System::Milliseconds( 20 );
	options.recursive = true;
	auto opened = waitFor( vfs.watch( URI( "mem://w/dir" ), options ) );
	EXPECT_TRUE( opened.hasValue() );
	if ( !opened )
		return;
	WatchHandle watch( std::move( opened ).value() );
	EXPECT_EQ( WatchStatus::Polling, watch.status() );
	TestLoop loop;
	EventCollector collector;
	watch.onEvents( loop, collector.callback() );

	mem->putFile( URI( "mem://w/dir/new.txt" ), "2" );
	mem->putFile( URI( "mem://w/dir/sub/deep.txt" ), "3" );
	EXPECT_TRUE(
		loop.runUntil( [&] { return collector.has( WatchEventType::Added, "sub/deep.txt" ); } ) );
	EXPECT_TRUE( collector.has( WatchEventType::Added, "new.txt" ) );
	mem->putFile( URI( "mem://w/dir/existing.txt" ), "changed" );
	EXPECT_TRUE( loop.runUntil(
		[&] { return collector.has( WatchEventType::Modified, "existing.txt" ); } ) );
	waitFor( vfs.remove( URI( "mem://w/dir/new.txt" ) ) );
	EXPECT_TRUE(
		loop.runUntil( [&] { return collector.has( WatchEventType::Removed, "new.txt" ); } ) );

	// Losing the root reports a rescan.
	waitFor( vfs.remove( URI( "mem://w/dir" ), RemoveOptions{ true } ) );
	EXPECT_TRUE(
		loop.runUntil( [&] { return collector.has( WatchEventType::RescanRequired, "" ); } ) );
}

UTEST( VFSWatch, unsupportedWithoutPolling ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	mem->putDirectory( URI( "mem://u/d" ) );
	WatchOptions options;
	options.allowPolling = false;
	auto result = waitFor( vfs.watch( URI( "mem://u/d" ), options ) );
	EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( result ) );
}

// ---------------------------------------------------------------------------------------------
// Data safety: copies, moves and saves never lose existing data
// ---------------------------------------------------------------------------------------------

namespace {

/** Every read fails: a copy from it fails after the destination was opened. */
class FailingReadHandle : public FileHandleImpl {
  public:
	VFSTask<size_t> read( Uint64, std::span<Uint8>, CancellationToken ) override {
		return readyError<size_t>( VFSErrorCode::IO, URI(), "injected read failure" );
	}

	VFSTask<size_t> write( Uint64, std::span<const Uint8>, CancellationToken ) override {
		return readyError<size_t>( VFSErrorCode::IO, URI() );
	}

	VFSTask<void> truncate( Uint64 ) override { return readyValue<void>(); }

	VFSTask<void> flush() override { return readyValue<void>(); }

	VFSTask<FileInfo> stat() override { return readyError<FileInfo>( VFSErrorCode::IO, URI() ); }

	VFSTask<void> close() override { return readyValue<void>(); }

	ProviderCapabilities capabilities() const override { return {}; }
};

class FailingReadProvider : public MemoryProvider {
  public:
	FailingReadProvider() : MemoryProvider( "fault" ) {}

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions&, CancellationToken ) override {
		return readyValue<FileHandle>( uri, std::make_shared<FailingReadHandle>() );
	}
};

class NonAtomicMemoryProvider : public MemoryProvider {
  public:
	ProviderCapabilities capabilities( const URI& uri ) const override {
		auto caps = MemoryProvider::capabilities( uri );
		return caps.set( VFSCapability::AtomicReplace, false );
	}
};

/** Publishes another application's destination after the transfer's existence check. */
class RacingReadHandle : public FailingReadHandle {
  public:
	explicit RacingReadHandle( std::shared_ptr<MemoryProvider> destination ) :
		mDestination( std::move( destination ) ) {}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken ) override {
		if ( offset != 0 )
			return readyValue<size_t>( 0 );
		mDestination->putFile( URI( "mem://t/destination" ), "concurrently created data" );
		data[0] = 'x';
		return readyValue<size_t>( 1 );
	}

  private:
	std::shared_ptr<MemoryProvider> mDestination;
};

class RacingReadProvider : public MemoryProvider {
  public:
	explicit RacingReadProvider( std::shared_ptr<MemoryProvider> destination ) :
		MemoryProvider( "race" ), mDestination( std::move( destination ) ) {}

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions&, CancellationToken ) override {
		return readyValue<FileHandle>( uri, std::make_shared<RacingReadHandle>( mDestination ) );
	}

  private:
	std::shared_ptr<MemoryProvider> mDestination;
};

} // namespace

UTEST( VFSSafety, unsafeReplacementIsRefusedWithoutAtomicReplace ) {
	VirtualFileSystem vfs;
	auto destination = std::make_shared<NonAtomicMemoryProvider>();
	auto source = std::make_shared<FailingReadProvider>();
	vfs.registerProvider( destination );
	vfs.registerProvider( source );
	source->putFile( URI( "fault://t/source" ), "new content" );
	destination->putFile( URI( "mem://t/destination" ), "valuable data" );
	CopyOptions options;
	options.overwrite = OverwritePolicy::Overwrite;
	auto result = waitFor(
		vfs.copy( URI( "fault://t/source" ), URI( "mem://t/destination" ), options ).completion() );
	ASSERT_FALSE( result.hasValue() );
	EXPECT_EQ( VFSErrorCode::Unsupported, result.error().code );
	EXPECT_STDSTREQ( "valuable data", *destination->fileContent( URI( "mem://t/destination" ) ) );
	// A new file still needs only a no-replace rename, not atomic replacement support.
	auto memory = std::make_shared<MemoryProvider>( "source" );
	memory->putFile( URI( "source://t/file" ), "new content" );
	vfs.registerProvider( memory );
	EXPECT_TRUE(
		waitFor( vfs.copy( URI( "source://t/file" ), URI( "mem://t/new" ) ).completion() ) );
	EXPECT_STDSTREQ( "new content", *destination->fileContent( URI( "mem://t/new" ) ) );
}

UTEST( VFSSafety, concurrentDestinationRespectsFailAndSkip ) {
	for ( auto policy : { OverwritePolicy::Fail, OverwritePolicy::Skip } ) {
		VirtualFileSystem vfs;
		auto destination = std::make_shared<MemoryProvider>();
		auto source = std::make_shared<RacingReadProvider>( destination );
		source->putFile( URI( "race://t/source" ), "x" );
		vfs.registerProvider( destination );
		vfs.registerProvider( source );
		CopyOptions options;
		options.overwrite = policy;
		auto result =
			waitFor( vfs.move( URI( "race://t/source" ), URI( "mem://t/destination" ), options )
						 .completion() );
		if ( policy == OverwritePolicy::Fail ) {
			ASSERT_FALSE( result.hasValue() );
			EXPECT_EQ( VFSErrorCode::AlreadyExists, result.error().code );
		} else {
			ASSERT_TRUE( result.hasValue() );
			EXPECT_EQ( 1u, result.value().filesSkipped );
			EXPECT_EQ( 0u, result.value().sourcesRemoved );
		}
		EXPECT_STDSTREQ( "concurrently created data",
						 *destination->fileContent( URI( "mem://t/destination" ) ) );
		EXPECT_TRUE( source->fileContent( URI( "race://t/source" ) ).has_value() );
	}
}

UTEST( VFSSafety, failedOverwriteKeepsTheDestination ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	auto fault = std::make_shared<FailingReadProvider>();
	vfs.registerProvider( mem );
	vfs.registerProvider( fault );
	fault->putFile( URI( "fault://t/source" ), "new content" );
	mem->putFile( URI( "mem://t/file" ), "valuable data" );
	CopyOptions options;
	options.overwrite = OverwritePolicy::Overwrite;
	auto result = waitFor(
		vfs.copy( URI( "fault://t/source" ), URI( "mem://t/file" ), options ).completion() );
	EXPECT_FALSE( result.hasValue() );
	EXPECT_STDSTREQ( "valuable data", readString( vfs, URI( "mem://t/file" ) ) );
	// No temporary is left behind.
	auto listed = waitFor( vfs.listAll( URI( "mem://t/" ) ) );
	ASSERT_TRUE( listed.hasValue() );
	EXPECT_EQ( 1u, listed.value().size() );
}

UTEST( VFSSafety, writeAllCreateParentsWithOneOrchestrationThread ) {
	// writeAll() runs on the orchestration pool: creating the parents must not wait on another
	// job of that pool.
	VirtualFileSystem::Config config;
	config.registerDefaultProviders = false;
	config.orchestrationThreads = 1;
	VirtualFileSystem vfs( config );
	vfs.registerProvider( std::make_shared<MemoryProvider>() );
	WriteOptions options;
	options.createParents = true;
	auto task = vfs.writeAll( URI( "mem://t/new/dir/file" ), std::string_view( "data" ), options );
	ASSERT_EQ( TaskState::Ready, task.waitFor( std::chrono::seconds( 5 ) ) );
	EXPECT_TRUE( std::move( task ).takeValue().hasValue() );
	EXPECT_STDSTREQ( "data", readString( vfs, URI( "mem://t/new/dir/file" ) ) );
}

#if EE_PLATFORM != EE_PLATFORM_WIN

UTEST( VFSSafety, copyOntoAHardLinkKeepsTheSource ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	URI source = tmp.uri().withAppendedSegment( "file" );
	URI alias = tmp.uri().withAppendedSegment( "hardlink" );
	ASSERT_TRUE( waitFor( vfs.writeAll( source, std::string_view( "valuable data" ) ) ) );
	ASSERT_EQ( 0, ::link( source.getFSPath().c_str(), alias.getFSPath().c_str() ) );
	CopyOptions options;
	options.overwrite = OverwritePolicy::Overwrite;
	waitFor( vfs.copy( source, alias, options ).completion() );
	EXPECT_STDSTREQ( "valuable data", readString( vfs, source ) );
	EXPECT_STDSTREQ( "valuable data", readString( vfs, alias ) );
}

UTEST( VFSSafety, copyOntoASymlinkToTheSourceFails ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	URI source = tmp.uri().withAppendedSegment( "file" );
	URI alias = tmp.uri().withAppendedSegment( "link" );
	ASSERT_TRUE( waitFor( vfs.writeAll( source, std::string_view( "valuable data" ) ) ) );
	ASSERT_EQ( 0, ::symlink( source.getFSPath().c_str(), alias.getFSPath().c_str() ) );
	CopyOptions options;
	options.overwrite = OverwritePolicy::Overwrite;
	auto result = waitFor( vfs.copy( source, alias, options ).completion() );
	EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( result ) );
	EXPECT_STDSTREQ( "valuable data", readString( vfs, source ) );
}

UTEST( VFSSafety, movingADirectorySymlinkAcrossProvidersKeepsItsTarget ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	vfs.registerProvider( std::make_shared<MemoryProvider>() );
	URI real = tmp.uri().withAppendedSegment( "real" );
	URI link = tmp.uri().withAppendedSegment( "link" );
	ASSERT_TRUE( waitFor( vfs.makeDirs( real ) ) );
	ASSERT_TRUE(
		waitFor( vfs.writeAll( real.withAppendedSegment( "file" ), std::string_view( "data" ) ) ) );
	ASSERT_EQ( 0, ::symlink( real.getFSPath().c_str(), link.getFSPath().c_str() ) );
	auto result = waitFor( vfs.move( link, URI( "mem://t/moved" ) ).completion() );
	EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( result ) );
	EXPECT_STDSTREQ( "data", readString( vfs, real.withAppendedSegment( "file" ) ) );
	EXPECT_TRUE( waitFor( vfs.stat( link, StatOptions{ false } ) ).hasValue() );
}

UTEST( VFSSafety, atomicSaveKeepsTheExactMode ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	URI file = tmp.uri().withAppendedSegment( "file" );
	ASSERT_TRUE( waitFor( vfs.writeAll( file, std::string_view( "v1" ) ) ) );
	ASSERT_TRUE( waitFor( vfs.setPermissions( file, 0777 ) ) );
	mode_t mask = ::umask( 0077 );
	auto saved = waitFor( vfs.writeAll( file, std::string_view( "v2" ) ) );
	::umask( mask );
	ASSERT_TRUE( saved.hasValue() );
	EXPECT_EQ( 0777u, *saved.value().permissions() );
	EXPECT_STDSTREQ( "v2", readString( vfs, file ) );
}

#endif

UTEST( VFSWatch, localRegistrationRunsOnThePool ) {
	// Registering touches the filesystem (slow mounts block): it must not run on the caller's
	// (UI) thread. With the backend's pool busy, watch() returns a pending task.
	TempDir tmp;
	auto pool = ThreadPool::createShared( 1 );
	std::mutex gate;
	std::unique_lock<std::mutex> hold( gate );
	pool->run( [&gate] { std::lock_guard<std::mutex> wait( gate ); } );
	auto backend = std::make_shared<LocalWatchBackend>( pool );
	WatchManager manager;
	manager.registerBackend( "file", backend );
	auto task = manager.watch( tmp.uri() );
	EXPECT_EQ( TaskState::Pending, task.state() );
	hold.unlock();
	ASSERT_EQ( TaskState::Ready, task.waitFor( std::chrono::seconds( 5 ) ) );
	auto handle = std::move( task ).takeValue();
	ASSERT_TRUE( handle.hasValue() );
	EXPECT_EQ( WatchStatus::Active, handle.value().status() );
	handle.value().reset();
}

namespace {

/** open() blocks until released: a transfer reading from it keeps its thread busy. */
class GatedProvider : public MemoryProvider {
  public:
	GatedProvider() : MemoryProvider( "gated" ) {}

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
							  CancellationToken token ) override {
		{
			std::unique_lock<std::mutex> lock( mMutex );
			mOpened = true;
			mChanged.notify_all();
			mChanged.wait( lock, [this] { return mReleased; } );
		}
		return MemoryProvider::open( uri, options, std::move( token ) );
	}

	void waitOpened() {
		std::unique_lock<std::mutex> lock( mMutex );
		mChanged.wait_for( lock, std::chrono::seconds( 5 ), [this] { return mOpened; } );
	}

	void release() {
		std::lock_guard<std::mutex> lock( mMutex );
		mReleased = true;
		mChanged.notify_all();
	}

  private:
	std::mutex mMutex;
	std::condition_variable mChanged;
	bool mOpened{ false };
	bool mReleased{ false };
};

} // namespace

UTEST( VFSSafety, transfersDoNotDelayOtherOperations ) {
	// Copies run on their own pool: with every transfer thread busy, saves still complete.
	VirtualFileSystem::Config config;
	config.registerDefaultProviders = false;
	config.orchestrationThreads = 1;
	config.transferThreads = 1;
	VirtualFileSystem vfs( config );
	auto mem = std::make_shared<MemoryProvider>();
	auto gated = std::make_shared<GatedProvider>();
	vfs.registerProvider( mem );
	vfs.registerProvider( gated );
	gated->putFile( URI( "gated://t/big" ), std::string( 1024, 'x' ) );
	Transfer transfer = vfs.copy( URI( "gated://t/big" ), URI( "mem://t/copy" ) );
	gated->waitOpened();
	auto saved = vfs.writeAll( URI( "mem://t/other" ), std::string_view( "data" ) );
	EXPECT_EQ( TaskState::Ready, saved.waitFor( std::chrono::seconds( 5 ) ) );
	gated->release();
	auto done = transfer.completion();
	ASSERT_EQ( TaskState::Ready, done.waitFor( std::chrono::seconds( 5 ) ) );
	EXPECT_TRUE( std::move( done ).takeValue().hasValue() );
}
