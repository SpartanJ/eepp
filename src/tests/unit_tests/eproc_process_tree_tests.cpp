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

class SortableTreeView : public UITreeView {
  public:
	SortableTreeView() : UITreeView() {}

	void clickSortHeader( size_t column ) { onSortColumn( column ); }
};

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
	EXPECT_TRUE( view->getHeaderColumn( ProcessModel::ColMemory ) != nullptr );
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

UTEST( EProcProcessTree, SortsSiblingsAndPreservesExpansionAndSelection ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto source = ProcessModel::create( app.getUI() );
	auto tree = ProcessTreeModel::create( source );
	source->setFilter( ProcessModel::AllProcessesInTreeForm );
	source->applySnapshot( { process( 20, 0, "zeta" ), process( 13, 10, "gamma" ),
							 process( 10, 0, "Alpha" ), process( 12, 10, "Beta" ),
							 process( 14, 10, "beta" ), process( 11, 10, "alpha" ) },
						   {} );
	auto* view = eeNew( SortableTreeView, () );
	view->setParent( app.getUI() );
	view->setPixelsSize( 500, 200 );
	view->setModel( tree );
	view->setExpanded( tree->indexForPid( 10 ), true );
	view->setSelection( tree->indexForPid( 12 ) );

	view->clickSortHeader( ProcessModel::ColName );
	EXPECT_EQ( tree->keyColumn(), static_cast<int>( ProcessModel::ColName ) );
	EXPECT_EQ( tree->sortOrder(), SortOrder::Ascending );
	EXPECT_EQ( tree->processForIndex( tree->index( 0 ) )->pid, 10 );
	EXPECT_EQ( tree->processForIndex( tree->index( 0, 0, tree->indexForPid( 10 ) ) )->pid, 11 );
	EXPECT_EQ( tree->processForIndex( tree->index( 1, 0, tree->indexForPid( 10 ) ) )->pid, 12 );
	EXPECT_EQ( tree->processForIndex( tree->index( 2, 0, tree->indexForPid( 10 ) ) )->pid, 14 );
	EXPECT_TRUE( tree->parentIndex( tree->indexForPid( 12 ) ) == tree->indexForPid( 10 ) );
	EXPECT_TRUE( view->isExpanded( tree->indexForPid( 10 ) ) );
	EXPECT_EQ( tree->processForIndex( view->getSelection().first() )->pid, 12 );

	view->clickSortHeader( ProcessModel::ColName );
	EXPECT_EQ( tree->sortOrder(), SortOrder::Descending );
	EXPECT_EQ( tree->processForIndex( tree->index( 0 ) )->pid, 20 );
	EXPECT_EQ( tree->processForIndex( tree->index( 0, 0, tree->indexForPid( 10 ) ) )->pid, 13 );
	EXPECT_EQ( tree->processForIndex( tree->index( 1, 0, tree->indexForPid( 10 ) ) )->pid, 12 );
	EXPECT_EQ( tree->processForIndex( tree->index( 2, 0, tree->indexForPid( 10 ) ) )->pid, 14 );
	EXPECT_TRUE( view->isExpanded( tree->indexForPid( 10 ) ) );
	EXPECT_EQ( tree->processForIndex( view->getSelection().first() )->pid, 12 );

	source->applySnapshot( { process( 11, 10, "alpha" ), process( 10, 0, "Alpha" ),
							 process( 13, 10, "gamma" ), process( 20, 0, "zeta" ),
							 process( 14, 10, "beta" ), process( 12, 10, "Beta" ) },
						   {} );
	EXPECT_EQ( tree->processForIndex( tree->index( 0 ) )->pid, 20 );
	EXPECT_EQ( tree->processForIndex( tree->index( 0, 0, tree->indexForPid( 10 ) ) )->pid, 13 );
}

UTEST( EProcProcessTree, FamilyMemoryIncludesHiddenDescendants ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto source = ProcessModel::create( app.getUI() );
	auto parent = process( 1, 0, "browser" );
	auto child = process( 2, 1, "renderer" );
	auto grandchild = process( 3, 2, "helper" );
	auto other = process( 4, 0, "other" );
#if EE_PLATFORM == EE_PLATFORM_LINUX
	parent.vmPSS = 100;
	child.vmPSS = 200;
	grandchild.vmPSS = 300;
	other.vmPSS = 400;
#else
	parent.vmRSS = 100;
	child.vmRSS = 200;
	grandchild.vmRSS = 300;
	other.vmRSS = 400;
#endif
	SystemInfo system;
	system.totalMemory = 1000;
	source->applySnapshot( { parent, child, grandchild, other }, system );
	EXPECT_EQ( source->getProcessByRow( source->rowForPid( 1 ) )->familyMemoryKB, 600 );
	EXPECT_TRUE(
		source
			->data( source->index( source->rowForPid( 1 ), ProcessModel::ColFamilyMemoryPercent ) )
			.toString() == "60.0%" );
	EXPECT_EQ(
		source
			->data( source->index( source->rowForPid( 1 ), ProcessModel::ColFamilyMemoryPercent ),
					ModelRole::Sort )
			.asInt64(),
		600 );
	EXPECT_EQ( source->getProcessByRow( source->rowForPid( 2 ) )->familyMemoryKB, 500 );
	EXPECT_EQ( source->getProcessByRow( source->rowForPid( 4 ) )->familyMemoryKB, 400 );
	source->setTextFilter( "browser" );
	EXPECT_EQ( source->visibleCount(), 1u );
	EXPECT_EQ( source->getProcessByRow( 0 )->familyMemoryKB, 600 );
	EXPECT_TRUE( source->data( source->index( 0, ProcessModel::ColFamilyMemory ) ).isValid() );
	source->setTextFilter( "" );
	source->applySnapshot( { parent, child }, system );
	EXPECT_EQ( source->getProcessByRow( source->rowForPid( 1 ) )->familyMemoryKB, 300 );
}

#if EE_PLATFORM == EE_PLATFORM_LINUX
UTEST( EProcProcessTree, FamilyMemoryCountsZeroRssWithoutPss ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto source = ProcessModel::create( app.getUI() );
	auto parent = process( 1, 0, "editor" );
	auto child = process( 2, 1, "helper" );
	auto zombie = process( 3, 1, "shell" );
	parent.vmPSS = 100;
	child.vmPSS = 200;
	zombie.status = ProcessStatus::Zombie;
	zombie.vmRSS = 0;
	SystemInfo system;
	system.totalMemory = 1000;
	source->applySnapshot( { parent, child, zombie }, system );
	EXPECT_EQ( source->getProcessByRow( source->rowForPid( 1 ) )->familyMemoryKB, 300 );
	EXPECT_EQ( source->getProcessByRow( source->rowForPid( 3 ) )->familyMemoryKB, 0 );

	child.vmPSS = -1;
	child.vmRSS = 100;
	source->applySnapshot( { parent, child, zombie }, system );
	EXPECT_EQ( source->getProcessByRow( source->rowForPid( 1 ) )->familyMemoryKB, -1 );
	EXPECT_TRUE(
		source
			->data( source->index( source->rowForPid( 1 ), ProcessModel::ColFamilyMemoryPercent ) )
			.toString()
			.empty() );
}
#endif
