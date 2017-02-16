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
	mTheme( UIThemeManager::instance()->defaultTheme() ),
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
		mUIWindow = UIManager::instance()->mainControl();
		mUIWindow->setSkinFromTheme( mTheme, "winback" );
	}

	if ( UIManager::instance()->mainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->getContainer();
	}

	mUIWindow->title( "Map Editor" );
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
	mTileBox->size( 100, WinMenu->size().height() );
	mTileBox->position( Vector2i( mUIContainer->size().width() - mTileBox->size().width(), 0 ) );
	mTileBox->updateAnchorsDistances();

	UIPopUpMenu * PU1 = mTheme->createPopUpMenu( mUIContainer );
	PU1->Add( "New...", mTheme->getIconByName( "document-new" ) );
	PU1->Add( "Open...", mTheme->getIconByName( "document-open" ) );
	PU1->AddSeparator();
	PU1->Add( "Save", mTheme->getIconByName( "document-save" ) );
	PU1->Add( "Save As...", mTheme->getIconByName( "document-save-as" ) );
	PU1->AddSeparator();
	PU1->Add( "Close", mTheme->getIconByName( "document-close" ) );
	PU1->AddSeparator();
	PU1->Add( "Quit", mTheme->getIconByName( "quit" ) );

	PU1->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::FileMenuClick ) );
	WinMenu->addMenuButton( "File", PU1 );

	UIPopUpMenu * PU3 = mTheme->createPopUpMenu( mUIContainer );
	mChkShowGrid = reinterpret_cast<UIMenuCheckBox*>( PU3->GetItem( PU3->AddCheckBox( "Show Grid" ) ) );

	mChkShowGrid->active( true );

	mChkMarkTileOver = reinterpret_cast<UIMenuCheckBox*>( PU3->GetItem( PU3->AddCheckBox( "Mark Tile Over" ) ) );

	mChkShowBlocked = reinterpret_cast<UIMenuCheckBox*>( PU3->GetItem( PU3->AddCheckBox( "Show Blocked" ) ) );

	PU3->AddSeparator();
	mUIWindow->addShortcut( KEY_KP_PLUS	, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->GetItem( PU3->Add( "Zoom In", mTheme->getIconByName( "zoom-in" ) ) ) ) );
	mUIWindow->addShortcut( KEY_KP_MINUS, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->GetItem( PU3->Add( "Zoom Out", mTheme->getIconByName( "zoom-out" ) ) ) ) );
	mUIWindow->addShortcut( KEY_KP0		, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->GetItem( PU3->Add( "Normal Size", mTheme->getIconByName( "zoom-original" ) ) ) ) );
	PU3->AddSeparator();

	PU3->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::ViewMenuClick ) );
	WinMenu->addMenuButton( "View", PU3 );

	UIPopUpMenu * PU4 = mTheme->createPopUpMenu( mUIContainer );
	PU4->Add( "Properties..." );
	PU4->Add( "Resize..." );

	PU4->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::MapMenuClick ) );
	WinMenu->addMenuButton( "Map", PU4 );

	UIPopUpMenu * PU5 = mTheme->createPopUpMenu( mUIContainer );
	PU5->Add( "Add Tile Layer..." );
	PU5->Add( "Add Object Layer..." );
	PU5->AddSeparator();
	PU5->Add( "Remove Layer" );
	PU5->AddSeparator();
	PU5->Add( "Move Layer Up" );
	PU5->Add( "Move Layer Down" );
	PU5->AddSeparator();
	PU5->Add( "Layer Properties..." );
	PU5->AddSeparator();

	Uint32 LayerChkBoxIndex = PU5->AddCheckBox( "Lights Enabled" );
	mLayerChkLights = reinterpret_cast<UIMenuCheckBox*> ( PU5->GetItem( LayerChkBoxIndex ) );

	PU5->AddSeparator();

	LayerChkBoxIndex = PU5->AddCheckBox( "Visible" );
	mLayerChkVisible = reinterpret_cast<UIMenuCheckBox*> ( PU5->GetItem( LayerChkBoxIndex ) );

	PU5->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::LayerMenuClick ) );
	WinMenu->addMenuButton( "Layer", PU5 );

	UIPopUpMenu * PU6 = mTheme->createPopUpMenu( mUIContainer );
	PU6->Add( "New Texture Atlas..." );
	PU6->Add( "Add External Texture Atlas..." );
	WinMenu->addMenuButton( "Atlases", PU6 );
	PU6->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::MapMenuClick ) );

	UIComplexControl::CreateParams Params;
	Params.Parent( mUIContainer );
	Params.PosSet( 0, WinMenu->size().height() );
	Params.SizeSet( mUIContainer->size().width(), mUIContainer->size().height() - WinMenu->size().height() );
	Params.Flags = UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_REPORT_SIZE_CHANGE_TO_CHILDS;
	mWinContainer = eeNew( UIComplexControl, ( Params ) );
	mWinContainer->visible( true );
	mWinContainer->enabled( true );
}

