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

	mUIWindow	= UIWindow::New();
	mUIWindow->setSizeWithDecoration( 500, 500 )->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )->setMinWindowSize( 500, 500 );

	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &TileMapProperties::onWindowClose ) );
	mUIWindow->setTitle( "Map Properties" );


	Uint32 DiffIfLights = 0;

	if ( mMap->getLightsEnabled() ) {
		DiffIfLights = 100;

		UITextBox * Txt = mUITheme->createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizei(), Vector2i( 50, 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		UIComplexControl::CreateParams ComParams;
		ComParams.setParent( mUIWindow->getContainer() );
		ComParams.setPosition( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 4 );
		ComParams.setSize( 64, 64 );
		ComParams.Background.setColor( mMap->getBaseColor() );
		ComParams.Border.setColor( ColorA( 100, 100, 100, 200 ) );
		ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
		mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
		mUIBaseColor->setVisible( true );
		mUIBaseColor->setEnabled( true );

		Txt = mUITheme->createTextBox( "Red Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIBaseColor->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIRedSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( Txt->getPosition().x + Txt->getSize().getWidth() + 16, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIRedSlider->setMaxValue( 255 );
		mUIRedSlider->setValue( mMap->getBaseColor().r() );
		mUIRedSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::onRedChange ) );

		mUIRedTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->getBaseColor().r() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4, mUIRedSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->createTextBox( "Green Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIGreenSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( mUIRedSlider->getPosition().x, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIGreenSlider->setMaxValue( 255 );
		mUIGreenSlider->setValue( mMap->getBaseColor().g() );
		mUIGreenSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::onGreenChange ) );

		mUIGreenTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->getBaseColor().g() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

		Txt = mUITheme->createTextBox( "Blue Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
		mUIBlueSlider = mUITheme->createSlider( mUIWindow->getContainer(), Sizei( 255, 20 ), Vector2i( mUIRedSlider->getPosition().x, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
		mUIBlueSlider->setMaxValue( 255 );
		mUIBlueSlider->setValue( mMap->getBaseColor().b() );
		mUIBlueSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TileMapProperties::onBlueChange ) );

		mUIBlueTxt = mUITheme->createTextBox( String::toStr( (Uint32)mMap->getBaseColor().b() ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4, mUIBlueSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	}

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	UITextBox * TxtBox = mUITheme->createTextBox( "Property Name", mUIWindow->getContainer(), Sizei(192, 24), Vector2i( 50, 10 + DiffIfLights ), TxtBoxFlags );
	mUITheme->createTextBox( "Property Value", mUIWindow->getContainer(), Sizei(192, 24), Vector2i(50+192, TxtBox->getPosition().y ), TxtBoxFlags );

	UIPushButton * OKButton = UIPushButton::New();
	OKButton->setParent(  mUIWindow->getContainer() )->setSize( 80, 0 );
	OKButton->setIcon( mUITheme->getIconByName( "ok" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::onOKClick ) );

	OKButton->setText( "OK" );

	UIPushButton * CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )->setSize( OKButton->getSize() )->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y );
	CancelButton->setIcon( mUITheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::onCancelClick ) );
	CancelButton->setText( "Cancel" );

