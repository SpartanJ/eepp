#include <algorithm>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/ui/tools/textureatlasnew.hpp>
#include <eepp/ui/tools/textureatlastextureregioneditor.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace UI { namespace Tools {

UIWidget* TextureAtlasEditor::createTextureAtlasTextureRegionEditor( std::string ) {
	mTextureRegionEditor = TextureAtlasTextureRegionEditor::New( this );
	return mTextureRegionEditor;
}

TextureAtlasEditor* TextureAtlasEditor::New( UIWindow* attachTo,
											 const TextureAtlasEditor::TGEditorCloseCb& callback ) {
	return eeNew( TextureAtlasEditor, ( attachTo, callback ) );
}

TextureAtlasEditor::TextureAtlasEditor( UIWindow* attachTo, const TGEditorCloseCb& callback ) :
	mUIWindow( attachTo ),
	mCloseCb( callback ),
	mTexturePacker( NULL ),
	mTextureAtlasLoader( NULL ),
	mCurTextureRegion( NULL ),
	mEdited( false ) {
	UISceneNode* uiSceneNode =
		NULL != attachTo ? attachTo->getUISceneNode() : SceneManager::instance()->getUISceneNode();

	if ( NULL == uiSceneNode || NULL == uiSceneNode->getUIThemeManager() ||
		 NULL == uiSceneNode->getUIThemeManager()->getDefaultTheme() ) {
		Log::error( "TextureAtlasEditor needs a default theme assigned to work." );
		return;
	}

	mTheme = uiSceneNode->getUIThemeManager()->getDefaultTheme();

	if ( NULL == mUIWindow ) {
		mUIContainer = uiSceneNode;
		uiSceneNode->getRoot()->addClass( "appbackground" );
	} else {
		mUIContainer = mUIWindow->getContainer();
		mUIWindow->getContainer()->addClass( "appbackground" );
	}

	std::string layout = R"xml(
	<LinearLayout id="texture_atlas_editor_root" orientation="vertical" layout_width="match_parent" layout_height="match_parent">
		<MenuBar layout_width="match_parent" layout_height="wrap_content">
			<Menu id="fileMenu" text="File">
				<item text="New..." icon="document-new" />
				<item text="Open..." icon="document-open" />
				<separator />
				<item text="Save" icon="document-save" />
				<separator />
				<item text="Close" icon="document-close" />
				<separator />
				<item text="Quit" icon="quit" />
			</Menu>
		</MenuBar>
		<LinearLayout layout_width="match_parent" layout_height="0dp" layout_weight="1" orientation="horizontal">
			<TextureAtlasTextureRegionEditor layout_width="match_parent" layout_height="match_parent" layout_weight="1"
											flags="clip" background-color="#00000032" border-width="1" border-color="#000000FF" />
			<LinearLayout orientation="vertical" layout_width="210dp" layout_height="match_parent" margin-left="8dp" margin-right="8dp">
				<TextView text="Texture Filter:" font-style="shadow" margin-top="4dp" margin-bottom="4dp" />
				<DropDownList id="textureFilter" layout_width="match_parent" layout_height="wrap_content" layout_gravity="center_vertical" selected-text="Linear">
					<item>Linear</item>
					<item>Nearest</item>
				</DropDownList>
				<TextView text="TextureRegion List:" font-style="shadow" margin-top="8dp" margin-bottom="8dp" />
				<TabWidget layout_width="match_parent" layout_height="0dp" layout_weight="1">
					<ScrollView id="GridView" layout_width="match_parent" layout_height="144dp" touchdrag="true">
						<GridLayout columnMode="size" rowMode="size" columnWidth="64dp" row-height="64dp" layout_width="match_parent" layout_height="wrap_content" id="gridlayout" />
					</ScrollView>
					<ListBox id="TextureRegionList" layout_width="match_parent" layout_height="144dp" touchDrag="true" />
					<Tab text="List" owns="TextureRegionList" />
					<Tab text="Grid" owns="GridView" />
				</TabWidget>
				<TextView text="Current TextureRegion:" font-style="shadow" margin-top="16dp" margin-bottom="16dp" />
				<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
					<TextView text="Offset X:" font-style="shadow" layout_width="match_parent" layout_height="wrap_content" layout_weight="1"
									margin-right="8dp" layout_gravity="center" gravity="right|center_vertical" />
					<SpinBox id="offX" layout_width="100dp" layout_height="wrap_content" min-value="-32000" max-value="32000" />
				</LinearLayout>
				<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
					<TextView text="Offset Y:" font-style="shadow" layout_width="match_parent" layout_height="wrap_content" layout_weight="1"
									margin-right="8dp" layout_gravity="center" gravity="right|center_vertical" />
					<SpinBox id="offY" layout_width="100dp" layout_height="wrap_content" min-value="-32000" max-value="32000" />
				</LinearLayout>
				<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
					<TextView text="Dest. Width" font-style="shadow" layout_width="match_parent" layout_height="wrap_content" layout_weight="1"
									margin-right="8dp" layout_gravity="center" gravity="right|center_vertical" />
					<SpinBox id="destW" layout_width="100dp" layout_height="wrap_content" min-value="0" max-value="32000" />
				</LinearLayout>
				<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
					<TextView text="Dest. Height" font-style="shadow" layout_width="match_parent" layout_height="wrap_content" layout_weight="1"
									margin-right="8dp" layout_gravity="center" gravity="right|center_vertical" />
					<SpinBox id="destH" layout_width="100dp" layout_height="wrap_content" min-value="0" max-value="32000" />
				</LinearLayout>
				<PushButton id="resetDest" text="Reset Dest. Size" layout_width="match_parent" layout_height="wrap_content" margin-bottom="8dp" margin-top="8dp" />
				<PushButton id="resetOff" text="Reset Offset" layout_width="match_parent" layout_height="wrap_content" margin-bottom="8dp" />
				<PushButton id="centerOff" text="Centered Offset" layout_width="match_parent" layout_height="wrap_content" margin-bottom="8dp" />
				<PushButton id="hbotOff" text="Half-Bottom Offset" layout_width="match_parent" layout_height="wrap_content" margin-bottom="8dp" />
			</LinearLayout>
		</LinearLayout>
	</LinearLayout>
	)xml";

	UIWidgetCreator::addCustomWidgetCallback(
		"TextureAtlasTextureRegionEditor",
		cb::Make1( this, &TextureAtlasEditor::createTextureAtlasTextureRegionEditor ) );

	if ( NULL != mUIContainer->getSceneNode()->asType<UISceneNode>()->getUIThemeManager() )
		mUIContainer->getSceneNode()->asType<UISceneNode>()->loadLayoutFromString( layout,
																				   mUIContainer );

	UIWidgetCreator::removeCustomWidgetCallback( "TextureAtlasTextureRegionEditor" );

	mUIContainer->bind( "TextureRegionList", mTextureRegionList );
	mTextureRegionList->addEventListener(
		Event::OnItemSelected, cb::Make1( this, &TextureAtlasEditor::onTextureRegionChange ) );

	mUIContainer->bind( "gridlayout", mTextureRegionGrid );

	mUIContainer->bind( "offX", mSpinOffX );
	mSpinOffX->addEventListener( Event::OnValueChange,
								 cb::Make1( this, &TextureAtlasEditor::onOffXChange ) );

	mUIContainer->bind( "offY", mSpinOffY );
	mSpinOffY->addEventListener( Event::OnValueChange,
								 cb::Make1( this, &TextureAtlasEditor::onOffYChange ) );

	mUIContainer->bind( "destW", mSpinDestW );
	mSpinDestW->addEventListener( Event::OnValueChange,
								  cb::Make1( this, &TextureAtlasEditor::onDestWChange ) );

	mUIContainer->bind( "destH", mSpinDestH );
	mSpinDestH->addEventListener( Event::OnValueChange,
								  cb::Make1( this, &TextureAtlasEditor::onDestHChange ) );

	mUIContainer->bind( "textureFilter", mTextureFilterList );
	mTextureFilterList->addEventListener(
		Event::OnItemSelected, cb::Make1( this, &TextureAtlasEditor::onTextureFilterChange ) );

	mUIContainer->find<UIPushButton>( "resetDest" )
		->addEventListener( Event::MouseClick,
							cb::Make1( this, &TextureAtlasEditor::onResetDestSize ) );

	mUIContainer->find<UIPushButton>( "resetOff" )
		->addEventListener( Event::MouseClick,
							cb::Make1( this, &TextureAtlasEditor::onResetOffset ) );

	mUIContainer->find<UIPushButton>( "centerOff" )
		->addEventListener( Event::MouseClick,
							cb::Make1( this, &TextureAtlasEditor::onCenterOffset ) );

	mUIContainer->find<UIPushButton>( "hbotOff" )
		->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasEditor::onHBOffset ) );

	mUIContainer->find<UIPopUpMenu>( "fileMenu" )
		->addEventListener( Event::OnItemClicked,
							cb::Make1( this, &TextureAtlasEditor::fileMenuClick ) );

	if ( NULL != mUIWindow ) {
		mUIWindow->setTitle( "Texture Atlas Editor" );
		mUIWindow->addEventListener( Event::OnWindowClose,
									 cb::Make1( this, &TextureAtlasEditor::windowClose ) );
	} else {
		mUIContainer->addEventListener( Event::OnClose,
										cb::Make1( this, &TextureAtlasEditor::windowClose ) );
		mUIContainer->find<UINode>( "texture_atlas_editor_root" )
			->setThemeSkin( mUIContainer->getSceneNode()
								->asType<UISceneNode>()
								->getUIThemeManager()
								->getDefaultTheme(),
							"winback" );
	}
}