void MapEditor::CreateETGMenu() {
	Int32 Width = 200;
	Int32 DistToBorder = 5;
	Int32 ContPosX = mWinContainer->size().width() - Width - DistToBorder;
	Int32 DistFromTopMenu = 4;

	UIComplexControl::CreateParams CParams;
	CParams.Parent( mWinContainer );
	CParams.SizeSet( Sizei( Width + DistToBorder, mWinContainer->size().height() ) );
	CParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mSubTextureCont = eeNew( UIComplexControl, ( CParams ) );
	mSubTextureCont->enabled( true );
	mSubTextureCont->visible( true );

	mLightCont = eeNew( UIComplexControl, ( CParams ) );

	mObjectCont = eeNew( UIComplexControl, ( CParams ) );

	mTabWidget = mTheme->createTabWidget( mWinContainer, Sizei( Width + DistToBorder, mWinContainer->size().height() - DistFromTopMenu ), Vector2i( ContPosX, DistFromTopMenu ), UI_HALIGN_CENTER | UI_VALIGN_BOTTOM | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM );
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
		if ( mUIMap->Map()->LightsEnabled() ) {
			mTabWidget->add( "Lights", mLightCont );
		}
	}

	mTabWidget->add( "Objects", mObjectCont );
}

void MapEditor::OnTabSelected( const UIEvent * Event ) {
	if ( NULL != mUIMap ) {
		switch ( mTabWidget->getSelectedTabIndex() ) {
			case 0:
				mUIMap->EditingDisabled();
				break;
			case 1:
				mUIMap->EditingLights( true );
				break;
			case 2:
				mUIMap->EditingObjects( true );
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

	mGOTypeList = mTheme->createDropDownList( mSubTextureCont, Sizei( Width - 26, 21 ), Vector2i( TAB_CONT_X_DIST, Txt->position().y + Txt->size().height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mGOTypeList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnTypeChange ) );
	FillGotyList();

	mBtnGOTypeAdd = mTheme->createPushButton( mSubTextureCont, Sizei( 24, 21 ), Vector2i( mGOTypeList->position().x + mGOTypeList->size().width() + 2, mGOTypeList->position().y ), UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mTheme->getIconByName( "add" ) );
	mBtnGOTypeAdd->tooltipText( "Adds a new game object type\nunknown by the map editor." );
	mBtnGOTypeAdd->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::AddNewGOType ) );

	if ( NULL == mBtnGOTypeAdd->icon()->subTexture() )
		mBtnGOTypeAdd->text( "..." );

	Txt = mTheme->createTextBox( "Layers:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mGOTypeList->position().y + mGOTypeList->size().height() + 4 ), TxtFlags );

	mLayerList = mTheme->createDropDownList( mSubTextureCont, Sizei( Width, 21 ), Vector2i( TAB_CONT_X_DIST, Txt->position().y + Txt->size().height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mLayerList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnLayerSelect ) );

	Txt = mTheme->createTextBox( "Game Object Flags:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mLayerList->position().y + mLayerList->size().height() + 4 ), TxtFlags );

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkMirrored = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( TAB_CONT_X_DIST, Txt->position().y + Txt->size().height() + 4 ), ChkFlags );
	mChkMirrored->text( "Mirrored" );
	mChkMirrored->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickMirrored ) );

	mChkFliped = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkMirrored->position().x + mChkMirrored->size().width() + 32, mChkMirrored->position().y ), ChkFlags );
	mChkFliped->text( "Fliped" );
	mChkFliped->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickFliped ) );

	mChkBlocked = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkMirrored->position().x, mChkMirrored->position().y + mChkMirrored->size().height() + 4 ), ChkFlags );
	mChkBlocked->text( "Blocked" );
	mChkBlocked->tooltipText( "Blocks the tile occupied by the sprite." );
	mChkBlocked->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickBlocked ) );

	mChkAnim = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkFliped->position().x, mChkFliped->position().y + mChkFliped->size().height() + 4 ), ChkFlags );
	mChkAnim->text( "Animated" );
	mChkAnim->tooltipText( "Indicates if the Sprite is animated." );
	mChkAnim->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickAnimated ) );

	mChkRot90 = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkBlocked->position().x, mChkBlocked->position().y + mChkBlocked->size().height() + 4 ), ChkFlags );
	mChkRot90->text( String::fromUtf8( "Rotate 90º" ) );
	mChkRot90->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickRot90 ) );

	mChkAutoFix = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkAnim->position().x, mChkAnim->position().y + mChkAnim->size().height() + 4 ), ChkFlags );
	mChkAutoFix->text( "AutoFix TilePos" );
	mChkAutoFix->tooltipText( "In a tiled layer if the sprite is moved,\nit will update the current tile position automatically." );
	mChkAutoFix->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickAutoFix ) );

	Txt = mTheme->createTextBox( "Game Object Data:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mChkRot90->position().y + mChkRot90->size().height() + 8 ), TxtFlags );

	mChkDI = mTheme->createCheckBox( mSubTextureCont, Sizei(), Vector2i( TAB_CONT_X_DIST, Txt->position().y + Txt->size().height() + 4 ), ChkFlags );
	mChkDI->text( "Add as DataId" );
	mChkDI->tooltipText( "If the resource it's not a sprite,\nyou can reference it with a data id" );
	mChkDI->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickDI ) );

	UIComplexControl::CreateParams SGParams;
	SGParams.Parent( mSubTextureCont );
	SGParams.PosSet( Vector2i( TAB_CONT_X_DIST, mChkDI->position().y + mChkDI->size().height() + 8 ) );
	SGParams.SizeSet( Sizei( Width, 400 ) );
	SGParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mSGCont = eeNew( UIComplexControl, ( SGParams ) );
	mSGCont->enabled( true );
	mSGCont->visible( true );

	Txt = mTheme->createTextBox( "Texture Atlases:", mSGCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 0 ), TxtFlags );

	mTextureAtlasesList = mTheme->createDropDownList( mSGCont, Sizei( Width, 21 ), Vector2i( 0, Txt->position().y +Txt->size().height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mTextureAtlasesList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnTextureAtlasChange ) );

	mSubTextureList = mTheme->createListBox( mSGCont, Sizei( Width, 156 ), Vector2i( 0, mTextureAtlasesList->position().y + mTextureAtlasesList->size().height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mSubTextureList->size( mSubTextureList->size().width(), mSubTextureList->rowHeight() * 9 + mSubTextureList->paddingContainer().Top + mSubTextureList->paddingContainer().Bottom );
	mSubTextureList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnSubTextureChange ) );

	mGfxPreview = mTheme->createGfx( NULL, mSGCont, Sizei( Width, Width ), Vector2i( 0, mSubTextureList->position().y + mSubTextureList->size().height() + 4 ), UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_AUTO_FIT );
	mGfxPreview->border( true );
	mGfxPreview->border()->color( ColorA( 0, 0, 0, 200 ) );

	UIComplexControl::CreateParams DIParams;
	DIParams.Parent( mSubTextureCont );
	DIParams.PosSet( SGParams.Pos );
	DIParams.SizeSet( Sizei( Width, 400 ) );
	DIParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mDICont = eeNew( UIComplexControl, ( DIParams ) );
	mDICont->enabled( false );
	mDICont->visible( false );

	Txt = mTheme->createTextBox( "DataId String:", mDICont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 0 ), TxtFlags );

	mDataIdInput = mTheme->createTextInput( mDICont, Sizei( Width / 4 * 3, 21 ), Vector2i( TAB_CONT_X_DIST + 8, Txt->position().y + Txt->size().height() + 8 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	FillSGCombo();
}

void MapEditor::CreateLighContainer() {
	UIPushButton * NewLightBut = mTheme->createPushButton( mLightCont, Sizei( mLightCont->size().width() - TAB_CONT_X_DIST * 2, 22 ), Vector2i( TAB_CONT_X_DIST, 0 ) );
	NewLightBut->text( "New Light" );
	NewLightBut->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnNewLight ) );

	UITextBox * Txt = mTheme->createTextBox( "Light Color:", mLightCont, Sizei(), Vector2i( TAB_CONT_X_DIST, 32 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	UIComplexControl::CreateParams ComParams;
	ComParams.Parent( mLightCont );
	ComParams.PosSet( Txt->position().x, Txt->position().y + Txt->size().height() + 4 );
	ComParams.SizeSet( 58, 64 );
	ComParams.Background.color( ColorA(255,255,255,255) );
	ComParams.Border.color( ColorA( 100, 100, 100, 200 ) );
	ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
	mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
	mUIBaseColor->visible( true );
	mUIBaseColor->enabled( true );

	Txt = mTheme->createTextBox( "R:", mLightCont, Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIBaseColor->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIRedSlider = mTheme->createSlider( mLightCont, Sizei( 100, 20 ), Vector2i( Txt->position().x + Txt->size().width(), Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIRedSlider->maxValue( 255 );
	mUIRedSlider->value( 255 );
	mUIRedSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnRedChange ) );

	mUIRedTxt = mTheme->createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIRedSlider->position().x + mUIRedSlider->size().width() + 4, mUIRedSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "G:", mLightCont, Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIRedSlider->position().y + mUIRedSlider->size().height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIGreenSlider = mTheme->createSlider( mLightCont, Sizei( 100, 20 ), Vector2i( mUIRedSlider->position().x, Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIGreenSlider->maxValue( 255 );
	mUIGreenSlider->value( 255 );
	mUIGreenSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnGreenChange ) );

	mUIGreenTxt = mTheme->createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIGreenSlider->position().x + mUIGreenSlider->size().width() + 4, mUIGreenSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "B:", mLightCont, Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIGreenSlider->position().y + mUIGreenSlider->size().height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIBlueSlider = mTheme->createSlider( mLightCont, Sizei( 100, 20 ), Vector2i( mUIRedSlider->position().x, Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIBlueSlider->maxValue( 255 );
	mUIBlueSlider->value( 255 );
	mUIBlueSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnBlueChange ) );

	mUIBlueTxt = mTheme->createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIBlueSlider->position().x + mUIBlueSlider->size().width() + 4, mUIBlueSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "Light Radius:", mLightCont, Sizei(), Vector2i( TAB_CONT_X_DIST, mUIBlueTxt->position().y + mUIBlueTxt->size().height() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mLightRadius = mTheme->createSpinBox( mLightCont, Sizei( 100, 22 ), Vector2i( Txt->position().x, Txt->position().y + Txt->size().height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, 100, false );
	mLightRadius->maxValue( 2000 );
	mLightRadius->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnLightRadiusChangeVal ) );

	mLightTypeChk = mTheme->createCheckBox( mLightCont, Sizei(), Vector2i( mLightRadius->position().x, mLightRadius->position().y + mLightRadius->size().height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mLightTypeChk->text( "Isometric Light" );
	mLightTypeChk->active( false );
	mLightTypeChk->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnLightTypeChange ) );
}

UISelectButton * MapEditor::AddObjContButton( String text, Uint32 mode ) {
	UISelectButton * Button = mTheme->createSelectButton( mObjectCont, Sizei( mObjectCont->size().width() - TAB_CONT_X_DIST * 2, 22 ), Vector2i( TAB_CONT_X_DIST, mLastSelButtonY ) );

	Button->text( text );
	Button->data( mode );

	Button->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnObjectModeSel ) );

	mLastSelButtonY += Button->size().height() + 4;

	mObjContButton.push_back( Button );

	return Button;
}

