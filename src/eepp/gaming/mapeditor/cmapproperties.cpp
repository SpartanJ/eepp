#include <eepp/gaming/mapeditor/cmapproperties.hpp>

namespace EE { namespace Gaming { namespace MapEditor {

cMapProperties::cMapProperties( cMap * Map ) :
	mUITheme( NULL ),
	mUIWindow( NULL ),
	mGenGrid( NULL ),
	mMap( Map )
{
	if ( NULL == mMap ) {
		eeDelete( this );
		return;
	}

	mUITheme		= cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->CreateWindow( NULL, eeSize( 500, 500 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cMapProperties::WindowClose ) );
	mUIWindow->Title( "Map Properties" );


	Uint32 DiffIfLights = 0;

	if ( mMap->LightsEnabled() ) {
		DiffIfLights = 100;

		cUITextBox * Txt = mUITheme->CreateTextBox( "Map Base Color:", mUIWindow->Container(), eeSize(), eeVector2i( 50, 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		cUIComplexControl::CreateParams ComParams;
		ComParams.Parent( mUIWindow->Container() );
		ComParams.PosSet( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 4 );
		ComParams.SizeSet( 64, 64 );
		ComParams.Background.Color( mMap->BaseColor() );
		ComParams.Border.Color( eeColorA( 100, 100, 100, 200 ) );
		ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
		mUIBaseColor = eeNew( cUIComplexControl, ( ComParams ) );
		mUIBaseColor->Visible( true );
		mUIBaseColor->Enabled( true );

		Txt = mUITheme->CreateTextBox( "Red Color:", mUIWindow->Container(), eeSize(), eeVector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIBaseColor->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIRedSlider = mUITheme->CreateSlider( mUIWindow->Container(), eeSize( 255, 20 ), eeVector2i( Txt->Pos().x + Txt->Size().Width() + 16, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIRedSlider->MaxValue( 255 );
		mUIRedSlider->Value( mMap->BaseColor().R() );
		mUIRedSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cMapProperties::OnRedChange ) );

		mUIRedTxt = mUITheme->CreateTextBox( String::toStr( (Uint32)mMap->BaseColor().R() ), mUIWindow->Container(), eeSize(), eeVector2i( mUIRedSlider->Pos().x + mUIRedSlider->Size().Width() + 4, mUIRedSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->CreateTextBox( "Green Color:", mUIWindow->Container(), eeSize(), eeVector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIRedSlider->Pos().y + mUIRedSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIGreenSlider = mUITheme->CreateSlider( mUIWindow->Container(), eeSize( 255, 20 ), eeVector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIGreenSlider->MaxValue( 255 );
		mUIGreenSlider->Value( mMap->BaseColor().G() );
		mUIGreenSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cMapProperties::OnGreenChange ) );

		mUIGreenTxt = mUITheme->CreateTextBox( String::toStr( (Uint32)mMap->BaseColor().G() ), mUIWindow->Container(), eeSize(), eeVector2i( mUIGreenSlider->Pos().x + mUIGreenSlider->Size().Width() + 4, mUIGreenSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->CreateTextBox( "Blue Color:", mUIWindow->Container(), eeSize(), eeVector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIGreenSlider->Pos().y + mUIGreenSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIBlueSlider = mUITheme->CreateSlider( mUIWindow->Container(), eeSize( 255, 20 ), eeVector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIBlueSlider->MaxValue( 255 );
		mUIBlueSlider->Value( mMap->BaseColor().B() );
		mUIBlueSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cMapProperties::OnBlueChange ) );

		mUIBlueTxt = mUITheme->CreateTextBox( String::toStr( (Uint32)mMap->BaseColor().B() ), mUIWindow->Container(), eeSize(), eeVector2i( mUIBlueSlider->Pos().x + mUIBlueSlider->Size().Width() + 4, mUIBlueSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	}

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	cUITextBox * TxtBox = mUITheme->CreateTextBox( "Property Name", mUIWindow->Container(), eeSize(192, 24), eeVector2i( 50, 10 + DiffIfLights ), TxtBoxFlags );
	mUITheme->CreateTextBox( "Property Value", mUIWindow->Container(), eeSize(192, 24), eeVector2i(50+192, TxtBox->Pos().y ), TxtBoxFlags );

	cUIPushButton * OKButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize( 80, 22 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapProperties::OKClick ) );

	OKButton->Text( "OK" );

	cUIPushButton * CancelButton = mUITheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), eeVector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapProperties::CancelClick ) );
	CancelButton->Text( "Cancel" );

	cUIGenericGrid::CreateParams GridParams;
	GridParams.Parent( mUIWindow->Container() );
	GridParams.PosSet( 50, TxtBox->Pos().y + 20 );
	GridParams.SizeSet( 400, 400 - DiffIfLights );
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
	AddButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapProperties::AddCellClick ) );

	if ( NULL == AddButton->Icon()->SubTexture() )
		AddButton->Text( "+" );

	Pos.y += AddButton->Size().Height() + 5;

	cUIPushButton * RemoveButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "remove" )  );
	RemoveButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cMapProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->Icon()->SubTexture() )
		RemoveButton->Text( "-" );

	CreateGridElems();

	mUIWindow->Center();
	mUIWindow->Show();
}

cMapProperties::~cMapProperties() {
}

void cMapProperties::OnRedChange( const cUIEvent * Event ) {
	eeColorA Col = mUIBaseColor->Background()->Color();
	Col.Red = (Uint8)mUIRedSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIRedTxt->Text( String::toStr( (Int32)mUIRedSlider->Value() ) );

	eeColorA MapCol = mMap->BaseColor();
	MapCol.Red = Col.Red;
	mMap->BaseColor( MapCol );
}

void cMapProperties::OnGreenChange( const cUIEvent * Event ) {
	eeColorA Col = mUIBaseColor->Background()->Color();
	Col.Green = (Uint8)mUIGreenSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIGreenTxt->Text( String::toStr( (Uint32)mUIGreenSlider->Value() ) );

	eeColorA MapCol = mMap->BaseColor();
	MapCol.Green = Col.Green;
	mMap->BaseColor( MapCol );
}

void cMapProperties::OnBlueChange( const cUIEvent * Event ) {
	eeColorA Col = mUIBaseColor->Background()->Color();
	Col.Blue = (Uint8)mUIBlueSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIBlueTxt->Text( String::toStr( (Uint32)mUIBlueSlider->Value() ) );

	eeColorA MapCol = mMap->BaseColor();
	MapCol.Blue = Col.Blue;
	mMap->BaseColor( MapCol );
}


void cMapProperties::SaveProperties() {
	mMap->ClearProperties();

	for ( Uint32 i = 0; i < mGenGrid->Count(); i++ ) {
		cUIGridCell * Cell = mGenGrid->GetCell( i );

		cUITextInput * Input = reinterpret_cast<cUITextInput*>( Cell->Cell( 1 ) );
		cUITextInput * Input2 = reinterpret_cast<cUITextInput*>( Cell->Cell( 3 ) );

		if ( NULL != Cell && Input->Text().size() && Input2->Text().size() ) {
			mMap->AddProperty(	Input->Text(), Input2->Text() );
		}
	}
}

void cMapProperties::LoadProperties() {
	cMap::PropertiesMap& Proper = mMap->GetProperties();

	for ( cMap::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		cUIGridCell * Cell = CreateCell();

		cUITextInput * Input = reinterpret_cast<cUITextInput*>( Cell->Cell( 1 ) );
		cUITextInput * Input2 = reinterpret_cast<cUITextInput*>( Cell->Cell( 3 ) );

		Input->Text( it->first );
		Input2->Text( it->second );

		mGenGrid->Add( Cell );
	}
}

void cMapProperties::OKClick( const cUIEvent * Event ) {
	SaveProperties();

	mUIWindow->CloseWindow();
}

void cMapProperties::CancelClick( const cUIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void cMapProperties::WindowClose( const cUIEvent * Event ) {
	eeDelete( this );
}

void cMapProperties::AddCellClick( const cUIEvent * Event ) {
	mGenGrid->Add( CreateCell() );

	Uint32 Index = mGenGrid->GetItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->GetCell( 0 )->Select();
	}
}

void cMapProperties::RemoveCellClick( const cUIEvent * Event ) {
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

void cMapProperties::CreateGridElems() {
	LoadProperties();

	if ( 0 == mGenGrid->Count() ) {
		AddCellClick( NULL );
	} else {
		mGenGrid->GetCell( 0 )->Select();
	}
}

cUIGridCell * cMapProperties::CreateCell() {
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

