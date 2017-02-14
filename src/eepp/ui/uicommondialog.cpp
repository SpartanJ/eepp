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
	if ( mSize.Width() < CDLG_MIN_WIDTH )
		mSize.x = CDLG_MIN_WIDTH;

	if ( mSize.Height() < CDLG_MIN_HEIGHT )
		mSize.y = CDLG_MIN_HEIGHT;

	if ( mMinWindowSize.Width() < CDLG_MIN_WIDTH )
		mMinWindowSize.Width( CDLG_MIN_WIDTH );

	if ( mMinWindowSize.Height() < CDLG_MIN_HEIGHT )
		mMinWindowSize.Height( CDLG_MIN_HEIGHT );

	if ( AllowFolderSelect() ) {
		Title( "Select a folder" );
	} else {
		Title( "Select a file" );
	}

	UITextBox::CreateParams TxtBoxParams;
	TxtBoxParams.Parent( Container() );
	TxtBoxParams.PosSet( 6, 13 );
	TxtBoxParams.Flags |= UI_AUTO_SIZE;
	UITextBox * TBox = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox->Visible( true );
	TBox->Enabled( false );
	TBox->Text( "Look in:" );

	UIPushButton::CreateParams ButtonParams;
	ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE;
	ButtonParams.Parent( Container() );
	ButtonParams.PosSet( Container()->Size().Width() - 86, Container()->Size().Height() - 24 );
	ButtonParams.SizeSet( 80, 22 );
	mButtonCancel = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonCancel->Visible( true );
	mButtonCancel->Enabled( true );
	mButtonCancel->Text( "Cancel" );
	mButtonCancel->Pos( Vector2i( mButtonCancel->Pos().x, Container()->Size().Height() - mButtonCancel->Size().Height() - 2 ) );
	mButtonCancel->UpdateAnchorsDistances();

	ButtonParams.PosSet( mButtonCancel->Pos().x, mButtonCancel->Pos().y - mButtonCancel->Size().Height() );
	mButtonOpen = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonOpen->Visible( true );
	mButtonOpen->Enabled( true );

	if ( IsSaveDialog() )
		mButtonOpen->Text( "Save" );
	else
		mButtonOpen->Text( "Open" );

	UITextInput::CreateParams TInputParams;
	TInputParams.Parent( Container() );
	TInputParams.Flags = UI_AUTO_PADDING | UI_CLIP_ENABLE | UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_VALIGN_CENTER | UI_TEXT_SELECTION_ENABLED;
	TInputParams.PosSet( 70, 6 );
	TInputParams.SizeSet( Container()->Size().Width() - TInputParams.Pos.x - 42, 22 );
	mPath = eeNew( UITextInput, ( TInputParams ) );
	mPath->AddEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UICommonDialog::OnPressEnter ) );
	mPath->Visible( true );
	mPath->Enabled( true );
	mPath->Text( mCurPath );

	ButtonParams.PosSet( TInputParams.Pos.x + TInputParams.Size.Width() + 6, TInputParams.Pos.y );
	ButtonParams.SizeSet( 24, 22 );
	ButtonParams.Flags |= UI_ANCHOR_TOP;
	mButtonUp = eeNew( UIPushButton, ( ButtonParams ) );
	mButtonUp->Visible( true );
	mButtonUp->Enabled( true );
	mButtonUp->Text( "Up" );

	UIListBox::CreateParams LBParams;
	LBParams.Parent( Container() );
	LBParams.PosSet( 6, mButtonUp->Pos().y + mButtonUp->Size().Height() + 4 );
	LBParams.Size = Sizei( Container()->Size().Width() - 12,
							Container()->Size().Height() -
								mButtonUp->Size().Height() -
								mButtonUp->Pos().y -
								mButtonOpen->Size().Height() -
								mButtonCancel->Size().Height() -
								8
						);

	LBParams.Flags = UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_CLIP_ENABLE;
	LBParams.FontSelectedColor = ColorA( 255, 255, 255, 255 );

	if ( NULL != UIThemeManager::instance()->DefaultTheme() ) {
		UITheme * Theme = UIThemeManager::instance()->DefaultTheme();

		LBParams.FontSelectedColor = Theme->FontSelectedColor();
	}

	mList = eeNew( UIListBox, ( LBParams ) );
	mList->Visible( true );
	mList->Enabled( true );

	TxtBoxParams.PosSet( 6, Container()->Size().Height() - 54 );
	TxtBoxParams.SizeSet( 74, 19 );
	TxtBoxParams.Flags = UI_ANCHOR_LEFT | UI_VALIGN_CENTER;
	TBox = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox->Visible( true );
	TBox->Enabled( false );
	TBox->Text( "File Name:" );

	TxtBoxParams.PosSet( TBox->Pos().x, TBox->Pos().y + TBox->Size().Height()+ 6 );
	UITextBox * TBox2 = eeNew( UITextBox, ( TxtBoxParams ) );
	TBox2->Visible( true );
	TBox2->Enabled( false );
	TBox2->Text( "Files of type:" );

	TInputParams.Flags &= ~UI_ANCHOR_TOP;
	TInputParams.PosSet( TBox->Pos().x + TBox->Size().Width(), TBox->Pos().y );
	TInputParams.SizeSet( Container()->Size().Width() - mButtonOpen->Size().Width() - TInputParams.Pos.x - 20, TInputParams.Size.Height() );
	mFile = eeNew( UITextInput, ( TInputParams ) );
	mFile->Visible( true );
	mFile->Enabled( true );
	mFile->AddEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UICommonDialog::OnPressFileEnter ) );

	UIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( Container() );
	DDLParams.PosSet( TBox2->Pos().x + TBox2->Size().Width(), TBox2->Pos().y );
	DDLParams.SizeSet( Container()->Size().Width() - mButtonCancel->Size().Width() - DDLParams.Pos.x - 20, 22 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_AUTO_SIZE;
	DDLParams.PopUpToMainControl = true;
	mFiletype = eeNew( UIDropDownList, ( DDLParams ) );
	mFiletype->Visible( true );
	mFiletype->Enabled( true );
	mFiletype->ListBox()->AddListBoxItem( Params.DefaultFilePattern );
	mFiletype->ListBox()->SetSelected(0);

	ApplyDefaultTheme();

	RefreshFolder();
}

