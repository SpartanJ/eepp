#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/models/sortingproxymodel.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

#define FDLG_MIN_WIDTH 640
#define FDLG_MIN_HEIGHT 400
#define FDLG_DRIVE_PATH "drives://"

UIFileDialog* UIFileDialog::New( Uint32 dialogFlags, const std::string& defaultFilePattern,
								 const std::string& defaultDirectory ) {
	return eeNew( UIFileDialog, ( dialogFlags, defaultFilePattern, defaultDirectory ) );
}

UIFileDialog::UIFileDialog( Uint32 dialogFlags, const std::string& defaultFilePattern,
							const std::string& defaultDirectory ) :
	UIWindow(),
	mCurPath( FileSystem::getRealPath( defaultDirectory ) ),
	mDialogFlags( dialogFlags ),
	mCloseShortcut( KEY_ESCAPE ) {
	if ( getSize().getWidth() < FDLG_MIN_WIDTH ) {
		mDpSize.x = FDLG_MIN_WIDTH;
		mSize.x = PixelDensity::dpToPxI( FDLG_MIN_WIDTH );
	}

	if ( getSize().getHeight() < FDLG_MIN_HEIGHT ) {
		mDpSize.y = FDLG_MIN_HEIGHT;
		mSize.y = PixelDensity::dpToPxI( FDLG_MIN_HEIGHT );
	}

	if ( mStyleConfig.MinWindowSize.getWidth() < FDLG_MIN_WIDTH )
		mStyleConfig.MinWindowSize.setWidth( FDLG_MIN_WIDTH );

	if ( mStyleConfig.MinWindowSize.getHeight() < FDLG_MIN_HEIGHT )
		mStyleConfig.MinWindowSize.setHeight( FDLG_MIN_HEIGHT );

	bool loading = isSceneNodeLoading();
	mUISceneNode->setIsLoading( true );

	mContainer->setSize( getSize() );

	if ( allowFolderSelect() ) {
		setTitle( i18n( "uifiledialog_select_folder", "Select a folder" ) );
	} else {
		setTitle( i18n( "uifiledialog_select_file", "Select a file" ) );
	}

	UILinearLayout* linearLayout = UILinearLayout::NewVertical();
	linearLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent )
		->setLayoutMargin( Rectf( 4, 2, 4, 2 ) )
		->setParent( getContainer() );

	UILinearLayout* hLayout = UILinearLayout::NewHorizontal();
	hLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setLayoutMargin( Rectf( 0, 0, 0, 4 ) )
		->setParent( linearLayout )
		->setId( "lay1" );

	UITextView::New()
		->setText( i18n( "uifiledialog_look_in", "Look in:" ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent )
		->setLayoutMargin( Rectf( 0, 0, 4, 0 ) )
		->setParent( hLayout )
		->setEnabled( false );

	FileSystem::dirAddSlashAtEnd( mCurPath );

	mPath = UITextInput::New();
	mPath->setText( mCurPath )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutWeight( 1 )
		->setParent( hLayout );
	mPath->on( Event::OnPressEnter, [this]( auto event ) { onPressEnter( event ); } );

	mButtonUp = UIPushButton::New();
	mButtonUp->setText( i18n( "uifiledialog_go_up", "Up" ) )
		->setLayoutMarginLeft( 4 )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent )
		->setParent( hLayout );

	mButtonNewFolder = UIPushButton::New();
	mButtonNewFolder->setText( i18n( "uifiledialog_new_folder", "New Folder" ) )
		->setLayoutMarginLeft( 4 )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent )
		->setParent( hLayout );
	mButtonNewFolder->on( Event::MouseClick, [this]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, i18n( "uifiledialog_enter_new_folder_name",
															  "Enter new folder name:" ) );
			msgBox->setTitle( i18n( "uifiledialog_create_new_folder", "Create new folder" ) );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->show();
			msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
				auto folderName( msgBox->getTextInput()->getText() );
				auto newFolderPath( getCurPath() + folderName );
				if ( !FileSystem::fileExists( newFolderPath ) &&
					 FileSystem::makeDir( newFolderPath ) ) {
					refreshFolder();

					ModelIndex index =
						mMultiView->getCurrentView()->findRowWithText( folderName, true, true );
					if ( index.isValid() )
						mMultiView->setSelection( index );
				}

				msgBox->closeWindow();
			} );
		}
	} );

	mButtonListView = UISelectButton::New();
	mButtonListView->setText( i18n( "uifiledialog_list", "List" ) )
		->setLayoutMarginLeft( 4 )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent )
		->setParent( hLayout );
	mButtonListView->on( Event::MouseClick, [&]( const Event* event ) {
		const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
		if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
			setViewMode( UIMultiModelView::ViewMode::List );
	} );

	mButtonTableView = UISelectButton::New();
	mButtonTableView->setText( i18n( "uifiledialog_table", "Table" ) )
		->setLayoutMarginLeft( 4 )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent )
		->setParent( hLayout );
	mButtonTableView->on( Event::MouseClick, [&]( const Event* event ) {
		const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
		if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
			setViewMode( UIMultiModelView::ViewMode::Table );
	} );

	mMultiView = UIMultiModelView::New();
	mMultiView->setParent( linearLayout );
	mMultiView->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setLayoutWeight( 1 )
		->setLayoutMargin( Rectf( 0, 0, 0, 4 ) );
	mMultiView->on( Event::KeyDown, [this]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_BACKSPACE )
			goFolderUp();
	} );
	mMultiView->on( Event::OnModelEvent, [&]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vPath(
				modelEvent->getModel()->data( modelEvent->getModelIndex(), ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.isString() ) {
				bool shouldOpenFolder = false;
				if ( allowFolderSelect() && modelEvent->getTriggerEvent() &&
					 modelEvent->getTriggerEvent()->getType() == Event::EventType::KeyDown ) {
					const KeyEvent* keyEvent =
						static_cast<const KeyEvent*>( modelEvent->getTriggerEvent() );
					if ( keyEvent->getMod() & KeyMod::getDefaultModifier() ) {
						shouldOpenFolder = true;
					}
				}
				openFileOrFolder( shouldOpenFolder );
			}
		}
	} );
	mMultiView->setOnSelectionChange( [this] {
		if ( mMultiView->getSelection().isEmpty() || mDisplayingDrives ||
			 ( isSaveDialog() && allowMultiFileSelect() ) )
			return;
		auto nodes = getSelectionNodes();
		if ( nodes.empty() ) {
			Log::error( "UIFileDialog() - mMultiView->setOnSelectionChange - "
						"UIFileDialog::getSelectionNode() was empty, shouldn't "
						"be empty" );
			return;
		}
		if ( nodes.size() == 1 ) {
			const FileSystemModel::Node* node = nodes[0];
			if ( !isSaveDialog() ) {
				if ( allowFolderSelect() || !FileSystem::isDirectory( node->fullPath() ) )
					setFileName( node->getName() );
			} else if ( !FileSystem::isDirectory( node->fullPath() ) ) {
				setFileName( node->getName() );
			}
		} else {
			std::string names;
			for ( size_t i = 0; i < nodes.size(); i++ ) {
				auto node = nodes[i];
				names += node->getName() + ( ( i != nodes.size() - 1 ) ? "; " : "" );
			}
			setFileName( names );
		}
	} );

	hLayout = UILinearLayout::NewHorizontal();
	hLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setLayoutMargin( Rectf( 0, 0, 0, 4 ) )
		->setParent( linearLayout );

	UITextView::New()
		->setText( i18n( "uifiledialog_file_name", "File Name:" ) )
		->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::MatchParent )
		->setSize( 74, 0 )
		->setParent( hLayout )
		->setEnabled( false );

	mFile = UITextInput::New();
	mFile->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent )
		->setLayoutWeight( 1 )
		->setParent( hLayout );
	mFile->setLayoutMargin( Rectf( 0, 0, 4, 0 ) );
	mFile->on( Event::OnPressEnter, [this]( auto event ) { onPressFileEnter( event ); } );
	mFile->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* kevent = event->asKeyEvent();
		if ( kevent->getKeyCode() == KEY_UP || kevent->getKeyCode() == KEY_DOWN ) {
			mMultiView->getCurrentView()->setFocus();
			mMultiView->getCurrentView()->forceKeyDown( *kevent );
		}
	} );

	mButtonOpen = UIPushButton::New();
	mButtonOpen
		->setText( isSaveDialog() ? i18n( "uifiledialog_save", "Save" )
								  : i18n( "uifiledialog_open", "Open" ) )
		->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent )
		->setSize( 80, 0 )
		->setParent( hLayout );

	hLayout = UILinearLayout::NewHorizontal();
	hLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( linearLayout );

	UITextView::New()
		->setText( i18n( "uifiledialog_files_of_type", "Files of type:" ) )
		->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::MatchParent )
		->setSize( 74, 0 )
		->setParent( hLayout )
		->setEnabled( false );

	mFiletype = UIDropDownList::New();
	mFiletype->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutWeight( 1 )
		->setParent( hLayout );
	mFiletype->setPopUpToRoot( true );
	mFiletype->getListBox()->addListBoxItem( defaultFilePattern );
	mFiletype->getListBox()->setSelected( 0 );
	mFiletype->setLayoutMargin( Rectf( 0, 0, 4, 0 ) );

	mButtonCancel = UIPushButton::New();
	mButtonCancel->setText( i18n( "uifiledialog_cancel", "Cancel" ) )
		->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent )
		->setSize( 80, 0 )
		->setParent( hLayout );

	mMultiView->getCurrentView()->setFocus();
	mMultiView->getListView()->setColumnsVisible( { FileSystemModel::Name } );
	mMultiView->getTableView()->setColumnsVisible(
		{ FileSystemModel::Name, FileSystemModel::Size, FileSystemModel::ModificationTime } );
	mMultiView->getTableView()->setAutoColumnsWidth( true );
	mMultiView->getTableView()->setMainColumn( FileSystemModel::Name );
	setViewMode( UIMultiModelView::ViewMode::List );

	applyDefaultTheme();

	mUISceneNode->setIsLoading( loading );
}