/*	UIGenericGrid::CreateParams GridParams;
	GridParams.setParent( mUIWindow->getContainer() );
	GridParams.setPosition( 50, TxtBox->getPosition().y + 20 );
	GridParams.setSize( 400, 400 - DiffIfLights );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 5;
	mGenGrid = eeNew( UIGenericGrid, ( GridParams ) );*/
	mGenGrid = UIGenericGrid::New();
	mGenGrid->setParent( mUIWindow->getContainer() );
	mGenGrid->setSize( 400, 350 );
	mGenGrid->setPosition( 50, TxtBox->getPosition().y + TxtBox->getSize().getHeight() );
	mGenGrid->setRowHeight( 24 );
	mGenGrid->setCollumnsCount( 5 );
	mGenGrid->setVisible( true );
	mGenGrid->setEnabled( true );
	mGenGrid->setCollumnWidth( 0, 10 );
	mGenGrid->setCollumnWidth( 1, 175 );
	mGenGrid->setCollumnWidth( 2, 10 );
	mGenGrid->setCollumnWidth( 3, 175 );
	mGenGrid->setCollumnWidth( 4, 10 );

	Vector2i Pos( mGenGrid->getPosition().x + mGenGrid->getSize().getWidth() + 10, mGenGrid->getPosition().y );

	UIPushButton * AddButton = UIPushButton::New();
	AddButton->setParent( mUIWindow->getContainer() )->setSize( 24, 0 )->setPosition( Pos );
	AddButton->setIcon( mUITheme->getIconByName( "add" ) );
	AddButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	AddButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::onAddCellClick ) );

	if ( NULL == AddButton->getIcon()->getSubTexture() )
		AddButton->setText( "+" );

	Pos.y += AddButton->getSize().getHeight() + 5;

	UIPushButton * RemoveButton = UIPushButton::New();
	RemoveButton->setParent( mUIWindow->getContainer() )->setSize( 24, 0 )->setPosition( Pos );
	RemoveButton->setIcon( mUITheme->getIconByName( "remove" ) );
	RemoveButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	RemoveButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TileMapProperties::onRemoveCellClick ) );

	if ( NULL == RemoveButton->getIcon()->getSubTexture() )
		RemoveButton->setText( "-" );

	createGridElems();

	mUIWindow->center();
	mUIWindow->show();
}

TileMapProperties::~TileMapProperties() {
}

void TileMapProperties::onRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Red = (Uint8)mUIRedSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIRedTxt->setText( String::toStr( (Int32)mUIRedSlider->getValue() ) );

	ColorA MapCol = mMap->getBaseColor();
	MapCol.Red = Col.Red;
	mMap->setBaseColor( MapCol );
}

void TileMapProperties::onGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Green = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIGreenTxt->setText( String::toStr( (Uint32)mUIGreenSlider->getValue() ) );

	ColorA MapCol = mMap->getBaseColor();
	MapCol.Green = Col.Green;
	mMap->setBaseColor( MapCol );
}

void TileMapProperties::onBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Blue = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIBlueTxt->setText( String::toStr( (Uint32)mUIBlueSlider->getValue() ) );

	ColorA MapCol = mMap->getBaseColor();
	MapCol.Blue = Col.Blue;
	mMap->setBaseColor( MapCol );
}


void TileMapProperties::saveProperties() {
	mMap->clearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UIGridCell * Cell = mGenGrid->getCell( i );

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->getCell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->getCell( 3 ) );

		if ( NULL != Cell && Input->getText().size() && Input2->getText().size() ) {
			mMap->addProperty(	Input->getText(), Input2->getText() );
		}
	}
}

void TileMapProperties::loadProperties() {
	TileMap::PropertiesMap& Proper = mMap->getProperties();

	for ( TileMap::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		UIGridCell * Cell = createCell();

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->getCell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->getCell( 3 ) );

		Input->setText( it->first );
		Input2->setText( it->second );

		mGenGrid->add( Cell );
	}
}

void TileMapProperties::onOKClick( const UIEvent * Event ) {
	saveProperties();

	mUIWindow->closeWindow();
}

void TileMapProperties::onCancelClick( const UIEvent * Event ) {
	mUIWindow->closeWindow();
}

void TileMapProperties::onWindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

void TileMapProperties::onAddCellClick( const UIEvent * Event ) {
	mGenGrid->add( createCell() );

	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->getCell( 0 )->select();
	}
}

void TileMapProperties::onRemoveCellClick( const UIEvent * Event ) {
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

void TileMapProperties::createGridElems() {
	loadProperties();

	if ( 0 == mGenGrid->getCount() ) {
		onAddCellClick( NULL );
	} else {
		mGenGrid->getCell( 0 )->select();
	}
}

UIGridCell * TileMapProperties::createCell() {
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

