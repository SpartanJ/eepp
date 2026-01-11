#include <eepp/maps/mapeditor/uimaplayernew.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace Maps { namespace Private {

UIMapLayerNew::UIMapLayerNew( UIMap* Map, EE_LAYER_TYPE Type, NewLayerCb newLayerCb ) :
	mTheme( NULL ),
	mUIMap( Map ),
	mType( Type ),
	mNewLayerCb( newLayerCb ),
	mUIWindow( NULL ),
	mLayer( NULL ) {
	if ( SceneManager::instance()->getUISceneNode() == NULL )
		return;

	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	mTheme = sceneNode->getUIThemeManager()->getDefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow = UIWindow::New();
	mUIWindow->setSizeWithDecoration( 278, 114 )
		->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )
		->setMinWindowSize( 278, 114 );

	mUIWindow->on( Event::OnWindowClose, [this]( auto event ) { onWindowClose( event ); } );

	if ( MAP_LAYER_TILED == mType )
		mUIWindow->setTitle( "New Tile Layer" );
	else if ( MAP_LAYER_OBJECT == mType )
		mUIWindow->setTitle( "New Object Layer" );

	Int32 InitialY = 16;
	Int32 DistFromTitle = 18;

	UITextView* Txt = UITextView::New();
	Txt->setFontStyle( Text::Shadow )
		->setFlags( UI_AUTO_SIZE )
		->setParent( mUIWindow->getContainer() )
		->setPosition( 16, InitialY );
	Txt->setText( "Layer Name" );

	mUILayerName = UITextInput::New()->setMaxLength( 64 );
	mUILayerName->setSize( 120, 0 )
		->setParent( mUIWindow->getContainer() )
		->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );
	mUILayerName->setText( "Layer " + String::toString( mUIMap->Map()->getLayerCount() + 1 ) );

	UIPushButton* OKButton = UIPushButton::New();
	OKButton->setSize( 80, 0 )->setParent( mUIWindow->getContainer() );
	OKButton->setIcon( sceneNode->findIconDrawable( "add", PixelDensity::dpToPxI( 16 ) ) );
	OKButton->setPosition(
		mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4,
		mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->on( Event::MouseClick, [this]( auto event ) { onOKClick( event ); } );
	mUILayerName->on( Event::OnPressEnter, [this]( auto event ) { onOKClick( event ); } );

	OKButton->setText( "Add" );

	UIPushButton* CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )
		->setSize( OKButton->getSize() )
		->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4,
					   OKButton->getPosition().y );
	CancelButton->setIcon( sceneNode->findIconDrawable( "cancel", PixelDensity::dpToPxI( 16 ) ) );
	CancelButton->on( Event::MouseClick, [this]( auto event ) { onCancelClick( event ); } );
	CancelButton->setText( "Cancel" );

	mUIWindow->on( Event::KeyUp, [this]( auto event ) { onOnKeyUp( event ); } );

	mUIWindow->center();
	mUIWindow->show();

	mUILayerName->setFocus();
}

UIMapLayerNew::~UIMapLayerNew() {}

void UIMapLayerNew::onOnKeyUp( const Event* event ) {
	const KeyEvent* keyEvent = reinterpret_cast<const KeyEvent*>( event );

	if ( keyEvent->getKeyCode() == KEY_ESCAPE ) {
		onCancelClick( event );
	}
}

void UIMapLayerNew::onOKClick( const Event* ) {
	if ( mUILayerName->getText().size() ) {
		mLayer = mUIMap->Map()->addLayer( mType, LAYER_FLAG_VISIBLE | LAYER_FLAG_LIGHTS_ENABLED,
										  mUILayerName->getText() );

		if ( mNewLayerCb )
			mNewLayerCb( this );
	}

	mUIWindow->closeWindow();
}

void UIMapLayerNew::onCancelClick( const Event* ) {
	mUIWindow->closeWindow();
}

void UIMapLayerNew::onWindowClose( const Event* ) {
	eeDelete( this );
}

const EE_LAYER_TYPE& UIMapLayerNew::getType() const {
	return mType;
}

UITextInput* UIMapLayerNew::getUILayerName() const {
	return mUILayerName;
}

const String& UIMapLayerNew::getName() const {
	return mUILayerName->getText();
}

MapLayer* UIMapLayerNew::getLayer() const {
	return mLayer;
}

}}} // namespace EE::Maps::Private
