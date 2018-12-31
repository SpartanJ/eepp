#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/system/filesystem.hpp>
#include <algorithm>

namespace EE { namespace UI {

#define CDLG_MIN_WIDTH 420
#define CDLG_MIN_HEIGHT 320

UICommonDialog * UICommonDialog::New(Uint32 CDLFlags, std::string DefaultFilePattern, std::string DefaultDirectory) {
	return eeNew( UICommonDialog, ( CDLFlags, DefaultFilePattern, DefaultDirectory ) );
}

UICommonDialog::UICommonDialog( Uint32 CDLFlags , std::string DefaultFilePattern, std::string DefaultDirectory ) :
	UIWindow(),
	mCurPath( DefaultDirectory ),
	mCDLFlags( CDLFlags )
{
	if ( mDpSize.getWidth() < CDLG_MIN_WIDTH ) {
		mDpSize.x = CDLG_MIN_WIDTH;
		mSize.x = PixelDensity::dpToPxI( CDLG_MIN_WIDTH );
	}

	if ( mDpSize.getHeight() < CDLG_MIN_HEIGHT ) {
		mDpSize.y = CDLG_MIN_HEIGHT;
		mSize.y = PixelDensity::dpToPxI( CDLG_MIN_HEIGHT );
	}

	if ( mStyleConfig.MinWindowSize.getWidth() < CDLG_MIN_WIDTH )
		mStyleConfig.MinWindowSize.setWidth( CDLG_MIN_WIDTH );

	if ( mStyleConfig.MinWindowSize.getHeight() < CDLG_MIN_HEIGHT )
		mStyleConfig.MinWindowSize.setHeight( CDLG_MIN_HEIGHT );

	mContainer->setSize( mDpSize );

	if ( getAllowFolderSelect() ) {
		setTitle( "Select a folder" );
	} else {
		setTitle( "Select a file" );
	}

	UILinearLayout * linearLayout = UILinearLayout::NewVertical();
	linearLayout->setLayoutSizeRules( MATCH_PARENT, MATCH_PARENT )->setLayoutMargin( Rect( 4, 2, 4, 2 ) )->setParent( getContainer() );

	UILinearLayout * hLayout = UILinearLayout::NewHorizontal();
	hLayout->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setLayoutMargin( Rect(0,0,0,4) )->setParent( linearLayout );

	UITextView::New()->setText( "Look in:" )->setLayoutSizeRules( WRAP_CONTENT, MATCH_PARENT )->setLayoutMargin( Rect( 0, 0, 4, 0 ) )
			->setParent( hLayout )->setEnabled( false );

	mPath = UITextInput::New();
	mPath->setText( mCurPath )->setLayoutSizeRules( WRAP_CONTENT, MATCH_PARENT )->setLayoutWeight( 1 )->setParent( hLayout );
	mPath->addEventListener( Event::OnPressEnter, cb::Make1( this, &UICommonDialog::onPressEnter ) );

	mButtonUp = UIPushButton::New();
	mButtonUp->setText( "Up" )->setLayoutSizeRules( WRAP_CONTENT, MATCH_PARENT )->setParent( hLayout );

	mList = UIListBox::New();
	mList->setParent( linearLayout );
	mList->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setLayoutWeight( 1 )->setLayoutMargin( Rect(0,0,0,4) );

	hLayout = UILinearLayout::NewHorizontal();
	hLayout->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setLayoutMargin( Rect(0,0,0,4) )->setParent( linearLayout );

	UITextView::New()->setText( "File Name:" )->setLayoutSizeRules( FIXED, MATCH_PARENT )->setSize(74,0)->setParent( hLayout )->setEnabled( false );

	mFile = UITextInput::New();
	mFile->setLayoutSizeRules( WRAP_CONTENT, MATCH_PARENT )->setLayoutWeight( 1 )->setParent( hLayout );
	mFile->setLayoutMargin( Rect( 0, 0, 4, 0 ) );
	mFile->addEventListener( Event::OnPressEnter, cb::Make1( this, &UICommonDialog::onPressFileEnter ) );

	mButtonOpen = UIPushButton::New();
	mButtonOpen->setText( isSaveDialog() ? "Save" : "Open" )->setLayoutSizeRules( FIXED, WRAP_CONTENT )->setSize(80,0)->setParent( hLayout );

	hLayout = UILinearLayout::NewHorizontal();
	hLayout->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( linearLayout );

	UITextView::New()->setText( "Files of type:" )->setLayoutSizeRules( FIXED, MATCH_PARENT )->setSize(74,0)->setParent( hLayout )->setEnabled( false );

	mFiletype = UIDropDownList::New();
	mFiletype->setLayoutSizeRules( WRAP_CONTENT, MATCH_PARENT )->setLayoutWeight( 1 )->setParent( hLayout );
	mFiletype->setPopUpToMainControl( true );
	mFiletype->getListBox()->addListBoxItem( DefaultFilePattern );
	mFiletype->getListBox()->setSelected(0);
	mFiletype->setLayoutMargin( Rect( 0, 0, 4, 0 ) );

	mButtonCancel = UIPushButton::New();
	mButtonCancel->setText( "Cancel" )->setLayoutSizeRules( FIXED, WRAP_CONTENT )->setSize(80,0)->setParent( hLayout );

	applyDefaultTheme();

	refreshFolder();
}

UICommonDialog::~UICommonDialog() {
}

Uint32 UICommonDialog::getType() const {
	return UI_TYPE_COMMONDIALOG;
}

bool UICommonDialog::isType( const Uint32& type ) const {
	return UICommonDialog::getType() == type ? true : UIWindow::isType( type );
}

void UICommonDialog::setTheme( UITheme * Theme ) {
	UIWindow::setTheme( Theme );

	mButtonOpen->setTheme( Theme );
	mButtonCancel->setTheme( Theme );
	mButtonUp->setTheme( Theme );
	mList->setTheme( Theme );
	mPath->setTheme( Theme );
	mFile->setTheme( Theme );
	mFiletype->setTheme( Theme );

	Drawable * Icon = Theme->getIconByName( "go-up" );

	if ( NULL != Icon ) {
		mButtonUp->setText( "" );
		mButtonUp->setIcon( Icon );
	}

	onThemeLoaded();
}

void UICommonDialog::refreshFolder() {
	std::vector<String>			flist = FileSystem::filesGetInPath( String( mCurPath ) );
	std::vector<String>			files;
	std::vector<String>			folders;
	std::vector<std::string>	patterns;
	bool						accepted;
	Uint32 i, z;

	if ( "*" != mFiletype->getText() ) {
		patterns = String::split( mFiletype->getText().toUtf8(), ';' );

		for ( i = 0; i < patterns.size(); i++ )
			patterns[i] = FileSystem::fileExtension( patterns[i] );
	}

	for ( i = 0; i < flist.size(); i++ ) {
		if ( getFoldersFirst() && FileSystem::isDirectory( mCurPath + flist[i] ) ) {
			folders.push_back( flist[i] );
		} else {
			accepted = false;

			if ( patterns.size() ) {
				for ( z = 0; z < patterns.size(); z++ ) {
					if ( patterns[z] == FileSystem::fileExtension( flist[i] ) ) {
						accepted = true;
						break;
					}
				}
			} else {
				accepted = true;
			}

			if ( accepted )
				files.push_back( flist[i] );
		}
	}

	if ( getSortAlphabetically() ) {
		std::sort( folders.begin(), folders.end() );
		std::sort( files.begin(), files.end() );
	}

	mList->clear();

	if ( getFoldersFirst() ) {
		mList->addListBoxItems( folders );
	}

	mList->addListBoxItems( files );

	if ( NULL != mList->getVerticalScrollBar() ) {
		mList->getVerticalScrollBar()->setClickStep( 1.f / ( ( mList->getCount() * mList->getRowHeight() ) / (Float)mList->getSize().getHeight() ) );
	}
}

void UICommonDialog::openSaveClick() {
	if ( isSaveDialog() ) {
		save();
	} else {
		open();
	}
}

void UICommonDialog::onPressFileEnter( const Event * ) {
	openSaveClick();
}

void UICommonDialog::disableButtons() {
	mButtonOpen->setEnabled( false );
	mButtonCancel->setEnabled( false );
	mButtonUp->setEnabled( false );

	if ( NULL != mButtonClose )
		mButtonClose->setEnabled( false );

	if ( NULL != mButtonMinimize )
		mButtonMinimize->setEnabled( false );

	if ( NULL != mButtonMaximize )
		mButtonMaximize->setEnabled( false );
}

Uint32 UICommonDialog::onMessage( const NodeMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Click:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mButtonOpen ) {
					openSaveClick();
				} else if ( Msg->getSender() == mButtonCancel ) {
					disableButtons();

					closeWindow();
				} else if ( Msg->getSender() == mButtonUp ) {
					mCurPath = FileSystem::removeLastFolderFromPath( mCurPath );
					mPath->setText( mCurPath );
					refreshFolder();
				}
			}

