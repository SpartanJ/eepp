#include <eepp/gaming/mapeditor/clayerproperties.hpp>
#include <eepp/gaming/mapeditor/cmapeditor.hpp>

namespace EE { namespace Gaming { namespace MapEditor {

cLayerProperties::cLayerProperties( cLayer * Map, RefreshLayerListCb Cb ) :
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

	mUIWindow	= mUITheme->CreateWindow( NULL, eeSize( 500, 500 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, eeSize( 500, 500 ) );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cLayerProperties::WindowClose ) );
	mUIWindow->Title( "Layer Properties" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	cUITextBox * Txt = mUITheme->CreateTextBox( "Layer name:", mUIWindow->Container(), eeSize(), eeVector2i( 50, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput = mUITheme->CreateTextInput( mUIWindow->Container(), eeSize( 120, 22 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput->Text( mLayer->Name() );
	mUIInput->AddEventListener( cUIEvent::EventOnPressEnter, cb::Make1( this, &cLayerProperties::OKClick ) );

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	mUITheme->CreateTextBox( "Property Name", mUIWindow->Container(), eeSize(192, 24), eeVector2i( 50, mUIInput->Pos().y + mUIInput->Size().Height() + 12 ), TxtBoxFlags );
	cUITextBox * TxtBox = mUITheme->CreateTextBox( "Property Value", mUIWindow->Container(), eeSize(192, 24), eeVector2i( 50+192, mUIInput->Pos().y + mUIInput->Size().Height() + 12 ), TxtBoxFlags );

	cUIPushButton * OKButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize( 80, 22 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cLayerProperties::OKClick ) );

	OKButton->Text( "OK" );

	cUIPushButton * CancelButton = mUITheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), eeVector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cLayerProperties::CancelClick ) );
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

	eeVector2i Pos( mGenGrid->Pos().x + mGenGrid->Size().Width() + 10, mGenGrid->Pos().y );

	cUIPushButton * AddButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "add" ) );
	AddButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cLayerProperties::AddCellClick ) );

	if ( NULL == AddButton->Icon()->SubTexture() )
		AddButton->Text( "+" );

	Pos.y += AddButton->Size().Height() + 5;

	cUIPushButton * RemoveButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "remove" )  );
	RemoveButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cLayerProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->Icon()->SubTexture() )
		RemoveButton->Text( "-" );

	CreateGridElems();

	mUIWindow->Center();
	mUIWindow->Show();
}

cLayerProperties::~cLayerProperties() {
}

void cLayerProperties::SaveProperties() {
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

void cLayerProperties::LoadProperties() {
	cLayer::PropertiesMap& Proper = mLayer->GetProperties();

	for ( cLayer::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		cUIGridCell * Cell = CreateCell();

		cUITextInput * Input = reinterpret_cast<cUITextInput*>( Cell->Cell( 1 ) );
		cUITextInput * Input2 = reinterpret_cast<cUITextInput*>( Cell->Cell( 3 ) );

		Input->Text( it->first );
		Input2->Text( it->second );

		mGenGrid->Add( Cell );
	}
}

void cLayerProperties::OKClick( const cUIEvent * Event ) {
	SaveProperties();

	mLayer->Name( mUIInput->Text().ToUtf8() );

	if ( mRefreshCb.IsSet() ) {
		mRefreshCb();
	}

	mUIWindow->CloseWindow();
}

void cLayerProperties::CancelClick( const cUIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void cLayerProperties::WindowClose( const cUIEvent * Event ) {
	eeDelete( this );
}

void cLayerProperties::AddCellClick( const cUIEvent * Event ) {
	mGenGrid->Add( CreateCell() );

	Uint32 Index = mGenGrid->GetItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->GetCell( 0 )->Select();
	}
}

void cLayerProperties::RemoveCellClick( const cUIEvent * Event ) {
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

void cLayerProperties::CreateGridElems() {
	LoadProperties();

	if ( 0 == mGenGrid->Count() ) {
		AddCellClick( NULL );
	} else {
		mGenGrid->GetCell( 0 )->Select();
	}
}

cUIGridCell * cLayerProperties::CreateCell() {
	cUIGridCell::CreateParams CellParams;
	CellParams.Parent( mGenGrid->Container() );

	cUITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_AUTO_PADDING;
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

