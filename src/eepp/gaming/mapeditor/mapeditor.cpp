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
#include <eepp/ui/cuimanager.hpp>
#include <eepp/ui/cuithememanager.hpp>
#include <eepp/ui/cuiwinmenu.hpp>
#include <eepp/ui/cuipopupmenu.hpp>
#include <eepp/ui/cuispinbox.hpp>
#include <eepp/ui/cuicheckbox.hpp>
#include <eepp/ui/cuicommondialog.hpp>
#include <eepp/ui/cuimessagebox.hpp>
#include <eepp/ui/cuitabwidget.hpp>
#include <eepp/ui/tools/ctextureatlaseditor.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>

#include <algorithm>

using namespace EE::Graphics;
using namespace EE::Gaming::Private;

#define TAB_CONT_X_DIST 3

namespace EE { namespace Gaming {

MapEditor::MapEditor( cUIWindow * AttatchTo, const MapEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mTheme( cUIThemeManager::instance()->DefaultTheme() ),
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
		mUIWindow = cUIManager::instance()->MainControl();
		mUIWindow->SetSkinFromTheme( mTheme, "winback" );
	}

	if ( cUIManager::instance()->MainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->Container();
	}

	mUIWindow->Title( "Map Editor" );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &MapEditor::WindowClose ) );

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
	cUIWinMenu * WinMenu = mTheme->CreateWinMenu( mUIContainer );

	mTileBox = mTheme->CreateTextBox( "", mUIContainer, Sizei(), Vector2i(), UI_HALIGN_RIGHT | UI_VALIGN_CENTER | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	mTileBox->Size( 100, WinMenu->Size().Height() );
	mTileBox->Pos( Vector2i( mUIContainer->Size().Width() - mTileBox->Size().Width(), 0 ) );
	mTileBox->UpdateAnchorsDistances();

	cUIPopUpMenu * PU1 = mTheme->CreatePopUpMenu( mUIContainer );
	PU1->Add( "New...", mTheme->GetIconByName( "document-new" ) );
	PU1->Add( "Open...", mTheme->GetIconByName( "document-open" ) );
	PU1->AddSeparator();
	PU1->Add( "Save", mTheme->GetIconByName( "document-save" ) );
	PU1->Add( "Save As...", mTheme->GetIconByName( "document-save-as" ) );
	PU1->AddSeparator();
	PU1->Add( "Close", mTheme->GetIconByName( "document-close" ) );
	PU1->AddSeparator();
	PU1->Add( "Quit", mTheme->GetIconByName( "quit" ) );

	PU1->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::FileMenuClick ) );
	WinMenu->AddMenuButton( "File", PU1 );

	cUIPopUpMenu * PU3 = mTheme->CreatePopUpMenu( mUIContainer );
	mChkShowGrid = reinterpret_cast<cUIMenuCheckBox*>( PU3->GetItem( PU3->AddCheckBox( "Show Grid" ) ) );

	mChkShowGrid->Active( true );

	mChkMarkTileOver = reinterpret_cast<cUIMenuCheckBox*>( PU3->GetItem( PU3->AddCheckBox( "Mark Tile Over" ) ) );

	mChkShowBlocked = reinterpret_cast<cUIMenuCheckBox*>( PU3->GetItem( PU3->AddCheckBox( "Show Blocked" ) ) );

	PU3->AddSeparator();
	mUIWindow->AddShortcut( KEY_KP_PLUS	, KEYMOD_CTRL, reinterpret_cast<cUIPushButton*> ( PU3->GetItem( PU3->Add( "Zoom In", mTheme->GetIconByName( "zoom-in" ) ) ) ) );
	mUIWindow->AddShortcut( KEY_KP_MINUS, KEYMOD_CTRL, reinterpret_cast<cUIPushButton*> ( PU3->GetItem( PU3->Add( "Zoom Out", mTheme->GetIconByName( "zoom-out" ) ) ) ) );
	mUIWindow->AddShortcut( KEY_KP0		, KEYMOD_CTRL, reinterpret_cast<cUIPushButton*> ( PU3->GetItem( PU3->Add( "Normal Size", mTheme->GetIconByName( "zoom-original" ) ) ) ) );
	PU3->AddSeparator();

	PU3->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::ViewMenuClick ) );
	WinMenu->AddMenuButton( "View", PU3 );

	cUIPopUpMenu * PU4 = mTheme->CreatePopUpMenu( mUIContainer );
	PU4->Add( "Properties..." );
	PU4->Add( "Resize..." );

	PU4->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::MapMenuClick ) );
	WinMenu->AddMenuButton( "Map", PU4 );

	cUIPopUpMenu * PU5 = mTheme->CreatePopUpMenu( mUIContainer );
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
	mLayerChkLights = reinterpret_cast<cUIMenuCheckBox*> ( PU5->GetItem( LayerChkBoxIndex ) );

	PU5->AddSeparator();

	LayerChkBoxIndex = PU5->AddCheckBox( "Visible" );
	mLayerChkVisible = reinterpret_cast<cUIMenuCheckBox*> ( PU5->GetItem( LayerChkBoxIndex ) );

	PU5->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::LayerMenuClick ) );
	WinMenu->AddMenuButton( "Layer", PU5 );

	cUIPopUpMenu * PU6 = mTheme->CreatePopUpMenu( mUIContainer );
	PU6->Add( "New Texture Atlas..." );
	PU6->Add( "Add External Texture Atlas..." );
	WinMenu->AddMenuButton( "Atlases", PU6 );
	PU6->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &MapEditor::MapMenuClick ) );

	cUIComplexControl::CreateParams Params;
	Params.Parent( mUIContainer );
	Params.PosSet( 0, WinMenu->Size().Height() );
	Params.SizeSet( mUIContainer->Size().Width(), mUIContainer->Size().Height() - WinMenu->Size().Height() );
	Params.Flags = UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_REPORT_SIZE_CHANGE_TO_CHILDS;
	mWinContainer = eeNew( cUIComplexControl, ( Params ) );
	mWinContainer->Visible( true );
	mWinContainer->Enabled( true );
}