UIFileDialog::~UIFileDialog() {}

void UIFileDialog::onWindowReady() {
	updateClickStep();
	if ( isSaveDialog() ) {
		mFile->getDocument().selectAll();
		mFile->setFocus();
	}
	UIWindow::onWindowReady();
}

Uint32 UIFileDialog::getType() const {
	return UI_TYPE_FILEDIALOG;
}

bool UIFileDialog::isType( const Uint32& type ) const {
	return UIFileDialog::getType() == type ? true : UIWindow::isType( type );
}

void UIFileDialog::setTheme( UITheme* Theme ) {
	UIWindow::setTheme( Theme );

	mButtonOpen->setTheme( Theme );
	mButtonCancel->setTheme( Theme );
	mButtonUp->setTheme( Theme );
	mMultiView->setTheme( Theme );
	mPath->setTheme( Theme );
	mFile->setTheme( Theme );
	mFiletype->setTheme( Theme );

	Drawable* icon = getUISceneNode()->findIconDrawable( "go-up", PixelDensity::dpToPxI( 16 ) );
	if ( icon ) {
		mButtonUp->setText( "" );
		mButtonUp->setIcon( icon );
	}

	icon = getUISceneNode()->findIconDrawable( "folder-add", PixelDensity::dpToPxI( 16 ) );
	if ( icon ) {
		mButtonNewFolder->setText( "" );
		mButtonNewFolder->setIcon( icon );
	}

	icon = getUISceneNode()->findIconDrawable( "list-view", PixelDensity::dpToPxI( 16 ) );
	if ( icon ) {
		mButtonListView->setText( "" );
		mButtonListView->setIcon( icon );
	}

	icon = getUISceneNode()->findIconDrawable( "table-view", PixelDensity::dpToPxI( 16 ) );
	if ( icon ) {
		mButtonTableView->setText( "" );
		mButtonTableView->setIcon( icon );
	}

	onThemeLoaded();
}

