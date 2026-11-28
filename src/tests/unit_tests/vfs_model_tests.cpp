#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <atomic>
#include <chrono>
#include <eepp/scene/mainthreadexecutor.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/vfs/detail/memoryhandles.hpp>
#include <eepp/vfs/ui/vfsdirectorymodel.hpp>
#include <eepp/window/input.hpp>
#include <future>

using namespace EE::System;

using namespace vfstest;
using EE::UI::UIApplication;
using EE::Window::WindowBackend;
using EE::Window::WindowSettings;
using EE::Window::WindowStyle;

namespace {

std::vector<std::string> modelNames( const VFSDirectoryModel& model ) {
	std::vector<std::string> names;
	for ( size_t row = 0; row < model.rowCount(); ++row )
		names.push_back( model.entry( row ).name );
	return names;
}

class ControlledWatchBackend : public WatchBackend {
  public:
	std::shared_ptr<WatchSink> sink;

	VFSTask<std::unique_ptr<WatchRegistration>>
	watch( const URI&, const WatchOptions&, std::shared_ptr<WatchSink> target ) override {
		sink = std::move( target );
		return readyValue<std::unique_ptr<WatchRegistration>>(
			std::make_unique<WatchRegistration>() );
	}

	void send( WatchEventType type ) { sink->push( WatchEvent{ "file", {}, type } ); }
};

class DelayedStatProvider : public MemoryProvider {
  public:
	std::vector<TaskResolver<VFSResult<VFSFileInfo>>> pending;

	VFSTask<VFSFileInfo> stat( const URI&, const StatOptions&, CancellationToken ) override {
		TaskResolver<VFSResult<VFSFileInfo>> resolve;
		auto task = VFSTask<VFSFileInfo>::create( resolve );
		pending.push_back( std::move( resolve ) );
		return task;
	}
};

class MetadataListingProvider : public MemoryProvider {
  public:
	std::vector<VFSFileInfo> entries;

	VFSTask<DirectoryHandle> openDirectory( const URI& uri, const DirectoryOpenOptions&,
											CancellationToken ) override {
		return readyValue<DirectoryHandle>(
			uri, std::make_shared<EE::VFS::detail::VectorDirectoryImpl>( entries ) );
	}
};

} // namespace

