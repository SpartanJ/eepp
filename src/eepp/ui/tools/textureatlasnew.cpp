#include <SOIL2/src/SOIL2/stb_image.h>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/tools/textureatlasnew.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI { namespace Tools {

TextureAtlasNew::TextureAtlasNew( TGCreateCb NewTGCb ) : mUIWindow( NULL ), mNewTGCb( NewTGCb ) {
	mUIWindow = UIWindow::New();
	mUIWindow->setSizeWithDecoration( 378, 0 )->setMinWindowSize( 378, 0 )->setWindowFlags(
		UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_SHARE_ALPHA_WITH_CHILDS |
		UI_WIN_MODAL );

	mUIWindow->addEventListener( Event::OnWindowClose,
								 [this] ( auto event ) { windowClose( event ); } );
	mUIWindow->setTitle( "New Texture Atlas" );

	static const auto layout = R"xml(
	<LinearLayout id='container' layout_width='match_parent' layout_height='wrap_content' margin='8dp'>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' margin-bottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Save File Format:' />
			<DropDownList id='saveType' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' selected-text='PNG'>
				<item>TGA</item>
				<item>BMP</item>
				<item>PNG</item>
				<item>DDS</item>
				<item>JPG</item>
			</DropDownList>
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' margin-bottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Max. Texture Atlas Width:' />
			<ComboBox id='maxTAWidth' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' allowFloat='true' />
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' margin-bottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Max. Texture Atlas Height:' />
			<ComboBox id='maxTAHeight' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' allowFloat='true' />
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' margin-bottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Space between images (pixels):' />
			<SpinBox id='pixelSpace' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' />
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' margin-bottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Pixel Density:' />
			<DropDownList id='pixelDensity' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' selected-text='MDPI'>
				<item>MDPI</item>
				<item>HDPI</item>
				<item>XHDPI</item>
				<item>XXHDPI</item>
				<item>XXXHDPI</item>
			</DropDownList>
		</LinearLayout>
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' margin-bottom='8dp'>
			<TextView layout_width='match_parent' layout_weight='0.7' layout_height='wrap_content' layout_gravity='center_vertical' text='Texture Filter:' />
			<DropDownList id='textureFilter' layout_width='match_parent' layout_weight='0.3' layout_height='wrap_content' layout_gravity='center_vertical' selected-text='Linear'>
				<item>Linear</item>
				<item>Nearest</item>
			</DropDownList>
		</LinearLayout>
		<TextView layout_width='match_parent' layout_height='wrap_content' layout_gravity='center_vertical' text='TextureAtlas Folder Path:' />
		<LinearLayout layout_width='match_parent' layout_height='wrap_content' orientation='horizontal' margin-bottom='8dp'>
			<TextInput id='pathInput' layout_width='match_parent' layout_height='match_parent' layout_weight='1' allowEditing='false' />
			<PushButton id='openPath' layout_width='32dp' layout_height='wrap_content' text='...'  />
		</LinearLayout>
		<CheckBox id="forcePow2" layout_width='match_parent' layout_height='wrap_content' text="Force power of 2 textures." />
		<CheckBox id="scalableSVG" layout_width='match_parent' layout_height='wrap_content' text="Scale SVG source files using the pixel-density provided." />
		<CheckBox id="saveExtensions" layout_width='match_parent' layout_height='wrap_content' text="Save the images extensions in regions names." tooltip="Save the image files extensions as part of the texture regions names." />
		<CheckBox id="allowChilds" layout_width='match_parent' layout_height='wrap_content' text="Allow create multiple texture atlases on save."
			tooltip="When enabled in the case of an atlas not having enough space in the&#10;image to fit all the source input images it will create new child&#10;atlas images to save them."
		/>
		<LinearLayout layout_gravity='center_vertical|right' layout_width='wrap_content' layout_height='wrap_content' orientation='horizontal' margin-top="8dp" margin-bottom='16dp'>
			<PushButton id='cancelButton' layout_width='wrap_content' layout_height='wrap_content' layout_weight='0.2' icon='cancel' text='Cancel' margin-right='4dp' />
			<PushButton id='okButton' layout_width='wrap_content' layout_height='wrap_content' layout_weight='0.2' icon='ok' text='OK' />
		</LinearLayout>
	 </LinearLayout>
	 )xml";

	UIWidget* container =
		mUIWindow->getUISceneNode()->loadLayoutFromString( layout, mUIWindow->getContainer() );
	mUIWindow->bind( "saveType", mSaveFileType );
	mUIWindow->bind( "maxTAWidth", mComboWidth );
	mUIWindow->bind( "maxTAHeight", mComboHeight );
	mUIWindow->bind( "pixelSpace", mPixelSpace );
	mUIWindow->bind( "pixelDensity", mPixelDensity );
	mUIWindow->bind( "pathInput", mTGPath );
	mUIWindow->bind( "openPath", mSetPathButton );
	mUIWindow->bind( "textureFilter", mTextureFilter );
	mUIWindow->bind( "forcePow2", mForcePow2 );
	mUIWindow->bind( "scalableSVG", mScalableSVG );
	mUIWindow->bind( "saveExtensions", mSaveExtensions );
	mUIWindow->bind( "allowChilds", mAllowChilds );

	std::vector<String> Sizes;

	for ( Uint32 i = 8; i < 15; i++ ) {
		Sizes.push_back( String::toString( 1 << i ) );
	}

	mComboWidth->getListBox()->addListBoxItems( Sizes );
	mComboHeight->getListBox()->addListBoxItems( Sizes );
	mComboWidth->getListBox()->setSelected( "2048" );
	mComboHeight->getListBox()->setSelected( "2048" );

	mSetPathButton->addEventListener( Event::MouseClick,
									  [this] ( auto event ) { onDialogFolderSelect( event ); } );
	mUIWindow->find<UIPushButton>( "okButton" )
		->addEventListener( Event::MouseClick, [this] ( auto event ) { okClick( event ); } );
	mUIWindow->find<UIPushButton>( "cancelButton" )
		->addEventListener( Event::MouseClick, [this] ( auto event ) { cancelClick( event ); } );

	container->addEventListener( Event::OnLayoutUpdate, [this]( const Event* event ) {
		mUIWindow->setMinWindowSize( event->getNode()->getSize() );
		mUIWindow->center();
		mUIWindow->show();
		event->getNode()->removeEventListener( event->getCallbackId() );
	} );
}

