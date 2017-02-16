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
	mTheme		= UIThemeManager::instance()->defaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->createWindow( NULL, Sizei( 320, 380 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_SHARE_ALPHA_WITH_CHILDS | UI_WIN_MODAL, Sizei( 320, 380 ) );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIMapNew::WindowClose ) );

	if ( !mResizeMap ) {
		mUIWindow->title( "New Map" );
	} else {
		mUIWindow->title( "Resize Map" );
	}

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mTheme->createTextBox( "Map Size", mUIWindow->getContainer(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "Width:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->position().x + DistFromTitle, Txt->position().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapWidth = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->position().x + Txt->size().width(), Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 100, false );
	mUIMapWidth->minValue(1);

	if ( ResizeMap ) {
		mUIMapWidth->value( mUIMap->Map()->Size().width() );
	}

	Txt = mTheme->createTextBox( "Height:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->position().x, Txt->position().y + Txt->size().height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapHeight = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->position().x + Txt->size().width(), Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 100, false );
	mUIMapHeight->minValue(1);

	if ( ResizeMap ) {
		mUIMapHeight->value( mUIMap->Map()->Size().height() );
	}

	Txt = mTheme->createTextBox( "Tile Size", mUIWindow->getContainer(), Sizei(), Vector2i( mUIWindow->getContainer()->size().width() / 2, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "Width:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->position().x + DistFromTitle, Txt->position().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapTWidth = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->position().x + Txt->size().width(), Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 32, false );
	mUIMapTWidth->minValue(1);

	if ( ResizeMap ) {
		mUIMapTWidth->value( mUIMap->Map()->TileSize().width() );
	}

	Txt = mTheme->createTextBox( "Height:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->position().x, Txt->position().y + Txt->size().height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapTHeight = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->position().x + Txt->size().width(), Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 32, false );
	mUIMapTHeight->minValue(1);

	if ( ResizeMap ) {
		mUIMapTHeight->value( mUIMap->Map()->TileSize().height() );
	}

	Txt = mTheme->createTextBox( "Max Layers", mUIWindow->getContainer(), Sizei(), Vector2i( 16, mUIMapTHeight->position().y + mUIMapTHeight->size().height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIMapMaxLayers = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->position().x + DistFromTitle, Txt->position().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 8, false );
	mUIMapMaxLayers->maxValue( 32 );

	Txt = mTheme->createTextBox( "Map Flags:", mUIWindow->getContainer(), Sizei(), Vector2i( Txt->position().x, mUIMapMaxLayers->position().y + mUIMapMaxLayers->size().height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUILightsEnabled = mTheme->createCheckBox( mUIWindow->getContainer(), Sizei(), Vector2i( Txt->position().x + DistFromTitle, Txt->position().y + Txt->size().height() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUILightsEnabled->text( "Lights Enabled" );
	mUILightsEnabled->active( true );

	if ( ResizeMap ) {
		mUILightsEnabled->active( 0 != mUIMap->Map()->LightsEnabled() );
	}

	mUILightsByVertex = mTheme->createCheckBox( mUIWindow->getContainer(), Sizei(), Vector2i( mUIWindow->getContainer()->size().width() / 2, mUILightsEnabled->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUILightsByVertex->text( "Lights By Vertex" );
	mUILightsByVertex->active( true );

	if ( ResizeMap ) {
		mUILightsByVertex->active( 0 != mUIMap->Map()->LightsByVertex() );
	}

	mUIClampBorders = mTheme->createCheckBox( mUIWindow->getContainer(), Sizei(), Vector2i( Txt->position().x + DistFromTitle, mUILightsEnabled->position().y + mUILightsEnabled->size().height() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIClampBorders->text( "Clamp Borders" );
	mUIClampBorders->active( true );

	if ( ResizeMap ) {
		mUIClampBorders->active( 0 != mUIMap->Map()->ClampBorders() );
	}

	mUIClipArea = mTheme->createCheckBox( mUIWindow->getContainer(), Sizei(), Vector2i( mUIWindow->getContainer()->size().width() / 2, mUIClampBorders->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIClipArea->text( "Clip View Area" );
	mUIClipArea->active( true );

	if ( ResizeMap ) {
		mUIClipArea->active( 0 != mUIMap->Map()->ClipedArea() );
	}

	Txt = mTheme->createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizei(), Vector2i( Txt->position().x, mUIClipArea->position().y + mUIClipArea->size().height() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	UIComplexControl::CreateParams ComParams;
	ComParams.Parent( mUIWindow->getContainer() );
	ComParams.PosSet( Txt->position().x, Txt->position().y + Txt->size().height() + 4 );
	ComParams.SizeSet( 64, 64 );
	ComParams.Background.color( ColorA( 255, 255, 255, 255 ) );

	if ( ResizeMap ) {
		ComParams.Background.color( mUIMap->Map()->BaseColor() );
	}

	ComParams.Border.color( ColorA( 100, 100, 100, 200 ) );
	ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
	mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
	mUIBaseColor->visible( true );
	mUIBaseColor->enabled( true );

	Txt = mTheme->createTextBox( "Red Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIBaseColor->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIRedSlider = mTheme->createSlider( mUIWindow->getContainer(), Sizei( 128, 20 ), Vector2i( Txt->position().x + Txt->size().width() + 16, Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIRedSlider->maxValue( 255 );
	mUIRedSlider->value( 255 );
	mUIRedSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnRedChange ) );

	mUIRedTxt = mTheme->createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIRedSlider->position().x + mUIRedSlider->size().width() + 4, mUIRedSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIRedSlider->value( mUIMap->Map()->BaseColor().r() );
	}

	Txt = mTheme->createTextBox( "Green Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIRedSlider->position().y + mUIRedSlider->size().height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIGreenSlider = mTheme->createSlider( mUIWindow->getContainer(), Sizei( 128, 20 ), Vector2i( mUIRedSlider->position().x, Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIGreenSlider->maxValue( 255 );
	mUIGreenSlider->value( 255 );

	mUIGreenSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnGreenChange ) );

	mUIGreenTxt = mTheme->createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIGreenSlider->position().x + mUIGreenSlider->size().width() + 4, mUIGreenSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIGreenSlider->value( mUIMap->Map()->BaseColor().g() );
	}

	Txt = mTheme->createTextBox( "Blue Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->position().x + mUIBaseColor->size().width() + 4, mUIGreenSlider->position().y + mUIGreenSlider->size().height() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIBlueSlider = mTheme->createSlider( mUIWindow->getContainer(), Sizei( 128, 20 ), Vector2i( mUIRedSlider->position().x, Txt->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIBlueSlider->maxValue( 255 );
	mUIBlueSlider->value( 255 );
	mUIBlueSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnBlueChange ) );

	mUIBlueTxt = mTheme->createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIBlueSlider->position().x + mUIBlueSlider->size().width() + 4, mUIBlueSlider->position().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIBlueSlider->value( mUIMap->Map()->BaseColor().b() );
	}

	UIPushButton * OKButton = mTheme->createPushButton( mUIWindow->getContainer(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->getIconByName( "ok" ) );
	OKButton->position( mUIWindow->getContainer()->size().width() - OKButton->size().width() - 4, mUIWindow->getContainer()->size().height() - OKButton->size().height() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapNew::OKClick ) );
	OKButton->text( "OK" );

	UIPushButton * CancelButton = mTheme->createPushButton( mUIWindow->getContainer(), OKButton->size(), Vector2i( OKButton->position().x - OKButton->size().width() - 4, OKButton->position().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapNew::CancelClick ) );
	CancelButton->text( "Cancel" );

	mUIWindow->center();
	mUIWindow->show();
}

UIMapNew::~UIMapNew() {
}

void UIMapNew::OnRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Red = (Uint8)mUIRedSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIRedTxt->text( String::toStr( (Int32)mUIRedSlider->value() ) );
}

void UIMapNew::OnGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Green = (Uint8)mUIGreenSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIGreenTxt->text( String::toStr( (Uint32)mUIGreenSlider->value() ) );
}

void UIMapNew::OnBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->background()->color();
	Col.Blue = (Uint8)mUIBlueSlider->value();
	mUIBaseColor->background()->color( Col );
	mUIBlueTxt->text( String::toStr( (Uint32)mUIBlueSlider->value() ) );
}

void UIMapNew::OKClick( const UIEvent * Event ) {
	Int32 w = static_cast<Int32>( mUIMapWidth->value() );
	Int32 h = static_cast<Int32>( mUIMapHeight->value() );
	Int32 tw = static_cast<Int32>( mUIMapTWidth->value() );
	Int32 th = static_cast<Int32>( mUIMapTHeight->value() );
	Int32 ml = static_cast<Int32>( mUIMapMaxLayers->value() );

	Uint32 Flags = MAP_EDITOR_DEFAULT_FLAGS;

	if ( mUILightsEnabled->active() )
		Flags |= MAP_FLAG_LIGHTS_ENABLED;

	if ( mUILightsByVertex->active() )
		Flags |= MAP_FLAG_LIGHTS_BYVERTEX;

	if ( mUIClampBorders->active() )
		Flags |= MAP_FLAG_CLAMP_BORDERS;

	if ( mUIClipArea->active() )
		Flags |= MAP_FLAG_CLIP_AREA;

	if ( w > 0 && h > 0 && tw > 0 && th > 0 && ml > 0 ) {
		if ( !mResizeMap ) {
			mUIMap->Map()->Create( Sizei( w, h ), ml, Sizei( tw, th ), Flags, mUIMap->Map()->ViewSize() );
			mUIMap->Map()->BaseColor( mUIBaseColor->background()->color() );
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