UTEST( VFSDirectoryModel, optionalColumnsDisplayAndSortWithoutFetchingMetadata ) {
	VirtualFileSystem vfs;
	auto provider = std::make_shared<MetadataListingProvider>();
	for ( const char* name :
		  { "folder.with.dot", ".profile", "a.TXT", "archive.tar.gz", "link", "z.txt" } ) {
		auto& entry = provider->entries.emplace_back();
		entry.name = name;
		entry.type = FileType::Regular;
	}
	provider->entries[0].type = FileType::Directory;
	provider->entries[2].setCreatedTime( 200 ).setAccessedTime( 10 );
	provider->entries[3].setCreatedTime( 100 ).setAccessedTime( 30 );
	provider->entries[4].type = FileType::Symlink;
	provider->entries[4].symlinkTarget = "../target";
	provider->entries[5].setCreatedTime( 0 ).setAccessedTime( 20 );
	vfs.registerProvider( provider );
	TestLoop loop;
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	config.watch = false;
	auto model = VFSDirectoryModel::New( loop, config );
	model->setTimeFormatter( []( Int64 time ) { return "epoch:" + String::toString( time ); } );
	model->setURI( URI( "mem://m/dir%20name/" ) );
	ASSERT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	const auto operations = provider->operationCount();
	auto display = [&]( const char* name, VFSDirectoryModel::Column column ) {
		return model->data( model->index( *model->rowOf( name ), column ) ).toString();
	};
	EXPECT_STDSTREQ( "epoch:200", display( "a.TXT", VFSDirectoryModel::Created ) );
	EXPECT_STDSTREQ( "epoch:0", display( "z.txt", VFSDirectoryModel::Created ) );
	EXPECT_STDSTREQ( "", display( ".profile", VFSDirectoryModel::Created ) );
	EXPECT_STDSTREQ( "epoch:10", display( "a.TXT", VFSDirectoryModel::Accessed ) );
	EXPECT_STDSTREQ( "", display( "link", VFSDirectoryModel::Accessed ) );
	EXPECT_STDSTREQ( "TXT", display( "a.TXT", VFSDirectoryModel::Extension ) );
	EXPECT_STDSTREQ( "gz", display( "archive.tar.gz", VFSDirectoryModel::Extension ) );
	EXPECT_STDSTREQ( "", display( ".profile", VFSDirectoryModel::Extension ) );
	EXPECT_STDSTREQ( "", display( "folder.with.dot", VFSDirectoryModel::Extension ) );
	EXPECT_STDSTREQ( "../target", display( "link", VFSDirectoryModel::LinkDestination ) );
	EXPECT_STDSTREQ( "", display( "a.TXT", VFSDirectoryModel::LinkDestination ) );
	EXPECT_STDSTREQ( "mem://m/dir%20name/", display( "a.TXT", VFSDirectoryModel::Path ) );
	EXPECT_STDSTREQ( "dir name", display( "a.TXT", VFSDirectoryModel::FolderName ) );
	model->sort( VFSDirectoryModel::Created, SortOrder::Ascending );
	EXPECT_LT( *model->rowOf( "archive.tar.gz" ), *model->rowOf( "a.TXT" ) );
	model->sort( VFSDirectoryModel::Created, SortOrder::Descending );
	EXPECT_LT( *model->rowOf( "a.TXT" ), *model->rowOf( "archive.tar.gz" ) );
	model->sort( VFSDirectoryModel::Accessed, SortOrder::Ascending );
	EXPECT_LT( *model->rowOf( "a.TXT" ), *model->rowOf( "z.txt" ) );
	EXPECT_LT( *model->rowOf( "z.txt" ), *model->rowOf( "archive.tar.gz" ) );
	model->sort( VFSDirectoryModel::Extension, SortOrder::Ascending );
	EXPECT_LT( *model->rowOf( "archive.tar.gz" ), *model->rowOf( "a.TXT" ) );
	EXPECT_LT( *model->rowOf( "a.TXT" ), *model->rowOf( "z.txt" ) );
	model->sort( VFSDirectoryModel::LinkDestination, SortOrder::Descending );
	EXPECT_EQ( 1u, *model->rowOf( "link" ) );
	EXPECT_STDSTREQ( "folder.with.dot", model->entry( 0 ).name );
	EXPECT_EQ( operations, provider->operationCount() );
	model->setURI( URI( "mem://m/" ) );
	ASSERT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	EXPECT_STDSTREQ( "mem://m/", display( "a.TXT", VFSDirectoryModel::Path ) );
	EXPECT_STDSTREQ( "", display( "a.TXT", VFSDirectoryModel::FolderName ) );
}

