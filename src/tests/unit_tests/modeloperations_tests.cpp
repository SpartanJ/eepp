#include "utest.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/models/persistentmodelindex.hpp>
#include <filesystem>

using namespace EE::UI::Models;
using namespace EE::System;

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