void MapEditor::CreateObjectsContainer() {
	AddObjContButton( "Select Objects", UIMap::SELECT_OBJECTS )->select();
	AddObjContButton( "Edit Polygons", UIMap::EDIT_POLYGONS );
	AddObjContButton( "Insert Object", UIMap::INSERT_OBJECT );
	AddObjContButton( "Insert Polygon", UIMap::INSERT_POLYGON );
	UISelectButton * Button = AddObjContButton( "Insert Polyline", UIMap::INSERT_POLYLINE );

	Int32 nextY = Button->position().y + Button->size().height() + 4;

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkClampToTile = mTheme->createCheckBox( mObjectCont, Sizei(), Vector2i( 12, nextY ), ChkFlags );
	mChkClampToTile->text( "Clamp Position to Tile" );
	mChkClampToTile->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickClampToTile ) );
	mChkClampToTile->active( true );
}

void MapEditor::OnObjectModeSel( const UIEvent * Event ) {
	UISelectButton * Button = static_cast<UISelectButton*>( Event->getControl() );
	UISelectButton * ButtonT = NULL;

	for ( std::list<UISelectButton*>::iterator it = mObjContButton.begin(); it != mObjContButton.end(); it++ ) {
		ButtonT = *it;

		ButtonT->unselect();
	}

	Button->select();

	mUIMap->EditingObjMode( (UIMap::EDITING_OBJ_MODE)Button->data() );
}

