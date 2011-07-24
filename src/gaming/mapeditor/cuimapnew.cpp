#include "cuimapnew.hpp"
#include "../../ui/cuispinbox.hpp"

namespace EE { namespace Gaming { namespace MapEditor {

cUIMapNew::cUIMapNew( cUIMap * Map, cb::Callback0<void> NewMapCb ) :
	mTheme( NULL ),
	mUIWindow( NULL ),
	mUIMap( Map ),
	mNewMapCb( NewMapCb )
{
	mTheme		= cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->CreateWindow( NULL, eeSize( 278, 214 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cUIMapNew::WindowClose ) );
	mUIWindow->Title( "New Map" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	cUITextBox * Txt = mTheme->CreateTextBox( mUIWindow->Container(), eeSize(), eeVector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	Txt->Text( "Map size" );

	Txt = mTheme->CreateTextBox( mUIWindow->Container(), eeSize( 46, 24 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );
	Txt->Text( "Width:" );

	mUIMapWidth = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS, 25, false );
	mUIMapWidth->MinValue(1);

	Txt = mTheme->CreateTextBox( mUIWindow->Container(), eeSize( 46, 24 ), eeVector2i( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );
	Txt->Text( "Height:" );

	mUIMapHeight = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS, 18, false );
	mUIMapHeight->MinValue(1);

	Txt = mTheme->CreateTextBox( mUIWindow->Container(), eeSize(), eeVector2i( mUIWindow->Container()->Size().Width() / 2, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	Txt->Text( "Tile size" );

	Txt = mTheme->CreateTextBox( mUIWindow->Container(), eeSize( 46, 24 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );
	Txt->Text( "Width:" );

	mUIMapTWidth = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS, 32, false );
	mUIMapTWidth->MinValue(1);

	Txt = mTheme->CreateTextBox( mUIWindow->Container(), eeSize( 46, 24 ), eeVector2i( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );
	Txt->Text( "Height:" );

	mUIMapTHeight = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS, 32, false );
	mUIMapTHeight->MinValue(1);

	Txt = mTheme->CreateTextBox( mUIWindow->Container(), eeSize(), eeVector2i( 16, mUIWindow->Container()->Size().Height() / 2 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	Txt->Text( "Max layers" );

	mUIMapMaxLayers = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS, 8, false );

	cUIPushButton * OKButton = mTheme->CreatePushButton( mUIWindow->Container(), eeSize( 80, 22 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUIMapNew::OKClick ) );
	OKButton->Text( "OK" );

	cUIPushButton * CancelButton = mTheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), eeVector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUIMapNew::CancelClick ) );
	CancelButton->Text( "Cancel" );

	mUIWindow->Center();
	mUIWindow->Show();
}

cUIMapNew::~cUIMapNew() {
}

void cUIMapNew::OKClick( const cUIEvent * Event ) {
	Int32 w = static_cast<Int32>( mUIMapWidth->Value() );
	Int32 h = static_cast<Int32>( mUIMapHeight->Value() );
	Int32 tw = static_cast<Int32>( mUIMapTWidth->Value() );
	Int32 th = static_cast<Int32>( mUIMapTHeight->Value() );
	Int32 ml = static_cast<Int32>( mUIMapMaxLayers->Value() );

	if ( w > 0 && h > 0 && tw > 0 && th > 0 && ml > 0 ) {
		mUIMap->Map()->Create( eeSize( w, h ), ml, eeSize( tw, th ), MAP_FLAG_CLAMP_BODERS | MAP_FLAG_CLIP_AREA | MAP_FLAG_DRAW_GRID | MAP_FLAG_DRAW_BACKGROUND, mUIMap->Map()->ViewSize() );

		if ( mNewMapCb.IsSet() )
			mNewMapCb();
	}

	mUIWindow->CloseWindow();
}

void cUIMapNew::CancelClick( const cUIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void cUIMapNew::WindowClose( const cUIEvent * Event ) {
	eeDelete( this );
}

}}}
