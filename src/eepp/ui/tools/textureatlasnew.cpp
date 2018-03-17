#include <eepp/ui/tools/textureatlasnew.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <SOIL2/src/SOIL2/stb_image.h>

namespace EE { namespace UI { namespace Tools {

TextureAtlasNew::TextureAtlasNew( TGCreateCb NewTGCb ) :
	mTheme( NULL ),
	mUIWindow( NULL ),
	mNewTGCb( NewTGCb )
{
	mTheme		= UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= UIWindow::New();
	mUIWindow->setSizeWithDecoration( 378, 303 )
			 ->setMinWindowSize( 378, 303 )
			 ->setWinFlags( UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_SHARE_ALPHA_WITH_CHILDS | UI_WIN_MODAL );

	mUIWindow->addEventListener( Event::OnWindowClose, cb::Make1( this, &TextureAtlasNew::windowClose ) );
	mUIWindow->setTitle( "New Texture Atlas" );

	std::string layout = R"xml(
	<LinearLayout id='container' layout_width='match_parent' layout_height='wrap_content' layout_marginLeft='8dp' layout_marginRight='8dp' layout_marginTop='8dp'>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' layout_marginBottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Save File Format:' />
			<DropDownList id='saveType' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' selectedText='PNG'>
				<item>TGA</item>
				<item>BMP</item>
				<item>PNG</item>
				<item>DDS</item>
				<item>JPG</item>
			</DropDownList>
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' layout_marginBottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Max. Texture Atlas Width:' />
			<ComboBox id='maxTAWidth' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' onlyNumbers='true' />
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' layout_marginBottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Max. Texture Atlas Height:' />
			<ComboBox id='maxTAHeight' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' onlyNumbers='true' />
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' layout_marginBottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Space between sub textures (pixels):' />
			<SpinBox id='pixelSpace' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' />
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' layout_marginBottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Pixel Density:' />
			<DropDownList id='pixelDensity' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' selectedText='MDPI'>
				<item>MDPI</item>
				<item>HDPI</item>
				<item>XHDPI</item>
				<item>XXHDPI</item>
				<item>XXXHDPI</item>
			</DropDownList>
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' layout_marginBottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Texture Filter:' />
			<DropDownList id='textureFilter' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' selectedText='Linear'>
				<item>Linear</item>
				<item>Nearest</item>
			</DropDownList>
		</LinearLayout>
		<TextView layout_width='match_parent' layout_height='wrap_content' layout_gravity='center_vertical' text='TextureAtlas Folder Path:' />
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' layout_marginBottom='8dp'>
			<TextInput id='pathInput' layout_width='match_parent' layout_height='match_parent' layout_weight='1' allowEditing='false' />
			<PushButton id='openPath' layout_width='32dp' layout_height='wrap_content' text='...'  />
		</LinearLayout>
		<LinearLayout layout_gravity='center_vertical|right' layout_width='wrap_content' layout_height='wrap_content' orientation='horizontal' layout_marginBottom='16dp'>
			<PushButton id='okButton' layout_width='wrap_content' layout_height='wrap_content' layout_weight='0.2' icon='ok' text='OK' layout_marginRight='4dp' />
			<PushButton id='cancelButton' layout_width='wrap_content' layout_height='wrap_content' layout_weight='0.2' icon='cancel' text='Cancel'  />
		</LinearLayout>
	 </LinearLayout>
	 )xml";

	if ( mUIWindow->getSceneNode()->isUISceneNode() )
		static_cast<UISceneNode*>( mUIWindow->getSceneNode() )->loadLayoutFromString( layout, mUIWindow->getContainer() );

	mUIWindow->bind( "saveType", mSaveFileType );
	mUIWindow->bind( "maxTAWidth", mComboWidth );
	mUIWindow->bind( "maxTAHeight", mComboHeight );
	mUIWindow->bind( "pixelSpace", mPixelSpace );
	mUIWindow->bind( "pixelDensity", mPixelDensity );
	mUIWindow->bind( "pathInput", mTGPath );
	mUIWindow->bind( "openPath", mSetPathButton ) ;
	mUIWindow->bind( "textureFilter", mTextureFilter );

	std::vector<String> Sizes;

	for ( Uint32 i = 8; i < 15; i++ ) {
		Sizes.push_back( String::toStr( 1 << i ) );
	}

