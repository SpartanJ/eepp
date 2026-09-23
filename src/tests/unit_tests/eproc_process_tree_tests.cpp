#include "../../tools/eproc/process_model.hpp"
#include "utest.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uitreeview.hpp>

using namespace eproc;

namespace {

ProcessInfo process( long pid, long parentPid, const char* name ) {
	ProcessInfo info;
	info.pid = pid;
	info.parentPid = parentPid;
	info.name = name;
	return info;
}

} // namespace

UTEST( EProcProcessTree, ParentAndChildIndexesFollowPids ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto source = ProcessModel::create( app.getUI() );
	auto tree = ProcessTreeModel::create( source );
	source->setFilter( ProcessModel::AllProcessesInTreeForm );
	source->applySnapshot( { process( 1, 0, "init" ), process( 2, 1, "child" ),
							 process( 3, 2, "grandchild" ), process( 4, 999, "orphan" ) },
						   {} );

	EXPECT_EQ( tree->rowCount(), 2u );
	ModelIndex init = tree->indexForPid( 1 );
	ModelIndex child = tree->indexForPid( 2 );
	ModelIndex grandchild = tree->indexForPid( 3 );
	ModelIndex orphan = tree->indexForPid( 4 );
	ASSERT_TRUE( init.isValid() && child.isValid() && grandchild.isValid() && orphan.isValid() );
	EXPECT_EQ( tree->rowCount( init ), 1u );
	EXPECT_EQ( tree->rowCount( child ), 1u );
	EXPECT_TRUE( tree->parentIndex( child ) == init );
	EXPECT_TRUE( tree->parentIndex( grandchild ) == child );
	EXPECT_FALSE( tree->parentIndex( orphan ).isValid() );
	EXPECT_TRUE( tree->index( 0, ProcessModel::ColName, child ) == grandchild );
	EXPECT_EQ( tree->processForIndex( grandchild )->pid, 3 );

	auto* view = UITreeView::New();
	view->setParent( app.getUI() );
	view->setPixelsSize( 500, 200 );
	view->setModel( tree );
	view->setExpanded( init, true );
	view->setExpanded( child, true );
	EXPECT_TRUE( view->isExpanded( init ) );
	EXPECT_TRUE( view->isExpanded( child ) );
}

UTEST( EProcProcessTree, SearchKeepsAncestorsAndOrphans ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto source = ProcessModel::create( app.getUI() );
	auto tree = ProcessTreeModel::create( source );
	source->setFilter( ProcessModel::AllProcessesInTreeForm );
	source->applySnapshot( { process( 1, 0, "init" ), process( 2, 1, "child" ),
							 process( 3, 2, "grandchild" ), process( 4, 999, "orphan" ) },
						   {} );
	source->setTextFilter( "grandchild" );
	EXPECT_EQ( source->visibleCount(), 3u );
	EXPECT_EQ( source->textMatchedPids().size(), 1u );
	EXPECT_EQ( source->textMatchedPids().front(), 3 );
	EXPECT_EQ( tree->rowCount(), 1u );
	EXPECT_TRUE( tree->parentIndex( tree->indexForPid( 3 ) ) == tree->indexForPid( 2 ) );
	source->setTextFilter( "" );
	EXPECT_EQ( tree->rowCount(), 2u );
	EXPECT_TRUE( tree->indexForPid( 4 ).isValid() );
}
