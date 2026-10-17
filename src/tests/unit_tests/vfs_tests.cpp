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
#include <eepp/vfs/providers/localioscheduler.hpp>
#include <eepp/vfs/providers/localprovider.hpp>
#include <eepp/vfs/watchers/localwatchbackend.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>
#include <mutex>
#include <optional>
#include <thread>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <sys/stat.h> // umask
#include <unistd.h>	  // symlink, link
#endif

using namespace EE;
using namespace EE::VFS;
using namespace EE::System;
using namespace vfstest;

// ---------------------------------------------------------------------------------------------
// Value types
// ---------------------------------------------------------------------------------------------

UTEST( VFSFileInfo, unknownMetadataIsDistinguishableFromZero ) {
	VFSFileInfo info;
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
	VFSFileInfo info;
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
	EXPECT_LE( sizeof( VFSFileInfo ), 4 * sizeof( std::string ) + 40 + 56 );
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
	DirectoryPack pack;
	EXPECT_TRUE( pack.open( tmp.path + "pack" ) );
	auto* index = PackFileIndex::instance();
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
		Zip zip;
		ASSERT_TRUE( zip.create( zipPath ) );
		std::vector<Uint8> a{ 'a', '1' }, b{ 'b', '2' }, c{ 'c', '3' };
		EXPECT_TRUE( zip.addFile( a, "vfs-zip/a.txt" ) );
		EXPECT_TRUE( zip.addFile( b, "vfs-zip/sub/b.txt" ) );
		EXPECT_TRUE( zip.addFile( c, "vfs-zip/sub/c.txt" ) );
		std::vector<Uint8> empty;
		EXPECT_TRUE( zip.addFile( empty, "vfs-zip/empty.txt" ) );
		EXPECT_TRUE( zip.addFile( empty, "vfs-zip/empty-dir/" ) );
		EXPECT_TRUE( zip.close() );
	}

	Zip zip;
	ASSERT_TRUE( zip.open( zipPath ) );
	EXPECT_EQ( 4u, zip.getFileList().size() );
	std::vector<Uint8> data;
	EXPECT_TRUE( zip.extractFileToMemory( "vfs-zip/sub/c.txt", data ) );
	EXPECT_STDSTREQ( "c3", std::string( data.begin(), data.end() ) );

	VirtualFileSystem vfs;
	EXPECT_STDSTREQ( "a1", readString( vfs, URI( "res:///vfs-zip/a.txt" ) ) );
	EXPECT_STDSTREQ( "b2", readString( vfs, URI( "res:///vfs-zip/sub/b.txt" ) ) );
	EXPECT_STDSTREQ( "", readString( vfs, URI( "res:///vfs-zip/empty.txt" ) ) );
	EXPECT_FALSE( PackFileIndex::instance()->fileExists( "vfs-zip/empty-dir" ) );
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
	options.pollingInterval = Milliseconds( 20 );
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

	VFSTask<VFSFileInfo> stat() override {
		return readyError<VFSFileInfo>( VFSErrorCode::IO, URI() );
	}

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

// ---------------------------------------------------------------------------------------------
// Network mounts: an unreachable server never blocks local folders, and its calls time out
// ---------------------------------------------------------------------------------------------

namespace {

/** Holds the calls that wait on it, like a server that stopped answering, until opened. */
class Gate {
  public:
	void wait() {
		std::unique_lock<std::mutex> lock( mMutex );
		mOpened.wait( lock, [this] { return mOpen; } );
	}

	/** @return Whether it opened within timeout. */
	bool waitFor( std::chrono::milliseconds timeout ) {
		std::unique_lock<std::mutex> lock( mMutex );
		return mOpened.wait_for( lock, timeout, [this] { return mOpen; } );
	}

	void open() {
		std::lock_guard<std::mutex> lock( mMutex );
		mOpen = true;
		mOpened.notify_all();
	}