void MapEditor::CreateETGMenu() {
	Int32 Width = 200;
	Int32 DistToBorder = 5;
	Int32 ContPosX = mWinContainer->Size().Width() - Width - DistToBorder;
	Int32 DistFromTopMenu = 4;

	cUIComplexControl::CreateParams CParams;
	CParams.Parent( mWinContainer );
	CParams.SizeSet( Sizei( Width + DistToBorder, mWinContainer->Size().Height() ) );
	CParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mSubTextureCont = eeNew( cUIComplexControl, ( CParams ) );
	mSubTextureCont->Enabled( true );
	mSubTextureCont->Visible( true );

	mLightCont = eeNew( cUIComplexControl, ( CParams ) );

	mObjectCont = eeNew( cUIComplexControl, ( CParams ) );

	mTabWidget = mTheme->CreateTabWidget( mWinContainer, Sizei( Width + DistToBorder, mWinContainer->Size().Height() - DistFromTopMenu ), Vector2i( ContPosX, DistFromTopMenu ), UI_HALIGN_CENTER | UI_VALIGN_BOTTOM | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM );
	mTabWidget->AddEventListener( cUIEvent::EventOnTabSelected, cb::Make1( this, &MapEditor::OnTabSelected ) );
	CreateTabs();

	CreateLighContainer();

	CreateSubTextureContainer( Width );

	CreateObjectsContainer();
}

void MapEditor::CreateTabs() {
	mTabWidget->RemoveAll();
	mTabWidget->Add( "Sprites", mSubTextureCont );

	if ( NULL != mUIMap && NULL != mUIMap->Map() ) {
		if ( mUIMap->Map()->LightsEnabled() ) {
			mTabWidget->Add( "Lights", mLightCont );
		}
	}

	mTabWidget->Add( "Objects", mObjectCont );
}

