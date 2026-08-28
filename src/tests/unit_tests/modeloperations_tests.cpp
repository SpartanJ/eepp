#include "utest.hpp"

#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/models/persistentmodelindex.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/window/engine.hpp>
#include <filesystem>
#include <thread>

using namespace EE::UI::Models;
using namespace EE::System;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::Window;
using namespace EE::Graphics;

namespace {

class MoveTestModel : public Model {
  public:
	struct Node {
		std::string name;
		Node* parent{ nullptr };
		std::vector<std::unique_ptr<Node>> children;
	};

	MoveTestModel() {
		a = append( root, "a" );
		b = append( root, "b" );
		a0 = append( *a, "a0" );
		a1 = append( *a, "a1" );
		a2 = append( *a, "a2" );
		b0 = append( *b, "b0" );
		b1 = append( *b, "b1" );
	}

	size_t rowCount( const ModelIndex& parent = {} ) const {
		return node( parent ).children.size();
	}

	size_t columnCount( const ModelIndex& = {} ) const { return 1; }

	Variant data( const ModelIndex& index, ModelRole = ModelRole::Display ) const {
		return static_cast<Node*>( index.internalData() )->name.c_str();
	}

	ModelIndex parentIndex( const ModelIndex& index ) const {
		Node* parent = static_cast<Node*>( index.internalData() )->parent;
		if ( !parent || parent == &root )
			return {};
		return indexFor( parent );
	}

	ModelIndex index( int row, int column = 0, const ModelIndex& parent = {} ) const {
		const auto& children = node( parent ).children;
		if ( row < 0 || static_cast<size_t>( row ) >= children.size() )
			return {};
		return createIndex( row, column, children[row].get() );
	}

	ModelIndex indexFor( Node* node ) const {
		for ( size_t row = 0; row < node->parent->children.size(); ++row )
			if ( node->parent->children[row].get() == node )
				return createIndex( row, 0, node );
		return {};
	}

	void deleteRootAt( int row ) {
		if ( beginDeleteRows( {}, row, row ) ) {
			root.children.erase( root.children.begin() + row );
			endDeleteRows();
		}
	}

	void moveA1ToB() {
		beginMoveRows( indexFor( a ), 1, 1, indexFor( b ), 1 );
		a1->parent = b;
		auto moved = std::move( a->children[1] );
		a->children.erase( a->children.begin() + 1 );
		b->children.insert( b->children.begin() + 1, std::move( moved ) );
		endMoveRows();
	}

	Node root{ "root" };
	Node* a{ nullptr };
	Node* b{ nullptr };
	Node* a0{ nullptr };
	Node* a1{ nullptr };
	Node* a2{ nullptr };
	Node* b0{ nullptr };
	Node* b1{ nullptr };

  private:
	static Node* append( Node& parent, std::string name ) {
		parent.children.emplace_back(
			std::make_unique<Node>( Node{ std::move( name ), &parent } ) );
		return parent.children.back().get();
	}

	const Node& node( const ModelIndex& index ) const {
		return index.isValid() ? *static_cast<Node*>( index.internalData() ) : root;
	}
};

class PersistentRowsModel : public Model {
  public:
	PersistentRowsModel( bool useInternalIds ) : mUseInternalIds( useInternalIds ) {
		mRows = { 10, 20, 30, 40 };
	}

	size_t rowCount( const ModelIndex& = {} ) const { return mRows.size(); }

	size_t columnCount( const ModelIndex& = {} ) const { return 1; }

	Variant data( const ModelIndex& index, ModelRole = ModelRole::Display ) const {
		return mRows[index.row()];
	}

	ModelIndex index( int row, int column = 0, const ModelIndex& = {} ) const {
		if ( row < 0 || static_cast<size_t>( row ) >= mRows.size() )
			return {};
		return createIndex( row, column, nullptr, mUseInternalIds ? mRows[row] : 0 );
	}

	void insertAt( int row, int value ) {
		beginInsertRows( {}, row, row );
		mRows.insert( mRows.begin() + row, value );
		endInsertRows();
	}

	void deleteAt( int row ) {
		if ( beginDeleteRows( {}, row, row ) ) {
			mRows.erase( mRows.begin() + row );
			endDeleteRows();
		}
	}

