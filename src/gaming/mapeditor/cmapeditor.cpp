#include "cmapeditor.hpp"
#include "cuimapnew.hpp"
#include "cuilayernew.hpp"
#include "cuigotypenew.hpp"
#include "../ctilelayer.hpp"
#include "../cobjectlayer.hpp"
#include "../cgameobjectvirtual.hpp"
#include "../cgameobjectshape.hpp"
#include "../cgameobjectshapeex.hpp"
#include "../cgameobjectsprite.hpp"
#include "../../ui/cuimanager.hpp"
#include "../../ui/cuithememanager.hpp"
#include "../../ui/cuiwinmenu.hpp"
#include "../../ui/cuipopupmenu.hpp"
#include "../../ui/cuispinbox.hpp"
#include "../../ui/cuicheckbox.hpp"
#include "../../ui/cuicommondialog.hpp"
#include "../../graphics/cshapegroupmanager.hpp"
#include "../../graphics/cglobalshapegroup.hpp"
#include "../../graphics/ctexturegrouploader.hpp"

using namespace EE::Graphics;

namespace EE { namespace Gaming { namespace MapEditor {

cMapEditor::cMapEditor( cUIWindow * AttatchTo, const MapEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mCloseCb( callback ),
	mGOTypeList( NULL ),
	mChkAnim( NULL ),
	mCurLayer( NULL )
{
	if ( NULL == cUIThemeManager::instance()->DefaultTheme() ) {
		eePRINT( "cMapEditor needs a default theme seted to work." );
		return;
	}

	mTheme = cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUIWindow )
		mUIWindow = cUIManager::instance()->MainControl();



	if ( cUIManager::instance()->MainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->Container();
	}

	mUIWindow->Title( "Map Editor" );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cMapEditor::WindowClose ) );

	CreateME();
}

cMapEditor::~cMapEditor() {
}

void cMapEditor::CreateME() {
	CreateWinMenu();

	CreateETGMenu();

	CreateUIMap();
}

void cMapEditor::CreateWinMenu() {
	cUIWinMenu * WinMenu = mTheme->CreateWinMenu( mUIContainer );

	cUIPopUpMenu * PU1 = mTheme->CreatePopUpMenu();
	PU1->Add( "New...", mTheme->GetIconByName( "document-new" ) );
	PU1->Add( "Open...", mTheme->GetIconByName( "document-open" ) );
	PU1->AddSeparator();
	PU1->Add( "Save", mTheme->GetIconByName( "document-save" ) );
	PU1->Add( "Save As...", mTheme->GetIconByName( "document-save-as" ) );
	PU1->AddSeparator();
	PU1->Add( "Close", mTheme->GetIconByName( "document-close" ) );
	PU1->AddSeparator();
	PU1->Add( "Quit", mTheme->GetIconByName( "quit" ) );

	PU1->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::FileMenuClick ) );
	WinMenu->AddMenuButton( "File", PU1 );

	cUIPopUpMenu * PU2 = mTheme->CreatePopUpMenu();
	PU2->Add( "Preferences..." );

	PU2->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::EditMenuClick ) );
	WinMenu->AddMenuButton( "Edit", PU2 );

	cUIPopUpMenu * PU3 = mTheme->CreatePopUpMenu();
	PU3->AddCheckBox( "Show Grid" );

	reinterpret_cast<cUIMenuCheckBox*> ( PU3->GetItem( "Show Grid" ) )->Active( true );

	PU3->AddCheckBox( "Mark Tile Over" );

	PU3->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::ViewMenuClick ) );
	WinMenu->AddMenuButton( "View", PU3 );

	cUIPopUpMenu * PU4 = mTheme->CreatePopUpMenu();
	PU4->Add( "New Texture Group..." );
	PU4->Add( "Add External Texture Group..." );
	PU4->AddSeparator();
	PU4->Add( "Map Properties..." );

	PU4->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::MapMenuClick ) );
	WinMenu->AddMenuButton( "Map", PU4 );

	cUIPopUpMenu * PU5 = mTheme->CreatePopUpMenu();
	PU5->Add( "Add Tile Layer..." );
	PU5->Add( "Add Object Layer..." );
	PU5->AddSeparator();
	PU5->Add( "Remove Layer" );
	PU5->AddSeparator();
	PU5->Add( "Move Layer Up" );
	PU5->Add( "Move Layer Down" );
	PU5->AddSeparator();
	PU5->Add( "Layer Properties..." );

	PU5->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::LayerMenuClick ) );
	WinMenu->AddMenuButton( "Layer", PU5 );

	cUIComplexControl::CreateParams Params;
	Params.Parent( mUIContainer );
	Params.PosSet( 0, WinMenu->Size().Height() );
	Params.SizeSet( mUIContainer->Size().Width(), mUIContainer->Size().Height() - WinMenu->Size().Height() );
	Params.Flags = UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_REPORT_SIZE_CHANGE_TO_CHILDS;
	mWinContainer = eeNew( cUIComplexControl, ( Params ) );
	mWinContainer->Visible( true );
	mWinContainer->Enabled( true );
}