UTEST( VFSDirectoryModel, naturalNamesAcrossBatchesSortDirectionsAndColumnTies ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	for ( const char* name : { "file10", "file2", "file1", "file02", "File2" } )
		mem->putFile( URI( std::string( "mem://m/dir/" ) + name ), "" );
	mem->putDirectory( URI( "mem://m/dir/folder10" ) );
	mem->putDirectory( URI( "mem://m/dir/folder2" ) );
	TestLoop loop;
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	config.watch = false;
	config.naturalSort = true;
	config.firstBatchSize = 1;
	config.batchSize = 2;
	auto model = VFSDirectoryModel::New( loop, config );
	model->setURI( URI( "mem://m/dir" ) );
	ASSERT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	const std::vector<std::string> ascending{ "folder2", "folder10", "file1", "File2",
											  "file02",	 "file2",	 "file10" };
	const std::vector<std::string> descending{ "folder10", "folder2", "file10", "file2",
											   "file02",   "File2",	  "file1" };
	EXPECT_TRUE( modelNames( *model ) == ascending );
	model->sort( VFSDirectoryModel::Name, SortOrder::Descending );
	EXPECT_TRUE( modelNames( *model ) == descending );
	model->sort( VFSDirectoryModel::Size, SortOrder::Ascending );
	EXPECT_TRUE( modelNames( *model ) == ascending );
	model->sort( VFSDirectoryModel::Size, SortOrder::Descending );
	EXPECT_TRUE( modelNames( *model ) == descending );
	model->setNameFilter( "file", VFSDirectoryModel::NameFilterMode::PlainText );
	EXPECT_TRUE( modelNames( *model ) ==
				 std::vector<std::string>( descending.begin() + 2, descending.end() ) );
	config.naturalSort = false;
	auto lexical = VFSDirectoryModel::New( loop, config );
	lexical->setURI( URI( "mem://m/dir" ) );
	ASSERT_TRUE( loop.runUntil(
		[&] { return lexical->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	EXPECT_TRUE( modelNames( *lexical ) ==
				 std::vector<std::string>(
					 { "folder10", "folder2", "file02", "file1", "file10", "File2", "file2" } ) );
}

UTEST( VFSDirectoryModel, listsIncrementallySortsAndFiltersHidden ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	for ( int i = 0; i < 300; ++i )
		mem->putFile( URI( "mem://m/dir/f" + String::toString( 1000 + i ) ), "" );
	mem->putDirectory( URI( "mem://m/dir/zdir" ) );
	mem->putFile( URI( "mem://m/dir/.hidden" ), "" );
	TestLoop loop;
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	config.firstBatchSize = 10;
	config.batchSize = 64;
	auto model = VFSDirectoryModel::New( loop, config );
	int stateChanges = 0;
	model->setOnStateChanged( [&] { ++stateChanges; } );
	model->setURI( URI( "mem://m/dir" ) );
	EXPECT_EQ( VFSDirectoryModel::LoadState::Loading, model->loadState() );
	EXPECT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	EXPECT_EQ( 301u, model->rowCount() );
	// Directories first, then names ascending; hidden entries filtered.
	EXPECT_STDSTREQ( "zdir", model->entry( 0 ).name );
	EXPECT_STDSTREQ( "f1000", model->entry( 1 ).name );
	EXPECT_FALSE( model->rowOf( ".hidden" ).has_value() );
	model->setShowHidden( true );
	EXPECT_EQ( 302u, model->rowCount() );
	model->sort( VFSDirectoryModel::Name, SortOrder::Descending );
	EXPECT_STDSTREQ( "zdir", model->entry( 0 ).name );
	EXPECT_STDSTREQ( "f1299", model->entry( 1 ).name );
	EXPECT_GE( stateChanges, 2 );
	EXPECT_STDSTREQ( "drwxr-xr-x", VFSDirectoryModel::permissionsToString( model->entry( 0 ) ) );
	auto display = model->data( model->index( 1, VFSDirectoryModel::Name ) );
	EXPECT_STDSTREQ( "f1299", display.toString() );
}

UTEST( VFSDirectoryModel, nameFilterModesTypesAndItemCounts ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	for ( const char* name : { "Notes.md", "notes.txt", "photo.png", "readme.MD" } )
		mem->putFile( URI( std::string( "mem://m/dir/" ) + name ), "" );
	mem->putDirectory( URI( "mem://m/dir/docs" ) );
	mem->putFile( URI( "mem://m/dir/docs/a.md" ), "" );
	mem->putFile( URI( "mem://m/dir/docs/b.md" ), "" );
	mem->putFile( URI( "mem://m/dir/docs/.hidden" ), "" );
	mem->putDirectory( URI( "mem://m/dir/empty" ) );
	TestLoop loop;
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	config.countDirectoryItems = true;
	auto model = VFSDirectoryModel::New( loop, config );
	model->setTypeProvider( []( const EE::VFS::VFSFileInfo& info ) {
		return info.isDirectory() ? std::string( "Folder" )
								  : String::toUpper( FileSystem::fileExtension( info.name ) );
	} );
	model->setURI( URI( "mem://m/dir" ) );
	EXPECT_TRUE( loop.runUntil( [&] {
		auto docs = model->rowOf( "docs" );
		auto empty = model->rowOf( "empty" );
		return model->loadState() == VFSDirectoryModel::LoadState::Ready && docs && empty &&
			   model->directoryItemCount( *docs ) && model->directoryItemCount( *empty );
	} ) );
	// Hidden entries are counted only when they are shown.
	EXPECT_EQ( 2u, *model->directoryItemCount( *model->rowOf( "docs" ) ) );
	EXPECT_EQ( 0u, *model->directoryItemCount( *model->rowOf( "empty" ) ) );
	EXPECT_STDSTREQ( "2 items",
					 model->data( model->index( *model->rowOf( "docs" ), VFSDirectoryModel::Size ) )
						 .toString() );
	model->setShowHidden( true );
	EXPECT_EQ( 3u, *model->directoryItemCount( *model->rowOf( "docs" ) ) );
	model->setShowHidden( false );
	EXPECT_STDSTREQ(
		"PNG", model->data( model->index( *model->rowOf( "photo.png" ), VFSDirectoryModel::Type ) )
				   .toString() );

	// Glob (default): unanchored, case-insensitive unless asked.
	model->setNameFilter( "md" );
	EXPECT_EQ( 2u, model->rowCount() );
	EXPECT_EQ( 4u, model->filteredOutCount() );
	model->setNameFilter( "*.md", VFSDirectoryModel::NameFilterMode::Glob, true );
	EXPECT_EQ( 1u, model->rowCount() );
	EXPECT_STDSTREQ( "Notes.md", model->entry( 0 ).name );
	model->setNameFilter( "notes", VFSDirectoryModel::NameFilterMode::PlainText );
	EXPECT_EQ( 2u, model->rowCount() );
	model->setNameFilter( "^n.*\\.(md|txt)$", VFSDirectoryModel::NameFilterMode::RegEx );
	EXPECT_TRUE( model->isNameFilterValid() );
	EXPECT_EQ( 2u, model->rowCount() );
	model->setNameFilter( "([", VFSDirectoryModel::NameFilterMode::RegEx );
	EXPECT_FALSE( model->isNameFilterValid() );
	EXPECT_EQ( 6u, model->rowCount() ); // An invalid expression filters nothing.
	model->setNameFilter( "" );
	EXPECT_EQ( 6u, model->rowCount() );

	// Directories sort by their item count, files by type.
	model->sort( VFSDirectoryModel::Size, SortOrder::Ascending );
	EXPECT_STDSTREQ( "empty", model->entry( 0 ).name );
	EXPECT_STDSTREQ( "docs", model->entry( 1 ).name );
	model->sort( VFSDirectoryModel::Type, SortOrder::Ascending );
	EXPECT_STDSTREQ( "md", FileSystem::fileExtension( model->entry( 2 ).name ).c_str() );
}

UTEST( VFSDirectoryModel, navigationDiscardsStaleResultsAndReportsErrors ) {
	VirtualFileSystem vfs;
	auto pool = ThreadPool::createShared( 1 );
	auto mem = std::make_shared<MemoryProvider>( "mem", pool.get() );
	vfs.registerProvider( mem );
	for ( int i = 0; i < 50; ++i )
		mem->putFile( URI( "mem://n/a/a" + String::toString( i ) ), "" );
	mem->putFile( URI( "mem://n/b/only" ), "" );
	TestLoop loop;
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	config.watch = false;
	config.firstBatchSize = 1;
	config.batchSize = 1;
	auto model = VFSDirectoryModel::New( loop, config );
	model->setURI( URI( "mem://n/a" ) );
	loop.drain();
	// Navigate away while the first listing is still streaming.
	model->setURI( URI( "mem://n/b" ) );
	EXPECT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	Sys::sleep( Milliseconds( 50 ) );
	loop.drain();
	EXPECT_EQ( 1u, model->rowCount() );
	EXPECT_STDSTREQ( "only", model->entry( 0 ).name );

	model->setURI( URI( "mem://n/missing" ) );
	EXPECT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Error; } ) );
	EXPECT_EQ( VFSErrorCode::NotFound, model->error()->code );
	EXPECT_EQ( 0u, model->rowCount() );
	vfs.unregisterProvider( "mem" );
}

