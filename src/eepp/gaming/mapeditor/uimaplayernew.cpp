#include <eepp/gaming/mapeditor/uimaplayernew.hpp>
#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace Gaming { namespace Private {

UIMapLayerNew::UIMapLayerNew( UIMap * Map, EE_LAYER_TYPE Type, NewLayerCb newLayerCb ) :
	mTheme( NULL ),
	mUIMap( Map ),
	mType( Type ),
	mNewLayerCb( newLayerCb ),
	mUIWindow( NULL ),
	mLayer( NULL )
{
	mTheme		= UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= UIWindow::New();
	mUIWindow->setSizeWithDecoration( 278, 114 )->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )->setMinWindowSize( 278, 114 );

	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIMapLayerNew::onWindowClose ) );

	if ( MAP_LAYER_TILED == mType )
		mUIWindow->setTitle( "New Tile Layer" );
	else if ( MAP_LAYER_OBJECT == mType )
		mUIWindow->setTitle( "New Object Layer" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = UITextBox::New();
	Txt->setFlags( UI_DRAW_SHADOW | UI_AUTO_SIZE )->setParent( mUIWindow->getContainer() )->setPosition( 16, InitialY );
	Txt->setText( "Layer Name" );

	mUILayerName = UITextInput::New()->setMaxLength( 64 );
	mUILayerName->setParent( mUIWindow->getContainer() )->setSize( 120, 0 )->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );
	mUILayerName->setText( "Layer " + String::toStr( mUIMap->Map()->getLayerCount() + 1 ) );

	UIPushButton * OKButton = UIPushButton::New();
	OKButton->setParent(  mUIWindow->getContainer() )->setSize( 80, 0 );
	OKButton->setIcon( mTheme->getIconByName( "add" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapLayerNew::onOKClick ) );
	mUILayerName->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UIMapLayerNew::onOKClick ) );

	OKButton->setText( "Add" );

	UIPushButton * CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )->setSize( OKButton->getSize() )->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y );
	CancelButton->setIcon( mTheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapLayerNew::onCancelClick ) );
	CancelButton->setText( "Cancel" );

	mUIWindow->addEventListener( UIEvent::EventKeyUp, cb::Make1( this, &UIMapLayerNew::onOnKeyUp ) );

	mUIWindow->center();
	mUIWindow->show();

	mUILayerName->setFocus();
}

UIMapLayerNew::~UIMapLayerNew() {

}

void UIMapLayerNew::onOnKeyUp( const UIEvent * Event ) {
	const UIEventKey * KeyEvent = reinterpret_cast<const UIEventKey*> ( Event );

	if ( KeyEvent->getKeyCode() == KEY_ESCAPE ) {
		onCancelClick( Event );
	}
}

void UIMapLayerNew::onOKClick( const UIEvent * Event ) {
	if ( mUILayerName->getText().size() ) {
		mLayer = mUIMap->Map()->addLayer( mType, LAYER_FLAG_VISIBLE | LAYER_FLAG_LIGHTS_ENABLED, mUILayerName->getText() );

		if ( mNewLayerCb.IsSet() )
			mNewLayerCb( this );
	}

	mUIWindow->closeWindow();
}

void UIMapLayerNew::onCancelClick( const UIEvent * Event ) {
	mUIWindow->closeWindow();
}

void UIMapLayerNew::onWindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

const EE_LAYER_TYPE& UIMapLayerNew::getType() const {
	return mType;
}

UITextInput * UIMapLayerNew::getUILayerName() const {
	return mUILayerName;
}

const String& UIMapLayerNew::getName() const {
	return mUILayerName->getText();
}

MapLayer * UIMapLayerNew::getLayer() const {
	return mLayer;
}

}}}
