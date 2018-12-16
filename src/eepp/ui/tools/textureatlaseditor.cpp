#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/ui/tools/textureatlastextureregioneditor.hpp>
#include <eepp/ui/tools/textureatlasnew.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <algorithm>

namespace EE { namespace UI { namespace Tools {

UIWidget * TextureAtlasEditor::createTextureAtlasTextureRegionEditor( std::string name ) {
	mTextureRegionEditor = TextureAtlasTextureRegionEditor::New( this );
	return mTextureRegionEditor;
}

TextureAtlasEditor *TextureAtlasEditor::New(UIWindow * AttatchTo, const TextureAtlasEditor::TGEditorCloseCb & callback) {
	return eeNew( TextureAtlasEditor, ( AttatchTo, callback ) );
}

TextureAtlasEditor::TextureAtlasEditor( UIWindow * AttatchTo, const TGEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mCloseCb( callback ),
	mTexturePacker( NULL ),
	mTextureAtlasLoader( NULL ),
	mCurTextureRegion( NULL ),
	mEdited( false )
{
	if ( NULL == UIThemeManager::instance()->getDefaultTheme() ) {
		eePRINTL( "TextureAtlasEditor needs a default theme assigned to work." );
		return;
	}

	mTheme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mUIWindow ) {
		mUIContainer = SceneManager::instance()->getUISceneNode();
	} else {
		mUIContainer = mUIWindow->getContainer();
	}

	std::string layout = R"xml(
	<LinearLayout id="texture_atlas_editor_root" orientation="vertical" layout_width="match_parent" layout_height="match_parent">
		<WinMenu layout_width="match_parent" layout_height="wrap_content">
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
		</WinMenu>
		<LinearLayout layout_width="match_parent" layout_height="0dp" layout_weight="1" orientation="horizontal">
			<TextureAtlasTextureRegionEditor layout_width="match_parent" layout_height="match_parent" layout_weight="1"
											flags="clip" backgroundColor="#00000032" borderWidth="1" borderColor="#000000FF" />
			<LinearLayout orientation="vertical" layout_width="210dp" layout_height="match_parent" layout_marginLeft="8dp" layout_marginRight="8dp">
				<TextView text="Texture Filter:" fontStyle="shadow" layout_marginTop="4dp" layout_marginBottom="4dp" />
				<DropDownList id="textureFilter" layout_width="match_parent" layout_height="wrap_content" layout_gravity="center_vertical" selectedText="Linear">
					<item>Linear</item>
					<item>Nearest</item>
				</DropDownList>
				<TextView text="TextureRegion List:" fontStyle="shadow" layout_marginTop="8dp" layout_marginBottom="8dp" />
				<TabWidget layout_width="match_parent" layout_height="0dp" layout_weight="1">
					<ScrollView id="GridView" layout_width="match_parent" layout_height="144dp" touchdrag="true">
						<GridLayout columnMode="size" rowMode="size" columnWidth="64dp" rowHeight="64dp" layout_width="match_parent" layout_height="wrap_content" id="gridlayout" />
					</ScrollView>
					<ListBox id="TextureRegionList" layout_width="match_parent" layout_height="144dp" touchDrag="true" />
					<Tab name="List" owns="TextureRegionList" />
					<Tab name="Grid" owns="GridView" />
				</TabWidget>
				<TextView text="Current TextureRegion:" fontStyle="shadow" layout_marginTop="16dp" layout_marginBottom="16dp" />
				<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
					<TextView text="Offset X:" fontStyle="shadow" layout_width="match_parent" layout_height="wrap_content" layout_weight="1"
									layout_marginRight="8dp" layout_gravity="center" gravity="right|center_vertical" />
					<SpinBox id="offX" layout_width="100dp" layout_height="wrap_content" minValue="-32000" maxValue="32000" />
				</LinearLayout>
				<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
					<TextView text="Offset Y:" fontStyle="shadow" layout_width="match_parent" layout_height="wrap_content" layout_weight="1"
									layout_marginRight="8dp" layout_gravity="center" gravity="right|center_vertical" />
					<SpinBox id="offY" layout_width="100dp" layout_height="wrap_content" minValue="-32000" maxValue="32000" />
				</LinearLayout>
				<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
					<TextView text="Dest. Width" fontStyle="shadow" layout_width="match_parent" layout_height="wrap_content" layout_weight="1"
									layout_marginRight="8dp" layout_gravity="center" gravity="right|center_vertical" />
					<SpinBox id="destW" layout_width="100dp" layout_height="wrap_content" minValue="0" maxValue="32000" />
				</LinearLayout>
				<LinearLayout orientation="horizontal" layout_width="match_parent" layout_height="wrap_content">
					<TextView text="Dest. Height" fontStyle="shadow" layout_width="match_parent" layout_height="wrap_content" layout_weight="1"
									layout_marginRight="8dp" layout_gravity="center" gravity="right|center_vertical" />
					<SpinBox id="destH" layout_width="100dp" layout_height="wrap_content" minValue="0" maxValue="32000" />
				</LinearLayout>
				<PushButton id="resetDest" text="Reset Dest. Size" layout_width="match_parent" layout_height="wrap_content" layout_marginBottom="8dp" layout_marginTop="8dp" />
				<PushButton id="resetOff" text="Reset Offset" layout_width="match_parent" layout_height="wrap_content" layout_marginBottom="8dp" />
				<PushButton id="centerOff" text="Centered Offset" layout_width="match_parent" layout_height="wrap_content" layout_marginBottom="8dp" />
				<PushButton id="hbotOff" text="Half-Bottom Offset" layout_width="match_parent" layout_height="wrap_content" layout_marginBottom="8dp" />
			</LinearLayout>
		</LinearLayout>
	</LinearLayout>
	)xml";

