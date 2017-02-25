#include <eepp/gaming/mapeditor/mapeditor.hpp>
#include <eepp/gaming/mapeditor/uimapnew.hpp>
#include <eepp/gaming/mapeditor/uimaplayernew.hpp>
#include <eepp/gaming/mapeditor/uigotypenew.hpp>
#include <eepp/gaming/mapeditor/tilemapproperties.hpp>
#include <eepp/gaming/mapeditor/maplayerproperties.hpp>
#include <eepp/gaming/mapeditor/uimap.hpp>

#include <eepp/gaming/tilemaplayer.hpp>
#include <eepp/gaming/mapobjectlayer.hpp>
#include <eepp/gaming/gameobjectvirtual.hpp>
#include <eepp/gaming/gameobjectsubtexture.hpp>
#include <eepp/gaming/gameobjectsubtextureex.hpp>
#include <eepp/gaming/gameobjectsprite.hpp>
#include <eepp/gaming/gameobjectobject.hpp>
#include <eepp/gaming/gameobjectpolygon.hpp>
#include <eepp/gaming/gameobjectpolyline.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>

#include <algorithm>

using namespace EE::Graphics;
using namespace EE::Gaming::Private;

#define TAB_CONT_X_DIST 3

namespace EE { namespace Gaming {

MapEditor::MapEditor( UIWindow * AttatchTo, const MapEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mTheme( UIThemeManager::instance()->getDefaultTheme() ),
	mUIMap( NULL ),
	mCloseCb( callback ),
	mGOTypeList( NULL ),
	mChkAnim( NULL ),
	mCurLayer( NULL ),
	mLastSelButtonY(2),
	mMouseScrolling( false )
{
	if ( NULL == mTheme ) {
		eePRINTL( "MapEditor needs a default theme seted to work." );
		return;
	}

	if ( NULL == mUIWindow ) {
		mUIWindow = UIManager::instance()->getMainControl();
		mUIWindow->setSkinFromTheme( mTheme, "winback" );
	}

	if ( UIManager::instance()->getMainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->getContainer();
	}

	mUIWindow->setTitle( "Map Editor" );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &MapEditor::WindowClose ) );

	CreateME();
}

MapEditor::~MapEditor() {
}

void MapEditor::CreateME() {
	CreateWinMenu();

	CreateETGMenu();

	CreateUIMap();
}

void MapEditor::CreateWinMenu() {
	UIWinMenu * WinMenu = mTheme->createWinMenu( mUIContainer );

	mTileBox = mTheme->createTextBox( "", mUIContainer, Sizei(), Vector2i(), UI_HALIGN_RIGHT | UI_VALIGN_CENTER | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	mTileBox->setSize( 100, WinMenu->getSize().getHeight() );
	mTileBox->setPosition( Vector2i( mUIContainer->getSize().getWidth() - mTileBox->getSize().getWidth(), 0 ) );
	mTileBox->updateAnchorsDistances();

	UIPopUpMenu * PU1 = mTheme->createPopUpMenu( mUIContainer );
	PU1->add( "New...", mTheme->getIconByName( "document-new" ) );
	PU1->add( "Open...", mTheme->getIconByName( "document-open" ) );
	PU1->addSeparator();
	PU1->add( "Save", mTheme->getIconByName( "document-save" ) );
	PU1->add( "Save As...", mTheme->getIconByName( "document-save-as" ) );
	PU1->addSeparator();
	PU1->add( "Close", mTheme->getIconByName( "document-close" ) );
	PU1->addSeparator();
	PU1->add( "Quit", mTheme->getIconByName( "quit" ) );

	PU1->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::FileMenuClick ) );
	WinMenu->addMenuButton( "File", PU1 );

	UIPopUpMenu * PU3 = mTheme->createPopUpMenu( mUIContainer );
	mChkShowGrid = reinterpret_cast<UIMenuCheckBox*>( PU3->getItem( PU3->addCheckBox( "Show Grid" ) ) );

	mChkShowGrid->setActive( true );

	mChkMarkTileOver = reinterpret_cast<UIMenuCheckBox*>( PU3->getItem( PU3->addCheckBox( "Mark Tile Over" ) ) );

	mChkShowBlocked = reinterpret_cast<UIMenuCheckBox*>( PU3->getItem( PU3->addCheckBox( "Show Blocked" ) ) );

	PU3->addSeparator();
	mUIWindow->addShortcut( KEY_KP_PLUS	, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->getItem( PU3->add( "Zoom In", mTheme->getIconByName( "zoom-in" ) ) ) ) );
	mUIWindow->addShortcut( KEY_KP_MINUS, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->getItem( PU3->add( "Zoom Out", mTheme->getIconByName( "zoom-out" ) ) ) ) );
	mUIWindow->addShortcut( KEY_KP0		, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->getItem( PU3->add( "Normal Size", mTheme->getIconByName( "zoom-original" ) ) ) ) );
	PU3->addSeparator();

	PU3->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::ViewMenuClick ) );
	WinMenu->addMenuButton( "View", PU3 );

	UIPopUpMenu * PU4 = mTheme->createPopUpMenu( mUIContainer );
	PU4->add( "Properties..." );
	PU4->add( "Resize..." );

	PU4->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::MapMenuClick ) );
	WinMenu->addMenuButton( "Map", PU4 );

	UIPopUpMenu * PU5 = mTheme->createPopUpMenu( mUIContainer );
	PU5->add( "Add Tile Layer..." );
	PU5->add( "Add Object Layer..." );
	PU5->addSeparator();
	PU5->add( "Remove Layer" );
	PU5->addSeparator();
	PU5->add( "Move Layer Up" );
	PU5->add( "Move Layer Down" );
	PU5->addSeparator();
	PU5->add( "Layer Properties..." );
	PU5->addSeparator();

	Uint32 LayerChkBoxIndex = PU5->addCheckBox( "Lights Enabled" );
	mLayerChkLights = reinterpret_cast<UIMenuCheckBox*> ( PU5->getItem( LayerChkBoxIndex ) );

	PU5->addSeparator();

	LayerChkBoxIndex = PU5->addCheckBox( "Visible" );
	mLayerChkVisible = reinterpret_cast<UIMenuCheckBox*> ( PU5->getItem( LayerChkBoxIndex ) );

	PU5->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::LayerMenuClick ) );
	WinMenu->addMenuButton( "Layer", PU5 );

	UIPopUpMenu * PU6 = mTheme->createPopUpMenu( mUIContainer );
	PU6->add( "New Texture Atlas..." );
	PU6->add( "Add External Texture Atlas..." );
	WinMenu->addMenuButton( "Atlases", PU6 );
	PU6->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::MapMenuClick ) );

	UIComplexControl::CreateParams Params;
	Params.setParent( mUIContainer );
	Params.setPosition( 0, WinMenu->getSize().getHeight() );
	Params.setSize( mUIContainer->getSize().getWidth(), mUIContainer->getSize().getHeight() - WinMenu->getSize().getHeight() );
	Params.Flags = UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_REPORT_SIZE_CHANGE_TO_CHILDS;
	mWinContainer = eeNew( UIComplexControl, ( Params ) );
	mWinContainer->setVisible( true );
	mWinContainer->setEnabled( true );
}