void MapEditor::OnTabSelected( const cUIEvent * Event ) {
	if ( NULL != mUIMap ) {
		switch ( mTabWidget->GetSelectedTabIndex() ) {
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
	mGOTypeList->ListBox()->Clear();
	mGOTypeList->ListBox()->AddListBoxItems( items );
	mGOTypeList->ListBox()->SetSelected(0);
}

void MapEditor::CreateSubTextureContainer( Int32 Width ) {
	cUITextBox * Txt;
	Uint32 TxtFlags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_DRAW_SHADOW;

	Txt = mTheme->CreateTextBox( "Add Game Object as...", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 4 ), TxtFlags );

	mGOTypeList = mTheme->CreateDropDownList( mSubTextureCont, Sizei( Width - 26, 21 ), Vector2i( TAB_CONT_X_DIST, Txt->Pos().y + Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mGOTypeList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnTypeChange ) );
	FillGotyList();

	mBtnGOTypeAdd = mTheme->CreatePushButton( mSubTextureCont, Sizei( 24, 21 ), Vector2i( mGOTypeList->Pos().x + mGOTypeList->Size().Width() + 2, mGOTypeList->Pos().y ), UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mTheme->GetIconByName( "add" ) );
	mBtnGOTypeAdd->TooltipText( "Adds a new game object type\nunknown by the map editor." );
	mBtnGOTypeAdd->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::AddNewGOType ) );

	if ( NULL == mBtnGOTypeAdd->Icon()->SubTexture() )
		mBtnGOTypeAdd->Text( "..." );

	Txt = mTheme->CreateTextBox( "Layers:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mGOTypeList->Pos().y + mGOTypeList->Size().Height() + 4 ), TxtFlags );

	mLayerList = mTheme->CreateDropDownList( mSubTextureCont, Sizei( Width, 21 ), Vector2i( TAB_CONT_X_DIST, Txt->Pos().y + Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mLayerList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnLayerSelect ) );

	Txt = mTheme->CreateTextBox( "Game Object Flags:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mLayerList->Pos().y + mLayerList->Size().Height() + 4 ), TxtFlags );

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkMirrored = mTheme->CreateCheckBox( mSubTextureCont, Sizei(), Vector2i( TAB_CONT_X_DIST, Txt->Pos().y + Txt->Size().Height() + 4 ), ChkFlags );
	mChkMirrored->Text( "Mirrored" );
	mChkMirrored->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickMirrored ) );

	mChkFliped = mTheme->CreateCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkMirrored->Pos().x + mChkMirrored->Size().Width() + 32, mChkMirrored->Pos().y ), ChkFlags );
	mChkFliped->Text( "Fliped" );
	mChkFliped->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickFliped ) );

	mChkBlocked = mTheme->CreateCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkMirrored->Pos().x, mChkMirrored->Pos().y + mChkMirrored->Size().Height() + 4 ), ChkFlags );
	mChkBlocked->Text( "Blocked" );
	mChkBlocked->TooltipText( "Blocks the tile occupied by the sprite." );
	mChkBlocked->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickBlocked ) );

	mChkAnim = mTheme->CreateCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkFliped->Pos().x, mChkFliped->Pos().y + mChkFliped->Size().Height() + 4 ), ChkFlags );
	mChkAnim->Text( "Animated" );
	mChkAnim->TooltipText( "Indicates if the Sprite is animated." );
	mChkAnim->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickAnimated ) );

	mChkRot90 = mTheme->CreateCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkBlocked->Pos().x, mChkBlocked->Pos().y + mChkBlocked->Size().Height() + 4 ), ChkFlags );
	mChkRot90->Text( String::FromUtf8( "Rotate 90º" ) );
	mChkRot90->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickRot90 ) );

	mChkAutoFix = mTheme->CreateCheckBox( mSubTextureCont, Sizei(), Vector2i( mChkAnim->Pos().x, mChkAnim->Pos().y + mChkAnim->Size().Height() + 4 ), ChkFlags );
	mChkAutoFix->Text( "AutoFix TilePos" );
	mChkAutoFix->TooltipText( "In a tiled layer if the sprite is moved,\nit will update the current tile position automatically." );
	mChkAutoFix->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickAutoFix ) );

	Txt = mTheme->CreateTextBox( "Game Object Data:", mSubTextureCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mChkRot90->Pos().y + mChkRot90->Size().Height() + 8 ), TxtFlags );

	mChkDI = mTheme->CreateCheckBox( mSubTextureCont, Sizei(), Vector2i( TAB_CONT_X_DIST, Txt->Pos().y + Txt->Size().Height() + 4 ), ChkFlags );
	mChkDI->Text( "Add as DataId" );
	mChkDI->TooltipText( "If the resource it's not a sprite,\nyou can reference it with a data id" );
	mChkDI->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickDI ) );

	cUIComplexControl::CreateParams SGParams;
	SGParams.Parent( mSubTextureCont );
	SGParams.PosSet( Vector2i( TAB_CONT_X_DIST, mChkDI->Pos().y + mChkDI->Size().Height() + 8 ) );
	SGParams.SizeSet( Sizei( Width, 400 ) );
	SGParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mSGCont = eeNew( cUIComplexControl, ( SGParams ) );
	mSGCont->Enabled( true );
	mSGCont->Visible( true );

	Txt = mTheme->CreateTextBox( "Texture Atlases:", mSGCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 0 ), TxtFlags );

	mTextureAtlasesList = mTheme->CreateDropDownList( mSGCont, Sizei( Width, 21 ), Vector2i( 0, Txt->Pos().y +Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mTextureAtlasesList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnTextureAtlasChange ) );

	mSubTextureList = mTheme->CreateListBox( mSGCont, Sizei( Width, 156 ), Vector2i( 0, mTextureAtlasesList->Pos().y + mTextureAtlasesList->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mSubTextureList->Size( mSubTextureList->Size().Width(), mSubTextureList->RowHeight() * 9 + mSubTextureList->PaddingContainer().Top + mSubTextureList->PaddingContainer().Bottom );
	mSubTextureList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &MapEditor::OnSubTextureChange ) );

	mGfxPreview = mTheme->CreateGfx( NULL, mSGCont, Sizei( Width, Width ), Vector2i( 0, mSubTextureList->Pos().y + mSubTextureList->Size().Height() + 4 ), UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_AUTO_FIT );
	mGfxPreview->Border( true );
	mGfxPreview->Border()->Color( ColorA( 0, 0, 0, 200 ) );

	cUIComplexControl::CreateParams DIParams;
	DIParams.Parent( mSubTextureCont );
	DIParams.PosSet( SGParams.Pos );
	DIParams.SizeSet( Sizei( Width, 400 ) );
	DIParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mDICont = eeNew( cUIComplexControl, ( DIParams ) );
	mDICont->Enabled( false );
	mDICont->Visible( false );

	Txt = mTheme->CreateTextBox( "DataId String:", mDICont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 0 ), TxtFlags );

	mDataIdInput = mTheme->CreateTextInput( mDICont, Sizei( Width / 4 * 3, 21 ), Vector2i( TAB_CONT_X_DIST + 8, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	FillSGCombo();
}

void MapEditor::CreateLighContainer() {
	cUIPushButton * NewLightBut = mTheme->CreatePushButton( mLightCont, Sizei( mLightCont->Size().Width() - TAB_CONT_X_DIST * 2, 22 ), Vector2i( TAB_CONT_X_DIST, 0 ) );
	NewLightBut->Text( "New Light" );
	NewLightBut->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnNewLight ) );

	cUITextBox * Txt = mTheme->CreateTextBox( "Light Color:", mLightCont, Sizei(), Vector2i( TAB_CONT_X_DIST, 32 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	cUIComplexControl::CreateParams ComParams;
	ComParams.Parent( mLightCont );
	ComParams.PosSet( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 4 );
	ComParams.SizeSet( 58, 64 );
	ComParams.Background.Color( ColorA(255,255,255,255) );
	ComParams.Border.Color( ColorA( 100, 100, 100, 200 ) );
	ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
	mUIBaseColor = eeNew( cUIComplexControl, ( ComParams ) );
	mUIBaseColor->Visible( true );
	mUIBaseColor->Enabled( true );

	Txt = mTheme->CreateTextBox( "R:", mLightCont, Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIBaseColor->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIRedSlider = mTheme->CreateSlider( mLightCont, Sizei( 100, 20 ), Vector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIRedSlider->MaxValue( 255 );
	mUIRedSlider->Value( 255 );
	mUIRedSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnRedChange ) );

	mUIRedTxt = mTheme->CreateTextBox( String::ToStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIRedSlider->Pos().x + mUIRedSlider->Size().Width() + 4, mUIRedSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "G:", mLightCont, Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIRedSlider->Pos().y + mUIRedSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIGreenSlider = mTheme->CreateSlider( mLightCont, Sizei( 100, 20 ), Vector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIGreenSlider->MaxValue( 255 );
	mUIGreenSlider->Value( 255 );
	mUIGreenSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnGreenChange ) );

	mUIGreenTxt = mTheme->CreateTextBox( String::ToStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIGreenSlider->Pos().x + mUIGreenSlider->Size().Width() + 4, mUIGreenSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "B:", mLightCont, Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIGreenSlider->Pos().y + mUIGreenSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIBlueSlider = mTheme->CreateSlider( mLightCont, Sizei( 100, 20 ), Vector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIBlueSlider->MaxValue( 255 );
	mUIBlueSlider->Value( 255 );
	mUIBlueSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnBlueChange ) );

	mUIBlueTxt = mTheme->CreateTextBox( String::ToStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIBlueSlider->Pos().x + mUIBlueSlider->Size().Width() + 4, mUIBlueSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "Light Radius:", mLightCont, Sizei(), Vector2i( TAB_CONT_X_DIST, mUIBlueTxt->Pos().y + mUIBlueTxt->Size().Height() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mLightRadius = mTheme->CreateSpinBox( mLightCont, Sizei( 100, 22 ), Vector2i( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, 100, false );
	mLightRadius->MaxValue( 2000 );
	mLightRadius->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnLightRadiusChangeVal ) );

	mLightTypeChk = mTheme->CreateCheckBox( mLightCont, Sizei(), Vector2i( mLightRadius->Pos().x, mLightRadius->Pos().y + mLightRadius->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mLightTypeChk->Text( "Isometric Light" );
	mLightTypeChk->Active( false );
	mLightTypeChk->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnLightTypeChange ) );
}

cUISelectButton * MapEditor::AddObjContButton( String text, Uint32 mode ) {
	cUISelectButton * Button = mTheme->CreateSelectButton( mObjectCont, Sizei( mObjectCont->Size().Width() - TAB_CONT_X_DIST * 2, 22 ), Vector2i( TAB_CONT_X_DIST, mLastSelButtonY ) );

	Button->Text( text );
	Button->Data( mode );

	Button->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnObjectModeSel ) );

	mLastSelButtonY += Button->Size().Height() + 4;

	mObjContButton.push_back( Button );

	return Button;
}