  private:
	std::vector<int> mRows;
	bool mUseInternalIds;
};

struct TempTree {
	TempTree() {
		static unsigned long long id = 0;
		path = std::filesystem::temp_directory_path() /
			   ( "eepp-filesystem-model-move-" + std::to_string( ++id ) );
		FileSystem::dirRemoveAll( path.string() );
		std::filesystem::create_directories( path );
	}

	~TempTree() { FileSystem::dirRemoveAll( path.string() ); }

	std::filesystem::path path;
};

static UISceneNode* initTreeViewTestScene() {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	// Use a unique font name and skip applyDefaultTheme(): the default theme
	// and the "NotoSans-Regular" resource-scope entry are shared globals that
	// later tests (e.g. UIDiffView's UIApplication) replace and free.
	FontTrueType* font = FontTrueType::New( "eepp-fsm-test-font" ).get();
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	SceneManager::instance()->setCurrentUISceneNode( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );
	return sceneNode;
}

} // namespace

UTEST( ModelMove, crossParentPersistentIndexesFollowNodesAndShiftSiblings ) {
	MoveTestModel model;
	PersistentModelIndex moved( model.indexFor( model.a1 ) );
	PersistentModelIndex sourceSibling( model.indexFor( model.a2 ) );
	PersistentModelIndex targetSibling( model.indexFor( model.b1 ) );

	model.moveA1ToB();

	ASSERT_EQ( moved.row(), 1 );
	ASSERT_EQ( static_cast<ModelIndex>( moved ).internalData(), model.a1 );
	ASSERT_EQ( static_cast<ModelIndex>( moved.parent() ).internalData(), model.b );
	ASSERT_EQ( sourceSibling.row(), 1 );
	ASSERT_EQ( static_cast<ModelIndex>( sourceSibling ).internalData(), model.a2 );
	ASSERT_EQ( static_cast<ModelIndex>( sourceSibling.parent() ).internalData(), model.a );
	ASSERT_EQ( targetSibling.row(), 2 );
	ASSERT_EQ( static_cast<ModelIndex>( targetSibling ).internalData(), model.b1 );
	ASSERT_EQ( static_cast<ModelIndex>( targetSibling.parent() ).internalData(), model.b );
}

UTEST( ModelInsert, persistentIndexesShiftWithoutIdentityCollisions ) {
	PersistentRowsModel model( false );
	PersistentModelIndex first( model.index( 0 ) );
	PersistentModelIndex second( model.index( 1 ) );
	PersistentModelIndex third( model.index( 2 ) );

	model.insertAt( 0, 5 );

	ASSERT_EQ( first.row(), 1 );
	ASSERT_EQ( second.row(), 2 );
	ASSERT_EQ( third.row(), 3 );
	ASSERT_EQ( first.data().asInt(), 10 );
	ASSERT_EQ( second.data().asInt(), 20 );
	ASSERT_EQ( third.data().asInt(), 30 );
}

UTEST( ModelInsert, persistentIndexesPreserveInternalIds ) {
	PersistentRowsModel model( true );
	PersistentModelIndex tracked( model.index( 2 ) );

	model.insertAt( 1, 15 );

	ModelIndex index = tracked;
	ASSERT_EQ( tracked.row(), 3 );
	ASSERT_EQ( index.internalId(), 30 );
}

UTEST( ModelDelete, selectedTreeIndexIsRemovedBeforeNodeFree ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "Model selection deletion test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	initTreeViewTestScene();

	auto model = std::make_shared<MoveTestModel>();
	UITreeView* treeView = UITreeView::New();
	treeView->setModel( model );
	treeView->getSelection().set( model->indexFor( model->b ) );
	ASSERT_EQ( 1, treeView->getSelection().size() );
	int selectionCallbacks = 0;
	treeView->setOnSelectionChange( [&selectionCallbacks] { ++selectionCallbacks; } );

	model->deleteRootAt( 1 );
	// This is the same non-index-invalidating refresh used after History pagination.
	model->invalidate( Model::DontInvalidateIndexes );

	EXPECT_TRUE( treeView->getSelection().isEmpty() );
	EXPECT_EQ( 0, selectionCallbacks );
}