	UIWidgetCreator::addCustomWidgetCallback( "TextureAtlasTextureRegionEditor", cb::Make1( this, &TextureAtlasEditor::createTextureAtlasTextureRegionEditor ) );

	if ( NULL != mUIContainer->getSceneNode() && mUIContainer->getSceneNode()->isUISceneNode() )
		static_cast<UISceneNode*>( mUIContainer->getSceneNode() )->loadLayoutFromString( layout, mUIContainer );

	UIWidgetCreator::removeCustomWidgetCallback( "TextureAtlasTextureRegionEditor" );

	mUIContainer->bind( "TextureRegionList", mTextureRegionList );
	mTextureRegionList->addEventListener( Event::OnItemSelected, cb::Make1( this, &TextureAtlasEditor::onTextureRegionChange ) );

	mUIContainer->bind( "gridlayout", mTextureRegionGrid );

	mUIContainer->bind( "offX", mSpinOffX );
	mSpinOffX->addEventListener( Event::OnValueChange, cb::Make1( this, &TextureAtlasEditor::onOffXChange ) );

	mUIContainer->bind( "offY", mSpinOffY );
	mSpinOffY->addEventListener( Event::OnValueChange, cb::Make1( this, &TextureAtlasEditor::onOffYChange ) );

	mUIContainer->bind( "destW", mSpinDestW );
	mSpinDestW->addEventListener( Event::OnValueChange, cb::Make1( this, &TextureAtlasEditor::onDestWChange ) );

	mUIContainer->bind( "destH", mSpinDestH );
	mSpinDestH->addEventListener( Event::OnValueChange, cb::Make1( this, &TextureAtlasEditor::onDestHChange ) );

	mUIContainer->bind( "textureFilter", mTextureFilterList );
	mTextureFilterList->addEventListener( Event::OnItemSelected, cb::Make1( this, &TextureAtlasEditor::onTextureFilterChange ) );

	mUIContainer->find<UIPushButton>( "resetDest" )->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasEditor::onResetDestSize ) );

	mUIContainer->find<UIPushButton>( "resetOff" )->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasEditor::onResetOffset ) );

	mUIContainer->find<UIPushButton>( "centerOff" )->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasEditor::onCenterOffset ) );

	mUIContainer->find<UIPushButton>( "hbotOff" )->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasEditor::onHBOffset ) );

	mUIContainer->find<UIPopUpMenu>("fileMenu")->addEventListener( Event::OnItemClicked, cb::Make1( this, &TextureAtlasEditor::fileMenuClick ) );

	if ( NULL != mUIWindow ) {
		mUIWindow->setTitle( "Texture Atlas Editor" );
		mUIWindow->addEventListener( Event::OnWindowClose, cb::Make1( this, &TextureAtlasEditor::windowClose ) );
	} else {
		mUIContainer->addEventListener( Event::OnClose, cb::Make1( this, &TextureAtlasEditor::windowClose ) );
		static_cast<UINode*>( mUIContainer->find("texture_atlas_editor_root") )->setThemeSkin( mTheme, "winback" );
	}

	mTGEU = eeNew( UITGEUpdater, ( this ) );
}

TextureAtlasEditor::~TextureAtlasEditor() {
	eeSAFE_DELETE( mTexturePacker );
	eeSAFE_DELETE( mTextureAtlasLoader );

	if ( !SceneManager::instance()->isShootingDown() ) {
		mTGEU->close();
	}
}

void TextureAtlasEditor::onResetDestSize( const Event * event ) {
	const MouseEvent * mouseEvent = reinterpret_cast<const MouseEvent*> ( event );

	if ( NULL != mCurTextureRegion && mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizef RealSize( mCurTextureRegion->getRealSize().getWidth() * mCurTextureRegion->getPixelDensity(), mCurTextureRegion->getRealSize().getHeight() * mCurTextureRegion->getPixelDensity() );

		mCurTextureRegion->setOriDestSize( Sizef( RealSize.x, RealSize.y ) );

		mSpinDestW->setValue( RealSize.getWidth() );
		mSpinDestH->setValue( RealSize.getHeight() );
		mEdited = true;
	}
}