void MapEditor::CreateETGMenu() {
	Int32 Width = 200;
	Int32 DistToBorder = 5;
	Int32 ContPosX = mWinContainer->getSize().getWidth() - Width - DistToBorder;
	Int32 DistFromTopMenu = 4;

	UIComplexControl::CreateParams CParams;
	CParams.setParent( mWinContainer );
	CParams.setSize( Sizei( Width + DistToBorder, mWinContainer->getSize().getHeight() ) );
	CParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mSubTextureCont = eeNew( UIComplexControl, ( CParams ) );
	mSubTextureCont->setEnabled( true );
	mSubTextureCont->setVisible( true );

	mLightCont = eeNew( UIComplexControl, ( CParams ) );

	mObjectCont = eeNew( UIComplexControl, ( CParams ) );

	mTabWidget = mTheme->createTabWidget( mWinContainer, Sizei( Width + DistToBorder, mWinContainer->getSize().getHeight() - DistFromTopMenu ), Vector2i( ContPosX, DistFromTopMenu ), UI_HALIGN_CENTER | UI_VALIGN_BOTTOM | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM );
	mTabWidget->addEventListener( UIEvent::EventOnTabSelected, cb::Make1( this, &MapEditor::OnTabSelected ) );
	CreateTabs();

	CreateLighContainer();

	CreateSubTextureContainer( Width );

	CreateObjectsContainer();
}

void MapEditor::CreateTabs() {
	mTabWidget->removeAll();
	mTabWidget->add( "Sprites", mSubTextureCont );

	if ( NULL != mUIMap && NULL != mUIMap->Map() ) {
		if ( mUIMap->Map()->getLightsEnabled() ) {
			mTabWidget->add( "Lights", mLightCont );
		}
	}

	mTabWidget->add( "Objects", mObjectCont );
}

void MapEditor::OnTabSelected( const UIEvent * Event ) {
	if ( NULL != mUIMap ) {
		switch ( mTabWidget->getSelectedTabIndex() ) {
			case 0:
				mUIMap->editingDisable();
				break;
			case 1:
				mUIMap->setEditingLights( true );
				break;
			case 2:
				mUIMap->setEditingObjects( true );
				break;
		}
	}
}

void MapEditor::FillGotyList() {
	std::vector<String> items;
	items.push_back( "SubTexture" );
	items.push_back( "SubTextureEx" );
	items.push_back( "Sprite" );
	mGOTypeList->getListBox()->clear();
	mGOTypeList->getListBox()->addListBoxItems( items );
	mGOTypeList->getListBox()->setSelected(0);
}

