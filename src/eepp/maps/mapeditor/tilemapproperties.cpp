#include <eepp/maps/mapeditor/tilemapproperties.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace Maps { namespace Private {

static UITextView* createTextBox( const String& Text = "", Node* Parent = NULL,
								  const Sizef& Size = Sizef(), const Vector2f& Pos = Vector2f(),
								  const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE,
								  const Uint32& fontStyle = Text::Regular ) {
	UITextView* Ctrl = UITextView::New();
	Ctrl->setFontStyle( fontStyle );
	Ctrl->resetFlags( Flags )
		->setParent( Parent )
		->setSize( Size )
		->setVisible( true )
		->setEnabled( false )
		->setPosition( Pos );
	Ctrl->setText( Text );
	return Ctrl;
}

TileMapProperties::TileMapProperties( TileMap* Map ) :
	mUITheme( NULL ), mUIWindow( NULL ), mGenGrid( NULL ), mMap( Map ) {
	if ( NULL == mMap ) {
		eeDelete( this );
		return;
	}

	if ( SceneManager::instance()->getUISceneNode() == NULL )
		return;

	mUITheme = SceneManager::instance()->getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow = UIWindow::New();
	mUIWindow->setSizeWithDecoration( 500, 500 )
		->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )
		->setMinWindowSize( 500, 500 );

	mUIWindow->addEventListener( Event::OnWindowClose,
								 cb::Make1( this, &TileMapProperties::onWindowClose ) );
	mUIWindow->setTitle( "Map Properties" );

	Uint32 DiffIfLights = 0;

	if ( mMap->getLightsEnabled() ) {
		DiffIfLights = 100;

		UITextView* Txt = createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizef(),
										 Vector2f( 50, 16 ),
										 UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

		mUIBaseColor = UIWidget::New();
		mUIBaseColor->setFlags( UI_FILL_BACKGROUND | UI_BORDER );
		mUIBaseColor->setParent( mUIWindow->getContainer() );
		mUIBaseColor->setPosition( Txt->getPosition().x,
								   Txt->getPosition().y + Txt->getSize().getHeight() + 4 );
		mUIBaseColor->setSize( 64, 64 );
		mUIBaseColor->setBackgroundColor( mMap->getBaseColor() );
		mUIBaseColor->setBorderColor( Color( 100, 100, 100, 200 ) );

		Txt = createTextBox(
			"Red Color:", mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4,
					  mUIBaseColor->getPosition().y ),
			UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
		mUIRedSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
		mUIRedSlider->setParent( mUIWindow->getContainer() )
			->setSize( 255, 20 )
			->setPosition( Txt->getPosition().x + Txt->getSize().getWidth() + 16,
						   Txt->getPosition().y );
		mUIRedSlider->setMaxValue( 255 );
		mUIRedSlider->setValue( mMap->getBaseColor().r );
		mUIRedSlider->addEventListener( Event::OnValueChange,
										cb::Make1( this, &TileMapProperties::onRedChange ) );

		mUIRedTxt = createTextBox(
			String::toStr( (Uint32)mMap->getBaseColor().r ), mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4,
					  mUIRedSlider->getPosition().y ),
			UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

		Txt = createTextBox(
			"Green Color:", mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4,
					  mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ),
			UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

		mUIGreenSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
		mUIGreenSlider->setParent( mUIWindow->getContainer() )
			->setSize( 255, 20 )
			->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
		mUIGreenSlider->setMaxValue( 255 );
		mUIGreenSlider->setValue( mMap->getBaseColor().g );
		mUIGreenSlider->addEventListener( Event::OnValueChange,
										  cb::Make1( this, &TileMapProperties::onGreenChange ) );

		mUIGreenTxt = createTextBox(
			String::toStr( (Uint32)mMap->getBaseColor().g ), mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4,
					  mUIGreenSlider->getPosition().y ),
			UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

		Txt = createTextBox(
			"Blue Color:", mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4,
					  mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ),
			UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
		mUIBlueSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
		mUIBlueSlider->setParent( mUIWindow->getContainer() )
			->setSize( 255, 20 )
			->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
		mUIBlueSlider->setMaxValue( 255 );
		mUIBlueSlider->setValue( mMap->getBaseColor().b );
		mUIBlueSlider->addEventListener( Event::OnValueChange,
										 cb::Make1( this, &TileMapProperties::onBlueChange ) );

		mUIBlueTxt = createTextBox(
			String::toStr( (Uint32)mMap->getBaseColor().b ), mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4,
					  mUIBlueSlider->getPosition().y ),
			UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
	}

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	UITextView* TxtBox =
		createTextBox( "Property Name", mUIWindow->getContainer(), Sizef( 192, 24 ),
					   Vector2f( 50, 10 + DiffIfLights ), TxtBoxFlags, Text::Shadow );
	createTextBox( "Property Value", mUIWindow->getContainer(), Sizef( 192, 24 ),
				   Vector2f( 50 + 192, TxtBox->getPosition().y ), TxtBoxFlags );

	UIPushButton* OKButton = UIPushButton::New();
	OKButton->setSize( 80, 0 )->setParent( mUIWindow->getContainer() );
	OKButton->setIcon( mUITheme->getIconByName( "ok" ) );
	OKButton->setPosition(
		mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4,
		mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( Event::MouseClick,
								cb::Make1( this, &TileMapProperties::onOKClick ) );
	OKButton->setText( "OK" );
	OKButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	UIPushButton* CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )
		->setSize( OKButton->getSize() )
		->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4,
					   OKButton->getPosition().y );
	CancelButton->setIcon( mUITheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( Event::MouseClick,
									cb::Make1( this, &TileMapProperties::onCancelClick ) );
	CancelButton->setText( "Cancel" );
	CancelButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	mGenGrid = UITable::New();
	mGenGrid->setParent( mUIWindow->getContainer() );
	mGenGrid->setSize( 400, 310 )
		->setPosition( 50, TxtBox->getPosition().y + TxtBox->getSize().getHeight() );
	mGenGrid->setRowHeight( 24 )->setCollumnsCount( 5 );
	mGenGrid->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mGenGrid->setCollumnWidth( 0, 10 );
	mGenGrid->setCollumnWidth( 1, 175 );
	mGenGrid->setCollumnWidth( 2, 10 );
	mGenGrid->setCollumnWidth( 3, 175 );
	mGenGrid->setCollumnWidth( 4, 10 );

	Vector2f Pos( mGenGrid->getPosition().x + mGenGrid->getSize().getWidth() + 10,
				  mGenGrid->getPosition().y );

	UIPushButton* AddButton = UIPushButton::New();
	AddButton->setSize( 24, 0 )->setParent( mUIWindow->getContainer() )->setPosition( Pos );
	AddButton->setIcon( mUITheme->getIconByName( "add" ) );
	AddButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	AddButton->addEventListener( Event::MouseClick,
								 cb::Make1( this, &TileMapProperties::onAddCellClick ) );

	if ( NULL == AddButton->getIcon()->getDrawable() )
		AddButton->setText( "+" );

	Pos.y += AddButton->getSize().getHeight() + 5;

	UIPushButton* RemoveButton = UIPushButton::New();
	RemoveButton->setSize( 24, 0 )->setParent( mUIWindow->getContainer() )->setPosition( Pos );
	RemoveButton->setIcon( mUITheme->getIconByName( "remove" ) );
	RemoveButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	RemoveButton->addEventListener( Event::MouseClick,
									cb::Make1( this, &TileMapProperties::onRemoveCellClick ) );

	if ( NULL == RemoveButton->getIcon()->getDrawable() )
		RemoveButton->setText( "-" );

	createGridElems();

	mUIWindow->center();
	mUIWindow->show();
}

