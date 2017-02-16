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
	if ( mSize.width() < CDLG_MIN_WIDTH )
		mSize.x = CDLG_MIN_WIDTH;

	if ( mSize.height() < CDLG_MIN_HEIGHT )
		mSize.y = CDLG_MIN_HEIGHT;

	if ( mMinWindowSize.width() < CDLG_MIN_WIDTH )
		mMinWindowSize.width( CDLG_MIN_WIDTH );

	if ( mMinWindowSize.height() < CDLG_MIN_HEIGHT )
		mMinWindowSize.height( CDLG_MIN_HEIGHT );

	if ( allowFolderSelect() ) {
		title( "Select a folder" );
	} else {
		title( "Select a file" );
	}

	UITextBox::CreateParams TxtBoxParams;
	TxtBoxParams.Parent( getContainer() );
	TxtBoxParams.PosSet( 6, 13 );
	TxtBoxParams.Flags |= UI_AUTO_SIZE;
	UITextBox * TBox = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox->visible( true );
	TBox->enabled( false );
	TBox->text( "Look in:" );

	UIPushButton::CreateParams ButtonParams;
	ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE;
	ButtonParams.Parent( getContainer() );
	ButtonParams.PosSet( getContainer()->size().width() - 86, getContainer()->size().height() - 24 );
	ButtonParams.SizeSet( 80, 22 );
	mButtonCancel = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonCancel->visible( true );
	mButtonCancel->enabled( true );
	mButtonCancel->text( "Cancel" );
	mButtonCancel->position( Vector2i( mButtonCancel->position().x, getContainer()->size().height() - mButtonCancel->size().height() - 2 ) );
	mButtonCancel->updateAnchorsDistances();

	ButtonParams.PosSet( mButtonCancel->position().x, mButtonCancel->position().y - mButtonCancel->size().height() );
	mButtonOpen = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonOpen->visible( true );
	mButtonOpen->enabled( true );

	if ( isSaveDialog() )
		mButtonOpen->text( "Save" );
	else
		mButtonOpen->text( "Open" );

	UITextInput::CreateParams TInputParams;
	TInputParams.Parent( getContainer() );
	TInputParams.Flags = UI_AUTO_PADDING | UI_CLIP_ENABLE | UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_VALIGN_CENTER | UI_TEXT_SELECTION_ENABLED;
	TInputParams.PosSet( 70, 6 );
	TInputParams.SizeSet( getContainer()->size().width() - TInputParams.Pos.x - 42, 22 );
	mPath = eeNew( UITextInput, ( TInputParams ) );
	mPath->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UICommonDialog::onPressEnter ) );
	mPath->visible( true );
	mPath->enabled( true );
	mPath->text( mCurPath );

	ButtonParams.PosSet( TInputParams.Pos.x + TInputParams.Size.width() + 6, TInputParams.Pos.y );
	ButtonParams.SizeSet( 24, 22 );
	ButtonParams.Flags |= UI_ANCHOR_TOP;
	mButtonUp = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonUp->visible( true );
	mButtonUp->enabled( true );
	mButtonUp->text( "Up" );

	UIListBox::CreateParams LBParams;
	LBParams.Parent( getContainer() );
	LBParams.PosSet( 6, mButtonUp->position().y + mButtonUp->size().height() + 4 );
	LBParams.Size = Sizei( getContainer()->size().width() - 12,
							getContainer()->size().height() -
								mButtonUp->size().height() -
								mButtonUp->position().y -
								mButtonOpen->size().height() -
								mButtonCancel->size().height() -
								8
						);

	LBParams.Flags = UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_CLIP_ENABLE;
	LBParams.FontSelectedColor = ColorA( 255, 255, 255, 255 );

	if ( NULL != UIThemeManager::instance()->defaultTheme() ) {
		UITheme * Theme = UIThemeManager::instance()->defaultTheme();

		LBParams.FontSelectedColor = Theme->fontSelectedColor();
	}

	mList = eeNew( UIListBox, ( LBParams ) );
	mList->visible( true );
	mList->enabled( true );

	TxtBoxParams.PosSet( 6, getContainer()->size().height() - 54 );
	TxtBoxParams.SizeSet( 74, 19 );
	TxtBoxParams.Flags = UI_ANCHOR_LEFT | UI_VALIGN_CENTER;
	TBox = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox->visible( true );
	TBox->enabled( false );
	TBox->text( "File Name:" );

	TxtBoxParams.PosSet( TBox->position().x, TBox->position().y + TBox->size().height()+ 6 );
	UITextBox * TBox2 = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox2->visible( true );
	TBox2->enabled( false );
	TBox2->text( "Files of type:" );

	TInputParams.Flags &= ~UI_ANCHOR_TOP;
	TInputParams.PosSet( TBox->position().x + TBox->size().width(), TBox->position().y );
	TInputParams.SizeSet( getContainer()->size().width() - mButtonOpen->size().width() - TInputParams.Pos.x - 20, TInputParams.Size.height() );
	mFile = eeNew( UITextInput, ( TInputParams ) );
	mFile->visible( true );
	mFile->enabled( true );
	mFile->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UICommonDialog::onPressFileEnter ) );

	UIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( getContainer() );
	DDLParams.PosSet( TBox2->position().x + TBox2->size().width(), TBox2->position().y );
	DDLParams.SizeSet( getContainer()->size().width() - mButtonCancel->size().width() - DDLParams.Pos.x - 20, 22 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_AUTO_SIZE;
	DDLParams.PopUpToMainControl = true;
	mFiletype = eeNew( UIDropDownList, ( DDLParams ) );
	mFiletype->visible( true );
	mFiletype->enabled( true );
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
		mButtonUp->text( "" );
		mButtonUp->icon( Icon );
	}
}