UICommonDialog::~UICommonDialog() {
}

Uint32 UICommonDialog::Type() const {
	return UI_TYPE_COMMONDIALOG;
}

bool UICommonDialog::IsType( const Uint32& type ) const {
	return UICommonDialog::Type() == type ? true : UIWindow::IsType( type );
}

void UICommonDialog::SetTheme( UITheme * Theme ) {
	UIWindow::SetTheme( Theme );

	SubTexture * Icon = Theme->GetIconByName( "go-up" );

	if ( NULL != Icon ) {
		mButtonUp->Text( "" );
		mButtonUp->Icon( Icon );
	}
}

void UICommonDialog::RefreshFolder() {
	std::vector<String>			flist = FileSystem::filesGetInPath( String( mCurPath ) );
	std::vector<String>			files;
	std::vector<String>			folders;
	std::vector<std::string>	patterns;
	bool						accepted;
	Uint32 i, z;

	if ( "*" != mFiletype->Text() ) {
		patterns = String::split( mFiletype->Text().toUtf8(), ';' );

		for ( i = 0; i < patterns.size(); i++ )
			patterns[i] = FileSystem::fileExtension( patterns[i] );
	}

	for ( i = 0; i < flist.size(); i++ ) {
		if ( FoldersFirst() && FileSystem::isDirectory( mCurPath + flist[i] ) ) {
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

	if ( SortAlphabetically() ) {
		std::sort( folders.begin(), folders.end() );
		std::sort( files.begin(), files.end() );
	}

	mList->Clear();

	if ( FoldersFirst() ) {
		mList->AddListBoxItems( folders );
	}

	mList->AddListBoxItems( files );

	if ( NULL != mList->VerticalScrollBar() ) {
		mList->VerticalScrollBar()->ClickStep( 1.f / ( ( mList->Count() * mList->RowHeight() ) / (Float)mList->Size().Height() ) );
	}
}

void UICommonDialog::OpenSaveClick() {
	if ( IsSaveDialog() ) {
		Save();
	} else {
		Open();
	}
}

void UICommonDialog::OnPressFileEnter( const UIEvent * Event ) {
	OpenSaveClick();
}

void UICommonDialog::DisableButtons() {
	mButtonOpen->Enabled( false );
	mButtonCancel->Enabled( false );
	mButtonUp->Enabled( false );

	if ( NULL != mButtonClose )
		mButtonClose->Enabled( false );

	if ( NULL != mButtonMinimize )
		mButtonMinimize->Enabled( false );

	if ( NULL != mButtonMaximize )
		mButtonMaximize->Enabled( false );
}

Uint32 UICommonDialog::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgClick:
		{
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				if ( Msg->Sender() == mButtonOpen ) {
					OpenSaveClick();
				} else if ( Msg->Sender() == mButtonCancel ) {
					DisableButtons();

					CloseWindow();
				} else if ( Msg->Sender() == mButtonUp ) {
					mCurPath = FileSystem::removeLastFolderFromPath( mCurPath );
					mPath->Text( mCurPath );
					RefreshFolder();
				}
			}

			break;
		}
		case UIMessage::MsgDoubleClick:
		{
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				if ( Msg->Sender()->IsType( UI_TYPE_LISTBOXITEM ) ) {
					std::string newPath = mCurPath + mList->GetItemSelectedText();

					if ( FileSystem::isDirectory( newPath ) ) {
						mCurPath = newPath + FileSystem::getOSlash();
						mPath->Text( mCurPath );
						RefreshFolder();
					} else {
						Open();
					}
				}
			}

			break;
		}
		case UIMessage::MsgSelected:
		{
			if ( Msg->Sender() == mList ) {
				if ( !IsSaveDialog() ) {
					if ( AllowFolderSelect() ) {
						mFile->Text( mList->GetItemSelectedText() );
					} else {
						if ( !FileSystem::isDirectory( GetTempFullPath() ) ) {
							mFile->Text( mList->GetItemSelectedText() );
						}
					}
				} else {
					if ( !FileSystem::isDirectory( GetTempFullPath() ) ) {
						mFile->Text( mList->GetItemSelectedText() );
					}
				}
			} else if ( Msg->Sender() == mFiletype ) {
				RefreshFolder();
			}

			break;
		}
	}

	return UIWindow::OnMessage( Msg );
}