void MapEditor::CreateObjectsContainer() {
	AddObjContButton( "Select Objects", UIMap::SELECT_OBJECTS )->Select();
	AddObjContButton( "Edit Polygons", UIMap::EDIT_POLYGONS );
	AddObjContButton( "Insert Object", UIMap::INSERT_OBJECT );
	AddObjContButton( "Insert Polygon", UIMap::INSERT_POLYGON );
	cUISelectButton * Button = AddObjContButton( "Insert Polyline", UIMap::INSERT_POLYLINE );

	Int32 nextY = Button->Pos().y + Button->Size().Height() + 4;

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkClampToTile = mTheme->CreateCheckBox( mObjectCont, Sizei(), Vector2i( 12, nextY ), ChkFlags );
	mChkClampToTile->Text( "Clamp Position to Tile" );
	mChkClampToTile->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::ChkClickClampToTile ) );
	mChkClampToTile->Active( true );
}

void MapEditor::OnObjectModeSel( const cUIEvent * Event ) {
	cUISelectButton * Button = static_cast<cUISelectButton*>( Event->Ctrl() );
	cUISelectButton * ButtonT = NULL;

	for ( std::list<cUISelectButton*>::iterator it = mObjContButton.begin(); it != mObjContButton.end(); it++ ) {
		ButtonT = *it;

		ButtonT->Unselect();
	}

	Button->Select();

	mUIMap->EditingObjMode( (UIMap::EDITING_OBJ_MODE)Button->Data() );
}

void MapEditor::CreateUIMap() {
	cUISkin * HScrollSkin = mTheme->GetByName( mTheme->Abbr() + "_" + "hscrollbar_bg" );
	cUISkin * VScrollSkin = mTheme->GetByName( mTheme->Abbr() + "_" + "vscrollbar_bg" );

	Float ScrollH = 16;
	Float ScrollV = 16;

	if ( NULL != HScrollSkin ) {
		SubTexture * tTex = HScrollSkin->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != tTex ) {
			ScrollH = tTex->Size().Height();
		}
	}

	if ( NULL != VScrollSkin ) {
		SubTexture * tTex = VScrollSkin->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != tTex ) {
			ScrollV = tTex->Size().Height();
		}
	}

	cUIComplexControl::CreateParams Params;
	Params.Parent( mWinContainer );
	Params.PosSet( 0, 0 );
	Params.SizeSet( mWinContainer->Size().Width() - 225 - ScrollV, mWinContainer->Size().Height() - ScrollH );

	Params.Flags |= UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT;
	mUIMap = eeNew( UIMap, ( Params, mTheme ) );
	mUIMap->Visible( true );
	mUIMap->Enabled( true );
	CreateNewEmptyMap();
	mUIMap->AddEventListener( cUIEvent::EventOnSizeChange, cb::Make1( this, &MapEditor::OnMapSizeChange ) );
	mUIMap->AddEventListener( cUIEvent::EventMouseDown, cb::Make1( this, &MapEditor::OnMapMouseDown ) );
	mUIMap->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapEditor::OnMapMouseClick ) );
	mUIMap->SetLightSelectCb( cb::Make1( this, &MapEditor::OnLightSelect ) );
	mUIMap->SetLightRadiusChangeCb( cb::Make1( this, &MapEditor::OnLightRadiusChange ) );
	mUIMap->SetAddObjectCallback( cb::Make2( this, &MapEditor::OnAddObject ) );
	mUIMap->SetAlertCb( cb::Make2( this, &MapEditor::CreateAlert ) );
	mUIMap->SetUpdateScrollCb( cb::Make0( this, &MapEditor::UpdateScroll ) );
	mUIMap->SetTileBox( mTileBox );

	mMapHScroll = mTheme->CreateScrollBar( mWinContainer, Sizei( Params.Size.Width(), ScrollH ), Vector2i( 0, mWinContainer->Size().Height() - ScrollH ), UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM | UI_AUTO_SIZE );
	mMapHScroll->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnScrollMapH ) );

	mMapVScroll = mTheme->CreateScrollBar( mWinContainer, Sizei( ScrollV, Params.Size.Height() ), Vector2i( Params.Size.Width() + ScrollV, 0 ), UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_AUTO_SIZE , true );
	mMapVScroll->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &MapEditor::OnScrollMapV ) );

	MapCreated();
}