void MapEditor::CreateSubTextureContainer( Int32 Width ) {
	UITextBox * Txt;
	Uint32 TxtFlags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_DRAW_SHADOW;

	Txt = mTheme->createTextBox( "Add Game Object as...", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 4 ), TxtFlags );

	mGOTypeList = mTheme->createDropDownList( mSubTextureCont, Sizei( Width - 26, 21 ), Vector2i( TAB_CONT_X_DIST, Txt->getPosition().y + Txt->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mGOTypeList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnTypeChange ) );
	FillGotyList();

	mBtnGOTypeAdd = mTheme->createPushButton( mSubTextureCont, Sizei( 24, 21 ), Vector2i( mGOTypeList->getPosition().x + mGOTypeList->getSize().getWidth() + 2, mGOTypeList->getPosition().y ), UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mTheme->getIconByName( "add" ) );
	mBtnGOTypeAdd->setTooltipText( "Adds a new game object type\nunknown by the map editor." );
	mBtnGOTypeAdd->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::AddNewGOType ) );

	if ( NULL == mBtnGOTypeAdd->getIcon()->getSubTexture() )
		mBtnGOTypeAdd->setText( "..." );

	Txt = mTheme->createTextBox( "Layers:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mGOTypeList->getPosition().y + mGOTypeList->getSize().getHeight() + 4 ), TxtFlags );

	mLayerList = mTheme->createDropDownList( mSubTextureCont, Sizei( Width, 21 ), Vector2i( TAB_CONT_X_DIST, Txt->getPosition().y + Txt->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mLayerList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnLayerSelect ) );

	Txt = mTheme->createTextBox( "Game Object Flags:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mLayerList->getPosition().y + mLayerList->getSize().getHeight() + 4 ), TxtFlags );

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkMirrored = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( TAB_CONT_X_DIST, Txt->getPosition().y + Txt->getSize().getHeight() + 4 ), ChkFlags );
	mChkMirrored->setText( "Mirrored" );
	mChkMirrored->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickMirrored ) );

	mChkFliped = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkMirrored->getPosition().x + mChkMirrored->getSize().getWidth() + 32, mChkMirrored->getPosition().y ), ChkFlags );
	mChkFliped->setText( "Fliped" );
	mChkFliped->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickFliped ) );

	mChkBlocked = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkMirrored->getPosition().x, mChkMirrored->getPosition().y + mChkMirrored->getSize().getHeight() + 4 ), ChkFlags );
	mChkBlocked->setText( "Blocked" );
	mChkBlocked->setTooltipText( "Blocks the tile occupied by the sprite." );
	mChkBlocked->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickBlocked ) );

	mChkAnim = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkFliped->getPosition().x, mChkFliped->getPosition().y + mChkFliped->getSize().getHeight() + 4 ), ChkFlags );
	mChkAnim->setText( "Animated" );
	mChkAnim->setTooltipText( "Indicates if the Sprite is animated." );
	mChkAnim->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickAnimated ) );

	mChkRot90 = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkBlocked->getPosition().x, mChkBlocked->getPosition().y + mChkBlocked->getSize().getHeight() + 4 ), ChkFlags );
	mChkRot90->setText( String::fromUtf8( "Rotate 90º" ) );
	mChkRot90->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickRot90 ) );

	mChkAutoFix = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkAnim->getPosition().x, mChkAnim->getPosition().y + mChkAnim->getSize().getHeight() + 4 ), ChkFlags );
	mChkAutoFix->setText( "AutoFix TilePos" );
	mChkAutoFix->setTooltipText( "In a tiled layer if the sprite is moved,\nit will update the current tile position automatically." );
	mChkAutoFix->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickAutoFix ) );

	Txt = mTheme->createTextBox( "Game Object Data:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mChkRot90->getPosition().y + mChkRot90->getSize().getHeight() + 8 ), TxtFlags );

	mChkDI = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( TAB_CONT_X_DIST, Txt->getPosition().y + Txt->getSize().getHeight() + 4 ), ChkFlags );
	mChkDI->setText( "Add as DataId" );
	mChkDI->setTooltipText( "If the resource it's not a sprite,\nyou can reference it with a data id" );
	mChkDI->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickDI ) );

	UIComplexControl::CreateParams SGParams;
	SGParams.setParent( mSubTextureCont );
	SGParams.setPosition( Vector2i( TAB_CONT_X_DIST, mChkDI->getPosition().y + mChkDI->getSize().getHeight() + 8 ) );
	SGParams.setSize( Sizei( Width, 400 ) );
	SGParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mSGCont = eeNew( UIComplexControl, ( SGParams ) );
	mSGCont->setEnabled( true );
	mSGCont->setVisible( true );

	Txt = mTheme->createTextBox( "Texture Atlases:", mSGCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 0 ), TxtFlags );

	mTextureAtlasesList = mTheme->createDropDownList( mSGCont, Sizei( Width, 21 ), Vector2i( 0, Txt->getPosition().y +Txt->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mTextureAtlasesList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnTextureAtlasChange ) );

	mSubTextureList = mTheme->createListBox( mSGCont, Sizei( Width, 156 ), Vector2i( 0, mTextureAtlasesList->getPosition().y + mTextureAtlasesList->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mSubTextureList->setSize( mSubTextureList->getSize().getWidth(), mSubTextureList->getRowHeight() * 9 + mSubTextureList->getContainerPadding().Top + mSubTextureList->getContainerPadding().Bottom );
	mSubTextureList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnSubTextureChange ) );

	mGfxPreview = mTheme->createGfx( NULL, mSGCont, Sizei( Width, Width ), Vector2i( 0, mSubTextureList->getPosition().y + mSubTextureList->getSize().getHeight() + 4 ), UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_AUTO_FIT );
	mGfxPreview->setBorderEnabled( true );
	mGfxPreview->getBorder()->setColor( ColorA( 0, 0, 0, 200 ) );

	UIComplexControl::CreateParams DIParams;
	DIParams.setParent( mSubTextureCont );
	DIParams.setPosition( SGParams.Pos );
	DIParams.setSize( Sizei( Width, 400 ) );
	DIParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mDICont = eeNew( UIComplexControl, ( DIParams ) );
	mDICont->setEnabled( false );
	mDICont->setVisible( false );

	Txt = mTheme->createTextBox( "DataId String:", mDICont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 0 ), TxtFlags );

	mDataIdInput = mTheme->createTextInput( mDICont, Sizei( Width / 4 * 3, 21 ), Vector2i( TAB_CONT_X_DIST + 8, Txt->getPosition().y + Txt->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	FillSGCombo();
}

void MapEditor::CreateLighContainer() {
	UIPushButton * NewLightBut = mTheme->createPushButton( mLightCont, Sizei( mLightCont->getSize().getWidth() - TAB_CONT_X_DIST * 2, 22 ), Vector2i( TAB_CONT_X_DIST, 0 ) );
	NewLightBut->setText( "New Light" );
	NewLightBut->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnNewLight ) );

	UITextBox * Txt = mTheme->createTextBox( "Light Color:", mLightCont, Sizei(), Vector2i( TAB_CONT_X_DIST, 32 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	UIComplexControl::CreateParams ComParams;
	ComParams.setParent( mLightCont );
	ComParams.setPosition( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 4 );
	ComParams.setSize( 58, 64 );
	ComParams.Background.setColor( ColorA(255,255,255,255) );
	ComParams.Border.setColor( ColorA( 100, 100, 100, 200 ) );
	ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
	mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
	mUIBaseColor->setVisible( true );
	mUIBaseColor->setEnabled( true );

	Txt = mTheme->createTextBox( "R:", mLightCont, Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIBaseColor->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIRedSlider = mTheme->createSlider( mLightCont, Sizei( 100, 20 ), Vector2i( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIRedSlider->setMaxValue( 255 );
	mUIRedSlider->setValue( 255 );
	mUIRedSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnRedChange ) );

	mUIRedTxt = mTheme->createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4, mUIRedSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "G:", mLightCont, Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIGreenSlider = mTheme->createSlider( mLightCont, Sizei( 100, 20 ), Vector2i( mUIRedSlider->getPosition().x, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIGreenSlider->setMaxValue( 255 );
	mUIGreenSlider->setValue( 255 );
	mUIGreenSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnGreenChange ) );

	mUIGreenTxt = mTheme->createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "B:", mLightCont, Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIBlueSlider = mTheme->createSlider( mLightCont, Sizei( 100, 20 ), Vector2i( mUIRedSlider->getPosition().x, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIBlueSlider->setMaxValue( 255 );
	mUIBlueSlider->setValue( 255 );
	mUIBlueSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnBlueChange ) );

	mUIBlueTxt = mTheme->createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4, mUIBlueSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "Light Radius:", mLightCont, Sizei(), Vector2i( TAB_CONT_X_DIST, mUIBlueTxt->getPosition().y + mUIBlueTxt->getSize().getHeight() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mLightRadius = mTheme->createSpinBox( mLightCont, Sizei( 100, 22 ), Vector2i( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, 100, false );
	mLightRadius->setMaxValue( 2000 );
	mLightRadius->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnLightRadiusChangeVal ) );

	mLightTypeChk = mTheme->createCheckBox( mLightCont, Sizei(), Vector2i( mLightRadius->getPosition().x, mLightRadius->getPosition().y + mLightRadius->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mLightTypeChk->setText( "Isometric Light" );
	mLightTypeChk->setActive( false );
	mLightTypeChk->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnLightTypeChange ) );
}

UISelectButton * MapEditor::AddObjContButton( String text, Uint32 mode ) {
	UISelectButton * Button = mTheme->createSelectButton( mObjectCont, Sizei( mObjectCont->getSize().getWidth() - TAB_CONT_X_DIST * 2, 22 ), Vector2i( TAB_CONT_X_DIST, mLastSelButtonY ) );

	Button->setText( text );
	Button->setData( mode );

	Button->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnObjectModeSel ) );

	mLastSelButtonY += Button->getSize().getHeight() + 4;

	mObjContButton.push_back( Button );

	return Button;
}

