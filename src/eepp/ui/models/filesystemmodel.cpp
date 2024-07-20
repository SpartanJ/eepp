#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>

#ifndef INDEX_ALREADY_EXISTS
#define INDEX_ALREADY_EXISTS eeINDEX_NOT_FOUND
#endif

using namespace EE::Scene;

namespace EE { namespace UI { namespace Models {

FileSystemModel::Node::Node( const std::string& rootPath, const FileSystemModel& model ) :
	mInfo( FileSystem::getRealPath( rootPath ) ) {
	mInfoDirty = false;
	mName = FileSystem::fileNameFromPath( mInfo.getFilepath() );
	mMimeType = "";
	mHash = String::hash( mName );
	mDisplayName = mName;
	traverseIfNeeded( model );
}

FileSystemModel::Node::Node( FileInfo&& info, FileSystemModel::Node* parent ) :
	mParent( parent ), mInfo( info ) {
	mInfoDirty = false;
	mName = FileSystem::fileNameFromPath( mInfo.getFilepath() );
	mHash = String::hash( mName );
	mDisplayName = mName;
	updateMimeType();
}

const std::string& FileSystemModel::Node::fullPath() const {
	return mInfo.getFilepath();
}

const FileSystemModel::Node& FileSystemModel::Node::getChild( const size_t& index ) {
	eeASSERT( index < mChildren.size() );
	return *mChildren[index];
}

void FileSystemModel::Node::invalidate() {
	mHasTraversed = false;
	mInfoDirty = true;
}

bool FileSystemModel::Node::inParentTree( Node* parent ) const {
	Node* parentLoop = mParent;
	while ( parentLoop != nullptr ) {
		if ( parentLoop == parent )
			return true;
		parentLoop = parentLoop->getParent();
	}
	return false;
}

FileSystemModel::Node* FileSystemModel::Node::findChildName( const std::string& name,
															 const FileSystemModel& model,
															 bool forceRefresh ) {
	if ( forceRefresh )
		refreshIfNeeded( model );
	for ( auto& child : mChildren ) {
		if ( child->getName() == name )
			return child;
	}
	return nullptr;
}

Int64 FileSystemModel::Node::findChildRowFromInternalData( void* internalData,
														   const FileSystemModel& model,
														   bool forceRefresh ) {
	if ( forceRefresh )
		refreshIfNeeded( model );
	for ( size_t i = 0; i < mChildren.size(); i++ ) {
		if ( mChildren[i] == internalData ) {
			return i;
		}
	}
	return -1;
}

Int64 FileSystemModel::Node::findChildRowFromName( const std::string& name,
												   const FileSystemModel& model,
												   bool forceRefresh ) {
	if ( forceRefresh )
		refreshIfNeeded( model );
	for ( size_t i = 0; i < mChildren.size(); i++ ) {
		if ( mChildren[i]->getName() == name ) {
			return i;
		}
	}
	return -1;
}

FileSystemModel::Node::~Node() {
	cleanChildren();
}

FileSystemModel::Node* FileSystemModel::Node::createChild( const std::string& childName,
														   const FileSystemModel& model ) {
	std::string childPath( mInfo.getDirectoryPath() + childName );
	FileInfo file( childPath, false );

	if ( model.getDisplayConfig().ignoreHidden && file.isHidden() )
		return nullptr;

	if ( model.getMode() == Mode::DirectoriesOnly && !file.isDirectory() )
		return nullptr;

	auto hash = String::hash( childName );

	for ( auto node : mChildren )
		if ( node->mParent == this && node->mHash == hash )
			return nullptr;

	return eeNew( Node, ( std::move( file ), this ) );
}

void FileSystemModel::Node::rename( const FileInfo& file ) {
	mInfo = file;
	mName = file.getFileName();
	mHash = String::hash( mName );
	mDisplayName = mName;
	updateMimeType();
}

ModelIndex FileSystemModel::Node::index( const FileSystemModel& model, int column ) const {
	if ( !mParent )
		return {};
	for ( size_t row = 0; row < mParent->mChildren.size(); ++row ) {
		if ( mParent->mChildren[row] == this )
			return model.createIndex( row, column, const_cast<Node*>( this ) );
	}
	eeASSERT( false );
	return {};
}

FileSystemModel::Node* FileSystemModel::Node::childWithPathExists( const std::string& path ) {
	for ( auto child : mChildren ) {
		if ( child->info().getFilepath() == path )
			return child;
	}
	return nullptr;
}

static bool isAcceptedExtension( const std::vector<std::string>& acceptedExtensions,
								 const FileInfo& file ) {
	if ( !acceptedExtensions.empty() && file.isRegularFile() ) {
		for ( size_t z = 0; z < acceptedExtensions.size(); z++ )
			if ( acceptedExtensions[z] == FileSystem::fileExtension( file.getFilepath() ) )
				return true;
		return false;
	}
	return true;
}

void FileSystemModel::Node::refresh( const FileSystemModel& model ) {
	if ( !mInfo.isDirectory() )
		return;

	auto oldFiles = mChildren;

	const auto& displayCfg = model.getDisplayConfig();

	auto files = FileSystem::filesInfoGetInPath( mInfo.getFilepath(), false, displayCfg.sortByName,
												 displayCfg.foldersFirst, displayCfg.ignoreHidden );

	std::vector<Node*> newChildren;
	Node* node = nullptr;

	for ( auto file : files ) {
		node = childWithPathExists( file.getFilepath() );

		if ( !isAcceptedExtension( displayCfg.acceptedExtensions, file ) )
			continue;

		if ( displayCfg.fileIsVisibleFn && !displayCfg.fileIsVisibleFn( file.getFilepath() ) )
			continue;

		if ( node ) {
			auto it = std::find( oldFiles.begin(), oldFiles.end(), node );
			if ( it != oldFiles.end() )
				oldFiles.erase( it );

			newChildren.emplace_back( node );

			if ( node->info().isDirectory() && node->mHasTraversed )
				node->refresh( model );
		} else {
			newChildren.emplace_back( eeNew( Node, ( std::move( file ), this ) ) );
		}
	}

	for ( Node* oldNode : oldFiles )
		eeDelete( oldNode );

	mChildren = newChildren;
}

void FileSystemModel::Node::cleanChildren() {
	for ( size_t i = 0; i < mChildren.size(); ++i )
		eeDelete( mChildren[i] );
	mChildren.clear();
}

void FileSystemModel::Node::traverseIfNeeded( const FileSystemModel& model ) {
	if ( !mInfo.isDirectory() || mHasTraversed )
		return;
	mHasTraversed = true;
	cleanChildren();

	const auto& displayCfg = model.getDisplayConfig();

	auto files = FileSystem::filesInfoGetInPath( mInfo.getFilepath(), false, displayCfg.sortByName,
												 displayCfg.foldersFirst, displayCfg.ignoreHidden );

	const auto& patterns = displayCfg.acceptedExtensions;
	bool accepted;
	for ( auto file : files ) {
		if ( ( model.getMode() == Mode::DirectoriesOnly &&
			   ( file.isDirectory() || file.linksToDirectory() ) ) ||
			 model.getMode() == Mode::FilesAndDirectories ) {
			if ( file.isDirectory() || file.linksToDirectory() || patterns.empty() ) {
				if ( displayCfg.fileIsVisibleFn &&
					 !displayCfg.fileIsVisibleFn( file.getFilepath() ) )
					continue;
				mChildren.emplace_back( eeNew( Node, ( std::move( file ), this ) ) );
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
					if ( displayCfg.fileIsVisibleFn ) {
						if ( displayCfg.fileIsVisibleFn( file.getFilepath() ) )
							accepted = true;
					} else {
						accepted = true;
					}
				}

				if ( accepted )
					mChildren.emplace_back( eeNew( Node, ( std::move( file ), this ) ) );
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
		mHash = String::hash( mName );
		mDisplayName = mName;
		mInfoDirty = false;
	}
	return true;
}

void FileSystemModel::Node::updateMimeType() {
	if ( !mInfo.isDirectory() ) {
		mMimeType = UIIconThemeManager::getIconNameFromFileName( mName );
	} else {
		mMimeType = "folder";
	}
}

std::shared_ptr<FileSystemModel> FileSystemModel::New( const std::string& rootPath,
													   const FileSystemModel::Mode& mode,
													   const DisplayConfig& displayConfig,
													   Translator* translator ) {
	return std::shared_ptr<FileSystemModel>(
		new FileSystemModel( rootPath, mode, displayConfig, translator ) );
}

FileSystemModel::FileSystemModel( const std::string& rootPath, const FileSystemModel::Mode& mode,
								  const DisplayConfig& displayConfig, Translator* translator ) :
	mRootPath( rootPath ),
	mRealRootPath( FileSystem::getRealPath( rootPath ) ),
	mMode( mode ),
	mDisplayConfig( displayConfig ) {
	mRoot = std::make_unique<Node>( mRootPath, *this );
	mInitOK = true;
	setupColumnNames( translator );
	invalidate();
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
			if ( ( foundNode = curNode->findChildName( part, *this, invalidateTree ) ) ) {
				curNode = foundNode;
			} else {
				return nullptr;
			}
		}
	}