void cMapEditor::CreateETGMenu() {
	cUITextBox * Txt;
	Uint32 TxtFlags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_DRAW_SHADOW;

	Int32 Width = 200;
	Int32 DistToBorder = 5;

	cUIComplexControl::CreateParams CParams;
	CParams.Parent( mWinContainer );
	CParams.PosSet( eeVector2i( mWinContainer->Size().Width() - Width - DistToBorder, 0 ) );
	CParams.SizeSet( eeSize( Width + DistToBorder, mWinContainer->Size().Height() ) );
	CParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mRPCont = eeNew( cUIComplexControl, ( CParams ) );
	mRPCont->Enabled( true );
	mRPCont->Visible( true );

	Txt = mTheme->CreateTextBox( mRPCont, eeSize( Width, 16 ), eeVector2i( 0, 4 ), TxtFlags );
	Txt->Text( "Add Game Object as..." );

	mGOTypeList = mTheme->CreateDropDownList( mRPCont, eeSize( Width - 26, 21 ), eeVector2i( 0, Txt->Pos().y + Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	std::vector<String> items;
	items.push_back( "Shape" );
	items.push_back( "ShapeEx" );
	items.push_back( "Sprite" );
	mGOTypeList->ListBox()->AddListBoxItems( items );
	mGOTypeList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &cMapEditor::OnTypeChange ) );
	mGOTypeList->ListBox()->SetSelected(0);

	mBtnGOTypeAdd = mTheme->CreatePushButton( mRPCont, eeSize( 24, 21 ), eeVector2i( mGOTypeList->Pos().x + mGOTypeList->Size().Width() + 2, mGOTypeList->Pos().y ), UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mTheme->GetIconByName( "add" ) );
	mBtnGOTypeAdd->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapEditor::AddNewGOType ) );

	if ( NULL == mBtnGOTypeAdd->Icon()->Shape() )
		mBtnGOTypeAdd->Text( "..." );

	Txt = mTheme->CreateTextBox( mRPCont, eeSize( Width, 16 ), eeVector2i( 0, mGOTypeList->Pos().y + mGOTypeList->Size().Height() + 4 ), TxtFlags );
	Txt->Text( "Layers:" );

	mLayerList = mTheme->CreateDropDownList( mRPCont, eeSize( Width, 21 ), eeVector2i( 0, Txt->Pos().y + Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mLayerList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &cMapEditor::OnLayerSelect ) );

	Txt = mTheme->CreateTextBox( mRPCont, eeSize( Width, 16 ), eeVector2i( 0, mLayerList->Pos().y + mLayerList->Size().Height() + 4 ), TxtFlags );
	Txt->Text( "Game Object Flags:" );

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkMirrored = mTheme->CreateCheckBox( mRPCont, eeSize(), eeVector2i( 0, Txt->Pos().y + Txt->Size().Height() + 4 ), ChkFlags );
	mChkMirrored->Text( "Mirrored" );
	mChkMirrored->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapEditor::ChkClickMirrored ) );

	mChkFliped = mTheme->CreateCheckBox( mRPCont, eeSize(), eeVector2i( mChkMirrored->Pos().x + mChkMirrored->Size().Width() + 32, mChkMirrored->Pos().y ), ChkFlags );
	mChkFliped->Text( "Fliped" );
	mChkFliped->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapEditor::ChkClickFliped ) );

	mChkBlocked = mTheme->CreateCheckBox( mRPCont, eeSize(), eeVector2i( mChkMirrored->Pos().x, mChkMirrored->Pos().y + mChkMirrored->Size().Height() + 4 ), ChkFlags );
	mChkBlocked->Text( "Blocked" );
	mChkBlocked->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapEditor::ChkClickBlocked ) );

	mChkAnim = mTheme->CreateCheckBox( mRPCont, eeSize(), eeVector2i( mChkFliped->Pos().x, mChkFliped->Pos().y + mChkFliped->Size().Height() + 4 ), ChkFlags );
	mChkAnim->Text( "Animated" );
	mChkAnim->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapEditor::ChkClickAnimated ) );

	Txt = mTheme->CreateTextBox( mRPCont, eeSize( Width, 16 ), eeVector2i( 0, mChkBlocked->Pos().y + mChkBlocked->Size().Height() + 8 ), TxtFlags );
	Txt->Text( "Game Object Data:" );

	mChkDI = mTheme->CreateCheckBox( mRPCont, eeSize(), eeVector2i( 0, Txt->Pos().y + Txt->Size().Height() + 4 ), ChkFlags );
	mChkDI->Text( "Add as DataId" );
	mChkDI->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapEditor::ChkClickDI ) );

	cUIComplexControl::CreateParams SGParams;
	SGParams.Parent( mRPCont );
	SGParams.PosSet( eeVector2i( 0, mChkDI->Pos().y + mChkDI->Size().Height() + 8 ) );
	SGParams.SizeSet( eeSize( Width, 400 ) );
	SGParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mSGCont = eeNew( cUIComplexControl, ( SGParams ) );
	mSGCont->Enabled( true );
	mSGCont->Visible( true );

	Txt = mTheme->CreateTextBox( mSGCont, eeSize( Width, 16 ), eeVector2i( 0, 0 ), TxtFlags );
	Txt->Text( "Shape Groups:" );

	mShapeGroupsList = mTheme->CreateDropDownList( mSGCont, eeSize( Width, 21 ), eeVector2i( 0, Txt->Pos().y +Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mShapeGroupsList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &cMapEditor::OnShapeGroupChange ) );

	mShapeList = mTheme->CreateListBox( mSGCont, eeSize( Width, 156 ), eeVector2i( 0, mShapeGroupsList->Pos().y + mShapeGroupsList->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mShapeList->Size( mShapeList->Size().Width(), mShapeList->RowHeight() * 9 + mShapeList->PaddingContainer().Top + mShapeList->PaddingContainer().Bottom );
	mShapeList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &cMapEditor::OnShapeChange ) );

	mGfxPreview = mTheme->CreateGfx( NULL, mSGCont, eeSize( Width, Width ), eeVector2i( 0, mShapeList->Pos().y + mShapeList->Size().Height() + 4 ), UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_AUTO_FIT );
	mGfxPreview->Border( true );

	cUIComplexControl::CreateParams DIParams;
	DIParams.Parent( mRPCont );
	DIParams.PosSet( SGParams.Pos );
	DIParams.SizeSet( eeSize( Width, 400 ) );
	DIParams.Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;
	mDICont = eeNew( cUIComplexControl, ( DIParams ) );
	mDICont->Enabled( false );
	mDICont->Visible( false );

	Txt = mTheme->CreateTextBox( mDICont, eeSize( Width, 16 ), eeVector2i( 0, 0 ), TxtFlags );
	Txt->Text( "DataId String:" );

	mDataIdInput = mTheme->CreateTextInput( mDICont, eeSize( Width / 4 * 3, 21 ), eeVector2i( 8, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

	FillSGCombo();
}

void cMapEditor::ChkClickDI( const cUIEvent * Event ) {
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

void cMapEditor::UpdateGfx() {
	if ( mChkMirrored->Active() && mChkFliped->Active() )
		mGfxPreview->RenderType( RN_FLIPMIRROR );
	else if( mChkMirrored->Active() )
		mGfxPreview->RenderType( RN_MIRROR );
	else if ( mChkFliped->Active() )
		mGfxPreview->RenderType( RN_FLIP );
	else
		mGfxPreview->RenderType( RN_NORMAL );
}

void cMapEditor::UpdateFlags() {
	mCurGOFlags = 0;

	if ( mChkMirrored->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_MIRRORED;

	if ( mChkFliped->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_FLIPED;

	if ( mChkBlocked->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_BLOCKED;

	if ( mChkAnim->Active() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ANIMATED;
}

void cMapEditor::OnTypeChange( const cUIEvent * Event ) {
	if ( mGOTypeList->Text() == "Shape" )
		mCurGOType = GAMEOBJECT_TYPE_SHAPE;
	else if ( mGOTypeList->Text() == "ShapeEx" )
		mCurGOType = GAMEOBJECT_TYPE_SHAPEEX;
	else if ( mGOTypeList->Text() == "Sprite" )
		mCurGOType = GAMEOBJECT_TYPE_SPRITE;
	else
		mCurGOType = MakeHash( mGOTypeList->Text().ToUtf8() );

	if ( NULL != mChkAnim && NULL != mGOTypeList && mChkAnim->Active() && mGOTypeList->Text() != "Sprite" ) {
		if ( mGOTypeList->Text() == "Shape" || mGOTypeList->Text() == "ShapeEx" ) {
			mChkAnim->Active( false );
		}
	}
}

void cMapEditor::ChkClickMirrored( const cUIEvent * Event ) {
	UpdateGfx();
	UpdateFlags();
}

void cMapEditor::ChkClickFliped( const cUIEvent * Event ) {
	UpdateGfx();
	UpdateFlags();
}

void cMapEditor::ChkClickBlocked( const cUIEvent * Event ) {
	UpdateFlags();
}

void cMapEditor::ChkClickAnimated( const cUIEvent * Event ) {
	UpdateFlags();

	if ( mChkAnim->Active() && ( mGOTypeList->Text() == "Shape" || mGOTypeList->Text() == "ShapeEx" ) ) {
		mGOTypeList->ListBox()->SetSelected( "Sprite" );
	}
}

void cMapEditor::AddNewGOType( const cUIEvent * Event ) {
	eeNew( cUIGOTypeNew, ( cb::Make2( this, &cMapEditor::OnNewGOTypeAdded ) ) );
}

void cMapEditor::OnNewGOTypeAdded( std::string name, Uint32 hash ) {
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

void cMapEditor::FillSGCombo() {
	cShapeGroupManager * SGM = cShapeGroupManager::instance();
	std::list<cShapeGroup*>& Res = SGM->GetResources();

	mShapeGroupsList->ListBox()->Clear();

	std::vector<String> items;

	Uint32 Restricted1 = MakeHash( std::string( "global" ) );
	Uint32 Restricted2 = MakeHash( mTheme->ShapeGroup()->Name() );

	for ( std::list<cShapeGroup*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
		if ( (*it)->Id() != Restricted1 && (*it)->Id() != Restricted2 )
			items.push_back( (*it)->Name() );
	}

	if ( items.size() ) {
		mShapeGroupsList->ListBox()->AddListBoxItems( items );
	}

	if ( mShapeGroupsList->ListBox()->Count() && NULL == mShapeGroupsList->ListBox()->GetItemSelected() ) {
		mShapeGroupsList->ListBox()->SetSelected( 0 );
	}
}

void cMapEditor::FillShapeList() {
	cShapeGroupManager * SGM = cShapeGroupManager::instance();
	mCurSG = SGM->GetByName( mShapeGroupsList->Text() );
	std::list<cShape*>& Res = mCurSG->GetResources();

	mShapeList->Clear();

	if ( NULL != mCurSG ) {
		std::vector<String> items;

		for ( std::list<cShape*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
				items.push_back( (*it)->Name() );
		}

		if ( items.size() ) {
			std::sort( items.begin(), items.end() );

			mShapeList->AddListBoxItems( items );
			mShapeList->SetSelected( 0 );
		}
	}

	mShapeList->VerticalScrollBar()->ClickStep( 8.f / (eeFloat)mShapeList->Count() );
}

void cMapEditor::OnShapeChange( const cUIEvent * Event ) {
	if ( NULL != mCurSG ) {
		cShape * tShape = mCurSG->GetByName( mShapeList->GetItemSelectedText() );

		if ( NULL != tShape ) {
			mGfxPreview->Shape( tShape );
		}
	}
}

void cMapEditor::OnShapeGroupChange( const cUIEvent * Event ) {
	FillShapeList();
}

void cMapEditor::CreateNewMap() {
	eeNew( cUIMapNew, ( mUIMap, cb::Make0( this, &cMapEditor::MapCreated ) ) );
}

void cMapEditor::MapCreated() {
	mLayerList->ListBox()->Clear();
	FillSGCombo();

	mMapHScroll->Value( 0 );
	mMapVScroll->Value( 0 );
	OnMapSizeChange( NULL );
}

void cMapEditor::CreateUIMap() {
	cUIComplexControl::CreateParams Params;
	Params.Parent( mWinContainer );
	Params.PosSet( 0, 0 );
	Params.SizeSet( 800, 600 );
	Params.Flags |= UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT;
	mUIMap = eeNew( cUIMap, ( Params ) );
	mUIMap->Visible( true );
	mUIMap->Enabled( true );
	mUIMap->Map()->Create( eeSize( 100, 100 ), 16, eeSize( 32, 32 ), MAP_FLAG_DRAW_GRID | MAP_FLAG_CLAMP_BODERS | MAP_FLAG_CLIP_AREA, Params.Size );
	mUIMap->AddEventListener( cUIEvent::EventOnSizeChange, cb::Make1( this, &cMapEditor::OnMapSizeChange ) );
	mUIMap->AddEventListener( cUIEvent::EventMouseDown, cb::Make1( this, &cMapEditor::OnMapMouseDown ) );
	mUIMap->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapEditor::OnMapMouseClick ) );

	mMapHScroll = mTheme->CreateScrollBar( mWinContainer, eeSize( Params.Size.Width(), 15 ), eeVector2i( 0, Params.Size.Height() ), UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mMapHScroll->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cMapEditor::OnScrollMapH ) );

	mMapVScroll = mTheme->CreateScrollBar( mWinContainer, eeSize( 15, Params.Size.Height() ), eeVector2i( Params.Size.Width(), 0 ), UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM, true );
	mMapVScroll->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cMapEditor::OnScrollMapV ) );

	MapCreated();
}

void cMapEditor::OnMapSizeChange( const cUIEvent *Event ) {
	mMapHScroll->MinValue( 0 );
	mMapHScroll->MaxValue( mUIMap->Map()->TotalSize().Width() - mUIMap->Size().Width() );
	mMapHScroll->ClickStep( mUIMap->Map()->TileSize().Width() );
	mMapVScroll->MinValue( 0 );
	mMapVScroll->MaxValue( mUIMap->Map()->TotalSize().Height() - mUIMap->Size().Height() );
	mMapVScroll->ClickStep( mUIMap->Map()->TileSize().Height() );
}

void cMapEditor::OnScrollMapH( const cUIEvent * Event ) {
	cUIScrollBar * Scr = reinterpret_cast<cUIScrollBar*> ( Event->Ctrl() );

	eeVector2f Off = mUIMap->Map()->Offset();

	Off.x = -Scr->Value();

	mUIMap->Map()->Offset( Off ) ;
}

void cMapEditor::OnScrollMapV( const cUIEvent * Event ) {
	cUIScrollBar * Scr = reinterpret_cast<cUIScrollBar*> ( Event->Ctrl() );

	eeVector2f Off = mUIMap->Map()->Offset();

	Off.y = -Scr->Value();

	mUIMap->Map()->Offset( Off ) ;
}

void cMapEditor::MapOpen( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );

	mUIMap->Map()->Load( CDL->GetFullPath() );

	MapCreated();

	RefreshLayersList();
}

void cMapEditor::FileMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsTypeOrInheritsFrom( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "New..." == txt ) {
		CreateNewMap();
	} else if ( "Open..." == txt ) {
		cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, eeSize(), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, eeSize(), 255, UI_CDL_DEFAULT_FLAGS, "*.eem" );

		TGDialog->Title( "Open Map" );
		TGDialog->AddEventListener( cUIEvent::EventOpenFile, cb::Make1( this, &cMapEditor::MapOpen ) );
		TGDialog->Center();
		TGDialog->Show();
	} else if ( "Save" == txt ) {
		//! Testing, temporal!
		mUIMap->Map()->Save( GetProcessPath() + "data/test.eem" );
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == cUIManager::instance()->MainControl() ) {
			cUIManager::instance()->GetWindow()->Close();
		} else {
			mUIWindow->CloseWindow();
		}
	}
}