void UIFileDialog::refreshFolder( bool resetScroll ) {
	FileSystem::dirAddSlashAtEnd( mCurPath );

	if ( mCurPath == FDLG_DRIVE_PATH ) {
		if ( !mDiskDrivesModel )
			mDiskDrivesModel = DiskDrivesModel::create();

		mDisplayingDrives = true;
		mMultiView->getTableView()->setColumnsVisible( { FileSystemModel::Name } );
		mMultiView->setModel( SortingProxyModel::New( mDiskDrivesModel ) );
	} else {
		std::vector<String> flist = FileSystem::filesGetInPath(
			String( mCurPath ), getSortAlphabetically(), getFoldersFirst(), !getShowHidden() );
		std::vector<String> files;
		std::vector<std::string> patterns;

		if ( "*" != mFiletype->getText() ) {
			patterns = String::split( mFiletype->getText().toUtf8(), ';' );

			for ( size_t i = 0; i < patterns.size(); i++ )
				patterns[i] = FileSystem::fileExtension( String::trim( patterns[i] ) );
		}

		if ( !mModel ) {
			mModel = FileSystemModel::New(
				mCurPath,
				getShowOnlyFolders() ? FileSystemModel::Mode::DirectoriesOnly
									 : FileSystemModel::Mode::FilesAndDirectories,
				FileSystemModel::DisplayConfig( getSortAlphabetically(), getFoldersFirst(),
												!getShowHidden(), patterns ),
				&getUISceneNode()->getTranslator() );
		} else {
			mModel->setRootPath( mCurPath );
		}

		mMultiView->setModel( SortingProxyModel::New( mModel ) );

		mMultiView->getTableView()->setColumnsVisible(
			{ FileSystemModel::Name, FileSystemModel::Size, FileSystemModel::ModificationTime } );

		mDisplayingDrives = false;
	}

	updateClickStep();

	if ( resetScroll )
		mMultiView->getCurrentView()->scrollToTop();

	mMultiView->getCurrentView()->setFocus();
}

