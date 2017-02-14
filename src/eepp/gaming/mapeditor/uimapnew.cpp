#include <eepp/gaming/mapeditor/uimapnew.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uislider.hpp>

namespace EE { namespace Gaming { namespace Private {

UIMapNew::UIMapNew( UIMap * Map, cb::Callback0<void> NewMapCb, bool ResizeMap ) :
	mTheme( NULL ),
	mUIWindow( NULL ),
	mUIMap( Map ),
	mNewMapCb( NewMapCb ),
	mResizeMap( ResizeMap )
{
	mTheme		= UIThemeManager::instance()->DefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->CreateWindow( NULL, Sizei( 320, 380 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_SHARE_ALPHA_WITH_CHILDS | UI_WIN_MODAL, Sizei( 320, 380 ) );
	mUIWindow->AddEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIMapNew::WindowClose ) );

	if ( !mResizeMap ) {
		mUIWindow->Title( "New Map" );
	} else {
		mUIWindow->Title( "Resize Map" );
	}

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mTheme->CreateTextBox( "Map Size", mUIWindow->Container(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "Width:", mUIWindow->Container(), Sizei( 46, 24 ), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapWidth = mTheme->CreateSpinBox( mUIWindow->Container(), Sizei( 53, 24 ), Vector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 100, false );
	mUIMapWidth->MinValue(1);

	if ( ResizeMap ) {
		mUIMapWidth->Value( mUIMap->Map()->Size().Width() );
	}

	Txt = mTheme->CreateTextBox( "Height:", mUIWindow->Container(), Sizei( 46, 24 ), Vector2i( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapHeight = mTheme->CreateSpinBox( mUIWindow->Container(), Sizei( 53, 24 ), Vector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 100, false );
	mUIMapHeight->MinValue(1);

	if ( ResizeMap ) {
		mUIMapHeight->Value( mUIMap->Map()->Size().Height() );
	}

	Txt = mTheme->CreateTextBox( "Tile Size", mUIWindow->Container(), Sizei(), Vector2i( mUIWindow->Container()->Size().Width() / 2, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->CreateTextBox( "Width:", mUIWindow->Container(), Sizei( 46, 24 ), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapTWidth = mTheme->CreateSpinBox( mUIWindow->Container(), Sizei( 53, 24 ), Vector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 32, false );
	mUIMapTWidth->MinValue(1);

	if ( ResizeMap ) {
		mUIMapTWidth->Value( mUIMap->Map()->TileSize().Width() );
	}

	Txt = mTheme->CreateTextBox( "Height:", mUIWindow->Container(), Sizei( 46, 24 ), Vector2i( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapTHeight = mTheme->CreateSpinBox( mUIWindow->Container(), Sizei( 53, 24 ), Vector2i( Txt->Pos().x + Txt->Size().Width(), Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 32, false );
	mUIMapTHeight->MinValue(1);

	if ( ResizeMap ) {
		mUIMapTHeight->Value( mUIMap->Map()->TileSize().Height() );
	}

	Txt = mTheme->CreateTextBox( "Max Layers", mUIWindow->Container(), Sizei(), Vector2i( 16, mUIMapTHeight->Pos().y + mUIMapTHeight->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIMapMaxLayers = mTheme->CreateSpinBox( mUIWindow->Container(), Sizei( 53, 24 ), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 8, false );
	mUIMapMaxLayers->MaxValue( 32 );

	Txt = mTheme->CreateTextBox( "Map Flags:", mUIWindow->Container(), Sizei(), Vector2i( Txt->Pos().x, mUIMapMaxLayers->Pos().y + mUIMapMaxLayers->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUILightsEnabled = mTheme->CreateCheckBox( mUIWindow->Container(), Sizei(), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + Txt->Size().Height() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUILightsEnabled->Text( "Lights Enabled" );
	mUILightsEnabled->Active( true );

	if ( ResizeMap ) {
		mUILightsEnabled->Active( 0 != mUIMap->Map()->LightsEnabled() );
	}

	mUILightsByVertex = mTheme->CreateCheckBox( mUIWindow->Container(), Sizei(), Vector2i( mUIWindow->Container()->Size().Width() / 2, mUILightsEnabled->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUILightsByVertex->Text( "Lights By Vertex" );
	mUILightsByVertex->Active( true );

	if ( ResizeMap ) {
		mUILightsByVertex->Active( 0 != mUIMap->Map()->LightsByVertex() );
	}

	mUIClampBorders = mTheme->CreateCheckBox( mUIWindow->Container(), Sizei(), Vector2i( Txt->Pos().x + DistFromTitle, mUILightsEnabled->Pos().y + mUILightsEnabled->Size().Height() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIClampBorders->Text( "Clamp Borders" );
	mUIClampBorders->Active( true );

	if ( ResizeMap ) {
		mUIClampBorders->Active( 0 != mUIMap->Map()->ClampBorders() );
	}

	mUIClipArea = mTheme->CreateCheckBox( mUIWindow->Container(), Sizei(), Vector2i( mUIWindow->Container()->Size().Width() / 2, mUIClampBorders->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIClipArea->Text( "Clip View Area" );
	mUIClipArea->Active( true );

	if ( ResizeMap ) {
		mUIClipArea->Active( 0 != mUIMap->Map()->ClipedArea() );
	}

	Txt = mTheme->CreateTextBox( "Map Base Color:", mUIWindow->Container(), Sizei(), Vector2i( Txt->Pos().x, mUIClipArea->Pos().y + mUIClipArea->Size().Height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	UIComplexControl::CreateParams ComParams;
	ComParams.Parent( mUIWindow->Container() );
	ComParams.PosSet( Txt->Pos().x, Txt->Pos().y + Txt->Size().Height() + 4 );
	ComParams.SizeSet( 64, 64 );
	ComParams.Background.Color( ColorA( 255, 255, 255, 255 ) );

	if ( ResizeMap ) {
		ComParams.Background.Color( mUIMap->Map()->BaseColor() );
	}

	ComParams.Border.Color( ColorA( 100, 100, 100, 200 ) );
	ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
	mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
	mUIBaseColor->Visible( true );
	mUIBaseColor->Enabled( true );

	Txt = mTheme->CreateTextBox( "Red Color:", mUIWindow->Container(), Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIBaseColor->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIRedSlider = mTheme->CreateSlider( mUIWindow->Container(), Sizei( 128, 20 ), Vector2i( Txt->Pos().x + Txt->Size().Width() + 16, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIRedSlider->MaxValue( 255 );
	mUIRedSlider->Value( 255 );
	mUIRedSlider->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnRedChange ) );

	mUIRedTxt = mTheme->CreateTextBox( String::toStr( 255 ), mUIWindow->Container(), Sizei(), Vector2i( mUIRedSlider->Pos().x + mUIRedSlider->Size().Width() + 4, mUIRedSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIRedSlider->Value( mUIMap->Map()->BaseColor().r() );
	}

	Txt = mTheme->CreateTextBox( "Green Color:", mUIWindow->Container(), Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIRedSlider->Pos().y + mUIRedSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIGreenSlider = mTheme->CreateSlider( mUIWindow->Container(), Sizei( 128, 20 ), Vector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIGreenSlider->MaxValue( 255 );
	mUIGreenSlider->Value( 255 );

	mUIGreenSlider->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnGreenChange ) );

	mUIGreenTxt = mTheme->CreateTextBox( String::toStr( 255 ), mUIWindow->Container(), Sizei(), Vector2i( mUIGreenSlider->Pos().x + mUIGreenSlider->Size().Width() + 4, mUIGreenSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIGreenSlider->Value( mUIMap->Map()->BaseColor().g() );
	}

	Txt = mTheme->CreateTextBox( "Blue Color:", mUIWindow->Container(), Sizei(), Vector2i( mUIBaseColor->Pos().x + mUIBaseColor->Size().Width() + 4, mUIGreenSlider->Pos().y + mUIGreenSlider->Size().Height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIBlueSlider = mTheme->CreateSlider( mUIWindow->Container(), Sizei( 128, 20 ), Vector2i( mUIRedSlider->Pos().x, Txt->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIBlueSlider->MaxValue( 255 );
	mUIBlueSlider->Value( 255 );
	mUIBlueSlider->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnBlueChange ) );

	mUIBlueTxt = mTheme->CreateTextBox( String::toStr( 255 ), mUIWindow->Container(), Sizei(), Vector2i( mUIBlueSlider->Pos().x + mUIBlueSlider->Size().Width() + 4, mUIBlueSlider->Pos().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIBlueSlider->Value( mUIMap->Map()->BaseColor().b() );
	}

	UIPushButton * OKButton = mTheme->CreatePushButton( mUIWindow->Container(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapNew::OKClick ) );
	OKButton->Text( "OK" );

	UIPushButton * CancelButton = mTheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), Vector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapNew::CancelClick ) );
	CancelButton->Text( "Cancel" );

	mUIWindow->Center();
	mUIWindow->Show();
}

UIMapNew::~UIMapNew() {
}

void UIMapNew::OnRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Red = (Uint8)mUIRedSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIRedTxt->Text( String::toStr( (Int32)mUIRedSlider->Value() ) );
}

void UIMapNew::OnGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Green = (Uint8)mUIGreenSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIGreenTxt->Text( String::toStr( (Uint32)mUIGreenSlider->Value() ) );
}

void UIMapNew::OnBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->Background()->Color();
	Col.Blue = (Uint8)mUIBlueSlider->Value();
	mUIBaseColor->Background()->Color( Col );
	mUIBlueTxt->Text( String::toStr( (Uint32)mUIBlueSlider->Value() ) );
}

void UIMapNew::OKClick( const UIEvent * Event ) {
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
		if ( !mResizeMap ) {
			mUIMap->Map()->Create( Sizei( w, h ), ml, Sizei( tw, th ), Flags, mUIMap->Map()->ViewSize() );
			mUIMap->Map()->BaseColor( mUIBaseColor->Background()->Color() );
		} else {
			std::string oldPath( mUIMap->Map()->Path() );
			std::string mapPath( Sys::getTempPath() + "temp.eepp.map.eem" );
			mUIMap->Map()->Save( mapPath );

			TileMap * Map = eeNew( TileMap, () );
			Map->BackColor( ColorA( 100, 100, 100, 100 ) );
			Map->GridLinesColor( ColorA( 150, 150, 150, 150 ) );
			Map->ForceHeadersOnLoad( Sizei( w, h ), Sizei( tw, th ), ml, Flags );
			Map->Load( mapPath );
			Map->DisableForcedHeaders();
			Map->mPath = oldPath;

			mUIMap->ReplaceMap( Map );

			FileSystem::fileRemove( mapPath );
		}

		if ( mNewMapCb.IsSet() )
			mNewMapCb();
	}

	mUIWindow->CloseWindow();
}

void UIMapNew::CancelClick( const UIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void UIMapNew::WindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

}}}
