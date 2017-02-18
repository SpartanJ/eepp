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

	mUITheme		= UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->createWindow( NULL, Sizei( 500, 500 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 500, 500 ) );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &TileMapProperties::WindowClose ) );
	mUIWindow->setTitle( "Map Properties" );


	Uint32 DiffIfLights = 0;

	if ( mMap->LightsEnabled() ) {
		DiffIfLights = 100;

		UITextBox * Txt = mUITheme->createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizei(), Vector2i( 50, 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		UIComplexControl::CreateParams ComParams;
		ComParams.setParent( mUIWindow->getContainer() );
		ComParams.setPosition( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 4 );
		ComParams.setSize( 64, 64 );
		ComParams.Background.setColor( mMap->BaseColor() );
		ComParams.Border.setColor( ColorA( 100, 100, 100, 200 ) );
		ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
		mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
		mUIBaseColor->setVisible( true );
		mUIBaseColor->setEnabled( true );

		Txt = mUITheme->createTextBox( "Red Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIBaseColor->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIRedSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( Txt->getPosition().x + Txt->getSize().getWidth() + 16, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIRedSlider->setMaxValue( 255 );
		mUIRedSlider->setValue( mMap->BaseColor().r() );
		mUIRedSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnRedChange ) );

		mUIRedTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->BaseColor().r() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4, mUIRedSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->createTextBox( "Green Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIGreenSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( mUIRedSlider->getPosition().x, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIGreenSlider->setMaxValue( 255 );
		mUIGreenSlider->setValue( mMap->BaseColor().g() );
		mUIGreenSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnGreenChange ) );

		mUIGreenTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->BaseColor().g() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->createTextBox( "Blue Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIBlueSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( mUIRedSlider->getPosition().x, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIBlueSlider->setMaxValue( 255 );
		mUIBlueSlider->setValue( mMap->BaseColor().b() );
		mUIBlueSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::OnBlueChange ) );

		mUIBlueTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->BaseColor().b() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4, mUIBlueSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	}

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	UITextBox * TxtBox = mUITheme->createTextBox( "Property Name", mUIWindow->getContainer(), Sizei(192, 24), Vector2i( 50, 10 + DiffIfLights ), TxtBoxFlags );
	mUITheme->createTextBox( "Property Value", mUIWindow->getContainer(), Sizei(192, 24), Vector2i(50+192, TxtBox->getPosition().y ), TxtBoxFlags );

	UIPushButton * OKButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->getIconByName( "ok" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::OKClick ) );

	OKButton->setText( "OK" );

	UIPushButton * CancelButton = mUITheme->createPushButton( mUIWindow->getContainer(), OKButton->getSize(), Vector2i( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::CancelClick ) );
	CancelButton->setText( "Cancel" );

	UIGenericGrid::CreateParams GridParams;
	GridParams.setParent( mUIWindow->getContainer() );
	GridParams.setPosition( 50, TxtBox->getPosition().y + 20 );
	GridParams.setSize( 400, 400 - DiffIfLights );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 5;
	mGenGrid = eeNew( UIGenericGrid, ( GridParams ) );
	mGenGrid->setVisible( true );
	mGenGrid->setEnabled( true );
	mGenGrid->setCollumnWidth( 0, 10 );
	mGenGrid->setCollumnWidth( 1, 175 );
	mGenGrid->setCollumnWidth( 2, 10 );
	mGenGrid->setCollumnWidth( 3, 175 );
	mGenGrid->setCollumnWidth( 4, 10 );

	Vector2i Pos( mGenGrid->getPosition().x + mGenGrid->getSize().getWidth() + 10, mGenGrid->getPosition().y );

	UIPushButton * AddButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->getIconByName( "add" ) );
	AddButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::AddCellClick ) );

	if ( NULL == AddButton->getIcon()->getSubTexture() )
		AddButton->setText( "+" );

	Pos.y += AddButton->getSize().getHeight() + 5;

	UIPushButton * RemoveButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->getIconByName( "remove" )  );
	RemoveButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->getIcon()->getSubTexture() )
		RemoveButton->setText( "-" );

	CreateGridElems();

	mUIWindow->center();
	mUIWindow->show();
}

TileMapProperties::~TileMapProperties() {
}

void TileMapProperties::OnRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Red = (Uint8)mUIRedSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIRedTxt->setText( String::toStr( (Int32)mUIRedSlider->getValue() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Red = Col.Red;
	mMap->BaseColor( MapCol );
}

void TileMapProperties::OnGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Green = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIGreenTxt->setText( String::toStr( (Uint32)mUIGreenSlider->getValue() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Green = Col.Green;
	mMap->BaseColor( MapCol );
}

void TileMapProperties::OnBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Blue = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIBlueTxt->setText( String::toStr( (Uint32)mUIBlueSlider->getValue() ) );

	ColorA MapCol = mMap->BaseColor();
	MapCol.Blue = Col.Blue;
	mMap->BaseColor( MapCol );
}


void TileMapProperties::SaveProperties() {
	mMap->ClearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UIGridCell * Cell = mGenGrid->getCell( i );

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->getCell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->getCell( 3 ) );

		if ( NULL != Cell && Input->getText().size() && Input2->getText().size() ) {
			mMap->AddProperty(	Input->getText(), Input2->getText() );
		}
	}
}

void TileMapProperties::LoadProperties() {
	TileMap::PropertiesMap& Proper = mMap->GetProperties();

	for ( TileMap::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		UIGridCell * Cell = CreateCell();

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->getCell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->getCell( 3 ) );

		Input->setText( it->first );
		Input2->setText( it->second );

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

	Cell->setCell( 0, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->setCell( 1, TxtInput );

	Cell->setCell( 2, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->setCell( 3, TxtInput2 );

	Cell->setCell( 4, eeNew( UIComplexControl, ( CControl ) ) );

	return Cell;
}

}}}