void UICommonDialog::Save() {
	SendCommonEvent( UIEvent::EventSaveFile );

	DisableButtons();

	CloseWindow();
}

void UICommonDialog::Open() {
	if ( "" != mList->GetItemSelectedText() || AllowFolderSelect() ) {
		if ( !AllowFolderSelect() ) {
			if ( FileSystem::isDirectory( GetFullPath() ) )
				return;
		} else {
			if ( !FileSystem::isDirectory( GetFullPath() ) && !FileSystem::isDirectory( GetCurPath() ) )
				return;
		}

		SendCommonEvent( UIEvent::EventOpenFile );

		DisableButtons();

		CloseWindow();
	}
}

void UICommonDialog::OnPressEnter( const UIEvent * Event ) {
	if ( FileSystem::isDirectory( mPath->Text() ) ) {
		std::string tpath = mPath->Text();
		FileSystem::dirPathAddSlashAtEnd( tpath );
		mPath->Text( tpath );
		mCurPath = mPath->Text();
		RefreshFolder();
	}
}

void UICommonDialog::AddFilePattern( std::string pattern, bool select ) {
	Uint32 index = mFiletype->ListBox()->AddListBoxItem( pattern );

	if ( select ) {
		mFiletype->ListBox()->SetSelected( index );

		RefreshFolder();
	}
}

bool UICommonDialog::IsSaveDialog() {
	return 0 != ( mCDLFlags & CDL_FLAG_SAVE_DIALOG );
}

bool UICommonDialog::SortAlphabetically() {
	return 0 != ( mCDLFlags & CDL_FLAG_SORT_ALPHABETICALLY );
}

bool UICommonDialog::FoldersFirst() {
	return 0 != ( mCDLFlags & CDL_FLAG_FOLDERS_FISRT );
}

bool UICommonDialog::AllowFolderSelect() {
	return 0 != ( mCDLFlags & CDL_FLAG_ALLOW_FOLDER_SELECT );
}

void UICommonDialog::SortAlphabetically( const bool& sortAlphabetically ) {
	BitOp::setBitFlagValue( &mCDLFlags, CDL_FLAG_SORT_ALPHABETICALLY, sortAlphabetically ? 1 : 0 );
	RefreshFolder();
}

void UICommonDialog::FoldersFirst( const bool& foldersFirst ) {
	BitOp::setBitFlagValue( &mCDLFlags, CDL_FLAG_FOLDERS_FISRT , foldersFirst ? 1 : 0 );
	RefreshFolder();
}

void UICommonDialog::AllowFolderSelect( const bool& allowFolderSelect ) {
	BitOp::setBitFlagValue( &mCDLFlags, CDL_FLAG_ALLOW_FOLDER_SELECT, allowFolderSelect ? 1 : 0 );
}

std::string UICommonDialog::GetFullPath() {
	std::string tPath = mCurPath;

	FileSystem::dirPathAddSlashAtEnd( tPath );

	tPath += GetCurFile();

	return tPath;
}

std::string	UICommonDialog::GetTempFullPath() {
	std::string tPath = mCurPath;

	FileSystem::dirPathAddSlashAtEnd( tPath );

	tPath += mList->GetItemSelectedText().toUtf8();

	return tPath;
}

std::string UICommonDialog::GetCurPath() const {
	return mCurPath;
}

std::string UICommonDialog::GetCurFile() const {
	if ( mCDLFlags & CDL_FLAG_SAVE_DIALOG )
		return mFile->Text();

	return mList->GetItemSelectedText().toUtf8();
}

UIPushButton *	UICommonDialog::GetButtonOpen() const {
	return mButtonOpen;
}

UIPushButton *	UICommonDialog::GetButtonCancel() const {
	return mButtonCancel;
}

UIPushButton *	UICommonDialog::GetButtonUp() const {
	return mButtonUp;
}

UIListBox * UICommonDialog::GetList() const {
	return mList;
}

UITextInput * UICommonDialog::GetPathInput() const {
	return mPath;
}

UITextInput * UICommonDialog::GetFileInput() const {
	return mFile;
}

UIDropDownList * UICommonDialog::GetFiletypeList() const {
	return mFiletype;
}

}}
