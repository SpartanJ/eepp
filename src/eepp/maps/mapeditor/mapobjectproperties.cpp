#include <eepp/maps/mapeditor/mapeditor.hpp>
#include <eepp/maps/mapeditor/mapobjectproperties.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>

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

MapObjectProperties::MapObjectProperties( GameObjectObject* Obj ) :
	mUITheme( NULL ), mUIWindow( NULL ), mGenGrid( NULL ), mObj( Obj ) {
	if ( NULL == mObj ) {
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
		->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )
		->setMinWindowSize( 500, 500 );

	mUIWindow->addEventListener( Event::OnWindowClose,
								 cb::Make1( this, &MapObjectProperties::onWindowClose ) );
	mUIWindow->setTitle( "Object Properties" );

	Int32 InitialY = 16;
	Int32 DistFromTitle = 18;

	UITextView* Txt =
		createTextBox( "Object name:", mUIWindow->getContainer(), Sizef(), Vector2f( 50, InitialY ),
					   UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
	mUIInput = UITextInput::New();
	mUIInput->setParent( mUIWindow->getContainer() )
		->setSize( 120, 0 )
		->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );
	mUIInput->setMaxLength( 64 );
	mUIInput->setText( mObj->getName() );
	mUIInput->addEventListener( Event::OnPressEnter,
								cb::Make1( this, &MapObjectProperties::onOKClick ) );

	UITextView* Txt2 = createTextBox( "Object type:", mUIWindow->getContainer(), Sizef(),
									  Vector2f( 50 + 192, InitialY ),
									  UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
	mUIInput2 = UITextInput::New();
	mUIInput2->setParent( mUIWindow->getContainer() )
		->setSize( 120, 0 )
		->setPosition( Txt2->getPosition().x + DistFromTitle,
					   Txt2->getPosition().y + DistFromTitle );
	mUIInput2->setMaxLength( 64 );
	mUIInput2->setText( mObj->getTypeName() );
	mUIInput2->addEventListener( Event::OnPressEnter,
								 cb::Make1( this, &MapObjectProperties::onOKClick ) );

	Uint32 TxtBoxFlags = UI_NODE_DEFAULT_FLAGS | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	createTextBox( "Property Name", mUIWindow->getContainer(), Sizef( 192, 24 ),
				   Vector2f( 50, mUIInput->getPosition().y + mUIInput->getSize().getHeight() + 12 ),
				   TxtBoxFlags, Text::Shadow );
	UITextView* TxtBox = createTextBox(
		"Property Value", mUIWindow->getContainer(), Sizef( 192, 24 ),
		Vector2f( 50 + 192, mUIInput->getPosition().y + mUIInput->getSize().getHeight() + 12 ),
		TxtBoxFlags, Text::Shadow );

	UIPushButton* OKButton = UIPushButton::New();
	OKButton->setParent( mUIWindow->getContainer() )->setSize( 80, 0 );
	OKButton->setIcon( sceneNode->findIconDrawable( "ok", PixelDensity::dpToPxI( 16 ) ) );
	OKButton->setPosition(
		mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4,
		mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( Event::MouseClick,
								cb::Make1( this, &MapObjectProperties::onOKClick ) );
	OKButton->setText( "OK" );
	OKButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	UIPushButton* CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )
		->setSize( OKButton->getSize() )
		->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4,
					   OKButton->getPosition().y );
	CancelButton->setIcon( sceneNode->findIconDrawable( "cancel", PixelDensity::dpToPxI( 16 ) ) );
	CancelButton->addEventListener( Event::MouseClick,
									cb::Make1( this, &MapObjectProperties::onCancelClick ) );
	CancelButton->setText( "Cancel" );
	CancelButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	mGenGrid = UIWidgetTable::New();
	mGenGrid->setParent( mUIWindow->getContainer() );
	mGenGrid->setSize( 400, 340 )
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
	AddButton->setParent( mUIWindow->getContainer() )->setSize( 24, 0 )->setPosition( Pos );
	AddButton->setIcon( sceneNode->findIconDrawable( "add", PixelDensity::dpToPxI( 16 ) ) );
	AddButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	AddButton->addEventListener( Event::MouseClick,
								 cb::Make1( this, &MapObjectProperties::onAddCellClick ) );

	if ( NULL == AddButton->getIcon()->getDrawable() )
		AddButton->setText( "+" );

	Pos.y += AddButton->getSize().getHeight() + 5;

	UIPushButton* RemoveButton = UIPushButton::New();
	RemoveButton->setParent( mUIWindow->getContainer() )->setSize( 24, 0 )->setPosition( Pos );
	RemoveButton->setIcon( sceneNode->findIconDrawable( "remove", PixelDensity::dpToPxI( 16 ) ) );
	RemoveButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	RemoveButton->addEventListener( Event::MouseClick,
									cb::Make1( this, &MapObjectProperties::onRemoveCellClick ) );

	if ( NULL == RemoveButton->getIcon()->getDrawable() )
		RemoveButton->setText( "-" );

	createGridElems();

	mUIWindow->center();
	mUIWindow->show();
}

MapObjectProperties::~MapObjectProperties() {}

void MapObjectProperties::saveProperties() {
	mObj->clearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UIWidgetTableRow* Cell = mGenGrid->getRow( i );

		UITextInput* Input = Cell->getColumn( 1 )->asType<UITextInput>();
		UITextInput* Input2 = Cell->getColumn( 3 )->asType<UITextInput>();

		if ( NULL != Cell && Input->getText().size() && Input2->getText().size() ) {
			mObj->addProperty( Input->getText(), Input2->getText() );
		}
	}
}

void MapObjectProperties::loadProperties() {
	GameObjectObject::PropertiesMap& Proper = mObj->getProperties();

	for ( GameObjectObject::PropertiesMap::iterator it = Proper.begin(); it != Proper.end();
		  ++it ) {
		UIWidgetTableRow* Cell = createCell();

		UITextInput* Input = Cell->getColumn( 1 )->asType<UITextInput>();
		UITextInput* Input2 = Cell->getColumn( 3 )->asType<UITextInput>();

		Input->setText( it->first );
		Input2->setText( it->second );

		mGenGrid->add( Cell );
	}
}

void MapObjectProperties::onOKClick( const Event* Event ) {
	saveProperties();

	mObj->setName( mUIInput->getText().toUtf8() );
	mObj->setTypeName( mUIInput2->getText().toUtf8() );

	mUIWindow->closeWindow();
}

void MapObjectProperties::onCancelClick( const Event* Event ) {
	mUIWindow->closeWindow();
}

void MapObjectProperties::onWindowClose( const Event* Event ) {
	eeDelete( this );
}

void MapObjectProperties::onAddCellClick( const Event* Event ) {
	mGenGrid->add( createCell() );

	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->getRow( 0 )->select();
	}
}

void MapObjectProperties::onRemoveCellClick( const Event* Event ) {
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

void MapObjectProperties::createGridElems() {
	loadProperties();

	if ( 0 == mGenGrid->getCount() ) {
		onAddCellClick( NULL );
	} else {
		mGenGrid->getRow( 0 )->select();
	}
}

UIWidgetTableRow* MapObjectProperties::createCell() {
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