void cMapEditor::EditMenuClick( const cUIEvent * Event ) {

}

void cMapEditor::ViewMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsTypeOrInheritsFrom( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "Show Grid" == txt ) {
		mUIMap->Map()->DrawGrid( reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() )->Active() );
	} else if ( "Mark Tile Over" == txt ) {
		mUIMap->Map()->DrawTileOver( reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() )->Active() );
	}
}
void cMapEditor::MapMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsTypeOrInheritsFrom( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "Add External Texture Group..." == txt ) {
		cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, eeSize(), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, eeSize(), 255, UI_CDL_DEFAULT_FLAGS, "*.etg" );

		TGDialog->Title( "Load texture group..." );
		TGDialog->AddEventListener( cUIEvent::EventOpenFile, cb::Make1( this, &cMapEditor::TextureGroupOpen ) );
		TGDialog->Center();
		TGDialog->Show();
	}
}

void cMapEditor::LayerMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsTypeOrInheritsFrom( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if (  "Add Tile Layer..." == txt ) {
		eeNew( cUILayerNew, ( mUIMap, MAP_LAYER_TILED, cb::Make1( this, &cMapEditor::OnLayerAdd ) ) );
	} else if ( "Add Object Layer..." == txt ) {
		eeNew( cUILayerNew, ( mUIMap, MAP_LAYER_OBJECT, cb::Make1( this, &cMapEditor::OnLayerAdd ) ) );
	} else if ( "Remove Layer" == txt ) {
		RemoveLayer();
	} else if ( "Move Layer Up" == txt ) {
		MoveLayerUp();
	} else if ( "Move Layer Down" == txt ) {
		MoveLayerDown();
	}
}