void MapEditor::OnAddObject( Uint32 Type, Polygon2f poly ) {
	if ( NULL == mCurLayer ) {
		CreateNoLayerAlert( "No layers found" )->SetFocus();
		return;
	}

	if ( mCurLayer->Type() != MAP_LAYER_OBJECT ) {
		CreateAlert( "Wrong Layer", "Objects only can be added to an Object Layer" )->SetFocus();
		return;
	}

	if ( poly.Size() < 3 ) {
		return;
	}

	MapObjectLayer * OL = static_cast<MapObjectLayer*> ( mCurLayer );

	if ( GAMEOBJECT_TYPE_OBJECT == Type ) {
		OL->AddGameObject( eeNew( GameObjectObject, ( mUIMap->Map()->GetNewObjectId(), poly.ToAABB(), mCurLayer ) ) );
	} else if ( GAMEOBJECT_TYPE_POLYGON == Type ) {
		OL->AddGameObject( eeNew( GameObjectPolygon, ( mUIMap->Map()->GetNewObjectId(), poly, mCurLayer ) ) );
	} else if ( GAMEOBJECT_TYPE_POLYLINE == Type ) {
		OL->AddGameObject( eeNew( GameObjectPolyline, ( mUIMap->Map()->GetNewObjectId(), poly, mCurLayer ) ) );
	}
}

void MapEditor::OnLightTypeChange( const cUIEvent * Event ) {
	if ( NULL != mUIMap->GetSelectedLight() ) {
		mUIMap->GetSelectedLight()->Type( mLightTypeChk->Active() ? LIGHT_ISOMETRIC : LIGHT_NORMAL );
	}
}

void MapEditor::OnLightRadiusChangeVal( const cUIEvent * Event ) {
	if ( NULL != mUIMap->GetSelectedLight() ) {
		mUIMap->GetSelectedLight()->Radius( mLightRadius->Value() );
	}
}

void MapEditor::OnLightRadiusChange( MapLight * Light ) {
	mLightRadius->Value( Light->Radius() );
}

void MapEditor::OnLightSelect( MapLight * Light ) {
	ColorA Col( Light->Color() );

	mUIRedSlider->Value( Col.R() );
	mUIGreenSlider->Value( Col.G() );
	mUIBlueSlider->Value( Col.B() );
	mLightRadius->Value( Light->Radius() );
	mLightTypeChk->Active( Light->Type() == LIGHT_ISOMETRIC ? true : false );
}

void MapEditor::OnNewLight( const cUIEvent * Event ) {
	const cUIEventMouse * MEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MEvent->Flags() & EE_BUTTON_LMASK ) {
		Vector2i Pos = mUIMap->Map()->GetMouseMapPos();
		mUIMap->AddLight( eeNew( MapLight, ( mLightRadius->Value(), Pos.x, Pos.y, mUIBaseColor->Background()->Color().ToColor(), mLightTypeChk->Active() ? LIGHT_ISOMETRIC : LIGHT_NORMAL ) ) );
	}
}

void MapEditor::OnRedChange( const cUIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Red = (Uint8)mUIRedSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIRedTxt->Text( String::ToStr( (Int32)mUIRedSlider->Value() ) );

	if ( NULL != mUIMap->GetSelectedLight() ) {
		RGB lCol( mUIMap->GetSelectedLight()->Color() );
		lCol.Red = Col.R();
		mUIMap->GetSelectedLight()->Color( lCol );
	}
}

void MapEditor::OnGreenChange( const cUIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Green = (Uint8)mUIGreenSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIGreenTxt->Text( String::ToStr( (Uint32)mUIGreenSlider->Value() ) );

	if ( NULL != mUIMap->GetSelectedLight() ) {
		RGB lCol( mUIMap->GetSelectedLight()->Color() );
		lCol.Green = Col.G();
		mUIMap->GetSelectedLight()->Color( lCol );
	}
}

void MapEditor::OnBlueChange( const cUIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Blue = (Uint8)mUIBlueSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIBlueTxt->Text( String::ToStr( (Uint32)mUIBlueSlider->Value() ) );

	if ( NULL != mUIMap->GetSelectedLight() ) {
		RGB lCol( mUIMap->GetSelectedLight()->Color() );
		lCol.Blue = Col.B();
		mUIMap->GetSelectedLight()->Color( lCol );
	}
}

void MapEditor::ChkClickDI( const cUIEvent * Event ) {
	if ( mChkDI->Active() ) {
		mSGCont->Enabled( false );
		mSGCont->Visible( false );
		mDICont->Enabled( true );
		mDICont->Visible( true );
	} else {
		mSGCont->Enabled( true );
		mSGCont->Visible( true );
		mDICont->Enabled( false );
		mDICont->Visible( false );
	}
}

void MapEditor::ChkClickClampToTile( const cUIEvent * Event ) {
	mUIMap->ClampToTile( mChkClampToTile->Active() );
}

void MapEditor::UpdateGfx() {
	if ( mChkMirrored->Active() && mChkFliped->Active() )
		mGfxPreview->RenderMode( RN_FLIPMIRROR );
	else if( mChkMirrored->Active() )
		mGfxPreview->RenderMode( RN_MIRROR );
	else if ( mChkFliped->Active() )
		mGfxPreview->RenderMode( RN_FLIP );
	else
		mGfxPreview->RenderMode( RN_NORMAL );

	if ( mChkRot90->Active() )
		mGfxPreview->Angle( 90 );
	else
		mGfxPreview->Angle( 0 );
}