			break;
		}
		case NodeMessage::DoubleClick:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender()->isType( UI_TYPE_LISTBOXITEM ) ) {
					std::string newPath = mCurPath + mList->getItemSelectedText();

					if ( FileSystem::isDirectory( newPath ) ) {
						mCurPath = newPath + FileSystem::getOSSlash();
						mPath->setText( mCurPath );
						refreshFolder();
					} else {
						open();
					}
				}
			}

			break;
		}
		case NodeMessage::Selected:
		{
			if ( Msg->getSender() == mList ) {
				if ( !isSaveDialog() ) {
					if ( getAllowFolderSelect() ) {
						mFile->setText( mList->getItemSelectedText() );
					} else {
						if ( !FileSystem::isDirectory( getTempFullPath() ) ) {
							mFile->setText( mList->getItemSelectedText() );
						}
					}
				} else {
					if ( !FileSystem::isDirectory( getTempFullPath() ) ) {
						mFile->setText( mList->getItemSelectedText() );
					}
				}
			} else if ( Msg->getSender() == mFiletype ) {
				refreshFolder();
			}

			break;
		}
	}

	return UIWindow::onMessage( Msg );
}

void UICommonDialog::save() {
	sendCommonEvent( Event::SaveFile );

	disableButtons();

	closeWindow();
}

