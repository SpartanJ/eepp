#include "cmapeditor.hpp"
#include "../../ui/cuimanager.hpp"
#include "../../ui/cuithememanager.hpp"
#include "../../ui/cuiwinmenu.hpp"
#include "../../ui/cuipopupmenu.hpp"
#include "../../ui/cuicommondialog.hpp"
#include "../../graphics/cshapegroupmanager.hpp"
#include "../../graphics/ctexturegrouploader.hpp"

using namespace EE::Graphics;

namespace EE { namespace Gaming { namespace MapEditor {

cMapEditor::cMapEditor( cUIWindow * AttatchTo, const MapEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mCloseCb( callback )
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
	PU1->Add( "New..." );
	PU1->Add( "Open..." );
	PU1->AddSeparator();
	PU1->Add( "Save" );
	PU1->Add( "Save As..." );
	PU1->AddSeparator();
	PU1->Add( "Close" );
	PU1->AddSeparator();
	PU1->Add( "Quit" );

	PU1->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::FileMenuClick ) );
	WinMenu->AddMenuButton( "File", PU1 );

	cUIPopUpMenu * PU2 = mTheme->CreatePopUpMenu();
	PU2->Add( "Preferences..." );

	PU2->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::EditMenuClick ) );
	WinMenu->AddMenuButton( "Edit", PU2 );

	cUIPopUpMenu * PU3 = mTheme->CreatePopUpMenu();
	PU3->AddCheckBox( "Show Grid" );

	reinterpret_cast<cUIMenuCheckBox*> ( PU3->GetItem( "Show Grid" ) )->Active( true );

	PU3->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::ViewMenuClick ) );
	WinMenu->AddMenuButton( "View", PU3 );

	cUIPopUpMenu * PU4 = mTheme->CreatePopUpMenu();
	PU4->Add( "Create New Texture Group..." );
	PU4->Add( "Add External Texture Group..." );
	PU4->AddSeparator();
	PU4->Add( "Map Properties..." );

	PU4->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cMapEditor::MapMenuClick ) );
	WinMenu->AddMenuButton( "Map", PU4 );

	cUIPopUpMenu * PU5 = mTheme->CreatePopUpMenu();
	PU5->Add( "Add Tile Layer..." );
	PU5->Add( "Add Object Layer..." );
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

void cMapEditor::FileMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsTypeOrInheritsFrom( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "New..." == txt ) {
		CreateNewMap();
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == cUIManager::instance()->MainControl() ) {
			cUIManager::instance()->GetWindow()->Close();
		} else {
			mUIWindow->CloseWindow();
		}
	}
}