void MapEditor::UpdateFlags() {
	mCurGOFlags = 0;

	if ( mChkMirrored->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_MIRRORED;

	if ( mChkFliped->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_FLIPED;

	if ( mChkBlocked->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_BLOCKED;

	if ( mChkAnim->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ANIMATED;

	if ( mChkRot90->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ROTATE_90DEG;

	if ( mChkAutoFix->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_AUTO_FIX_TILE_POS;
}

void MapEditor::OnTypeChange( const cUIEvent * Event ) {
	if ( mGOTypeList->Text() == "SubTexture" )
		mCurGOType = GAMEOBJECT_TYPE_SUBTEXTURE;
	else if ( mGOTypeList->Text() == "SubTextureEx" )
		mCurGOType = GAMEOBJECT_TYPE_SUBTEXTUREEX;
	else if ( mGOTypeList->Text() == "Sprite" )
		mCurGOType = GAMEOBJECT_TYPE_SPRITE;
	else
		mCurGOType = String::Hash( mGOTypeList->Text().ToUtf8() );

	if ( NULL != mChkAnim && NULL != mGOTypeList && mChkAnim->Active() && mGOTypeList->Text() != "Sprite" ) {
		if ( mGOTypeList->Text() == "SubTexture" || mGOTypeList->Text() == "SubTextureEx" ) {
			mChkAnim->Active( false );
		}
	}
}

void MapEditor::ChkClickMirrored( const cUIEvent * Event ) {
	UpdateGfx();
	UpdateFlags();
}

void MapEditor::ChkClickFliped( const cUIEvent * Event ) {
	UpdateGfx();
	UpdateFlags();
}

void MapEditor::ChkClickRot90( const cUIEvent * Event ) {
	UpdateGfx();
	UpdateFlags();
}

void MapEditor::ChkClickBlocked( const cUIEvent * Event ) {
	UpdateFlags();
}

void MapEditor::ChkClickAutoFix( const cUIEvent * Event ) {
	UpdateFlags();
}

void MapEditor::ChkClickAnimated( const cUIEvent * Event ) {
	UpdateFlags();

	if ( mChkAnim->Active() && ( mGOTypeList->Text() == "SubTexture" || mGOTypeList->Text() == "SubTextureEx" ) ) {
		mGOTypeList->ListBox()->SetSelected( "Sprite" );
	}
}

void MapEditor::AddNewGOType( const cUIEvent * Event ) {
	eeNew( UIGOTypeNew, ( cb::Make2( this, &MapEditor::OnNewGOTypeAdded ) ) );
}

void MapEditor::OnNewGOTypeAdded( std::string name, Uint32 hash ) {
	if ( "" != name ) {
		for ( Uint32 i = 0; i < mGOTypeList->ListBox()->Count(); i++ ) {
			cUIListBoxItem * Item = mGOTypeList->ListBox()->GetItem(i);

			if ( Item->Text() == name )
				return;
		}

		mGOTypeList->ListBox()->AddListBoxItem( name );
		mUIMap->Map()->AddVirtualObjectType( name );
	}
}

void MapEditor::FillSGCombo() {
	TextureAtlasManager * SGM = TextureAtlasManager::instance();
	std::list<TextureAtlas*>& Res = SGM->GetResources();

	mTextureAtlasesList->ListBox()->Clear();

	std::vector<String> items;

	Uint32 Restricted1 = String::Hash( std::string( "global" ) );
	Uint32 Restricted2 = String::Hash( mTheme->TextureAtlas()->Name() );

	for ( std::list<TextureAtlas*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
		if ( (*it)->Id() != Restricted1 && (*it)->Id() != Restricted2 )
			items.push_back( (*it)->Name() );
	}

	if ( items.size() ) {
		mTextureAtlasesList->ListBox()->AddListBoxItems( items );
	}

	if ( mTextureAtlasesList->ListBox()->Count() && NULL == mTextureAtlasesList->ListBox()->GetItemSelected() ) {
		mTextureAtlasesList->ListBox()->SetSelected( 0 );
	}
}

void MapEditor::FillSubTextureList() {
	TextureAtlasManager * SGM = TextureAtlasManager::instance();
	mCurSG = SGM->GetByName( mTextureAtlasesList->Text() );
	std::list<SubTexture*>& Res = mCurSG->GetResources();

	mSubTextureList->Clear();

	if ( NULL != mCurSG ) {
		std::vector<String> items;

		for ( std::list<SubTexture*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
				items.push_back( (*it)->Name() );
		}

		if ( items.size() ) {
			std::sort( items.begin(), items.end() );

			mSubTextureList->AddListBoxItems( items );
			mSubTextureList->SetSelected( 0 );
		}
	}

	mSubTextureList->VerticalScrollBar()->ClickStep( 8.f / (Float)mSubTextureList->Count() );
}

void MapEditor::OnSubTextureChange( const cUIEvent * Event ) {
	if ( NULL != mCurSG ) {
		SubTexture * tSubTexture = mCurSG->GetByName( mSubTextureList->GetItemSelectedText() );

		if ( NULL != tSubTexture ) {
			mGfxPreview->SubTexture( tSubTexture );
		}
	}
}

void MapEditor::OnTextureAtlasChange( const cUIEvent * Event ) {
	FillSubTextureList();
}

void MapEditor::CreateNewMap() {
	eeNew( UIMapNew, ( mUIMap, cb::Make0( this, &MapEditor::MapCreated ) ) );
}

void MapEditor::CreateNewEmptyMap() {
	mUIMap->Map()->Create( Sizei( 100, 100 ), 16, Sizei( 32, 32 ), MAP_EDITOR_DEFAULT_FLAGS | MAP_FLAG_LIGHTS_ENABLED, mUIMap->Size() );
}

void MapEditor::MapCreated() {
	mCurLayer = NULL;
	mLayerList->ListBox()->Clear();
	UpdateFlags();
	SetViewOptions();
	FillSGCombo();
	FillGotyList();

	mMapHScroll->Value( 0 );
	mMapVScroll->Value( 0 );
	OnMapSizeChange( NULL );

	mUIMap->ClearLights();

	CreateTabs();
}

void MapEditor::OnMapSizeChange( const cUIEvent *Event ) {
	if ( mMouseScrolling )
		return;

	Vector2i v( mUIMap->Map()->GetMaxOffset() );

	mMapHScroll->MinValue( 0 );
	mMapHScroll->MaxValue( v.x );
	mMapHScroll->ClickStep( mUIMap->Map()->TileSize().Width() * mUIMap->Map()->Scale() );
	mMapVScroll->MinValue( 0 );
	mMapVScroll->MaxValue( v.y );
	mMapVScroll->ClickStep( mUIMap->Map()->TileSize().Height() * mUIMap->Map()->Scale() );
}

void MapEditor::OnScrollMapH( const cUIEvent * Event ) {
	if ( mMouseScrolling )
		return;

	cUIScrollBar * Scr = reinterpret_cast<cUIScrollBar*> ( Event->Ctrl() );

	Vector2f Off = mUIMap->Map()->Offset();

	Off.x = -Scr->Value();

	mUIMap->Map()->Offset( Off ) ;
}

void MapEditor::OnScrollMapV( const cUIEvent * Event ) {
	cUIScrollBar * Scr = reinterpret_cast<cUIScrollBar*> ( Event->Ctrl() );

	Vector2f Off = mUIMap->Map()->Offset();

	Off.y = -Scr->Value();

	mUIMap->Map()->Offset( Off ) ;
}

void MapEditor::UpdateScroll() {
	mMouseScrolling = true;
	mMapHScroll->Value( -mUIMap->Map()->Offset().x );
	mMapVScroll->Value( -mUIMap->Map()->Offset().y );
	mMouseScrolling = false;
}

void MapEditor::MapOpen( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );

	if ( mUIMap->Map()->Load( CDL->GetFullPath() ) ) {
		OnMapLoad();
	}
}

void MapEditor::OnMapLoad() {
	mCurLayer = NULL;

	mUIMap->Map()->ViewSize( mUIMap->Size() );

	MapCreated();

	RefreshLayersList();

	RefreshGotyList();
}

void MapEditor::MapSave( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );

	std::string path( CDL->GetFullPath() );

	if ( path.substr( path.size() - 4 ) != ".eem" ) {
		path += ".eem";
	}

	mUIMap->Map()->Save( path );
}

void MapEditor::FileMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "New..." == txt ) {
		CreateNewMap();
	} else if ( "Open..." == txt ) {
		cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS, "*.eem" );

		TGDialog->Title( "Open Map" );
		TGDialog->AddEventListener( cUIEvent::EventOpenFile, cb::Make1( this, &MapEditor::MapOpen ) );
		TGDialog->Center();
		TGDialog->Show();
	} else if ( "Save As..." == txt ) {
		cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS | CDL_FLAG_SAVE_DIALOG, "*.eem" );

		TGDialog->Title( "Save Map" );
		TGDialog->AddEventListener( cUIEvent::EventSaveFile, cb::Make1( this, &MapEditor::MapSave ) );
		TGDialog->Center();
		TGDialog->Show();
	} else if ( "Save" == txt ) {
		if ( mUIMap->Map()->Path().size() ) {
			mUIMap->Map()->Save( mUIMap->Map()->Path() );
		}
	} else if ( "Close" == txt ) {
		cUIMessageBox * MsgBox = mTheme->CreateMessageBox( MSGBOX_OKCANCEL, "Do you really want to close the current map?\nAll changes will be lost." );
		MsgBox->AddEventListener( cUIEvent::EventMsgBoxConfirmClick, cb::Make1( this, &MapEditor::OnMapClose ) );
		MsgBox->Title( "Close Map?" );
		MsgBox->Center();
		MsgBox->Show();
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == cUIManager::instance()->MainControl() ) {
			cUIManager::instance()->GetWindow()->Close();
		} else {
			mUIWindow->CloseWindow();
		}
	}
}

