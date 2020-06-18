#include <eepp/maps/mapeditor/uimapnew.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace Maps { namespace Private {

static UITextView* createTextBox( const String& Text = "", Node* Parent = NULL,
								  const Sizef& Size = Sizef(), const Vector2f& Pos = Vector2f(),
								  const Uint32& Flags = UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE,
								  const Uint32& fontStyle = Text::Regular ) {
	UITextView* widget = UITextView::New();
	widget->setFontStyle( fontStyle );
	widget->resetFlags( Flags )
		->setParent( Parent )
		->setSize( Size )
		->setVisible( true )
		->setEnabled( false )
		->setPosition( Pos );
	widget->setText( Text );
	return widget;
}

UIMapNew::UIMapNew( UIMap* Map, std::function<void()> NewMapCb, bool ResizeMap ) :
	mTheme( NULL ),
	mUIWindow( NULL ),
	mUIMap( Map ),
	mNewMapCb( NewMapCb ),
	mResizeMap( ResizeMap ) {
	if ( SceneManager::instance()->getUISceneNode() == NULL )
		return;

	mTheme = SceneManager::instance()->getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL == mTheme )
		return;

	mUIWindow = UIWindow::New();
	mUIWindow->setSizeWithDecoration( 320, 380 )
		->setWinFlags( UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS |
					   UI_WIN_SHARE_ALPHA_WITH_CHILDS | UI_WIN_MODAL )
		->setMinWindowSize( 320, 380 );

	mUIWindow->addEventListener( Event::OnWindowClose,
								 cb::Make1( this, &UIMapNew::onWindowClose ) );

	if ( !mResizeMap ) {
		mUIWindow->setTitle( "New Map" );
	} else {
		mUIWindow->setTitle( "Resize Map" );
	}

	Int32 InitialY = 16;
	Int32 DistFromTitle = 18;

	UITextView* Txt =
		createTextBox( "Map Size", mUIWindow->getContainer(), Sizef(), Vector2f( 16, InitialY ),
					   UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	Txt = createTextBox(
		"Width:", mUIWindow->getContainer(), Sizef( 46, 24 ),
		Vector2f( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ),
		UI_NODE_DEFAULT_FLAGS, Text::Shadow );

	mUIMapWidth = UISpinBox::New()->setValue( 100 );
	mUIMapWidth->setParent( mUIWindow->getContainer() )
		->setSize( 53, 0 )
		->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIMapWidth->setMinValue( 1 );

	if ( ResizeMap ) {
		mUIMapWidth->setValue( mUIMap->Map()->getSize().getWidth() );
	}

	Txt = createTextBox(
		"Height:", mUIWindow->getContainer(), Sizef( 46, 24 ),
		Vector2f( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 8 ),
		UI_NODE_DEFAULT_FLAGS, Text::Shadow );

	mUIMapHeight = UISpinBox::New()->setValue( 100 );
	mUIMapHeight->setParent( mUIWindow->getContainer() )
		->setSize( 53, 0 )
		->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIMapHeight->setMinValue( 1 );

	if ( ResizeMap ) {
		mUIMapHeight->setValue( mUIMap->Map()->getSize().getHeight() );
	}

	Txt = createTextBox( "Tile Size", mUIWindow->getContainer(), Sizef(),
						 Vector2f( mUIWindow->getContainer()->getSize().getWidth() / 2, InitialY ),
						 UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	Txt = createTextBox(
		"Width:", mUIWindow->getContainer(), Sizef( 46, 24 ),
		Vector2f( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ),
		UI_NODE_DEFAULT_FLAGS, Text::Shadow );

	mUIMapTWidth = UISpinBox::New()->setValue( 32 );
	mUIMapTWidth->setParent( mUIWindow->getContainer() )
		->setSize( 53, 0 )
		->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIMapTWidth->setMinValue( 1 );

	if ( ResizeMap ) {
		mUIMapTWidth->setValue( mUIMap->Map()->getTileSize().getWidth() );
	}

	Txt = createTextBox(
		"Height:", mUIWindow->getContainer(), Sizef( 46, 24 ),
		Vector2f( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 8 ),
		UI_NODE_DEFAULT_FLAGS, Text::Shadow );

	mUIMapTHeight = UISpinBox::New()->setValue( 32 );
	mUIMapTHeight->setParent( mUIWindow->getContainer() )
		->setSize( 53, 0 )
		->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIMapTHeight->setMinValue( 1 );

	if ( ResizeMap ) {
		mUIMapTHeight->setValue( mUIMap->Map()->getTileSize().getHeight() );
	}

	Txt = createTextBox(
		"Max Layers", mUIWindow->getContainer(), Sizef(),
		Vector2f( 16, mUIMapTHeight->getPosition().y + mUIMapTHeight->getSize().getHeight() + 8 ),
		UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIMapMaxLayers = UISpinBox::New()->setValue( 8 );
	mUIMapMaxLayers->setParent( mUIWindow->getContainer() )
		->setSize( 53, 0 )
		->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );
	mUIMapMaxLayers->setMaxValue( 32 );

	Txt = createTextBox(
		"Map Flags:", mUIWindow->getContainer(), Sizef(),
		Vector2f( Txt->getPosition().x,
				  mUIMapMaxLayers->getPosition().y + mUIMapMaxLayers->getSize().getHeight() + 8 ),
		UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUILightsEnabled = UICheckBox::New();
	mUILightsEnabled->setFlags( UI_AUTO_SIZE )
		->setParent( mUIWindow->getContainer() )
		->setPosition( Txt->getPosition().x + DistFromTitle,
					   Txt->getPosition().y + Txt->getSize().getHeight() + 16 );
	mUILightsEnabled->setText( "Lights Enabled" );
	mUILightsEnabled->setChecked( true );

	if ( ResizeMap ) {
		mUILightsEnabled->setChecked( 0 != mUIMap->Map()->getLightsEnabled() );
	}

	mUILightsByVertex = UICheckBox::New();
	mUILightsByVertex->setFlags( UI_AUTO_SIZE )
		->setParent( mUIWindow->getContainer() )
		->setPosition( mUIWindow->getContainer()->getSize().getWidth() / 2,
					   mUILightsEnabled->getPosition().y );
	mUILightsByVertex->setText( "Lights By Vertex" );
	mUILightsByVertex->setChecked( true );

	if ( ResizeMap ) {
		mUILightsByVertex->setChecked( 0 != mUIMap->Map()->getLightsByVertex() );
	}

	mUIClampBorders = UICheckBox::New();
	mUIClampBorders->setFlags( UI_AUTO_SIZE )
		->setParent( mUIWindow->getContainer() )
		->setPosition( Txt->getPosition().x + DistFromTitle,
					   mUILightsEnabled->getPosition().y + mUILightsEnabled->getSize().getHeight() +
						   16 );
	mUIClampBorders->setText( "Clamp Borders" );
	mUIClampBorders->setChecked( true );

	if ( ResizeMap ) {
		mUIClampBorders->setChecked( 0 != mUIMap->Map()->getClampBorders() );
	}

	mUIClipArea = UICheckBox::New();
	mUIClipArea->setFlags( UI_AUTO_SIZE )
		->setParent( mUIWindow->getContainer() )
		->setPosition( mUIWindow->getContainer()->getSize().getWidth() / 2,
					   mUIClampBorders->getPosition().y );
	mUIClipArea->setText( "Clip View Area" );
	mUIClipArea->setChecked( true );

	if ( ResizeMap ) {
		mUIClipArea->setChecked( 0 != mUIMap->Map()->getClipedArea() );
	}

	Txt =
		createTextBox( "Map Base Color:", mUIWindow->getContainer(), Sizef(),
					   Vector2f( Txt->getPosition().x, mUIClipArea->getPosition().y +
														   mUIClipArea->getSize().getHeight() + 8 ),
					   UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIBaseColor = UIWidget::New();
	mUIBaseColor->setFlags( UI_FILL_BACKGROUND | UI_BORDER );
	mUIBaseColor->setBorderColor( Color( 100, 100, 100, 200 ) );
	mUIBaseColor->setBackgroundColor( ResizeMap ? mUIMap->Map()->getBaseColor()
												: Color( 255, 255, 255, 255 ) );
	mUIBaseColor->setParent( mUIWindow->getContainer() )
		->setPosition( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 4 )
		->setSize( 64, 64 );

	Txt = createTextBox(
		"Red Color:", mUIWindow->getContainer(), Sizef(),
		Vector2f( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4,
				  mUIBaseColor->getPosition().y ),
		UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIRedSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
	mUIRedSlider->setParent( mUIWindow->getContainer() )
		->setSize( 128, 20 )
		->setPosition( Txt->getPosition().x + Txt->getSize().getWidth() + 16,
					   Txt->getPosition().y );
	mUIRedSlider->setMaxValue( 255 );
	mUIRedSlider->setValue( 255 );
	mUIRedSlider->addEventListener( Event::OnValueChange,
									cb::Make1( this, &UIMapNew::onRedChange ) );

	mUIRedTxt = createTextBox(
		String::toString( 255 ), mUIWindow->getContainer(), Sizef(),
		Vector2f( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4,
				  mUIRedSlider->getPosition().y ),
		UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	if ( ResizeMap ) {
		mUIRedSlider->setValue( mUIMap->Map()->getBaseColor().r );
	}

	Txt = createTextBox(
		"Green Color:", mUIWindow->getContainer(), Sizef(),
		Vector2f( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4,
				  mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ),
		UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIGreenSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
	mUIGreenSlider->setParent( mUIWindow->getContainer() )
		->setSize( 128, 20 )
		->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
	mUIGreenSlider->setMaxValue( 255 );
	mUIGreenSlider->setValue( 255 );
	mUIGreenSlider->addEventListener( Event::OnValueChange,
									  cb::Make1( this, &UIMapNew::onGreenChange ) );

	mUIGreenTxt = createTextBox(
		String::toString( 255 ), mUIWindow->getContainer(), Sizef(),
		Vector2f( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4,
				  mUIGreenSlider->getPosition().y ),
		UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	if ( ResizeMap ) {
		mUIGreenSlider->setValue( mUIMap->Map()->getBaseColor().g );
	}

	Txt = createTextBox(
		"Blue Color:", mUIWindow->getContainer(), Sizef(),
		Vector2f( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4,
				  mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ),
		UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIBlueSlider = UISlider::New()->setOrientation( UIOrientation::Horizontal );
	mUIBlueSlider->setParent( mUIWindow->getContainer() )
		->setSize( 128, 20 )
		->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
	mUIBlueSlider->setMaxValue( 255 );
	mUIBlueSlider->setValue( 255 );
	mUIBlueSlider->addEventListener( Event::OnValueChange,
									 cb::Make1( this, &UIMapNew::onBlueChange ) );

	mUIBlueTxt = createTextBox(
		String::toString( 255 ), mUIWindow->getContainer(), Sizef(),
		Vector2f( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4,
				  mUIBlueSlider->getPosition().y ),
		UI_NODE_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	if ( ResizeMap ) {
		mUIBlueSlider->setValue( mUIMap->Map()->getBaseColor().b );
	}

	UIPushButton* OKButton = UIPushButton::New();
	OKButton->setParent( mUIWindow->getContainer() )->setSize( 80, 0 );
	OKButton->setIcon( mTheme->getIconByName( "ok" ) );
	OKButton->setPosition(
		mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4,
		mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( Event::MouseClick, cb::Make1( this, &UIMapNew::onOKClick ) );
	OKButton->setText( "OK" );

	UIPushButton* CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )
		->setSize( OKButton->getSize() )
		->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4,
					   OKButton->getPosition().y );
	CancelButton->setIcon( mTheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( Event::MouseClick,
									cb::Make1( this, &UIMapNew::onCancelClick ) );
	CancelButton->setText( "Cancel" );

	mUIWindow->center();
	mUIWindow->show();
}

UIMapNew::~UIMapNew() {}

void UIMapNew::onRedChange( const Event* ) {
	Color Col = mUIBaseColor->getBackgroundColor();
	Col.r = (Uint8)mUIRedSlider->getValue();
	mUIBaseColor->setBackgroundColor( Col );
	mUIRedTxt->setText( String::toString( (Int32)mUIRedSlider->getValue() ) );
}

void UIMapNew::onGreenChange( const Event* ) {
	Color Col = mUIBaseColor->getBackgroundColor();
	Col.g = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->setBackgroundColor( Col );
	mUIGreenTxt->setText( String::toString( (Uint32)mUIGreenSlider->getValue() ) );
}

void UIMapNew::onBlueChange( const Event* ) {
	Color Col = mUIBaseColor->getBackgroundColor();
	Col.b = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->setBackgroundColor( Col );
	mUIBlueTxt->setText( String::toString( (Uint32)mUIBlueSlider->getValue() ) );
}

void UIMapNew::onOKClick( const Event* ) {
	Int32 w = static_cast<Int32>( mUIMapWidth->getValue() );
	Int32 h = static_cast<Int32>( mUIMapHeight->getValue() );
	Int32 tw = static_cast<Int32>( mUIMapTWidth->getValue() );
	Int32 th = static_cast<Int32>( mUIMapTHeight->getValue() );
	Int32 ml = static_cast<Int32>( mUIMapMaxLayers->getValue() );

	Uint32 Flags = MAP_EDITOR_DEFAULT_FLAGS;

	if ( mUILightsEnabled->isChecked() )
		Flags |= MAP_FLAG_LIGHTS_ENABLED;

	if ( mUILightsByVertex->isChecked() )
		Flags |= MAP_FLAG_LIGHTS_BYVERTEX;

	if ( mUIClampBorders->isChecked() )
		Flags |= MAP_FLAG_CLAMP_BORDERS;

	if ( mUIClipArea->isChecked() )
		Flags |= MAP_FLAG_CLIP_AREA;

	if ( w > 0 && h > 0 && tw > 0 && th > 0 && ml > 0 ) {
		if ( !mResizeMap ) {
			mUIMap->Map()->create( Sizei( w, h ), ml, Sizei( tw, th ), Flags,
								   mUIMap->Map()->getViewSize() );
			mUIMap->Map()->setBaseColor( mUIBaseColor->getBackgroundColor() );
		} else {
			std::string oldPath( mUIMap->Map()->getPath() );
			std::string mapPath( Sys::getTempPath() + "temp.eepp.map.eem" );
			mUIMap->Map()->saveToFile( mapPath );

			TileMap* Map = eeNew( TileMap, () );
			Map->setBackColor( Color( 100, 100, 100, 100 ) );
			Map->setGridLinesColor( Color( 150, 150, 150, 150 ) );
			Map->forceHeadersOnLoad( Sizei( w, h ), Sizei( tw, th ), ml, Flags );
			Map->loadFromFile( mapPath );
			Map->disableForcedHeaders();
			Map->mPath = oldPath;

			mUIMap->replaceMap( Map );

			FileSystem::fileRemove( mapPath );
		}

		if ( mNewMapCb )
			mNewMapCb();
	}

	mUIWindow->closeWindow();
}

void UIMapNew::onCancelClick( const Event* ) {
	mUIWindow->closeWindow();
}

void UIMapNew::onWindowClose( const Event* ) {
	eeDelete( this );
}

}}} // namespace EE::Maps::Private
