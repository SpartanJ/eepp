#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <algorithm>

namespace EE { namespace UI {

#define CDLG_MIN_WIDTH 420
#define CDLG_MIN_HEIGHT 300

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

	if ( mMinWindowSize.getWidth() < CDLG_MIN_WIDTH )
		mMinWindowSize.setWidth( CDLG_MIN_WIDTH );

	if ( mMinWindowSize.getHeight() < CDLG_MIN_HEIGHT )
		mMinWindowSize.setHeight( CDLG_MIN_HEIGHT );

	if ( getAllowFolderSelect() ) {
		setTitle( "Select a folder" );
	} else {
		setTitle( "Select a file" );
	}

	UITextBox::CreateParams TxtBoxParams;
	TxtBoxParams.setParent( getContainer() );
	TxtBoxParams.setPosition( 6, 13 );
	TxtBoxParams.Flags |= UI_AUTO_SIZE;
	UITextBox * TBox = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox->setVisible( true );
	TBox->setEnabled( false );
	TBox->setText( "Look in:" );

	UIPushButton::CreateParams ButtonParams;
	ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE;
	ButtonParams.setParent( getContainer() );
	ButtonParams.setPosition( getContainer()->getSize().getWidth() - 86, getContainer()->getSize().getHeight() - 24 );
	ButtonParams.setSize( 80, 22 );
	mButtonCancel = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonCancel->setVisible( true );
	mButtonCancel->setEnabled( true );
	mButtonCancel->setText( "Cancel" );
	mButtonCancel->setPosition( Vector2i( mButtonCancel->getPosition().x, getContainer()->getSize().getHeight() - mButtonCancel->getSize().getHeight() - 2 ) );
	mButtonCancel->updateAnchorsDistances();

	ButtonParams.setPosition( mButtonCancel->getPosition().x, mButtonCancel->getPosition().y - mButtonCancel->getSize().getHeight() );
	mButtonOpen = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonOpen->setVisible( true );
	mButtonOpen->setEnabled( true );

	if ( isSaveDialog() )
		mButtonOpen->setText( "Save" );
	else
		mButtonOpen->setText( "Open" );

	UITextInput::CreateParams TInputParams;
	TInputParams.setParent( getContainer() );
	TInputParams.Flags = UI_AUTO_PADDING | UI_CLIP_ENABLE | UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_VALIGN_CENTER | UI_TEXT_SELECTION_ENABLED;
	TInputParams.setPosition( 70, 6 );
	TInputParams.setSize( getContainer()->getSize().getWidth() - TInputParams.Pos.x - 42, 22 );
	mPath = eeNew( UITextInput, ( TInputParams ) );
	mPath->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UICommonDialog::onPressEnter ) );
	mPath->setVisible( true );
	mPath->setEnabled( true );
	mPath->setText( mCurPath );

	ButtonParams.setPosition( TInputParams.Pos.x + TInputParams.Size.getWidth() + 6, TInputParams.Pos.y );
	ButtonParams.setSize( 24, 22 );
	ButtonParams.Flags |= UI_ANCHOR_TOP;
	mButtonUp = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonUp->setVisible( true );
	mButtonUp->setEnabled( true );
	mButtonUp->setText( "Up" );

	UIListBox::CreateParams LBParams;
	LBParams.setParent( getContainer() );
	LBParams.setPosition( 6, mButtonUp->getPosition().y + mButtonUp->getSize().getHeight() + 4 );
	LBParams.Size = Sizei( getContainer()->getSize().getWidth() - 12,
							getContainer()->getSize().getHeight() -
								mButtonUp->getSize().getHeight() -
								mButtonUp->getPosition().y -
								mButtonOpen->getSize().getHeight() -
								mButtonCancel->getSize().getHeight() -
								8
						);

	LBParams.Flags = UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_CLIP_ENABLE;
	LBParams.fontStyleConfig.fontSelectedColor = ColorA( 255, 255, 255, 255 );

	mList = eeNew( UIListBox, ( LBParams ) );
	mList->setVisible( true );
	mList->setEnabled( true );

	TxtBoxParams.setPosition( 6, getContainer()->getSize().getHeight() - 54 );
	TxtBoxParams.setSize( 74, 19 );
	TxtBoxParams.Flags = UI_ANCHOR_LEFT | UI_VALIGN_CENTER;
	TBox = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox->setVisible( true );
	TBox->setEnabled( false );
	TBox->setText( "File Name:" );

	TxtBoxParams.setPosition( TBox->getPosition().x, TBox->getPosition().y + TBox->getSize().getHeight()+ 6 );
	UITextBox * TBox2 = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox2->setVisible( true );
	TBox2->setEnabled( false );
	TBox2->setText( "Files of type:" );

	TInputParams.Flags &= ~UI_ANCHOR_TOP;
	TInputParams.setPosition( TBox->getPosition().x + TBox->getSize().getWidth(), TBox->getPosition().y );
	TInputParams.setSize( getContainer()->getSize().getWidth() - mButtonOpen->getSize().getWidth() - TInputParams.Pos.x - 20, TInputParams.Size.getHeight() );
	mFile = eeNew( UITextInput, ( TInputParams ) );
	mFile->setVisible( true );
	mFile->setEnabled( true );
	mFile->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UICommonDialog::onPressFileEnter ) );

	UIDropDownList::CreateParams DDLParams;
	DDLParams.setParent( getContainer() );
	DDLParams.setPosition( TBox2->getPosition().x + TBox2->getSize().getWidth(), TBox2->getPosition().y );
	DDLParams.setSize( getContainer()->getSize().getWidth() - mButtonCancel->getSize().getWidth() - DDLParams.Pos.x - 20, 22 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_AUTO_SIZE;
	DDLParams.PopUpToMainControl = true;
	mFiletype = eeNew( UIDropDownList, ( DDLParams ) );
	mFiletype->setVisible( true );
	mFiletype->setEnabled( true );
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
