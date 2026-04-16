#include "uitreeviewfs.hpp"
#include "customwidgets.hpp"
#include "ecode.hpp"
#include "notificationcenter.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/window/cursormanager.hpp>

#include <filesystem>

namespace ecode {

static const std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings() {
	return {
		{ { KEY_C, KeyMod::getDefaultModifier() }, "copy" },
		{ { KEY_X, KeyMod::getDefaultModifier() }, "cut" },
		{ { KEY_V, KeyMod::getDefaultModifier() }, "paste" },
		{ { KEY_RETURN }, "open-selected-files" },
		{ { KEY_KP_ENTER }, "open-selected-files" },
		{ { KEY_DELETE }, "delete-selected-files" },
		{ { KEY_BACKSPACE }, "delete-selected-files" },
	};
}

class UITreeViewCellFS : public UITreeViewCell {
  public:
	static UITextView* sDragTV;

	static UITreeViewCellFS* New() { return eeNew( UITreeViewCellFS, () ); }

	UITreeViewCellFS() : UITreeViewCell() { setDragEnabled( true ); }

	Uint32 getType() const override {
		return static_cast<Uint32>( CustomWidgets::UI_TYPE_TREEVIEWCELLFS );
	}

	bool isType( const Uint32& type ) const override {
		return getType() == type ? true : UITreeViewCell::isType( type );
	}

	bool acceptsDropOfWidget( const UIWidget* widget ) override {
		return widget->isType( static_cast<Uint32>( CustomWidgets::UI_TYPE_TREEVIEWCELLFS ) );
	}

	inline UITreeViewFS* getTreeView() const {
		return getParent()->getParent()->asType<UITreeViewFS>();
	}

	const FileSystemModel* getModel() const {
		auto model = getParent()->getParent()->asType<UITreeView>()->getModel();
		auto fsm = static_cast<const FileSystemModel*>( model );
		return fsm;
	}

	Uint32 onDrag( const Vector2f& position, const Uint32& flags, const Sizef& dragDiff ) override {
		sDragTV->setVisible( true )->setEnabled( false );
		sDragTV->setPixelsPosition( position + PixelDensity::dpToPx( 8 ) );
		getUISceneNode()->getWindow()->getCursorManager()->set( Cursor::Hand );

		setEnabled( false );
		Node* overFind = getUISceneNode()->overFind( position );
		setEnabled( true );

		if ( overFind && overFind->isType( UI_TYPE_WIDGET ) ) {
			UIWidget* widget = overFind->asType<UIWidget>()->acceptsDropOfWidgetInTree( this );

			if ( mCurDropWidget ) {
				mCurDropWidget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 0 );
				mCurDropWidget = nullptr;
			}

			if ( widget ) {
				mCurDropWidget = widget;
				mCurDropWidget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 1 );
			}
		}

		return 1;
	}

	Uint32 onDragStart( const Vector2i& position, const Uint32& flags ) override {
		if ( sDragTV == nullptr ) {
			sDragTV = UITextView::New();
			sDragTV->setClass( "dragged_cell" );
		}
		sDragTV->setVisible( false )->setEnabled( false );
		sDragTV->setText( getTextView()->getText() );
		setClass( "dragged" );

		auto tvfs = getTreeView();
		const auto& selection = tvfs->getSelection();
		std::string dragPaths;
		auto fsm = static_cast<const FileSystemModel*>( getModel() );
		std::string rootPath = fsm->getRootPath();
		tvfs->getSourceDragMultiplePaths().clear();
		for ( int i = 0; i < selection.size(); ++i ) {
			auto path = tvfs->getSelectionPathAtIndex( i );
			std::string relPath( path );
			FileSystem::filePathRemoveBasePath( rootPath, relPath );
			dragPaths += relPath;
			if ( i < selection.size() - 1 )
				dragPaths += "\n";
			tvfs->getSourceDragMultiplePaths().emplace_back( path );
		}
		sDragTV->setText( dragPaths );
		return UITreeViewCell::onDragStart( position, flags );
	}

	Uint32 onDragStop( const Vector2i& position, const Uint32& flags ) override {
		sDragTV->setVisible( false );
		removeClass( "dragged" );
		getUISceneNode()->getWindow()->getCursorManager()->set( Cursor::Arrow );

		if ( mCurDropWidget ) {
			mCurDropWidget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 0 );
			mCurDropWidget = nullptr;
		}
		return UITreeViewCell::onDragStop( position, flags );
	}

	Uint32 onKeyDown( const KeyEvent& event ) override {
		if ( event.getKeyCode() == KEY_ESCAPE && getEventDispatcher()->isNodeDragging() &&
			 getEventDispatcher()->getNodeDragging()->isUINode() ) {
			getEventDispatcher()->getNodeDragging()->asType<UINode>()->setDragging( false, false );
			getEventDispatcher()->setNodeDragging( nullptr );
			return 1;
		}

		std::string cmd = getTreeView()->getKeyBindings().getCommandFromKeyBind(
			{ event.getKeyCode(), event.getMod() } );
		if ( !cmd.empty() ) {
			getTreeView()->execute( cmd );
			return 1;
		}
		return UITreeViewCell::onKeyDown( event );
	}

	const std::string& getCurrentPath() const {
		return getModel()->node( getCurIndex() ).fullPath();
	}

  protected:
	UIWidget* mCurDropWidget{ nullptr };
};