TextureAtlasNew::~TextureAtlasNew() {}

void TextureAtlasNew::okClick( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		std::string ext( mSaveFileType->getText() );
		String::toLowerInPlace( ext );

		UIFileDialog* TGDialog =
			UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::SaveDialog, "*." + ext );
		TGDialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Save Texture Atlas" );
		TGDialog->addEventListener( Event::SaveFile,
									[this] ( auto event ) { textureAtlasSave( event ); } );
		TGDialog->center();
		TGDialog->show();
	}
}

void TextureAtlasNew::cancelClick( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mUIWindow->closeWindow();
	}
}

void TextureAtlasNew::windowClose( const Event* ) {
	eeDelete( this );
}

static bool isValidExtension( const std::string& ext ) {
	return ext == "png" || ext == "bmp" || ext == "dds" || ext == "tga" || ext == "jpg" ||
		   ext == "qoi";
}

void TextureAtlasNew::textureAtlasSave( const Event* Event ) {
	std::string FPath( Event->getNode()->asType<UIFileDialog>()->getFullPath() );

	if ( !FileSystem::isDirectory( FPath ) ) {
		Int32 w = 0, h = 0, b;
		bool Res1 = String::fromString( w, mComboWidth->getText() );
		bool Res2 = String::fromString( h, mComboHeight->getText() );
		b = static_cast<Int32>( mPixelSpace->getValue() );
		Texture::Filter textureFilter = mTextureFilter->getText() == "Nearest"
											? Texture::Filter::Nearest
											: Texture::Filter::Linear;

		if ( Res1 && Res2 ) {
			TexturePacker* texturePacker = TexturePacker::New(
				w, h, PixelDensity::toFloat( PixelDensity::fromString( mPixelDensity->getText() ) ),
				mForcePow2->isChecked(), mScalableSVG->isChecked(), b, textureFilter,
				mAllowChilds->isChecked(), false );

			texturePacker->addTexturesPath( mTGPath->getText() );

			texturePacker->packTextures();

			std::string ext = FileSystem::fileExtension( FPath, true );

			if ( !isValidExtension( ext ) ) {
				FPath = FileSystem::fileRemoveExtension( FPath );

				ext = mSaveFileType->getText();

				String::toLowerInPlace( ext );

				FPath += "." + ext;
			}

			texturePacker->save(
				FPath,
				static_cast<Image::SaveType>( mSaveFileType->getListBox()->getItemSelectedIndex() ),
				mSaveExtensions->isChecked() );

			if ( mNewTGCb )
				mNewTGCb( texturePacker );

			mUIWindow->closeWindow();
		}
	}
}

void TextureAtlasNew::onDialogFolderSelect( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		UIFileDialog* TGDialog =
			UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect, "*" );
		TGDialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Create Texture Atlas ( Select Folder Containing Textures )" );
		TGDialog->addEventListener( Event::OpenFile,
									[this] ( auto event ) { onSelectFolder( event ); } );
		TGDialog->center();
		TGDialog->show();
	}
}

void TextureAtlasNew::onSelectFolder( const Event* Event ) {
	UIFileDialog* CDL = Event->getNode()->asType<UIFileDialog>();
	UIMessageBox* MsgBox;
	std::string FPath( CDL->getFullPath() );
	FileSystem::dirAddSlashAtEnd( FPath );

	if ( !FileSystem::isDirectory( FPath ) ) {
		FPath = CDL->getCurPath();
		FileSystem::dirAddSlashAtEnd( FPath );
	}

	if ( FileSystem::isDirectory( FPath ) ) {
		std::vector<std::string> files = FileSystem::filesGetInPath( FPath );

		int x, y, c, count = 0;
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
			MsgBox = UIMessageBox::New( UIMessageBox::OK,
										"The folder must contain at least one image!" );
			MsgBox->setTitle( "Error" );
			MsgBox->center();
			MsgBox->show();
		}
	} else {
		MsgBox = UIMessageBox::New( UIMessageBox::OK, "You must select a folder!" );
		MsgBox->setTitle( "Error" );
		MsgBox->center();
		MsgBox->show();
	}
}

}}} // namespace EE::UI::Tools
