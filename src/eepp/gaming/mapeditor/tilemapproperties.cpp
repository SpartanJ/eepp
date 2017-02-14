#include <eepp/gaming/mapeditor/tilemapproperties.hpp>

namespace EE { namespace Gaming { namespace Private {

TileMapProperties::TileMapProperties( TileMap * Map ) :
	mUITheme( NULL ),
	mUIWindow( NULL ),
	mGenGrid( NULL ),
	mMap( Map )
{
	if ( NULL == mMap ) {
		eeDelete( this );
		return;
	}

	mUITheme		= UIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->CreateWindow( NULL, Sizei( 500, 500 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 500, 500 ) );
	mUIWindow->AddEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &TileMapProperties::WindowClose ) );
	mUIWindow->Title( "Map Properties" );


	Uint32 DiffIfLights = 0;

	if ( mMap->LightsEnabled() ) {
		DiffIfLights = 100;

		UITextBox * Txt = mUITheme->CreateTextBox( "Map Base Color:", mUIWindow->Container(), Sizei(), Vector2i( 50, 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		UIComplexControl::CreateParams ComParams;
		ComParams.Parent( mUIWindow->Container() );
		ComParams.PosSet( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 4 );
		ComParams.SizeSet( 64, 64 );
		ComParams.Background.Color( mMap->BaseColor() );
		ComParams.Border.Color( ColorA( 100, 100, 100, 200 ) );
		ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
		mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
		mUIBaseColor->Visible( true );
		mUIBaseColor->Enabled( true );

		Txt = mUITheme->CreateTextBox( "Red Color:", mUIWindow->Container(), Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIBaseColor->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIRedSlider = mUITheme->CreateSlider( mUIWindow->Container(), Sizei( 255, 20 ), Vector2i( Txt->Pos().x + Txt->Size().Width() + 16, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIRedSlider->MaxValue( 255 );
		mUIRedSlider->Value( mMap->BaseColor().r() );
		mUIRedSlider->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnRedChange ) );

		mUIRedTxt = mUITheme->CreateTextBox( String::toStr( (Uint32)mMap->BaseColor().r() ), mUIWindow->Container(), Sizei(), Vector2i( mUIRedSlider->Pos().x + mUIRedSlider->Size().Width() + 4, mUIRedSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->CreateTextBox( "Green Color:", mUIWindow->Container(), Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIRedSlider->Pos().y + mUIRedSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIGreenSlider = mUITheme->CreateSlider( mUIWindow->Container(), Sizei( 255, 20 ), Vector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIGreenSlider->MaxValue( 255 );
		mUIGreenSlider->Value( mMap->BaseColor().g() );
		mUIGreenSlider->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnGreenChange ) );

		mUIGreenTxt = mUITheme->CreateTextBox( String::toStr( (Uint32)mMap->BaseColor().g() ), mUIWindow->Container(), Sizei(), Vector2i( mUIGreenSlider->Pos().x + mUIGreenSlider->Size().Width() + 4, mUIGreenSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->CreateTextBox( "Blue Color:", mUIWindow->Container(), Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIGreenSlider->Pos().y + mUIGreenSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIBlueSlider = mUITheme->CreateSlider( mUIWindow->Container(), Sizei( 255, 20 ), Vector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIBlueSlider->MaxValue( 255 );
		mUIBlueSlider->Value( mMap->BaseColor().b() );
		mUIBlueSlider->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnBlueChange ) );

		mUIBlueTxt = mUITheme->CreateTextBox( String::toStr( (Uint32)mMap->BaseColor().b() ), mUIWindow->Container(), Sizei(), Vector2i( mUIBlueSlider->Pos().x + mUIBlueSlider->Size().Width() + 4, mUIBlueSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	}

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	UITextBox * TxtBox = mUITheme->CreateTextBox( "Property Name", mUIWindow->Container(), Sizei(192, 24), Vector2i( 50, 10 + DiffIfLights ), TxtBoxFlags );
	mUITheme->CreateTextBox( "Property Value", mUIWindow->Container(), Sizei(192, 24), Vector2i(50+192, TxtBox->Pos().y ), TxtBoxFlags );

	UIPushButton * OKButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::OKClick ) );

	OKButton->Text( "OK" );

	UIPushButton * CancelButton = mUITheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), Vector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::CancelClick ) );
	CancelButton->Text( "Cancel" );

	UIGenericGrid::CreateParams GridParams;
	GridParams.Parent( mUIWindow->Container() );
	GridParams.PosSet( 50, TxtBox->Pos().y + 20 );
	GridParams.SizeSet( 400, 400 - DiffIfLights );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 5;
	mGenGrid = eeNew( UIGenericGrid, ( GridParams ) );
	mGenGrid->Visible( true );
	mGenGrid->Enabled( true );
	mGenGrid->CollumnWidth( 0, 10 );
	mGenGrid->CollumnWidth( 1, 175 );
	mGenGrid->CollumnWidth( 2, 10 );
	mGenGrid->CollumnWidth( 3, 175 );
	mGenGrid->CollumnWidth( 4, 10 );

	Vector2i Pos( mGenGrid->Pos().x + mGenGrid->Size().Width() + 10, mGenGrid->Pos().y );

	UIPushButton * AddButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "add" ) );
	AddButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::AddCellClick ) );