UTEST( VFSDirectoryModel, staysLiveThroughWatchEvents ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	FileSystem::fileWrite( tmp.path + "keep.txt", std::string_view( "1" ) );
	FileSystem::fileWrite( tmp.path + "gone.txt", std::string_view( "1" ) );
	TestLoop loop;
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	auto model = VFSDirectoryModel::New( loop, config );
	model->setURI( tmp.uri() );
	EXPECT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	EXPECT_EQ( 2u, model->rowCount() );
	EXPECT_TRUE( model->watchStatus().has_value() );

	FileSystem::fileWrite( tmp.path + "new.txt", std::string_view( "abc" ) );
	FileSystem::fileRemove( tmp.path + "gone.txt" );
	FileSystem::makeDir( tmp.path + "adir" );
	EXPECT_TRUE( loop.runUntil( [&] {
		return model->rowOf( "new.txt" ).has_value() && !model->rowOf( "gone.txt" ) &&
			   model->rowOf( "adir" ).has_value();
	} ) );
	auto names = modelNames( *model );
	EXPECT_EQ( 3u, names.size() );
	EXPECT_STDSTREQ( "adir", names[0] );
	// Modifications update the existing row in place.
	FileSystem::fileWrite( tmp.path + "keep.txt", std::string_view( "longer content" ) );
	EXPECT_TRUE( loop.runUntil( [&] {
		auto row = model->rowOf( "keep.txt" );
		return row && model->entry( *row ).size() && *model->entry( *row ).size() == 14;
	} ) );
	// Refresh reconciles without losing anything.
	model->refresh();
	EXPECT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	EXPECT_EQ( 3u, model->rowCount() );
}