UTEST( TableView, fitSingleColumnUsesViewportWidth ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "Single-column fit test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	initTreeViewTestScene();

	auto model = std::make_shared<MoveTestModel>();
	model->a->name.assign( 1024, 'x' );
	UITreeView* treeView = UITreeView::New();
	treeView->setPixelsSize( 220, 300 );
	treeView->setModel( model );
	treeView->setAutoExpandOnSingleColumn( true );
	treeView->setFitAllColumnsToWidget( true );
	treeView->setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
	treeView->recalculateColumnsWidth();

	EXPECT_TRUE( treeView->getColumnWidth( 0 ) <= treeView->getContentSpaceWidth() );
	EXPECT_EQ( ScrollBarMode::AlwaysOff, treeView->getHorizontalScrollMode() );
}

UTEST( ModelDelete, persistentIndexesExpireOrShiftWithoutIdentityCollisions ) {
	PersistentRowsModel model( false );
	PersistentModelIndex deleted( model.index( 1 ) );
	PersistentModelIndex third( model.index( 2 ) );
	PersistentModelIndex fourth( model.index( 3 ) );

	model.deleteAt( 1 );

	ASSERT_FALSE( deleted.isValid() );
	ASSERT_EQ( third.row(), 1 );
	ASSERT_EQ( fourth.row(), 2 );
	ASSERT_EQ( third.data().asInt(), 30 );
	ASSERT_EQ( fourth.data().asInt(), 40 );
}

UTEST( ModelDelete, persistentIndexesPreserveInternalIds ) {
	PersistentRowsModel model( true );
	PersistentModelIndex tracked( model.index( 3 ) );

	model.deleteAt( 1 );

	ModelIndex index = tracked;
	ASSERT_EQ( tracked.row(), 2 );
	ASSERT_EQ( index.internalId(), 40 );
}

UTEST( FileSystemModelMove, preservesNodeAndDescendantPathsAcrossDirectories ) {
	TempTree tree;
	auto source = tree.path / "source";
	auto target = tree.path / "target";
	std::filesystem::create_directories( source / "folder" );
	std::filesystem::create_directories( target );
	std::FILE* file = std::fopen( ( source / "folder" / "file.txt" ).string().c_str(), "wb" );
	ASSERT_TRUE( file != nullptr );
	std::fclose( file );

	auto model = FileSystemModel::New( tree.path.string() );
	auto* folderNode = model->getNodeFromPath( ( source / "folder" ).string(), true );
	auto* fileNode = model->getNodeFromPath( ( source / "folder" / "file.txt" ).string() );
	ASSERT_TRUE( folderNode != nullptr );
	ASSERT_TRUE( fileNode != nullptr );

	std::filesystem::rename( source / "folder", target / "renamed" );
	ASSERT_EQ( model->getNodeFromPath( ( source / "folder" ).string(), false, false ), folderNode );
	ASSERT_TRUE( model->handleFileEvent( { FileSystemEventType::Moved,
										   target.string() + EE::System::FileSystem::getOSSlash(),
										   "renamed", ( source / "folder" ).string() } ) );

	std::string expectedFolderPath( ( target / "renamed" ).string() );
	EE::System::FileSystem::dirAddSlashAtEnd( expectedFolderPath );
	ASSERT_STDSTREQ( folderNode->fullPath(), expectedFolderPath );
	ASSERT_EQ( model->getNodeFromPath( ( target / "renamed" ).string(), true, false ), folderNode );
	ASSERT_EQ( model->getNodeFromPath( ( target / "renamed" / "file.txt" ).string(), false, false ),
			   fileNode );
	ASSERT_STDSTREQ( fileNode->fullPath(), ( target / "renamed" / "file.txt" ).string() );
}