void MapEditor::OnMapClose( const cUIEvent * Event ) {
	CreateNewEmptyMap();

	MapCreated();

	mCurLayer = NULL;

	RefreshLayersList();
}

void MapEditor::ViewMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "Show Grid" == txt ) {
		mUIMap->Map()->DrawGrid( reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() )->Active() );
	} else if ( "Mark Tile Over" == txt ) {
		mUIMap->Map()->DrawTileOver( reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() )->Active() );
	} else if ( "Show Blocked" == txt ) {
		mUIMap->Map()->ShowBlocked( reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() )->Active() );
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

void MapEditor::MapMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "New Texture Atlas..." == txt ) {
		cUIWindow * tWin = mTheme->CreateWindow( NULL, Sizei( 1024, 768 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, Sizei( 1024, 768 ) );
		eeNew ( Tools::TextureAtlasEditor, ( tWin ) );
		tWin->Center();
		tWin->Show();
	} else if ( "Add External Texture Atlas..." == txt ) {
		cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );

		TGDialog->Title( "Load Texture Atlas..." );
		TGDialog->AddEventListener( cUIEvent::EventOpenFile, cb::Make1( this, &MapEditor::TextureAtlasOpen ) );
		TGDialog->Center();
		TGDialog->Show();
	} else if ( "Properties..." == txt ) {
		eeNew( TileMapProperties, ( mUIMap->Map() ) );
	} else if ( "Resize..." ) {
		eeNew( UIMapNew, ( mUIMap, cb::Make0( this, &MapEditor::OnMapLoad ), true ) );
	}
}

void MapEditor::LayerMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

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

cUIMessageBox * MapEditor::CreateAlert( const String& title, const String& text ) {
	cUIMessageBox * MsgBox = mTheme->CreateMessageBox( MSGBOX_OK, text );
	MsgBox->Title( title );
	MsgBox->Center();
	MsgBox->Show();
	return MsgBox;
}

cUIMessageBox * MapEditor::CreateNoLayerAlert( const String title ) {
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
		mGOTypeList->ListBox()->AddListBoxItem( (*it) );
	}
}

void MapEditor::RefreshLayersList() {
	mLayerList->ListBox()->Clear();

	if ( mUIMap->Map()->LayerCount() ) {
		std::vector<String> layers;

		for ( Uint32 i = 0; i < mUIMap->Map()->LayerCount(); i++ ) {
			layers.push_back( mUIMap->Map()->GetLayer(i)->Name() );
		}

		mLayerList->ListBox()->AddListBoxItems( layers );
	}

	if ( NULL != mCurLayer ) {
		mLayerList->ListBox()->SetSelected( mCurLayer->Name() );
	} else {
		if ( mUIMap->Map()->LayerCount() ) {
			mLayerList->ListBox()->SetSelected(0);
		}
	}
}