void MapEditor::CreateObjectsContainer() {
	AddObjContButton( "Select Objects", UIMap::SELECT_OBJECTS )->select();
	AddObjContButton( "Edit Polygons", UIMap::EDIT_POLYGONS );
	AddObjContButton( "Insert Object", UIMap::INSERT_OBJECT );
	AddObjContButton( "Insert Polygon", UIMap::INSERT_POLYGON );
	UISelectButton * Button = AddObjContButton( "Insert Polyline", UIMap::INSERT_POLYLINE );

	Int32 nextY = Button->getPosition().y + Button->getSize().getHeight() + 4;

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkClampToTile = mTheme->createCheckBox( mObjectCont, Sizei(), Vector2i( 12, nextY ), ChkFlags );
	mChkClampToTile->setText( "Clamp Position to Tile" );
	mChkClampToTile->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickClampToTile ) );
	mChkClampToTile->setActive( true );
}

void MapEditor::OnObjectModeSel( const UIEvent * Event ) {
	UISelectButton * Button = static_cast<UISelectButton*>( Event->getControl() );
	UISelectButton * ButtonT = NULL;

	for ( std::list<UISelectButton*>::iterator it = mObjContButton.begin(); it != mObjContButton.end(); it++ ) {
		ButtonT = *it;

		ButtonT->unselect();
	}

	Button->select();

	mUIMap->setEditingObjMode( (UIMap::EDITING_OBJ_MODE)Button->getData() );
}

void MapEditor::CreateUIMap() {
	UISkin * HScrollSkin = mTheme->getByName( mTheme->getAbbr() + "_" + "hscrollbar_bg" );
	UISkin * VScrollSkin = mTheme->getByName( mTheme->getAbbr() + "_" + "vscrollbar_bg" );

	Float ScrollH = 16;
	Float ScrollV = 16;

	if ( NULL != HScrollSkin ) {
		ScrollH = HScrollSkin->getSize().getHeight();
	}

	if ( NULL != VScrollSkin ) {
		ScrollV = VScrollSkin->getSize().getHeight();
	}

	UIComplexControl::CreateParams Params;
	Params.setParent( mWinContainer );
	Params.setPosition( 0, 0 );
	Params.setSize( mWinContainer->getSize().getWidth() - 225 - ScrollV, mWinContainer->getSize().getHeight() - ScrollH );

	Params.Flags |= UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT;
	mUIMap = eeNew( UIMap, ( Params, mTheme ) );
	mUIMap->setVisible( true );
	mUIMap->setEnabled( true );
	CreateNewEmptyMap();
	mUIMap->addEventListener( UIEvent::EventOnSizeChange, cb::Make1( this, &MapEditor::OnMapSizeChange ) );
	mUIMap->addEventListener( UIEvent::EventMouseDown, cb::Make1( this, &MapEditor::OnMapMouseDown ) );
	mUIMap->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnMapMouseClick ) );
	mUIMap->setLightSelectCb( cb::Make1( this, &MapEditor::OnLightSelect ) );
	mUIMap->setLightRadiusChangeCb( cb::Make1( this, &MapEditor::OnLightRadiusChange ) );
	mUIMap->setAddObjectCallback( cb::Make2( this, &MapEditor::OnAddObject ) );
	mUIMap->setAlertCb( cb::Make2( this, &MapEditor::CreateAlert ) );
	mUIMap->setUpdateScrollCb( cb::Make0( this, &MapEditor::UpdateScroll ) );
	mUIMap->setTileBox( mTileBox );

	mMapHScroll = mTheme->createScrollBar( mWinContainer, Sizei( Params.Size.getWidth(), ScrollH ), Vector2i( 0, mWinContainer->getSize().getHeight() - ScrollH ), UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM | UI_AUTO_SIZE );
	mMapHScroll->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnScrollMapH ) );

	mMapVScroll = mTheme->createScrollBar( mWinContainer, Sizei( ScrollV, Params.Size.getHeight() ), Vector2i( Params.Size.getWidth() + ScrollV, 0 ), UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_AUTO_SIZE , true );
	mMapVScroll->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnScrollMapV ) );

	MapCreated();
}

void MapEditor::OnAddObject( Uint32 Type, Polygon2f poly ) {
	if ( NULL == mCurLayer ) {
		CreateNoLayerAlert( "No layers found" )->setFocus();
		return;
	}

	if ( mCurLayer->getType() != MAP_LAYER_OBJECT ) {
		CreateAlert( "Wrong Layer", "Objects only can be added to an Object Layer" )->setFocus();
		return;
	}

	if ( poly.getSize() < 3 ) {
		return;
	}

	MapObjectLayer * OL = static_cast<MapObjectLayer*> ( mCurLayer );

	if ( GAMEOBJECT_TYPE_OBJECT == Type ) {
		OL->addGameObject( eeNew( GameObjectObject, ( mUIMap->Map()->getNewObjectId(), poly.toAABB(), mCurLayer ) ) );
	} else if ( GAMEOBJECT_TYPE_POLYGON == Type ) {
		OL->addGameObject( eeNew( GameObjectPolygon, ( mUIMap->Map()->getNewObjectId(), poly, mCurLayer ) ) );
	} else if ( GAMEOBJECT_TYPE_POLYLINE == Type ) {
		OL->addGameObject( eeNew( GameObjectPolyline, ( mUIMap->Map()->getNewObjectId(), poly, mCurLayer ) ) );
	}
}

void MapEditor::OnLightTypeChange( const UIEvent * Event ) {
	if ( NULL != mUIMap->getSelectedLight() ) {
		mUIMap->getSelectedLight()->setType( mLightTypeChk->isActive() ? LIGHT_ISOMETRIC : LIGHT_NORMAL );
	}
}

void MapEditor::OnLightRadiusChangeVal( const UIEvent * Event ) {
	if ( NULL != mUIMap->getSelectedLight() ) {
		mUIMap->getSelectedLight()->setRadius( mLightRadius->getValue() );
	}
}

void MapEditor::OnLightRadiusChange( MapLight * Light ) {
	mLightRadius->setValue( Light->getRadius() );
}

void MapEditor::OnLightSelect( MapLight * Light ) {
	ColorA Col( Light->getColor() );

	mUIRedSlider->setValue( Col.r() );
	mUIGreenSlider->setValue( Col.g() );
	mUIBlueSlider->setValue( Col.b() );
	mLightRadius->setValue( Light->getRadius() );
	mLightTypeChk->setActive( Light->getType() == LIGHT_ISOMETRIC ? true : false );
}

void MapEditor::OnNewLight( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
		Vector2i Pos = mUIMap->Map()->getMouseMapPos();
		mUIMap->addLight( eeNew( MapLight, ( mLightRadius->getValue(), Pos.x, Pos.y, mUIBaseColor->getBackground()->getColor().toColor(), mLightTypeChk->isActive() ? LIGHT_ISOMETRIC : LIGHT_NORMAL ) ) );
	}
}

