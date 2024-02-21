#include <eepp/maps/mapeditor/tilemapproperties.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace Maps { namespace Private {

static UITextView* createTextBox( const String& Text = "", Node* Parent = NULL,
								  const Sizef& Size = Sizef(), const Vector2f& Pos = Vector2f(),
								  const Uint32& Flags = UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE,
								  const Uint32& fontStyle = Text::Regular ) {
	UITextView* widget = UITextView::New();
	widget->setFontStyle( fontStyle );
	widget->resetFlags( Flags )
		->setParent( Parent )
		->setSize( Size )
		->setVisible( true )
		->setEnabled( false )
		->setPosition( Pos );
	widget->setText( Text );
	return widget;
}

TileMapProperties::TileMapProperties( TileMap* Map ) :
	mUITheme( NULL ), mUIWindow( NULL ), mGenGrid( NULL ), mMap( Map ) {
	if ( NULL == mMap ) {
		eeDelete( this );
		return;
	}

	if ( SceneManager::instance()->getUISceneNode() == NULL )
		return;

	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	mUITheme = sceneNode->getUIThemeManager()->getDefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow = UIWindow::New();
	mUIWindow->setSizeWithDecoration( 500, 500 )
		->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )
		->setMinWindowSize( 500, 500 );

	mUIWindow->addEventListener( Event::OnWindowClose,
								 [this] ( auto event ) { onWindowClose( event ); } );
	mUIWindow->setTitle( "Map Properties" );

	Uint32 DiffIfLights = 0;

	if ( mMap->getLightsEnabled() ) {
		DiffIfLights = 100;

		UITextView* Txt =
			createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizef(),
						   Vector2f( 50, 16 ), UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

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
			UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
		mUIRedSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
		mUIRedSlider->setParent( mUIWindow->getContainer() )
			->setSize( 255, 20 )
			->setPosition( Txt->getPosition().x + Txt->getSize().getWidth() + 16,
						   Txt->getPosition().y );
		mUIRedSlider->setMaxValue( 255 );
		mUIRedSlider->setValue( mMap->getBaseColor().r );
		mUIRedSlider->addEventListener( Event::OnValueChange,
										[this] ( auto event ) { onRedChange( event ); } );

		mUIRedTxt = createTextBox(
			String::toString( (Uint32)mMap->getBaseColor().r ), mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4,
					  mUIRedSlider->getPosition().y ),
			UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

		Txt = createTextBox(
			"Green Color:", mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4,
					  mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ),
			UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

		mUIGreenSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
		mUIGreenSlider->setParent( mUIWindow->getContainer() )
			->setSize( 255, 20 )
			->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
		mUIGreenSlider->setMaxValue( 255 );
		mUIGreenSlider->setValue( mMap->getBaseColor().g );
		mUIGreenSlider->addEventListener( Event::OnValueChange,
										  [this] ( auto event ) { onGreenChange( event ); } );

		mUIGreenTxt = createTextBox(
			String::toString( (Uint32)mMap->getBaseColor().g ), mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4,
					  mUIGreenSlider->getPosition().y ),
			UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

		Txt = createTextBox(
			"Blue Color:", mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4,
					  mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ),
			UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
		mUIBlueSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
		mUIBlueSlider->setParent( mUIWindow->getContainer() )
			->setSize( 255, 20 )
			->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
		mUIBlueSlider->setMaxValue( 255 );
		mUIBlueSlider->setValue( mMap->getBaseColor().b );
		mUIBlueSlider->addEventListener( Event::OnValueChange,
										 [this] ( auto event ) { onBlueChange( event ); } );

		mUIBlueTxt = createTextBox(
			String::toString( (Uint32)mMap->getBaseColor().b ), mUIWindow->getContainer(), Sizef(),
			Vector2f( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4,
					  mUIBlueSlider->getPosition().y ),
			UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
	}

	Uint32 TxtBoxFlags = UI_NODE_DEFAULT_FLAGS | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	UITextView* TxtBox =
		createTextBox( "Property Name", mUIWindow->getContainer(), Sizef( 192, 24 ),
					   Vector2f( 50, 10 + DiffIfLights ), TxtBoxFlags, Text::Shadow );
	createTextBox( "Property Value", mUIWindow->getContainer(), Sizef( 192, 24 ),
				   Vector2f( 50 + 192, TxtBox->getPosition().y ), TxtBoxFlags );

	UIPushButton* OKButton = UIPushButton::New();
	OKButton->setSize( 80, 0 )->setParent( mUIWindow->getContainer() );
	OKButton->setIcon( sceneNode->findIconDrawable( "ok", PixelDensity::dpToPxI( 16 ) ) );
	OKButton->setPosition(
		mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4,
		mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( Event::MouseClick,
								[this] ( auto event ) { onOKClick( event ); } );
	OKButton->setText( "OK" );
	OKButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	UIPushButton* CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )
		->setSize( OKButton->getSize() )
		->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4,
					   OKButton->getPosition().y );
	CancelButton->setIcon( sceneNode->findIconDrawable( "cancel", PixelDensity::dpToPxI( 16 ) ) );
	CancelButton->addEventListener( Event::MouseClick,
									[this] ( auto event ) { onCancelClick( event ); } );
	CancelButton->setText( "Cancel" );
	CancelButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	mGenGrid = UIWidgetTable::New();
	mGenGrid->setParent( mUIWindow->getContainer() );
	mGenGrid->setSize( 400, 310 )
		->setPosition( 50, TxtBox->getPosition().y + TxtBox->getSize().getHeight() );
	mGenGrid->setRowHeight( 24 )->setColumnsCount( 5 );
	mGenGrid->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mGenGrid->setColumnWidth( 0, 10 );
	mGenGrid->setColumnWidth( 1, 175 );
	mGenGrid->setColumnWidth( 2, 10 );
	mGenGrid->setColumnWidth( 3, 175 );
	mGenGrid->setColumnWidth( 4, 10 );

	Vector2f Pos( mGenGrid->getPosition().x + mGenGrid->getSize().getWidth() + 10,
				  mGenGrid->getPosition().y );

	UIPushButton* AddButton = UIPushButton::New();
	AddButton->setSize( 24, 0 )->setParent( mUIWindow->getContainer() )->setPosition( Pos );
	AddButton->setIcon( sceneNode->findIconDrawable( "add", PixelDensity::dpToPxI( 16 ) ) );
	AddButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	AddButton->addEventListener( Event::MouseClick,
								 [this] ( auto event ) { onAddCellClick( event ); } );

	if ( NULL == AddButton->getIcon()->getDrawable() )
		AddButton->setText( "+" );

	Pos.y += AddButton->getSize().getHeight() + 5;

	UIPushButton* RemoveButton = UIPushButton::New();
	RemoveButton->setSize( 24, 0 )->setParent( mUIWindow->getContainer() )->setPosition( Pos );
	RemoveButton->setIcon( sceneNode->findIconDrawable( "remove", PixelDensity::dpToPxI( 16 ) ) );
	RemoveButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	RemoveButton->addEventListener( Event::MouseClick,
									[this] ( auto event ) { onRemoveCellClick( event ); } );

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
	mUIRedTxt->setText( String::toString( (Int32)mUIRedSlider->getValue() ) );

	Color MapCol = mMap->getBaseColor();
	MapCol.r = Col.r;
	mMap->setBaseColor( MapCol );
}

