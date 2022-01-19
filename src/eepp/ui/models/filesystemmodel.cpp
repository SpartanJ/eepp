#include <ctime>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <iomanip>
#include <iostream>

using namespace EE::Scene;

namespace EE { namespace UI { namespace Models {

FileSystemModel::Node::Node( const std::string& rootPath, const FileSystemModel& model ) :
	mInfo( FileSystem::getRealPath( rootPath ) ) {
	mInfoDirty = false;
	mName = FileSystem::fileNameFromPath( mInfo.getFilepath() );
	mMimeType = "";
	traverseIfNeeded( model );
}

FileSystemModel::Node::Node( FileInfo&& info, FileSystemModel::Node* parent ) :
	mParent( parent ), mInfo( info ) {
	mInfoDirty = false;
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

const FileSystemModel::Node& FileSystemModel::Node::getChild( const size_t& index ) {
	eeASSERT( index < mChildren.size() );
	return mChildren[index];
}

void FileSystemModel::Node::invalidate() {
	mHasTraversed = false;
	mInfoDirty = true;
}

FileSystemModel::Node* FileSystemModel::Node::findChildName( const std::string& name,
															 const FileSystemModel& model,
															 bool forceRefresh ) {
	if ( forceRefresh )
		refreshIfNeeded( model );
	for ( auto& child : mChildren ) {
		if ( child.getName() == name )
			return &child;
	}
	return nullptr;
}

FileSystemModel::Node FileSystemModel::Node::createChild( const std::string& childName,
														  const FileSystemModel& model ) {
	std::string childPath( mInfo.getDirectoryPath() + childName );
	FileInfo file( childPath );
	auto child = Node( std::move( file ), this );

	if ( model.getDisplayConfig().ignoreHidden && file.isHidden() )
		return {};

	if ( model.getMode() == Mode::DirectoriesOnly && !file.isDirectory() )
		return {};

	return child;
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
	mChildren.clear();

	auto files = FileSystem::filesInfoGetInPath(
		mInfo.getFilepath(), true, model.getDisplayConfig().sortByName,
		model.getDisplayConfig().foldersFirst, model.getDisplayConfig().ignoreHidden );

	const auto& patterns = model.getDisplayConfig().acceptedExtensions;
	bool accepted;
	for ( auto file : files ) {
		if ( ( model.getMode() == Mode::DirectoriesOnly &&
			   ( file.isDirectory() || file.linksToDirectory() ) ) ||
			 model.getMode() == Mode::FilesAndDirectories ) {
			if ( file.isDirectory() || file.linksToDirectory() || patterns.empty() ) {
				mChildren.emplace_back( Node( std::move( file ), this ) );
			} else {
				accepted = false;
				if ( patterns.size() ) {
					for ( size_t z = 0; z < patterns.size(); z++ ) {
						if ( patterns[z] == FileSystem::fileExtension( file.getFilepath() ) ) {
							accepted = true;
							break;
						}
					}
				} else {
					accepted = true;
				}
				if ( accepted )
					mChildren.emplace_back( Node( std::move( file ), this ) );
			}
		}
	}
}

void FileSystemModel::Node::refreshIfNeeded( const FileSystemModel& model ) {
	traverseIfNeeded( model );
	if ( mInfoDirty )
		fetchData( fullPath() );
}

bool FileSystemModel::Node::fetchData( const String& fullPath ) {
	if ( mInfoDirty ) {
		mInfo = FileInfo( fullPath, mParent == nullptr );
		mName = FileSystem::fileNameFromPath( mInfo.getFilepath() );
		mInfoDirty = false;
	}
	return true;
}

std::shared_ptr<FileSystemModel> FileSystemModel::New( const std::string& rootPath,
													   const FileSystemModel::Mode& mode,
													   const DisplayConfig displayConfig ) {
	return std::shared_ptr<FileSystemModel>( new FileSystemModel( rootPath, mode, displayConfig ) );
}

FileSystemModel::FileSystemModel( const std::string& rootPath, const FileSystemModel::Mode& mode,
								  const DisplayConfig displayConfig ) :
	mRootPath( rootPath ),
	mRealRootPath( FileSystem::getRealPath( rootPath ) ),
	mMode( mode ),
	mDisplayConfig( displayConfig ) {
	mRoot = std::make_unique<Node>( mRootPath, *this );
	mInitOK = true;
	onModelUpdate();
}

FileSystemModel::~FileSystemModel() {
	mInitOK = false;
}

const std::string& FileSystemModel::getRootPath() const {
	return mRootPath;
}

void FileSystemModel::setRootPath( const std::string& rootPath ) {
	mRootPath = rootPath;
	mRealRootPath = FileSystem::getRealPath( mRootPath );
	update();
}

FileSystemModel::Node* FileSystemModel::getNodeFromPath( std::string path, bool folderNode,
														 bool invalidateTree ) {
	path = FileSystem::getRealPath( path );
	if ( folderNode && !FileSystem::isDirectory( path ) )
		path = FileSystem::fileRemoveFileName( path );
	if ( String::startsWith( path, mRealRootPath ) )
		path = path.substr( mRealRootPath.size() );
	else if ( path.empty() ||
			  !( path[0] == '/' ||
				 ( path.size() >= 2 && String::isLetter( path[0] ) && path[1] == ':' ) ) ) {
		return nullptr;
	}
	if ( String::contains( path, "\\" ) )
		String::replaceAll( path, "\\", "/" );
	auto folders = String::split( path, '/' );
	Node* curNode = mRoot.get();
	Node* foundNode = nullptr;

	if ( !folders.empty() ) {
		for ( size_t i = 0; i < folders.size(); i++ ) {
			auto& part = folders[i];
			if ( ( foundNode = curNode->findChildName(
					   part, *this, invalidateTree || i == folders.size() - 1 ) ) ) {
				curNode = foundNode;
			} else {
				return nullptr;
			}
		}
	}

	return curNode;
}

void FileSystemModel::reload() {
	setRootPath( mRootPath );
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
	return *(Node*)index.internalData();
}

size_t FileSystemModel::rowCount( const ModelIndex& index ) const {
	Node& node = const_cast<Node&>( this->node( index ) );
	node.refreshIfNeeded( *this );
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
		case Column::Path:
			return "Path";
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

Variant FileSystemModel::data( const ModelIndex& index, ModelRole role ) const {
	eeASSERT( index.isValid() );

	auto& node = this->nodeRef( index );

	if ( role == ModelRole::Custom )
		return Variant( node.info().getFilepath().c_str() );

	if ( role == ModelRole::Sort ) {
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
			case Column::Path:
				return Variant( node.info().getFilepath().c_str() );
			case Column::SymlinkTarget:
				return node.info().isLink() ? Variant( node.info().linksTo() ) : Variant( "" );
			default:
				eeASSERT( false );
		}
	}

	if ( role == ModelRole::Display ) {
		switch ( index.column() ) {
			case Column::Icon:
				return iconFor( node, index );
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
				return Variant( Sys::epochToString( node.info().getModificationTime() ) );
			case Column::Inode:
				return Variant( String::toString( node.info().getInode() ) );
			case Column::Path:
				return Variant( node.info().getFilepath().c_str() );
			case Column::SymlinkTarget:
				return node.info().isLink() ? Variant( node.info().linksTo() ) : Variant( "" );
		}
	}

	if ( role == ModelRole::Icon )
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
	const_cast<Node&>( node ).refreshIfNeeded( *this );
	if ( static_cast<size_t>( row ) >= node.mChildren.size() )
		return {};
	return createIndex( row, column, &node.mChildren[row] );
}

UIIcon* FileSystemModel::iconFor( const Node& node, const ModelIndex& index ) const {
	if ( index.column() == (Int64)treeColumn() || Column::Icon == index.column() ) {
		auto* scene = SceneManager::instance()->getUISceneNode();
		auto* d = scene->findIcon( node.getMimeType() );
		if ( !d )
			return scene->findIcon( !node.info().isDirectory() ? "file" : "folder" );
		return d;
	}
	return nullptr;
}

void FileSystemModel::setMode( const Mode& mode ) {
	if ( mode != mMode ) {
		mMode = mode;
		reload();
	}
}

const FileSystemModel::DisplayConfig& FileSystemModel::getDisplayConfig() const {
	return mDisplayConfig;
}

void FileSystemModel::setDisplayConfig( const DisplayConfig& displayConfig ) {
	if ( mDisplayConfig != displayConfig ) {
		mDisplayConfig = displayConfig;
		reload();
	}
}

const ModelIndex& FileSystemModel::getPreviouslySelectedIndex() const {
	return mPreviouslySelectedIndex;
}

void FileSystemModel::setPreviouslySelectedIndex( const ModelIndex& previouslySelectedIndex ) {
	mPreviouslySelectedIndex = previouslySelectedIndex;
}

void FileSystemModel::handleFileEvent( const FileEvent& event ) {
	if ( !mInitOK )
		return;

	switch ( event.type ) {
		case FileSystemEventType::Add: {
			FileInfo file( event.directory + event.filename );
			auto* parent = getNodeFromPath(
				file.isDirectory() ? FileSystem::removeLastFolderFromPath( file.getDirectoryPath() )
								   : file.getDirectoryPath(),
				true, false );

			if ( parent ) {
				auto* childNodeExists =
					getNodeFromPath( file.getFilepath(), file.isDirectory(), false );
				if ( childNodeExists )
					return;

				size_t childCount = parent->childCount();

				Node childNode = parent->createChild( file.getFilepath(), *this );

				if ( !childNode.getName().empty() ) {
					beginInsertRows( parent->index( *this, 0 ), childCount, childCount );

					parent->mChildren.emplace_back( std::move( childNode ) );

					endInsertRows();
				} else {
					return;
				}
			}
			break;
		}
		case FileSystemEventType::Delete: {
			FileInfo file( event.directory + event.filename );

			auto* child = getNodeFromPath( file.getFilepath(), file.isDirectory(), false );
			if ( !child )
				return;

			Node* parent = child->mParent;
			if ( !parent )
				return;

			ModelIndex index = child->index( *this, 0 );
			if ( !index.isValid() )
				return;

			beginDeleteRows( index.parent(), index.row(), index.row() );

			parent->mChildren.erase( parent->mChildren.begin() + index.row() );

			endDeleteRows();

			forEachView( [&]( UIAbstractView* view ) {
				view->getSelection().removeAllMatching( [&]( auto& selectionIndex ) {
					return selectionIndex.internalData() == index.internalData();
				} );
			} );

			break;
		}
		case FileSystemEventType::Moved: {
			FileInfo file( event.directory + event.filename );
			if ( file.exists() ) {
				auto* node = getNodeFromPath( event.directory + event.oldFilename,
											  file.isDirectory(), false );
				if ( node ) {
					node->mInfo = std::move( file );
					node->mName = FileSystem::fileNameFromPath( node->mInfo.getFilepath() );
				} else {
					handleFileEvent(
						{ FileSystemEventType::Add, event.directory, event.filename } );
				}
			}
			break;
		}
		case FileSystemEventType::Modified: {
			break;
		}
	}

	onModelUpdate( UpdateFlag::DontInvalidateIndexes );
}

}}} // namespace EE::UI::Models