UTEST( VFSDirectoryModel, moveBetweenWatchedDirectoriesUpdatesBothModels ) {
	TempDir tmp;
	VirtualFileSystem vfs;
	TestLoop loop;
	const auto from = tmp.uri().withAppendedSegment( "source" );
	const auto to = tmp.uri().withAppendedSegment( "destination" );
	ASSERT_TRUE( waitFor( vfs.makeDir( from ) ) );
	ASSERT_TRUE( waitFor( vfs.makeDir( to ) ) );
	const auto file = from.withAppendedSegment( "moved.txt" );
	ASSERT_TRUE( FileSystem::fileWrite( file.getFSPath(), std::string_view( "data" ) ) );
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	auto source = VFSDirectoryModel::New( loop, config );
	auto destination = VFSDirectoryModel::New( loop, config );
	source->setURI( from );
	destination->setURI( to );
	ASSERT_TRUE( loop.runUntil( [&] {
		return source->loadState() == VFSDirectoryModel::LoadState::Ready &&
			   destination->loadState() == VFSDirectoryModel::LoadState::Ready;
	} ) );
	ASSERT_TRUE( source->rowOf( "moved.txt" ).has_value() );
	ASSERT_TRUE( source->watchStatus().has_value() && destination->watchStatus().has_value() );
	auto transfer = vfs.move( file, to.withAppendedSegment( "moved.txt" ) );
	ASSERT_TRUE( waitFor( transfer.completion() ) );
	EXPECT_TRUE( loop.runUntil( [&] {
		return !source->rowOf( "moved.txt" ) && destination->rowOf( "moved.txt" ).has_value();
	} ) );
}