void TileMapProperties::onGreenChange( const Event* ) {
	Color Col = mUIBaseColor->getBackgroundColor();
	Col.g = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->setBackgroundColor( Col );
	mUIGreenTxt->setText( String::toString( (Uint32)mUIGreenSlider->getValue() ) );

	Color MapCol = mMap->getBaseColor();
	MapCol.g = Col.g;
	mMap->setBaseColor( MapCol );
}

void TileMapProperties::onBlueChange( const Event* ) {
	Color Col = mUIBaseColor->getBackgroundColor();
	Col.b = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->setBackgroundColor( Col );
	mUIBlueTxt->setText( String::toString( (Uint32)mUIBlueSlider->getValue() ) );

	Color MapCol = mMap->getBaseColor();
	MapCol.b = Col.b;
	mMap->setBaseColor( MapCol );
}

void TileMapProperties::saveProperties() {
	mMap->clearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UIWidgetTableRow* Cell = mGenGrid->getRow( i );

		UITextInput* Input = Cell->getColumn( 1 )->asType<UITextInput>();
		UITextInput* Input2 = Cell->getColumn( 3 )->asType<UITextInput>();

		if ( NULL != Cell && Input->getText().size() && Input2->getText().size() ) {
			mMap->addProperty( Input->getText(), Input2->getText() );
		}
	}
}

void TileMapProperties::loadProperties() {
	TileMap::PropertiesMap& Proper = mMap->getProperties();

	for ( TileMap::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); ++it ) {
		UIWidgetTableRow* Cell = createCell();

		UITextInput* Input = Cell->getColumn( 1 )->asType<UITextInput>();
		UITextInput* Input2 = Cell->getColumn( 3 )->asType<UITextInput>();

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
		mGenGrid->getRow( 0 )->select();
	}
}

void TileMapProperties::onRemoveCellClick( const Event* ) {
	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND != Index ) {
		mGenGrid->remove( Index );

		if ( Index < mGenGrid->getCount() ) {
			mGenGrid->getRow( Index )->select();
		} else {
			if ( mGenGrid->getCount() ) {
				if ( Index > 0 )
					mGenGrid->getRow( Index - 1 )->select();
			}
		}
	}
}

void TileMapProperties::createGridElems() {
	loadProperties();

	if ( 0 == mGenGrid->getCount() ) {
		onAddCellClick( NULL );
	} else {
		mGenGrid->getRow( 0 )->select();
	}
}

UIWidgetTableRow* TileMapProperties::createCell() {
	UIWidgetTableRow* Cell = UIWidgetTableRow::New();
	UITextInput* TxtInput = UITextInput::New();
	UITextInput* TxtInput2 = UITextInput::New();

	Cell->setParent( mGenGrid->getContainer() );
	TxtInput->setMaxLength( LAYER_NAME_SIZE );
	TxtInput2->setMaxLength( LAYER_NAME_SIZE );

	Cell->setColumn( 0, UIWidget::New() );
	Cell->setColumn( 1, TxtInput );
	Cell->setColumn( 2, UIWidget::New() );
	Cell->setColumn( 3, TxtInput2 );
	Cell->setColumn( 4, UIWidget::New() );

	return Cell;
}

}}} // namespace EE::Maps::Private