void UIFileDialog::updateClickStep() {
	if ( mMultiView->getListView()->getVerticalScrollBar() ) {
		mMultiView->getListView()->getVerticalScrollBar()->setClickStep(
			1.f /
			( ( mMultiView->getModel()->rowCount() * mMultiView->getListView()->getRowHeight() ) /
			  (Float)mMultiView->getListView()->getSize().getHeight() ) );
	}
	if ( mMultiView->getTableView()->getVerticalScrollBar() ) {
		Float step =
			1.f /
			( ( mMultiView->getModel()->rowCount() * mMultiView->getTableView()->getRowHeight() ) /
			  (Float)mMultiView->getTableView()->getSize().getHeight() );
		mMultiView->getTableView()->getVerticalScrollBar()->setClickStep(
			step > 0 ? step : mMultiView->getListView()->getVerticalScrollBar()->getClickStep() );
	}
}

void UIFileDialog::setCurPath( const std::string& path ) {
	mCurPath = path;
	FileSystem::dirAddSlashAtEnd( mCurPath );
	mPath->setText( mCurPath );
	if ( !isSaveDialog() )
		mFile->setText( "" );
	refreshFolder( true );
}

std::vector<ModelIndex> UIFileDialog::getSelectionModelIndex() const {
	if ( mMultiView->getSelection().isEmpty() )
		return {};
	std::vector<ModelIndex> indexes;
	mMultiView->getSelection().forEachIndex( [this, &indexes]( const ModelIndex& index ) {
		auto* filterModel = (SortingProxyModel*)mMultiView->getModel().get();
		auto localIndex = filterModel->mapToSource( index );
		indexes.emplace_back( std::move( localIndex ) );
	} );
	return indexes;
}

std::vector<const FileSystemModel::Node*> UIFileDialog::getSelectionNodes() const {
	if ( mMultiView->getSelection().isEmpty() || mDisplayingDrives )
		return {};
	auto localIndexes = getSelectionModelIndex();
	std::vector<const FileSystemModel::Node*> nodes;
	nodes.reserve( localIndexes.size() );
	for ( const auto& localIndex : localIndexes ) {
		const FileSystemModel::Node& node = mModel->node( localIndex );
		nodes.push_back( &node );
	}
	return nodes;
}

void UIFileDialog::openSaveClick() {
	if ( isSaveDialog() ) {
		save();
	} else {
		open();
	}
}

void UIFileDialog::onPressFileEnter( const Event* ) {
	openSaveClick();
}

