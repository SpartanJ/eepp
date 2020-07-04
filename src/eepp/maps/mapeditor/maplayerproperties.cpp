#include <eepp/maps/mapeditor/mapeditor.hpp>
#include <eepp/maps/mapeditor/maplayerproperties.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace Maps { namespace Private {

MapLayerProperties::MapLayerProperties( MapLayer* Map, RefreshLayerListCb Cb ) :
	mUITheme( NULL ), mUIWindow( NULL ), mGenGrid( NULL ), mLayer( Map ), mRefreshCb( Cb ) {
	if ( NULL == mLayer ) {
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
								 cb::Make1( this, &MapLayerProperties::onWindowClose ) );
	mUIWindow->setTitle( "Layer Properties" );

	Int32 InitialY = 16;
	Int32 DistFromTitle = 18;

	UITextView* Txt = UITextView::New();
	Txt->setFontStyle( Text::Shadow )
		->setFlags( UI_AUTO_SIZE )
		->setParent( mUIWindow->getContainer() )
		->setPosition( 50, InitialY );
	Txt->setText( "Layer name:" );

	mUIInput = UITextInput::New()->setMaxLength( 64 );
	mUIInput->setSize( 120, 0 )
		->setParent( mUIWindow->getContainer() )
		->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );
	mUIInput->setText( mLayer->getName() );
	mUIInput->addEventListener( Event::OnPressEnter,
								cb::Make1( this, &MapLayerProperties::onOKClick ) );

	UITextView* TxtBox = UITextView::New();
	TxtBox->setFontStyle( Text::Shadow )
		->setHorizontalAlign( UI_HALIGN_CENTER )
		->setPosition( 50, mUIInput->getPosition().y + mUIInput->getSize().getHeight() + 12 )
		->setSize( 192, 24 )
		->setParent( mUIWindow->getContainer() );
	TxtBox->setText( "Property Name" );

	TxtBox = UITextView::New();
	TxtBox->setFontStyle( Text::Shadow )
		->setHorizontalAlign( UI_HALIGN_CENTER )
		->setPosition( 50 + 192, mUIInput->getPosition().y + mUIInput->getSize().getHeight() + 12 )
		->setParent( mUIWindow->getContainer() )
		->setSize( 192, 24 );
	TxtBox->setText( "Property Value" );

	UIPushButton* OKButton = UIPushButton::New();
	OKButton->setSize( 80, 0 )->setParent( mUIWindow->getContainer() );
	OKButton->setIcon( sceneNode->findIcon( "ok" ) );
	OKButton->setPosition(
		mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4,
		mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( Event::MouseClick,
								cb::Make1( this, &MapLayerProperties::onOKClick ) );
	OKButton->setText( "OK" );
	OKButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	UIPushButton* CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )
		->setSize( OKButton->getSize() )
		->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4,
					   OKButton->getPosition().y );
	CancelButton->setIcon( sceneNode->findIcon( "cancel" ) );
	CancelButton->addEventListener( Event::MouseClick,
									cb::Make1( this, &MapLayerProperties::onCancelClick ) );
	CancelButton->setText( "Cancel" );
	CancelButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

	mGenGrid = UITable::New();
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
	AddButton->setIcon( sceneNode->findIcon( "add" ) );
	AddButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	AddButton->addEventListener( Event::MouseClick,
								 cb::Make1( this, &MapLayerProperties::onAddCellClick ) );

	if ( NULL == AddButton->getIcon()->getDrawable() )
		AddButton->setText( "+" );

	Pos.y += AddButton->getSize().getHeight() + 5;

	UIPushButton* RemoveButton = UIPushButton::New();
	RemoveButton->setParent( mUIWindow->getContainer() )->setSize( 24, 0 )->setPosition( Pos );
	RemoveButton->setIcon( sceneNode->findIcon( "remove" ) );
	RemoveButton->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	RemoveButton->addEventListener( Event::MouseClick,
									cb::Make1( this, &MapLayerProperties::onRemoveCellClick ) );

	if ( NULL == RemoveButton->getIcon()->getDrawable() )
		RemoveButton->setText( "-" );

	createGridElems();

	mUIWindow->center();
	mUIWindow->show();
}

MapLayerProperties::~MapLayerProperties() {}

void MapLayerProperties::saveProperties() {
	mLayer->clearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UITableCell* Cell = mGenGrid->getCell( i );

		UITextInput* Input = Cell->getCell( 1 )->asType<UITextInput>();
		UITextInput* Input2 = Cell->getCell( 3 )->asType<UITextInput>();

		if ( NULL != Cell && Input->getText().size() && Input2->getText().size() ) {
			mLayer->addProperty( Input->getText(), Input2->getText() );
		}
	}
}

void MapLayerProperties::loadProperties() {
	MapLayer::PropertiesMap& Proper = mLayer->getProperties();

	for ( MapLayer::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); ++it ) {
		UITableCell* Cell = createCell();

		UITextInput* Input = Cell->getCell( 1 )->asType<UITextInput>();
		UITextInput* Input2 = Cell->getCell( 3 )->asType<UITextInput>();

		Input->setText( it->first );
		Input2->setText( it->second );

		mGenGrid->add( Cell );
	}
}

void MapLayerProperties::onOKClick( const Event* ) {
	saveProperties();

	mLayer->setName( mUIInput->getText().toUtf8() );

	if ( mRefreshCb ) {
		mRefreshCb();
	}

	mUIWindow->closeWindow();
}

void MapLayerProperties::onCancelClick( const Event* ) {
	mUIWindow->closeWindow();
}

void MapLayerProperties::onWindowClose( const Event* ) {
	eeDelete( this );
}

void MapLayerProperties::onAddCellClick( const Event* ) {
	mGenGrid->add( createCell() );

	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->getCell( 0 )->select();
	}
}

void MapLayerProperties::onRemoveCellClick( const Event* ) {
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

UITableCell* MapLayerProperties::createCell() {
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