void cMapEditor::MoveLayerUp() {
	if ( mUIMap->Map()->MoveLayerUp( mCurLayer ) ) {
		RefreshLayersList();
	}
}

void cMapEditor::MoveLayerDown() {
	if ( mUIMap->Map()->MoveLayerDown( mCurLayer ) ) {
		RefreshLayersList();
	}
}

void cMapEditor::RemoveLayer() {
	if ( mUIMap->Map()->RemoveLayer( mCurLayer ) ) {
		mCurLayer = NULL;

		RefreshLayersList();
	}
}

void cMapEditor::RefreshLayersList() {
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

void cMapEditor::TextureGroupOpen( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );

	std::string sgname = FileRemoveExtension( FileNameFromPath( CDL->GetFullPath() ) );

	if ( NULL == cShapeGroupManager::instance()->GetByName( sgname ) ) {
		cTextureGroupLoader tgl( CDL->GetFullPath() );

		if ( tgl.IsLoaded() ) {
			mShapeGroupsList->ListBox()->AddListBoxItem( sgname );
		}
	}
}

void cMapEditor::OnLayerAdd( cUILayerNew * UILayer ) {
	bool SetSelected = ( 0 == mLayerList->ListBox()->Count() ) ? true : false;

	mLayerList->ListBox()->AddListBoxItem( UILayer->Name() );

	if ( SetSelected ) {
		mCurLayer = UILayer->Layer();
		mLayerList->ListBox()->SetSelected(0);
	}
}