UITextView* UITreeViewCellFS::sDragTV = nullptr;

UITreeViewFS::UITreeViewFS() : UITreeView(), mKeyBindings( getInput() ) {
	mCommands["copy"] = [this] {
		if ( getSelection().isEmpty() )
			return;
		auto& selection = getSelection();
		mSrcCopyMultiplePaths.clear();
		mWasCut = false;
		for ( int i = 0; i < selection.size(); ++i )
			mSrcCopyMultiplePaths.emplace_back( getSelectionPathAtIndex( i ) );
		if ( selection.size() > 1 ) {
			NotificationCenter::instance()->addNotification( String::format(
				i18n( "copied_files", "Copied %d items" ).toUtf8(), selection.size() ) );
		} else if ( selection.size() == 1 ) {
			NotificationCenter::instance()->addNotification(
				String::format( i18n( "copied_file", "Copied '%s'" ).toUtf8(),
								FileSystem::fileNameFromPath( getSelectionPathAtIndex( 0 ) ) ) );
		}
	};

	mCommands["cut"] = [this] {
		if ( getSelection().isEmpty() )
			return;
		auto& selection = getSelection();
		mSrcCopyMultiplePaths.clear();
		mWasCut = true;
		for ( int i = 0; i < selection.size(); ++i )
			mSrcCopyMultiplePaths.emplace_back( getSelectionPathAtIndex( i ) );

		if ( selection.size() > 1 ) {
			NotificationCenter::instance()->addNotification(
				String::format( i18n( "cut_files", "Cut %d items" ).toUtf8(), selection.size() ) );
		} else {
			NotificationCenter::instance()->addNotification(
				String::format( i18n( "cut_file", "Cut '%s'" ).toUtf8(),
								FileSystem::fileNameFromPath( getSelectionPathAtIndex( 0 ) ) ) );
		}
	};

	mCommands["paste"] = [this] {
		auto& selection = getSelection();
		std::string targetPath = selection.isEmpty() ? "" : getSelectionPath();
		if ( targetPath.empty() )
			return;
		FileInfo targetInfo( targetPath );
		if ( !targetInfo.isDirectory() ) {
			targetInfo = FileInfo( targetInfo.getDirectoryPath() );
		}
		std::string dstPath( targetInfo.getFilepath() );
		if ( mWasCut ) {
			if ( !mSrcCopyMultiplePaths.empty() )
				moveFiles( mSrcCopyMultiplePaths, dstPath );
			mSrcCopyMultiplePaths.clear();
		} else {
			if ( !mSrcCopyMultiplePaths.empty() )
				copyFiles( mSrcCopyMultiplePaths, dstPath );
			mSrcCopyMultiplePaths.clear();
		}
	};

	mCommands["open-selected-files"] = [this] { openSelectedFiles(); };

	mCommands["delete-selected-files"] = [this] { deleteSelectedFiles(); };

	mKeyBindings.addKeybinds( getDefaultKeybindings() );

	setSelectionKind( SelectionKind::Multiple );
}