TileMapProperties::~TileMapProperties() {}

void TileMapProperties::onRedChange( const Event* ) {
	Color Col = mUIBaseColor->getBackgroundColor();
	Col.r = (Uint8)mUIRedSlider->getValue();
	mUIBaseColor->setBackgroundColor( Col );
	mUIRedTxt->setText( String::toStr( (Int32)mUIRedSlider->getValue() ) );

	Color MapCol = mMap->getBaseColor();
	MapCol.r = Col.r;
	mMap->setBaseColor( MapCol );
}

void TileMapProperties::onGreenChange( const Event* ) {
	Color Col = mUIBaseColor->getBackgroundColor();
	Col.g = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->setBackgroundColor( Col );
	mUIGreenTxt->setText( String::toStr( (Uint32)mUIGreenSlider->getValue() ) );

	Color MapCol = mMap->getBaseColor();
	MapCol.g = Col.g;
	mMap->setBaseColor( MapCol );
}

void TileMapProperties::onBlueChange( const Event* ) {
	Color Col = mUIBaseColor->getBackgroundColor();
	Col.b = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->setBackgroundColor( Col );
	mUIBlueTxt->setText( String::toStr( (Uint32)mUIBlueSlider->getValue() ) );

	Color MapCol = mMap->getBaseColor();
	MapCol.b = Col.b;
	mMap->setBaseColor( MapCol );
}

void TileMapProperties::saveProperties() {
	mMap->clearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UITableCell* Cell = mGenGrid->getCell( i );

		UITextInput* Input = Cell->getCell( 1 )->asType<UITextInput>();
		UITextInput* Input2 = Cell->getCell( 3 )->asType<UITextInput>();

		if ( NULL != Cell && Input->getText().size() && Input2->getText().size() ) {
			mMap->addProperty( Input->getText(), Input2->getText() );
		}
	}
}

void TileMapProperties::loadProperties() {
	TileMap::PropertiesMap& Proper = mMap->getProperties();

	for ( TileMap::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); ++it ) {
		UITableCell* Cell = createCell();

		UITextInput* Input = Cell->getCell( 1 )->asType<UITextInput>();
		UITextInput* Input2 = Cell->getCell( 3 )->asType<UITextInput>();

		Input->setText( it->first );
		Input2->setText( it->second );

		mGenGrid->add( Cell );
	}
}

void TileMapProperties::onOKClick( const Event* ) {
	saveProperties();

	mUIWindow->closeWindow();
}

void TileMapProperties::onCancelClick( const Event* ) {
	mUIWindow->closeWindow();
}

void TileMapProperties::onWindowClose( const Event* ) {
	eeDelete( this );
}

void TileMapProperties::onAddCellClick( const Event* ) {
	mGenGrid->add( createCell() );

	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->getCell( 0 )->select();
	}
}

void TileMapProperties::onRemoveCellClick( const Event* ) {
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

UITableCell* TileMapProperties::createCell() {
	UITableCell* Cell = UITableCell::New();
	UITextInput* TxtInput = UITextInput::New();
	UITextInput* TxtInput2 = UITextInput::New();

	Cell->setParent( mGenGrid->getContainer() );
	TxtInput->setMaxLength( LAYER_NAME_SIZE );
	TxtInput2->setMaxLength( LAYER_NAME_SIZE );

	Cell->setCell( 0, UIWidget::New() );
	Cell->setCell( 1, TxtInput );
	Cell->setCell( 2, UIWidget::New() );
	Cell->setCell( 3, TxtInput2 );
	Cell->setCell( 4, UIWidget::New() );

	return Cell;
}

}}} // namespace EE::Maps::Private
