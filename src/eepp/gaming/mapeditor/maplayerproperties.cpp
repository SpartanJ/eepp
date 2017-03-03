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

	UITextBox * Txt = UITextBox::New();
	Txt->setFlags( UI_DRAW_SHADOW | UI_AUTO_SIZE )->setParent( mUIWindow->getContainer() )->setPosition( 50, InitialY );
	Txt->setText( "Layer name:" );

	mUIInput = UITextInput::New()->setMaxLength( 64 );
	mUIInput->setParent( mUIWindow->getContainer() )->setSize( 120, 0 )->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );
	mUIInput->setText( mLayer->getName() );
	mUIInput->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &MapLayerProperties::onOKClick ) );

	UITextBox * TxtBox = UITextBox::New();
	TxtBox->setParent( mUIWindow->getContainer() )->setSize( 192, 24 )->setHorizontalAlign( UI_HALIGN_CENTER )->setFlags( UI_DRAW_SHADOW )
		  ->setPosition( 50, mUIInput->getPosition().y + mUIInput->getSize().getHeight() + 12 );
	TxtBox->setText( "Property Name" );

	TxtBox = UITextBox::New();
	TxtBox->setParent( mUIWindow->getContainer() )->setSize( 192, 24 )->setHorizontalAlign( UI_HALIGN_CENTER )->setFlags( UI_DRAW_SHADOW )
		  ->setPosition( 50+192, mUIInput->getPosition().y + mUIInput->getSize().getHeight() + 12 );
	TxtBox->setText( "Property Value" );

	UIPushButton * OKButton = UIPushButton::New();
	OKButton->setParent(  mUIWindow->getContainer() )->setSize( 80, 0 );
	OKButton->setIcon( mUITheme->getIconByName( "ok" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::onOKClick ) );
	OKButton->setText( "OK" );
	OKButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	UIPushButton * CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )->setSize( OKButton->getSize() )->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y );
	CancelButton->setIcon( mUITheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapLayerProperties::onCancelClick ) );
	CancelButton->setText( "Cancel" );
	CancelButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	mGenGrid = UIGenericGrid::New();
	mGenGrid->setParent( mUIWindow->getContainer() );
	mGenGrid->setSize( 400, 340 )->setPosition( 50, TxtBox->getPosition().y + TxtBox->getSize().getHeight() );
	mGenGrid->setRowHeight( 24 )->setCollumnsCount( 5 );
	mGenGrid->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
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
	UIGridCell * Cell = UIGridCell::New();
	UITextInput * TxtInput = UITextInput::New();
	UITextInput * TxtInput2 = UITextInput::New();

	Cell->setParent( mGenGrid->getContainer() );
	TxtInput->setMaxLength( LAYER_NAME_SIZE );
	TxtInput2->setMaxLength( LAYER_NAME_SIZE );

	Cell->setCell( 0, UIComplexControl::New() );
	Cell->setCell( 1, TxtInput );
	Cell->setCell( 2, UIComplexControl::New() );
	Cell->setCell( 3, TxtInput2 );
	Cell->setCell( 4, UIComplexControl::New() );

	return Cell;
}

}}}
