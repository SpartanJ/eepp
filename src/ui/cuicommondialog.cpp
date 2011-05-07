#include "cuicommondialog.hpp"
#include "cuimanager.hpp"
#include "cuilistboxitem.hpp"

namespace EE { namespace UI {

#define CDLG_MIN_WIDTH 420
#define CDLG_MIN_HEIGHT 267

cUICommonDialog::cUICommonDialog( const cUICommonDialog::CreateParams& Params ) :
	cUIWindow( Params ),
	mCurPath( Params.DefaultDirectory )
{
	if ( mSize.Width() < CDLG_MIN_WIDTH )
		mSize.x = CDLG_MIN_WIDTH;

	if ( mSize.Height() < CDLG_MIN_HEIGHT )
		mSize.y = CDLG_MIN_HEIGHT;

	if ( mMinWindowSize.Width() < CDLG_MIN_WIDTH )
		mMinWindowSize.Width( CDLG_MIN_WIDTH );

	if ( mMinWindowSize.Height() < CDLG_MIN_HEIGHT )
		mMinWindowSize.Height( CDLG_MIN_HEIGHT );

	Title( "Select a file" );

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
	ButtonParams.PosSet( Container()->Size().Width() - 81, Container()->Size().Height() - 54 );
	ButtonParams.SizeSet( 75, 22 );
	ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER;
	mButtonOpen = eeNew( cUIPushButton, ( ButtonParams ) );
	mButtonOpen->Visible( true );
	mButtonOpen->Enabled( true );
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

	cUIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( Container() );
	DDLParams.PosSet( TBox2->Pos().x + TBox2->Size().Width(), TBox2->Pos().y );
	DDLParams.SizeSet( Container()->Size().Width() - mButtonCancel->Size().Width() - DDLParams.Pos.x - 20, 22 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT;
	DDLParams.PopUpToMainControl = true;
	mFiletype = eeNew( cUIDropDownList, ( DDLParams ) );
	mFiletype->Visible( true );
	mFiletype->Enabled( true );
	mFiletype->ListBox()->AddListBoxItem( "*" );
	mFiletype->ListBox()->SetSelected(0);

	ApplyDefaultTheme();

	RefreshFolder();
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
		if ( IsDirectory( mCurPath + flist[i] ) ) {
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

	std::sort( folders.begin(), folders.end() );
	std::sort( files.begin(), files.end() );

	mList->Clear();
	mList->AddListBoxItems( folders );
	mList->AddListBoxItems( files );
}

Uint32 cUICommonDialog::OnMessage( const cUIMessage *Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick:
		{
			if ( Msg->Sender() == mButtonOpen ) {
				Open();
			} else if ( Msg->Sender() == mButtonCancel ) {
				CloseWindow();
			} else if ( Msg->Sender() == mButtonUp ) {
				mCurPath = RemoveLastFolderFromPath( mCurPath );
				mPath->Text( mCurPath );
				RefreshFolder();
			}

			break;
		}
		case cUIMessage::MsgDoubleClick:
		{
			if ( Msg->Sender()->IsType( UI_TYPE_LISTBOXITEM ) ) {
				std::string newPath = mCurPath + mList->GetItemSelectedText();

				if ( IsDirectory( newPath ) ) {
					mCurPath = newPath + GetOSlash();
					mPath->Text( mCurPath );
					RefreshFolder();
				} else {
					Open();
				}
			}

			break;
		}
		case cUIMessage::MsgSelected:
		{
			if ( Msg->Sender() == mList ) {
				mFile->Text( mList->GetItemSelectedText() );
			} else if ( Msg->Sender() == mFiletype ) {
				RefreshFolder();
			}

			break;
		}
	}

	return cUIWindow::OnMessage( Msg );
}

void cUICommonDialog::Open() {
	if ( "" != mList->GetItemSelectedText() ) {
		SendCommonEvent( cUIEvent::EventOpenFile );

		CloseWindow();
	}
}

void cUICommonDialog::AddFilePattern( std::string pattern, bool select ) {
	Uint32 index = mFiletype->ListBox()->AddListBoxItem( pattern );

	if ( select ) {
		mFiletype->ListBox()->SetSelected( index );

		RefreshFolder();
	}
}

std::string cUICommonDialog::GetCurPath() const {
	return mCurPath;
}

std::string cUICommonDialog::GetCurFile() const {
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
