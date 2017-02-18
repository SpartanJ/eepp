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
	mTheme		= UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow	= mTheme->createWindow( NULL, Sizei( 320, 380 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_SHARE_ALPHA_WITH_CHILDS | UI_WIN_MODAL, Sizei( 320, 380 ) );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIMapNew::WindowClose ) );

	if ( !mResizeMap ) {
		mUIWindow->setTitle( "New Map" );
	} else {
		mUIWindow->setTitle( "Resize Map" );
	}

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mTheme->createTextBox( "Map Size", mUIWindow->getContainer(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "Width:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapWidth = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 100, false );
	mUIMapWidth->setMinValue(1);

	if ( ResizeMap ) {
		mUIMapWidth->setValue( mUIMap->Map()->Size().getWidth() );
	}

	Txt = mTheme->createTextBox( "Height:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapHeight = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 100, false );
	mUIMapHeight->setMinValue(1);

	if ( ResizeMap ) {
		mUIMapHeight->setValue( mUIMap->Map()->Size().getHeight() );
	}

	Txt = mTheme->createTextBox( "Tile Size", mUIWindow->getContainer(), Sizei(), Vector2i( mUIWindow->getContainer()->getSize().getWidth() / 2, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	Txt = mTheme->createTextBox( "Width:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapTWidth = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 32, false );
	mUIMapTWidth->setMinValue(1);

	if ( ResizeMap ) {
		mUIMapTWidth->setValue( mUIMap->Map()->TileSize().getWidth() );
	}

	Txt = mTheme->createTextBox( "Height:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW  );

	mUIMapTHeight = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 32, false );
	mUIMapTHeight->setMinValue(1);

	if ( ResizeMap ) {
		mUIMapTHeight->setValue( mUIMap->Map()->TileSize().getHeight() );
	}

	Txt = mTheme->createTextBox( "Max Layers", mUIWindow->getContainer(), Sizei(), Vector2i( 16, mUIMapTHeight->getPosition().y + mUIMapTHeight->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIMapMaxLayers = mTheme->createSpinBox( mUIWindow->getContainer(), Sizei( 53, 24 ), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_TEXT_SELECTION_ENABLED, 8, false );
	mUIMapMaxLayers->setMaxValue( 32 );

	Txt = mTheme->createTextBox( "Map Flags:", mUIWindow->getContainer(), Sizei(), Vector2i( Txt->getPosition().x, mUIMapMaxLayers->getPosition().y + mUIMapMaxLayers->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUILightsEnabled = mTheme->createCheckBox( mUIWindow->getContainer(), Sizei(), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + Txt->getSize().getHeight() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUILightsEnabled->setText( "Lights Enabled" );
	mUILightsEnabled->setActive( true );

	if ( ResizeMap ) {
		mUILightsEnabled->setActive( 0 != mUIMap->Map()->LightsEnabled() );
	}

	mUILightsByVertex = mTheme->createCheckBox( mUIWindow->getContainer(), Sizei(), Vector2i( mUIWindow->getContainer()->getSize().getWidth() / 2, mUILightsEnabled->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUILightsByVertex->setText( "Lights By Vertex" );
	mUILightsByVertex->setActive( true );

	if ( ResizeMap ) {
		mUILightsByVertex->setActive( 0 != mUIMap->Map()->LightsByVertex() );
	}

	mUIClampBorders = mTheme->createCheckBox( mUIWindow->getContainer(), Sizei(), Vector2i( Txt->getPosition().x + DistFromTitle, mUILightsEnabled->getPosition().y + mUILightsEnabled->getSize().getHeight() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIClampBorders->setText( "Clamp Borders" );
	mUIClampBorders->setActive( true );

	if ( ResizeMap ) {
		mUIClampBorders->setActive( 0 != mUIMap->Map()->ClampBorders() );
	}

	mUIClipArea = mTheme->createCheckBox( mUIWindow->getContainer(), Sizei(), Vector2i( mUIWindow->getContainer()->getSize().getWidth() / 2, mUIClampBorders->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIClipArea->setText( "Clip View Area" );
	mUIClipArea->setActive( true );

	if ( ResizeMap ) {
		mUIClipArea->setActive( 0 != mUIMap->Map()->ClipedArea() );
	}

	Txt = mTheme->createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizei(), Vector2i( Txt->getPosition().x, mUIClipArea->getPosition().y + mUIClipArea->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	UIComplexControl::CreateParams ComParams;
	ComParams.setParent( mUIWindow->getContainer() );
	ComParams.setPosition( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 4 );
	ComParams.setSize( 64, 64 );
	ComParams.Background.setColor( ColorA( 255, 255, 255, 255 ) );

	if ( ResizeMap ) {
		ComParams.Background.setColor( mUIMap->Map()->BaseColor() );
	}

	ComParams.Border.setColor( ColorA( 100, 100, 100, 200 ) );
	ComParams.Flags |= UI_FILL_BACKGROUND | UI_BORDER;
	mUIBaseColor = eeNew( UIComplexControl, ( ComParams ) );
	mUIBaseColor->setVisible( true );
	mUIBaseColor->setEnabled( true );

	Txt = mTheme->createTextBox( "Red Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIBaseColor->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIRedSlider = mTheme->createSlider( mUIWindow->getContainer(), Sizei( 128, 20 ), Vector2i( Txt->getPosition().x + Txt->getSize().getWidth() + 16, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIRedSlider->setMaxValue( 255 );
	mUIRedSlider->setValue( 255 );
	mUIRedSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnRedChange ) );

	mUIRedTxt = mTheme->createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4, mUIRedSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIRedSlider->setValue( mUIMap->Map()->BaseColor().r() );
	}

	Txt = mTheme->createTextBox( "Green Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIGreenSlider = mTheme->createSlider( mUIWindow->getContainer(), Sizei( 128, 20 ), Vector2i( mUIRedSlider->getPosition().x, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIGreenSlider->setMaxValue( 255 );
	mUIGreenSlider->setValue( 255 );

	mUIGreenSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnGreenChange ) );

	mUIGreenTxt = mTheme->createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIGreenSlider->setValue( mUIMap->Map()->BaseColor().g() );
	}

	Txt = mTheme->createTextBox( "Blue Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIBlueSlider = mTheme->createSlider( mUIWindow->getContainer(), Sizei( 128, 20 ), Vector2i( mUIRedSlider->getPosition().x, Txt->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	mUIBlueSlider->setMaxValue( 255 );
	mUIBlueSlider->setValue( 255 );
	mUIBlueSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::OnBlueChange ) );

	mUIBlueTxt = mTheme->createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4, mUIBlueSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	if ( ResizeMap ) {
		mUIBlueSlider->setValue( mUIMap->Map()->BaseColor().b() );
	}

	UIPushButton * OKButton = mTheme->createPushButton( mUIWindow->getContainer(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->getIconByName( "ok" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapNew::OKClick ) );
	OKButton->setText( "OK" );

	UIPushButton * CancelButton = mTheme->createPushButton( mUIWindow->getContainer(), OKButton->getSize(), Vector2i( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mTheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapNew::CancelClick ) );
	CancelButton->setText( "Cancel" );

	mUIWindow->center();
	mUIWindow->show();
}

UIMapNew::~UIMapNew() {
}

void UIMapNew::OnRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Red = (Uint8)mUIRedSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIRedTxt->setText( String::toStr( (Int32)mUIRedSlider->getValue() ) );
}

void UIMapNew::OnGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Green = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIGreenTxt->setText( String::toStr( (Uint32)mUIGreenSlider->getValue() ) );
}

void UIMapNew::OnBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Blue = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIBlueTxt->setText( String::toStr( (Uint32)mUIBlueSlider->getValue() ) );
}

void UIMapNew::OKClick( const UIEvent * Event ) {
	Int32 w = static_cast<Int32>( mUIMapWidth->getValue() );
	Int32 h = static_cast<Int32>( mUIMapHeight->getValue() );
	Int32 tw = static_cast<Int32>( mUIMapTWidth->getValue() );
	Int32 th = static_cast<Int32>( mUIMapTHeight->getValue() );
	Int32 ml = static_cast<Int32>( mUIMapMaxLayers->getValue() );

	Uint32 Flags = MAP_EDITOR_DEFAULT_FLAGS;

	if ( mUILightsEnabled->isActive() )
		Flags |= MAP_FLAG_LIGHTS_ENABLED;

	if ( mUILightsByVertex->isActive() )
		Flags |= MAP_FLAG_LIGHTS_BYVERTEX;

	if ( mUIClampBorders->isActive() )
		Flags |= MAP_FLAG_CLAMP_BORDERS;

	if ( mUIClipArea->isActive() )
		Flags |= MAP_FLAG_CLIP_AREA;

	if ( w > 0 && h > 0 && tw > 0 && th > 0 && ml > 0 ) {
		if ( !mResizeMap ) {
			mUIMap->Map()->Create( Sizei( w, h ), ml, Sizei( tw, th ), Flags, mUIMap->Map()->ViewSize() );
			mUIMap->Map()->BaseColor( mUIBaseColor->getBackground()->getColor() );
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