TextureAtlasEditor::~TextureAtlasEditor() {
	eeSAFE_DELETE( mTexturePacker );
	eeSAFE_DELETE( mTextureAtlasLoader );
}

void TextureAtlasEditor::onResetDestSize( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( NULL != mCurTextureRegion && mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizef RealSize(
			mCurTextureRegion->getRealSize().getWidth() * mCurTextureRegion->getPixelDensity(),
			mCurTextureRegion->getRealSize().getHeight() * mCurTextureRegion->getPixelDensity() );

		mCurTextureRegion->setOriDestSize( Sizef( RealSize.x, RealSize.y ) );

		mSpinDestW->setValue( RealSize.getWidth() );
		mSpinDestH->setValue( RealSize.getHeight() );
		mEdited = true;
	}
}

void TextureAtlasEditor::onResetOffset( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( NULL != mCurTextureRegion && mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mSpinOffX->setValue( 0 );
		mSpinOffY->setValue( 0 );
		mEdited = true;
	}
}

void TextureAtlasEditor::onCenterOffset( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( NULL != mCurTextureRegion && mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurTextureRegion->getDpSize().x / 2 ),
					 -( (Int32)mCurTextureRegion->getDpSize().y / 2 ) );

		mSpinOffX->setValue( NSize.x );
		mSpinOffY->setValue( NSize.y );
		mEdited = true;
	}
}