void MapEditor::OnRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Red = (Uint8)mUIRedSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIRedTxt->setText( String::toStr( (Int32)mUIRedSlider->getValue() ) );

	if ( NULL != mUIMap->getSelectedLight() ) {
		RGB lCol( mUIMap->getSelectedLight()->getColor() );
		lCol.Red = Col.r();
		mUIMap->getSelectedLight()->setColor( lCol );
	}
}

void MapEditor::OnGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Green = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIGreenTxt->setText( String::toStr( (Uint32)mUIGreenSlider->getValue() ) );

	if ( NULL != mUIMap->getSelectedLight() ) {
		RGB lCol( mUIMap->getSelectedLight()->getColor() );
		lCol.Green = Col.g();
		mUIMap->getSelectedLight()->setColor( lCol );
	}
}

void MapEditor::OnBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Blue = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIBlueTxt->setText( String::toStr( (Uint32)mUIBlueSlider->getValue() ) );

	if ( NULL != mUIMap->getSelectedLight() ) {
		RGB lCol( mUIMap->getSelectedLight()->getColor() );
		lCol.Blue = Col.b();
		mUIMap->getSelectedLight()->setColor( lCol );
	}
}

void MapEditor::ChkClickDI( const UIEvent * Event ) {
	if ( mChkDI->isActive() ) {
		mSGCont->setEnabled( false );
		mSGCont->setVisible( false );
		mDICont->setEnabled( true );
		mDICont->setVisible( true );
	} else {
		mSGCont->setEnabled( true );
		mSGCont->setVisible( true );
		mDICont->setEnabled( false );
		mDICont->setVisible( false );
	}
}

void MapEditor::ChkClickClampToTile( const UIEvent * Event ) {
	mUIMap->setClampToTile( mChkClampToTile->isActive() );
}

void MapEditor::UpdateGfx() {
	if ( mChkMirrored->isActive() && mChkFliped->isActive() )
		mGfxPreview->setRenderMode( RN_FLIPMIRROR );
	else if( mChkMirrored->isActive() )
		mGfxPreview->setRenderMode( RN_MIRROR );
	else if ( mChkFliped->isActive() )
		mGfxPreview->setRenderMode( RN_FLIP );
	else
		mGfxPreview->setRenderMode( RN_NORMAL );

	if ( mChkRot90->isActive() )
		mGfxPreview->setRotation( 90 );
	else
		mGfxPreview->setRotation( 0 );
}