void UIFileDialog::disableButtons() {
	mButtonOpen->setEnabled( false );
	mButtonCancel->setEnabled( false );
	mButtonUp->setEnabled( false );

	if ( NULL != mButtonClose )
		mButtonClose->setEnabled( false );

	if ( NULL != mButtonMinimize )
		mButtonMinimize->setEnabled( false );

	if ( NULL != mButtonMaximize )
		mButtonMaximize->setEnabled( false );

	mOpenShortut = {};
	mCloseShortcut = {};
}

std::string UIFileDialog::getSelectedDrive() const {
	if ( !mDisplayingDrives )
		return "";
	auto indexes = getSelectionModelIndex();
	if ( indexes.empty() )
		return "";
	ModelIndex index = indexes[0];
	ModelIndex modelIndex( mDiskDrivesModel->index( index.row(), DiskDrivesModel::Name ) );
	Variant var( mDiskDrivesModel->data( modelIndex ) );
	return var.toString();
}

void UIFileDialog::openFileOrFolder( bool shouldOpenFolder = false ) {
	if ( mMultiView->getSelection().isEmpty() )
		return;

	if ( mDisplayingDrives ) {
		setCurPath( getSelectedDrive() );
		return;
	}

	auto nodes = getSelectionNodes();
	if ( nodes.empty() )
		return;
	auto* node = nodes[0];
	if ( !node ) {
		Log::error( "UIFileDialog::getSelectionNode() was empty, shouldn't be empty" );
		return;
	}
	std::string newPath = mCurPath + node->getName();

	if ( FileSystem::isDirectory( newPath ) ) {
		if ( shouldOpenFolder ) {
			open();
		} else {
			setCurPath( newPath );
		}
	} else {
		open();
	}
}

void UIFileDialog::goFolderUp() {
	if ( mCurPath == FDLG_DRIVE_PATH )
		return;
	std::string prevFolderName( FileSystem::fileNameFromPath( mCurPath ) );
	std::string newPath( FileSystem::removeLastFolderFromPath( mCurPath ) );
	if ( newPath == mCurPath ) {
		auto drives = Sys::getLogicalDrives();
		if ( !drives.empty() ) {
			setCurPath( newPath != mCurPath ? newPath : FDLG_DRIVE_PATH );
		} else {
			setCurPath( newPath );
		}
	} else {
		setCurPath( newPath );
	}
	ModelIndex index = mMultiView->getCurrentView()->findRowWithText( prevFolderName, true, true );
	if ( index.isValid() )
		mMultiView->setSelection( index );
}

Uint32 UIFileDialog::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseClick: {
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mButtonOpen ) {
					openSaveClick();
				} else if ( Msg->getSender() == mButtonCancel ) {
					disableButtons();

					closeWindow();
				} else if ( Msg->getSender() == mButtonUp ) {
					goFolderUp();
				}
			}

			break;
		}
		case NodeMessage::MouseDoubleClick: {
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender()->isType( UI_TYPE_LISTBOXITEM ) ) {
					openFileOrFolder();
				}
			}

			break;
		}
		case NodeMessage::Selected: {
			if ( Msg->getSender() == mFiletype ) {
				refreshFolder();
			}

			break;
		}
	}

	return UIWindow::onMessage( Msg );
}

void UIFileDialog::save() {
	sendCommonEvent( Event::SaveFile );

	disableButtons();

	closeWindow();
}

void UIFileDialog::open() {
	auto fullPath( getFullPath() );

	if ( mMultiView->getSelection().isEmpty() &&
		 !( allowFolderSelect() && FileSystem::isDirectory( fullPath ) ) &&
		 !FileSystem::fileExists( fullPath ) )
		return;

	if ( mDisplayingDrives ) {
		if ( !allowFolderSelect() )
			return;
		if ( FileSystem::isDirectory( fullPath ) )
			return;
	}

	auto nodes = getSelectionNodes();
	auto* node = !nodes.empty() ? nodes[0] : nullptr;
	if ( !node ) {
		node = mModel->getNodeFromPath( fullPath );

		if ( !node ) {
			Log::warning( "UIFileDialog::open() - Should contain a valid path." );
			return;
		}
	}

	if ( ( node && "" != node->getName() ) || allowFolderSelect() ) {
		if ( !allowFolderSelect() ) {
			if ( FileSystem::isDirectory( fullPath ) )
				return;
		} else {
			if ( !FileSystem::isDirectory( fullPath ) && !FileSystem::isDirectory( getCurPath() ) )
				return;
		}

		sendCommonEvent( Event::OpenFile );

		disableButtons();

		closeWindow();
	}
}