UIWidget* UITreeViewFS::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	auto* widget = UITreeViewCellFS::New();
	return setupCell( widget, rowWidget, index );
}

Uint32 UITreeViewFS::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::Drop ) {
		const NodeDropMessage* dropMsg = static_cast<const NodeDropMessage*>( msg );
		static constexpr auto expectedType =
			static_cast<Uint32>( CustomWidgets::UI_TYPE_TREEVIEWCELLFS );
		if ( dropMsg->getSender()->isType( expectedType ) &&
			 dropMsg->getDroppedNode()->isType( expectedType ) ) {
			auto dst = dropMsg->getSender()->asType<UITreeViewCellFS>()->getCurrentPath();
			moveFiles( mSrcDragMultiplePaths, dst );
			return 1;
		}
	}
	return UITreeView::onMessage( msg );
}

void UITreeViewFS::moveFiles( const std::vector<std::string>& paths, const std::string& dstDir ) {
	if ( paths.empty() )
		return;
	std::string confirmMsg;
	std::vector<std::pair<std::string, std::string>> validMoves;
	auto fsm = static_cast<const FileSystemModel*>( getModel() );

	for ( const auto& srcPath : paths ) {
		FileInfo srcInfo( srcPath );
		if ( !srcInfo.exists() )
			continue;
		FileInfo dstInfo( dstDir );
		if ( !dstInfo.isDirectory() ) {
			dstInfo = FileInfo( dstInfo.getDirectoryPath() );
		}
		if ( srcInfo.getDirectoryPath() == dstInfo.getFilepath() )
			continue;
		validMoves.emplace_back( srcPath, dstInfo.getFilepath() );
	}

	if ( validMoves.empty() )
		return;

	std::string partialDst( dstDir );
	FileSystem::filePathRemoveBasePath( fsm->getRootPath(), partialDst );
	if ( !FileSystem::isDirectory( partialDst ) )
		partialDst = FileSystem::fileRemoveFileName( partialDst );
	FileSystem::dirAddSlashAtEnd( partialDst );
	if ( partialDst.empty() ) // If it's empty it's the root path
		partialDst = fsm->getRootPath();

	if ( validMoves.size() == 1 ) {
		std::string partialSrc( validMoves[0].first );
		FileSystem::filePathRemoveBasePath( fsm->getRootPath(), partialSrc );

		confirmMsg = ( String::format(
			i18n( "confirm_move_file_or_dir", "Are you sure you want to move:\n%s\ninto:\n%s" )
				.toUtf8(),
			partialSrc, partialDst ) );
	} else {
		std::string listFiles;
		static constexpr auto MAX_LISTED_FILES = 10;
		for ( size_t i = 0; i < validMoves.size() && i < MAX_LISTED_FILES; ++i ) {
			std::string partialSrc( validMoves[i].first );
			FileSystem::filePathRemoveBasePath( fsm->getRootPath(), partialSrc );
			listFiles += partialSrc + "\n";
		}
		if ( validMoves.size() > MAX_LISTED_FILES ) {
			listFiles +=
				"... and " + String::format( "%d", validMoves.size() - MAX_LISTED_FILES ) + " more";
		}

		confirmMsg = String::format(
			i18n( "confirm_move_multiple", "Are you sure you want to move:\n%s\ninto: %s" )
				.toUtf8(),
			listFiles, partialDst );
	}

	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK_CANCEL, confirmMsg );
	msgBox->setTitle( "ecode" );
	msgBox->center();
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [validMoves]( const Event* ) {
		for ( const auto& move : validMoves ) {
			std::string dstPath( move.second );
			FileSystem::dirAddSlashAtEnd( dstPath );
			dstPath += FileInfo( move.first ).getFileName();
			FileSystem::fileMove( move.first, dstPath );
		}
	} );
}

