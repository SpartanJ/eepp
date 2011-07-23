#include "cuicommondialog.hpp"
#include "cuimanager.hpp"
#include "cuilistboxitem.hpp"

namespace EE { namespace UI {

#define CDLG_MIN_WIDTH 420
#define CDLG_MIN_HEIGHT 267

cUICommonDialog::cUICommonDialog( const cUICommonDialog::CreateParams& Params ) :
	cUIWindow( Params ),
	mCurPath( Params.DefaultDirectory ),
	mCDLFlags( Params.CDLFlags )
{
	mType = UI_TYPE_COMMONDIALOG;

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

	cUITextBox::CreateParams TxtBoxParams;
	TxtBoxParams.Parent( Container() );
	TxtBoxParams.PosSet( 6, 13 );
	TxtBoxParams.Flags |= UI_AUTO_SIZE;
	cUITextBox * TBox = eeNew( cUITextBox, ( TxtBoxParams ) );
	TBox->Visible( true );
	TBox->Enabled( false );
	TBox->Text( "Look in:" );

	cUIPushButton::CreateParams ButtonParams;
	ButtonParams.Parent( Container() );
	ButtonParams.PosSet( Container()->Size().Width() - 86, Container()->Size().Height() - 54 );
	ButtonParams.SizeSet( 80, 22 );
	ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE;
	mButtonOpen = eeNew( cUIPushButton, ( ButtonParams ) );
	mButtonOpen->Visible( true );
	mButtonOpen->Enabled( true );

	if ( IsSaveDialog() )
		mButtonOpen->Text( "Save" );
	else
		mButtonOpen->Text( "Open" );

	ButtonParams.Pos.y = mButtonOpen->Pos().y + mButtonOpen->Size().Height() + 6;
	mButtonCancel = eeNew( cUIPushButton, ( ButtonParams ) );
	mButtonCancel->Visible( true );
	mButtonCancel->Enabled( true );
	mButtonCancel->Text( "Cancel" );

	cUITextInput::CreateParams TInputParams;
	TInputParams.Parent( Container() );
	TInputParams.Flags = UI_AUTO_PADDING | UI_CLIP_ENABLE | UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_VALIGN_CENTER;
	TInputParams.PosSet( 70, 6 );
	TInputParams.SizeSet( Container()->Size().Width() - TInputParams.Pos.x - 42, 22 );
	mPath = eeNew( cUITextInput, ( TInputParams ) );
	mPath->AddEventListener( cUIEvent::EventOnPressEnter, cb::Make1( this, &cUICommonDialog::OnPressEnter ) );
	mPath->Visible( true );
	mPath->Enabled( true );
	mPath->Text( mCurPath );

	ButtonParams.PosSet( TInputParams.Pos.x + TInputParams.Size.Width() + 6, TInputParams.Pos.y );
	ButtonParams.SizeSet( 24, 22 );
	ButtonParams.Flags |= UI_ANCHOR_TOP;
	mButtonUp = eeNew( cUIPushButton, ( ButtonParams ) );
	mButtonUp->Visible( true );
	mButtonUp->Enabled( true );
	mButtonUp->Text( "Up" );

	cUIListBox::CreateParams LBParams;
	LBParams.Parent( Container() );
	LBParams.PosSet( 6, 33 );
	LBParams.Size = eeSize( Container()->Size().Width() - 12, Container()->Size().Height() - 92 );
	LBParams.Flags = UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_CLIP_ENABLE;
	LBParams.FontSelectedColor = eeColorA( 255, 255, 255, 255 );
	mList = eeNew( cUIListBox, ( LBParams ) );
	mList->Visible( true );
	mList->Enabled( true );

	TxtBoxParams.PosSet( 6, Container()->Size().Height() - 54 );
	TxtBoxParams.SizeSet( 74, 19 );
	TxtBoxParams.Flags = UI_ANCHOR_LEFT | UI_VALIGN_CENTER;
	TBox = eeNew( cUITextBox, ( TxtBoxParams ) );
	TBox->Visible( true );
	TBox->Enabled( false );
	TBox->Text( "File Name:" );

	TxtBoxParams.PosSet( TBox->Pos().x, TBox->Pos().y + TBox->Size().Height()+ 6 );
	cUITextBox * TBox2 = eeNew( cUITextBox, ( TxtBoxParams ) );
	TBox2->Visible( true );
	TBox2->Enabled( false );
	TBox2->Text( "Files of type:" );

	TInputParams.Flags &= ~UI_ANCHOR_TOP;
	TInputParams.PosSet( TBox->Pos().x + TBox->Size().Width(), TBox->Pos().y );
	TInputParams.SizeSet( Container()->Size().Width() - mButtonOpen->Size().Width() - TInputParams.Pos.x - 20, TInputParams.Size.Height() );
	mFile = eeNew( cUITextInput, ( TInputParams ) );
	mFile->Visible( true );
	mFile->Enabled( true );
	mFile->AddEventListener( cUIEvent::EventOnPressEnter, cb::Make1( this, &cUICommonDialog::OnPressFileEnter ) );

	cUIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( Container() );
	DDLParams.PosSet( TBox2->Pos().x + TBox2->Size().Width(), TBox2->Pos().y );
	DDLParams.SizeSet( Container()->Size().Width() - mButtonCancel->Size().Width() - DDLParams.Pos.x - 20, 22 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT;
	DDLParams.PopUpToMainControl = true;
	mFiletype = eeNew( cUIDropDownList, ( DDLParams ) );
	mFiletype->Visible( true );
	mFiletype->Enabled( true );
	mFiletype->ListBox()->AddListBoxItem( Params.DefaultFilePattern );
	mFiletype->ListBox()->SetSelected(0);

	ApplyDefaultTheme();

	RefreshFolder();
}

cUICommonDialog::~cUICommonDialog() {
}

void cUICommonDialog::SetTheme( cUITheme * Theme ) {
	cUIWindow::SetTheme( Theme );

	cShape * Icon = Theme->GetIconByName( "go-up" );

	if ( NULL != Icon ) {
		mButtonUp->Text( "" );
		mButtonUp->Icon( Icon );
	}
}

void cUICommonDialog::RefreshFolder() {
	std::vector<String>			flist = FilesGetInPath( String( mCurPath ) );
	std::vector<String>			files;
	std::vector<String>			folders;
	std::vector<std::string>	patterns;
	bool						accepted;
	Uint32 i, z;

	if ( "*" != mFiletype->Text() ) {
		patterns = SplitString( mFiletype->Text().ToUtf8(), ';' );

		for ( i = 0; i < patterns.size(); i++ )
			patterns[i] = FileExtension( patterns[i] );
	}

	for ( i = 0; i < flist.size(); i++ ) {
		if ( FoldersFirst() && IsDirectory( mCurPath + flist[i] ) ) {
			folders.push_back( flist[i] );
		} else {
			accepted = false;

			if ( patterns.size() ) {
				for ( z = 0; z < patterns.size(); z++ ) {
					if ( patterns[z] == FileExtension( flist[i] ) ) {
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
}

void cUICommonDialog::OpenSaveClick() {
	if ( IsSaveDialog() ) {
		Save();
	} else {
		Open();
	}
}

void cUICommonDialog::OnPressFileEnter( const cUIEvent * Event ) {
	OpenSaveClick();
}

Uint32 cUICommonDialog::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick:
		{
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				if ( Msg->Sender() == mButtonOpen ) {
					OpenSaveClick();
				} else if ( Msg->Sender() == mButtonCancel ) {
					CloseWindow();
				} else if ( Msg->Sender() == mButtonUp ) {
					mCurPath = RemoveLastFolderFromPath( mCurPath );
					mPath->Text( mCurPath );
					RefreshFolder();
				}
			}

			break;
		}
		case cUIMessage::MsgDoubleClick:
		{
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				if ( Msg->Sender()->IsTypeOrInheritsFrom( UI_TYPE_LISTBOXITEM ) ) {
					std::string newPath = mCurPath + mList->GetItemSelectedText();

					if ( IsDirectory( newPath ) ) {
						mCurPath = newPath + GetOSlash();
						mPath->Text( mCurPath );
						RefreshFolder();
					} else {
						Open();
					}
				}
			}

			break;
		}
		case cUIMessage::MsgSelected:
		{
			if ( Msg->Sender() == mList ) {
				if ( !IsSaveDialog() ) {
					if ( AllowFolderSelect() ) {
						mFile->Text( mList->GetItemSelectedText() );
					} else {
						if ( !IsDirectory( GetTempFullPath() ) ) {
							mFile->Text( mList->GetItemSelectedText() );
						}
					}
				} else {
					if ( !IsDirectory( GetTempFullPath() ) ) {
						mFile->Text( mList->GetItemSelectedText() );
					}
				}
			} else if ( Msg->Sender() == mFiletype ) {
				RefreshFolder();
			}

			break;
		}
	}

	return cUIWindow::OnMessage( Msg );
}

void cUICommonDialog::Save() {
	SendCommonEvent( cUIEvent::EventSaveFile );

	CloseWindow();
}

void cUICommonDialog::Open() {
	if ( "" != mList->GetItemSelectedText() ) {
		if ( !AllowFolderSelect() ) {
			if ( IsDirectory( GetFullPath() ) )
				return;
		} else {
			if ( !IsDirectory( GetFullPath() ) )
				return;
		}

		SendCommonEvent( cUIEvent::EventOpenFile );

		CloseWindow();
	}
}

void cUICommonDialog::OnPressEnter( const cUIEvent * Event ) {
	if ( IsDirectory( mPath->Text() ) ) {
		std::string tpath = mPath->Text();
		DirPathAddSlashAtEnd( tpath );
		mPath->Text( tpath );
		mCurPath = mPath->Text();
		RefreshFolder();
	}
}

void cUICommonDialog::AddFilePattern( std::string pattern, bool select ) {
	Uint32 index = mFiletype->ListBox()->AddListBoxItem( pattern );

	if ( select ) {
		mFiletype->ListBox()->SetSelected( index );

		RefreshFolder();
	}
}

bool cUICommonDialog::IsSaveDialog() {
	return 0 != ( mCDLFlags & CDL_FLAG_SAVE_DIALOG );
}

bool cUICommonDialog::SortAlphabetically() {
	return 0 != ( mCDLFlags & CDL_FLAG_SORT_ALPHABETICALLY );
}

bool cUICommonDialog::FoldersFirst() {
	return 0 != ( mCDLFlags & CDL_FLAG_FOLDERS_FISRT );
}

bool cUICommonDialog::AllowFolderSelect() {
	return 0 != ( mCDLFlags & CDL_FLAG_ALLOW_FOLDER_SELECT );
}

void cUICommonDialog::SortAlphabetically( const bool& sortAlphabetically ) {
	SetFlagValue( &mCDLFlags, CDL_FLAG_SORT_ALPHABETICALLY, sortAlphabetically ? 1 : 0 );
	RefreshFolder();
}

void cUICommonDialog::FoldersFirst( const bool& foldersFirst ) {
	SetFlagValue( &mCDLFlags, CDL_FLAG_FOLDERS_FISRT , foldersFirst ? 1 : 0 );
	RefreshFolder();
}

void cUICommonDialog::AllowFolderSelect( const bool& allowFolderSelect ) {
	SetFlagValue( &mCDLFlags, CDL_FLAG_ALLOW_FOLDER_SELECT, allowFolderSelect ? 1 : 0 );
}

std::string cUICommonDialog::GetFullPath() {
	std::string tPath = mCurPath;

	DirPathAddSlashAtEnd( tPath );

	tPath += GetCurFile();

	return tPath;
}

std::string	cUICommonDialog::GetTempFullPath() {
	std::string tPath = mCurPath;

	DirPathAddSlashAtEnd( tPath );

	tPath += mList->GetItemSelectedText().ToUtf8();

	return tPath;
}

std::string cUICommonDialog::GetCurPath() const {
	return mCurPath;
}

std::string cUICommonDialog::GetCurFile() const {
	if ( mCDLFlags & CDL_FLAG_SAVE_DIALOG )
		return mFile->Text();

	return mList->GetItemSelectedText().ToUtf8();
}

cUIPushButton *	cUICommonDialog::GetButtonOpen() const {
	return mButtonOpen;
}

cUIPushButton *	cUICommonDialog::GetButtonCancel() const {
	return mButtonCancel;
}

cUIPushButton *	cUICommonDialog::GetButtonUp() const {
	return mButtonUp;
}

cUIListBox * cUICommonDialog::GetList() const {
	return mList;
}

cUITextInput * cUICommonDialog::GetPathInput() const {
	return mPath;
}

cUITextInput * cUICommonDialog::GetFileInput() const {
	return mFile;
}

cUIDropDownList * cUICommonDialog::GetFiletypeList() const {
	return mFiletype;
}

}}
