#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <eepp/scene/mainthreadexecutor.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/vfs/ui/vfsdirectorymodel.hpp>

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

} // namespace

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
	model->setTypeProvider( []( const EE::VFS::FileInfo& info ) {
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
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
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
