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
	mTheme		= UIThemeManager::instance()->DefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->CreateWindow( NULL, Sizei( 278, 114 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 278, 114 ) );
	mUIWindow->AddEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIMapLayerNew::WindowClose ) );

	if ( MAP_LAYER_TILED == mType )
		mUIWindow->Title( "New Tile Layer" );
	else if ( MAP_LAYER_OBJECT == mType )
		mUIWindow->Title( "New Object Layer" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mTheme->CreateTextBox( "Layer Name", mUIWindow->Container(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUILayerName = mTheme->CreateTextInput( mUIWindow->Container(), Sizei( 120, 22 ), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUILayerName->Text( "Layer " + String::ToStr( mUIMap->Map()->LayerCount() + 1 ) );

	UIPushButton * OKButton = mTheme->CreatePushButton( mUIWindow->Container(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "add" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapLayerNew::OKClick ) );
	mUILayerName->AddEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UIMapLayerNew::OKClick ) );

	OKButton->Text( "Add" );

	UIPushButton * CancelButton = mTheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), Vector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapLayerNew::CancelClick ) );
	CancelButton->Text( "Cancel" );

	mUIWindow->AddEventListener( UIEvent::EventKeyUp, cb::Make1( this, &UIMapLayerNew::OnKeyUp ) );

	mUIWindow->Center();
	mUIWindow->Show();

	mUILayerName->SetFocus();
}

UIMapLayerNew::~UIMapLayerNew() {

}

void UIMapLayerNew::OnKeyUp( const UIEvent * Event ) {
	const UIEventKey * KeyEvent = reinterpret_cast<const UIEventKey*> ( Event );

	if ( KeyEvent->KeyCode() == KEY_ESCAPE ) {
		CancelClick( Event );
	}
}

void UIMapLayerNew::OKClick( const UIEvent * Event ) {
	if ( mUILayerName->Text().size() ) {
		mLayer = mUIMap->Map()->AddLayer( mType, LAYER_FLAG_VISIBLE | LAYER_FLAG_LIGHTS_ENABLED, mUILayerName->Text() );

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
	return mUILayerName->Text();
}

MapLayer * UIMapLayerNew::Layer() const {
	return mLayer;
}

}}}