void MapEditor::UpdateFlags() {
	mCurGOFlags = 0;

	if ( mChkMirrored->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_MIRRORED;

	if ( mChkFliped->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_FLIPED;

	if ( mChkBlocked->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_BLOCKED;

	if ( mChkAnim->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ANIMATED;

	if ( mChkRot90->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ROTATE_90DEG;

	if ( mChkAutoFix->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_AUTO_FIX_TILE_POS;
}

void MapEditor::OnTypeChange( const UIEvent * Event ) {
	if ( mGOTypeList->getText() == "SubTexture" )
		mCurGOType = GAMEOBJECT_TYPE_SUBTEXTURE;
	else if ( mGOTypeList->getText() == "SubTextureEx" )
		mCurGOType = GAMEOBJECT_TYPE_SUBTEXTUREEX;
	else if ( mGOTypeList->getText() == "Sprite" )
		mCurGOType = GAMEOBJECT_TYPE_SPRITE;
	else
		mCurGOType = String::hash( mGOTypeList->getText().toUtf8() );

	if ( NULL != mChkAnim && NULL != mGOTypeList && mChkAnim->isActive() && mGOTypeList->getText() != "Sprite" ) {
		if ( mGOTypeList->getText() == "SubTexture" || mGOTypeList->getText() == "SubTextureEx" ) {
			mChkAnim->setActive( false );
		}
	}
}

void MapEditor::ChkClickMirrored( const UIEvent * Event ) {
	UpdateGfx();
	UpdateFlags();
}

void MapEditor::ChkClickFliped( const UIEvent * Event ) {
	UpdateGfx();
	UpdateFlags();
}

void MapEditor::ChkClickRot90( const UIEvent * Event ) {
	UpdateGfx();
	UpdateFlags();
}

void MapEditor::ChkClickBlocked( const UIEvent * Event ) {
	UpdateFlags();
}

void MapEditor::ChkClickAutoFix( const UIEvent * Event ) {
	UpdateFlags();
}

void MapEditor::ChkClickAnimated( const UIEvent * Event ) {
	UpdateFlags();

	if ( mChkAnim->isActive() && ( mGOTypeList->getText() == "SubTexture" || mGOTypeList->getText() == "SubTextureEx" ) ) {
		mGOTypeList->getListBox()->setSelected( "Sprite" );
	}
}

void MapEditor::AddNewGOType( const UIEvent * Event ) {
	eeNew( UIGOTypeNew, ( cb::Make2( this, &MapEditor::OnNewGOTypeAdded ) ) );
}

void MapEditor::OnNewGOTypeAdded( std::string name, Uint32 hash ) {
	if ( "" != name ) {
		for ( Uint32 i = 0; i < mGOTypeList->getListBox()->getCount(); i++ ) {
			UIListBoxItem * Item = mGOTypeList->getListBox()->getItem(i);

			if ( Item->getText() == name )
				return;
		}

		mGOTypeList->getListBox()->addListBoxItem( name );
		mUIMap->Map()->addVirtualObjectType( name );
	}
}

void MapEditor::FillSGCombo() {
	TextureAtlasManager * SGM = TextureAtlasManager::instance();
	std::list<TextureAtlas*>& Res = SGM->getResources();

	mTextureAtlasesList->getListBox()->clear();

	std::vector<String> items;

	Uint32 Restricted1 = String::hash( std::string( "global" ) );
	Uint32 Restricted2 = String::hash( mTheme->getTextureAtlas()->getName() );

	for ( std::list<TextureAtlas*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
		if ( (*it)->getId() != Restricted1 && (*it)->getId() != Restricted2 )
			items.push_back( (*it)->getName() );
	}

	if ( items.size() ) {
		mTextureAtlasesList->getListBox()->addListBoxItems( items );
	}

	if ( mTextureAtlasesList->getListBox()->getCount() && NULL == mTextureAtlasesList->getListBox()->getItemSelected() ) {
		mTextureAtlasesList->getListBox()->setSelected( 0 );
	}
}

void MapEditor::FillSubTextureList() {
	TextureAtlasManager * SGM = TextureAtlasManager::instance();
	mCurSG = SGM->getByName( mTextureAtlasesList->getText() );
	std::list<SubTexture*>& Res = mCurSG->getResources();

	mSubTextureList->clear();

	if ( NULL != mCurSG ) {
		std::vector<String> items;

		for ( std::list<SubTexture*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
				items.push_back( (*it)->getName() );
		}

		if ( items.size() ) {
			std::sort( items.begin(), items.end() );

			mSubTextureList->addListBoxItems( items );
			mSubTextureList->setSelected( 0 );
		}
	}

	mSubTextureList->getVerticalScrollBar()->setClickStep( 8.f / (Float)mSubTextureList->getCount() );
}

void MapEditor::OnSubTextureChange( const UIEvent * Event ) {
	if ( NULL != mCurSG ) {
		SubTexture * tSubTexture = mCurSG->getByName( mSubTextureList->getItemSelectedText() );

		if ( NULL != tSubTexture ) {
			mGfxPreview->setSubTexture( tSubTexture );
		}
	}
}

void MapEditor::OnTextureAtlasChange( const UIEvent * Event ) {
	FillSubTextureList();
}

void MapEditor::CreateNewMap() {
	eeNew( UIMapNew, ( mUIMap, cb::Make0( this, &MapEditor::MapCreated ) ) );
}

void MapEditor::CreateNewEmptyMap() {
	mUIMap->Map()->create( Sizei( 100, 100 ), 16, Sizei( 32, 32 ), MAP_EDITOR_DEFAULT_FLAGS | MAP_FLAG_LIGHTS_ENABLED, mUIMap->getRealSize() );
}

void MapEditor::MapCreated() {
	mCurLayer = NULL;
	mLayerList->getListBox()->clear();
	UpdateFlags();
	SetViewOptions();
	FillSGCombo();
	FillGotyList();

	mMapHScroll->setValue( 0 );
	mMapVScroll->setValue( 0 );
	OnMapSizeChange( NULL );

	mUIMap->clearLights();

	CreateTabs();
}

void MapEditor::OnMapSizeChange( const UIEvent *Event ) {
	if ( mMouseScrolling )
		return;

	Vector2i v( mUIMap->Map()->getMaxOffset() );

	mMapHScroll->setMinValue( 0 );
	mMapHScroll->setMaxValue( v.x );
	mMapHScroll->setClickStep( mUIMap->Map()->getTileSize().getWidth() * mUIMap->Map()->getScale() );
	mMapVScroll->setMinValue( 0 );
	mMapVScroll->setMaxValue( v.y );
	mMapVScroll->setClickStep( mUIMap->Map()->getTileSize().getHeight() * mUIMap->Map()->getScale() );
	mMapHScroll->setPageStep( (Float)mUIMap->Map()->getViewSize().getWidth() );
	mMapVScroll->setPageStep( (Float)mUIMap->Map()->getViewSize().getHeight() );
}

void MapEditor::OnScrollMapH( const UIEvent * Event ) {
	if ( mMouseScrolling )
		return;

	UIScrollBar * Scr = reinterpret_cast<UIScrollBar*> ( Event->getControl() );

	Vector2f Off = mUIMap->Map()->getOffset();

	Off.x = -Scr->getValue();

	mUIMap->Map()->setOffset( Off ) ;
}

void MapEditor::OnScrollMapV( const UIEvent * Event ) {
	UIScrollBar * Scr = reinterpret_cast<UIScrollBar*> ( Event->getControl() );

	Vector2f Off = mUIMap->Map()->getOffset();

	Off.y = -Scr->getValue();

	mUIMap->Map()->setOffset( Off ) ;
}

void MapEditor::UpdateScroll() {
	mMouseScrolling = true;
	mMapHScroll->setValue( -mUIMap->Map()->getOffset().x );
	mMapVScroll->setValue( -mUIMap->Map()->getOffset().y );
	mMouseScrolling = false;
}

void MapEditor::MapOpen( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	if ( mUIMap->Map()->load( CDL->getFullPath() ) ) {
		OnMapLoad();
	}
}

void MapEditor::OnMapLoad() {
	mCurLayer = NULL;

	mUIMap->Map()->setViewSize( mUIMap->getRealSize() );

	MapCreated();

	RefreshLayersList();

	RefreshGotyList();
}

void MapEditor::MapSave( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	std::string path( CDL->getFullPath() );

	if ( path.substr( path.size() - 4 ) != ".eem" ) {
		path += ".eem";
	}

	mUIMap->Map()->save( path );
}

void MapEditor::FileMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "New..." == txt ) {
		CreateNewMap();
	} else if ( "Open..." == txt ) {
		UICommonDialog * TGDialog = mTheme->createCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS, "*.eem" );

		TGDialog->setTitle( "Open Map" );
		TGDialog->addEventListener( UIEvent::EventOpenFile, cb::Make1( this, &MapEditor::MapOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save As..." == txt ) {
		UICommonDialog * TGDialog = mTheme->createCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_SAVE_DIALOG, "*.eem" );

		TGDialog->setTitle( "Save Map" );
		TGDialog->addEventListener( UIEvent::EventSaveFile, cb::Make1( this, &MapEditor::MapSave ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save" == txt ) {
		if ( mUIMap->Map()->getPath().size() ) {
			mUIMap->Map()->save( mUIMap->Map()->getPath() );
		}
	} else if ( "Close" == txt ) {
		UIMessageBox * MsgBox = mTheme->createMessageBox( MSGBOX_OKCANCEL, "Do you really want to close the current map?\nAll changes will be lost." );
		MsgBox->addEventListener( UIEvent::EventMsgBoxConfirmClick, cb::Make1( this, &MapEditor::OnMapClose ) );
		MsgBox->setTitle( "Close Map?" );
		MsgBox->center();
		MsgBox->show();
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == UIManager::instance()->getMainControl() ) {
			UIManager::instance()->getWindow()->close();
		} else {
			mUIWindow->CloseWindow();
		}
	}
}

void MapEditor::OnMapClose( const UIEvent * Event ) {
	CreateNewEmptyMap();

	MapCreated();

	mCurLayer = NULL;

	RefreshLayersList();
}

void MapEditor::ViewMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "Show Grid" == txt ) {
		mUIMap->Map()->setDrawGrid( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->isActive() );
	} else if ( "Mark Tile Over" == txt ) {
		mUIMap->Map()->setDrawTileOver( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->isActive() );
	} else if ( "Show Blocked" == txt ) {
		mUIMap->Map()->setShowBlocked( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->isActive() );
	} else if ( "Zoom In" == txt ) {
		ZoomIn();
	} else if ( "Zoom Out" == txt ) {
		ZoomOut();
	} else if ( "Normal Size" == txt ) {
		mUIMap->Map()->setScale( 1 );
	}
}

void MapEditor::ZoomIn() {
	TileMap * Map = mUIMap->Map();
	Float S = mUIMap->Map()->getScale();

	if ( S < 4 ) {
		if ( 0.0625f == S ) {
			Map->setScale( 0.125f );
		} else if ( 0.125f == S ) {
			Map->setScale( 0.25f );
		} else if ( 0.25f == S ) {
			Map->setScale( 0.5f );
		} else if ( 0.5f == S ) {
			Map->setScale( 0.75f );
		} else if ( 0.75f == S ) {
			Map->setScale( 1.0f );
		} else if ( 1.0f == S ) {
			Map->setScale( 1.5f );
		} else if ( 1.5f == S ) {
			Map->setScale( 2.0f );
		} else if ( 2.0f == S ) {
			Map->setScale( 3.0f );
		} else if ( 3.0f == S ) {
			Map->setScale( 4.0f );
		}
	}

	OnMapSizeChange();
}

void MapEditor::ZoomOut() {
	TileMap * Map = mUIMap->Map();
	Float S = mUIMap->Map()->getScale();

	if ( S > 0.0625f ) {
		if ( 0.125f == S ) {
			Map->setScale( 0.0625f );
		} else if ( 0.25f == S ) {
			Map->setScale( 0.125f );
		} else if ( 0.5f == S ) {
			Map->setScale( 0.25f );
		} else if ( 0.75f == S ) {
			Map->setScale( 0.5f );
		} else if ( 1.0f == S ) {
			Map->setScale( 0.75f );
		} else if ( 1.5f == S ) {
			Map->setScale( 1.0f );
		} else if ( 2.0f == S ) {
			Map->setScale( 1.5f );
		} else if ( 3.0f == S ) {
			Map->setScale( 2.0f );
		} else if ( 4.0f == S ) {
			Map->setScale( 3.0f );
		}
	}

	OnMapSizeChange();
}

void MapEditor::MapMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "New Texture Atlas..." == txt ) {
		UIWindow * tWin = mTheme->createWindow( NULL, Sizei( 1024, 768 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, Sizei( 1024, 768 ) );
		eeNew ( Tools::TextureAtlasEditor, ( tWin ) );
		tWin->center();
		tWin->show();
	} else if ( "Add External Texture Atlas..." == txt ) {
		UICommonDialog * TGDialog = mTheme->createCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );

		TGDialog->setTitle( "Load Texture Atlas..." );
		TGDialog->addEventListener( UIEvent::EventOpenFile, cb::Make1( this, &MapEditor::TextureAtlasOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Properties..." == txt ) {
		eeNew( TileMapProperties, ( mUIMap->Map() ) );
	} else if ( "Resize..." ) {
		eeNew( UIMapNew, ( mUIMap, cb::Make0( this, &MapEditor::OnMapLoad ), true ) );
	}
}

void MapEditor::LayerMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "Add Tile Layer..." == txt ) {
		eeNew( UIMapLayerNew, ( mUIMap, MAP_LAYER_TILED, cb::Make1( this, &MapEditor::OnLayerAdd ) ) );
	} else if ( "Add Object Layer..." == txt ) {
		eeNew( UIMapLayerNew, ( mUIMap, MAP_LAYER_OBJECT, cb::Make1( this, &MapEditor::OnLayerAdd ) ) );
	} else if ( "Remove Layer" == txt ) {
		RemoveLayer();
	} else if ( "Move Layer Up" == txt ) {
		MoveLayerUp();
	} else if ( "Move Layer Down" == txt ) {
		MoveLayerDown();
	} else if ( "Layer Properties..." == txt ) {
		if ( NULL != mCurLayer) {
			eeNew( MapLayerProperties, ( mCurLayer, cb::Make0( this, &MapEditor::RefreshLayersList ) ) );
		} else {
			CreateNoLayerAlert( "Error retrieving layer properties" );
		}
	} else if ( "Lights Enabled" == txt ) {
		if ( NULL != mCurLayer ) {
			mCurLayer->setLightsEnabled( !mCurLayer->getLightsEnabled() );
		}
	} else if ( "Visible" == txt ) {
		if ( NULL != mCurLayer ) {
			mCurLayer->setVisible( !mCurLayer->isVisible() );
		}
	}
}

UIMessageBox * MapEditor::CreateAlert( const String& title, const String& text ) {
	UIMessageBox * MsgBox = mTheme->createMessageBox( MSGBOX_OK, text );
	MsgBox->setTitle( title );
	MsgBox->center();
	MsgBox->show();
	return MsgBox;
}

UIMessageBox * MapEditor::CreateNoLayerAlert( const String title ) {
	return CreateAlert( title, "First select and add a new layer." );
}

void MapEditor::MoveLayerUp() {
	if ( mUIMap->Map()->moveLayerUp( mCurLayer ) ) {
		RefreshLayersList();
	}
}

void MapEditor::MoveLayerDown() {
	if ( mUIMap->Map()->moveLayerDown( mCurLayer ) ) {
		RefreshLayersList();
	}
}

void MapEditor::RemoveLayer() {
	if ( mUIMap->Map()->removeLayer( mCurLayer ) ) {
		mCurLayer = NULL;

		RefreshLayersList();
	}
}

void MapEditor::RefreshGotyList() {
	TileMap::GOTypesList& GOList = mUIMap->Map()->getVirtualObjectTypes();

	for ( TileMap::GOTypesList::iterator it = GOList.begin(); it != GOList.end(); it++ ) {
		mGOTypeList->getListBox()->addListBoxItem( (*it) );
	}
}

void MapEditor::RefreshLayersList() {
	mLayerList->getListBox()->clear();

	if ( mUIMap->Map()->getLayerCount() ) {
		std::vector<String> layers;

		for ( Uint32 i = 0; i < mUIMap->Map()->getLayerCount(); i++ ) {
			layers.push_back( mUIMap->Map()->getLayer(i)->getName() );
		}

		mLayerList->getListBox()->addListBoxItems( layers );
	}

	if ( NULL != mCurLayer ) {
		mLayerList->getListBox()->setSelected( mCurLayer->getName() );
	} else {
		if ( mUIMap->Map()->getLayerCount() ) {
			mLayerList->getListBox()->setSelected(0);
		}
	}
}

void MapEditor::TextureAtlasOpen( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	std::string sgname = FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( CDL->getFullPath() ) );

	TextureAtlas * SG = TextureAtlasManager::instance()->getByName( sgname );

	if ( NULL == SG ) {
		TextureAtlasLoader tgl( CDL->getFullPath() );

		if ( tgl.isLoaded() ) {
			mTextureAtlasesList->getListBox()->addListBoxItem( sgname );
		}
	} else {
		if ( eeINDEX_NOT_FOUND == mTextureAtlasesList->getListBox()->getItemIndex( sgname ) ) {
			mTextureAtlasesList->getListBox()->addListBoxItem( sgname );
		}
	}
}