void cMapEditor::CreateETGMenu() {
	Int32 Width = 200;

	cUITextBox * Txt = mTheme->CreateTextBox( mWinContainer, eeSize( Width, 16 ), eeVector2i( mWinContainer->Size().Width() - Width - 5, 4 ), UI_CONTROL_DEFAULT_ALIGN  | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	Txt->Text( "Shape Groups:" );

	mShapeGroupsList = mTheme->CreateDropDownList( mWinContainer, eeSize( Width, 21 ), eeVector2i( mWinContainer->Size().Width() - Width - 5, Txt->Pos().y +Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mShapeGroupsList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &cMapEditor::OnShapeGroupChange ) );

	mShapeList = mTheme->CreateListBox( mWinContainer, eeSize( Width, 156 ), eeVector2i( mWinContainer->Size().Width() - Width - 5, mShapeGroupsList->Pos().y + mShapeGroupsList->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mShapeList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &cMapEditor::OnShapeChange ) );

	mGfxPreview = mTheme->CreateGfx( NULL, mWinContainer, eeSize( Width, Width ), eeVector2i( mWinContainer->Size().Width() - Width - 5, mShapeList->Pos().y + mShapeList->Size().Height() + 4 ), UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_AUTO_FIT );
	mGfxPreview->Border( true );

	Txt = mTheme->CreateTextBox( mWinContainer, eeSize( Width, 16 ), eeVector2i( mWinContainer->Size().Width() - Width - 5, mGfxPreview->Pos().y + mGfxPreview->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	Txt->Text( "Add Game Object as..." );

	mTypeAdd = mTheme->CreateDropDownList( mWinContainer, eeSize( Width, 1 ), eeVector2i( mWinContainer->Size().Width() - Width - 5, Txt->Pos().y + Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	std::vector<String> items;
	items.push_back( "Shape" );
	items.push_back( "ShapeEx" );
	items.push_back( "Sprite" );
	mTypeAdd->ListBox()->AddListBoxItems( items );
	mTypeAdd->ListBox()->SetSelected(0);

	Txt = mTheme->CreateTextBox( mWinContainer, eeSize( Width, 16 ), eeVector2i( mWinContainer->Size().Width() - Width - 5, mTypeAdd->Pos().y + mTypeAdd->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	Txt->Text( "Layers:" );

	mLayerList = mTheme->CreateDropDownList( mWinContainer, eeSize( Width, 21 ), eeVector2i( mWinContainer->Size().Width() - Width - 5, Txt->Pos().y + Txt->Size().Height() + 4 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mLayerList->ListBox()->AddListBoxItem( "Floor" );
	mLayerList->ListBox()->SetSelected(0);

	FillSGCombo();
}

void cMapEditor::FillSGCombo() {
	cShapeGroupManager * SGM = cShapeGroupManager::instance();
	std::list<cShapeGroup*>& Res = SGM->GetResources();

	mShapeGroupsList->ListBox()->Clear();

	std::vector<String> items;

	for ( std::list<cShapeGroup*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
		items.push_back( reinterpret_cast<cShapeGroup*>( *it )->Name() );
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
			mShapeList->AddListBoxItems( items );
			mShapeList->SetSelected(0);
		}
	}
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
	mUIMap->Map()->Create( eeSize( 50, 50 ), 16, eeSize( 32, 32 ), MAP_FLAG_DRAW_GRID | MAP_FLAG_CLAMP_BODERS | MAP_FLAG_CLIP_AREA, Params.Size );
	mUIMap->AddEventListener( cUIEvent::EventOnSizeChange, cb::Make1( this, &cMapEditor::OnMapSizeChange ) );

	mMapHScroll = mTheme->CreateScrollBar( mWinContainer, eeSize( Params.Size.Width(), 15 ), eeVector2i( 0, Params.Size.Height() ), UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mMapHScroll->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cMapEditor::OnScrollMapH ) );

	mMapVScroll = mTheme->CreateScrollBar( mWinContainer, eeSize( 15, Params.Size.Height() ), eeVector2i( Params.Size.Width(), 0 ), UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM, true );
	mMapVScroll->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cMapEditor::OnScrollMapV ) );

	OnMapSizeChange( NULL );
}

void cMapEditor::OnMapSizeChange( const cUIEvent *Event ) {
	mMapHScroll->MinValue( 0 );
	mMapHScroll->MaxValue( mUIMap->Map()->Size().Width() * mUIMap->Map()->TileSize().Width() - mUIMap->Size().Width() );
	mMapHScroll->ClickStep( mUIMap->Map()->TileSize().Width() );
	mMapVScroll->MinValue( 0 );
	mMapVScroll->MaxValue( mUIMap->Map()->Size().Height() * mUIMap->Map()->TileSize().Height() - mUIMap->Size().Height() );
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

void cMapEditor::EditMenuClick( const cUIEvent * Event ) {

}

void cMapEditor::ViewMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsTypeOrInheritsFrom( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "Show Grid" == txt ) {
		mUIMap->Map()->DrawGrid( reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() )->Active() );
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

void cMapEditor::TextureGroupOpen( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );

	std::string sgname = FileRemoveExtension( FileNameFromPath( CDL->GetFullPath() ) );

	if ( NULL == cShapeGroupManager::instance()->GetByName( sgname ) ) {
		cTextureGroupLoader tgl( CDL->GetFullPath() );

		if ( tgl.IsLoaded() ) {
			FillSGCombo();
		}
	}
}

void cMapEditor::LayerMenuClick( const cUIEvent * Event ) {

}

void cMapEditor::WindowClose( const cUIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

}}}