void MapEditor::CreateUIMap() {
	UISkin * HScrollSkin = mTheme->getByName( mTheme->abbr() + "_" + "hscrollbar_bg" );
	UISkin * VScrollSkin = mTheme->getByName( mTheme->abbr() + "_" + "vscrollbar_bg" );

	Float ScrollH = 16;
	Float ScrollV = 16;

	if ( NULL != HScrollSkin ) {
		SubTexture * tTex = HScrollSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tTex ) {
			ScrollH = tTex->size().height();
		}
	}

	if ( NULL != VScrollSkin ) {
		SubTexture * tTex = VScrollSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tTex ) {
			ScrollV = tTex->size().height();
		}
	}

	UIComplexControl::CreateParams Params;
	Params.Parent( mWinContainer );
	Params.PosSet( 0, 0 );
	Params.SizeSet( mWinContainer->size().width() - 225 - ScrollV, mWinContainer->size().height() - ScrollH );

	Params.Flags |= UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT;
	mUIMap = eeNew( UIMap, ( Params, mTheme ) );
	mUIMap->visible( true );
	mUIMap->enabled( true );
	CreateNewEmptyMap();
	mUIMap->addEventListener( UIEvent::EventOnSizeChange, cb::Make1( this, &MapEditor::OnMapSizeChange ) );
	mUIMap->addEventListener( UIEvent::EventMouseDown, cb::Make1( this, &MapEditor::OnMapMouseDown ) );
	mUIMap->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnMapMouseClick ) );
	mUIMap->SetLightSelectCb( cb::Make1( this, &MapEditor::OnLightSelect ) );
	mUIMap->SetLightRadiusChangeCb( cb::Make1( this, &MapEditor::OnLightRadiusChange ) );
	mUIMap->SetAddObjectCallback( cb::Make2( this, &MapEditor::OnAddObject ) );
	mUIMap->SetAlertCb( cb::Make2( this, &MapEditor::CreateAlert ) );
	mUIMap->SetUpdateScrollCb( cb::Make0( this, &MapEditor::UpdateScroll ) );
	mUIMap->SetTileBox( mTileBox );

	mMapHScroll = mTheme->createScrollBar( mWinContainer, Sizei( Params.Size.width(), ScrollH ), Vector2i( 0, mWinContainer->size().height() - ScrollH ), UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM | UI_AUTO_SIZE );
	mMapHScroll->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnScrollMapH ) );

	mMapVScroll = mTheme->createScrollBar( mWinContainer, Sizei( ScrollV, Params.Size.height() ), Vector2i( Params.Size.width() + ScrollV, 0 ), UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_AUTO_SIZE , true );
	mMapVScroll->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnScrollMapV ) );

	MapCreated();
}

void MapEditor::OnAddObject( Uint32 Type, Polygon2f poly ) {
	if ( NULL == mCurLayer ) {
		CreateNoLayerAlert( "No layers found" )->setFocus();
		return;
	}

	if ( mCurLayer->Type() != MAP_LAYER_OBJECT ) {
		CreateAlert( "Wrong Layer", "Objects only can be added to an Object Layer" )->setFocus();
		return;
	}

	if ( poly.size() < 3 ) {
		return;
	}

	MapObjectLayer * OL = static_cast<MapObjectLayer*> ( mCurLayer );

	if ( GAMEOBJECT_TYPE_OBJECT == Type ) {
		OL->AddGameObject( eeNew( GameObjectObject, ( mUIMap->Map()->GetNewObjectId(), poly.toAABB(), mCurLayer ) ) );
	} else if ( GAMEOBJECT_TYPE_POLYGON == Type ) {
		OL->AddGameObject( eeNew( GameObjectPolygon, ( mUIMap->Map()->GetNewObjectId(), poly, mCurLayer ) ) );
	} else if ( GAMEOBJECT_TYPE_POLYLINE == Type ) {
		OL->AddGameObject( eeNew( GameObjectPolyline, ( mUIMap->Map()->GetNewObjectId(), poly, mCurLayer ) ) );
	}
}

