#include <eepp/gaming/mapeditor/uimapnew.hpp>

namespace EE { namespace Gaming { namespace Private {

static UITextView * createTextBox( const String& Text = "", UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, const Uint32& fontStyle = Text::Regular ) {
	UITextView * Ctrl = UITextView::New();
	Ctrl->setFontStyle( fontStyle );
	Ctrl->resetFlags( Flags )->setParent( Parent )->setPosition( Pos )->setSize( Size )->setVisible( true )->setEnabled( false );
	Ctrl->setText( Text );
	return Ctrl;
}

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

	mUIWindow	= UIWindow::New();
	mUIWindow->setSizeWithDecoration( 320, 380 )
			->setWinFlags( UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_SHARE_ALPHA_WITH_CHILDS | UI_WIN_MODAL )
			->setMinWindowSize( 320, 380 );

	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIMapNew::onWindowClose ) );

	if ( !mResizeMap ) {
		mUIWindow->setTitle( "New Map" );
	} else {
		mUIWindow->setTitle( "Resize Map" );
	}

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextView * Txt = createTextBox( "Map Size", mUIWindow->getContainer(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	Txt = createTextBox( "Width:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS, Text::Shadow );

	mUIMapWidth = UISpinBox::New()->setAllowOnlyNumbers( false )->setValue( 100 );
	mUIMapWidth->setParent( mUIWindow->getContainer() )->setSize( 53, 0 )->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIMapWidth->setMinValue(1);

	if ( ResizeMap ) {
		mUIMapWidth->setValue( mUIMap->Map()->getSize().getWidth() );
	}

	Txt = createTextBox( "Height:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS, Text::Shadow  );

	mUIMapHeight = UISpinBox::New()->setAllowOnlyNumbers( false )->setValue( 100 );
	mUIMapHeight->setParent( mUIWindow->getContainer() )->setSize( 53, 0 )->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIMapHeight->setMinValue(1);

	if ( ResizeMap ) {
		mUIMapHeight->setValue( mUIMap->Map()->getSize().getHeight() );
	}

	Txt = createTextBox( "Tile Size", mUIWindow->getContainer(), Sizei(), Vector2i( mUIWindow->getContainer()->getSize().getWidth() / 2, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	Txt = createTextBox( "Width:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS, Text::Shadow );

	mUIMapTWidth = UISpinBox::New()->setAllowOnlyNumbers( false )->setValue( 32 );
	mUIMapTWidth->setParent( mUIWindow->getContainer() )->setSize( 53, 0 )->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIMapTWidth->setMinValue(1);

	if ( ResizeMap ) {
		mUIMapTWidth->setValue( mUIMap->Map()->getTileSize().getWidth() );
	}

	Txt = createTextBox( "Height:", mUIWindow->getContainer(), Sizei( 46, 24 ), Vector2i( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS, Text::Shadow );

	mUIMapTHeight = UISpinBox::New()->setAllowOnlyNumbers( false )->setValue( 32 );
	mUIMapTHeight->setParent( mUIWindow->getContainer() )->setSize( 53, 0 )->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIMapTHeight->setMinValue(1);

	if ( ResizeMap ) {
		mUIMapTHeight->setValue( mUIMap->Map()->getTileSize().getHeight() );
	}

	Txt = createTextBox( "Max Layers", mUIWindow->getContainer(), Sizei(), Vector2i( 16, mUIMapTHeight->getPosition().y + mUIMapTHeight->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS  | UI_AUTO_SIZE, Text::Shadow );

	mUIMapMaxLayers = UISpinBox::New()->setAllowOnlyNumbers( false )->setValue( 8 );
	mUIMapMaxLayers->setParent( mUIWindow->getContainer() )->setSize( 53, 0 )->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );
	mUIMapMaxLayers->setMaxValue( 32 );

	Txt = createTextBox( "Map Flags:", mUIWindow->getContainer(), Sizei(), Vector2i( Txt->getPosition().x, mUIMapMaxLayers->getPosition().y + mUIMapMaxLayers->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUILightsEnabled = UICheckBox::New();
	mUILightsEnabled->setParent( mUIWindow->getContainer() )
			->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + Txt->getSize().getHeight() + 16 )
			->setFlags( UI_AUTO_SIZE );
	mUILightsEnabled->setText( "Lights Enabled" );
	mUILightsEnabled->setActive( true );

	if ( ResizeMap ) {
		mUILightsEnabled->setActive( 0 != mUIMap->Map()->getLightsEnabled() );
	}

	mUILightsByVertex = UICheckBox::New();
	mUILightsByVertex->setParent( mUIWindow->getContainer() )
			->setPosition( mUIWindow->getContainer()->getSize().getWidth() / 2, mUILightsEnabled->getPosition().y )
			->setFlags( UI_AUTO_SIZE );
	mUILightsByVertex->setText( "Lights By Vertex" );
	mUILightsByVertex->setActive( true );

	if ( ResizeMap ) {
		mUILightsByVertex->setActive( 0 != mUIMap->Map()->getLightsByVertex() );
	}

	mUIClampBorders = UICheckBox::New();
	mUIClampBorders->setParent( mUIWindow->getContainer() )
			->setPosition( Txt->getPosition().x + DistFromTitle, mUILightsEnabled->getPosition().y + mUILightsEnabled->getSize().getHeight() + 16 )
			->setFlags( UI_AUTO_SIZE );
	mUIClampBorders->setText( "Clamp Borders" );
	mUIClampBorders->setActive( true );

	if ( ResizeMap ) {
		mUIClampBorders->setActive( 0 != mUIMap->Map()->getClampBorders() );
	}

	mUIClipArea = UICheckBox::New();
	mUIClipArea->setParent( mUIWindow->getContainer() )
			->setPosition( mUIWindow->getContainer()->getSize().getWidth() / 2, mUIClampBorders->getPosition().y )
			->setFlags( UI_AUTO_SIZE );
	mUIClipArea->setText( "Clip View Area" );
	mUIClipArea->setActive( true );

	if ( ResizeMap ) {
		mUIClipArea->setActive( 0 != mUIMap->Map()->getClipedArea() );
	}

	Txt = createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizei(), Vector2i( Txt->getPosition().x, mUIClipArea->getPosition().y + mUIClipArea->getSize().getHeight() + 8 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIBaseColor = UIWidget::New();
	mUIBaseColor->setFlags( UI_FILL_BACKGROUND | UI_BORDER );
	mUIBaseColor->getBorder()->setColor( ColorA( 100, 100, 100, 200 ) );
	mUIBaseColor->getBackground()->setColor( ResizeMap ? mUIMap->Map()->getBaseColor() : ColorA( 255, 255, 255, 255 ) );
	mUIBaseColor->setParent( mUIWindow->getContainer() )->setPosition( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 4 )->setSize( 64, 64 );

	Txt = createTextBox( "Red Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIBaseColor->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIRedSlider = UISlider::New()->setOrientation( UI_HORIZONTAL );
	mUIRedSlider->setParent( mUIWindow->getContainer() )->setSize( 128, 20 )->setPosition( Txt->getPosition().x + Txt->getSize().getWidth() + 16, Txt->getPosition().y );
	mUIRedSlider->setMaxValue( 255 );
	mUIRedSlider->setValue( 255 );
	mUIRedSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::onRedChange ) );

	mUIRedTxt = createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4, mUIRedSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	if ( ResizeMap ) {
		mUIRedSlider->setValue( mUIMap->Map()->getBaseColor().r() );
	}

	Txt = createTextBox( "Green Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIGreenSlider = UISlider::New()->setOrientation( UI_HORIZONTAL );
	mUIGreenSlider->setParent( mUIWindow->getContainer() )->setSize( 128, 20 )->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
	mUIGreenSlider->setMaxValue( 255 );
	mUIGreenSlider->setValue( 255 );
	mUIGreenSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::onGreenChange ) );

	mUIGreenTxt = createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	if ( ResizeMap ) {
		mUIGreenSlider->setValue( mUIMap->Map()->getBaseColor().g() );
	}

	Txt = createTextBox( "Blue Color:", mUIWindow->getContainer(), Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIBlueSlider = UISlider::New()->setOrientation( UI_HORIZONTAL );
	mUIBlueSlider->setParent( mUIWindow->getContainer() )->setSize( 128, 20 )->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
	mUIBlueSlider->setMaxValue( 255 );
	mUIBlueSlider->setValue( 255 );
	mUIBlueSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIMapNew::onBlueChange ) );

	mUIBlueTxt = createTextBox( String::toStr( 255 ), mUIWindow->getContainer(), Sizei(), Vector2i( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4, mUIBlueSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	if ( ResizeMap ) {
		mUIBlueSlider->setValue( mUIMap->Map()->getBaseColor().b() );
	}

	UIPushButton * OKButton = UIPushButton::New();
	OKButton->setParent(  mUIWindow->getContainer() )->setSize( 80, 0 );
	OKButton->setIcon( mTheme->getIconByName( "ok" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapNew::onOKClick ) );
	OKButton->setText( "OK" );

	UIPushButton * CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )->setSize( OKButton->getSize() )->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y );
	CancelButton->setIcon( mTheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIMapNew::onCancelClick ) );
	CancelButton->setText( "Cancel" );

	mUIWindow->center();
	mUIWindow->show();
}

UIMapNew::~UIMapNew() {
}

void UIMapNew::onRedChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Red = (Uint8)mUIRedSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIRedTxt->setText( String::toStr( (Int32)mUIRedSlider->getValue() ) );
}

void UIMapNew::onGreenChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Green = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIGreenTxt->setText( String::toStr( (Uint32)mUIGreenSlider->getValue() ) );
}

void UIMapNew::onBlueChange( const UIEvent * Event ) {
	ColorA Col = mUIBaseColor->getBackground()->getColor();
	Col.Blue = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIBlueTxt->setText( String::toStr( (Uint32)mUIBlueSlider->getValue() ) );
}

void UIMapNew::onOKClick( const UIEvent * Event ) {
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
			mUIMap->Map()->create( Sizei( w, h ), ml, Sizei( tw, th ), Flags, mUIMap->Map()->getViewSize() );
			mUIMap->Map()->setBaseColor( mUIBaseColor->getBackground()->getColor() );
		} else {
			std::string oldPath( mUIMap->Map()->getPath() );
			std::string mapPath( Sys::getTempPath() + "temp.eepp.map.eem" );
			mUIMap->Map()->save( mapPath );

			TileMap * Map = eeNew( TileMap, () );
			Map->setBackColor( ColorA( 100, 100, 100, 100 ) );
			Map->setGridLinesColor( ColorA( 150, 150, 150, 150 ) );
			Map->forceHeadersOnLoad( Sizei( w, h ), Sizei( tw, th ), ml, Flags );
			Map->loadFromFile( mapPath );
			Map->disableForcedHeaders();
			Map->mPath = oldPath;

			mUIMap->replaceMap( Map );

			FileSystem::fileRemove( mapPath );
		}

		if ( mNewMapCb.IsSet() )
			mNewMapCb();
	}

	mUIWindow->closeWindow();
}

void UIMapNew::onCancelClick( const UIEvent * Event ) {
	mUIWindow->closeWindow();
}

void UIMapNew::onWindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

}}}