UTEST( VFSDirectoryModel, watchEventsKeepRowsInStepWithHiddenEntries ) {
	// Rows are updated in place by watch events: hidden and filtered entries in between must
	// keep the visible rows pointing at the right entries.
	TempDir tmp;
	VirtualFileSystem vfs;
	for ( const char* name : { ".a-hidden", "b-keep.txt", "c-other.txt", ".d-hidden" } )
		FileSystem::fileWrite( tmp.path + name, std::string_view( "1" ) );
	TestLoop loop;
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	auto model = VFSDirectoryModel::New( loop, config );
	model->setURI( tmp.uri() );
	EXPECT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	model->setNameFilter( "keep", VFSDirectoryModel::NameFilterMode::PlainText );
	EXPECT_EQ( 1u, model->rowCount() );

	for ( const char* name : { ".0-hidden", "a-keep.txt", "bb-other.txt", "z-KEEP.txt" } )
		FileSystem::fileWrite( tmp.path + name, std::string_view( "1" ) );
	FileSystem::fileRemove( tmp.path + ".a-hidden" );
	EXPECT_TRUE( loop.runUntil( [&] {
		return model->rowOf( "a-keep.txt" ) && model->rowOf( "z-KEEP.txt" ) &&
			   model->rowCount() == 3;
	} ) );
	FileSystem::fileRemove( tmp.path + "b-keep.txt" );
	EXPECT_TRUE( loop.runUntil( [&] { return !model->rowOf( "b-keep.txt" ); } ) );
	auto names = modelNames( *model );
	ASSERT_EQ( 2u, names.size() );
	EXPECT_STDSTREQ( "a-keep.txt", names[0] );
	EXPECT_STDSTREQ( "z-KEEP.txt", names[1] );
	// The same rows as a full rebuild.
	model->setNameFilter( "" );
	model->setNameFilter( "keep", VFSDirectoryModel::NameFilterMode::PlainText );
	EXPECT_EQ( 2u, model->rowCount() );
	EXPECT_STDSTREQ( "a-keep.txt", model->entry( 0 ).name );
	EXPECT_STDSTREQ( "z-KEEP.txt", model->entry( 1 ).name );
}

UTEST( VFSDirectoryModel, inlineExecutorAllowsReadyListingsAndReentrantRelease ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<MemoryProvider>();
	vfs.registerProvider( mem );
	for ( int i = 0; i < 5000; ++i )
		mem->putFile( URI( "mem://inline/dir/f" + String::toString( i ) ), "" );
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	config.watch = false;
	config.firstBatchSize = 1;
	config.batchSize = 1;
	auto model = VFSDirectoryModel::New( InlineExecutor::instance(), config );
	model->setURI( URI( "mem://inline/dir" ) );
	EXPECT_EQ( VFSDirectoryModel::LoadState::Ready, model->loadState() );
	EXPECT_EQ( 5000u, model->rowCount() );
	std::weak_ptr<VFSDirectoryModel> weak = model;
	model->setOnStateChanged( [&] {
		if ( model && model->loadState() == VFSDirectoryModel::LoadState::Ready )
			model.reset();
	} );
	model->refresh();
	EXPECT_TRUE( weak.expired() );
}