void MapEditor::TextureAtlasOpen( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );

	std::string sgname = FileSystem::FileRemoveExtension( FileSystem::FileNameFromPath( CDL->GetFullPath() ) );

	TextureAtlas * SG = TextureAtlasManager::instance()->GetByName( sgname );

	if ( NULL == SG ) {
		TextureAtlasLoader tgl( CDL->GetFullPath() );

		if ( tgl.IsLoaded() ) {
			mTextureAtlasesList->ListBox()->AddListBoxItem( sgname );
		}
	} else {
		if ( eeINDEX_NOT_FOUND == mTextureAtlasesList->ListBox()->GetItemIndex( sgname ) ) {
			mTextureAtlasesList->ListBox()->AddListBoxItem( sgname );
		}
	}
}

void MapEditor::OnLayerAdd( UIMapLayerNew * UILayer ) {
	bool SetSelected = ( 0 == mLayerList->ListBox()->Count() ) ? true : false;

	mLayerList->ListBox()->AddListBoxItem( UILayer->Name() );

	if ( SetSelected ) {
		mCurLayer = UILayer->Layer();

		mUIMap->CurLayer( mCurLayer );

		mLayerList->ListBox()->SetSelected(0);
	}
}

void MapEditor::OnLayerSelect( const cUIEvent * Event ) {
	MapLayer * tLayer = mUIMap->Map()->GetLayer( mLayerList->Text() );

	if ( NULL != tLayer ) {
		mCurLayer = tLayer;

		mUIMap->CurLayer( mCurLayer );

		mLayerChkVisible->Active( mCurLayer->Visible() );

		mLayerChkLights->Active( mCurLayer->LightsEnabled() );
	}
}

void MapEditor::WindowClose( const cUIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

GameObject * MapEditor::CreateGameObject() {
	GameObject * tObj	= NULL;

	if ( GAMEOBJECT_TYPE_SUBTEXTURE == mCurGOType ) {

		tObj = eeNew( GameObjectSubTexture, ( mCurGOFlags, mCurLayer, mGfxPreview->SubTexture() ) );

	} else if ( GAMEOBJECT_TYPE_SUBTEXTUREEX == mCurGOType ) {

		tObj = eeNew( GameObjectSubTextureEx, ( mCurGOFlags, mCurLayer, mGfxPreview->SubTexture() ) );

	} else if ( GAMEOBJECT_TYPE_SPRITE == mCurGOType ) {

		if ( mChkAnim->Active() ) {

			Sprite * tAnimSprite = eeNew( Sprite, ( String::RemoveNumbersAtEnd( mGfxPreview->SubTexture()->Name() ) ) );
			tObj = eeNew( GameObjectSprite, ( mCurGOFlags, mCurLayer, tAnimSprite ) );

		} else {

			Sprite * tStatiSprite = eeNew( Sprite, ( mGfxPreview->SubTexture() ) );
			tObj = eeNew( GameObjectSprite, ( mCurGOFlags, mCurLayer, tStatiSprite ) );

		}
	} else {
		//! Creates an empty game object. The client will interpret the GameObject Type, and instanciate the corresponding class.

		if ( mChkDI->Active() )
			tObj = eeNew( GameObjectVirtual, ( String::Hash( mDataIdInput->Text().ToUtf8() ), mCurLayer, mCurGOFlags, mCurGOType ) );
		else
			tObj = eeNew( GameObjectVirtual, ( mGfxPreview->SubTexture(), mCurLayer, mCurGOFlags, mCurGOType ) );
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

		if ( cUIManager::instance()->GetInput()->IsKeyDown( KEY_LCTRL ) ) {
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

void MapEditor::OnMapMouseClick( const cUIEvent * Event ) {
	const cUIEventMouse * MEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( mSubTextureCont->Visible() ) {
		if ( NULL == mCurLayer || NULL == mGfxPreview->SubTexture() || cUIManager::instance()->DownControl() != mUIMap ) {
			if ( NULL == mCurLayer )
				CreateNoLayerAlert( "No layers found" );

			return;
		}

		if ( MEvent->Flags() & EE_BUTTON_LMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_OBJECT )
				AddGameObject();
		} else if ( MEvent->Flags() & EE_BUTTON_RMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_OBJECT )
				RemoveGameObject();
		} else if ( MEvent->Flags() & EE_BUTTON_MMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->Blocked( !tObj->Blocked() );
				}
			}
		} else if ( MEvent->Flags() & EE_BUTTON_WUMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->Mirrored( !tObj->Mirrored() );
				}
			}
		} else if ( MEvent->Flags() & EE_BUTTON_WDMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED ) {
				GameObject * tObj = GetCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->Rotated( !tObj->Rotated() );
				}
			}
		}
	}
}

void MapEditor::OnMapMouseDown( const cUIEvent * Event ) {
	const cUIEventMouse * MEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( mSubTextureCont->Visible() ) {
		if ( NULL == mCurLayer || NULL == mGfxPreview->SubTexture() || cUIManager::instance()->DownControl() != mUIMap )
			return;


		if ( MEvent->Flags() & EE_BUTTON_LMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED )
				AddGameObjectToTile();
		} else if ( MEvent->Flags() & EE_BUTTON_RMASK ) {
			if ( mCurLayer->Type() == MAP_LAYER_TILED )
				RemoveGameObjectFromTile();
		}
	}
}

void MapEditor::SetViewOptions() {
	mChkShowGrid->Active( mUIMap->Map()->DrawGrid() ? true : false );
	mChkMarkTileOver->Active( mUIMap->Map()->DrawTileOver() ? true : false  );
	mChkShowBlocked->Active( mUIMap->Map()->ShowBlocked() ? true : false );
}

}}