void cMapEditor::OnLayerSelect( const cUIEvent * Event ) {
	cLayer * tLayer = mUIMap->Map()->GetLayer( mLayerList->Text() );

	if ( NULL != tLayer )
		mCurLayer = tLayer;
}

void cMapEditor::WindowClose( const cUIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

cGameObject * cMapEditor::CreateGameObject() {
	cGameObject * tObj	= NULL;

	if ( GAMEOBJECT_TYPE_SHAPE == mCurGOType ) {

		tObj = eeNew( cGameObjectShape, ( mCurGOFlags, mGfxPreview->Shape() ) );

	} else if ( GAMEOBJECT_TYPE_SHAPEEX == mCurGOType ) {

		tObj = eeNew( cGameObjectShapeEx, ( mCurGOFlags, mGfxPreview->Shape() ) );

	} else if ( GAMEOBJECT_TYPE_SPRITE == mCurGOType ) {

		if ( mChkAnim->Active() ) {

			cSprite * tAnimSprite = eeNew( cSprite, ( RemoveNumbersAtEnd( mGfxPreview->Shape()->Name() ) ) );
			tObj = eeNew( cGameObjectSprite, ( mCurGOFlags, tAnimSprite ) );

		} else {

			cSprite * tStaticSprite = eeNew( cSprite, ( mGfxPreview->Shape() ) );
			tObj = eeNew( cGameObjectSprite, ( mCurGOFlags, tStaticSprite ) );

		}
	} else {
		//! Creates an empty game object. The client will interpret the GameObject Type, and instanciate the corresponding class.

		if ( mChkDI->Active() )
			tObj = eeNew( cGameObjectVirtual, ( MakeHash( mDataIdInput->Text().ToUtf8() ), mCurGOFlags, mCurGOType ) );
		else
			tObj = eeNew( cGameObjectVirtual, ( mGfxPreview->Shape(), mCurGOFlags, mCurGOType ) );
	}

	return tObj;
}

void cMapEditor::AddGameObjectToTile() {
	cTileLayer * tLayer	= reinterpret_cast<cTileLayer*> ( mCurLayer );
	cMap * tMap			= mUIMap->Map();
	cGameObject * tObj	= CreateGameObject();

	if ( NULL != tObj ) {
		if ( tObj->Type() == GAMEOBJECT_TYPE_VIRTUAL )
			reinterpret_cast<cGameObjectVirtual*> ( tObj )->SetLayer( tLayer );

		tLayer->AddGameObject( tObj, tMap->GetMouseTilePos() );
	}
}

void cMapEditor::RemoveGameObjectFromTile() {
	cTileLayer * tLayer = reinterpret_cast<cTileLayer*> ( mCurLayer );
	cMap * tMap			= mUIMap->Map();

	tLayer->RemoveGameObject( tMap->GetMouseTilePos() );
}

void cMapEditor::AddGameObject() {
	cObjectLayer * tLayer	= reinterpret_cast<cObjectLayer*> ( mCurLayer );
	cMap * tMap			= mUIMap->Map();
	cGameObject * tObj	= CreateGameObject();

	if ( NULL != tObj ) {
		if ( tObj->Type() == GAMEOBJECT_TYPE_VIRTUAL )
			reinterpret_cast<cGameObjectVirtual*> ( tObj )->SetLayer( tLayer );

		eeVector2i p( tMap->GetMouseMapPos() );

		tObj->Pos( eeVector2f( p.x, p.y ) );
		tLayer->AddGameObject( tObj );
	}
}

void cMapEditor::RemoveGameObject() {
	cObjectLayer * tLayer = reinterpret_cast<cObjectLayer*> ( mCurLayer );
	cMap * tMap			= mUIMap->Map();

	tLayer->RemoveGameObject( tMap->GetMouseMapPos() );
}

void cMapEditor::OnMapMouseClick( const cUIEvent * Event ) {
	const cUIEventMouse * MEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL == mCurLayer || NULL == mGfxPreview->Shape() || cUIManager::instance()->DownControl() != mUIMap )
		return;

	if ( MEvent->Flags() & EE_BUTTON_LMASK ) {
		if ( mCurLayer->Type() == MAP_LAYER_OBJECT )
			AddGameObject();
	} else if ( MEvent->Flags() & EE_BUTTON_RMASK ) {
		if ( mCurLayer->Type() == MAP_LAYER_OBJECT )
			RemoveGameObject();
	}
}

void cMapEditor::OnMapMouseDown( const cUIEvent * Event ) {
	const cUIEventMouse * MEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL == mCurLayer || NULL == mGfxPreview->Shape() || cUIManager::instance()->DownControl() != mUIMap )
		return;

	if ( MEvent->Flags() & EE_BUTTON_LMASK ) {
		if ( mCurLayer->Type() == MAP_LAYER_TILED )
			AddGameObjectToTile();
	} else if ( MEvent->Flags() & EE_BUTTON_RMASK ) {
		if ( mCurLayer->Type() == MAP_LAYER_TILED )
			RemoveGameObjectFromTile();
	}
}

}}}