UTEST( VFSDirectoryModel, watchStatsCannotResurrectDeletedEntriesOrReorderUpdates ) {
	VirtualFileSystem vfs;
	auto mem = std::make_shared<DelayedStatProvider>();
	auto watch = std::make_shared<ControlledWatchBackend>();
	mem->putFile( URI( "mem://watch/dir/file" ), "" );
	vfs.registerProvider( mem );
	vfs.watchManager().registerBackend( "mem", watch );
	TestLoop loop;
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	auto model = VFSDirectoryModel::New( loop, config );
	model->setURI( URI( "mem://watch/dir" ) );
	ASSERT_TRUE( loop.runUntil(
		[&] { return model->loadState() == VFSDirectoryModel::LoadState::Ready; } ) );
	watch->send( WatchEventType::Modified );
	loop.drain();
	ASSERT_EQ( 1u, mem->pending.size() );
	watch->send( WatchEventType::Removed );
	loop.drain();
	EXPECT_FALSE( model->rowOf( "file" ) );
	VFSFileInfo info;
	info.name = "file";
	info.type = FileType::Regular;
	mem->pending[0]( Ok( info ) );
	loop.drain();
	EXPECT_FALSE( model->rowOf( "file" ) );

	watch->send( WatchEventType::Added );
	loop.drain();
	watch->send( WatchEventType::Modified );
	loop.drain();
	ASSERT_EQ( 3u, mem->pending.size() );
	info.setSize( 200 );
	mem->pending[2]( Ok( info ) );
	loop.drain();
	info.setSize( 100 );
	mem->pending[1]( Ok( info ) );
	loop.drain();
	ASSERT_TRUE( model->rowOf( "file" ) );
	EXPECT_EQ( 200u, model->entry( *model->rowOf( "file" ) ).size().value_or( 0 ) );

	watch->send( WatchEventType::Modified );
	loop.drain();
	model.reset();
	mem->pending.back()( Ok( info ) );
	loop.drain(); // A queued completion must be harmless after model destruction.
}

UTEST( MainThreadExecutor, runsDeferredOnSceneUpdate ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - MainThreadExecutor Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	ASSERT_TRUE( scene != nullptr );
	std::vector<int> order;
	{
		Scene::MainThreadExecutor executor( scene );
		executor.execute( [&] { order.push_back( 1 ); } );
		executor.execute( [&] { order.push_back( 2 ); } );
		// Never inline, even on the main thread.
		EXPECT_TRUE( order.empty() );
		EXPECT_EQ( 2u, executor.pending() );
		auto task = makeReadyTask( 3 ).then( executor, [&]( int v ) { order.push_back( v ); } );
		std::thread worker( [&] { executor.execute( [&] { order.push_back( 4 ); } ); } );
		worker.join();
		scene->update( Time::Zero );
		EXPECT_EQ( 4u, order.size() );
		if ( order.size() == 4 ) {
			EXPECT_EQ( 1, order[0] );
			EXPECT_EQ( 2, order[1] );
			EXPECT_EQ( 3, order[2] );
			EXPECT_EQ( 4, order[3] );
		}
		EXPECT_TRUE( task.isReady() );
		executor.execute( [&] { order.push_back( 5 ); } );
	}
	// Functions still queued when the executor dies are dropped.
	scene->update( Time::Zero );
	EXPECT_EQ( 4u, order.size() );
}

UTEST( MainThreadExecutor, workerWakesHiddenWindow ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Executor Wake Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	ASSERT_TRUE( scene != nullptr );
	auto* input = app.getWindow()->getInput();
	input->update();

	bool woke = false;
	bool ran = false;
	Scene::MainThreadExecutor executor( scene,
										[&] { woke = EE::Window::Input::tryWakeUpEventLoop(); } );
	std::promise<void> start;
	auto started = start.get_future();
	std::thread worker( [&] {
		started.wait();
		executor.execute( [&] { ran = true; } );
	} );

	// The timeout bounds a test failure. The worker's wake must bring us back to scene update
	// before it expires, including when the wake arrives just before waitEvent().
	using Clock = std::chrono::steady_clock;
	const auto deadline = Clock::now() + std::chrono::seconds( 3 );
	start.set_value();
	while ( !ran ) {
		const auto remaining =
			std::chrono::duration_cast<std::chrono::milliseconds>( deadline - Clock::now() );
		if ( remaining.count() <= 0 )
			break;
		input->waitEvent( Milliseconds( static_cast<Int32>( remaining.count() ) ) );
		input->update();
		scene->update( Time::Zero );
	}
	const auto elapsed = Clock::now();
	worker.join();
	EXPECT_TRUE( woke );
	EXPECT_TRUE( ran );
	EXPECT_TRUE( elapsed < deadline - std::chrono::milliseconds( 100 ) );
}