  private:
	std::mutex mMutex;
	std::condition_variable mOpened;
	bool mOpen{ false };
};

/** Paths under /net are on a network mount; everything else is local. */
std::optional<LocalIOScheduler::NetworkMount> fakeNetworkMount( const std::string& path ) {
	if ( path.rfind( "/net", 0 ) == 0 )
		return LocalIOScheduler::NetworkMount{ "/net", "server:/export" };
	return std::nullopt;
}

std::atomic<int> sResolvedTwice{ 0 };

} // namespace

UTEST( LocalIOScheduler, hungMountTimesOutWithoutBlockingLocalCalls ) {
	sResolvedTwice = 0;
	auto previous = TaskDiagnostics::setHandler( []( TaskDiagnostic diagnostic ) {
		if ( diagnostic == TaskDiagnostic::ResolvedTwice )
			++sResolvedTwice;
	} );
	const auto timeout = Milliseconds( 200 );
	auto pool = ThreadPool::createShared( 2 );
	auto scheduler = std::make_shared<LocalIOScheduler>( pool, timeout, &fakeNetworkMount );
	auto gate = std::make_shared<Gate>();
	auto hung = [gate]() -> VFSResult<int> {
		gate->wait();
		return Ok( 1 );
	};
	ASSERT_TRUE( scheduler->routeFor( "/net/folder" ).isNetworkMount() );
	ASSERT_FALSE( scheduler->routeFor( "/home/user" ).isNetworkMount() );

	// Both lane threads hang, and a third call waits behind them.
	const auto started = std::chrono::steady_clock::now();
	auto first = scheduler->submit<int>( "/net/a", hung );
	auto second = scheduler->submit<int>( "/net/b", hung );
	auto queued = scheduler->submit<int>( "/net/c", []() -> VFSResult<int> { return Ok( 3 ); } );

	// Meanwhile local calls run on the pool as usual.
	auto local = waitFor(
		scheduler->submit<int>( "/home/user", []() -> VFSResult<int> { return Ok( 7 ); } ) );
	ASSERT_TRUE( local.hasValue() );
	EXPECT_EQ( 7, local.value() );

	// The hung calls and the one queued behind them fail once the timeout passes.
	auto firstResult = waitFor( std::move( first ) );
	auto queuedResult = waitFor( std::move( queued ) );
	auto secondResult = waitFor( std::move( second ) );
	const auto waited = std::chrono::steady_clock::now() - started;
	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( firstResult ) );
	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( secondResult ) );
	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( queuedResult ) );
	EXPECT_TRUE( !firstResult &&
				 firstResult.error().message == "server:/export is not responding" );
	EXPECT_TRUE( waited >= std::chrono::milliseconds( 200 ) );
	EXPECT_TRUE( waited < std::chrono::seconds( 5 ) );

	// While the mount does not answer, new calls fail at once instead of waiting.
	const auto rejectedAt = std::chrono::steady_clock::now();
	auto rejected =
		waitFor( scheduler->submit<int>( "/net/d", []() -> VFSResult<int> { return Ok( 4 ); } ) );
	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( rejected ) );
	EXPECT_TRUE( std::chrono::steady_clock::now() - rejectedAt < std::chrono::milliseconds( 100 ) );

	// The server answers again: the late results are discarded, and the mount works again.
	gate->open();
	VFSResult<int> recovered = Err( VFSError( VFSErrorCode::Unknown ) );
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
	while ( !recovered && std::chrono::steady_clock::now() < deadline ) {
		recovered = waitFor(
			scheduler->submit<int>( "/net/e", []() -> VFSResult<int> { return Ok( 5 ); } ) );
		if ( !recovered )
			std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	}
	ASSERT_TRUE( recovered.hasValue() );
	EXPECT_EQ( 5, recovered.value() );
	scheduler.reset();
	EXPECT_EQ( 0, sResolvedTwice.load() );
	TaskDiagnostics::setHandler( previous );
}