void UICommonDialog::open() {
	if ( "" != mList->getItemSelectedText() || getAllowFolderSelect() ) {
		if ( !getAllowFolderSelect() ) {
			if ( FileSystem::isDirectory( getFullPath() ) )
				return;
		} else {
			if ( !FileSystem::isDirectory( getFullPath() ) && !FileSystem::isDirectory( getCurPath() ) )
				return;
		}

		sendCommonEvent( Event::OpenFile );

		disableButtons();

		closeWindow();
	}
}

void UICommonDialog::onPressEnter( const Event * ) {
	if ( FileSystem::isDirectory( mPath->getText() ) ) {
		std::string tpath = mPath->getText();
		FileSystem::dirPathAddSlashAtEnd( tpath );
		mPath->setText( tpath );
		mCurPath = mPath->getText();
		refreshFolder();
	}
}

void UICommonDialog::addFilePattern( std::string pattern, bool select ) {
	Uint32 index = mFiletype->getListBox()->addListBoxItem( pattern );

	if ( select ) {
		mFiletype->getListBox()->setSelected( index );

		refreshFolder();
	}
}

bool UICommonDialog::isSaveDialog() {
	return 0 != ( mCDLFlags & CDL_FLAG_SAVE_DIALOG );
}

bool UICommonDialog::getSortAlphabetically() {
	return 0 != ( mCDLFlags & CDL_FLAG_SORT_ALPHABETICALLY );
}

bool UICommonDialog::getFoldersFirst() {
	return 0 != ( mCDLFlags & CDL_FLAG_FOLDERS_FISRT );
}

bool UICommonDialog::getAllowFolderSelect() {
	return 0 != ( mCDLFlags & CDL_FLAG_ALLOW_FOLDER_SELECT );
}

void UICommonDialog::setSortAlphabetically( const bool& sortAlphabetically ) {
	BitOp::setBitFlagValue( &mCDLFlags, CDL_FLAG_SORT_ALPHABETICALLY, sortAlphabetically ? 1 : 0 );
	refreshFolder();
}

void UICommonDialog::setFoldersFirst( const bool& foldersFirst ) {
	BitOp::setBitFlagValue( &mCDLFlags, CDL_FLAG_FOLDERS_FISRT , foldersFirst ? 1 : 0 );
	refreshFolder();
}

void UICommonDialog::setAllowFolderSelect( const bool& allowFolderSelect ) {
	BitOp::setBitFlagValue( &mCDLFlags, CDL_FLAG_ALLOW_FOLDER_SELECT, allowFolderSelect ? 1 : 0 );
}

std::string UICommonDialog::getFullPath() {
	std::string tPath = mCurPath;

	FileSystem::dirPathAddSlashAtEnd( tPath );

	tPath += getCurFile();

	return tPath;
}

std::string	UICommonDialog::getTempFullPath() {
	std::string tPath = mCurPath;

	FileSystem::dirPathAddSlashAtEnd( tPath );

	tPath += mList->getItemSelectedText().toUtf8();

	return tPath;
}

std::string UICommonDialog::getCurPath() const {
	return mCurPath;
}

std::string UICommonDialog::getCurFile() const {
	if ( mCDLFlags & CDL_FLAG_SAVE_DIALOG )
		return mFile->getText();

	return mList->getItemSelectedText().toUtf8();
}

UIPushButton *	UICommonDialog::getButtonOpen() const {
	return mButtonOpen;
}

UIPushButton *	UICommonDialog::getButtonCancel() const {
	return mButtonCancel;
}

UIPushButton *	UICommonDialog::getButtonUp() const {
	return mButtonUp;
}

UIListBox * UICommonDialog::getList() const {
	return mList;
}

UITextInput * UICommonDialog::getPathInput() const {
	return mPath;
}

UITextInput * UICommonDialog::getFileInput() const {
	return mFile;
}

UIDropDownList * UICommonDialog::getFiletypeList() const {
	return mFiletype;
}

}}