void TextureAtlasEditor::onResetOffset( const Event * event ) {
	const MouseEvent * mouseEvent = reinterpret_cast<const MouseEvent*> ( event );

	if ( NULL != mCurTextureRegion && mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mSpinOffX->setValue( 0 );
		mSpinOffY->setValue( 0 );
		mEdited = true;
	}
}

void TextureAtlasEditor::onCenterOffset( const Event * event ) {
	const MouseEvent * mouseEvent = reinterpret_cast<const MouseEvent*> ( event );

	if ( NULL != mCurTextureRegion && mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurTextureRegion->getDpSize().x / 2 ), -( (Int32)mCurTextureRegion->getDpSize().y / 2 ) );

		mSpinOffX->setValue( NSize.x );
		mSpinOffY->setValue( NSize.y );
		mEdited = true;
	}
}

void TextureAtlasEditor::onHBOffset( const Event * Event ) {
	const MouseEvent * mouseEvent = reinterpret_cast<const MouseEvent*> ( Event );

	if ( NULL != mCurTextureRegion && mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurTextureRegion->getDpSize().x / 2 ), -(Int32)mCurTextureRegion->getDpSize().y );

		mSpinOffX->setValue( NSize.x );
		mSpinOffY->setValue( NSize.y );
		mEdited = true;
	}
}

void TextureAtlasEditor::onOffXChange( const Event * Event ) {
	if ( NULL != mCurTextureRegion ) {
		mCurTextureRegion->setOffset( Vector2i( (Int32)mSpinOffX->getValue(), mCurTextureRegion->getOffset().y ) );
		mEdited = true;
	}
}

void TextureAtlasEditor::onOffYChange( const Event * Event ) {
	if ( NULL != mCurTextureRegion ) {
		mCurTextureRegion->setOffset( Vector2i( mCurTextureRegion->getOffset().x, (Int32)mSpinOffY->getValue() ) );
		mEdited = true;
	}
}

void TextureAtlasEditor::onDestWChange( const Event * Event ) {
	if ( NULL != mCurTextureRegion ) {
		mCurTextureRegion->setOriDestSize( Sizef( (Int32)mSpinDestW->getValue(), mCurTextureRegion->getDpSize().y ) );
		mTextureRegionEditor->getGfx()->setSize( (Int32)mSpinDestW->getValue(), mTextureRegionEditor->getGfx()->getSize().getHeight() );
		mEdited = true;
	}
}

void TextureAtlasEditor::onDestHChange( const Event * Event ) {
	if ( NULL != mCurTextureRegion ) {
		mCurTextureRegion->setOriDestSize( Sizef( mCurTextureRegion->getDpSize().x, (Int32)mSpinDestH->getValue() ) );
		mTextureRegionEditor->getGfx()->setSize( mTextureRegionEditor->getGfx()->getSize().getWidth(), (Int32)mSpinDestH->getValue() );
		mEdited = true;
	}
}

void TextureAtlasEditor::windowClose( const Event * Event ) {
	if ( mCloseCb )
		mCloseCb();

	eeDelete( this );
}

void TextureAtlasEditor::fileMenuClick( const Event * Event ) {
	if ( !Event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getNode() )->getText();

	if ( "New..." == txt ) {
		eeNew( TextureAtlasNew, ( cb::Make1( this, &TextureAtlasEditor::onTextureAtlasCreate ) ) );
	} else if ( "Open..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open Texture Atlas" );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( this, &TextureAtlasEditor::openTextureAtlas ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
			mTextureAtlasLoader->updateTextureAtlas();
		}
	} else if ( "Close" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded()  ) {
			UIMessageBox * MsgBox = UIMessageBox::New( MSGBOX_OKCANCEL, "Do you really want to close the current texture atlas?\nAll changes will be lost." );
			MsgBox->addEventListener( Event::MsgBoxConfirmClick, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasClose ) );
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

void TextureAtlasEditor::onTextureFilterChange( const Event * Event ) {
	if ( NULL == mTextureAtlasLoader || NULL == mTextureAtlasLoader->getTextureAtlas() || !mTextureAtlasLoader->isLoaded() )
		return;

	Texture::TextureFilter textureFilter = mTextureFilterList->getText() == "Nearest" ? Texture::TextureFilter::Nearest : Texture::TextureFilter::Linear;

	mTextureAtlasLoader->setTextureFilter( textureFilter );
}