UTEST( LocalIOScheduler, shutdownLeavesHungCallsBehind ) {
	auto pool = ThreadPool::createShared( 1 );
	auto scheduler = std::make_shared<LocalIOScheduler>( pool, Seconds( 60 ), &fakeNetworkMount );
	auto gate = std::make_shared<Gate>();
	auto hung = [gate]() -> VFSResult<int> {
		gate->wait();
		return Ok( 1 );
	};
	// Both lane threads hang; a third call waits behind them.
	auto first = scheduler->submit<int>( "/net/a", hung );
	auto second = scheduler->submit<int>( "/net/b", hung );
	auto queued = scheduler->submit<int>( "/net/c", []() -> VFSResult<int> { return Ok( 3 ); } );
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	// Destroying the scheduler does not wait for the hung calls, and fails the queued one.
	const auto destroyedAt = std::chrono::steady_clock::now();
	scheduler.reset();
	EXPECT_TRUE( std::chrono::steady_clock::now() - destroyedAt < std::chrono::seconds( 1 ) );
	EXPECT_EQ( VFSErrorCode::Cancelled, codeOf( waitFor( std::move( queued ) ) ) );
	// The detached threads still complete their calls when the server answers.
	gate->open();
	EXPECT_TRUE( waitFor( std::move( first ) ).hasValue() );
	EXPECT_TRUE( waitFor( std::move( second ) ).hasValue() );
}

UTEST( LocalIOScheduler, timedOutCallNeverWritesIntoTheCallersMemory ) {
	auto pool = ThreadPool::createShared( 1 );
	auto scheduler =
		std::make_shared<LocalIOScheduler>( pool, Milliseconds( 100 ), &fakeNetworkMount );
	auto gate = std::make_shared<Gate>();
	auto finished = std::make_shared<Gate>();
	auto committed = std::make_shared<std::atomic<bool>>( false );
	// The work runs on, past the timeout; its result must not reach the caller's buffer, which
	// the caller may have released once told TimedOut.
	auto task = scheduler->submitCommitted<int>(
		scheduler->routeFor( "/net/file" ),
		[gate, finished]() -> VFSResult<int> {
			gate->wait();
			finished->open();
			return Ok( 1 );
		},
		[committed]( VFSResult<int>&& result ) {
			*committed = true;
			return std::move( result );
		} );
	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( waitFor( std::move( task ) ) ) );
	gate->open();
	finished->wait();
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	EXPECT_FALSE( committed->load() );
	// In time, it commits.
	VFSResult<int> late = Err( VFSError( VFSErrorCode::Unknown ) );
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
	while ( !late && std::chrono::steady_clock::now() < deadline ) {
		late = waitFor( scheduler->submitCommitted<int>(
			scheduler->routeFor( "/net/file" ), []() -> VFSResult<int> { return Ok( 2 ); },
			[committed]( VFSResult<int>&& result ) {
				*committed = true;
				return std::move( result );
			} ) );
	}
	ASSERT_TRUE( late.hasValue() );
	EXPECT_TRUE( committed->load() );
}

UTEST( LocalIOScheduler, slowTimeoutCompletionNeverDelaysOtherMounts ) {
	// Two mounts, both hanging.
	auto lookup = []( const std::string& path ) -> std::optional<LocalIOScheduler::NetworkMount> {
		for ( const char* root : { "/neta", "/netb" } ) {
			if ( path.rfind( root, 0 ) == 0 )
				return LocalIOScheduler::NetworkMount{ root, root };
		}
		return std::nullopt;
	};
	const auto timeout = std::chrono::milliseconds( 200 );
	// One worker: the timeouts must not wait for it.
	auto pool = ThreadPool::createShared( 1 );
	auto scheduler = std::make_shared<LocalIOScheduler>( pool, Milliseconds( 200 ), lookup );
	auto gate = std::make_shared<Gate>();
	auto hung = [gate]() -> VFSResult<int> {
		gate->wait();
		return Ok( 1 );
	};
	// The first mount's timeout runs a completion that takes long (a close that waits, say).
	auto slowStarted = std::make_shared<Gate>();
	scheduler->submit<int>( "/neta/x", hung )
		.onComplete( [slowStarted]( std::optional<VFSResult<int>> ) {
			slowStarted->open();
			std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
		} );
	ASSERT_TRUE( slowStarted->waitFor( std::chrono::seconds( 5 ) ) );
	// Meanwhile the other mount's calls still time out on time.
	const auto started = std::chrono::steady_clock::now();
	auto other = scheduler->submit<int>( "/netb/x", hung );
	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( waitFor( std::move( other ) ) ) );
	EXPECT_TRUE( std::chrono::steady_clock::now() - started < timeout * 4 );
	gate->open();
}

