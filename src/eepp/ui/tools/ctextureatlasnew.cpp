#include <eepp/ui/tools/ctextureatlasnew.hpp>
#include <eepp/ui/cuicommondialog.hpp>
#include <eepp/ui/cuimessagebox.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace UI { namespace Tools {

cTextureAtlasNew::cTextureAtlasNew( TGCreateCb NewTGCb ) :
	mTheme( NULL ),
	mUIWindow( NULL ),
	mNewTGCb( NewTGCb )
{
	mTheme		= cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->CreateWindow( NULL, eeSize( 378, 244 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cTextureAtlasNew::WindowClose ) );
	mUIWindow->Title( "New Texture Group" );

	Int32 PosX = mUIWindow->Container()->Size().Width() - 110;

	CreateTxtBox( eeVector2i( 10, 20 ), "Save File Format:" );
	mSaveFileType = mTheme->CreateDropDownList( mUIWindow->Container(), eeSize( 100, 22 ), eeVector2i( PosX, 20 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	std::vector<String> FileTypes;
	FileTypes.push_back( "TGA" );
	FileTypes.push_back( "BMP" );
	FileTypes.push_back( "PNG" );
	FileTypes.push_back( "DDS" );

	mSaveFileType->ListBox()->AddListBoxItems( FileTypes );
	mSaveFileType->ListBox()->SetSelected( "PNG" );

	CreateTxtBox( eeVector2i( 10, 50 ), "Max. Texture Atlas Width:" );
	mComboWidth = mTheme->CreateComboBox( mUIWindow->Container(), eeSize( 100, 22 ), eeVector2i( PosX, 50 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	CreateTxtBox( eeVector2i( 10, 80 ), "Max. Texture Atlas Height:" );
	mComboHeight = mTheme->CreateComboBox( mUIWindow->Container(), eeSize( 100, 22 ), eeVector2i( PosX, 80 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	std::vector<String> Sizes;

	for ( Uint32 i = 6; i < 14; i++ ) {
		Sizes.push_back( String::toStr( 1 << i ) );
	}

	mComboWidth->ListBox()->AddListBoxItems( Sizes );
	mComboHeight->ListBox()->AddListBoxItems( Sizes );
	mComboWidth->GetInputTextBuffer()->AllowOnlyNumbers( true );
	mComboHeight->GetInputTextBuffer()->AllowOnlyNumbers( true );
	mComboWidth->ListBox()->SetSelected( "512" );
	mComboHeight->ListBox()->SetSelected( "512" );

	CreateTxtBox( eeVector2i( 10, 110 ), "Space between sub texturesF (pixels):" );
	mPixelSpace = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 100, 22 ), eeVector2i( PosX, 110 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_SIZE, 0, false );

	CreateTxtBox( eeVector2i( 10, 140 ), "Texture Group Folder Path:" );
	mTGPath = mTheme->CreateTextInput( mUIWindow->Container(), eeSize( mUIWindow->Container()->Size().Width() - 60, 22 ), eeVector2i( 10, 160 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE , false, 512 );
	mTGPath->AllowEditing( false );

	mSetPathButton = mTheme->CreatePushButton( mUIWindow->Container(), eeSize( 32, 32 ), eeVector2i( mUIWindow->Container()->Size().Width() - 10 - 32, 160 ) );
	mSetPathButton->Text( "..." );
	mSetPathButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureAtlasNew::OnDialogFolderSelect ) );

	cUIPushButton * OKButton = mTheme->CreatePushButton( mUIWindow->Container(), eeSize( 80, 22 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureAtlasNew::OKClick ) );
	OKButton->Text( "OK" );

	cUIPushButton * CancelButton = mTheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), eeVector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureAtlasNew::CancelClick ) );
	CancelButton->Text( "Cancel" );

	mUIWindow->Center();
	mUIWindow->Show();
}

cTextureAtlasNew::~cTextureAtlasNew() {
}

cUITextBox * cTextureAtlasNew::CreateTxtBox( eeVector2i Pos, const String& Text ) {
	return mTheme->CreateTextBox( Text, mUIWindow->Container(), eeSize(), Pos, UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
}

void cTextureAtlasNew::OKClick( const cUIEvent * Event ) {
	std::string ext( mSaveFileType->Text() );
	String::ToLower( ext );

	cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, eeSize(), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, eeSize(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_SAVE_DIALOG, "*." + ext );

	TGDialog->Title( "Save Texture Group" );
	TGDialog->AddEventListener( cUIEvent::EventSaveFile, cb::Make1( this, &cTextureAtlasNew::TextureGroupSave ) );
	TGDialog->Center();
	TGDialog->Show();
}

void cTextureAtlasNew::CancelClick( const cUIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void cTextureAtlasNew::WindowClose( const cUIEvent * Event ) {
	eeDelete( this );
}

static bool IsValidExtension( const std::string& ext ) {
	return ext == "png" || ext == "bmp" || ext == "dds" || ext == "tga";
}

void cTextureAtlasNew::TextureGroupSave( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );
	std::string FPath( CDL->GetFullPath() );

	if ( !FileSystem::IsDirectory( FPath ) ) {
		Int32 w,h,b;
		bool Res1 = String::fromString<Int32>( w, mComboWidth->Text() );
		bool Res2 = String::fromString<Int32>( h, mComboHeight->Text() );
		b = static_cast<Int32>( mPixelSpace->Value() );

		if ( Res1 && Res2 ) {
			cTexturePacker * TexturePacker = eeNew( cTexturePacker, ( w, h, false, b ) );

			TexturePacker->AddTexturesPath( mTGPath->Text() );

			TexturePacker->PackTextures();

			std::string ext = FileSystem::FileExtension( FPath, true );

			if ( !IsValidExtension( ext ) ) {
				FPath = FileSystem::FileRemoveExtension( FPath );

				ext = mSaveFileType->Text();

				String::ToLower( ext );

				FPath += "." + ext;
			}

			TexturePacker->Save( FPath, static_cast<EE_SAVE_TYPE> ( mSaveFileType->ListBox()->GetItemSelectedIndex() ) );

			if ( mNewTGCb.IsSet() )
				mNewTGCb( TexturePacker );

			mUIWindow->CloseWindow();
		}
	}
}

void cTextureAtlasNew::OnDialogFolderSelect( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*>( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, eeSize(), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, eeSize(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_ALLOW_FOLDER_SELECT, "*" );

		TGDialog->Title( "Create Texture Group ( Select Folder Containing Textures )" );
		TGDialog->AddEventListener( cUIEvent::EventOpenFile, cb::Make1( this, &cTextureAtlasNew::OnSelectFolder ) );
		TGDialog->Center();
		TGDialog->Show();
	}
}

void cTextureAtlasNew::OnSelectFolder( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );
	cUIMessageBox * MsgBox;
	std::string FPath( CDL->GetFullPath() );
	FileSystem::DirPathAddSlashAtEnd( FPath );

	if ( !FileSystem::IsDirectory( FPath ) ) {
		FPath = CDL->GetCurPath();
		FileSystem::DirPathAddSlashAtEnd( FPath );
	}

	if ( FileSystem::IsDirectory( FPath ) ) {
		std::vector<std::string> files = FileSystem::FilesGetInPath( FPath );

		int x,y,c, count = 0;
		for ( Uint32 i = 0; i < files.size(); i++ ) {
			std::string ImgPath( FPath + files[i] );

			if ( !FileSystem::IsDirectory( ImgPath ) ) {
				int res = stbi_info( ImgPath.c_str(), &x, &y, &c );

				if ( res ) {
					count++;
					break;
				}
			}
		}

		//! All OK
		if ( count ) {
			mTGPath->Text( FPath );
		} else {
			MsgBox = mTheme->CreateMessageBox( MSGBOX_OK, "The folder must contain at least one image!" );
			MsgBox->Title( "Error" );
			MsgBox->Center();
			MsgBox->Show();
		}
	} else {
		MsgBox = mTheme->CreateMessageBox( MSGBOX_OK, "You must select a folder!" );
		MsgBox->Title( "Error" );
		MsgBox->Center();
		MsgBox->Show();
	}
}

}}}
