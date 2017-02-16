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
	mTheme		= UIThemeManager::instance()->defaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->createWindow( NULL, Sizei( 378, 244 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &TextureAtlasNew::windowClose ) );
	mUIWindow->title( "New Texture Atlas" );

	Int32 PosX = mUIWindow->getContainer()->size().width() - 110;

	createTxtBox( Vector2i( 10, 20 ), "Save File Format:" );
	mSaveFileType = mTheme->createDropDownList( mUIWindow->getContainer(), Sizei( 100, 22 ), Vector2i( PosX, 20 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	std::vector<String> FileTypes;
	FileTypes.push_back( "TGA" );
	FileTypes.push_back( "BMP" );
	FileTypes.push_back( "PNG" );
	FileTypes.push_back( "DDS" );

	mSaveFileType->getListBox()->addListBoxItems( FileTypes );
	mSaveFileType->getListBox()->setSelected( "PNG" );

	createTxtBox( Vector2i( 10, 50 ), "Max. Texture Atlas Width:" );
	mComboWidth = mTheme->createComboBox( mUIWindow->getContainer(), Sizei( 100, 22 ), Vector2i( PosX, 50 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	createTxtBox( Vector2i( 10, 80 ), "Max. Texture Atlas Height:" );
	mComboHeight = mTheme->createComboBox( mUIWindow->getContainer(), Sizei( 100, 22 ), Vector2i( PosX, 80 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	std::vector<String> Sizes;

	for ( Uint32 i = 6; i < 14; i++ ) {
		Sizes.push_back( String::toStr( 1 << i ) );
	}

	mComboWidth->getListBox()->addListBoxItems( Sizes );
	mComboHeight->getListBox()->addListBoxItems( Sizes );
	mComboWidth->getInputTextBuffer()->allowOnlyNumbers( true );
	mComboHeight->getInputTextBuffer()->allowOnlyNumbers( true );
	mComboWidth->getListBox()->setSelected( "512" );
	mComboHeight->getListBox()->setSelected( "512" );

	createTxtBox( Vector2i( 10, 110 ), "Space between sub textures (pixels):" );
	mPixelSpace = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 100, 22 ), Vector2i( PosX, 110 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, 0, false );

	createTxtBox( Vector2i( 10, 140 ), "Texture Atlas Folder Path:" );
	mTGPath = mTheme->createTextInput( mUIWindow->getContainer(), Sizei( mUIWindow->getContainer()->size().width() - 60, 22 ), Vector2i( 10, 160 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE , false, 512 );
	mTGPath->allowEditing( false );

	mSetPathButton = mTheme->createPushButton( mUIWindow->getContainer(), Sizei( 32, 32 ), Vector2i( mUIWindow->getContainer()->size().width() - 10 - 32, 160 ) );
	mSetPathButton->text( "..." );
	mSetPathButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasNew::onDialogFolderSelect ) );

	UIPushButton * OKButton = mTheme->createPushButton( mUIWindow->getContainer(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->getIconByName( "ok" ) );
	OKButton->position( mUIWindow->getContainer()->size().width() - OKButton->size().width() - 4, mUIWindow->getContainer()->size().height() - OKButton->size().height() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasNew::okClick ) );
	OKButton->text( "OK" );

	UIPushButton * CancelButton = mTheme->createPushButton( mUIWindow->getContainer(), OKButton->size(), Vector2i( OKButton->position().x - OKButton->size().width() - 4, OKButton->position().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasNew::cancelClick ) );
	CancelButton->text( "Cancel" );

	mUIWindow->center();
	mUIWindow->show();
}

TextureAtlasNew::~TextureAtlasNew() {
}

UITextBox * TextureAtlasNew::createTxtBox( Vector2i Pos, const String& Text ) {
	return mTheme->createTextBox( Text, mUIWindow->getContainer(), Sizei(), Pos, UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
}

void TextureAtlasNew::okClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*>( Event );

	if ( MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		std::string ext( mSaveFileType->text() );
		String::toLowerInPlace( ext );

		UICommonDialog * TGDialog = mTheme->createCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_SAVE_DIALOG, "*." + ext );

		TGDialog->title( "Save Texture Atlas" );
		TGDialog->addEventListener( UIEvent::EventSaveFile, cb::Make1( this, &TextureAtlasNew::textureAtlasSave ) );
		TGDialog->center();
		TGDialog->show();
	}
}

void TextureAtlasNew::cancelClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*>( Event );

	if ( MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mUIWindow->CloseWindow();
	}
}

void TextureAtlasNew::windowClose( const UIEvent * Event ) {
	eeDelete( this );
}

static bool IsValidExtension( const std::string& ext ) {
	return ext == "png" || ext == "bmp" || ext == "dds" || ext == "tga";
}

void TextureAtlasNew::textureAtlasSave( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );
	std::string FPath( CDL->getFullPath() );

	if ( !FileSystem::isDirectory( FPath ) ) {
		Int32 w = 0, h = 0, b;
		bool Res1 = String::fromString<Int32>( w, mComboWidth->text() );
		bool Res2 = String::fromString<Int32>( h, mComboHeight->text() );
		b = static_cast<Int32>( mPixelSpace->value() );

		if ( Res1 && Res2 ) {
			Graphics::TexturePacker * TexturePacker = eeNew( Graphics::TexturePacker, ( w, h, false, b ) );

			TexturePacker->addTexturesPath( mTGPath->text() );

			TexturePacker->packTextures();

			std::string ext = FileSystem::fileExtension( FPath, true );

			if ( !IsValidExtension( ext ) ) {
				FPath = FileSystem::fileRemoveExtension( FPath );

				ext = mSaveFileType->text();

				String::toLowerInPlace( ext );

				FPath += "." + ext;
			}

			TexturePacker->save( FPath, static_cast<EE_SAVE_TYPE> ( mSaveFileType->getListBox()->getItemSelectedIndex() ) );

			if ( mNewTGCb.IsSet() )
				mNewTGCb( TexturePacker );

			mUIWindow->CloseWindow();
		}
	}
}

void TextureAtlasNew::onDialogFolderSelect( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*>( Event );

	if ( MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		UICommonDialog * TGDialog = mTheme->createCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_ALLOW_FOLDER_SELECT, "*" );

		TGDialog->title( "Create Texture Atlas ( Select Folder Containing Textures )" );
		TGDialog->addEventListener( UIEvent::EventOpenFile, cb::Make1( this, &TextureAtlasNew::onSelectFolder ) );
		TGDialog->center();
		TGDialog->show();
	}
}

void TextureAtlasNew::onSelectFolder( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );
	UIMessageBox * MsgBox;
	std::string FPath( CDL->getFullPath() );
	FileSystem::dirPathAddSlashAtEnd( FPath );

	if ( !FileSystem::isDirectory( FPath ) ) {
		FPath = CDL->getCurPath();
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
			mTGPath->text( FPath );
		} else {
			MsgBox = mTheme->createMessageBox( MSGBOX_OK, "The folder must contain at least one image!" );
			MsgBox->title( "Error" );
			MsgBox->center();
			MsgBox->show();
		}
	} else {
		MsgBox = mTheme->createMessageBox( MSGBOX_OK, "You must select a folder!" );
		MsgBox->title( "Error" );
		MsgBox->center();
		MsgBox->show();
	}
}

}}}