UTEST( LocalIOScheduler, lastOwnerReleasedByALaneCallTearsDown ) {
	auto pool = ThreadPool::createShared( 1 );
	auto scheduler =
		std::make_shared<LocalIOScheduler>( pool, Milliseconds( 100 ), &fakeNetworkMount );
	auto gate = std::make_shared<Gate>();
	auto destroyed = std::make_shared<Gate>();
	// Holds the scheduler, like a file handle's call; signals once the scheduler is gone.
	struct Owner {
		std::shared_ptr<LocalIOScheduler> scheduler;
		std::shared_ptr<Gate> destroyed;
		~Owner() {
			scheduler.reset();
			if ( destroyed )
				destroyed->open();
		}
	};
	auto owner = std::make_shared<Owner>();
	owner->scheduler = scheduler;
	owner->destroyed = destroyed;
	auto task = scheduler->submit<int>( "/net/a", [gate, owner]() -> VFSResult<int> {
		gate->wait();
		return Ok( 1 );
	} );
	owner.reset();
	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( waitFor( std::move( task ) ) ) );
	// The call recovers after the timeout, holding the last owner: the lane's thread tears the
	// scheduler down.
	scheduler.reset();
	gate->open();
	EXPECT_TRUE( destroyed->waitFor( std::chrono::seconds( 5 ) ) );
}

UTEST( LocalIOScheduler, neverOwnsThePool ) {
	auto pool = ThreadPool::createShared( 1 );
	auto scheduler = std::make_shared<LocalIOScheduler>( pool, Seconds( 5 ), &fakeNetworkMount );
	// A call that holds the last scheduler reference ends on a pool worker: the pool, owned
	// elsewhere, is not destroyed there.
	auto task = scheduler->submit<int>(
		"/home/user", [owner = scheduler]() -> VFSResult<int> { return Ok( 1 ); } );
	scheduler.reset();
	EXPECT_TRUE( waitFor( std::move( task ) ).hasValue() );
	std::weak_ptr<ThreadPool> weak = pool;
	auto other = std::make_shared<LocalIOScheduler>( weak, Seconds( 5 ), &fakeNetworkMount );
	pool.reset();
	EXPECT_TRUE( weak.expired() );
	// Once its owner releases the pool, every call fails instead of running: the file system is
	// shutting down, on network mounts too.
	auto ran = std::make_shared<std::atomic<bool>>( false );
	auto work = [ran]() -> VFSResult<int> {
		*ran = true;
		return Ok( 1 );
	};
	EXPECT_EQ( VFSErrorCode::Cancelled,
			   codeOf( waitFor( other->submit<int>( "/home/user", work ) ) ) );
	EXPECT_EQ( VFSErrorCode::Cancelled,
			   codeOf( waitFor( other->submit<int>( "/net/file", work ) ) ) );
	EXPECT_EQ( VFSErrorCode::Cancelled,
			   codeOf( waitFor( other->submitResolved<int>( "/net/file", true, work ) ) ) );
	EXPECT_EQ( VFSErrorCode::Cancelled,
			   codeOf( waitFor( other->submitCommitted<int>(
				   other->routeFor( "/net/file" ), work,
				   []( VFSResult<int>&& result ) { return std::move( result ); } ) ) ) );
	EXPECT_FALSE( ran->load() );
}

