#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>

#ifndef INDEX_ALREADY_EXISTS
#define INDEX_ALREADY_EXISTS eeINDEX_NOT_FOUND
#endif

using namespace EE::Scene;

namespace EE { namespace UI { namespace Models {

FileSystemModel::Node::Node( const std::string& rootPath, FileSystemModel& model,
							 const std::shared_ptr<ThreadPool>& threadPool ) :
	mModel( &model ), mInfo( FileSystem::getRealPath( rootPath ) ) {
	mInfoDirty = false;
	mName = FileSystem::fileNameFromPath( mInfo.getFilepath() );
	mMimeType = "";
	mHash = String::hash( mName );
	mDisplayName = mName;
	{
		Lock l( model.mResourceLock );
		mId = model.mNextNodeId++;
		model.mAliveNodes.insert( this );
	}
	if ( threadPool ) {
		mQueuedForTraversal = true;
		threadPool->run( [this, &model]() {
			traverseIfNeeded( model );
			model.refreshView();
			model.invalidate( Model::DontInvalidateIndexes );
		} );
	} else {
		traverseIfNeeded( model );
	}
}

FileSystemModel::Node::Node( FileInfo&& info, FileSystemModel::Node* parent,
							 const FileSystemModel& model ) :
	mParent( parent ), mModel( &model ), mInfo( info ) {
	mInfoDirty = false;
	mName = FileSystem::fileNameFromPath( mInfo.getFilepath() );
	mHash = String::hash( mName );
	mDisplayName = mName;
	updateMimeType();
	{
		Lock l( model.mResourceLock );
		mId = model.mNextNodeId++;
		model.mAliveNodes.insert( this );
	}
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
	while ( mIsTraversing )
		Sys::sleep( Milliseconds( 1 ) );
	cleanChildren();
	if ( mModel ) {
		Lock l( mModel->mResourceLock );
		mModel->mAliveNodes.erase( this );
	}
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

	return eeNew( Node, ( std::move( file ), this, model ) );
}

void FileSystemModel::Node::rename( const FileInfo& file ) {
	auto oldParentFile = mInfo;
	mInfo = file;
	mName = file.getFileName();
	mHash = String::hash( mName );
	mDisplayName = mName;
	updateMimeType();
	updateChildren( oldParentFile, mInfo );
}

void FileSystemModel::Node::updateChildren( const FileInfo& oldParentFile,
											const FileInfo& newParentFile ) {
	for ( Node* child : mChildren ) {
		if ( String::startsWith( child->mInfo.getFilepath(), oldParentFile.getFilepath() ) ) {
			std::string newFilePath( child->mInfo.getFilepath() );
			newFilePath.replace( 0, oldParentFile.getFilepath().size(),
								 newParentFile.getFilepath() );
			child->rename( FileInfo( newFilePath ) );
		}
	}
}

ModelIndex FileSystemModel::Node::index( const FileSystemModel& model, int column ) const {
	if ( !mParent )
		return {};
	for ( size_t row = 0; row < mParent->mChildren.size(); ++row ) {
		if ( mParent->mChildren[row] == this )
			return model.createIndex( row, column, const_cast<Node*>( this ), mId );
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

bool FileSystemModel::Node::refresh( const FileSystemModel& model ) {
	if ( !mInfo.isDirectory() )
		return false;

	std::vector<Node*> oldFiles;

	{
		Lock l( model.mResourceLock );
		oldFiles = mChildren;
	}

	const auto& displayCfg = model.getDisplayConfig();

	auto files = FileSystem::filesInfoGetInPath(
		mInfo.getFilepath(), false, displayCfg.sortByName, displayCfg.foldersFirst,
		displayCfg.ignoreHidden,
		[&model] { return model.mShuttingDown.load( std::memory_order_relaxed ); } );

	std::vector<Node*> newChildren;
	Node* node = nullptr;

	for ( auto& file : files ) {
		{
			Lock l( model.mResourceLock );
			node = childWithPathExists( file.getFilepath() );
		}

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
			newChildren.emplace_back( eeNew( Node, ( std::move( file ), this, model ) ) );
		}
	}

	Lock l( model.mResourceLock );

	for ( Node* oldNode : oldFiles )
		eeDelete( oldNode );

	mChildren = std::move( newChildren );
	return true;
}

void FileSystemModel::Node::cleanChildren() {
	size_t size = mChildren.size();
	for ( size_t i = 0; i < size; ++i )
		eeDelete( mChildren[i] );
	mChildren.clear();
}

bool FileSystemModel::Node::traverseIfNeeded( const FileSystemModel& model ) {
	if ( !mInfo.isDirectory() || mHasTraversed || mIsTraversing )
		return false;
	BoolScopedOp op( mIsTraversing, true );

	{
		Lock l( model.mResourceLock );
		cleanChildren();
	}

	const auto& displayCfg = model.getDisplayConfig();
	auto files = FileSystem::filesInfoGetInPath(
		mInfo.getFilepath(), false, displayCfg.sortByName, displayCfg.foldersFirst,
		displayCfg.ignoreHidden,
		[&model] { return model.mShuttingDown.load( std::memory_order_relaxed ); } );

	const auto& patterns = displayCfg.acceptedExtensions;
	bool accepted;
	std::vector<Node*> newChildren;
	newChildren.reserve( files.size() );

	for ( auto& file : files ) {
		if ( ( model.getMode() == Mode::DirectoriesOnly &&
			   ( file.isDirectory() || file.linksToDirectory() ) ) ||
			 model.getMode() == Mode::FilesAndDirectories ) {
			if ( file.isDirectory() || file.linksToDirectory() || patterns.empty() ) {
				if ( displayCfg.fileIsVisibleFn &&
					 !displayCfg.fileIsVisibleFn( file.getFilepath() ) )
					continue;
				newChildren.emplace_back( eeNew( Node, ( std::move( file ), this, model ) ) );
			} else {
				accepted = false;
				size_t psize = patterns.size();
				if ( psize ) {
					auto ext( FileSystem::fileExtension( file.getFilepath() ) );
					for ( size_t z = 0; z < psize; z++ ) {
						if ( patterns[z] == ext ) {
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
					newChildren.emplace_back( eeNew( Node, ( std::move( file ), this, model ) ) );
			}
		}
	}

	{
		Lock l( model.mResourceLock );
		mChildren = std::move( newChildren );
	}

	mHasTraversed = true;
	mQueuedForTraversal = false;
	return true;
}

bool FileSystemModel::Node::refreshIfNeeded( const FileSystemModel& model,
											 const std::shared_ptr<ThreadPool>& threadPool ) {
	bool res = false;
	if ( threadPool ) {
		if ( !mInfo.isDirectory() || mHasTraversed || mIsTraversing || mQueuedForTraversal )
			return false;
		threadPool->run( [this, &model] {
			traverseIfNeeded( model );
			model.refreshView();
		} );
		return true;
	} else {
		res = traverseIfNeeded( model );
	}
	if ( mInfoDirty )
		fetchData( fullPath() );
	return res;
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
													   Translator* translator,
													   std::shared_ptr<ThreadPool> threadPool ) {
	return std::shared_ptr<FileSystemModel>(
		new FileSystemModel( rootPath, mode, displayConfig, translator, threadPool ) );
}

FileSystemModel::FileSystemModel( const std::string& rootPath, const FileSystemModel::Mode& mode,
								  const DisplayConfig& displayConfig, Translator* translator,
								  std::shared_ptr<ThreadPool> threadPool ) :
	mRootPath( rootPath ),
	mRealRootPath( FileSystem::getRealPath( rootPath ) ),
	mMode( mode ),
	mDisplayConfig( displayConfig ),
	mThreadPool( threadPool ) {
	mRoot = std::make_unique<Node>( mRootPath, *this, threadPool );
	mInitOK = true;
	setupColumnNames( translator );
	invalidate();
}

FileSystemModel::~FileSystemModel() {
	mShuttingDown = true;
	mInitOK = false;
	mRoot.reset();
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
	// A configured root can be an alias of its canonical path (for example, /var maps to
	// /private/var on macOS). Translate that prefix first so a moved or deleted leaf, which can no
	// longer be resolved by realpath(), still maps to the canonical paths stored by the model.
	const bool rootMatches =
		!mRootPath.empty() &&
		( path == mRootPath ||
		  ( String::startsWith( path, mRootPath ) &&
			( mRootPath.back() == '/' || mRootPath.back() == '\\' ||
			  ( path.size() > mRootPath.size() &&
				( path[mRootPath.size()] == '/' || path[mRootPath.size()] == '\\' ) ) ) ) );
	if ( mRootPath != mRealRootPath && rootMatches )
		path.replace( 0, mRootPath.size(), mRealRootPath );
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
			{
				Lock l( mResourceLock );
				foundNode = curNode->findChildName( part, *this, invalidateTree );
			}
			if ( foundNode ) {
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
	if ( mRealRootPath.size() < rp.size() && String::startsWith( rp, mRealRootPath ) )
		return rp.substr( mRealRootPath.size() );
	if ( mRootPath.size() < rp.size() && String::startsWith( rp, mRootPath ) )
		return rp.substr( mRootPath.size() );
	return rp;
}

void FileSystemModel::reload() {
	setRootPath( mRootPath );
}

void FileSystemModel::refresh() {
	// NOTE: refresh() mutates the node tree (deletes stale nodes) on the
	// calling thread, which may be a worker. The resource lock protects the
	// mutation itself, but readers that dereference nodes without holding the
	// lock (data(), rowCount(), index(), ...) can race with the deletion: the
	// live-node registry is crash hardening, not full thread safety. Fully
	// closing this race requires applying structural changes on the main
	// thread or locking every access.
	{
		Lock l( resourceMutex() );
		mRoot->refresh( *this );
	}
	invalidate();
}

void FileSystemModel::update() {
	mRoot = std::make_unique<Node>( mRootPath, *this, mThreadPool );
	invalidate();
}

const FileSystemModel::Node& FileSystemModel::node( const ModelIndex& index ) const {
	// Unchecked accessor: the index must be valid (or the root {}), and its
	// node must still be alive. Callers that cannot guarantee this must use
	// nodePtr() and handle null. Violating the precondition is undefined
	// behavior (the debug assert fires; release dereferences the pointer).
	Node* node = nodeRef( index );
	eeASSERT( node != nullptr );
	return *node;
}

const FileSystemModel::Node* FileSystemModel::nodePtr( const ModelIndex& index ) const {
	return nodeRef( index );
}

FileSystemModel::Node* FileSystemModel::nodeRef( const ModelIndex& index ) const {
	// An invalid index is the root; only a valid-looking index whose node is
	// gone (deleted by a background refresh, or its address reused by a newer
	// node) resolves to null.
	if ( !index.isValid() )
		return mRoot.get();
	Node* node = static_cast<Node*>( index.internalData() );
	// A stale index (node already deleted by a background refresh, or its
	// address reused by a newer node) must not be dereferenced. The address +
	// generation check rejects both cases; the next model update drops the
	// stale index from the views. The check runs under the resource lock, but
	// the returned pointer is used after the lock is released: a concurrent
	// refresh() can still delete the node in between (see refresh()).
	if ( !isNodeAlive( node, index.internalId() ) )
		return nullptr;
	return node;
}

bool FileSystemModel::isNodeAlive( const Node* node, Uint64 id ) const {
	Lock l( mResourceLock );
	return mAliveNodes.find( node ) != mAliveNodes.end() && node->mId == id;
}

bool FileSystemModel::isValid( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return false;
	Lock l( mResourceLock );
	const Node* node = static_cast<const Node*>( index.internalData() );
	if ( mAliveNodes.find( node ) == mAliveNodes.end() ||
		 node->mId != static_cast<Uint64>( index.internalId() ) )
		return false;
	return Model::isValid( index );
}

size_t FileSystemModel::rowCount( const ModelIndex& index ) const {
	Node* node = nodeRef( index );
	if ( !node )
		return 0;
	if ( node->mIsTraversing )
		return 0;
	bool isThreaded = mThreadPool && node == mRoot.get();
	bool res = node->refreshIfNeeded( *this, isThreaded ? mThreadPool : nullptr );
	if ( isThreaded && res )
		return 0;
	if ( node->info().isDirectory() )
		return node->mChildren.size();
	return 0;
}

size_t FileSystemModel::columnCount( const ModelIndex& ) const {
	return Column::Count;
}

bool FileSystemModel::hasChildren( const ModelIndex& index ) const {
	Node* node = nodeRef( index );
	if ( !node )
		return false;
	if ( node->mInfoDirty )
		node->fetchData( node->fullPath() );
	return node->mInfo.isDirectory();
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

	Node* node = nodeRef( index );
	if ( !node )
		return {};

	switch ( role ) {
		case ModelRole::Custom: {
			return Variant( node->info().getFilepath().c_str() );
		}
		case ModelRole::Sort: {
			switch ( index.column() ) {
				case Column::Icon:
					return node->info().isDirectory() ? 0 : 1;
				case Column::Name:
					return Variant( node->getName().c_str() );
				case Column::Size:
					return node->info().getSize();
				case Column::Owner:
					return node->info().getOwnerId();
				case Column::Group:
					return node->info().getGroupId();
				case Column::Permissions:
					return Variant( permissionString( node->info() ) );
				case Column::ModificationTime:
					return node->info().getModificationTime();
				case Column::Inode:
					return node->info().getInode();
				case Column::Path:
					return Variant( node->info().getFilepath().c_str() );
				case Column::SymlinkTarget:
					return node->info().isLink() ? Variant( node->info().linksTo() )
												 : Variant( "" );
				default:
					eeASSERT( false );
			}
			break;
		}
		case ModelRole::Display: {
			switch ( index.column() ) {
				case Column::Icon:
					return iconFor( *node, index );
				case Column::Name:
					return Variant( &node->getDisplayName() );
				case Column::Size:
					return Variant( FileSystem::sizeToString( node->info().getSize() ) );
				case Column::Owner:
					return Variant( String::toString( node->info().getOwnerId() ) );
				case Column::Group:
					return Variant( String::toString( node->info().getGroupId() ) );
				case Column::Permissions:
					return Variant( permissionString( node->info() ) );
				case Column::ModificationTime:
					return Variant( Sys::epochToString( node->info().getModificationTime() ) );
				case Column::Inode:
					return Variant( String::toString( node->info().getInode() ) );
				case Column::Path:
					return Variant( node->info().getFilepath().c_str() );
				case Column::SymlinkTarget:
					return node->info().isLink() ? Variant( node->info().linksTo() )
												 : Variant( "" );
			}
			break;
		}
		case ModelRole::Icon: {
			return iconFor( *node, index );
		}
		case ModelRole::Class: {
			return stylizeModel( index, node );
		}
		default: {
		}
	}

	return {};
}

ModelIndex FileSystemModel::parentIndex( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return {};
	Node* node = nodeRef( index );
	if ( !node )
		return {};
	if ( !node->getParent() ) {
		eeASSERT( node == mRoot.get() );
		return {};
	}
	return node->getParent()->index( *this, index.column() );
}

ModelIndex FileSystemModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( row < 0 || column < 0 )
		return {};
	Node* node = nodeRef( parent );
	if ( !node )
		return {};
	bool isThreaded = mThreadPool && node == mRoot.get();
	bool res = node->refreshIfNeeded( *this, isThreaded ? mThreadPool : nullptr );
	if ( isThreaded && res )
		return {};
	if ( static_cast<size_t>( row ) >= node->mChildren.size() )
		return {};
	return createIndex( row, column, node->mChildren[row], node->mChildren[row]->mId );
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

size_t FileSystemModel::getFileIndex( Node* parent, const FileInfo& file,
									  const Node* excludedNode ) {
	std::vector<FileInfo> files;
	files.reserve( parent->mChildren.size() + 1 );

	for ( Node* nodeFile : parent->mChildren ) {
		if ( nodeFile == excludedNode )
			continue;
		files.emplace_back( nodeFile->info() );

		if ( nodeFile->info().getFileName() == file.getFileName() )
			return INDEX_ALREADY_EXISTS;
	}

	files.emplace_back( file );

	std::sort( files.begin(), files.end(), []( const FileInfo& a, const FileInfo& b ) {
		return std::strncmp( a.getFileName().c_str(), b.getFileName().c_str(),
							 a.getFileName().size() ) < 0;
	} );

	if ( getDisplayConfig().foldersFirst ) {
		std::stable_partition( files.begin(), files.end(),
							   []( const FileInfo& info ) { return info.isDirectory(); } );
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

bool FileSystemModel::handleFileEventLocked( const FileEvent& event,
											 const FileInfo* preparedFile ) {
	switch ( event.type ) {
		case FileSystemEventType::Add: {
			FileInfo file =
				preparedFile ? *preparedFile : FileInfo( event.directory + event.filename, false );

			if ( !preparedFile && !file.exists() )
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

			size_t pos = getFileIndex( parent, file );

			const auto& displayCfg = getDisplayConfig();

			if ( displayCfg.fileIsVisibleFn && !displayCfg.fileIsVisibleFn( file.getFilepath() ) )
				return false;

			if ( pos == INDEX_ALREADY_EXISTS )
				return false;

			// The listener can provide metadata captured on its worker thread. Construct the node
			// from that snapshot instead of probing the path again on the UI thread.
			Node* childNode = eeNew( Node, ( FileInfo( file ), parent, *this ) );

			if ( childNode == nullptr || childNode->getName().empty() ) {
				eeDelete( childNode );
				return false;
			}

			beginInsertRows( parent->index( *this, 0 ), pos, pos );

			{
				Lock l( mResourceLock );
				if ( pos >= parent->mChildren.size() ) {
					parent->mChildren.emplace_back( childNode );
				} else {
					parent->mChildren.insert( parent->mChildren.begin() + pos, childNode );
				}
			}

			endInsertRows();

			forEachView( [&]( UIAbstractView* view ) {
				std::vector<ModelIndex> newIndexes;
				view->getSelection().forEachIndex( [&]( const ModelIndex& selectedIndex ) {
					Node* curNode = static_cast<Node*>( selectedIndex.internalData() );
					if ( !isNodeAlive( curNode, selectedIndex.internalId() ) ) {
						// Stale selection entry (node deleted by a background
						// refresh): keep it untouched, the next model update
						// drops it. Never dereference freed memory.
						newIndexes.emplace_back( selectedIndex );
						return;
					}
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
			FileInfo file =
				preparedFile ? *preparedFile : FileInfo( event.directory + event.filename, false );

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
					// A stale selection entry points at freed memory: drop it
					// instead of dereferencing it.
					if ( !isNodeAlive( nodeSelected, selectionIndex.internalId() ) )
						return true;
					return selectionIndex.internalData() == index.internalData() ||
						   ( node->childCount() > 0 && nodeSelected->inParentTree( node ) );
				} );
			} );

			if ( beginDeleteRows( index.parent(), index.row(), index.row() ) ) {
				auto notifyDescendantsDeleted = [&]( auto&& notify, const Node* node ) -> void {
					for ( const Node* childNode : node->mChildren ) {
						notifyIndexDeleted( childNode );
						notify( notify, childNode );
					}
				};
				notifyDescendantsDeleted( notifyDescendantsDeleted, child );
				{
					Lock l( mResourceLock );
					eeDelete( parent->mChildren[index.row()] );
					parent->mChildren.erase( parent->mChildren.begin() + index.row() );
				}
				endDeleteRows();
			}

			forEachView( [&]( UIAbstractView* view ) {
				std::vector<ModelIndex> newIndexes;
				view->getSelection().forEachIndex( [&]( const ModelIndex& selectedIndex ) {
					if ( !selectedIndex.isValid() )
						return;
					Node* curNode = static_cast<Node*>( selectedIndex.internalData() );
					if ( !isNodeAlive( curNode, selectedIndex.internalId() ) ) {
						newIndexes.emplace_back( selectedIndex );
						return;
					}
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
			FileInfo file =
				preparedFile ? *preparedFile : FileInfo( event.directory + event.filename, false );
			const std::string oldFilePath = FileSystem::isRelativePath( event.oldFilename )
												? event.directory + event.oldFilename
												: event.oldFilename;

			if ( !preparedFile && !file.exists() )
				return false;

			auto* node = getNodeFromPath( oldFilePath, false, false );
			if ( !node ) {
				return handleFileEventLocked(
					{ FileSystemEventType::Add, event.directory, event.filename }, preparedFile );
			}

			ModelIndex index = node->index( *this, 0 );
			if ( !index.isValid() )
				return false;
			ModelIndex sourceParentIndex = index.parent();

			Node* sourceParent = node->mParent;
			if ( !sourceParent )
				return false;

			if ( ( getMode() == Mode::DirectoriesOnly && !file.isDirectory() ) )
				return false;

			if ( !node->info().isHidden() && getDisplayConfig().ignoreHidden && file.isHidden() ) {
				return handleFileEventLocked( { FileSystemEventType::Delete, "", oldFilePath } );
			}

			const auto& displayCfg = getDisplayConfig();

			if ( displayCfg.fileIsVisibleFn && !displayCfg.fileIsVisibleFn( file.getFilepath() ) ) {
				return handleFileEventLocked( { FileSystemEventType::Delete, "", oldFilePath } );
			}

			Node* targetParent = getNodeFromPath(
				file.isDirectory() ? FileSystem::removeLastFolderFromPath( file.getDirectoryPath() )
								   : file.getDirectoryPath(),
				true, false );
			// Keep unopened branches lazy. The node only needs to disappear from its old,
			// materialized parent; a later traversal of the destination will discover it.
			if ( !targetParent )
				return handleFileEventLocked( { FileSystemEventType::Delete, "", oldFilePath } );

			// Don't add the file if already exists (if moved an old file to another old
			// file)
			Node* targetChild = targetParent->findChildName( file.getFileName(), *this );
			if ( targetChild && targetChild != node )
				return handleFileEventLocked( { FileSystemEventType::Delete, "", oldFilePath } );

			UnorderedMap<UIAbstractView*, std::vector<ModelIndex>> selections;

			forEachView( [&]( UIAbstractView* view ) {
				view->getSelection().forEachIndex( [&]( const ModelIndex& selectedIndex ) {
					selections[view].emplace_back( selectedIndex );
				} );
			} );

			Node* childNode = sourceParent->mChildren[index.row()];
			size_t pos = getFileIndex( targetParent, file, childNode );
			eeASSERT( pos != INDEX_ALREADY_EXISTS );
			beginMoveRows( sourceParentIndex, index.row(), index.row(),
						   targetParent->index( *this, 0 ), pos );

			{
				Lock l( mResourceLock );
				sourceParent->mChildren.erase( sourceParent->mChildren.begin() + index.row() );
				childNode->rename( file );
				childNode->mParent = targetParent;
			}

			{
				Lock l( mResourceLock );
				if ( pos >= targetParent->mChildren.size() ) {
					targetParent->mChildren.emplace_back( childNode );
				} else {
					targetParent->mChildren.insert( targetParent->mChildren.begin() + pos,
													childNode );
				}
			}

			endMoveRows();

			forEachView( [&]( UIAbstractView* view ) {
				std::vector<ModelIndex> newIndexes;
				newIndexes.reserve( selections[view].size() );
				for ( const ModelIndex& selectedIndex : selections[view] ) {
					Node* selectedNode = static_cast<Node*>( selectedIndex.internalData() );
					if ( !isNodeAlive( selectedNode, selectedIndex.internalId() ) ) {
						newIndexes.emplace_back( selectedIndex );
						continue;
					}
					ModelIndex newIndex = selectedNode->index( *this, selectedIndex.column() );
					if ( newIndex.isValid() )
						newIndexes.emplace_back( std::move( newIndex ) );
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
	return handleFileEvent( event, nullptr );
}

bool FileSystemModel::handleFileEvent( const FileEvent& event, const FileInfo& file ) {
	return handleFileEvent( event, &file );
}

bool FileSystemModel::handleFileEvent( const FileEvent& event, const FileInfo* preparedFile ) {
	if ( !mInitOK )
		return false;

	// Views are UI objects: this must run on the main thread. ecode's
	// FileSystemListener dispatches watcher events to the main thread; direct
	// callers are responsible for the same requirement. Without an Engine no
	// view can exist, so the check is skipped.
	eeASSERT( !Engine::existsSingleton() || Engine::isMainThread() );

	bool ret;

	{
		Lock l( resourceMutex() );

		ret = handleFileEventLocked( event, preparedFile );
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
