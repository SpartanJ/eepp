#include <eepp/gaming/mapeditor/cuilayernew.hpp>
#include <eepp/ui/cuitextinput.hpp>

namespace EE { namespace Gaming { namespace MapEditor {

cUILayerNew::cUILayerNew( cUIMap * Map, EE_LAYER_TYPE Type, NewLayerCb newLayerCb ) :
	mTheme( NULL ),
	mUIMap( Map ),
	mType( Type ),
	mNewLayerCb( newLayerCb ),
	mUIWindow( NULL ),
	mLayer( NULL )
{
	mTheme		= cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->CreateWindow( NULL, Sizei( 278, 114 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 278, 114 ) );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cUILayerNew::WindowClose ) );

	if ( MAP_LAYER_TILED == mType )
		mUIWindow->Title( "New Tile Layer" );
	else if ( MAP_LAYER_OBJECT == mType )
		mUIWindow->Title( "New Object Layer" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	cUITextBox * Txt = mTheme->CreateTextBox( "Layer Name", mUIWindow->Container(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUILayerName = mTheme->CreateTextInput( mUIWindow->Container(), Sizei( 120, 22 ), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUILayerName->Text( "Layer " + String::ToStr( mUIMap->Map()->LayerCount() + 1 ) );

	cUIPushButton * OKButton = mTheme->CreatePushButton( mUIWindow->Container(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "add" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUILayerNew::OKClick ) );
	mUILayerName->AddEventListener( cUIEvent::EventOnPressEnter, cb::Make1( this, &cUILayerNew::OKClick ) );

	OKButton->Text( "Add" );

	cUIPushButton * CancelButton = mTheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), Vector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUILayerNew::CancelClick ) );
	CancelButton->Text( "Cancel" );

	mUIWindow->AddEventListener( cUIEvent::EventKeyUp, cb::Make1( this, &cUILayerNew::OnKeyUp ) );

	mUIWindow->Center();
	mUIWindow->Show();

	mUILayerName->SetFocus();
}

cUILayerNew::~cUILayerNew() {

}

void cUILayerNew::OnKeyUp( const cUIEvent * Event ) {
	const cUIEventKey * KeyEvent = reinterpret_cast<const cUIEventKey*> ( Event );

	if ( KeyEvent->KeyCode() == KEY_ESCAPE ) {
		CancelClick( Event );
	}
}

void cUILayerNew::OKClick( const cUIEvent * Event ) {
	if ( mUILayerName->Text().size() ) {
		mLayer = mUIMap->Map()->AddLayer( mType, LAYER_FLAG_VISIBLE | LAYER_FLAG_LIGHTS_ENABLED, mUILayerName->Text() );

		if ( mNewLayerCb.IsSet() )
			mNewLayerCb( this );
	}

	mUIWindow->CloseWindow();
}

void cUILayerNew::CancelClick( const cUIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void cUILayerNew::WindowClose( const cUIEvent * Event ) {
	eeDelete( this );
}

const EE_LAYER_TYPE& cUILayerNew::Type() const {
	return mType;
}

cUITextInput * cUILayerNew::UILayerName() const {
	return mUILayerName;
}

const String& cUILayerNew::Name() const {
	return mUILayerName->Text();
}

cLayer * cUILayerNew::Layer() const {
	return mLayer;
}

}}}