void UICommonDialog::refreshFolder() {
	std::vector<String>			flist = FileSystem::filesGetInPath( String( mCurPath ) );
	std::vector<String>			files;
	std::vector<String>			folders;
	std::vector<std::string>	patterns;
	bool						accepted;
	Uint32 i, z;

	if ( "*" != mFiletype->text() ) {
		patterns = String::split( mFiletype->text().toUtf8(), ';' );

		for ( i = 0; i < patterns.size(); i++ )
			patterns[i] = FileSystem::fileExtension( patterns[i] );
	}

	for ( i = 0; i < flist.size(); i++ ) {
		if ( foldersFirst() && FileSystem::isDirectory( mCurPath + flist[i] ) ) {
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

	if ( sortAlphabetically() ) {
		std::sort( folders.begin(), folders.end() );
		std::sort( files.begin(), files.end() );
	}

	mList->clear();

	if ( foldersFirst() ) {
		mList->addListBoxItems( folders );
	}

	mList->addListBoxItems( files );

	if ( NULL != mList->verticalScrollBar() ) {
		mList->verticalScrollBar()->clickStep( 1.f / ( ( mList->count() * mList->rowHeight() ) / (Float)mList->size().height() ) );
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
	mButtonOpen->enabled( false );
	mButtonCancel->enabled( false );
	mButtonUp->enabled( false );

	if ( NULL != mButtonClose )
		mButtonClose->enabled( false );

	if ( NULL != mButtonMinimize )
		mButtonMinimize->enabled( false );

	if ( NULL != mButtonMaximize )
		mButtonMaximize->enabled( false );
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

					CloseWindow();
				} else if ( Msg->getSender() == mButtonUp ) {
					mCurPath = FileSystem::removeLastFolderFromPath( mCurPath );
					mPath->text( mCurPath );
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
						mPath->text( mCurPath );
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
					if ( allowFolderSelect() ) {
						mFile->text( mList->getItemSelectedText() );
					} else {
						if ( !FileSystem::isDirectory( getTempFullPath() ) ) {
							mFile->text( mList->getItemSelectedText() );
						}
					}
				} else {
					if ( !FileSystem::isDirectory( getTempFullPath() ) ) {
						mFile->text( mList->getItemSelectedText() );
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

	CloseWindow();
}

void UICommonDialog::open() {
	if ( "" != mList->getItemSelectedText() || allowFolderSelect() ) {
		if ( !allowFolderSelect() ) {
			if ( FileSystem::isDirectory( getFullPath() ) )
				return;
		} else {
			if ( !FileSystem::isDirectory( getFullPath() ) && !FileSystem::isDirectory( getCurPath() ) )
				return;
		}

		sendCommonEvent( UIEvent::EventOpenFile );

		disableButtons();

		CloseWindow();
	}
}

void UICommonDialog::onPressEnter( const UIEvent * Event ) {
	if ( FileSystem::isDirectory( mPath->text() ) ) {
		std::string tpath = mPath->text();
		FileSystem::dirPathAddSlashAtEnd( tpath );
		mPath->text( tpath );
		mCurPath = mPath->text();
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

bool UICommonDialog::sortAlphabetically() {
	return 0 != ( mCDLFlags & CDL_FLAG_SORT_ALPHABETICALLY );
}

bool UICommonDialog::foldersFirst() {
	return 0 != ( mCDLFlags & CDL_FLAG_FOLDERS_FISRT );
}

bool UICommonDialog::allowFolderSelect() {
	return 0 != ( mCDLFlags & CDL_FLAG_ALLOW_FOLDER_SELECT );
}

void UICommonDialog::sortAlphabetically( const bool& sortAlphabetically ) {
	BitOp::setBitFlagValue( &mCDLFlags, CDL_FLAG_SORT_ALPHABETICALLY, sortAlphabetically ? 1 : 0 );
	refreshFolder();
}

void UICommonDialog::foldersFirst( const bool& foldersFirst ) {
	BitOp::setBitFlagValue( &mCDLFlags, CDL_FLAG_FOLDERS_FISRT , foldersFirst ? 1 : 0 );
	refreshFolder();
}

void UICommonDialog::allowFolderSelect( const bool& allowFolderSelect ) {
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
		return mFile->text();

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
