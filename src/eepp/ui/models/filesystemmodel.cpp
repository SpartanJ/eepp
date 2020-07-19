#include <ctime>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <iomanip>
#include <iostream>

using namespace EE::Scene;

namespace EE { namespace UI { namespace Models {

FileSystemModel::Node::Node( const std::string& rootPath, const FileSystemModel& model ) :
	mInfo( FileSystem::getRealPath( rootPath ) ) {
	mName = FileSystem::fileNameFromPath( mInfo.getFilepath() );
	mMimeType = "";
	traverseIfNeeded( model );
}

FileSystemModel::Node::Node( FileInfo&& info, FileSystemModel::Node* parent ) :
	mParent( parent ), mInfo( info ) {
	mName = FileSystem::fileNameFromPath( mInfo.getFilepath() );
	if ( !mInfo.isDirectory() ) {
		mMimeType = "filetype-" + FileSystem::fileExtension( mName );
	} else {
		mMimeType = "folder";
	}
}

const std::string& FileSystemModel::Node::fullPath() const {
	return mInfo.getFilepath();
}

ModelIndex FileSystemModel::Node::index( const FileSystemModel& model, int column ) const {
	if ( !mParent )
		return {};
	for ( size_t row = 0; row < mParent->mChildren.size(); ++row ) {
		if ( &mParent->mChildren[row] == this )
			return model.createIndex( row, column, const_cast<Node*>( this ) );
	}
	eeASSERT( false );
	return {};
}

void FileSystemModel::Node::traverseIfNeeded( const FileSystemModel& model ) {
	if ( !mInfo.isDirectory() || mHasTraversed )
		return;
	mHasTraversed = true;

	auto files = FileSystem::filesInfoGetInPath( mInfo.getFilepath(), true, true );

	for ( auto file : files ) {
		if ( ( model.getMode() == Mode::DirectoriesOnly && file.isDirectory() ) ||
			 model.getMode() == Mode::FilesAndDirectories )
			mChildren.emplace_back( Node( std::move( file ), this ) );
	}
}

void FileSystemModel::Node::reifyIfNeeded( const FileSystemModel& model ) {
	traverseIfNeeded( model );
	fetchData( fullPath() );
}

bool FileSystemModel::Node::fetchData( const String& fullPath ) {
	mInfo = FileInfo( fullPath, mParent == nullptr );
	return true;
}

std::shared_ptr<FileSystemModel> FileSystemModel::New( const std::string& rootPath,
													   const FileSystemModel::Mode& mode ) {
	return std::make_shared<FileSystemModel>( rootPath, mode );
}

FileSystemModel::FileSystemModel( const std::string& rootPath, const FileSystemModel::Mode& mode ) :
	mRootPath( rootPath ), mMode( mode ) {
	update();
}

std::string FileSystemModel::getRootPath() const {
	return mRootPath;
}

void FileSystemModel::setRootPath( const std::string& rootPath ) {
	mRootPath = rootPath;
	update();
}

void FileSystemModel::update() {
	mRoot = std::make_unique<Node>( mRootPath, *this );
	onModelUpdate();
}

const FileSystemModel::Node& FileSystemModel::node( const ModelIndex& index ) const {
	return nodeRef( index );
}

FileSystemModel::Node& FileSystemModel::nodeRef( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return *mRoot;
	return *(Node*)index.data();
}

size_t FileSystemModel::rowCount( const ModelIndex& index ) const {
	Node& node = const_cast<Node&>( this->node( index ) );
	node.reifyIfNeeded( *this );
	if ( node.info().isDirectory() )
		return node.mChildren.size();
	return 0;
}

size_t FileSystemModel::columnCount( const ModelIndex& ) const {
	return Column::Count;
}

std::string FileSystemModel::columnName( const size_t& column ) const {
	switch ( column ) {
		case Column::Icon:
			return "";
		case Column::Name:
			return "Name";
		case Column::Size:
			return "Size";
		case Column::Owner:
			return "Owner";
		case Column::Group:
			return "Group";
		case Column::Permissions:
			return "Mode";
		case Column::ModificationTime:
			return "Modified";
		case Column::Inode:
			return "Inode";
		case Column::SymlinkTarget:
			return "Symlink target";
		default:
			return "";
	}
}

static std::string permissionString( const FileInfo& info ) {
	std::string builder;
	if ( info.isDirectory() )
		builder.append( "d" );
	else if ( info.isLink() )
		builder.append( "l" );
	else if ( info.isRegularFile() )
		builder.append( "-" );
	else
		builder.append( "?" );
	return builder;
}

static std::string timestampString( const Uint64& time ) {
	std::time_t t = time;
	auto tm = *std::localtime( &t );
	std::ostringstream oss;
	oss << std::put_time( &tm, "%Y-%m-%d %H:%M" );
	return oss.str();
}

Variant FileSystemModel::data( const ModelIndex& index, Model::Role role ) const {
	eeASSERT( index.isValid() );

	auto& node = this->nodeRef( index );

	if ( role == Role::Custom ) {
		eeASSERT( index.column() == Column::Name );
		return Variant( node.info().getFilepath() );
	}

	if ( role == Role::Sort ) {
		switch ( index.column() ) {
			case Column::Icon:
				return node.info().isDirectory() ? 0 : 1;
			case Column::Name:
				return Variant( node.getName().c_str() );
			case Column::Size:
				return node.info().getSize();
			case Column::Owner:
				return node.info().getOwnerId();
			case Column::Group:
				return node.info().getGroupId();
			case Column::Permissions:
				return Variant( permissionString( node.info() ) );
			case Column::ModificationTime:
				return node.info().getModificationTime();
			case Column::Inode:
				return node.info().getInode();
			case Column::SymlinkTarget:
				return Variant( node.info().linksTo() );
			default:
				eeASSERT( false );
		}
	}

	if ( role == Role::Display ) {
		switch ( index.column() ) {
			case Column::Icon:
				return !node.getMimeType().empty() ? Variant( iconFor( node, index ) ) : Variant();
			case Column::Name:
				return Variant( node.getName().c_str() );
			case Column::Size:
				return Variant( FileSystem::sizeToString( node.info().getSize() ) );
			case Column::Owner:
				return Variant( String::toString( node.info().getOwnerId() ) );
			case Column::Group:
				return Variant( String::toString( node.info().getGroupId() ) );
			case Column::Permissions:
				return Variant( permissionString( node.info() ) );
			case Column::ModificationTime:
				return Variant( timestampString( node.info().getModificationTime() ) );
			case Column::Inode:
				return Variant( String::toString( node.info().getInode() ) );
			case Column::SymlinkTarget:
				return node.info().isLink() ? Variant( node.info().linksTo() ) : Variant();
		}
	}

	if ( role == Role::Icon )
		return iconFor( node, index );

	return {};
}

ModelIndex FileSystemModel::parentIndex( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return {};
	auto& node = this->node( index );
	if ( !node.getParent() ) {
		eeASSERT( &node == mRoot.get() );
		return {};
	}
	return node.getParent()->index( *this, index.column() );
}

ModelIndex FileSystemModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( row < 0 || column < 0 )
		return {};
	auto& node = this->node( parent );
	const_cast<Node&>( node ).reifyIfNeeded( *this );
	if ( static_cast<size_t>( row ) >= node.mChildren.size() )
		return {};
	return createIndex( row, column, &node.mChildren[row] );
}

Drawable* FileSystemModel::iconFor( const Node& node, const ModelIndex& index ) const {
	auto* scene = SceneManager::instance()->getUISceneNode();
	Drawable* d = scene->findIcon( node.getMimeType() );
	if ( !d && !node.info().isDirectory() && index.column() == (Int64)treeColumn() )
		return scene->findIcon( "file" );
	return nullptr;
}

}}} // namespace EE::UI::Models