UTEST( FileSystemModelMove, keepsUnopenedDestinationBranchLazy ) {
	TempTree tree;
	auto source = tree.path / "source";
	auto destination = tree.path / "outer" / "destination";
	std::filesystem::create_directories( source );
	std::filesystem::create_directories( destination );
	std::FILE* file = std::fopen( ( source / "file.txt" ).string().c_str(), "wb" );
	ASSERT_TRUE( file != nullptr );
	std::fclose( file );

	auto model = FileSystemModel::New( tree.path.string() );
	ASSERT_TRUE( model->getNodeFromPath( ( source / "file.txt" ).string() ) != nullptr );
	ASSERT_TRUE( model->getNodeFromPath( destination.string(), true, false ) == nullptr );

	std::filesystem::rename( source / "file.txt", destination / "file.txt" );
	ASSERT_TRUE( model->handleFileEvent(
		{ FileSystemEventType::Moved, destination.string() + EE::System::FileSystem::getOSSlash(),
		  "file.txt", ( source / "file.txt" ).string() } ) );

	ASSERT_TRUE( model->getNodeFromPath( ( source / "file.txt" ).string(), false, false ) ==
				 nullptr );
	ASSERT_TRUE( model->getNodeFromPath( destination.string(), true, false ) == nullptr );
	ASSERT_TRUE( model->getNodeFromPath( ( destination / "file.txt" ).string(), false, true ) !=
				 nullptr );
}

UTEST( FileSystemModelEvents, preparedMetadataSurvivesDelayedAddDelivery ) {
	TempTree tree;
	auto directory = tree.path / "directory";
	auto path = directory / "transient.txt";
	std::filesystem::create_directories( directory );

	auto model = FileSystemModel::New( tree.path.string() );
	ASSERT_TRUE( model->getNodeFromPath( directory.string(), true ) != nullptr );
	// Materialize the directory so this test isolates delayed prepared metadata delivery.
	ASSERT_TRUE( model->getNodeFromPath( ( directory / "missing" ).string(), false, true ) ==
				 nullptr );

	std::FILE* file = std::fopen( path.string().c_str(), "wb" );
	ASSERT_TRUE( file != nullptr );
	std::fclose( file );
	FileInfo preparedFile( path.string(), false );
	ASSERT_TRUE( preparedFile.exists() );
	std::filesystem::remove( path );

	ASSERT_TRUE(
		model->handleFileEvent( { FileSystemEventType::Add,
								  directory.string() + FileSystem::getOSSlash(), "transient.txt" },
								preparedFile ) );
	ASSERT_TRUE( model->getNodeFromPath( path.string(), false, false ) != nullptr );
}

UTEST( FileSystemModelEvents, deletesEventCreatedNodeFromUnopenedDirectory ) {
	TempTree tree;
	auto directory = tree.path / "unopened";
	auto path = directory / "transient.cpp";
	std::filesystem::create_directories( directory );

	auto model = FileSystemModel::New( tree.path.string() );
	ASSERT_TRUE( model->getNodeFromPath( directory.string(), true, false ) != nullptr );

	std::FILE* file = std::fopen( path.string().c_str(), "wb" );
	ASSERT_TRUE( file != nullptr );
	std::fclose( file );

	// Add attaches the event-created node to the lazy parent. Delete must use that already resolved
	// index directly: querying rowCount() traverses the parent and frees the child first.
	ASSERT_TRUE( model->handleFileEvent( { FileSystemEventType::Add,
										   directory.string() + FileSystem::getOSSlash(),
										   path.filename().string() } ) );
	ASSERT_TRUE( model->getNodeFromPath( path.string(), false, false ) != nullptr );

	std::filesystem::remove( path );
	ASSERT_TRUE( model->handleFileEvent( { FileSystemEventType::Delete,
										   directory.string() + FileSystem::getOSSlash(),
										   path.filename().string() } ) );
	ASSERT_TRUE( model->getNodeFromPath( path.string(), false, false ) == nullptr );
}