void UIFileDialog::onPressEnter( const Event* ) {
	if ( FileSystem::isDirectory( mPath->getText() ) ||
		 ( FDLG_DRIVE_PATH == mPath->getText().toUtf8() && !Sys::getLogicalDrives().empty() ) ) {
		setCurPath( mPath->getText() );
	} else if ( !allowFolderSelect() && FileSystem::fileExists( mPath->getText() ) ) {
		String folderPath( FileSystem::fileRemoveFileName( mPath->getText() ) );
		String fileName( FileSystem::fileNameFromPath( mPath->getText() ) );
		if ( FileSystem::isDirectory( folderPath ) ) {
			setCurPath( folderPath );
			setFileName( fileName );
			auto index = mMultiView->getCurrentView()->findRowWithText( fileName, true, true );
			if ( index.isValid() )
				mMultiView->setSelection( index );
		}
	}
}

void UIFileDialog::addFilePattern( std::string pattern, bool select ) {
	Uint32 index = mFiletype->getListBox()->addListBoxItem( pattern );

	if ( select ) {
		mFiletype->getListBox()->setSelected( index );

		refreshFolder();
	}
}

bool UIFileDialog::isSaveDialog() const {
	return 0 != ( mDialogFlags & SaveDialog );
}

bool UIFileDialog::getSortAlphabetically() const {
	return 0 != ( mDialogFlags & SortAlphabetically );
}

bool UIFileDialog::getFoldersFirst() const {
	return 0 != ( mDialogFlags & FoldersFirst );
}

bool UIFileDialog::allowFolderSelect() const {
	return 0 != ( mDialogFlags & AllowFolderSelect );
}

bool UIFileDialog::getShowOnlyFolders() const {
	return 0 != ( mDialogFlags & ShowOnlyFolders );
}

bool UIFileDialog::getShowHidden() const {
	return 0 != ( mDialogFlags & ShowHidden );
}

bool UIFileDialog::allowMultiFileSelect() const {
	return 0 != ( mDialogFlags & AllowMultiFileSelection );
}

void UIFileDialog::setSortAlphabetically( const bool& sortAlphabetically ) {
	BitOp::setBitFlagValue( &mDialogFlags, SortAlphabetically, sortAlphabetically ? 1 : 0 );
	refreshFolder();
}

void UIFileDialog::setFoldersFirst( const bool& foldersFirst ) {
	BitOp::setBitFlagValue( &mDialogFlags, FoldersFirst, foldersFirst ? 1 : 0 );
	refreshFolder();
}

void UIFileDialog::setAllowFolderSelect( const bool& allowFolderSelect ) {
	BitOp::setBitFlagValue( &mDialogFlags, AllowFolderSelect, allowFolderSelect ? 1 : 0 );
}

void UIFileDialog::setShowOnlyFolders( const bool& showOnlyFolders ) {
	BitOp::setBitFlagValue( &mDialogFlags, ShowOnlyFolders, showOnlyFolders ? 1 : 0 );
	refreshFolder();
}

void UIFileDialog::setShowHidden( const bool& showHidden ) {
	BitOp::setBitFlagValue( &mDialogFlags, ShowHidden, showHidden ? 1 : 0 );
	refreshFolder();
}

void UIFileDialog::setAllowsMultiFileSelect( bool allow ) {
	BitOp::setBitFlagValue( &mDialogFlags, AllowMultiFileSelection, allow ? 1 : 0 );
	mMultiView->setMultiSelect( allow );
	mFile->setEnabled( !allow );
}

std::string UIFileDialog::getFullPath( size_t index ) const {
	if ( mDisplayingDrives )
		return getCurFile();

	std::string tPath = mCurPath;
	FileSystem::dirAddSlashAtEnd( tPath );
	tPath += getCurFile( index );
	return tPath;
}