void TextureAtlasEditor::onTextureAtlasCreate( TexturePacker * TexPacker ) {
	eeSAFE_DELETE( mTexturePacker );
	mTexturePacker = TexPacker;

	eeSAFE_DELETE( mTextureAtlasLoader );

	std::string FPath( FileSystem::fileRemoveExtension( mTexturePacker->getFilepath() + EE_TEXTURE_ATLAS_EXTENSION ) );

	mTextureAtlasLoader = eeNew( TextureAtlasLoader, ( FPath, true, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasLoaded ) ) );
}

void TextureAtlasEditor::updateControls() {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded()  ) {
		mTextureFilterList->getListBox()->setSelected( mTextureAtlasLoader->getTextureAtlasHeader().TextureFilter );

		fillTextureRegionList();
	}
}

void TextureAtlasEditor::fillTextureRegionList() {
	if ( NULL == mTextureAtlasLoader || NULL == mTextureAtlasLoader->getTextureAtlas() || !mTextureAtlasLoader->isLoaded()  )
		return;

	std::list<TextureRegion*>& Res = mTextureAtlasLoader->getTextureAtlas()->getResources();

	mTextureRegionList->clear();

	std::vector<String> items;

	for ( std::list<TextureRegion*>::iterator it = Res.begin(); it != Res.end(); ++it ) {
			items.push_back( (*it)->getName() );
	}

	if ( items.size() ) {
		std::sort( items.begin(), items.end() );

		mTextureRegionList->addListBoxItems( items );
		mTextureRegionList->setSelected( 0 );
	}

	mTextureRegionList->getVerticalScrollBar()->setClickStep( 8.f / (Float)mTextureRegionList->getCount() );

	if ( Res.size() > 0 ) {
		mTextureRegionGrid->childsCloseAll();

		for ( auto it = Res.begin(); it != Res.end(); ++it ) {
			TextureRegion * tr = (*it);

			UITextureRegion::New()
					->setTextureRegion( tr )
					->setScaleType( UIScaleType::FitInside )
					->setTooltipText( tr->getName() )
					->setGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER )
					->setParent( mTextureRegionGrid )
					->addEventListener( Event::MouseClick, cb::Make1( this, &TextureAtlasEditor::onTextureRegionChange ) );;
		}
	}
}

void TextureAtlasEditor::onTextureRegionChange( const Event * Event ) {
	if ( NULL != mTextureAtlasLoader && NULL != mTextureAtlasLoader->getTextureAtlas() ) {
		mCurTextureRegion = Event->getNode()->isType( UI_TYPE_TEXTUREREGION ) ?
							mTextureAtlasLoader->getTextureAtlas()->getByName( static_cast<UIWidget*>( Event->getNode() )->getTooltipText() ) :
							mTextureAtlasLoader->getTextureAtlas()->getByName( mTextureRegionList->getItemSelectedText() );

		if ( Event->getNode()->isType( UI_TYPE_TEXTUREREGION ) )
			mTextureRegionList->setSelected( static_cast<UITextureRegion*>( Event->getNode() )->getTooltipText() );

		Node * node = mTextureRegionGrid->getFirstChild();

		while ( node ) {
			if ( node->isType( UI_TYPE_TEXTUREREGION ) ) {
				UITextureRegion * curImage = static_cast<UITextureRegion*>( node );

				if ( curImage->getTextureRegion() == mCurTextureRegion ) {
					curImage->setBackgroundFillEnabled( true )->setColor( Color( "#00000033" ) );
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

void TextureAtlasEditor::update() {
	if ( NULL != mTextureAtlasLoader && !mTextureAtlasLoader->isLoaded() ) {
		mTextureAtlasLoader->update();
	}
}

void TextureAtlasEditor::openTextureAtlas( const Event * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getNode() );

	eeSAFE_DELETE( mTextureAtlasLoader );
	bool threaded = true;
	#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	threaded = false;
	#endif

	mTextureAtlasLoader = eeNew( TextureAtlasLoader, ( CDL->getFullPath(), threaded, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasLoaded ) ) );
}

void TextureAtlasEditor::onTextureAtlasLoaded( TextureAtlasLoader * TGLoader ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
		updateControls();
	}
}

void TextureAtlasEditor::saveTextureAtlas( const Event * Event ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
		mTextureAtlasLoader->updateTextureAtlas();
	}
}

void TextureAtlasEditor::onTextureAtlasClose( const Event * Event ) {
	eeSAFE_DELETE( mTextureAtlasLoader );
	mTextureRegionList->clear();
	mSpinOffX->setValue( 0 );
	mSpinOffY->setValue( 0 );
	mSpinDestW->setValue( 0 );
	mSpinDestH->setValue( 0 );
	mTextureRegionEditor->setTextureRegion( NULL );
	mCurTextureRegion = NULL;
}

}}}
