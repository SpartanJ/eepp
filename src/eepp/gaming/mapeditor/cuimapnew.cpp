#include <eepp/gaming/mapeditor/cuimapnew.hpp>
#include <eepp/ui/cuispinbox.hpp>
#include <eepp/ui/cuicheckbox.hpp>
#include <eepp/ui/cuislider.hpp>

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

	mUIWindow	= mTheme->CreateWindow( NULL, eeSize( 320, 360 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cUIMapNew::WindowClose ) );
	mUIWindow->Title( "New Map" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	cUITextBox * Txt = mTheme->CreateTextBox( "Map Size", mUIWindow->Container(), eeSize(), eeVector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "Width:", mUIWindow->Container(), eeSize( 46, 24 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapWidth = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS, 25, false );
	mUIMapWidth->MinValue(1);

	Txt = mTheme->CreateTextBox( "Height:", mUIWindow->Container(), eeSize( 46, 24 ), eeVector2i( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapHeight = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS, 18, false );
	mUIMapHeight->MinValue(1);

	Txt = mTheme->CreateTextBox( "Tile Size", mUIWindow->Container(), eeSize(), eeVector2i( mUIWindow->Container()->Size().Width() / 2, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "Width:", mUIWindow->Container(), eeSize( 46, 24 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapTWidth = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS, 32, false );
	mUIMapTWidth->MinValue(1);

	Txt = mTheme->CreateTextBox( "Height:", mUIWindow->Container(), eeSize( 46, 24 ), eeVector2i( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapTHeight = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS, 32, false );
	mUIMapTHeight->MinValue(1);

	Txt = mTheme->CreateTextBox( "Max Layers", mUIWindow->Container(), eeSize(), eeVector2i( 16, mUIMapTHeight->Pos().y + mUIMapTHeight->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIMapMaxLayers = mTheme->CreateSpinBox( mUIWindow->Container(), eeSize( 53, 24 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS, 8, false );

	Txt = mTheme->CreateTextBox( "Map Flags:", mUIWindow->Container(), eeSize(), eeVector2i( Txt->Pos().x, mUIMapMaxLayers->Pos().y + mUIMapMaxLayers->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUILightsEnabled = mTheme->CreateCheckBox( mUIWindow->Container(), eeSize(), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + Txt->Size().Height() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUILightsEnabled->Text( "Lights Enabled" );

	mUILightsByVertex = mTheme->CreateCheckBox( mUIWindow->Container(), eeSize(), eeVector2i( mUIWindow->Container()->Size().Width() / 2, mUILightsEnabled->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUILightsByVertex->Text( "Lights By Vertex" );

	mUIClampBorders = mTheme->CreateCheckBox( mUIWindow->Container(), eeSize(), eeVector2i( Txt->Pos().x + DistFromTitle, mUILightsEnabled->Pos().y + mUILightsEnabled->Size().Height() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIClampBorders->Text( "Clamp Boders" );
	mUIClampBorders->Active( true );

	mUIClipArea = mTheme->CreateCheckBox( mUIWindow->Container(), eeSize(), eeVector2i( mUIWindow->Container()->Size().Width() / 2, mUIClampBorders->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIClipArea->Text( "Clip View Area" );

	Txt = mTheme->CreateTextBox( "Map Base Color:", mUIWindow->Container(), eeSize(), eeVector2i( Txt->Pos().x, mUIClipArea->Pos().y + mUIClipArea->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	cUIComplexControl::CreateParams ComParams;
	ComParams.Parent( mUIWindow->Container() );
	ComParams.PosSet( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 4 );
	ComParams.SizeSet( 64, 64 );
	ComParams.Background.Color( eeColorA( 255, 255, 255, 255 ) );
	ComParams.Border.Color( eeColorA( 100, 100, 100, 200 ) );
	ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
	mUIBaseColor = eeNew( cUIComplexControl, ( ComParams ) );
	mUIBaseColor->Visible( true );
	mUIBaseColor->Enabled( true );

	Txt = mTheme->CreateTextBox( "Red Color:", mUIWindow->Container(), eeSize(), eeVector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIBaseColor->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIRedSlider = mTheme->CreateSlider( mUIWindow->Container(), eeSize( 128, 20 ), eeVector2i( Txt->Pos().x + Txt->Size().Width() + 16, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIRedSlider->MaxValue( 255 );
	mUIRedSlider->Value( 255 );
	mUIRedSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIMapNew::OnRedChange ) );

	mUIRedTxt = mTheme->CreateTextBox( String::ToStr( 255 ), mUIWindow->Container(), eeSize(), eeVector2i( mUIRedSlider->Pos().x + mUIRedSlider->Size().Width() + 4, mUIRedSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "Green Color:", mUIWindow->Container(), eeSize(), eeVector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIRedSlider->Pos().y + mUIRedSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIGreenSlider = mTheme->CreateSlider( mUIWindow->Container(), eeSize( 128, 20 ), eeVector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIGreenSlider->MaxValue( 255 );
	mUIGreenSlider->Value( 255 );
	mUIGreenSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIMapNew::OnGreenChange ) );

	mUIGreenTxt = mTheme->CreateTextBox( String::ToStr( 255 ), mUIWindow->Container(), eeSize(), eeVector2i( mUIGreenSlider->Pos().x + mUIGreenSlider->Size().Width() + 4, mUIGreenSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "Blue Color:", mUIWindow->Container(), eeSize(), eeVector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIGreenSlider->Pos().y + mUIGreenSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIBlueSlider = mTheme->CreateSlider( mUIWindow->Container(), eeSize( 128, 20 ), eeVector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIBlueSlider->MaxValue( 255 );
	mUIBlueSlider->Value( 255 );
	mUIBlueSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIMapNew::OnBlueChange ) );

	mUIBlueTxt = mTheme->CreateTextBox( String::ToStr( 255 ), mUIWindow->Container(), eeSize(), eeVector2i( mUIBlueSlider->Pos().x + mUIBlueSlider->Size().Width() + 4, mUIBlueSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

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

void cUIMapNew::OnRedChange( const cUIEvent * Event ) {
	eeColorA Col = mUIBaseColor->Background()->Color();
	Col.Red = (Uint8)mUIRedSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIRedTxt->Text( String::ToStr( (Int32)mUIRedSlider->Value() ) );
}

void cUIMapNew::OnGreenChange( const cUIEvent * Event ) {
	eeColorA Col = mUIBaseColor->Background()->Color();
	Col.Green = (Uint8)mUIGreenSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIGreenTxt->Text( String::ToStr( (Uint32)mUIGreenSlider->Value() ) );
}

void cUIMapNew::OnBlueChange( const cUIEvent * Event ) {
	eeColorA Col = mUIBaseColor->Background()->Color();
	Col.Blue = (Uint8)mUIBlueSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIBlueTxt->Text( String::ToStr( (Uint32)mUIBlueSlider->Value() ) );
}

void cUIMapNew::OKClick( const cUIEvent * Event ) {
	Int32 w = static_cast<Int32>( mUIMapWidth->Value() );
	Int32 h = static_cast<Int32>( mUIMapHeight->Value() );
	Int32 tw = static_cast<Int32>( mUIMapTWidth->Value() );
	Int32 th = static_cast<Int32>( mUIMapTHeight->Value() );
	Int32 ml = static_cast<Int32>( mUIMapMaxLayers->Value() );

	Uint32 Flags = MAP_EDITOR_DEFAULT_FLAGS;

	if ( mUILightsEnabled->Active() )
		Flags |= MAP_FLAG_LIGHTS_ENABLED;

	if ( mUILightsByVertex->Active() )
		Flags |= MAP_FLAG_LIGHTS_BYVERTEX;

	if ( mUIClampBorders->Active() )
		Flags |= MAP_FLAG_CLAMP_BORDERS;

	if ( mUIClipArea->Active() )
		Flags |= MAP_FLAG_CLIP_AREA;

	if ( w > 0 && h > 0 && tw > 0 && th > 0 && ml > 0 ) {
		mUIMap->Map()->Create( eeSize( w, h ), ml, eeSize( tw, th ), Flags, mUIMap->Map()->ViewSize() );
		mUIMap->Map()->BaseColor( mUIBaseColor->Background()->Color() );

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