UTEST( LocalIOScheduler, localTemporaryFolderIsNotANetworkMount ) {
	TempDir tmp;
	EXPECT_FALSE( LocalIOScheduler::findNetworkMount( tmp.path ).has_value() );
}

UTEST( LocalIOScheduler, renameBetweenRoutesNeverWaitsOnTheOtherMount ) {
	TempDir tmp;
	const std::string mount = tmp.path + "net";
	ASSERT_TRUE( FileSystem::makeDir( mount ) );
	ASSERT_TRUE( FileSystem::fileWrite( tmp.path + "local.txt", "local" ) );
	ASSERT_TRUE( FileSystem::fileWrite( mount + "/a.txt", "a" ) );
	// The "net" folder stands in for a network mount: its calls run on a lane.
	auto pool = ThreadPool::createShared( 1 );
	auto scheduler = std::make_shared<LocalIOScheduler>(
		pool, Seconds( 5 ),
		[mount]( const std::string& path ) -> std::optional<LocalIOScheduler::NetworkMount> {
			if ( path.rfind( mount, 0 ) == 0 )
				return LocalIOScheduler::NetworkMount{ mount, "server:/export" };
			return std::nullopt;
		} );
	LocalProvider provider( scheduler );
	// Into the mount: answered at once as crossing file systems (the VFS then copies), without a
	// call that would look the mount up.
	auto across = waitFor( provider.rename( URI::fromFSPath( tmp.path + "local.txt" ),
											URI::fromFSPath( mount + "/local.txt" ), {}, {} ) );
	EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( across ) );
	EXPECT_TRUE( FileSystem::fileExists( tmp.path + "local.txt" ) );
	// Within the mount it renames, on the mount's lane.
	auto within = waitFor( provider.rename( URI::fromFSPath( mount + "/a.txt" ),
											URI::fromFSPath( mount + "/b.txt" ), {}, {} ) );
	EXPECT_TRUE( within.hasValue() );
	EXPECT_TRUE( FileSystem::fileExists( mount + "/b.txt" ) );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( LocalIOScheduler, linksIntoAMountRunOnItsLane ) {
	TempDir tmp;
	// Physical paths: the stand-in mount is matched by prefix, after the links are followed.
	char* real = ::realpath( tmp.path.c_str(), nullptr );
	ASSERT_TRUE( real != nullptr );
	const std::string base = std::string( real ) + "/";
	::free( real );
	const std::string mount = base + "net";
	ASSERT_TRUE( FileSystem::makeDir( mount ) );
	ASSERT_TRUE( FileSystem::makeDir( base + "dir" ) );
	ASSERT_TRUE( FileSystem::fileWrite( base + "local.txt", "local" ) );
	ASSERT_EQ( 0, ::symlink( mount.c_str(), ( base + "link" ).c_str() ) );
	ASSERT_EQ( 0, ::symlink( "link", ( base + "link2" ).c_str() ) );
	auto lookup =
		[mount]( const std::string& path ) -> std::optional<LocalIOScheduler::NetworkMount> {
		if ( path.rfind( mount, 0 ) == 0 )
			return LocalIOScheduler::NetworkMount{ mount, "server:/export" };
		return std::nullopt;
	};
	EXPECT_TRUE( ( mount + "/a.txt" ) ==
				 LocalIOScheduler::resolvePath( base + "link/a.txt", true, lookup ) );
	EXPECT_TRUE( mount == LocalIOScheduler::resolvePath( base + "dir/../link2", true, lookup ) );
	EXPECT_TRUE( ( base + "link" ) ==
				 LocalIOScheduler::resolvePath( base + "link", false, lookup ) );

	// The mount's server stops answering: both lane threads hang, and the mount counts as not
	// responding.
	auto pool = ThreadPool::createShared( 1 );
	auto scheduler = std::make_shared<LocalIOScheduler>( pool, Milliseconds( 100 ), lookup );
	auto gate = std::make_shared<Gate>();
	auto hung = [gate]() -> VFSResult<int> {
		gate->wait();
		return Ok( 1 );
	};
	auto first = scheduler->submit<int>( mount + "/a", hung );
	auto second = scheduler->submit<int>( mount + "/b", hung );
	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( waitFor( std::move( first ) ) ) );
	LocalProvider provider( scheduler );

	// Calls through the links run on the mount's lane: they fail at once, as the mount's own do,
	// instead of hanging on the shared pool.
	StatOptions follow;
	follow.followSymlinks = true;
	EXPECT_EQ( VFSErrorCode::TimedOut,
			   codeOf( waitFor( provider.stat( URI::fromFSPath( base + "link" ), follow, {} ) ) ) );
	EXPECT_EQ(
		VFSErrorCode::TimedOut,
		codeOf( waitFor( provider.stat( URI::fromFSPath( base + "link2" ), follow, {} ) ) ) );
	EXPECT_EQ(
		VFSErrorCode::TimedOut,
		codeOf( waitFor( provider.openDirectory( URI::fromFSPath( base + "link" ), {}, {} ) ) ) );
	EXPECT_EQ( VFSErrorCode::TimedOut,
			   codeOf( waitFor(
				   provider.stat( URI::fromFSPath( base + "link/a.txt" ), StatOptions{}, {} ) ) ) );
	// A move through a link into the mount crosses file systems, answered without the mount.
	EXPECT_EQ( VFSErrorCode::Unsupported,
			   codeOf( waitFor( provider.rename( URI::fromFSPath( base + "local.txt" ),
												 URI::fromFSPath( base + "link/local.txt" ), {},
												 {} ) ) ) );
	// The links themselves, and the local files, are local.
	StatOptions noFollow;
	noFollow.followSymlinks = false;
	auto linkInfo = waitFor( provider.stat( URI::fromFSPath( base + "link" ), noFollow, {} ) );
	ASSERT_TRUE( linkInfo.hasValue() );
	EXPECT_TRUE( linkInfo.value().type == FileType::Symlink );
	EXPECT_TRUE(
		waitFor( provider.stat( URI::fromFSPath( base + "local.txt" ), follow, {} ) ).hasValue() );

	EXPECT_EQ( VFSErrorCode::TimedOut, codeOf( waitFor( std::move( second ) ) ) );
	gate->open();
}
#endif