UTEST( MainThreadExecutor, shutdownWhileWakeIsInFlight ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Executor Shutdown Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	ASSERT_TRUE( scene != nullptr );

	std::promise<void> waking;
	std::promise<void> releaseWake;
	auto wakeStarted = waking.get_future();
	auto wakeReleased = releaseWake.get_future();
	std::atomic<unsigned> wakeCount{ 0 };
	bool ran = false;
	bool timedOut = false;
	Scene::MainThreadExecutor executor( scene, [&] {
		if ( wakeCount.fetch_add( 1 ) != 0 )
			return;
		waking.set_value();
		timedOut = wakeReleased.wait_for( std::chrono::seconds( 3 ) ) != std::future_status::ready;
	} );
	std::thread worker( [&] { executor.execute( [&] { ran = true; } ); } );
	const bool inFlight =
		wakeStarted.wait_for( std::chrono::seconds( 3 ) ) == std::future_status::ready;
	if ( inFlight ) {
		executor.execute( [&] { ran = true; } );
		EXPECT_EQ( 2u, executor.pending() );
		executor.shutdown();
		executor.execute( [&] { ran = true; } );
		EXPECT_EQ( 0u, executor.pending() );
	}
	releaseWake.set_value();
	worker.join();

	scene->update( Time::Zero );
	EXPECT_TRUE( inFlight );
	EXPECT_EQ( 1u, wakeCount.load() );
	EXPECT_FALSE( timedOut );
	EXPECT_FALSE( ran );
}

UTEST( VFSDirectoryModel, subclassConstructionDefersListingAndItemCountsToUIExecutor ) {
	class DerivedModel : public VFSDirectoryModel {
	  public:
		DerivedModel( Executor& executor, const Config& config ) :
			VFSDirectoryModel( executor, config ) {}
	};
	VirtualFileSystem vfs;
	TestLoop producer, ui;
	auto memory = std::make_shared<MemoryProvider>( "mem", &producer );
	vfs.registerProvider( memory );
	memory->putFile( URI( "mem:///root/folder/file" ), "data" );
	VFSDirectoryModel::Config config;
	config.vfs = &vfs;
	config.watch = false;
	config.countDirectoryItems = true;
	auto model = std::make_shared<DerivedModel>( ui, config );
	const auto uiThread = std::this_thread::get_id();
	std::atomic<bool> wrongThread{ false };
	model->setOnStateChanged( [&] {
		if ( std::this_thread::get_id() != uiThread )
			wrongThread = true;
	} );
	model->setURI( URI( "mem:///root" ) );
	// Resolve the provider's first result on a worker; no model state may change until UI drain.
	std::thread worker( [&] { producer.drain(); } );
	worker.join();
	EXPECT_EQ( VFSDirectoryModel::LoadState::Loading, model->loadState() );
	EXPECT_EQ( 0u, model->rowCount() );
	ASSERT_TRUE( ui.runUntil( [&] {
		std::thread worker( [&] { producer.drain(); } );
		worker.join();
		return model->loadState() == VFSDirectoryModel::LoadState::Ready &&
			   model->rowOf( "folder" ) && model->directoryItemCount( *model->rowOf( "folder" ) );
	} ) );
	EXPECT_FALSE( wrongThread.load() );
	EXPECT_EQ( 1u, *model->directoryItemCount( *model->rowOf( "folder" ) ) );
	// Navigation reuses the initialized gate, and pending completions remain safe on teardown.
	model->setURI( URI( "mem:///root/folder" ) );
	model.reset();
	std::thread lateWorker( [&] { producer.drain(); } );
	lateWorker.join();
	ui.drain();
}