	if ( NULL == AddButton->Icon()->SubTexture() )
		AddButton->Text( "+" );

	Pos.y += AddButton->Size().Height() + 5;

	UIPushButton * RemoveButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "remove" )  );
	RemoveButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->Icon()->SubTexture() )
		RemoveButton->Text( "-" );

	CreateGridElems();

	mUIWindow->Center();
	mUIWindow->Show();
}

TileMapProperties::~TileMapProperties() {
}

void TileMapProperties::OnRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Red = (Uint8)mUIRedSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIRedTxt->Text( String::toStr( (Int32)mUIRedSlider->Value() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Red = Col.Red;
	mMap->BaseColor( MapCol );
}

void TileMapProperties::OnGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Green = (Uint8)mUIGreenSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIGreenTxt->Text( String::toStr( (Uint32)mUIGreenSlider->Value() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Green = Col.Green;
	mMap->BaseColor( MapCol );
}

void TileMapProperties::OnBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Blue = (Uint8)mUIBlueSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIBlueTxt->Text( String::toStr( (Uint32)mUIBlueSlider->Value() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Blue = Col.Blue;
	mMap->BaseColor( MapCol );
}


void TileMapProperties::SaveProperties() {
	mMap->ClearProperties();

	for ( Uint32 i = 0; i < mGenGrid->Count(); i++ ) {
		UIGridCell * Cell = mGenGrid->GetCell( i );

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->Cell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->Cell( 3 ) );

		if ( NULL != Cell && Input->Text().size() && Input2->Text().size() ) {
			mMap->AddProperty(	Input->Text(), Input2->Text() );
		}
	}
}

void TileMapProperties::LoadProperties() {
	TileMap::PropertiesMap& Proper = mMap->GetProperties();

	for ( TileMap::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		UIGridCell * Cell = CreateCell();

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->Cell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->Cell( 3 ) );

		Input->Text( it->first );
		Input2->Text( it->second );

		mGenGrid->Add( Cell );
	}
}

void TileMapProperties::OKClick( const UIEvent * Event ) {
	SaveProperties();

	mUIWindow->CloseWindow();
}

void TileMapProperties::CancelClick( const UIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void TileMapProperties::WindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

void TileMapProperties::AddCellClick( const UIEvent * Event ) {
	mGenGrid->Add( CreateCell() );

	Uint32 Index = mGenGrid->GetItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->GetCell( 0 )->Select();
	}
}

void TileMapProperties::RemoveCellClick( const UIEvent * Event ) {
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

void TileMapProperties::CreateGridElems() {
	LoadProperties();

	if ( 0 == mGenGrid->Count() ) {
		AddCellClick( NULL );
	} else {
		mGenGrid->GetCell( 0 )->Select();
	}
}

UIGridCell * TileMapProperties::CreateCell() {
	UIGridCell::CreateParams CellParams;
	CellParams.Parent( mGenGrid->Container() );

	UITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED;
	TxtInputParams.MaxLength = LAYER_NAME_SIZE;

	UIComplexControl::CreateParams CControl;

	UIGridCell * Cell			= eeNew( UIGridCell, ( CellParams ) );
	UITextInput * TxtInput		= eeNew( UITextInput, ( TxtInputParams ) );
	UITextInput * TxtInput2	= eeNew( UITextInput, ( TxtInputParams ) );

	Cell->Cell( 0, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->Cell( 1, TxtInput );

	Cell->Cell( 2, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->Cell( 3, TxtInput2 );

	Cell->Cell( 4, eeNew( UIComplexControl, ( CControl ) ) );

	return Cell;
}

}}}