void TextureAtlasEditor::onHBOffset( const Event* Event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( Event );

	if ( NULL != mCurTextureRegion && mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurTextureRegion->getDpSize().x / 2 ),
					 -(Int32)mCurTextureRegion->getDpSize().y );

		mSpinOffX->setValue( NSize.x );
		mSpinOffY->setValue( NSize.y );
		mEdited = true;
	}
}

void TextureAtlasEditor::onOffXChange( const Event* ) {
	if ( NULL != mCurTextureRegion ) {
		mCurTextureRegion->setOffset(
			Vector2i( (Int32)mSpinOffX->getValue(), mCurTextureRegion->getOffset().y ) );
		mEdited = true;
	}
}

void TextureAtlasEditor::onOffYChange( const Event* ) {
	if ( NULL != mCurTextureRegion ) {
		mCurTextureRegion->setOffset(
			Vector2i( mCurTextureRegion->getOffset().x, (Int32)mSpinOffY->getValue() ) );
		mEdited = true;
	}
}

void TextureAtlasEditor::onDestWChange( const Event* ) {
	if ( NULL != mCurTextureRegion ) {
		mCurTextureRegion->setOriDestSize(
			Sizef( (Int32)mSpinDestW->getValue(), mCurTextureRegion->getDpSize().y ) );
		mTextureRegionEditor->getGfx()->setSize(
			(Int32)mSpinDestW->getValue(), mTextureRegionEditor->getGfx()->getSize().getHeight() );
		mEdited = true;
	}
}

