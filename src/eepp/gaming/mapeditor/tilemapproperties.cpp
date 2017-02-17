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

	mUITheme		= UIThemeManager::instance()->defaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->createWindow( NULL, Sizei( 500, 500 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 500, 500 ) );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &TileMapProperties::WindowClose ) );
	mUIWindow->title( "Map Properties" );


	Uint32 DiffIfLights = 0;

	if ( mMap->LightsEnabled() ) {
		DiffIfLights = 100;

		UITextBox * Txt = mUITheme->createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizei(), Vector2i( 50, 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		UIComplexControl::CreateParams ComParams;
		ComParams.setParent( mUIWindow->getContainer() );
		ComParams.setPos( Txt->position().x, Txt->position().y + Txt->size().height() + 4 );
		ComParams.setSize( 64, 64 );
		ComParams.Background.color( mMap->BaseColor() );
		ComParams.Border.color( ColorA( 100, 100, 100, 200 ) );
		ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
		mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
		mUIBaseColor->visible( true );
		mUIBaseColor->enabled( true );

		Txt = mUITheme->createTextBox( "Red Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIBaseColor->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIRedSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( Txt->position().x + Txt->size().width() + 16, Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIRedSlider->maxValue( 255 );
		mUIRedSlider->value( mMap->BaseColor().r() );
		mUIRedSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnRedChange ) );

		mUIRedTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->BaseColor().r() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIRedSlider->position().x + mUIRedSlider->size().width() + 4, mUIRedSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->createTextBox( "Green Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIRedSlider->position().y + mUIRedSlider->size().height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIGreenSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( mUIRedSlider->position().x, Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIGreenSlider->maxValue( 255 );
		mUIGreenSlider->value( mMap->BaseColor().g() );
		mUIGreenSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnGreenChange ) );

		mUIGreenTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->BaseColor().g() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIGreenSlider->position().x + mUIGreenSlider->size().width() + 4, mUIGreenSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->createTextBox( "Blue Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIGreenSlider->position().y + mUIGreenSlider->size().height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIBlueSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( mUIRedSlider->position().x, Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIBlueSlider->maxValue( 255 );
		mUIBlueSlider->value( mMap->BaseColor().b() );
		mUIBlueSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnBlueChange ) );

		mUIBlueTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->BaseColor().b() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIBlueSlider->position().x + mUIBlueSlider->size().width() + 4, mUIBlueSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	}

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	UITextBox * TxtBox = mUITheme->createTextBox( "Property Name", mUIWindow->getContainer(), Sizei(192, 24), Vector2i( 50, 10 + DiffIfLights ), TxtBoxFlags );
	mUITheme->createTextBox( "Property Value", mUIWindow->getContainer(), Sizei(192, 24), Vector2i(50+192, TxtBox->position().y ), TxtBoxFlags );

	UIPushButton * OKButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->getIconByName( "ok" ) );
	OKButton->position( mUIWindow->getContainer()->size().width() - OKButton->size().width() - 4, mUIWindow->getContainer()->size().height() - OKButton->size().height() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::OKClick ) );

	OKButton->text( "OK" );

	UIPushButton * CancelButton = mUITheme->createPushButton( mUIWindow->getContainer(), OKButton->size(), Vector2i( OKButton->position().x - OKButton->size().width() - 4, OKButton->position().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::CancelClick ) );
	CancelButton->text( "Cancel" );

	UIGenericGrid::CreateParams GridParams;
	GridParams.setParent( mUIWindow->getContainer() );
	GridParams.setPos( 50, TxtBox->position().y + 20 );
	GridParams.setSize( 400, 400 - DiffIfLights );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 5;
	mGenGrid = eeNew( UIGenericGrid, ( GridParams ) );
	mGenGrid->visible( true );
	mGenGrid->enabled( true );
	mGenGrid->collumnWidth( 0, 10 );
	mGenGrid->collumnWidth( 1, 175 );
	mGenGrid->collumnWidth( 2, 10 );
	mGenGrid->collumnWidth( 3, 175 );
	mGenGrid->collumnWidth( 4, 10 );

	Vector2i Pos( mGenGrid->position().x + mGenGrid->size().width() + 10, mGenGrid->position().y );

	UIPushButton * AddButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->getIconByName( "add" ) );
	AddButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::AddCellClick ) );

	if ( NULL == AddButton->icon()->subTexture() )
		AddButton->text( "+" );

	Pos.y += AddButton->size().height() + 5;

	UIPushButton * RemoveButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->getIconByName( "remove" )  );
	RemoveButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->icon()->subTexture() )
		RemoveButton->text( "-" );

	CreateGridElems();

	mUIWindow->center();
	mUIWindow->show();
}

TileMapProperties::~TileMapProperties() {
}

void TileMapProperties::OnRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Red = (Uint8)mUIRedSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIRedTxt->text( String::toStr( (Int32)mUIRedSlider->value() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Red = Col.Red;
	mMap->BaseColor( MapCol );
}

void TileMapProperties::OnGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Green = (Uint8)mUIGreenSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIGreenTxt->text( String::toStr( (Uint32)mUIGreenSlider->value() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Green = Col.Green;
	mMap->BaseColor( MapCol );
}

void TileMapProperties::OnBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Blue = (Uint8)mUIBlueSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIBlueTxt->text( String::toStr( (Uint32)mUIBlueSlider->value() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Blue = Col.Blue;
	mMap->BaseColor( MapCol );
}


void TileMapProperties::SaveProperties() {
	mMap->ClearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UIGridCell * Cell = mGenGrid->getCell( i );

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->cell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->cell( 3 ) );

		if ( NULL != Cell && Input->text().size() && Input2->text().size() ) {
			mMap->AddProperty(	Input->text(), Input2->text() );
		}
	}
}

void TileMapProperties::LoadProperties() {
	TileMap::PropertiesMap& Proper = mMap->GetProperties();

	for ( TileMap::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		UIGridCell * Cell = CreateCell();

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->cell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->cell( 3 ) );

		Input->text( it->first );
		Input2->text( it->second );

		mGenGrid->add( Cell );
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
	mGenGrid->add( CreateCell() );

	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->getCell( 0 )->select();
	}
}

void TileMapProperties::RemoveCellClick( const UIEvent * Event ) {
	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND != Index ) {
		mGenGrid->remove( Index );

		if ( Index < mGenGrid->getCount() ) {
			mGenGrid->getCell( Index )->select();
		} else {
			if ( mGenGrid->getCount() ) {
				if ( Index > 0 )
					mGenGrid->getCell( Index - 1 )->select();
			}
		}
	}
}

void TileMapProperties::CreateGridElems() {
	LoadProperties();

	if ( 0 == mGenGrid->getCount() ) {
		AddCellClick( NULL );
	} else {
		mGenGrid->getCell( 0 )->select();
	}
}

UIGridCell * TileMapProperties::CreateCell() {
	UIGridCell::CreateParams CellParams;
	CellParams.setParent( mGenGrid->getContainer() );

	UITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED;
	TxtInputParams.MaxLength = LAYER_NAME_SIZE;

	UIComplexControl::CreateParams CControl;

	UIGridCell * Cell			= eeNew( UIGridCell, ( CellParams ) );
	UITextInput * TxtInput		= eeNew( UITextInput, ( TxtInputParams ) );
	UITextInput * TxtInput2	= eeNew( UITextInput, ( TxtInputParams ) );

	Cell->cell( 0, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->cell( 1, TxtInput );

	Cell->cell( 2, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->cell( 3, TxtInput2 );

	Cell->cell( 4, eeNew( UIComplexControl, ( CControl ) ) );

	return Cell;
}

}}}