void UITreeViewFS::copyFile( const std::string& src, const std::string& dst ) {
	FileInfo srcInfo( src );
	if ( !srcInfo.exists() )
		return;
	FileInfo dstInfo( dst );
	if ( !dstInfo.exists() )
		return;
	if ( srcInfo.getFilepath() != srcInfo.getRealPath() )
		srcInfo = FileInfo( srcInfo.getRealPath() );
	if ( !dstInfo.isDirectory() ) {
		dstInfo = FileInfo( dstInfo.getDirectoryPath() );
		if ( dstInfo.getFilepath() != dstInfo.getRealPath() )
			dstInfo = FileInfo( dstInfo.getRealPath() );
	}
	if ( srcInfo.getDirectoryPath() == dstInfo.getFilepath() )
		return;

	std::string srcPath( srcInfo.getFilepath() );
	std::string dstPath( dstInfo.getFilepath() );
	FileSystem::dirAddSlashAtEnd( dstPath );
	dstPath += srcInfo.getFileName();
	FileSystem::fileCopy( srcPath, dstPath );
}

void UITreeViewFS::copyFiles( const std::vector<std::string>& paths, const std::string& dstDir ) {
	for ( const auto& srcPath : paths ) {
		copyFile( srcPath, dstDir );
	}
}

std::string UITreeViewFS::getSelectionPath() const {
	return static_cast<const FileSystemModel*>( getModel() )
		->node( getSelection().first() )
		.fullPath();
}

std::string UITreeViewFS::getSelectionPathAtIndex( int index ) const {
	auto& selection = getSelection();
	if ( index < 0 || static_cast<size_t>( index ) >= static_cast<size_t>( selection.size() ) )
		return "";
	auto indexVec = selection.indexes();
	return static_cast<const FileSystemModel*>( getModel() )
		->node( indexVec[static_cast<size_t>( index )] )
		.fullPath();
}

std::vector<FileInfo> UITreeViewFS::getSelectionsFileInfo() const {
	std::vector<FileInfo> ret;
	auto indexVec = getSelection().indexes();
	auto model = static_cast<const FileSystemModel*>( getModel() );
	for ( const auto& index : indexVec ) {
		ret.emplace_back( FileInfo( model->node( index ).fullPath() ) );
	}
	return ret;
}

void UITreeViewFS::deleteSelectedFiles() {
	auto& selection = getSelection();
	if ( selection.isEmpty() )
		return;
	std::vector<std::string> paths;
	for ( int i = 0; i < selection.size(); ++i )
		paths.emplace_back( getSelectionPathAtIndex( i ) );
	deleteItems( paths );
}

void UITreeViewFS::deleteItems( const std::vector<std::string>& paths ) {
	if ( paths.empty() )
		return;
	auto fsm = static_cast<const FileSystemModel*>( getModel() );
	std::string confirmMsg;
	if ( paths.size() == 1 ) {
		std::string partialPath( paths[0] );
		FileSystem::filePathRemoveBasePath( fsm->getRootPath(), partialPath );
		confirmMsg = String::format(
			i18n( "confirm_delete_file_or_dir", "Are you sure you want to delete:\n%s" ).toUtf8(),
			partialPath );
	} else {
		confirmMsg = String::format(
			i18n( "confirm_delete_multiple", "Are you sure you want to delete %d items?" ).toUtf8(),
			paths.size() );
	}

	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK_CANCEL, confirmMsg );
	msgBox->setTitle( "ecode" );
	msgBox->center();
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [paths]( const Event* ) {
		for ( const auto& path : paths ) {
			FileInfo info( path );
			try {
				if ( info.isDirectory() ) {
					std::filesystem::remove_all( std::filesystem::path( path ) );
				} else {
					FileSystem::fileRemove( path );
				}
			} catch ( const std::filesystem::filesystem_error& ) {
			}
		}
	} );
}

void UITreeViewFS::openSelectedFiles() {
	auto selection = getSelection().indexes();
	std::vector<FileInfo> paths;
	paths.reserve( selection.size() );
	for ( size_t i = 0; i < selection.size(); ++i )
		paths.emplace_back( FileInfo( getSelectionPathAtIndex( i ) ) );
	for ( const auto& path : paths ) {
		if ( !path.isDirectory() )
			App::instance()->openFileFromPath( path.getFilepath() );
	}
}

void UITreeViewFS::execute( const std::string& cmd ) {
	auto cmdIt = mCommands.find( cmd );
	if ( cmdIt != mCommands.end() )
		return cmdIt->second();
}

} // namespace ecode