void TextureAtlasEditor::onDestHChange( const Event* ) {
	if ( NULL != mCurTextureRegion ) {
		mCurTextureRegion->setOriDestSize(
			Sizef( mCurTextureRegion->getDpSize().x, (Int32)mSpinDestH->getValue() ) );
		mTextureRegionEditor->getGfx()->setSize(
			mTextureRegionEditor->getGfx()->getSize().getWidth(), (Int32)mSpinDestH->getValue() );
		mEdited = true;
	}
}

void TextureAtlasEditor::windowClose( const Event* ) {
	if ( mCloseCb )
		mCloseCb();

	eeDelete( this );
}

void TextureAtlasEditor::fileMenuClick( const Event* Event ) {
	if ( !Event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = Event->getNode()->asType<UIMenuItem>()->getText();

	if ( "New..." == txt ) {
		eeNew( TextureAtlasNew, ( cb::Make1( this, &TextureAtlasEditor::onTextureAtlasCreate ) ) );
	} else if ( "Open..." == txt ) {
		UIFileDialog* TGDialog = UIFileDialog::New(
			UIFileDialog::DefaultFlags, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open Texture Atlas" );
		TGDialog->addEventListener( Event::OpenFile,
									cb::Make1( this, &TextureAtlasEditor::openTextureAtlas ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
			mTextureAtlasLoader->updateTextureAtlas();
		}
	} else if ( "Close" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
			UIMessageBox* MsgBox = UIMessageBox::New( UIMessageBox::OK_CANCEL,
													  "Do you really want to close the current "
													  "texture atlas?\nAll changes will be lost." );
			MsgBox->addEventListener( Event::MsgBoxConfirmClick,
									  cb::Make1( this, &TextureAtlasEditor::onTextureAtlasClose ) );
			MsgBox->setTitle( "Close Texture Atlas?" );
			MsgBox->center();
			MsgBox->show();
		} else {
			onTextureAtlasClose( NULL );
		}
	} else if ( "Quit" == txt ) {
		if ( NULL == mUIWindow ) {
			mUIContainer->getSceneNode()->getWindow()->close();
		} else {
			mUIWindow->closeWindow();
		}
	}
}

void TextureAtlasEditor::onTextureFilterChange( const Event* ) {
	if ( NULL == mTextureAtlasLoader || NULL == mTextureAtlasLoader->getTextureAtlas() ||
		 !mTextureAtlasLoader->isLoaded() )
		return;

	Texture::Filter textureFilter = mTextureFilterList->getText() == "Nearest"
										? Texture::Filter::Nearest
										: Texture::Filter::Linear;

	mTextureAtlasLoader->setTextureFilter( textureFilter );
}

void TextureAtlasEditor::onTextureAtlasCreate( TexturePacker* TexPacker ) {
	eeSAFE_DELETE( mTexturePacker );
	mTexturePacker = TexPacker;

	eeSAFE_DELETE( mTextureAtlasLoader );

	std::string FPath( FileSystem::fileRemoveExtension( mTexturePacker->getFilepath() +
														EE_TEXTURE_ATLAS_EXTENSION ) );

	mTextureAtlasLoader =
		TextureAtlasLoader::New( FPath, Engine::instance()->isThreaded(),
								 cb::Make1( this, &TextureAtlasEditor::onTextureAtlasLoaded ) );
}

void TextureAtlasEditor::updateWidgets() {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
		mTextureFilterList->getListBox()->setSelected(
			mTextureAtlasLoader->getTextureAtlasHeader().TextureFilter );

		fillTextureRegionList();
	}
}

void TextureAtlasEditor::fillTextureRegionList() {
	if ( NULL == mTextureAtlasLoader || NULL == mTextureAtlasLoader->getTextureAtlas() ||
		 !mTextureAtlasLoader->isLoaded() )
		return;

	auto& res = mTextureAtlasLoader->getTextureAtlas()->getResources();

	mTextureRegionList->clear();

	std::vector<String> items;

	for ( auto& it : res ) {
		items.push_back( it.second->getName() );
	}

	if ( items.size() ) {
		std::sort( items.begin(), items.end() );

		mTextureRegionList->addListBoxItems( items );
		mTextureRegionList->setSelected( 0 );
	}

	mTextureRegionList->getVerticalScrollBar()->setClickStep(
		8.f / (Float)mTextureRegionList->getCount() );

	if ( !res.empty() ) {
		mTextureRegionGrid->childsCloseAll();

		for ( auto& it : res ) {
			TextureRegion* tr = it.second;

			UITextureRegion::New()
				->setTextureRegion( tr )
				->setScaleType( UIScaleType::FitInside )
				->setTooltipText( tr->getName() )
				->setGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER )
				->setParent( mTextureRegionGrid )
				->addEventListener( Event::MouseClick,
									cb::Make1( this, &TextureAtlasEditor::onTextureRegionChange ) );
			;
		}
	}
}