namespace {

class ReleasePollingBackendSink : public WatchSink {
  public:
	URI uri;
	std::mutex gate;
	std::shared_ptr<PollingWatchBackend> backend;

	void push( WatchEvent&& ) override {}

	void push( std::vector<WatchEvent>&& ) override {}

	void rescanRequired() override {}

	void setStatus( WatchStatus ) override {
		std::lock_guard<std::mutex> lock( gate );
		backend.reset();
	}

	const URI& root() const override { return uri; }
};

} // namespace

UTEST( VFSWatch, pollingCallbackCanReleaseTheLastBackendOwner ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem://polling-teardown/dir" );
	memory->putFile( root.withAppendedSegment( "file" ), "contents" );
	auto sink = std::make_shared<ReleasePollingBackendSink>();
	sink->uri = root;
	sink->backend = std::make_shared<PollingWatchBackend>( vfs );
	std::weak_ptr<PollingWatchBackend> weak = sink->backend;
	std::unique_lock<std::mutex> hold( sink->gate );
	auto task = sink->backend->watch( root, {}, sink );
	hold.unlock();
	ASSERT_EQ( TaskState::Ready, task.waitFor( std::chrono::seconds( 5 ) ) );
	ASSERT_TRUE( waitFor( std::move( task ) ).hasValue() );
	EXPECT_TRUE( weak.expired() );
}
