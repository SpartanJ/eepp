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

	mUITheme		= UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= UIWindow::New();
	mUIWindow->setSizeWithDecoration( 500, 500 )->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )->setMinWindowSize( 500, 500 );

	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &MapLayerProperties::onWindowClose ) );
	mUIWindow->setTitle( "Layer Properties" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mUITheme->createTextBox( "Layer name:", mUIWindow->getContainer(), Sizei(), Vector2i( 50, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput = mUITheme->createTextInput( mUIWindow->getContainer(), Sizei( 120, 22 ), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput->setText( mLayer->getName() );
	mUIInput->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &MapLayerProperties::onOKClick ) );

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	mUITheme->createTextBox( "Property Name", mUIWindow->getContainer(), Sizei(192, 24), Vector2i( 50, mUIInput->getPosition().y + mUIInput->getSize().getHeight() + 12 ), TxtBoxFlags );
	UITextBox * TxtBox = mUITheme->createTextBox( "Property Value", mUIWindow->getContainer(), Sizei(192, 24), Vector2i( 50+192, mUIInput->getPosition().y + mUIInput->getSize().getHeight() + 12 ), TxtBoxFlags );

	UIPushButton * OKButton = UIPushButton::New();
	OKButton->setParent(  mUIWindow->getContainer() )->setSize( 80, 0 );
	OKButton->setIcon( mUITheme->getIconByName( "ok" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::onOKClick ) );

	OKButton->setText( "OK" );

	UIPushButton * CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )->setSize( OKButton->getSize() )->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y );
	CancelButton->setIcon( mUITheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::onCancelClick ) );
	CancelButton->setText( "Cancel" );

/*	UIGenericGrid::CreateParams GridParams;
	GridParams.setParent( mUIWindow->getContainer() );
	GridParams.setPosition( 50, TxtBox->getPosition().y + TxtBox->getSize().getHeight() );
	GridParams.setSize( 400, 350 );
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
	AddButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::onAddCellClick ) );

	if ( NULL == AddButton->getIcon()->getSubTexture() )
		AddButton->setText( "+" );

	Pos.y += AddButton->getSize().getHeight() + 5;

	UIPushButton * RemoveButton = UIPushButton::New();
	RemoveButton->setParent( mUIWindow->getContainer() )->setSize( 24, 0 )->setPosition( Pos );
	RemoveButton->setIcon( mUITheme->getIconByName( "remove" ) );
	RemoveButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	RemoveButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::onRemoveCellClick ) );

	if ( NULL == RemoveButton->getIcon()->getSubTexture() )
		RemoveButton->setText( "-" );

	createGridElems();

	mUIWindow->center();
	mUIWindow->show();
}

MapLayerProperties::~MapLayerProperties() {
}

void MapLayerProperties::saveProperties() {
	mLayer->clearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UIGridCell * Cell = mGenGrid->getCell( i );

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->getCell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->getCell( 3 ) );

		if ( NULL != Cell && Input->getText().size() && Input2->getText().size() ) {
			mLayer->addProperty(	Input->getText(), Input2->getText() );
		}
	}
}

void MapLayerProperties::loadProperties() {
	MapLayer::PropertiesMap& Proper = mLayer->getProperties();

	for ( MapLayer::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		UIGridCell * Cell = createCell();

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->getCell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->getCell( 3 ) );

		Input->setText( it->first );
		Input2->setText( it->second );

		mGenGrid->add( Cell );
	}
}

void MapLayerProperties::onOKClick( const UIEvent * Event ) {
	saveProperties();

	mLayer->setName( mUIInput->getText().toUtf8() );

	if ( mRefreshCb.IsSet() ) {
		mRefreshCb();
	}

	mUIWindow->closeWindow();
}

void MapLayerProperties::onCancelClick( const UIEvent * Event ) {
	mUIWindow->closeWindow();
}

void MapLayerProperties::onWindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

void MapLayerProperties::onAddCellClick( const UIEvent * Event ) {
	mGenGrid->add( createCell() );

	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->getCell( 0 )->select();
	}
}

void MapLayerProperties::onRemoveCellClick( const UIEvent * Event ) {
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

void MapLayerProperties::createGridElems() {
	loadProperties();

	if ( 0 == mGenGrid->getCount() ) {
		onAddCellClick( NULL );
	} else {
		mGenGrid->getCell( 0 )->select();
	}
}

UIGridCell * MapLayerProperties::createCell() {
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
