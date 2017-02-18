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
	mTheme		= UIThemeManager::instance()->defaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->createWindow( NULL, Sizei( 278, 114 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 278, 114 ) );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIMapLayerNew::WindowClose ) );

	if ( MAP_LAYER_TILED == mType )
		mUIWindow->setTitle( "New Tile Layer" );
	else if ( MAP_LAYER_OBJECT == mType )
		mUIWindow->setTitle( "New Object Layer" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mTheme->createTextBox( "Layer Name", mUIWindow->getContainer(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUILayerName = mTheme->createTextInput( mUIWindow->getContainer(), Sizei( 120, 22 ), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUILayerName->setText( "Layer " + String::toStr( mUIMap->Map()->LayerCount() + 1 ) );

	UIPushButton * OKButton = mTheme->createPushButton( mUIWindow->getContainer(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->getIconByName( "add" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapLayerNew::OKClick ) );
	mUILayerName->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UIMapLayerNew::OKClick ) );

	OKButton->text( "Add" );

	UIPushButton * CancelButton = mTheme->createPushButton( mUIWindow->getContainer(), OKButton->getSize(), Vector2i( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapLayerNew::CancelClick ) );
	CancelButton->text( "Cancel" );

	mUIWindow->addEventListener( UIEvent::EventKeyUp, cb::Make1( this, &UIMapLayerNew::OnKeyUp ) );

	mUIWindow->center();
	mUIWindow->show();

	mUILayerName->setFocus();
}

UIMapLayerNew::~UIMapLayerNew() {

}

void UIMapLayerNew::OnKeyUp( const UIEvent * Event ) {
	const UIEventKey * KeyEvent = reinterpret_cast<const UIEventKey*> ( Event );

	if ( KeyEvent->getKeyCode() == KEY_ESCAPE ) {
		CancelClick( Event );
	}
}

void UIMapLayerNew::OKClick( const UIEvent * Event ) {
	if ( mUILayerName->getText().size() ) {
		mLayer = mUIMap->Map()->AddLayer( mType, LAYER_FLAG_VISIBLE | LAYER_FLAG_LIGHTS_ENABLED, mUILayerName->getText() );

		if ( mNewLayerCb.IsSet() )
			mNewLayerCb( this );
	}

	mUIWindow->CloseWindow();
}

void UIMapLayerNew::CancelClick( const UIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void UIMapLayerNew::WindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

const EE_LAYER_TYPE& UIMapLayerNew::Type() const {
	return mType;
}

UITextInput * UIMapLayerNew::UILayerName() const {
	return mUILayerName;
}

const String& UIMapLayerNew::Name() const {
	return mUILayerName->getText();
}

MapLayer * UIMapLayerNew::Layer() const {
	return mLayer;
}

}}}