std::string UIFileDialog::getFullPath() const {
	return getFullPath( 0 );
}

std::vector<std::string> UIFileDialog::getFullPaths() const {
	if ( mDisplayingDrives )
		return { getCurFile() };

	std::vector<std::string> paths;
	auto nodes = getSelectionNodes();

	for ( size_t i = 0; i < nodes.size(); i++ ) {
		std::string tPath( mCurPath );
		FileSystem::dirAddSlashAtEnd( tPath );
		tPath += nodes[i]->getName();
		paths.emplace_back( std::move( tPath ) );
	}

	return paths;
}

std::string UIFileDialog::getCurPath() const {
	return mCurPath;
}

std::string UIFileDialog::getCurFile( size_t index ) const {
	if ( mDialogFlags & SaveDialog )
		return mFile->getText();
	if ( mMultiView->getSelection().isEmpty() )
		return "";
	if ( mDisplayingDrives ) {
		return getSelectedDrive();
	} else {
		auto nodes = getSelectionNodes();
		if ( nodes.empty() || index >= nodes.size() )
			return "";
		auto* node = nodes[index];
		if ( !node ) {
			Log::error( "UIFileDialog::getCurFile() - UIFileDialog::getSelectionNode() was empty, "
						"shouldn't be empty" );
			return "";
		}
		return node->getName();
	}
}

UIPushButton* UIFileDialog::getButtonOpen() const {
	return mButtonOpen;
}

UIPushButton* UIFileDialog::getButtonCancel() const {
	return mButtonCancel;
}

UIPushButton* UIFileDialog::getButtonUp() const {
	return mButtonUp;
}

UIMultiModelView* UIFileDialog::getMultiView() const {
	return mMultiView;
}

UITextInput* UIFileDialog::getPathInput() const {
	return mPath;
}

UITextInput* UIFileDialog::getFileInput() const {
	return mFile;
}

UIDropDownList* UIFileDialog::getFileTypeList() const {
	return mFiletype;
}

Uint32 UIFileDialog::onKeyUp( const KeyEvent& event ) {
	if ( mCloseShortcut && event.getKeyCode() == mCloseShortcut.key &&
		 ( mCloseShortcut.mod == 0 || ( event.getMod() & mCloseShortcut.mod ) ) ) {
		disableButtons();

		closeWindow();
	}

	return UIWindow::onKeyUp( event );
}

Uint32 UIFileDialog::onKeyDown( const KeyEvent& event ) {
	if ( mOpenShortut && event.getKeyCode() == mOpenShortut.key &&
		 ( mOpenShortut.mod == 0 || ( event.getMod() & mOpenShortut.mod ) ) ) {
		open();
	}

	return UIWindow::onKeyDown( event );
}

const KeyBindings::Shortcut& UIFileDialog::getCloseShortcut() const {
	return mCloseShortcut;
}

void UIFileDialog::setFileName( const std::string& name ) {
	if ( mFile )
		mFile->setText( name );
}

void UIFileDialog::setCloseShortcut( const KeyBindings::Shortcut& closeWithKey ) {
	mCloseShortcut = closeWithKey;
}

void UIFileDialog::setViewMode( const UIMultiModelView::ViewMode& viewMode ) {
	mMultiView->setViewMode( viewMode );
	switch ( viewMode ) {
		case UIMultiModelView::ViewMode::Table:
			mButtonTableView->select();
			mButtonListView->unselect();
			break;
		case UIMultiModelView::ViewMode::List:
		default:
			mButtonTableView->unselect();
			mButtonListView->select();
			break;
	}
}

const UIMultiModelView::ViewMode& UIFileDialog::getViewMode() const {
	return mMultiView->getViewMode();
}

const KeyBindings::Shortcut& UIFileDialog::openShortut() const {
	return mOpenShortut;
}

void UIFileDialog::setOpenShortut( const KeyBindings::Shortcut& newOpenShortut ) {
	mOpenShortut = newOpenShortut;
}

void UIFileDialog::setSingleClickNavigation( bool singleClickNavigation ) {
	mMultiView->setSingleClickNavigation( singleClickNavigation );
}

}} // namespace EE::UI
