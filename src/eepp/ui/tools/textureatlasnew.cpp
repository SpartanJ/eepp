#include <eepp/ui/tools/textureatlasnew.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace UI { namespace Tools {

TextureAtlasNew::TextureAtlasNew( TGCreateCb NewTGCb ) :
	mTheme( NULL ),
	mUIWindow( NULL ),
	mNewTGCb( NewTGCb )
{
	mTheme		= UIThemeManager::instance()->DefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->CreateWindow( NULL, Sizei( 378, 244 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL );
	mUIWindow->AddEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &TextureAtlasNew::WindowClose ) );
	mUIWindow->Title( "New Texture Atlas" );

	Int32 PosX = mUIWindow->Container()->Size().width() - 110;

	CreateTxtBox( Vector2i( 10, 20 ), "Save File Format:" );
	mSaveFileType = mTheme->CreateDropDownList( mUIWindow->Container(), Sizei( 100, 22 ), Vector2i( PosX, 20 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	std::vector<String> FileTypes;
	FileTypes.push_back( "TGA" );
	FileTypes.push_back( "BMP" );
	FileTypes.push_back( "PNG" );
	FileTypes.push_back( "DDS" );

	mSaveFileType->ListBox()->AddListBoxItems( FileTypes );
	mSaveFileType->ListBox()->SetSelected( "PNG" );

	CreateTxtBox( Vector2i( 10, 50 ), "Max. Texture Atlas Width:" );
	mComboWidth = mTheme->CreateComboBox( mUIWindow->Container(), Sizei( 100, 22 ), Vector2i( PosX, 50 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	CreateTxtBox( Vector2i( 10, 80 ), "Max. Texture Atlas Height:" );
	mComboHeight = mTheme->CreateComboBox( mUIWindow->Container(), Sizei( 100, 22 ), Vector2i( PosX, 80 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	std::vector<String> Sizes;

	for ( Uint32 i = 6; i < 14; i++ ) {
		Sizes.push_back( String::toStr( 1 << i ) );
	}

	mComboWidth->ListBox()->AddListBoxItems( Sizes );
	mComboHeight->ListBox()->AddListBoxItems( Sizes );
	mComboWidth->GetInputTextBuffer()->allowOnlyNumbers( true );
	mComboHeight->GetInputTextBuffer()->allowOnlyNumbers( true );
	mComboWidth->ListBox()->SetSelected( "512" );
	mComboHeight->ListBox()->SetSelected( "512" );

	CreateTxtBox( Vector2i( 10, 110 ), "Space between sub textures (pixels):" );
	mPixelSpace = mTheme->CreateSpinBox( mUIWindow->Container(), Sizei( 100, 22 ), Vector2i( PosX, 110 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, 0, false );

	CreateTxtBox( Vector2i( 10, 140 ), "Texture Atlas Folder Path:" );
	mTGPath = mTheme->CreateTextInput( mUIWindow->Container(), Sizei( mUIWindow->Container()->Size().width() - 60, 22 ), Vector2i( 10, 160 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE , false, 512 );
	mTGPath->AllowEditing( false );

	mSetPathButton = mTheme->CreatePushButton( mUIWindow->Container(), Sizei( 32, 32 ), Vector2i( mUIWindow->Container()->Size().width() - 10 - 32, 160 ) );
	mSetPathButton->Text( "..." );
	mSetPathButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasNew::OnDialogFolderSelect ) );

	UIPushButton * OKButton = mTheme->CreatePushButton( mUIWindow->Container(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().width() - OKButton->Size().width() - 4, mUIWindow->Container()->Size().height() - OKButton->Size().height() - 4 );
	OKButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasNew::OKClick ) );
	OKButton->Text( "OK" );

	UIPushButton * CancelButton = mTheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), Vector2i( OKButton->Pos().x - OKButton->Size().width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasNew::CancelClick ) );
	CancelButton->Text( "Cancel" );

	mUIWindow->Center();
	mUIWindow->Show();
}

TextureAtlasNew::~TextureAtlasNew() {
}

UITextBox * TextureAtlasNew::CreateTxtBox( Vector2i Pos, const String& Text ) {
	return mTheme->CreateTextBox( Text, mUIWindow->Container(), Sizei(), Pos, UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
}

void TextureAtlasNew::OKClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*>( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		std::string ext( mSaveFileType->Text() );
		String::toLowerInPlace( ext );

		UICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_SAVE_DIALOG, "*." + ext );

		TGDialog->Title( "Save Texture Atlas" );
		TGDialog->AddEventListener( UIEvent::EventSaveFile, cb::Make1( this, &TextureAtlasNew::TextureAtlasSave ) );
		TGDialog->Center();
		TGDialog->Show();
	}
}

void TextureAtlasNew::CancelClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*>( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		mUIWindow->CloseWindow();
	}
}

void TextureAtlasNew::WindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

static bool IsValidExtension( const std::string& ext ) {
	return ext == "png" || ext == "bmp" || ext == "dds" || ext == "tga";
}

void TextureAtlasNew::TextureAtlasSave( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->Ctrl() );
	std::string FPath( CDL->GetFullPath() );

	if ( !FileSystem::isDirectory( FPath ) ) {
		Int32 w = 0, h = 0, b;
		bool Res1 = String::fromString<Int32>( w, mComboWidth->Text() );
		bool Res2 = String::fromString<Int32>( h, mComboHeight->Text() );
		b = static_cast<Int32>( mPixelSpace->Value() );

		if ( Res1 && Res2 ) {
			Graphics::TexturePacker * TexturePacker = eeNew( Graphics::TexturePacker, ( w, h, false, b ) );

			TexturePacker->addTexturesPath( mTGPath->Text() );

			TexturePacker->packTextures();

			std::string ext = FileSystem::fileExtension( FPath, true );

			if ( !IsValidExtension( ext ) ) {
				FPath = FileSystem::fileRemoveExtension( FPath );

				ext = mSaveFileType->Text();

				String::toLowerInPlace( ext );

				FPath += "." + ext;
			}

			TexturePacker->save( FPath, static_cast<EE_SAVE_TYPE> ( mSaveFileType->ListBox()->GetItemSelectedIndex() ) );

			if ( mNewTGCb.IsSet() )
				mNewTGCb( TexturePacker );

			mUIWindow->CloseWindow();
		}
	}
}

void TextureAtlasNew::OnDialogFolderSelect( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*>( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		UICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_ALLOW_FOLDER_SELECT, "*" );

		TGDialog->Title( "Create Texture Atlas ( Select Folder Containing Textures )" );
		TGDialog->AddEventListener( UIEvent::EventOpenFile, cb::Make1( this, &TextureAtlasNew::OnSelectFolder ) );
		TGDialog->Center();
		TGDialog->Show();
	}
}

void TextureAtlasNew::OnSelectFolder( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->Ctrl() );
	UIMessageBox * MsgBox;
	std::string FPath( CDL->GetFullPath() );
	FileSystem::dirPathAddSlashAtEnd( FPath );

	if ( !FileSystem::isDirectory( FPath ) ) {
		FPath = CDL->GetCurPath();
		FileSystem::dirPathAddSlashAtEnd( FPath );
	}

	if ( FileSystem::isDirectory( FPath ) ) {
		std::vector<std::string> files = FileSystem::filesGetInPath( FPath );

		int x,y,c, count = 0;
		for ( Uint32 i = 0; i < files.size(); i++ ) {
			std::string ImgPath( FPath + files[i] );

			if ( !FileSystem::isDirectory( ImgPath ) ) {
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