	mComboWidth->getListBox()->addListBoxItems( Sizes );
	mComboHeight->getListBox()->addListBoxItems( Sizes );
	mComboWidth->getListBox()->setSelected( "2048" );
	mComboHeight->getListBox()->setSelected( "2048" );

	mSetPathButton->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasNew::onDialogFolderSelect ) );
	mUIWindow->find<UIPushButton>( "okButton" )->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasNew::okClick ) );
	mUIWindow->find<UIPushButton>( "cancelButton" )->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasNew::cancelClick ) );

	mUIWindow->setSizeWithDecoration( mUIWindow->getContainer()->find( "container" )->getSize() );
	mUIWindow->center();
	mUIWindow->show();
}

TextureAtlasNew::~TextureAtlasNew() {
}

void TextureAtlasNew::okClick( const Event * event ) {
	const MouseEvent * mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		std::string ext( mSaveFileType->getText() );
		String::toLowerInPlace( ext );

		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS | CDL_FLAG_SAVE_DIALOG, "*." + ext );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Save Texture Atlas" );
		TGDialog->addEventListener( Event::SaveFile, cb::Make1( this, &TextureAtlasNew::textureAtlasSave ) );
		TGDialog->center();
		TGDialog->show();
	}
}

void TextureAtlasNew::cancelClick( const Event * event ) {
	const MouseEvent * mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mUIWindow->closeWindow();
	}
}

void TextureAtlasNew::windowClose( const Event * Event ) {
	eeDelete( this );
}

static bool isValidExtension( const std::string& ext ) {
	return ext == "png" || ext == "bmp" || ext == "dds" || ext == "tga" || ext == "jpg";
}

void TextureAtlasNew::textureAtlasSave( const Event * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getNode() );
	std::string FPath( CDL->getFullPath() );

	if ( !FileSystem::isDirectory( FPath ) ) {
		Int32 w = 0, h = 0, b;
		bool Res1 = String::fromString<Int32>( w, mComboWidth->getText() );
		bool Res2 = String::fromString<Int32>( h, mComboHeight->getText() );
		b = static_cast<Int32>( mPixelSpace->getValue() );
		Texture::TextureFilter textureFilter = mTextureFilter->getText() == "Nearest" ? Texture::TextureFilter::Nearest : Texture::TextureFilter::Linear;

		if ( Res1 && Res2 ) {
			TexturePacker * texturePacker = eeNew( TexturePacker, ( w, h, PixelDensity::fromString( mPixelDensity->getText() ), false, false, b, textureFilter ) );

			texturePacker->addTexturesPath( mTGPath->getText() );

			texturePacker->packTextures();

			std::string ext = FileSystem::fileExtension( FPath, true );

			if ( !isValidExtension( ext ) ) {
				FPath = FileSystem::fileRemoveExtension( FPath );

				ext = mSaveFileType->getText();

				String::toLowerInPlace( ext );

				FPath += "." + ext;
			}

			texturePacker->save( FPath, static_cast<Image::SaveType> ( mSaveFileType->getListBox()->getItemSelectedIndex() ) );

			if ( mNewTGCb.IsSet() )
				mNewTGCb( texturePacker );

			mUIWindow->closeWindow();
		}
	}
}

void TextureAtlasNew::onDialogFolderSelect( const Event * event ) {
	const MouseEvent * mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS | CDL_FLAG_ALLOW_FOLDER_SELECT, "*" );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Create Texture Atlas ( Select Folder Containing Textures )" );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( this, &TextureAtlasNew::onSelectFolder ) );
		TGDialog->center();
		TGDialog->show();
	}
}

void TextureAtlasNew::onSelectFolder( const Event * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getNode() );
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
				bool res = Image::getInfo( ImgPath.c_str(), &x, &y, &c );

				if ( res ) {
					count++;
					break;
				}
			}
		}

		//! All OK
		if ( count ) {
			mTGPath->setText( FPath );
		} else {
			MsgBox = UIMessageBox::New( MSGBOX_OK, "The folder must contain at least one image!" );
			MsgBox->setTitle( "Error" );
			MsgBox->center();
			MsgBox->show();
		}
	} else {
		MsgBox = UIMessageBox::New( MSGBOX_OK, "You must select a folder!" );
		MsgBox->setTitle( "Error" );
		MsgBox->center();
		MsgBox->show();
	}
}

}}}