void MapEditor::OnLightTypeChange( const UIEvent * Event ) {
	if ( NULL != mUIMap->GetSelectedLight() ) {
		mUIMap->GetSelectedLight()->Type( mLightTypeChk->active() ? LIGHT_ISOMETRIC : LIGHT_NORMAL );
	}
}

void MapEditor::OnLightRadiusChangeVal( const UIEvent * Event ) {
	if ( NULL != mUIMap->GetSelectedLight() ) {
		mUIMap->GetSelectedLight()->Radius( mLightRadius->value() );
	}
}

void MapEditor::OnLightRadiusChange( MapLight * Light ) {
	mLightRadius->value( Light->Radius() );
}

void MapEditor::OnLightSelect( MapLight * Light ) {
	ColorA Col( Light->Color() );

	mUIRedSlider->value( Col.r() );
	mUIGreenSlider->value( Col.g() );
	mUIBlueSlider->value( Col.b() );
	mLightRadius->value( Light->Radius() );
	mLightTypeChk->active( Light->Type() == LIGHT_ISOMETRIC ? true : false );
}

void MapEditor::OnNewLight( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
		Vector2i Pos = mUIMap->Map()->GetMouseMapPos();
		mUIMap->AddLight( eeNew( MapLight, ( mLightRadius->value(), Pos.x, Pos.y, mUIBaseColor->background()->color().ToColor(), mLightTypeChk->active() ? LIGHT_ISOMETRIC : LIGHT_NORMAL ) ) );
	}
}

void MapEditor::OnRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Red = (Uint8)mUIRedSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIRedTxt->text( String::toStr( (Int32)mUIRedSlider->value() ) );

	if ( NULL != mUIMap->GetSelectedLight() ) {
		RGB lCol( mUIMap->GetSelectedLight()->Color() );
		lCol.Red = Col.r();
		mUIMap->GetSelectedLight()->Color( lCol );
	}
}

void MapEditor::OnGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Green = (Uint8)mUIGreenSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIGreenTxt->text( String::toStr( (Uint32)mUIGreenSlider->value() ) );

	if ( NULL != mUIMap->GetSelectedLight() ) {
		RGB lCol( mUIMap->GetSelectedLight()->Color() );
		lCol.Green = Col.g();
		mUIMap->GetSelectedLight()->Color( lCol );
	}
}

void MapEditor::OnBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Blue = (Uint8)mUIBlueSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIBlueTxt->text( String::toStr( (Uint32)mUIBlueSlider->value() ) );

	if ( NULL != mUIMap->GetSelectedLight() ) {
		RGB lCol( mUIMap->GetSelectedLight()->Color() );
		lCol.Blue = Col.b();
		mUIMap->GetSelectedLight()->Color( lCol );
	}
}

void MapEditor::ChkClickDI( const UIEvent * Event ) {
	if ( mChkDI->active() ) {
		mSGCont->enabled( false );
		mSGCont->visible( false );
		mDICont->enabled( true );
		mDICont->visible( true );
	} else {
		mSGCont->enabled( true );
		mSGCont->visible( true );
		mDICont->enabled( false );
		mDICont->visible( false );
	}
}

void MapEditor::ChkClickClampToTile( const UIEvent * Event ) {
	mUIMap->ClampToTile( mChkClampToTile->active() );
}

void MapEditor::UpdateGfx() {
	if ( mChkMirrored->active() && mChkFliped->active() )
		mGfxPreview->renderMode( RN_FLIPMIRROR );
	else if( mChkMirrored->active() )
		mGfxPreview->renderMode( RN_MIRROR );
	else if ( mChkFliped->active() )
		mGfxPreview->renderMode( RN_FLIP );
	else
		mGfxPreview->renderMode( RN_NORMAL );

	if ( mChkRot90->active() )
		mGfxPreview->angle( 90 );
	else
		mGfxPreview->angle( 0 );
}

