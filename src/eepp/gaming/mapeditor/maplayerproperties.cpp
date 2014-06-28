#include <eepp/gaming/mapeditor/maplayerproperties.hpp>
#include <eepp/gaming/mapeditor/mapeditor.hpp>

namespace EE { namespace Gaming { namespace Private {

MapLayerProperties::MapLayerProperties( MapLayer * Map, RefreshLayerListCb Cb ) :
	mUITheme( NULL ),
	mUIWindow( NULL ),
	mGenGrid( NULL ),
	mLayer( Map ),
	mRefreshCb( Cb )
{
	if ( NULL == mLayer ) {
		eeDelete( this );
		return;
	}

	mUITheme		= cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->CreateWindow( NULL, Sizei( 500, 500 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 500, 500 ) );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &MapLayerProperties::WindowClose ) );
	mUIWindow->Title( "Layer Properties" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	cUITextBox * Txt = mUITheme->CreateTextBox( "Layer name:", mUIWindow->Container(), Sizei(), Vector2i( 50, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput = mUITheme->CreateTextInput( mUIWindow->Container(), Sizei( 120, 22 ), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput->Text( mLayer->Name() );
	mUIInput->AddEventListener( cUIEvent::EventOnPressEnter, cb::Make1( this, &MapLayerProperties::OKClick ) );

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	mUITheme->CreateTextBox( "Property Name", mUIWindow->Container(), Sizei(192, 24), Vector2i( 50, mUIInput->Pos().y + mUIInput->Size().Height() + 12 ), TxtBoxFlags );
	cUITextBox * TxtBox = mUITheme->CreateTextBox( "Property Value", mUIWindow->Container(), Sizei(192, 24), Vector2i( 50+192, mUIInput->Pos().y + mUIInput->Size().Height() + 12 ), TxtBoxFlags );

	cUIPushButton * OKButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::OKClick ) );

	OKButton->Text( "OK" );

	cUIPushButton * CancelButton = mUITheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), Vector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::CancelClick ) );
	CancelButton->Text( "Cancel" );

	cUIGenericGrid::CreateParams GridParams;
	GridParams.Parent( mUIWindow->Container() );
	GridParams.PosSet( 50, TxtBox->Pos().y + TxtBox->Size().Height() );
	GridParams.SizeSet( 400, 350 );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 5;
	mGenGrid = eeNew( cUIGenericGrid, ( GridParams ) );
	mGenGrid->Visible( true );
	mGenGrid->Enabled( true );
	mGenGrid->CollumnWidth( 0, 10 );
	mGenGrid->CollumnWidth( 1, 175 );
	mGenGrid->CollumnWidth( 2, 10 );
	mGenGrid->CollumnWidth( 3, 175 );
	mGenGrid->CollumnWidth( 4, 10 );

	Vector2i Pos( mGenGrid->Pos().x + mGenGrid->Size().Width() + 10, mGenGrid->Pos().y );

	cUIPushButton * AddButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "add" ) );
	AddButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::AddCellClick ) );

	if ( NULL == AddButton->Icon()->SubTexture() )
		AddButton->Text( "+" );

	Pos.y += AddButton->Size().Height() + 5;

	cUIPushButton * RemoveButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "remove" )  );
	RemoveButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->Icon()->SubTexture() )
		RemoveButton->Text( "-" );

	CreateGridElems();

	mUIWindow->Center();
	mUIWindow->Show();
}

MapLayerProperties::~MapLayerProperties() {
}

void MapLayerProperties::SaveProperties() {
	mLayer->ClearProperties();

	for ( Uint32 i = 0; i < mGenGrid->Count(); i++ ) {
		cUIGridCell * Cell = mGenGrid->GetCell( i );

		cUITextInput * Input = reinterpret_cast<cUITextInput*>( Cell->Cell( 1 ) );
		cUITextInput * Input2 = reinterpret_cast<cUITextInput*>( Cell->Cell( 3 ) );

		if ( NULL != Cell && Input->Text().size() && Input2->Text().size() ) {
			mLayer->AddProperty(	Input->Text(), Input2->Text() );
		}
	}
}

void MapLayerProperties::LoadProperties() {
	MapLayer::PropertiesMap& Proper = mLayer->GetProperties();

	for ( MapLayer::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		cUIGridCell * Cell = CreateCell();

		cUITextInput * Input = reinterpret_cast<cUITextInput*>( Cell->Cell( 1 ) );
		cUITextInput * Input2 = reinterpret_cast<cUITextInput*>( Cell->Cell( 3 ) );

		Input->Text( it->first );
		Input2->Text( it->second );

		mGenGrid->Add( Cell );
	}
}

void MapLayerProperties::OKClick( const cUIEvent * Event ) {
	SaveProperties();

	mLayer->Name( mUIInput->Text().ToUtf8() );

	if ( mRefreshCb.IsSet() ) {
		mRefreshCb();
	}

	mUIWindow->CloseWindow();
}

void MapLayerProperties::CancelClick( const cUIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void MapLayerProperties::WindowClose( const cUIEvent * Event ) {
	eeDelete( this );
}

void MapLayerProperties::AddCellClick( const cUIEvent * Event ) {
	mGenGrid->Add( CreateCell() );

	Uint32 Index = mGenGrid->GetItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->GetCell( 0 )->Select();
	}
}

void MapLayerProperties::RemoveCellClick( const cUIEvent * Event ) {
	Uint32 Index = mGenGrid->GetItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND != Index ) {
		mGenGrid->Remove( Index );

		if ( Index < mGenGrid->Count() ) {
			mGenGrid->GetCell( Index )->Select();
		} else {
			if ( mGenGrid->Count() ) {
				if ( Index > 0 )
					mGenGrid->GetCell( Index - 1 )->Select();
			}
		}
	}
}

void MapLayerProperties::CreateGridElems() {
	LoadProperties();

	if ( 0 == mGenGrid->Count() ) {
		AddCellClick( NULL );
	} else {
		mGenGrid->GetCell( 0 )->Select();
	}
}

cUIGridCell * MapLayerProperties::CreateCell() {
	cUIGridCell::CreateParams CellParams;
	CellParams.Parent( mGenGrid->Container() );

	cUITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED;
	TxtInputParams.MaxLength = LAYER_NAME_SIZE;

	cUIComplexControl::CreateParams CControl;

	cUIGridCell * Cell			= eeNew( cUIGridCell, ( CellParams ) );
	cUITextInput * TxtInput		= eeNew( cUITextInput, ( TxtInputParams ) );
	cUITextInput * TxtInput2	= eeNew( cUITextInput, ( TxtInputParams ) );

	Cell->Cell( 0, eeNew( cUIComplexControl, ( CControl ) ) );

	Cell->Cell( 1, TxtInput );

	Cell->Cell( 2, eeNew( cUIComplexControl, ( CControl ) ) );

	Cell->Cell( 3, TxtInput2 );

	Cell->Cell( 4, eeNew( cUIComplexControl, ( CControl ) ) );

	return Cell;
}

}}}