UTEST( FileSystemModelEvents, deletesNodeMovedIntoUnopenedDirectory ) {
	TempTree tree;
	auto source = tree.path / "source";
	auto target = tree.path / "target";
	auto sourceFile = source / "transient.cpp";
	auto targetFile = target / "transient.cpp";
	std::filesystem::create_directories( source );
	std::filesystem::create_directories( target );
	std::FILE* file = std::fopen( sourceFile.string().c_str(), "wb" );
	ASSERT_TRUE( file != nullptr );
	std::fclose( file );

	auto model = FileSystemModel::New( tree.path.string() );
	ASSERT_TRUE( model->getNodeFromPath( sourceFile.string() ) != nullptr );
	ASSERT_TRUE( model->getNodeFromPath( target.string(), true, false ) != nullptr );

	// This models two ordered watcher events waiting in the queue. Move attaches the node to a
	// known but unopened destination. Delete must not call rowCount() and traverse that
	// destination, since traversal frees the node that Delete has already resolved.
	std::filesystem::rename( sourceFile, targetFile );
	ASSERT_TRUE( model->handleFileEvent(
		{ FileSystemEventType::Moved, target.string() + FileSystem::getOSSlash(),
		  targetFile.filename().string(), sourceFile.string() } ) );
	ASSERT_TRUE( model->getNodeFromPath( targetFile.string(), false, false ) != nullptr );

	std::filesystem::remove( targetFile );
	ASSERT_TRUE( model->handleFileEvent( { FileSystemEventType::Delete,
										   target.string() + FileSystem::getOSSlash(),
										   targetFile.filename().string() } ) );
	ASSERT_TRUE( model->getNodeFromPath( targetFile.string(), false, false ) == nullptr );
}

UTEST( FileSystemModelEvents, deletesDirectoryMovedIntoUnopenedDirectory ) {
	TempTree tree;
	auto source = tree.path / "source";
	auto target = tree.path / "target";
	auto sourceDirectory = source / "folder";
	auto targetDirectory = target / "folder";
	auto descendant = sourceDirectory / "nested" / "file.cpp";
	std::filesystem::create_directories( descendant.parent_path() );
	std::filesystem::create_directories( target );
	std::FILE* file = std::fopen( descendant.string().c_str(), "wb" );
	ASSERT_TRUE( file != nullptr );
	std::fclose( file );

	auto model = FileSystemModel::New( tree.path.string() );
	auto* directoryNode = model->getNodeFromPath( sourceDirectory.string(), true );
	auto* descendantNode = model->getNodeFromPath( descendant.string() );
	ASSERT_TRUE( directoryNode != nullptr );
	ASSERT_TRUE( descendantNode != nullptr );
	ASSERT_TRUE( model->getNodeFromPath( target.string(), true, false ) != nullptr );

	std::filesystem::rename( sourceDirectory, targetDirectory );
	ASSERT_TRUE( model->handleFileEvent(
		{ FileSystemEventType::Moved, target.string() + FileSystem::getOSSlash(),
		  targetDirectory.filename().string(), sourceDirectory.string() } ) );
	ASSERT_TRUE( model->getNodeFromPath( targetDirectory.string(), true, false ) == directoryNode );
	ASSERT_TRUE( model->getNodeFromPath( ( targetDirectory / "nested" / "file.cpp" ).string(),
										 false, false ) == descendantNode );

	std::filesystem::remove_all( targetDirectory );
	ASSERT_TRUE( model->handleFileEvent( { FileSystemEventType::Delete,
										   target.string() + FileSystem::getOSSlash(),
										   targetDirectory.filename().string() } ) );
	ASSERT_TRUE( model->getNodeFromPath( targetDirectory.string(), false, false ) == nullptr );
}

static ModelIndex indexOfNode( const FileSystemModel& model, const void* node,
							   const ModelIndex& parent = {} ) {
	for ( Int64 row = 0; row < (Int64)model.rowCount( parent ); ++row ) {
		ModelIndex idx = model.index( row, 0, parent );
		if ( idx.isValid() && idx.internalData() == node )
			return idx;
	}
	return {};
}