void MapEditor::OnLayerAdd( UIMapLayerNew * UILayer ) {
	bool SetSelected = ( 0 == mLayerList->getListBox()->getCount() ) ? true : false;

	mLayerList->getListBox()->addListBoxItem( UILayer->getName() );

	if ( SetSelected ) {
		mCurLayer = UILayer->getLayer();

		mUIMap->setCurLayer( mCurLayer );

		mLayerList->getListBox()->setSelected(0);
	}
}

void MapEditor::OnLayerSelect( const UIEvent * Event ) {
	MapLayer * tLayer = mUIMap->Map()->getLayer( mLayerList->getText() );

	if ( NULL != tLayer ) {
		mCurLayer = tLayer;

		mUIMap->setCurLayer( mCurLayer );

		mLayerChkVisible->setActive( mCurLayer->isVisible() );

		mLayerChkLights->setActive( mCurLayer->getLightsEnabled() );
	}
}

void MapEditor::WindowClose( const UIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

GameObject * MapEditor::CreateGameObject() {
	GameObject * tObj	= NULL;

	if ( GAMEOBJECT_TYPE_SUBTEXTURE == mCurGOType ) {

		tObj = eeNew( GameObjectSubTexture, ( mCurGOFlags, mCurLayer, mGfxPreview->getSubTexture() ) );

	} else if ( GAMEOBJECT_TYPE_SUBTEXTUREEX == mCurGOType ) {

		tObj = eeNew( GameObjectSubTextureEx, ( mCurGOFlags, mCurLayer, mGfxPreview->getSubTexture() ) );

	} else if ( GAMEOBJECT_TYPE_SPRITE == mCurGOType ) {

		if ( mChkAnim->isActive() ) {

			Sprite * tAnimSprite = eeNew( Sprite, ( String::removeNumbersAtEnd( mGfxPreview->getSubTexture()->getName() ) ) );
			tObj = eeNew( GameObjectSprite, ( mCurGOFlags, mCurLayer, tAnimSprite ) );

		} else {

			Sprite * tStatiSprite = eeNew( Sprite, ( mGfxPreview->getSubTexture() ) );
			tObj = eeNew( GameObjectSprite, ( mCurGOFlags, mCurLayer, tStatiSprite ) );

		}
	} else {
		//! Creates an empty game object. The client will interpret the GameObject Type, and instanciate the corresponding class.

		if ( mChkDI->isActive() )
			tObj = eeNew( GameObjectVirtual, ( String::hash( mDataIdInput->getText().toUtf8() ), mCurLayer, mCurGOFlags, mCurGOType ) );
		else
			tObj = eeNew( GameObjectVirtual, ( mGfxPreview->getSubTexture(), mCurLayer, mCurGOFlags, mCurGOType ) );
	}

	return tObj;
}

void MapEditor::AddGameObjectToTile() {
	TileMapLayer * tLayer	= reinterpret_cast<TileMapLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();
	GameObject * tObj	= CreateGameObject();

	if ( NULL != tObj ) {
		if ( tObj->getType() == GAMEOBJECT_TYPE_VIRTUAL )
			reinterpret_cast<GameObjectVirtual*> ( tObj )->setLayer( tLayer );

		tLayer->addGameObject( tObj, tMap->getMouseTilePos() );
	}
}

void MapEditor::RemoveGameObjectFromTile() {
	TileMapLayer * tLayer = reinterpret_cast<TileMapLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();

	tLayer->removeGameObject( tMap->getMouseTilePos() );
}

void MapEditor::AddGameObject() {
	MapObjectLayer * tLayer	= reinterpret_cast<MapObjectLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();
	GameObject * tObj	= CreateGameObject();

	if ( NULL != tObj ) {
		if ( tObj->getType() == GAMEOBJECT_TYPE_VIRTUAL )
			reinterpret_cast<GameObjectVirtual*> ( tObj )->setLayer( tLayer );

		Vector2i p( tMap->getMouseMapPos() );

		if ( UIManager::instance()->getInput()->isKeyDown( KEY_LCTRL ) ) {
			p = tMap->getMouseTilePosCoords();
		}

		tObj->setPosition( Vector2f( p.x, p.y ) );
		tLayer->addGameObject( tObj );
	}
}

void MapEditor::RemoveGameObject() {
	MapObjectLayer * tLayer = reinterpret_cast<MapObjectLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();

	tLayer->removeGameObject( tMap->getMouseMapPos() );
}

GameObject * MapEditor::GetCurrentGOOver() {
	return reinterpret_cast<TileMapLayer*>( mCurLayer )->getGameObject( mUIMap->Map()->getMouseTilePos() );
}

void MapEditor::OnMapMouseClick( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( mSubTextureCont->isVisible() ) {
		if ( NULL == mCurLayer || NULL == mGfxPreview->getSubTexture() || UIManager::instance()->getDownControl() != mUIMap ) {
			if ( NULL == mCurLayer )
				CreateNoLayerAlert( "No layers found" );

			return;
		}

		if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_OBJECT )
				AddGameObject();
		} else if ( MEvent->getFlags() & EE_BUTTON_RMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_OBJECT )
				RemoveGameObject();
		} else if ( MEvent->getFlags() & EE_BUTTON_MMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->setBlocked( !tObj->isBlocked() );
				}
			}
		} else if ( MEvent->getFlags() & EE_BUTTON_WUMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->setMirrored( !tObj->isMirrored() );
				}
			}
		} else if ( MEvent->getFlags() & EE_BUTTON_WDMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->setRotated( !tObj->isRotated() );
				}
			}
		}
	}
}

void MapEditor::OnMapMouseDown( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( mSubTextureCont->isVisible() ) {
		if ( NULL == mCurLayer || NULL == mGfxPreview->getSubTexture() || UIManager::instance()->getDownControl() != mUIMap )
			return;


		if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED )
				AddGameObjectToTile();
		} else if ( MEvent->getFlags() & EE_BUTTON_RMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED )
				RemoveGameObjectFromTile();
		}
	}
}

void MapEditor::SetViewOptions() {
	mChkShowGrid->setActive( mUIMap->Map()->getDrawGrid() ? true : false );
	mChkMarkTileOver->setActive( mUIMap->Map()->getDrawTileOver() ? true : false  );
	mChkShowBlocked->setActive( mUIMap->Map()->getShowBlocked() ? true : false );
}

}}
