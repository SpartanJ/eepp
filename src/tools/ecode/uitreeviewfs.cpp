#include "uitreeviewfs.hpp"
#include "customwidgets.hpp"
#include "notificationcenter.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/window/cursormanager.hpp>

namespace ecode {

static const std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings() {
	return {
		{ { KEY_C, KeyMod::getDefaultModifier() }, "copy" },
		{ { KEY_X, KeyMod::getDefaultModifier() }, "cut" },
		{ { KEY_V, KeyMod::getDefaultModifier() }, "paste" },
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
		getTreeView()->setSourceDrag( getModel()->node( getCurIndex() ).fullPath() );
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

		return 0;
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
		mSrcCopy = getSelectionPath();
		mWasCut = false;
		NotificationCenter::instance()->addNotification(
			String::format( i18n( "copied_file", "Copied '%s'" ).toUtf8(),
							FileSystem::fileNameFromPath( mSrcCopy ) ) );
	};

	mCommands["cut"] = [this] {
		if ( getSelection().isEmpty() )
			return;
		mSrcCopy = getSelectionPath();
		mWasCut = true;
		NotificationCenter::instance()->addNotification( String::format(
			i18n( "cut_file", "Cut '%s'" ).toUtf8(), FileSystem::fileNameFromPath( mSrcCopy ) ) );
	};

	mCommands["paste"] = [this] {
		if ( mWasCut )
			moveFile( mSrcCopy, getSelectionPath() );
	};

	mKeyBindings.addKeybinds( getDefaultKeybindings() );
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
			moveFile( mSrcDrag, dst );
			return 1;
		}
	}
	return UITreeView::onMessage( msg );
}

void UITreeViewFS::moveFile( const std::string& src, const std::string& dst ) {
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

	auto fsm = static_cast<const FileSystemModel*>( getModel() );
	std::string partialSrc( srcPath );
	FileSystem::filePathRemoveBasePath( fsm->getRootPath(), partialSrc );
	std::string partialDst( dstPath );
	FileSystem::filePathRemoveBasePath( fsm->getRootPath(), partialDst );

	auto confirmMsg( String::format(
		i18n( "confirm_move_file_or_dir", "Are you sure you want to move:\n%s\ninto:\n%s" )
			.toUtf8(),
		partialSrc, partialDst ) );

	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK_CANCEL, confirmMsg );
	msgBox->setTitle( "ecode" );
	msgBox->center();
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [srcPath = std::move( srcPath ), dstPath = std::move( dstPath )](
									  auto ) { FileSystem::fileMove( srcPath, dstPath ); } );
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

std::string UITreeViewFS::getSelectionPath() const {
	return static_cast<const FileSystemModel*>( getModel() )
		->node( getSelection().first() )
		.fullPath();
}

void UITreeViewFS::execute( const std::string& cmd ) {
	auto cmdIt = mCommands.find( cmd );
	if ( cmdIt != mCommands.end() )
		return cmdIt->second();
}

} // namespace ecode