void TextureAtlasEditor::onTextureRegionChange( const Event* Event ) {
	if ( NULL != mTextureAtlasLoader && NULL != mTextureAtlasLoader->getTextureAtlas() ) {
		mCurTextureRegion = Event->getNode()->isType( UI_TYPE_TEXTUREREGION )
								? mTextureAtlasLoader->getTextureAtlas()->getByName(
									  static_cast<UIWidget*>( Event->getNode() )->getTooltipText() )
								: mTextureAtlasLoader->getTextureAtlas()->getByName(
									  mTextureRegionList->getItemSelectedText() );

		if ( Event->getNode()->isType( UI_TYPE_TEXTUREREGION ) )
			mTextureRegionList->setSelected(
				static_cast<UITextureRegion*>( Event->getNode() )->getTooltipText() );

		Node* node = mTextureRegionGrid->getFirstChild();

		while ( node ) {
			if ( node->isType( UI_TYPE_TEXTUREREGION ) ) {
				UITextureRegion* curImage = static_cast<UITextureRegion*>( node );

				if ( curImage->getTextureRegion() == mCurTextureRegion ) {
					curImage->setBackgroundColor( Color( "#00000033" ) );
				} else {
					curImage->setBackgroundFillEnabled( false );
				}
			}

			node = node->getNextNode();
		};

		if ( NULL != mCurTextureRegion ) {
			mTextureRegionEditor->setTextureRegion( mCurTextureRegion );
			mSpinOffX->setValue( mCurTextureRegion->getOffset().x );
			mSpinOffY->setValue( mCurTextureRegion->getOffset().y );
			mSpinDestW->setValue( mCurTextureRegion->getDpSize().x );
			mSpinDestH->setValue( mCurTextureRegion->getDpSize().y );
		}
	}
}

void TextureAtlasEditor::openTextureAtlas( const Event* Event ) {
	eeSAFE_DELETE( mTextureAtlasLoader );

	mTextureAtlasLoader = TextureAtlasLoader::New(
		Event->getNode()->asType<UIFileDialog>()->getFullPath(), Engine::instance()->isThreaded(),
		cb::Make1( this, &TextureAtlasEditor::onTextureAtlasLoaded ) );
}

void TextureAtlasEditor::onTextureAtlasLoaded( TextureAtlasLoader* textureAtlasLoader ) {
	mTextureAtlasLoader = textureAtlasLoader;

	if ( mTextureAtlasLoader->isLoaded() ) {
		mUIContainer->runOnMainThread( [&] { updateWidgets(); } );
	}
}

void TextureAtlasEditor::saveTextureAtlas( const Event* ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
		mTextureAtlasLoader->updateTextureAtlas();
	}
}

void TextureAtlasEditor::onTextureAtlasClose( const Event* ) {
	if ( NULL != mTextureAtlasLoader && NULL != mTextureAtlasLoader->getTextureAtlas() )
		TextureAtlasManager::instance()->remove( mTextureAtlasLoader->getTextureAtlas() );
	eeSAFE_DELETE( mTextureAtlasLoader );
	mTextureRegionList->clear();
	mTextureRegionGrid->childsCloseAll();
	mSpinOffX->setValue( 0 );
	mSpinOffY->setValue( 0 );
	mSpinDestW->setValue( 0 );
	mSpinDestH->setValue( 0 );
	mTextureRegionEditor->setTextureRegion( NULL );
	mCurTextureRegion = NULL;
}

}}} // namespace EE::UI::Tools