void MapEditor::UpdateFlags() {
	mCurGOFlags = 0;

	if ( mChkMirrored->active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_MIRRORED;

	if ( mChkFliped->active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_FLIPED;

	if ( mChkBlocked->active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_BLOCKED;

	if ( mChkAnim->active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ANIMATED;

	if ( mChkRot90->active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ROTATE_90DEG;

	if ( mChkAutoFix->active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_AUTO_FIX_TILE_POS;
}

void MapEditor::OnTypeChange( const UIEvent * Event ) {
	if ( mGOTypeList->text() == "SubTexture" )
		mCurGOType = GAMEOBJECT_TYPE_SUBTEXTURE;
	else if ( mGOTypeList->text() == "SubTextureEx" )
		mCurGOType = GAMEOBJECT_TYPE_SUBTEXTUREEX;
	else if ( mGOTypeList->text() == "Sprite" )
		mCurGOType = GAMEOBJECT_TYPE_SPRITE;
	else
		mCurGOType = String::hash( mGOTypeList->text().toUtf8() );

	if ( NULL != mChkAnim && NULL != mGOTypeList && mChkAnim->active() && mGOTypeList->text() != "Sprite" ) {
		if ( mGOTypeList->text() == "SubTexture" || mGOTypeList->text() == "SubTextureEx" ) {
			mChkAnim->active( false );
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

	if ( mChkAnim->active() && ( mGOTypeList->text() == "SubTexture" || mGOTypeList->text() == "SubTextureEx" ) ) {
		mGOTypeList->getListBox()->setSelected( "Sprite" );
	}
}

void MapEditor::AddNewGOType( const UIEvent * Event ) {
	eeNew( UIGOTypeNew, ( cb::Make2( this, &MapEditor::OnNewGOTypeAdded ) ) );
}

void MapEditor::OnNewGOTypeAdded( std::string name, Uint32 hash ) {
	if ( "" != name ) {
		for ( Uint32 i = 0; i < mGOTypeList->getListBox()->count(); i++ ) {
			UIListBoxItem * Item = mGOTypeList->getListBox()->getItem(i);

			if ( Item->text() == name )
				return;
		}

		mGOTypeList->getListBox()->addListBoxItem( name );
		mUIMap->Map()->AddVirtualObjectType( name );
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

	if ( mTextureAtlasesList->getListBox()->count() && NULL == mTextureAtlasesList->getListBox()->getItemSelected() ) {
		mTextureAtlasesList->getListBox()->setSelected( 0 );
	}
}

void MapEditor::FillSubTextureList() {
	TextureAtlasManager * SGM = TextureAtlasManager::instance();
	mCurSG = SGM->getByName( mTextureAtlasesList->text() );
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

	mSubTextureList->verticalScrollBar()->clickStep( 8.f / (Float)mSubTextureList->count() );
}

void MapEditor::OnSubTextureChange( const UIEvent * Event ) {
	if ( NULL != mCurSG ) {
		SubTexture * tSubTexture = mCurSG->getByName( mSubTextureList->getItemSelectedText() );

		if ( NULL != tSubTexture ) {
			mGfxPreview->subTexture( tSubTexture );
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
	mUIMap->Map()->Create( Sizei( 100, 100 ), 16, Sizei( 32, 32 ), MAP_EDITOR_DEFAULT_FLAGS | MAP_FLAG_LIGHTS_ENABLED, mUIMap->size() );
}

void MapEditor::MapCreated() {
	mCurLayer = NULL;
	mLayerList->getListBox()->clear();
	UpdateFlags();
	SetViewOptions();
	FillSGCombo();
	FillGotyList();

	mMapHScroll->value( 0 );
	mMapVScroll->value( 0 );
	OnMapSizeChange( NULL );

	mUIMap->ClearLights();

	CreateTabs();
}

void MapEditor::OnMapSizeChange( const UIEvent *Event ) {
	if ( mMouseScrolling )
		return;

	Vector2i v( mUIMap->Map()->GetMaxOffset() );

	mMapHScroll->minValue( 0 );
	mMapHScroll->maxValue( v.x );
	mMapHScroll->clickStep( mUIMap->Map()->TileSize().width() * mUIMap->Map()->Scale() );
	mMapVScroll->minValue( 0 );
	mMapVScroll->maxValue( v.y );
	mMapVScroll->clickStep( mUIMap->Map()->TileSize().height() * mUIMap->Map()->Scale() );
}

void MapEditor::OnScrollMapH( const UIEvent * Event ) {
	if ( mMouseScrolling )
		return;

	UIScrollBar * Scr = reinterpret_cast<UIScrollBar*> ( Event->getControl() );

	Vector2f Off = mUIMap->Map()->Offset();

	Off.x = -Scr->value();

	mUIMap->Map()->Offset( Off ) ;
}

void MapEditor::OnScrollMapV( const UIEvent * Event ) {
	UIScrollBar * Scr = reinterpret_cast<UIScrollBar*> ( Event->getControl() );

	Vector2f Off = mUIMap->Map()->Offset();

	Off.y = -Scr->value();

	mUIMap->Map()->Offset( Off ) ;
}

void MapEditor::UpdateScroll() {
	mMouseScrolling = true;
	mMapHScroll->value( -mUIMap->Map()->Offset().x );
	mMapVScroll->value( -mUIMap->Map()->Offset().y );
	mMouseScrolling = false;
}

void MapEditor::MapOpen( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	if ( mUIMap->Map()->Load( CDL->getFullPath() ) ) {
		OnMapLoad();
	}
}

void MapEditor::OnMapLoad() {
	mCurLayer = NULL;

	mUIMap->Map()->ViewSize( mUIMap->size() );

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

	mUIMap->Map()->Save( path );
}

void MapEditor::FileMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->text();

	if ( "New..." == txt ) {
		CreateNewMap();
	} else if ( "Open..." == txt ) {
		UICommonDialog * TGDialog = mTheme->createCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS, "*.eem" );

		TGDialog->title( "Open Map" );
		TGDialog->addEventListener( UIEvent::EventOpenFile, cb::Make1( this, &MapEditor::MapOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save As..." == txt ) {
		UICommonDialog * TGDialog = mTheme->createCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_SAVE_DIALOG, "*.eem" );

		TGDialog->title( "Save Map" );
		TGDialog->addEventListener( UIEvent::EventSaveFile, cb::Make1( this, &MapEditor::MapSave ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save" == txt ) {
		if ( mUIMap->Map()->Path().size() ) {
			mUIMap->Map()->Save( mUIMap->Map()->Path() );
		}
	} else if ( "Close" == txt ) {
		UIMessageBox * MsgBox = mTheme->createMessageBox( MSGBOX_OKCANCEL, "Do you really want to close the current map?\nAll changes will be lost." );
		MsgBox->addEventListener( UIEvent::EventMsgBoxConfirmClick, cb::Make1( this, &MapEditor::OnMapClose ) );
		MsgBox->title( "Close Map?" );
		MsgBox->center();
		MsgBox->show();
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == UIManager::instance()->mainControl() ) {
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

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->text();

	if ( "Show Grid" == txt ) {
		mUIMap->Map()->DrawGrid( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->active() );
	} else if ( "Mark Tile Over" == txt ) {
		mUIMap->Map()->DrawTileOver( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->active() );
	} else if ( "Show Blocked" == txt ) {
		mUIMap->Map()->ShowBlocked( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->active() );
	} else if ( "Zoom In" == txt ) {
		ZoomIn();
	} else if ( "Zoom Out" == txt ) {
		ZoomOut();
	} else if ( "Normal Size" == txt ) {
		mUIMap->Map()->Scale( 1 );
	}
}

void MapEditor::ZoomIn() {
	TileMap * Map = mUIMap->Map();
	Float S = mUIMap->Map()->Scale();

	if ( S < 4 ) {
		if ( 0.0625f == S ) {
			Map->Scale( 0.125f );
		} else if ( 0.125f == S ) {
			Map->Scale( 0.25f );
		} else if ( 0.25f == S ) {
			Map->Scale( 0.5f );
		} else if ( 0.5f == S ) {
			Map->Scale( 0.75f );
		} else if ( 0.75f == S ) {
			Map->Scale( 1.0f );
		} else if ( 1.0f == S ) {
			Map->Scale( 1.5f );
		} else if ( 1.5f == S ) {
			Map->Scale( 2.0f );
		} else if ( 2.0f == S ) {
			Map->Scale( 3.0f );
		} else if ( 3.0f == S ) {
			Map->Scale( 4.0f );
		}
	}

	OnMapSizeChange();
}

void MapEditor::ZoomOut() {
	TileMap * Map = mUIMap->Map();
	Float S = mUIMap->Map()->Scale();

	if ( S > 0.0625f ) {
		if ( 0.125f == S ) {
			Map->Scale( 0.0625f );
		} else if ( 0.25f == S ) {
			Map->Scale( 0.125f );
		} else if ( 0.5f == S ) {
			Map->Scale( 0.25f );
		} else if ( 0.75f == S ) {
			Map->Scale( 0.5f );
		} else if ( 1.0f == S ) {
			Map->Scale( 0.75f );
		} else if ( 1.5f == S ) {
			Map->Scale( 1.0f );
		} else if ( 2.0f == S ) {
			Map->Scale( 1.5f );
		} else if ( 3.0f == S ) {
			Map->Scale( 2.0f );
		} else if ( 4.0f == S ) {
			Map->Scale( 3.0f );
		}
	}

	OnMapSizeChange();
}

void MapEditor::MapMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->text();

	if ( "New Texture Atlas..." == txt ) {
		UIWindow * tWin = mTheme->createWindow( NULL, Sizei( 1024, 768 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, Sizei( 1024, 768 ) );
		eeNew ( Tools::TextureAtlasEditor, ( tWin ) );
		tWin->center();
		tWin->show();
	} else if ( "Add External Texture Atlas..." == txt ) {
		UICommonDialog * TGDialog = mTheme->createCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );

		TGDialog->title( "Load Texture Atlas..." );
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

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->text();

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
			mCurLayer->LightsEnabled( !mCurLayer->LightsEnabled() );
		}
	} else if ( "Visible" == txt ) {
		if ( NULL != mCurLayer ) {
			mCurLayer->Visible( !mCurLayer->Visible() );
		}
	}
}

UIMessageBox * MapEditor::CreateAlert( const String& title, const String& text ) {
	UIMessageBox * MsgBox = mTheme->createMessageBox( MSGBOX_OK, text );
	MsgBox->title( title );
	MsgBox->center();
	MsgBox->show();
	return MsgBox;
}

UIMessageBox * MapEditor::CreateNoLayerAlert( const String title ) {
	return CreateAlert( title, "First select and add a new layer." );
}

void MapEditor::MoveLayerUp() {
	if ( mUIMap->Map()->MoveLayerUp( mCurLayer ) ) {
		RefreshLayersList();
	}
}

void MapEditor::MoveLayerDown() {
	if ( mUIMap->Map()->MoveLayerDown( mCurLayer ) ) {
		RefreshLayersList();
	}
}

void MapEditor::RemoveLayer() {
	if ( mUIMap->Map()->RemoveLayer( mCurLayer ) ) {
		mCurLayer = NULL;

		RefreshLayersList();
	}
}

void MapEditor::RefreshGotyList() {
	TileMap::GOTypesList& GOList = mUIMap->Map()->GetVirtualObjectTypes();

	for ( TileMap::GOTypesList::iterator it = GOList.begin(); it != GOList.end(); it++ ) {
		mGOTypeList->getListBox()->addListBoxItem( (*it) );
	}
}

void MapEditor::RefreshLayersList() {
	mLayerList->getListBox()->clear();

	if ( mUIMap->Map()->LayerCount() ) {
		std::vector<String> layers;

		for ( Uint32 i = 0; i < mUIMap->Map()->LayerCount(); i++ ) {
			layers.push_back( mUIMap->Map()->GetLayer(i)->Name() );
		}

		mLayerList->getListBox()->addListBoxItems( layers );
	}

	if ( NULL != mCurLayer ) {
		mLayerList->getListBox()->setSelected( mCurLayer->Name() );
	} else {
		if ( mUIMap->Map()->LayerCount() ) {
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
	bool SetSelected = ( 0 == mLayerList->getListBox()->count() ) ? true : false;

	mLayerList->getListBox()->addListBoxItem( UILayer->Name() );

	if ( SetSelected ) {
		mCurLayer = UILayer->Layer();

		mUIMap->CurLayer( mCurLayer );

		mLayerList->getListBox()->setSelected(0);
	}
}

void MapEditor::OnLayerSelect( const UIEvent * Event ) {
	MapLayer * tLayer = mUIMap->Map()->GetLayer( mLayerList->text() );

	if ( NULL != tLayer ) {
		mCurLayer = tLayer;

		mUIMap->CurLayer( mCurLayer );

		mLayerChkVisible->active( mCurLayer->Visible() );

		mLayerChkLights->active( mCurLayer->LightsEnabled() );
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

		tObj = eeNew( GameObjectSubTexture, ( mCurGOFlags, mCurLayer, mGfxPreview->subTexture() ) );

	} else if ( GAMEOBJECT_TYPE_SUBTEXTUREEX == mCurGOType ) {

		tObj = eeNew( GameObjectSubTextureEx, ( mCurGOFlags, mCurLayer, mGfxPreview->subTexture() ) );

	} else if ( GAMEOBJECT_TYPE_SPRITE == mCurGOType ) {

		if ( mChkAnim->active() ) {

			Sprite * tAnimSprite = eeNew( Sprite, ( String::removeNumbersAtEnd( mGfxPreview->subTexture()->getName() ) ) );
			tObj = eeNew( GameObjectSprite, ( mCurGOFlags, mCurLayer, tAnimSprite ) );

		} else {

			Sprite * tStatiSprite = eeNew( Sprite, ( mGfxPreview->subTexture() ) );
			tObj = eeNew( GameObjectSprite, ( mCurGOFlags, mCurLayer, tStatiSprite ) );

		}
	} else {
		//! Creates an empty game object. The client will interpret the GameObject Type, and instanciate the corresponding class.

		if ( mChkDI->active() )
			tObj = eeNew( GameObjectVirtual, ( String::hash( mDataIdInput->text().toUtf8() ), mCurLayer, mCurGOFlags, mCurGOType ) );
		else
			tObj = eeNew( GameObjectVirtual, ( mGfxPreview->subTexture(), mCurLayer, mCurGOFlags, mCurGOType ) );
	}

	return tObj;
}

void MapEditor::AddGameObjectToTile() {
	TileMapLayer * tLayer	= reinterpret_cast<TileMapLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();
	GameObject * tObj	= CreateGameObject();

	if ( NULL != tObj ) {
		if ( tObj->Type() == GAMEOBJECT_TYPE_VIRTUAL )
			reinterpret_cast<GameObjectVirtual*> ( tObj )->SetLayer( tLayer );

		tLayer->AddGameObject( tObj, tMap->GetMouseTilePos() );
	}
}

void MapEditor::RemoveGameObjectFromTile() {
	TileMapLayer * tLayer = reinterpret_cast<TileMapLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();

	tLayer->RemoveGameObject( tMap->GetMouseTilePos() );
}

void MapEditor::AddGameObject() {
	MapObjectLayer * tLayer	= reinterpret_cast<MapObjectLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();
	GameObject * tObj	= CreateGameObject();

	if ( NULL != tObj ) {
		if ( tObj->Type() == GAMEOBJECT_TYPE_VIRTUAL )
			reinterpret_cast<GameObjectVirtual*> ( tObj )->SetLayer( tLayer );

		Vector2i p( tMap->GetMouseMapPos() );

		if ( UIManager::instance()->getInput()->isKeyDown( KEY_LCTRL ) ) {
			p = tMap->GetMouseTilePosCoords();
		}

		tObj->Pos( Vector2f( p.x, p.y ) );
		tLayer->AddGameObject( tObj );
	}
}

void MapEditor::RemoveGameObject() {
	MapObjectLayer * tLayer = reinterpret_cast<MapObjectLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();

	tLayer->RemoveGameObject( tMap->GetMouseMapPos() );
}

GameObject * MapEditor::GetCurrentGOOver() {
	return reinterpret_cast<TileMapLayer*>( mCurLayer )->GetGameObject( mUIMap->Map()->GetMouseTilePos() );
}

void MapEditor::OnMapMouseClick( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( mSubTextureCont->visible() ) {
		if ( NULL == mCurLayer || NULL == mGfxPreview->subTexture() || UIManager::instance()->downControl() != mUIMap ) {
			if ( NULL == mCurLayer )
				CreateNoLayerAlert( "No layers found" );

			return;
		}

		if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_OBJECT )
				AddGameObject();
		} else if ( MEvent->getFlags() & EE_BUTTON_RMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_OBJECT )
				RemoveGameObject();
		} else if ( MEvent->getFlags() & EE_BUTTON_MMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->Blocked( !tObj->Blocked() );
				}
			}
		} else if ( MEvent->getFlags() & EE_BUTTON_WUMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->Mirrored( !tObj->Mirrored() );
				}
			}
		} else if ( MEvent->getFlags() & EE_BUTTON_WDMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->Rotated( !tObj->Rotated() );
				}
			}
		}
	}
}

void MapEditor::OnMapMouseDown( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( mSubTextureCont->visible() ) {
		if ( NULL == mCurLayer || NULL == mGfxPreview->subTexture() || UIManager::instance()->downControl() != mUIMap )
			return;


		if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED )
				AddGameObjectToTile();
		} else if ( MEvent->getFlags() & EE_BUTTON_RMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED )
				RemoveGameObjectFromTile();
		}
	}
}

void MapEditor::SetViewOptions() {
	mChkShowGrid->active( mUIMap->Map()->DrawGrid() ? true : false );
	mChkMarkTileOver->active( mUIMap->Map()->DrawTileOver() ? true : false  );
	mChkShowBlocked->active( mUIMap->Map()->ShowBlocked() ? true : false );
}

}}