UTEST( FileSystemModelMove, deleteFallbackWithAttachedViewAndSelection ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "FileSystemModel test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	initTreeViewTestScene();

	TempTree tree;
	auto source = tree.path / "source";
	auto destination = tree.path / "outer" / "destination";
	std::filesystem::create_directories( source / "folder" );
	std::filesystem::create_directories( destination );
	std::FILE* file = std::fopen( ( source / "folder" / "file.txt" ).string().c_str(), "wb" );
	ASSERT_TRUE( file != nullptr );
	std::fclose( file );

	auto model = FileSystemModel::New( tree.path.string() );
	auto* sourceNode = model->getNodeFromPath( ( source ).string(), true );
	auto* folderNode = model->getNodeFromPath( ( source / "folder" ).string(), true );
	auto* fileNode = model->getNodeFromPath( ( source / "folder" / "file.txt" ).string() );
	ASSERT_TRUE( sourceNode != nullptr );
	ASSERT_TRUE( folderNode != nullptr );
	ASSERT_TRUE( fileNode != nullptr );

	UITreeView* treeView = UITreeView::New();
	treeView->setModel( model );

	// Select the file inside the folder that will be moved away.
	ModelIndex sourceIndex = indexOfNode( *model, sourceNode );
	ModelIndex folderIndex = indexOfNode( *model, folderNode, sourceIndex );
	ModelIndex fileIndex = indexOfNode( *model, fileNode, folderIndex );
	ASSERT_TRUE( fileIndex.isValid() );
	treeView->getSelection().set( fileIndex );
	ASSERT_EQ( treeView->getSelection().size(), 1 );
	int selectionCallbacks = 0;
	treeView->setOnSelectionChange( [&] {
		++selectionCallbacks;
		model->refresh();
	} );

	// Moving the folder to an unopened destination falls back to deleting the
	// materialized source node; the attached view and its selection must
	// survive without crashing.
	std::filesystem::rename( source / "folder", destination / "renamed" );
	ASSERT_TRUE( model->handleFileEvent(
		{ FileSystemEventType::Moved, destination.string() + EE::System::FileSystem::getOSSlash(),
		  "renamed", ( source / "folder" ).string() } ) );

	ASSERT_TRUE( model->getNodeFromPath( ( source / "folder" ).string(), false, false ) ==
				 nullptr );
	ASSERT_TRUE( treeView->getSelection().isEmpty() );
	// Model-driven selection repair is silent; invoking user callbacks while Delete retains raw
	// nodes would allow a re-entrant refresh to invalidate them.
	ASSERT_EQ( selectionCallbacks, 0 );
}

UTEST( FileSystemModelMove, deleteFallbackWithStaleSelectionDoesNotCrash ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "FileSystemModel test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	initTreeViewTestScene();

	TempTree tree;
	auto source = tree.path / "source";
	auto destination = tree.path / "outer" / "destination";
	std::filesystem::create_directories( source / "folder" );
	std::filesystem::create_directories( destination );
	std::FILE* file = std::fopen( ( source / "folder" / "file.txt" ).string().c_str(), "wb" );
	ASSERT_TRUE( file != nullptr );
	std::fclose( file );

	auto model = FileSystemModel::New( tree.path.string() );
	auto* sourceNode = model->getNodeFromPath( ( source ).string(), true );
	auto* folderNode = model->getNodeFromPath( ( source / "folder" ).string(), true );
	auto* fileNode = model->getNodeFromPath( ( source / "folder" / "file.txt" ).string() );
	ASSERT_TRUE( sourceNode != nullptr );
	ASSERT_TRUE( folderNode != nullptr );
	ASSERT_TRUE( fileNode != nullptr );

	UITreeView* treeView = UITreeView::New();
	treeView->setModel( model );

	ModelIndex sourceIndex = indexOfNode( *model, sourceNode );
	ModelIndex folderIndex = indexOfNode( *model, folderNode, sourceIndex );
	ModelIndex fileIndex = indexOfNode( *model, fileNode, folderIndex );
	ASSERT_TRUE( fileIndex.isValid() );
	treeView->getSelection().set( fileIndex );

	// Delete the file from disk and refresh from a background thread: the
	// model drops the node without touching the view selection (the debounced
	// selection cleanup is queued but has not run yet), leaving a stale
	// selection entry pointing at freed memory.
	std::filesystem::remove( source / "folder" / "file.txt" );
	std::thread refresher( [&model]() { model->refresh(); } );
	refresher.join();
	ASSERT_EQ( treeView->getSelection().size(), 1 );

	// Moving the folder to an unopened destination triggers the delete
	// fallback, which used to dereference the stale selection entry.
	std::filesystem::rename( source / "folder", destination / "renamed" );
	ASSERT_TRUE( model->handleFileEvent(
		{ FileSystemEventType::Moved, destination.string() + EE::System::FileSystem::getOSSlash(),
		  "renamed", ( source / "folder" ).string() } ) );

	// The stale entry must have been dropped without crashing.
	ASSERT_TRUE( treeView->getSelection().isEmpty() );
}