	return curNode;
}

std::string_view FileSystemModel::getNodeRelativePath( const Node* node ) const {
	auto rp = std::string_view{ node->fullPath() };
	if ( mRootPath.size() < rp.size() )
		return rp.substr( mRootPath.size() );
	return rp;
}

void FileSystemModel::reload() {
	setRootPath( mRootPath );
}

void FileSystemModel::refresh() {
	Lock l( resourceMutex() );
	mRoot->refresh( *this );
	invalidate();
}

void FileSystemModel::update() {
	mRoot = std::make_unique<Node>( mRootPath, *this );
	invalidate();
}

const FileSystemModel::Node& FileSystemModel::node( const ModelIndex& index ) const {
	return nodeRef( index );
}

FileSystemModel::Node& FileSystemModel::nodeRef( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return *mRoot;
	Node* node = static_cast<Node*>( index.internalData() );
	return *node;
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
	eeASSERT( column < mColumnNames.size() );
	if ( column < mColumnNames.size() )
		return mColumnNames[column];
	return "";
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

	switch ( role ) {
		case ModelRole::Custom: {
			return Variant( node.info().getFilepath().c_str() );
		}
		case ModelRole::Sort: {
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
			break;
		}
		case ModelRole::Display: {
			switch ( index.column() ) {
				case Column::Icon:
					return iconFor( node, index );
				case Column::Name:
					return Variant( &node.getDisplayName() );
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
			break;
		}
		case ModelRole::Icon: {
			return iconFor( node, index );
		}
		case ModelRole::Class: {
			return stylizeModel( index, &node );
		}
		default: {
		}
	}

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
	return createIndex( row, column, node.mChildren[row] );
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

size_t FileSystemModel::getFileIndex( Node* parent, const FileInfo& file ) {
	std::vector<FileInfo> files;

	for ( Node* nodeFile : parent->mChildren ) {
		files.emplace_back( nodeFile->info() );

		if ( nodeFile->info().getFileName() == file.getFileName() )
			return INDEX_ALREADY_EXISTS;
	}

	files.emplace_back( file );

	std::sort( files.begin(), files.end(), []( FileInfo a, FileInfo b ) {
		return std::strncmp( a.getFileName().c_str(), b.getFileName().c_str(),
							 a.getFileName().size() ) < 0;
	} );

	if ( getDisplayConfig().foldersFirst ) {
		std::vector<FileInfo> folders;
		std::vector<FileInfo> file;
		for ( size_t i = 0; i < files.size(); i++ ) {
			if ( files[i].isDirectory() ) {
				folders.push_back( files[i] );
			} else {
				file.push_back( files[i] );
			}
		}
		files.clear();
		for ( auto& folder : folders )
			files.push_back( folder );
		for ( auto& f : file )
			files.push_back( f );
	}

	size_t pos = parent->childCount();

	for ( size_t i = 0; i < files.size(); ++i ) {
		if ( file.getFileName() == files[i].getFileName() ) {
			pos = i;
			break;
		}
	}

	return pos;
}

bool FileSystemModel::handleFileEventLocked( const FileEvent& event ) {
	switch ( event.type ) {
		case FileSystemEventType::Add: {
			FileInfo file( event.directory + event.filename, false );

			if ( !file.exists() )
				return false;

			if ( ( getMode() == Mode::DirectoriesOnly && !file.isDirectory() ) ||
				 ( getDisplayConfig().ignoreHidden && file.isHidden() ) )
				return false;

			auto* parent = getNodeFromPath(
				file.isDirectory() ? FileSystem::removeLastFolderFromPath( file.getDirectoryPath() )
								   : file.getDirectoryPath(),
				true, false );

			if ( !parent )
				return false;

			auto* childNodeExists =
				getNodeFromPath( file.getFilepath(), file.isDirectory(), false );
			if ( childNodeExists )
				return false;

			Node* childNode = parent->createChild( file.getFileName(), *this );

			if ( childNode == nullptr || childNode->getName().empty() )
				return false;

			size_t pos = getFileIndex( parent, file );

			const auto& displayCfg = getDisplayConfig();

			if ( displayCfg.fileIsVisibleFn && !displayCfg.fileIsVisibleFn( file.getFilepath() ) )
				return false;

			if ( pos == INDEX_ALREADY_EXISTS )
				return false;

			beginInsertRows( parent->index( *this, 0 ), pos, pos );

			if ( pos >= parent->mChildren.size() ) {
				parent->mChildren.emplace_back( childNode );
			} else {
				parent->mChildren.insert( parent->mChildren.begin() + pos, childNode );
			}

			endInsertRows();

			forEachView( [&]( UIAbstractView* view ) {
				std::vector<ModelIndex> newIndexes;
				view->getSelection().forEachIndex( [&]( const ModelIndex& selectedIndex ) {
					Node* curNode = static_cast<Node*>( selectedIndex.internalData() );
					if ( curNode->getParent() == parent ) {
						if ( selectedIndex.row() >= (Int64)pos ) {
							newIndexes.emplace_back( this->index( selectedIndex.row() + 1,
																  selectedIndex.column(),
																  selectedIndex.parent() ) );
						} else {
							newIndexes.emplace_back( selectedIndex );
						}
					} else {
						newIndexes.emplace_back( selectedIndex );
					}
				} );
				view->getSelection().set( newIndexes, false );
			} );

			break;
		}
		case FileSystemEventType::Delete: {
			FileInfo file( event.directory + event.filename, false );

			auto* child = getNodeFromPath( file.getFilepath(), file.isDirectory(), false );
			if ( !child )
				return false;

			Node* parent = child->mParent;
			if ( !parent )
				return false;

			ModelIndex index = child->index( *this, 0 );
			if ( !index.isValid() )
				return false;

			Int64 pos = index.row();

			forEachView( [&]( UIAbstractView* view ) {
				view->getSelection().removeAllMatching( [&]( auto& selectionIndex ) {
					Node* node = static_cast<Node*>( index.internalData() );
					Node* nodeSelected = static_cast<Node*>( selectionIndex.internalData() );
					return selectionIndex.internalData() == index.internalData() ||
						   ( node->childCount() > 0 && nodeSelected->inParentTree( node ) );
				} );
			} );

			if ( beginDeleteRows( index.parent(), index.row(), index.row() ) ) {
				eeDelete( parent->mChildren[index.row()] );
				parent->mChildren.erase( parent->mChildren.begin() + index.row() );
				endDeleteRows();
			}

			forEachView( [&]( UIAbstractView* view ) {
				std::vector<ModelIndex> newIndexes;
				view->getSelection().forEachIndex( [&]( const ModelIndex& selectedIndex ) {
					if ( !selectedIndex.isValid() )
						return;
					Node* curNode = static_cast<Node*>( selectedIndex.internalData() );
					if ( curNode->getParent() == parent ) {
						if ( selectedIndex.row() >= (Int64)pos ) {
							auto newIndex =
								this->index( selectedIndex.row() - 1, selectedIndex.column(),
											 selectedIndex.parent() );
							if ( newIndex.isValid() )
								newIndexes.emplace_back( newIndex );
						} else {
							newIndexes.emplace_back( selectedIndex );
						}
					} else {
						newIndexes.emplace_back( selectedIndex );
					}
				} );

				view->getSelection().set( newIndexes, false );
			} );

			break;
		}
		case FileSystemEventType::Moved: {
			FileInfo file( event.directory + event.filename, false );

			if ( !file.exists() )
				return false;

			auto* node = getNodeFromPath( event.directory + event.oldFilename, false, false );
			if ( !node ) {
				return handleFileEventLocked(
					{ FileSystemEventType::Add, event.directory, event.filename } );
			}

			ModelIndex index = node->index( *this, 0 );
			if ( !index.isValid() )
				return false;

			Node* parent = node->mParent;
			if ( !parent )
				return false;

			if ( ( getMode() == Mode::DirectoriesOnly && !file.isDirectory() ) )
				return false;

			if ( !node->info().isHidden() && getDisplayConfig().ignoreHidden && file.isHidden() ) {
				return handleFileEventLocked(
					{ FileSystemEventType::Delete, event.directory, event.oldFilename } );
			}

			Node* childNode = parent->mChildren[index.row()];
			childNode->rename( file );
			parent->mChildren.erase( parent->mChildren.begin() + index.row() );

			size_t pos = getFileIndex( node->getParent(), file );

			// Don't add the file if already exists (if moved an old file to another old
			// file)
			if ( pos == INDEX_ALREADY_EXISTS ) {
				eeDelete( childNode );
				return false;
			}

			std::map<UIAbstractView*, std::vector<ModelIndex>> keptSelections;
			std::map<UIAbstractView*, std::vector<std::string>> prevSelections;
			std::map<UIAbstractView*, std::vector<ModelIndex>> prevSelectionsModelIndex;

			forEachView( [&]( UIAbstractView* view ) {
				view->getSelection().forEachIndex( [&]( const ModelIndex& selectedIndex ) {
					Node* curNode = static_cast<Node*>( selectedIndex.internalData() );
					if ( curNode->mParent == parent ) {
						prevSelectionsModelIndex[view].emplace_back( selectedIndex );
						prevSelections[view].emplace_back(
							( curNode->getName() == event.oldFilename ) ? event.filename
																		: curNode->getName() );
					} else {
						keptSelections[view].emplace_back( selectedIndex );
					}
				} );
			} );

			beginMoveRows( index.parent(), index.row(), index.row(), index.parent(), pos );

			if ( pos >= parent->mChildren.size() ) {
				parent->mChildren.emplace_back( childNode );
			} else {
				parent->mChildren.insert( parent->mChildren.begin() + pos, childNode );
			}

			endMoveRows();

			forEachView( [&]( UIAbstractView* view ) {
				std::vector<std::string> names = prevSelections[view];
				std::vector<ModelIndex> newIndexes = keptSelections[view];
				int i = 0;
				for ( const auto& name : names ) {
					Int64 row = parent->findChildRowFromName( name, *this );
					if ( row >= 0 ) {
						newIndexes.emplace_back(
							this->index( row, prevSelectionsModelIndex[view][i].column(),
										 prevSelectionsModelIndex[view][i].parent() ) );
					}
					++i;
				}
				view->getSelection().set( newIndexes, false );
			} );
			break;
		}
		case FileSystemEventType::Modified: {
			return false;
		}
	}

	return true;
}

void FileSystemModel::setupColumnNames( Translator* translator ) {
	const auto i18n = [&translator]( const std::string& key,
									 const std::string& value ) -> std::string {
		return translator ? translator->getString( "filesystemmodel_column_" + key, value ).toUtf8()
						  : value;
	};
	mColumnNames[Column::Icon] = "";
	mColumnNames[Column::Name] = i18n( "name", "Name" );
	mColumnNames[Column::Size] = i18n( "size", "Size" );
	mColumnNames[Column::Owner] = i18n( "owner", "Owner" );
	mColumnNames[Column::Group] = i18n( "group", "Group" );
	mColumnNames[Column::Permissions] = i18n( "mode", "Mode" );
	mColumnNames[Column::ModificationTime] = i18n( "modified", "Modified" );
	mColumnNames[Column::Inode] = i18n( "inode", "Inode" );
	mColumnNames[Column::Path] = i18n( "path", "Path" );
	mColumnNames[Column::SymlinkTarget] = i18n( "symlink_target", "Symlink target" );
}

bool FileSystemModel::handleFileEvent( const FileEvent& event ) {
	if ( !mInitOK )
		return false;

	bool ret;

	{
		Lock l( resourceMutex() );

		ret = handleFileEventLocked( event );
	}

	if ( ret )
		invalidate( UpdateFlag::DontInvalidateIndexes );

	return ret;
}

std::shared_ptr<DiskDrivesModel> DiskDrivesModel::create( const std::vector<std::string>& data ) {
	return std::shared_ptr<DiskDrivesModel>( new DiskDrivesModel( data ) );
}

std::shared_ptr<DiskDrivesModel> DiskDrivesModel::create() {
	return create( Sys::getLogicalDrives() );
}

UIIcon* DiskDrivesModel::diskIcon() const {
	auto* scene = SceneManager::instance()->getUISceneNode();
	auto* d = scene->findIcon( "drive" );
	if ( !d )
		d = scene->findIcon( "folder" );
	return d;
}

Variant DiskDrivesModel::data( const ModelIndex& index, ModelRole role ) const {
	eeASSERT( index.row() >= 0 && index.row() < (Int64)mData.size() );
	if ( role == ModelRole::Display ) {
		switch ( index.column() ) {
			case Column::Icon:
				return diskIcon();
			case Column::Name:
				return Variant( mData[index.row()].c_str() );
		}
	}

	if ( role == ModelRole::Icon )
		return diskIcon();

	if ( role == ModelRole::Custom )
		return Variant( mData[index.row()].c_str() );

	return {};
}

}}} // namespace EE::UI::Models
