#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <algorithm>

namespace EE { namespace UI {

#define CDLG_MIN_WIDTH 420
#define CDLG_MIN_HEIGHT 320

UICommonDialog::UICommonDialog( const UICommonDialog::CreateParams& Params ) :
	UIWindow( Params ),
	mCurPath( Params.DefaultDirectory ),
	mCDLFlags( Params.CDLFlags )
{
	if ( mSize.getWidth() < CDLG_MIN_WIDTH ) {
		mSize.x = CDLG_MIN_WIDTH;
		mRealSize.x = PixelDensity::dpToPxI( CDLG_MIN_WIDTH );
	}

	if ( mSize.getHeight() < CDLG_MIN_HEIGHT ) {
		mSize.y = CDLG_MIN_HEIGHT;
		mRealSize.y = PixelDensity::dpToPxI( CDLG_MIN_HEIGHT );
	}

	if ( mStyleConfig.MinWindowSize.getWidth() < CDLG_MIN_WIDTH )
		mStyleConfig.MinWindowSize.setWidth( CDLG_MIN_WIDTH );

	if ( mStyleConfig.MinWindowSize.getHeight() < CDLG_MIN_HEIGHT )
		mStyleConfig.MinWindowSize.setHeight( CDLG_MIN_HEIGHT );

	mContainer->setSize( mSize );

	if ( getAllowFolderSelect() ) {
		setTitle( "Select a folder" );
	} else {
		setTitle( "Select a file" );
	}

	UITextBox * lookIn = UITextBox::New();
	lookIn->setParent( getContainer() );
	lookIn->setPosition( 6, 13 );
	lookIn->setVisible( true );
	lookIn->setEnabled( false );
	lookIn->setText( "Look in:" );

	mButtonCancel = UIPushButton::New();
	mButtonCancel->setParent( getContainer() );
	mButtonCancel->setSize( 80, 0 );
	mButtonCancel->setPosition( getContainer()->getSize().getWidth() - 86, getContainer()->getSize().getHeight() - mButtonCancel->getSize().getHeight() - 2 );
	mButtonCancel->setText( "Cancel" );
	mButtonCancel->setAnchors( UI_ANCHOR_RIGHT );

	mButtonOpen = UIPushButton::New();
	mButtonOpen->setParent( getContainer() );
	mButtonOpen->setSize( 80, 0 );
	mButtonOpen->setPosition( mButtonCancel->getPosition().x, mButtonCancel->getPosition().y - mButtonCancel->getSize().getHeight() - 4 );
	mButtonOpen->setAnchors( UI_ANCHOR_RIGHT );

	if ( isSaveDialog() )
		mButtonOpen->setText( "Save" );
	else
		mButtonOpen->setText( "Open" );

	mPath = UITextInput::New();
	mPath->setParent( getContainer() );
	mPath->setPosition( 70, 6 );
	mPath->setSize( getContainer()->getSize().getWidth() - mPath->getPosition().x - 42, 22 );
	mPath->setFlags( UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP );
	mPath->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UICommonDialog::onPressEnter ) );
	mPath->setText( mCurPath );

	mButtonUp = UIPushButton::New();
	mButtonUp->setParent( getContainer() );
	mButtonUp->setSize( 24, 0 );
	mButtonUp->setPosition( mPath->getPosition().x + mPath->getSize().getWidth() + 6, mPath->getPosition().y );
	mButtonUp->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mButtonUp->setText( "Up" );

	mList = UIListBox::New();
	mList->setParent( getContainer() );
	mList->setPosition( 6, mButtonUp->getPosition().y + mButtonUp->getSize().getHeight() + 4 );
	mList->setSize( getContainer()->getSize().getWidth() - 12,
					getContainer()->getSize().getHeight() -
						mButtonUp->getSize().getHeight() -
						mButtonUp->getPosition().y -
						mButtonOpen->getSize().getHeight() -
						mButtonCancel->getSize().getHeight() -
						12
	);
	mList->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM );

	UITextBox * fileName = UITextBox::New();
	fileName->setParent( getContainer() );
	fileName->setPosition( 6, mButtonOpen->getPosition().y );
	fileName->setSize( 74, fileName->getTextHeight() );
	fileName->setAnchors( UI_ANCHOR_LEFT );
	fileName->setVerticalAlign( UI_VALIGN_TOP );
	fileName->setEnabled( false );
	fileName->setText( "File Name:" );

	UITextBox * fileTypes = UITextBox::New();
	fileTypes->setParent( getContainer() );
	fileTypes->setSize( 74, fileTypes->getTextHeight() );
	fileTypes->setPosition( fileName->getPosition().x, mButtonCancel->getPosition().y );
	fileTypes->setAnchors( UI_ANCHOR_LEFT );
	fileTypes->setVerticalAlign( UI_VALIGN_TOP );
	fileTypes->setEnabled( false );
	fileTypes->setText( "Files of type:" );

	mFile = UITextInput::New();
	mFile->setParent( getContainer() );
	mFile->setPosition( fileName->getPosition().x + fileName->getSize().getWidth(), fileName->getPosition().y );
	mFile->setSize( getContainer()->getSize().getWidth() - mButtonOpen->getSize().getWidth() - mPath->getPosition().x - 20, mButtonOpen->getSize().getHeight() );
	mFile->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT );
	mFile->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UICommonDialog::onPressFileEnter ) );

	mFiletype = UIDropDownList::New();
	mFiletype->setParent( getContainer() );
	mFiletype->setSize( mFile->getSize().getWidth(), 0 );
	mFiletype->setPosition( fileTypes->getPosition().x + fileTypes->getSize().getWidth(), fileTypes->getPosition().y );
	mFiletype->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT );
	mFiletype->setPopUpToMainControl( true );
	mFiletype->getListBox()->addListBoxItem( Params.DefaultFilePattern );
	mFiletype->getListBox()->setSelected(0);

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

	SubTexture * Icon = Theme->getIconByName( "go-up" );

	if ( NULL != Icon ) {
		mButtonUp->setText( "" );
		mButtonUp->setIcon( Icon );
	}
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

void UICommonDialog::onPressFileEnter( const UIEvent * Event ) {
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

Uint32 UICommonDialog::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgClick:
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
		case UIMessage::MsgDoubleClick:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender()->isType( UI_TYPE_LISTBOXITEM ) ) {
					std::string newPath = mCurPath + mList->getItemSelectedText();

					if ( FileSystem::isDirectory( newPath ) ) {
						mCurPath = newPath + FileSystem::getOSlash();
						mPath->setText( mCurPath );
						refreshFolder();
					} else {
						open();
					}
				}
			}

			break;
		}
		case UIMessage::MsgSelected:
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
	sendCommonEvent( UIEvent::EventSaveFile );

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

		sendCommonEvent( UIEvent::EventOpenFile );

		disableButtons();

		closeWindow();
	}
}

void UICommonDialog::onPressEnter( const UIEvent * Event ) {
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
