#include "eetest.hpp"

Demo_Test::EETest * TestInstance = NULL;

static void mainLoop() {
	TestInstance->update();
}

namespace Demo_Test {

void EETest::init() {
	EE = Engine::instance();

	Log::instance()->setLiveWrite( true );
	Log::instance()->setConsoleOutput( true );

	Translator t;

	t.loadFromString(
		"<resources language='en'>"
		"		<string name='app_name'>eepp</string>"
		"		<string name='formatted'>Test %d %s</string>"
		"</resources>"
	);

	DrawBack 			= false;
	MultiViewportMode 	= false;

	side = aside 		= true;
	ShowParticles 		= true;
	scale 				= 1.0f;
	Ang = ang = alpha 	= 0;
	lasttick 			= 0;
	AnimVal 			= 0.5f;
	mLastFPSLimit 		= 0;
	mWasMinimized 		= false;

	mAxisX				= 0;
	mAxisY				= 0;
	mCurDemo			= eeINDEX_NOT_FOUND;
	mMapEditor			= NULL;
	mETGEditor			= NULL;
	Mus					= NULL;
	mUIWindow			= NULL;
	mTerrainBut			= NULL;
	mShowMenu			= NULL;
	mTerrainUp			= true;

	MyPath 				= Sys::getProcessPath() + "assets/";

	IniFile Ini( MyPath + "ee.ini" );

	PartsNum			= Ini.getValueI( "EEPP", "ParticlesNum", 1000 );
	mUseShaders			= Ini.getValueB( "EEPP", "UseShaders", false );
	mJoyEnabled			= Ini.getValueB( "EEPP", "JoystickEnabled", false );

#if defined( EE_PLATFORM_TOUCH )
	mJoyEnabled = false;
#endif

	mMusEnabled			= Ini.getValueB( "EEPP", "Music", false );
	Int32 StartScreen	= Ini.getValueI( "EEPP", "StartScreen", 0 );

	WindowSettings WinSettings	= EE->createWindowSettings( &Ini );
	ContextSettings ConSettings	= EE->createContextSettings( &Ini );

	if ( !( WinSettings.Style & WindowStyle::Fullscreen ) && !( WinSettings.Style & WindowStyle::UseDesktopResolution ) ) {
#if EE_PLATFORM != EE_PLATFORM_MACOSX
		WinSettings.Width *= WinSettings.PixelDensity;
		WinSettings.Height *= WinSettings.PixelDensity;
#endif
	}

	mWindow = EE->createWindow( WinSettings, ConSettings );

	if ( NULL != mWindow && mWindow->isOpen() ) {
		setScreen( StartScreen );

		mWindow->setCaption( "eepp - Test Application" );
		mWindow->pushResizeCallback( cb::Make1( this, &EETest::onWindowResize ) );

		TF = TextureFactory::instance();
		TF->allocate(40);

		Log		= Log::instance();
		KM		= mWindow->getInput();
		JM		= KM->getJoystickManager();

		PS.resize(5);

		Scenes[0] = cb::Make0( this, &EETest::physicsUpdate );
		Scenes[1] = cb::Make0( this, &EETest::screen1 );
		Scenes[2] = cb::Make0( this, &EETest::screen2 );
		Scenes[3] = cb::Make0( this, &EETest::screen3 );
		Scenes[4] = cb::Make0( this, &EETest::screen4 );
		Scenes[5] = cb::Make0( this, &EETest::screen5 );

		setRandomSeed( static_cast<Uint32>( Sys::getSystemTime() * 1000 ) );

		loadTextures();

		loadFonts();

		createShaders();

		if ( mMusEnabled ) {
			Mus = eeNew( Music, () );

			if ( Mus->openFromFile( MyPath + "sounds/music.ogg" ) ) {
				Mus->setLoop( true );
				Mus->play();
			}
		}

		WP.setType( Ease::QuarticInOut );
		WP.addWaypoint( Vector2f(0,0), 100 );
		WP.addWaypoint( Vector2f(800,0), 100 );
		WP.addWaypoint( Vector2f(0,0), 100 );
		WP.addWaypoint( Vector2f(1024,768), 100 );
		WP.addWaypoint( Vector2f(0,600), 100 );
		WP.editWaypoint( 2, Vector2f(800,600), 100 );
		WP.eraseWaypoint( 3 );
		WP.setLoop(true);
		WP.setTotalTime( Milliseconds( 5000 ) );
		WP.start();

		Batch.allocVertexs( 2048 );
		Batch.setBlendMode( ALPHA_BLENDONE );

		mFBO = FrameBuffer::New( 256, 256, false );

		if ( NULL != mFBO )
			mFBO->setClearColor( ColorAf( 0, 0, 0, 0.5f ) );

		Polygon2f Poly = Polygon2f::createRoundedRectangle( 0, 0, 256, 50 );

		mVBO = VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, DM_TRIANGLE_FAN );

		if ( NULL != mVBO ) {
			for ( Uint32 i = 0; i < Poly.getSize(); i++ ) {
				mVBO->addVertex( Poly[i] );
				mVBO->addColor( ColorA( 100 + i, 255 - i, 150 + i, 200 ) );
			}

			mVBO->compile();
		}

		physicsCreate();

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		launch();
#endif

	} else {
		Engine::destroySingleton();

		exit(0);
	}
}

void EETest::createUIThemeTextureAtlas() {
	#if !defined( EE_DEBUG ) || defined( EE_GLES )
	return;
	#endif

	std::string tgpath( MyPath + "ui/" + mThemeName );
	std::string Path( MyPath + "ui/" + mThemeName );

	if ( !FileSystem::fileExists( tgpath + EE_TEXTURE_ATLAS_EXTENSION ) ) {
		TexturePacker tp( 512, 512, mThemeName.find_first_of( "2x" ) != std::string::npos ? PD_XHDPI : PD_MDPI, true, 2 );
		tp.addTexturesPath( Path );
		tp.packTextures();
		tp.save( tgpath + ".png", SAVE_TYPE_PNG );
	} else {
		TextureAtlasLoader tgl;
		tgl.updateTextureAtlas( tgpath + EE_TEXTURE_ATLAS_EXTENSION, Path );
	}
}

void EETest::loadFonts() {
	mFTE.restart();

	mFontLoader.add( eeNew( FontTrueTypeLoader, ( "arial", MyPath + "fonts/arial.ttf" ) ) );
	mFontLoader.add( eeNew( FontTrueTypeLoader, ( "DejaVuSansMono", MyPath + "fonts/DejaVuSansMono.ttf" ) ) );

	mFontLoader.load( cb::Make1( this, &EETest::onFontLoaded ) );
}

void EETest::onFontLoaded( ResourceLoader * ObjLoaded ) {
	TTF		= FontManager::instance()->getByName( "arial" );
	Font * DBSM	= FontManager::instance()->getByName( "DejaVuSansMono" );

	eePRINTL( "Fonts loading time: %4.3f ms.", mFTE.getElapsed().asMilliseconds() );

	eeASSERT( TTF != NULL );
	eeASSERT( DBSM != NULL );

	Con.create( DBSM, true );
	Con.setBackgroundColor( 0x201f1fEE );
	Con.setBackgroundLineColor( 0x666666EE );
	Con.setFontColor( 0xcfcfcfff );
	Con.ignoreCharOnPrompt( 186 ); // 'º'

	mBuda = String::fromUtf8( "El mono ve el pez en el agua y sufre. Piensa que su mundo es el único que existe, el mejor, el real. Sufre porque es bueno y tiene compasión, lo ve y piensa: \"Pobre se está ahogando no puede respirar\". Y lo saca, lo saca y se queda tranquilo, por fin lo salvé. Pero el pez se retuerce de dolor y muere. Por eso te mostré el sueño, es imposible meter el mar en tu cabeza, que es un balde." );

	createUI();

	mEEText.create( TTF, "Entropia Engine++\nCTRL + Number to change Demo Screen\nRight click to see the PopUp Menu" );
	mEEText.setOutlineThickness( 1 );
	mEEText.setOutlineColor( ColorA(0,0,0,255) );
	mFBOText.create( TTF, "This is a VBO\nInside of a FBO" );
	mFBOText.setOutlineThickness( 1 );
	mFBOText.setOutlineColor( ColorA(0,0,0,255) );

	mInfoText.create( DBSM, "", ColorA(100,100,100,255) );
	mInfoText.setOutlineThickness( 1 );
}

void EETest::createShaders() {
	mUseShaders = mUseShaders && GLi->shadersSupported();

	mShaderProgram = NULL;

	if ( mUseShaders ) {
		mBlurFactor = 0.01f;
		mShaderProgram = ShaderProgram::New( MyPath + "shaders/blur.vert", MyPath + "shaders/blur.frag" );
	}
}

void EETest::onWinMouseUp( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	UIControlAnim * CtrlAnim;

	if ( Event->getControl()->isType( UI_TYPE_WINDOW ) ) {
		CtrlAnim = reinterpret_cast<UIControlAnim*>( Event->getControl() );
	} else {
		CtrlAnim = reinterpret_cast<UIControlAnim*>( Event->getControl()->getParent() );
	}

	if ( MEvent->getFlags() & EE_BUTTON_WUMASK ) {
		CtrlAnim->setScale( CtrlAnim->getScale() + 0.1f );
	} else if ( MEvent->getFlags() & EE_BUTTON_WDMASK ) {
		CtrlAnim->setScale( CtrlAnim->getScale() - 0.1f );
	}
}

void EETest::onShowMenu( const UIEvent * Event ) {
	UIPushButton * PB = static_cast<UIPushButton*>( Event->getControl() );

	if ( Menu->show() ) {
		Vector2i Pos = Vector2i( (Int32)PB->getPolygon()[0].x, (Int32)PB->getPolygon()[0].y - 2 );
		UIMenu::fixMenuPos( Pos , Menu );
		Menu->setPosition( Sizei( (Float)Pos.x / PixelDensity::getPixelDensity(), (Float)Pos.y / PixelDensity::getPixelDensity() ) );
	}
}

void EETest::onWindowResize(EE::Window::Window * win) {
	Map.setViewSize( win->getSize() );
}

static std::vector<String> getTestStringArr() {
	Int32 wsize = 15;
	std::vector<String> str(wsize);

	if ( wsize ) {
		for ( Int32 i = 1; i <= wsize; i++ )
			str[i-1] = "Test ListBox " + String::toStr(i) + " testing it right now!";
	}

	return str;
}

void EETest::createUI() {
	Clock TE;

	mThemeName = "uitheme";

	if ( PixelDensity::getPixelDensity() >= 2 ) {
		mThemeName += "2x";
	}

	createUIThemeTextureAtlas();

	eePRINTL( "Texture Atlas Loading Time: %4.3f ms.", TE.getElapsed().asMilliseconds() );

	Uint32 UI_MAN_OPS = 0;
	//UI_MAN_OPS = UI_MANAGER_HIGHLIGHT_FOCUS | UI_MANAGER_HIGHLIGHT_OVER | UI_MANAGER_DRAW_DEBUG_DATA | UI_MANAGER_DRAW_BOXES;
	UIManager::instance()->init(UI_MAN_OPS);

	//mTheme = UITheme::loadFromFile( UIThemeDefault::New( mThemeName, mThemeName ), MyPath + mThemeName + "/" );

	TextureAtlasLoader tgl( MyPath + "ui/" + mThemeName + EE_TEXTURE_ATLAS_EXTENSION );

	mTheme = UITheme::loadFromTextureAtlas( UIThemeDefault::New( mThemeName, mThemeName ), TextureAtlasManager::instance()->getByName( mThemeName ) );

	UIThemeManager::instance()->add( mTheme );
	UIThemeManager::instance()->setDefaultEffectsEnabled( true );
	UIThemeManager::instance()->setDefaultFont( TTF );
	UIThemeManager::instance()->setDefaultTheme( mThemeName );

	std::vector<String> str = getTestStringArr();

	/**/
	UIWindow * tWin = UIWindow::New();
	tWin->setSize( 530, 405 )->setPosition( 320, 240 );
	UIWindowStyleConfig windowStyleConfig = tWin->getStyleConfig();
	windowStyleConfig.WinFlags = UI_WIN_DRAGABLE_CONTAINER;
	windowStyleConfig.MinWindowSize = Sizei( 530, 405 );
	windowStyleConfig.BaseAlpha = 200;
	tWin->setStyleConfig( windowStyleConfig );

	C = tWin->getContainer();
	tWin->setVisible( false )->setEnabled( false );

	tWin->setTitle( "Controls Test" );

	tWin->addEventListener( UIEvent::EventMouseUp, cb::Make1( this, &EETest::onWinMouseUp ) );
	C->addEventListener( UIEvent::EventMouseUp, cb::Make1( this, &EETest::onWinMouseUp ) );

	UISprite * sprite = UISprite::New();
	sprite->setFlags( UI_AUTO_SIZE );
	sprite->setSprite( eeNew( Sprite, ( "gn" ) ) );
	sprite->setParent( C );
	sprite->setPosition( 160, 100 );
	sprite->setDeallocSprite( true );

	UITextView * Text = UITextView::New();
	Text->setParent( C )->setEnabled( false )->setSize( 320, 240 )->setHorizontalAlign( UI_HALIGN_RIGHT )->setVerticalAlign( UI_VALIGN_TOP );
	Text->setText( "Turn around\nJust Turn Around\nAround!" );

	UITextInput::New()->setParent( C )->setPosition( 20, 216 )->setSize( 200, 0 );

	UIPushButton * Button = UIPushButton::New();
	Button->setParent( C )->setPosition( 225, 215 )->setSize( 90, 0 );
	Button->setIcon( mTheme->getIconByName( "ok" ) );
	Button->setText( "Click Me" );
	Button->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &EETest::onButtonClick ) );
	Button->setTooltipText( "Click and see what happens..." );

	UICheckBox * Checkbox = UICheckBox::New();
	Checkbox->setParent( C )->setPosition( 130, 20 )->setSize( 80, 22 );
	Checkbox->setText( "Check Me" );

	UIRadioButton * RadioButton = UIRadioButton::New();
	RadioButton->setParent( C )->setPosition( 130, 40 )->setSize( 80, 22 );
	RadioButton->setText( "Check Me" );

	RadioButton = UIRadioButton::New();
	RadioButton->setParent( C )->setPosition( 130, 60 )->setSize( 80, 22 );
	RadioButton->setText( "Check Me 2" );

	mSlider = UISlider::New();
	mSlider->setOrientation( UI_HORIZONTAL )->setParent( C )->setPosition( 220, 80 )->setSize( 80, 24 );
	mSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &EETest::onSliderValueChange ) );

	UISlider::New()->setOrientation( UI_VERTICAL )->setParent( C )->setPosition( 40, 110 )->setSize( 24, 80 );

	UISlider::New()->setOrientation( UI_HORIZONTAL )->setParent( C )->setPosition( 60, 110 )->setSize( 80, 24 );

	UISpinBox::New()->setAllowOnlyNumbers( true )->setParent( C )->setPosition( 80, 150 )->setSize( 80, 24 );

	mScrollBar = UIScrollBar::New();
	mScrollBar->setParent( C )->setSize( 0, 240 );
	mScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &EETest::onValueChange ) );

	mProgressBar = UIProgressBar::New();
	mProgressBar->setParent( C )->setSize( 200, 24 )->setPosition( 20, 190 );

	mTextBoxValue = UITextView::New();
	mTextBoxValue->setParent( C )->setPosition( 20, 0 )->setFlags( UI_AUTO_SIZE );
	mTextBoxValue->setVisible( true );
	onValueChange( NULL );

	mListBox = UIListBox::New();
	mListBox->setParent( C )->setPosition( 325, 8 )->setSize( 200, 224 )->setFlags( UI_TOUCH_DRAG_ENABLED );
	mListBox->addListBoxItems( str );

	UIDropDownList * dropDownList = UIDropDownList::New();
	dropDownList->setParent( C )->setPosition( 20, 50 )->setSize( 100, 21 );
	dropDownList->getListBox()->setFlags( UI_TOUCH_DRAG_ENABLED );
	dropDownList->setMaxNumVisibleItems( 4 );

	std::vector<String> combostrs;
	combostrs.push_back( "Plane" );
	combostrs.push_back( "Car" );
	combostrs.push_back( "Bus" );
	combostrs.push_back( "Train" );
	combostrs.push_back( "Overcraft" );
	combostrs.push_back( "Spaceship" );
	combostrs.push_back( "Bike" );
	combostrs.push_back( "Motorbike" );

	dropDownList->getListBox()->addListBoxItems( combostrs );
	dropDownList->getListBox()->setSelected( 0 );

	UIComboBox * comboBox = UIComboBox::New();
	comboBox->setParent( C )->setPosition( 20, 80 )->setSize(  100, 1 );
	comboBox->getListBox()->addListBoxItems( combostrs );
	comboBox->getListBox()->setSelected( 0 );

	Menu = UIPopUpMenu::New();
	Menu->add( "New", mTheme->getIconByName( "document-new" ) );

	Menu->add( "Open...", mTheme->getIconByName( "document-open" ) );
	Menu->addSeparator();
	Menu->add( "Map Editor" );
	Menu->add( "Texture Atlas Editor" );
	Menu->addSeparator();
	Menu->add( "Show Screen 1" );
	Menu->add( "Show Screen 2" );
	Menu->add( "Show Screen 3" );
	Menu->add( "Show Screen 4" );
	Menu->add( "Show Screen 5" );
	Menu->add( "Show Screen 6" );
	Menu->addSeparator();
	Menu->add( "Show Console" );
	Menu->addSeparator();
	Menu->addCheckBox( "Show Window" );
	Menu->add( "Show Window 2" );
	Menu->addCheckBox( "Multi Viewport" );

	UIPopUpMenu * Menu3 = UIPopUpMenu::New();
	Menu3->add( "Hello World 1" );
	Menu3->add( "Hello World 2" );
	Menu3->add( "Hello World 3" );
	Menu3->add( "Hello World 4" );

	UIPopUpMenu * Menu2 = UIPopUpMenu::New();
	Menu2->add( "Test 1" );
	Menu2->add( "Test 2" );
	Menu2->add( "Test 3" );
	Menu2->add( "Test 4" );
	Menu2->addSubMenu( "Hello World", NULL, Menu3 );

	Menu->addSeparator();
	Menu->addSubMenu( "Sub-Menu", NULL, Menu2 ) ;

	Menu->addSeparator();
	Menu->add( "Quit" );

	Menu->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &EETest::onItemClick ) );
	Menu->getItem( "Quit" )->addEventListener( UIEvent::EventMouseUp, cb::Make1( this, &EETest::onQuitClick ) );
	UIManager::instance()->getMainControl()->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &EETest::onMainClick ) );

	UITextEdit * TextEdit = UITextEdit::New();
	TextEdit->setParent( C )->setPosition( 5, 245 )->setSize( 315, 130 );
	TextEdit->setText( mBuda );

	UITable * genGrid = UITable::New();
	genGrid->setSmoothScroll( true );
	genGrid->setParent( C )->setPosition( 325, 245 )->setSize( 200, 130 );
	genGrid->setCollumnsCount( 3 )->setRowHeight( 24 );

	for ( Uint32 i = 0; i < 15; i++ ) {
		UITableCell * Cell			= UITableCell::New();
		UITextView * TxtBox			= UITextView::New();
		UITextInput * TxtInput		= UITextInput::New();
		UIImage * TxtGfx			= UIImage::New();

		Cell->setParent( genGrid->getContainer() );

		TxtGfx->setVerticalAlign( UI_VALIGN_CENTER );
		TxtGfx->setDrawable( mTheme->getIconByName( "ok" ) );
		TxtBox->setText( "Test " + String::toStr( i+1 ) );

		Cell->setCell( 0, TxtBox );
		Cell->setCell( 1, TxtGfx );
		Cell->setCell( 2, TxtInput );

		genGrid->add( Cell );
	}

	genGrid->setCollumnWidth( 0, 50 );
	genGrid->setCollumnWidth( 1, 24 );
	genGrid->setCollumnWidth( 2, 100 );

#ifdef EE_PLATFORM_TOUCH
	TextureAtlas * SG = GlobalTextureAtlas::instance();

	Texture * butTex = TF->getTexture( TF->loadFromFile( MyPath + "sprites/button-te_normal.png" ) );

	SG->add( butTex->getId(), "button-te_normal" );
	SG->add( TF->loadFromFile( MyPath + "sprites/button-te_mdown.png" ), "button-te_mdown" );

	UISkinSimple nSkin( "button-te" );
	Sizei screenSize = UIManager::instance()->getMainControl()->getSize();

	mShowMenu = UIPushButton::New();
	mShowMenu->setSkin( nSkin );

	Sizei skinSize = mShowMenu->getSkinSize();

	mShowMenu->setSize( mShowMenu->getSkinSize() )
			->setPosition( screenSize.getWidth() - skinSize.getWidth() - 20,
						   screenSize.getHeight() - skinSize.getHeight() - 10 );
	mShowMenu->setText( "Show Menu" );
	mShowMenu->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mShowMenu->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &EETest::onShowMenu ) );
#endif

	C = reinterpret_cast<UIControlAnim*> ( C->getParent() );

	eePRINTL( "CreateUI time: %4.3f ms.", TE.getElapsed().asMilliseconds() );
	/**/

	createNewUI();
}

void EETest::createNewUI() {
	std::vector<String> str = getTestStringArr();

	UIRadioButton * ctrl = UIRadioButton::New();
	ctrl->setPosition( 50, 100 )->setSize( 200, 32 );
	ctrl->setBackgroundFillEnabled( true )->setColor( 0x33333333 );
	ctrl->setBorderEnabled( true )->setColor( 0x66666666 );
	ctrl->setText( "Happy RadioButon :)" );
	ctrl->setFontColor( 0x000000FF );

	UICheckBox * cbox = UICheckBox::New();
	cbox->setPosition( 50, 164 )->setSize( 200, 32 );
	cbox->setBackgroundFillEnabled( true )->setColor( 0x33333333 );
	cbox->setBorderEnabled( true )->setColor( 0x66666666 );
	cbox->setText( "Happy CheckBox :)" );
	cbox->setFontColor( 0x000000FF );

	UIImage * gfx = UIImage::New();
	gfx->setPosition( 50, 140 )->setSize( 16, 16 );
	gfx->setBackgroundFillEnabled( true )->setColor( 0x33333333 );
	gfx->setDrawable( mTheme->getIconByName( "ok" ) );

	UISlider * slider = UISlider::New();
	slider->setOrientation( UI_HORIZONTAL )->setPosition( 50, 0 )->setSize( 100, 100 );
	slider->setAllowHalfSliderOut( true );

	UISlider * slider2 = UISlider::New();
	slider2->setOrientation( UI_VERTICAL )->setPosition( 100, 0 )->setSize( 100, 100 );
	slider2->setAllowHalfSliderOut( true );

	UITextInput * textInput = UITextInput::New();
	textInput->setPosition( 50, 210 )->setSize( 200, 0 );

	UITextInputPassword * textInputPass = UITextInputPassword::New();
	textInputPass->setPosition( 50, 245 )->setSize( 200, 0 );

	UIListBox * listBox = UIListBox::New();
	listBox->setPosition( 50, 360 )->setSize( 200, 160 );
	listBox->addListBoxItems( str );

	UIProgressBar * progressBar = UIProgressBar::New();
	progressBar->setPosition( 50, 530 )->setSize( 200, 0 );
	progressBar->setProgress( 60.f );
	progressBar->setDisplayPercent( true );

	UIPushButton * pushButton = UIPushButton::New();
	pushButton->setPosition( 50, 560 )->setSize( 200, 0 );
	pushButton->setText( "PushButton" );
	pushButton->setIcon( mTheme->getIconByName( "ok" ) );

	UISprite * sprite = UISprite::New();
	sprite->setFlags( UI_AUTO_SIZE );
	sprite->setPosition( 50, 600 );
	sprite->setSprite( &SP );

	UIScrollBar * scrollBar = UIScrollBar::New();
	scrollBar->setOrientation( UI_HORIZONTAL )->setPosition( 200, 0 )->setSize( 100, 0 );

	UIScrollBar * scrollBar2 = UIScrollBar::New();
	scrollBar2->setOrientation( UI_VERTICAL )->setPosition( 300, 0 )->setSize( 0, 100 );

	UIDropDownList * dropdownList = UIDropDownList::New();
	dropdownList->setPosition( 50, 320 )->setSize( 200, 100 );
	dropdownList->getListBox()->addListBoxItem( "Test 1" );
	dropdownList->getListBox()->addListBoxItem( "Test 2" );
	dropdownList->getListBox()->addListBoxItem( "Test 3" );

	UIComboBox * comboBox = UIComboBox::New();
	comboBox->setPosition( 50, 280 )->setSize( 200, 0 );
	comboBox->getListBox()->addListBoxItem( "Test 1234" );
	comboBox->getListBox()->addListBoxItem( "Test 2345" );
	comboBox->getListBox()->addListBoxItem( "Test 3567" );
	comboBox->getListBox()->setSelected( 0 );

	UITextEdit * textEdit = UITextEdit::New();
	textEdit->setPosition( 350, 4 )->setSize( 200, 200 );
	textEdit->setText( mBuda );

	UISpinBox * spinBox = UISpinBox::New();
	spinBox->setPosition( 350, 210 )->setSize( 200, 0 );

	UITable * genGrid = UITable::New();
	genGrid->setSmoothScroll( true );
	genGrid->setPosition( 350, 250 )->setSize( 200, 130 );
	genGrid->setCollumnsCount( 3 )->setRowHeight( 24 );
	genGrid->setCollumnWidth( 0, 50 );
	genGrid->setCollumnWidth( 1, 24 );
	genGrid->setCollumnWidth( 2, 100 );

	for ( Uint32 i = 0; i < 15; i++ ) {
		UITableCell * Cell			= UITableCell::New();
		UITextView * TxtBox			= UITextView::New();
		UITextInput * TxtInput		= UITextInput::New();
		UIImage * TxtGfx				= UIImage::New();
		TxtGfx->unsetFlags( UI_AUTO_SIZE );

		Cell->setParent( genGrid->getContainer() );

		Cell->setCell( 0, TxtBox );
		Cell->setCell( 1, TxtGfx );
		Cell->setCell( 2, TxtInput );

		TxtGfx->setDrawable( mTheme->getIconByName( "ok" ) );
		TxtBox->setText( "Test " + String::toStr( i+1 ) );

		genGrid->add( Cell );
	}

	UIWindow * MenuCont = UIWindow::New();
	MenuCont->setPosition( 350, 390 )->setSize( 200, 115 );

	UIWinMenu * WinMenu = UIWinMenu::New();
	WinMenu->setParent( MenuCont->getContainer() );

	UIPopUpMenu * PopMenu = UIPopUpMenu::New();
	PopMenu->add( "File" );
	PopMenu->add( "Open" );
	PopMenu->add( "Close" );
	PopMenu->add( "Quit" );

	UIPopUpMenu * PopMenu2 = UIPopUpMenu::New();
	PopMenu2->add( "Bla" );
	PopMenu2->add( "Bla 2" );
	PopMenu2->add( "Bla 3" );
	PopMenu2->add( "Bla 4" );

	WinMenu->addMenuButton( "File", PopMenu );
	WinMenu->addMenuButton( "Edit", PopMenu2 );

	UITabWidget * TabWidget = UITabWidget::New();
	TabWidget->setPosition( 350, 530 )->setSize( 200, 64 );

	TabWidget->add( "Tab 1", UIWidget::New()->setThemeControl( "winback" ), mTheme->getIconByName( "ok" ) );
	TabWidget->add( "Tab 2", UIWidget::New()->setThemeControl( "winback" ), mTheme->getIconByName( "go-up" ) );
	TabWidget->add( "Tab 3", UIWidget::New()->setThemeControl( "winback" ), mTheme->getIconByName( "add" ) );

	UIWindow * win = UIWindow::New();
	win->setSize( 500, 500 );
	win->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_RESIZEABLE | UI_WIN_MAXIMIZE_BUTTON );

	UILinearLayout * layWin = UILinearLayout::NewVertical();
	layWin->setLayoutSizeRules( MATCH_PARENT, MATCH_PARENT );
	layWin->setParent( win );

	UILinearLayout * layPar = UILinearLayout::NewHorizontal();
	layPar->setParent( layWin );
	layPar->setLayoutMargin( Recti( 16, 16, 16, 16 ) );
	layPar->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT );
	layPar->setLayoutGravity( UI_VALIGN_CENTER | UI_HALIGN_CENTER );
	layPar->setBackgroundFillEnabled( true )->setColor( 0x999999FF );

	UILinearLayout * lay = UILinearLayout::NewVertical();
	lay->setLayoutGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
	lay->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT );
	lay->setBackgroundFillEnabled( true )->setColor( 0x333333FF );
	lay->setLayoutWeight( 0.7f );

	UITextView::New()->setText( "Text on test 1" )->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( WRAP_CONTENT, WRAP_CONTENT )->setParent( lay );
	UITextView::New()->setText( "Text on test 2" )->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( lay );
	UICheckBox::New()->setText( "Checkbox" )->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( lay );
	UITextView::New()->setText( "Text on test 3" )->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( lay );
	UITextView::New()->setText( "Text on test 4" )->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( lay );
	UITextInput::New()->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( lay );

	UILinearLayout * lay2 = UILinearLayout::NewVertical();
	lay2->setLayoutGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
	lay2->setLayoutSizeRules( FIXED, MATCH_PARENT );
	lay2->setBackgroundFillEnabled( true )->setColor( 0x000000FF );
	lay2->setLayoutWeight( 0.3f );

	UIPushButton::New()->setText( "PushButton" )->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setLayoutGravity( UI_VALIGN_CENTER )->setParent( lay2 );
	UIListBox * lbox = UIListBox::New();
	lbox->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( MATCH_PARENT, FIXED )->setSize( 0, 120 )->setParent( lay2 );
	lbox->addListBoxItems( { "This", "is", "a", "ListBox" } );
	lay2->setParent( layPar );
	lay->setParent( layPar );

	UIDropDownList * drop = UIDropDownList::New();
	drop->setLayoutMargin( Recti( 10, 10, 10, 10 ) )->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( layWin );
	drop->getListBox()->addListBoxItems( { "Car", "Bus", "Plane", "Submarine" } );
	drop->getListBox()->setSelected(0);
	win->show();

	UIWindow * win2 = UIWindow::New();
	win2->setSize( 500, 500 );
	win2->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_RESIZEABLE | UI_WIN_MAXIMIZE_BUTTON );

	UIRelativeLayout * rlay = UIRelativeLayout::New();
	rlay->setParent( win2 );
	rlay->setLayoutSizeRules( MATCH_PARENT, MATCH_PARENT );
	rlay->setLayoutMargin( Recti( 16, 16, 16, 16 ) );
	rlay->setBackgroundFillEnabled( true )->setColor( 0x333333CC );

	UIPushButton * ofBut = UIPushButton::New();
	ofBut->setText( "OK" )->setLayoutGravity( UI_VALIGN_BOTTOM | UI_HALIGN_RIGHT )->setLayoutMargin( Recti( 0, 0, 16, 16 ) )->setParent( rlay );

	UIPushButton::New()->setText( "Cancel" )->setLayoutGravity( UI_VALIGN_BOTTOM | UI_HALIGN_RIGHT )->setLayoutMargin( Recti( 0, 0, 8, 0 ) )
			->setLayoutPositionRule( LEFT_OF, ofBut )->setParent( rlay );

	win2->show();

	UIManager::instance()->loadLayoutFromString(
		"<window layout_width='300dp' layout_height='300dp' winflags='default|maximize'>"
		"	<LinearLayout id='testlayout' orientation='vertical' layout_width='match_parent' layout_height='match_parent' layout_margin='8dp'>"
		"		<TextView text='Hello World!' gravity='center' layout_gravity='center_horizontal' layout_width='match_parent' layout_height='wrap_content' backgroundColor='000000FF' />"
		"		<PushButton text='OK!' textSize='16dp' icon='ok' gravity='center' layout_gravity='center_horizontal' layout_width='match_parent' layout_height='wrap_content' />"
		"		<Image src='thecircle' layout_width='match_parent' layout_height='32dp' flags='clip' />"
		"		<Sprite src='gn' />"
		"		<TextInput text='test' layout_width='match_parent' layout_height='wrap_content' />"
		"		<DropDownList layout_width='match_parent' layout_height='wrap_content' selectedIndex='0'>"
		"			<item>Test Item</item>"
		"			<item>Test Item 2</item>"
		"		</DropDownList>"
		"		<ListBox layout_width='match_parent' layout_height='match_parent' layout_weight='1'>"
		"			<item>Hello!</item>"
		"			<item>World!</item>"
		"		</ListBox>"
		"	</LinearLayout>"
		"</window>"
	);
}

void EETest::createMapEditor() {
	if ( NULL != mMapEditor )
		return;

	UIWindow * tWin = UIWindow::New();
	tWin->setSizeWithDecoration( 1024, 768 )->setPosition( 0, 0 );
	UIWindowStyleConfig windowStyleConfig = tWin->getStyleConfig();
	windowStyleConfig.WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER;
	windowStyleConfig.MinWindowSize = Sizei( 1024, 768 );
	tWin->setStyleConfig( windowStyleConfig );

	mMapEditor = eeNew( MapEditor, ( tWin, cb::Make0( this, &EETest::onMapEditorClose ) ) );
	tWin->center();
	tWin->show();
}

void EETest::onMapEditorClose() {
	mMapEditor = NULL;
}

void EETest::createETGEditor() {
	UIWindow * tWin = UIWindow::New();
	tWin->setSizeWithDecoration( 1024, 768 )->setPosition( 0, 0 );
	UIWindowStyleConfig windowStyleConfig = tWin->getStyleConfig();
	windowStyleConfig.WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER;
	windowStyleConfig.MinWindowSize = Sizei( 1024, 768 );
	tWin->setStyleConfig( windowStyleConfig );

	mETGEditor = eeNew ( Tools::TextureAtlasEditor, ( tWin, cb::Make0( this, &EETest::onETGEditorClose ) ) );
	tWin->center();
	tWin->show();
}

void EETest::onETGEditorClose() {
	mETGEditor = NULL;
}

void EETest::createCommonDialog() {
	UICommonDialog * CDialog = UICommonDialog::New();
	CDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON );
	CDialog->addFilePattern( "*.hpp;*.cpp", true );
	CDialog->center();
	CDialog->show();
}

void EETest::createDecoratedWindow() {
	mUIWindow = UIWindow::New();
	mUIWindow->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON )
			->setMinWindowSize( 530, 350 )->setPosition( 200, 50 );

	mUIWindow->addEventListener( UIEvent::EventOnWindowCloseClick, cb::Make1( this, &EETest::onCloseClick ) );
	mUIWindow->setTitle( "Test Window" );

	UILinearLayout * lay = UILinearLayout::NewVertical();
	lay->setLayoutSizeRules( MATCH_PARENT, MATCH_PARENT );
	lay->setParent( mUIWindow->getContainer() );

	UIWinMenu * WinMenu = UIWinMenu::New();
	WinMenu->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( lay );

	UIPopUpMenu * PopMenu = UIPopUpMenu::New();
	PopMenu->add( "File" );
	PopMenu->add( "Open" );
	PopMenu->add( "Close" );
	PopMenu->add( "Quit" );

	UIPopUpMenu * PopMenu2 = UIPopUpMenu::New();
	PopMenu2->add( "Bla" );
	PopMenu2->add( "Bla 2" );
	PopMenu2->add( "Bla 3" );
	PopMenu2->add( "Bla 4" );

	WinMenu->addMenuButton( "File", PopMenu );
	WinMenu->addMenuButton( "Edit", PopMenu2 );

	UIPushButton * Button = UIPushButton::New();
	Button->setLayoutMargin( Recti( 5, 5, 5, 5 ) );
	Button->setText( "Click Me" );
	Button->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )->setParent( lay );
	Button->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &EETest::onButtonClick ) );

	mUIWindow->addShortcut( KEY_C, KEYMOD_ALT, Button );

	UITabWidget * TabWidget = UITabWidget::New();
	TabWidget->setLayoutMargin( Recti( 5, 5, 5, 5 ) )
			->setLayoutWeight( 1 )
			->setLayoutSizeRules( MATCH_PARENT, WRAP_CONTENT )
			->setParent( lay )->setFlags( UI_HALIGN_CENTER | UI_VALIGN_CENTER );

	UITextEdit * TEdit = UITextEdit::New();
	TEdit->setParent( TabWidget );
	TEdit->setText( mBuda );
	TabWidget->add( "TextEdit", TEdit );

	UITextInput * Txt = UITextInput::New();
	Txt->setFlags( UI_WORD_WRAP );
	Txt->setParent( TabWidget );
	Txt->setText( mBuda );
	TabWidget->add( "TextInput", Txt );

	UITextView * txtBox = UITextView::New();
	txtBox->resetFlags( UI_HALIGN_LEFT | UI_VALIGN_TOP | UI_AUTO_PADDING | UI_WORD_WRAP | UI_TEXT_SELECTION_ENABLED );
	txtBox->setParent( TabWidget );
	txtBox->setText( mBuda );

	TabWidget->add( "TextBox", txtBox );
}

void EETest::onCloseClick( const UIEvent * Event ) {
	mUIWindow = NULL;
}

void EETest::onItemClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "Show Screen 1" == txt ) {
		setScreen( 0 );
	} else if ( "Show Screen 2" == txt ) {
		setScreen( 1 );
	} else if ( "Show Screen 3" == txt ) {
		setScreen( 2 );
	} else if ( "Show Screen 4" == txt ) {
		setScreen( 3 );
	} else if ( "Show Screen 5" == txt ) {
		setScreen( 4 );
	} else if ( "Show Screen 6" == txt ) {
		setScreen( 5 );
	} else if ( "Show Console" == txt ) {
		Con.toggle();

		if ( Con.isActive() ) {
			mWindow->startTextInput();
		} else {
			mWindow->stopTextInput();
		}
	} else if ( "Show Window" == txt ) {
		UIMenuCheckBox * Chk = reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() );

		C->setVisible( true );
		C->setEnabled( true );

		if ( Chk->isActive() ) {
			if ( C->getScale() == 1.f ) C->setScale( 0.f );
			C->startScaleAnim( C->getScale(), Vector2f::One, Milliseconds( 500.f ), Ease::SineOut );
			C->startAlphaAnim( C->getAlpha(), 255.f, Milliseconds( 500.f ) );
			C->startRotation( 0, 360, Milliseconds( 500.f ), Ease::SineOut );
		} else {
			C->startScaleAnim( C->getScale(), Vector2f::Zero, Milliseconds( 500.f ), Ease::SineIn );
			C->startAlphaAnim( C->getAlpha(), 0.f, Milliseconds( 500.f ) );
			C->startRotation( 0, 360, Milliseconds( 500.f ), Ease::SineIn );
		}
	} else if ( "Show Window 2" == txt ) {
		if ( NULL == mUIWindow ) {
			createDecoratedWindow();
		}

		mUIWindow->show();
	} else if ( "Map Editor" == txt ) {
		createMapEditor();
	} else if ( "Texture Atlas Editor" == txt ) {
		createETGEditor();
	} else if ( "Multi Viewport" == txt ) {
		MultiViewportMode = !MultiViewportMode;
	} else if ( "Open..." == txt ) {
		createCommonDialog();
	} else if ( "New" == txt ) {
		if ( 0 == Screen ) {
			changeDemo( 0 );
		}
	}
}

void EETest::onValueChange( const UIEvent * Event ) {
	mTextBoxValue->setText( "Scroll Value:\n" + String::toStr( mScrollBar->getValue() ) );

	mProgressBar->setProgress( mScrollBar->getValue() * 100.f );
}

void EETest::onSliderValueChange( const UIEvent * Event ) {
	UISlider * slider = static_cast<UISlider*>( Event->getControl() );

	C->setRotation( slider->getValue() * 90.f );
}

void EETest::onQuitClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mWindow->close();
	}
}

void EETest::showMenu() {
	if ( Menu->show() ) {
		Vector2i Pos = mWindow->getInput()->getMousePos();
		UIMenu::fixMenuPos( Pos , Menu );
		Menu->setPosition( Sizei( (Float)Pos.x / PixelDensity::getPixelDensity(), (Float)Pos.y / PixelDensity::getPixelDensity() ) );
	}
}

void EETest::onMainClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MouseEvent->getFlags() & EE_BUTTON_RMASK ) {
		showMenu();
	}
}

void EETest::onButtonClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MouseEvent->getFlags() & EE_BUTTONS_LRM ) {
		UIImage * Gfx = UIImage::New();
		Gfx->setDrawable( mTheme->getIconByName( "ok" ) );
		Gfx->setEnabled( false );

		Gfx->startRotation( 0, 2500, Milliseconds( 2500 ) );
		Gfx->startMovement( Vector2i( Math::randi( 0, mWindow->getWidth() ), -64 ), Vector2i( Math::randi( 0, mWindow->getWidth() ), mWindow->getHeight() + 64 ), Milliseconds( 2500 ) );
		Gfx->closeFadeOut( Milliseconds( 3500 ) );

		//mListBox->addListBoxItem( "Test ListBox " + String::toStr( mListBox->getCount() + 1 ) + " testing it right now!" );
	}
}

void EETest::setScreen( Uint32 num ) {
	if ( NULL != mTerrainBut ) mTerrainBut->setVisible( 1 == num );

	if ( 0 == num || 5 == num )
		mWindow->setClearColor( RGB( 240, 240, 240 ) );
	else
		mWindow->setClearColor( RGB( 0, 0, 0 ) );

	if ( num < 6 )
		Screen = num;
}

void EETest::cmdSetPartsNum ( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt >= 0 && tInt <= 100000 ) ) {
			PS[2].create( PSE_WormHole, tInt, TN[5], Vector2f( mWindow->getWidth() * 0.5f, mWindow->getHeight() * 0.5f ), 32, true );
			Con.pushText( "Wormhole Particles Number Changed to: " + String::toStr(tInt) );
		} else
			Con.pushText( "Valid parameters are between 0 and 100000 (0 = no limit)." );
	}
}

void EETest::onTextureLoaded( ResourceLoader * ResLoaded ) {
	SndMng.play( "mysound" );
}

void EETest::loadTextures() {
	if ( !FileSystem::fileExists( MyPath + "atlases/bnb.eta" ) ) {
		TexturePacker tp( 512, 512, PD_MDPI, true, 0 );
		tp.addTexturesPath( MyPath + "atlases/bnb/" );
		tp.save( MyPath + "atlases/bnb.png" );
	}

	if ( !FileSystem::fileExists( MyPath + "atlases/tiles.eta" ) ) {
		TexturePacker tp( 256, 32, PD_MDPI, true, 0 );
		tp.addTexturesPath( MyPath + "atlases/tiles/" );
		tp.save( MyPath + "atlases/tiles.png" );
	}

	Clock TE;

	Uint32 i;

	PakTest = eeNew( Zip, () );

	#ifndef EE_GLES

	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX
	Engine::instance()->enableSharedGLContext();
	#endif

	PakTest->open( MyPath + "test.zip" );

	std::vector<std::string> files = PakTest->getFileList();

	for ( i = 0; i < files.size(); i++ ) {
		std::string name( files[i] );

		if ( "jpg" == FileSystem::fileExtension( name ) ) {
			mResLoad.add( eeNew( TextureLoader, ( PakTest, name ) ) );
		}
	}
	#endif

	mResLoad.add( eeNew( SoundLoader, ( &SndMng, "mysound", MyPath + "sounds/sound.ogg" ) ) );

	mResLoad.load( cb::Make1( this, &EETest::onTextureLoaded ) );

	TN.resize(12);
	TNP.resize(12);

	for ( i = 0; i <= 6; i++ ) {
		TN[i] = TF->loadFromFile( MyPath + "sprites/" + String::toStr(i+1) + ".png", (i+1) == 7 ? true : false, ( (i+1) == 4 ) ? CLAMP_REPEAT : CLAMP_TO_EDGE );
		TNP[i] = TF->getTexture( TN[i] );
	}

	Tiles.resize(10);

	TextureAtlasLoader tgl( MyPath + "atlases/tiles.eta" );
	TextureAtlas * SG = TextureAtlasManager::instance()->getByName( "tiles" );

	if ( NULL != SG ) {
		for ( i = 0; i < 6; i++ ) {
			Tiles[i] = SG->getByName( String::toStr( i+1 ) );
		}

		Tiles[6] = SG->add( TF->loadFromFile( MyPath + "sprites/objects/1.png" ), "7" );

		#ifdef EE_GLES
		Image tImg( MyPath + "sprites/objects/2.png", 4 );
		tImg.CreateMaskFromColor( ColorA(0,0,0,255), 0 );
		Tiles[7] = SG->Add( TF->loadFromPixels( tImg.getPixelsPtr(), tImg.getWidth(), tImg.getHeight(), tImg.getChannels() ), "8" );
		#else
		Tiles[7] = SG->add( TF->loadFromFile( MyPath + "sprites/objects/2.png" ), "8" );
		Tiles[7]->getTexture()->createMaskFromColor( ColorA(0,0,0,255), 0 );
		#endif
	}

	int w, h;

	for ( Int32 my = 0; my < 4; my++ )
		for( Int32 mx = 0; mx < 8; mx++ )
			SP.addFrame( TN[4], Sizef( 0, 0 ), Vector2i( 0, 0 ), Recti( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );

	PS[0].setCallbackReset( cb::Make2( this, &EETest::particlesCallback ) );
	PS[0].create( PSE_Callback, 500, TN[5], Vector2f( 0, 0 ), 16, true );
	PS[1].create( PSE_Heal, 250, TN[5], Vector2f( mWindow->getWidth() * 0.5f, mWindow->getHeight() * 0.5f ), 16, true );
	PS[2].create( PSE_WormHole, PartsNum, TN[5], Vector2f( mWindow->getWidth() * 0.5f, mWindow->getHeight() * 0.5f ), 32, true );
	PS[3].create( PSE_Fire, 350, TN[5], Vector2f( -50.f, -50.f ), 32, true );
	PS[4].create( PSE_Fire, 350, TN[5], Vector2f( -50.f, -50.f ), 32, true );

	Con.addCommand( "setparticlesnum", cb::Make1( this, &EETest::cmdSetPartsNum ) );

	Texture * Tex = TNP[2];

	if ( NULL != Tex && Tex->lock() ) {
		w = (int)Tex->getWidth();
		h = (int)Tex->getHeight();

		for ( y = 0; y < h; y++) {
			for ( x = 0; x < w; x++) {
				ColorA C = Tex->getPixel(x, y);

				if ( C.r > 200 && C.g > 200 && C.b > 200 )
					Tex->setPixel(x, y, ColorA( Math::randi(0, 255), Math::randi(0, 255), Math::randi(0, 255), C.a ) );
				else
					Tex->setPixel(x, y, ColorA( Math::randi(200, 255), Math::randi(200, 255), Math::randi(200, 255), C.a ) );
			}
		}

		Tex->unlock(false, true);
	}

	Cursor[0] = TF->loadFromFile( MyPath + "cursors/cursor.tga" );
	CursorP[0] = TF->getTexture( Cursor[0] );

	CursorManager * CurMan = mWindow->getCursorManager();
	CurMan->setVisible( false );
	CurMan->setVisible( true );
	CurMan->set( EE::Window::SYS_CURSOR_HAND );
	CurMan->setGlobalCursor( EE_CURSOR_ARROW, CurMan->add( CurMan->create( CursorP[0], Vector2i( 1, 1 ), "cursor_special" ) ) );
	CurMan->set( EE_CURSOR_ARROW );

	CL1.addFrame( TN[2] );
	CL1.setPosition( 500, 400 );
	CL1.setScale( 0.5f );

	CL2.addFrame(TN[0], Sizef(96, 96) );
	CL2.setColor( ColorA( 255, 255, 255, 255 ) );

	mTGL = eeNew( TextureAtlasLoader, ( MyPath + "atlases/bnb" + EE_TEXTURE_ATLAS_EXTENSION ) );

	mBlindy.addFramesByPattern( "rn" );
	mBlindy.setPosition( 320.f, 0.f );

	mBoxSprite = eeNew( Sprite, ( GlobalTextureAtlas::instance()->add( eeNew( SubTexture, ( TN[3], "ilmare" ) ) ) ) );
	mCircleSprite = eeNew( Sprite, ( GlobalTextureAtlas::instance()->add( eeNew( SubTexture, ( TN[1], "thecircle" ) ) ) ) );

	eePRINTL( "Textures loading time: %4.3f ms.", TE.getElapsed().asMilliseconds() );

	Map.loadFromFile( MyPath + "maps/test.eem" );
	Map.setDrawGrid( false );
	Map.setClipedArea( false );
	Map.setDrawBackground( false );
	Map.setViewSize( mWindow->getSize() );

	eePRINTL( "Map creation time: %4.3f ms.", TE.getElapsed().asMilliseconds() );
}

void EETest::run() {
	particlesThread();
}

void EETest::particlesThread() {
	while ( mWindow->isRunning() ) {
		updateParticles();
		Sys::sleep(10);
	}
}

void EETest::updateParticles() {
	if ( MultiViewportMode || Screen == 2 ) {
		PSElapsed = cElapsed.getElapsed();

		for ( Uint8 i = 0; i < PS.size(); i++ )
			PS[i].update( PSElapsed );
	}
}

void EETest::screen1() {
	Map.update();
	Map.draw();
}

void EETest::screen2() {
	if ( mResLoad.isLoaded() ) {
		Texture * TexLoaded = TF->getByName( "1.jpg" );

		if ( NULL != TexLoaded )
			TexLoaded->draw( 0, 0 );
	}

	if ( KM->isMouseLeftPressed() )
		TNP[3]->drawEx( 0.f, 0.f, (Float)mWindow->getWidth(), (Float)mWindow->getHeight() );

	Batch.setTexture( TNP[2] );
	Batch.quadsBegin();
	Batch.quadsSetColor( ColorA(150,150,150,100) );
	Batch.quadsSetSubset( 0.0f, 0.0f, 0.5f, 0.5f );

	Batch.setBatchRotation( ang );
	Batch.setBatchScale( scale );
	Batch.setBatchCenter( Vector2f( HWidth, HHeight ) );

	Float aX = HWidth - 256.f;
	Float aY = HHeight - 256.f;
	Quad2f TmpQuad(
		Vector2f( aX	   , aY 		),
		Vector2f( aX	   , aY + 32.f  ),
		Vector2f( aX + 32.f, aY + 32.f  ),
		Vector2f( aX + 32.f, aY 		)
	);
	TmpQuad.rotate( ang, Vector2f( aX + 16.f, aY + 16.f ) );

	for ( Uint32 z = 0; z < 16; z++ ) {
		for ( Uint32 y = 0; y < 16; y++ ) {
			Float tmpx = (Float)z * 32.f;
			Float tmpy = (Float)y * 32.f;

			Batch.batchQuadFree( TmpQuad[0].x + tmpx, TmpQuad[0].y + tmpy, TmpQuad[1].x + tmpx, TmpQuad[1].y + tmpy, TmpQuad[2].x + tmpx, TmpQuad[2].y + tmpy, TmpQuad[3].x + tmpx, TmpQuad[3].y + tmpy );
		}
	}

	Batch.draw();

	Batch.setBatchRotation( 0.0f );
	Batch.setBatchScale( 1.0f );
	Batch.setBatchCenter( Vector2f( 0, 0 ) );

	Float PlanetX = HWidth  - TNP[6]->getWidth() * 0.5f;
	Float PlanetY = HHeight - TNP[6]->getHeight() * 0.5f;

	ang+=et.asMilliseconds() * 0.1f;
	ang = (ang>=360) ? 0 : ang;

	if (scale>=1.5f) {
		scale = 1.5f;
		side = true;
	} else if (scale<=0.5f) {
		side = false;
		scale = 0.5f;
	}
	scale = (!side) ? scale+et.asMilliseconds() * 0.00025f : scale-et.asMilliseconds() * 0.00025f;

	if ( mUseShaders ) {
		mBlurFactor = ( 1.5f * 0.01f ) - ( scale * 0.01f );
		mShaderProgram->bind();
		mShaderProgram->setUniform( "blurfactor" , (float)mBlurFactor );
	}

	TNP[6]->drawFast( PlanetX, PlanetY, ang, Vector2f(scale,scale));

	if ( mUseShaders )
		mShaderProgram->unbind();

	TNP[3]->draw( HWidth - 128, HHeight, 0, Vector2f::One, ColorA(255,255,255,150), ALPHA_NORMAL, RN_ISOMETRIC);
	TNP[3]->draw( HWidth - 128, HHeight - 128, 0, Vector2f::One, ColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRIC);
	TNP[3]->draw( HWidth - 128, HHeight, 0, Vector2f::One, ColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRICVERTICAL);
	TNP[3]->draw( HWidth, HHeight, 0, Vector2f::One, ColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRICVERTICALNEGATIVE);

	alpha = (!aside) ? alpha+et.asMilliseconds() * 0.1f : alpha-et.asMilliseconds() * 0.1f;
	if (alpha>=255) {
		aside = true;
		alpha = 255;
	} else if (alpha<=0) {
		alpha = 0;
		aside = false;
	}

	ColorA Col(255,255,255,(int)alpha);
	TNP[1]->drawEx( (Float)mWindow->getWidth() - 128.f, (Float)mWindow->getHeight() - 128.f, 128.f, 128.f, ang, Vector2f::One, Col, Col, Col, Col, ALPHA_BLENDONE, RN_FLIPMIRROR);

	SP.setPosition( alpha, alpha );
	SP.draw();

	#ifndef EE_GLES
	CL1.setRenderMode( RN_ISOMETRIC );

	if ( CL1.getAABB().intersectCircle( Mousef, 80.f ) )
		CL1.setColor( ColorA(255, 0, 0, 200) );
	else
		CL1.setColor( ColorA(255, 255, 255, 200) );

	if ( Polygon2f::intersectQuad2( CL1.getQuad() , CL2.getQuad() ) ) {
		CL1.setColor( ColorA(0, 255, 0, 255) );
		CL2.setColor( ColorA(0, 255, 0, 255) );
	} else
		CL2.setColor( ColorA(255, 255, 255, 255) );

	CL1.setRotation(ang);
	CL1.setScale(scale * 0.5f);

	CL2.setPosition( (Float)Mousef.x - 64.f, (Float)Mousef.y + 128.f );
	CL2.setRotation(-ang);

	CL1.draw();
	CL2.draw();

	PR.setFillMode( DRAW_LINE );
	PR.drawRectangle( CL1.getAABB() );

	PR.drawQuad( CL1.getQuad() );
	#endif

	Ang = Ang + mWindow->getElapsed().asMilliseconds() * 0.1f;
	if (Ang > 360.f) Ang = 1.f;

	if ( ShowParticles )
		particles();

	PR.setColor( ColorA(0, 255, 0, 50) );

	Line2f Line( Vector2f(0.f, 0.f), Vector2f( (Float)mWindow->getWidth(), (Float)mWindow->getHeight() ) );
	Line2f Line2( Vector2f(Mousef.x - 80.f, Mousef.y - 80.f), Vector2f(Mousef.x + 80.f, Mousef.y + 80.f) );
	Line2f Line3( Vector2f((Float)mWindow->getWidth(), 0.f), Vector2f( 0.f, (Float)mWindow->getHeight() ) );
	Line2f Line4( Vector2f(Mousef.x - 80.f, Mousef.y + 80.f), Vector2f(Mousef.x + 80.f, Mousef.y - 80.f) );

	if ( Line.intersect( Line2 ) )
		iL1 = true;
	else
		iL1 = false;

	if ( Line3.intersect( Line4 ) )
		iL2 = true;
	else
		iL2 = false;

	if (iL1 && iL2)
		PR.setColor( ColorA(255, 0, 0, 255) );
	else if (iL1)
		PR.setColor( ColorA(0, 0, 255, 255) );
	else if (iL2)
		PR.setColor( ColorA(255, 255, 0, 255) );

	PR.setFillMode( DRAW_LINE );
	PR.drawCircle( Vector2f( Mousef.x, Mousef.y ), 80.f, (Uint32)(Ang/3) );
	PR.drawTriangle( Triangle2f( Vector2f( Mousef.x, Mousef.y - 10.f ), Vector2f( Mousef.x - 10.f, Mousef.y + 10.f ), Vector2f( Mousef.x + 10.f, Mousef.y + 10.f ) ) );
	PR.drawLine( Line2f( Vector2f(Mousef.x - 80.f, Mousef.y - 80.f), Vector2f(Mousef.x + 80.f, Mousef.y + 80.f) ) );
	PR.drawLine( Line2f( Vector2f(Mousef.x - 80.f, Mousef.y + 80.f), Vector2f(Mousef.x + 80.f, Mousef.y - 80.f) ) );
	PR.drawLine( Line2f( Vector2f((Float)mWindow->getWidth(), 0.f), Vector2f( 0.f, (Float)mWindow->getHeight() ) ) );
	PR.setFillMode( DRAW_FILL );
	PR.drawQuad( Quad2f( Vector2f(0.f, 0.f), Vector2f(0.f, 100.f), Vector2f(150.f, 150.f), Vector2f(200.f, 150.f) ), ColorA(220, 240, 0, 125), ColorA(100, 0, 240, 125), ColorA(250, 50, 25, 125), ColorA(50, 150, 150, 125) );
	PR.setFillMode( DRAW_LINE );
	PR.drawRectangle( Rectf( Vector2f( Mousef.x - 80.f, Mousef.y - 80.f ), Sizef( 160.f, 160.f ) ), 45.f );
	PR.drawLine( Line2f( Vector2f(0.f, 0.f), Vector2f( (Float)mWindow->getWidth(), (Float)mWindow->getHeight() ) ) );

	TNP[3]->drawQuadEx( Quad2f( Vector2f(0.f, 0.f), Vector2f(0.f, 100.f), Vector2f(150.f, 150.f), Vector2f(200.f, 150.f) ), Vector2f(), ang, Vector2f(scale,scale), ColorA(220, 240, 0, 125), ColorA(100, 0, 240, 125), ColorA(250, 50, 25, 125), ColorA(50, 150, 150, 125) );

	WP.update( et );
	PR.setColor( ColorA(0, 255, 0, 255) );
	PR.drawPoint( WP.getPos(), 10.f );
}

void EETest::screen3() {
	if (AnimVal>=300.0f) {
		AnimVal = 300.0f;
		AnimSide = true;
	} else if (AnimVal<=0.5f) {
		AnimVal = 0.5f;
		AnimSide = false;
	}
	AnimVal = (!AnimSide) ? AnimVal+et.asMilliseconds() * 0.1f : AnimVal-et.asMilliseconds() * 0.1f;

	Batch.setTexture( TNP[3] );
	Batch.lineLoopBegin();
	for ( Float j = 0; j < 360; j++ ) {
		Batch.batchLineLoop( HWidth + 350 * Math::sinAng(j), HHeight + 350 * Math::cosAng(j), HWidth + AnimVal * Math::sinAng(j+1), HHeight + AnimVal * Math::cosAng(j+1) );
	}
	Batch.draw();
}

void EETest::screen4() {
	if ( NULL != mFBO ) {
		mFBO->bind();
		mFBO->clear();
	}

	if ( NULL != mVBO ) {
		mBlindy.setPosition( 128-16, 128-16 );
		mBlindy.draw();

		mVBO->bind();
		mVBO->draw();
		mVBO->unbind();

		mFBOText.setAlign( TEXT_ALIGN_CENTER );
		mFBOText.draw( 128.f - (Float)(Int32)( mFBOText.getTextWidth() * 0.5f ), 25.f - (Float)(Int32)( mFBOText.getTextHeight() * 0.5f ) );
	}

	if ( NULL != mFBO ) {
		mFBO->unbind();

		if ( NULL != mFBO->getTexture() ) {
			mFBO->getTexture()->draw( (Float)mWindow->getWidth() * 0.5f - (Float)mFBO->getWidth() * 0.5f, (Float)mWindow->getHeight() * 0.5f - (Float)mFBO->getHeight() * 0.5f, Ang );
			GlobalBatchRenderer::instance()->draw();
		}
	}
}

void EETest::screen5() {

}

void EETest::render() {
	HWidth = mWindow->getWidth() * 0.5f;
	HHeight = mWindow->getHeight() * 0.5f;

	if ( Sys::getTicks() - lasttick >= 50 ) {
		lasttick = Sys::getTicks();
		#ifdef EE_DEBUG
		mInfo = String::strFormated( "EE - FPS: %d Frame Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s\nApp Memory Usage: %s\nApp Peak Memory Usage: %s",
							mWindow->getFPS(),
							et.asMilliseconds(),
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							FileSystem::sizeToString( TF->getTextureMemorySize() ).c_str(),
							FileSystem::sizeToString( (Uint32)MemoryManager::getTotalMemoryUsage() ).c_str(),
							FileSystem::sizeToString( (Uint32)MemoryManager::getPeakMemoryUsage() ).c_str()
						);
		#else
		mInfo = String::strFormated( "EE - FPS: %d Elapsed Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s",
							mWindow->getFPS(),
							et.asMilliseconds(),
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							FileSystem::sizeToString( TF->getTextureMemorySize() ).c_str()
						);
		#endif

		mInfoText.setString( mInfo );

		if ( mWindow->getClearColor().r == 0 ) {
			mInfoText.setFillColor( ColorA(255,255,255,255) );
			mInfoText.setOutlineColor( ColorA(0,0,0,255) );
		} else {
			mInfoText.setFillColor( ColorA(0,0,0,255) );
			mInfoText.setOutlineColor( ColorA(255,255,255,255) );
		}
	}

	if ( !MultiViewportMode ) {
		Scenes[ Screen ]();
	} else {
		Views[0].setView( 0, 0, mWindow->getWidth(), static_cast<Uint32>( HHeight ) );
		Views[1].setView( 0, static_cast<Int32> ( HHeight ), mWindow->getWidth(), static_cast<Uint32>( HHeight ) );

		mWindow->setView( Views[1] );
		Mouse = KM->getMousePosFromView( Views[1] );
		Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );
		screen2();

		mWindow->setView( Views[0] );
		Mouse = KM->getMousePosFromView( Views[0] );
		Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );
		screen1();

		mWindow->setView( mWindow->getDefaultView() );
		mWindow->clipEnable( (Int32)HWidth - 320, (Int32)HHeight - 240, 640, 480 );
		screen3();
		mWindow->clipDisable();
	}

	ColorA ColRR1( 150, 150, 150, 220 );
	ColorA ColRR4( 150, 150, 150, 220 );
	ColorA ColRR2( 100, 100, 100, 220 );
	ColorA ColRR3( 100, 100, 100, 220 );

	mEEText.setAlign( TEXT_ALIGN_CENTER );

	PR.setColor( ColorA(150, 150, 150, 220) );
	PR.setFillMode( DRAW_FILL );
	PR.drawRectangle(
				Rectf(
					Vector2f(
						0.f,
						(Float)mWindow->getHeight() - mEEText.getTextHeight()
					),
					Vector2f(
						mEEText.getTextWidth(),
						mEEText.getTextHeight()
					)
				),
				ColRR1, ColRR2, ColRR3, ColRR4
	);

	mEEText.draw( 0.f, (Float)mWindow->getHeight() - mEEText.getTextHeight() );

	mInfoText.draw( 6.f, 6.f );

	UIManager::instance()->draw();
	UIManager::instance()->update();

	Con.draw();
}

void EETest::input() {
	KM->update();
	JM->update();

	Mouse = KM->getMousePos();
	Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );

	if ( KM->isKeyUp( KEY_F1 ) )
		Graphics::ShaderProgramManager::instance()->reload();

	if ( !mWindow->isVisible() ) {
		mWasMinimized = true;

		mWindow->setFrameRateLimit( 10 );

		if ( mMusEnabled && Mus->getState() == Sound::Playing )
			Mus->pause();

	} else {
		if ( mLastFPSLimit != mWindow->getFrameRateLimit() && !mWasMinimized )
			mLastFPSLimit = mWindow->getFrameRateLimit();

		if ( mWasMinimized ) {
			mWasMinimized = false;

			if ( !mWindow->isWindowed() )
				KM->grabInput( true );
		}

		mWindow->setFrameRateLimit( mLastFPSLimit );

		if ( mMusEnabled && Mus->getState() == Sound::Paused )
			Mus->play();
	}

	if ( KM->isKeyDown( KEY_ESCAPE ) )
		mWindow->close();

	if ( KM->isKeyUp( KEY_F1 ) )
		MultiViewportMode = !MultiViewportMode;

	if ( KM->isAltPressed() && KM->isKeyUp( KEY_C ) )
		mWindow->centerToScreen();

	if ( KM->isAltPressed() && KM->isKeyUp( KEY_M ) && !Con.isActive() ) {
		if ( !mWindow->isMaximized() )
			mWindow->maximize();
	}

	if ( KM->isKeyUp(KEY_F4) )
		TF->reloadAllTextures();

	if ( KM->isAltPressed() && KM->isKeyUp( KEY_RETURN ) ) {
		if ( mWindow->isWindowed() ) {
			mWindow->setSize( mWindow->getDesktopResolution().getWidth(), mWindow->getDesktopResolution().getHeight(), false );
		} else {
			mWindow->toggleFullscreen();
		}
	}

	if ( KM->grabInput() ) {
		if ( KM->isAltPressed() && KM->isKeyDown( KEY_TAB ) ) {
			mWindow->minimize();

			if ( KM->grabInput() )
				KM->grabInput( false );
		}
	}

	if ( KM->isControlPressed() && KM->isKeyUp(KEY_G) )
		KM->grabInput(  !KM->grabInput() );

	if ( KM->isKeyUp( KEY_F3 ) || KM->isKeyUp( KEY_WORLD_26 ) || KM->isKeyUp( KEY_BACKSLASH ) ) {
		Con.toggle();
	}

	if ( KM->isKeyUp(KEY_1) && KM->isControlPressed() )
		setScreen( 0 );

	if ( KM->isKeyUp(KEY_2) && KM->isControlPressed() )
		setScreen( 1 );

	if ( KM->isKeyUp(KEY_3) && KM->isControlPressed() )
		setScreen( 2 );

	if ( KM->isKeyUp(KEY_4) && KM->isControlPressed() )
		setScreen( 3 );

	if ( KM->isKeyUp(KEY_5) && KM->isControlPressed() )
		setScreen( 4 );

	if ( KM->isKeyUp(KEY_6) && KM->isControlPressed() )
		setScreen( 5 );

	Joystick * Joy = JM->getJoystick(0);

	if ( mJoyEnabled && NULL != Joy ) {
		if ( Joy->isButtonDown(0) )		KM->injectButtonPress(EE_BUTTON_LEFT);
		if ( Joy->isButtonDown(1) )		KM->injectButtonPress(EE_BUTTON_RIGHT);
		if ( Joy->isButtonDown(2) )		KM->injectButtonPress(EE_BUTTON_MIDDLE);
		if ( Joy->isButtonUp(0) )		KM->injectButtonRelease(EE_BUTTON_LEFT);
		if ( Joy->isButtonUp(1) )		KM->injectButtonRelease(EE_BUTTON_RIGHT);
		if ( Joy->isButtonUp(2) )		KM->injectButtonRelease(EE_BUTTON_MIDDLE);
		if ( Joy->isButtonUp(3) )		KM->injectButtonRelease(EE_BUTTON_WHEELUP);
		if ( Joy->isButtonUp(7) )		KM->injectButtonRelease(EE_BUTTON_WHEELDOWN);
		if ( Joy->isButtonUp(4) )		setScreen( 0 );
		if ( Joy->isButtonUp(5) )		setScreen( 1 );
		if ( Joy->isButtonUp(6) )		setScreen( 2 );

		Float aX = Joy->getAxis( AXIS_X );
		Float aY = Joy->getAxis( AXIS_Y );

		if ( 0 != aX || 0 != aY ) {
			double rE = mWindow->getElapsed().asMilliseconds();
			mAxisX += aX * rE;
			mAxisY += aY * rE;
		}

		if ( ( mAxisX != 0 && ( mAxisX >= 1.f || mAxisX <= -1.f ) ) || ( mAxisY != 0 && ( mAxisY >= 1.f || mAxisY <= -1.f )  ) ) {
			Float nmX = Mousef.x + mAxisX;
			Float nmY = Mousef.y + mAxisY;

			nmX = eemax<Float>( nmX, 0 );
			nmY = eemax<Float>( nmY, 0 );
			nmX = eemin( nmX, (Float)EE->getCurrentWindow()->getWidth() );
			nmY = eemin( nmY, (Float)EE->getCurrentWindow()->getHeight() );

			KM->injectMousePos( (Int32)nmX, (Int32)nmY );

			nmX -= (Int32)nmX;
			nmY -= (Int32)nmY;

			mAxisX 		= nmX;
			mAxisY	 	= nmY;
		}
	}

	switch (Screen) {
		case 0:
			if ( KM->isKeyUp( KEY_R ) ) {
				physicsDestroy();
				physicsCreate();
			}

			if ( KM->isKeyUp( KEY_1 ) )
				changeDemo( 0 );

			if ( KM->isKeyUp( KEY_2 ) )
				changeDemo( 1 );
		case 1:
			if ( NULL != Joy ) {
				Uint8 hat = Joy->getHat();

				if ( HAT_LEFT == hat || HAT_LEFTDOWN == hat || HAT_LEFTUP == hat )
					Map.move( (mWindow->getElapsed().asMilliseconds() * 0.2f), 0 );

				if ( HAT_RIGHT == hat || HAT_RIGHTDOWN == hat || HAT_RIGHTUP == hat )
					Map.move( -mWindow->getElapsed().asMilliseconds() * 0.2f, 0 );

				if ( HAT_UP == hat || HAT_LEFTUP == hat || HAT_RIGHTUP == hat )
					Map.move( 0, (mWindow->getElapsed().asMilliseconds() * 0.2f) );

				if ( HAT_DOWN == hat || HAT_LEFTDOWN == hat || HAT_RIGHTDOWN == hat )
					Map.move( 0, -mWindow->getElapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyDown(KEY_LEFT) ) {
				Map.move( mWindow->getElapsed().asMilliseconds() * 0.2f, 0 );
			}

			if ( KM->isKeyDown(KEY_RIGHT) ) {
				Map.move( -mWindow->getElapsed().asMilliseconds() * 0.2f, 0 );
			}

			if ( KM->isKeyDown(KEY_UP) ) {
				Map.move( 0, mWindow->getElapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyDown(KEY_DOWN) ) {
				Map.move( 0, -mWindow->getElapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyUp(KEY_F8) )
				Map.reset();

			break;
		case 2:
			if ( KM->isKeyUp(KEY_S) )
				SP.setRepetitions(1);

			if ( KM->isKeyUp(KEY_A) )
				SP.setRepetitions(-1);

			if ( KM->isKeyUp(KEY_D) )
				SP.setReverseAnimation( !SP.getReverseAnimation() );

			if ( KM->isMouseRightPressed() )
				DrawBack = true;
			else
				DrawBack = false;

			if ( KM->isKeyUp( KEY_P ) )
				SndMng.play( "mysound" );

			if ( KM->isControlPressed() && KM->isKeyUp(KEY_P) ) {
				ShowParticles = !ShowParticles;
			}

			break;
	}
}

void EETest::update() {
	mWindow->clear();

	et = mWindow->getElapsed();

	input();

	mResLoad.update();

	if ( mFontLoader.isLoaded() ) {
		render();
	} else {
		mFontLoader.update();
	}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	updateParticles();
#endif

	if ( KM->isKeyUp(KEY_F12) ) mWindow->takeScreenshot( MyPath + "screenshots/" ); //After render and before Display

	mWindow->display(false);
}

void EETest::process() {
	init();

	if ( NULL != mWindow && mWindow->isOpen() ) {
		TestInstance = this;

		mWindow->runMainLoop( &mainLoop );
	}

	end();
}

void EETest::particlesCallback( Particle * P, ParticleSystem * Me ) {
	Float x, y, radio;
	Vector2f MePos( Me->getPosition() );

	radio = (Math::randf(1.f, 1.2f) + sin( 20.0f / P->getId() )) * 24;
	x = MePos.x + radio * cos( (Float)P->getId() );
	y = MePos.y + radio * sin( (Float)P->getId() );
	P->reset(x, y, Math::randf(-10.f, 10.f), Math::randf(-10.f, 10.f), Math::randf(-10.f, 10.f), Math::randf(-10.f, 10.f));
	P->setColor( ColorAf(1.f, 0.6f, 0.3f, 1.f), 0.02f + Math::randf() * 0.3f );
}

void EETest::particles() {
	PS[0].setPosition( Mousef );

	if ( DrawBack )
		PS[1].setPosition( Mousef );

	PS[2].setPosition( HWidth, HHeight );
	PS[3].setPosition(  Math::cosAng(Ang) * 220.f + HWidth + Math::randf(0.f, 10.f),  Math::sinAng(Ang) * 220.f + HHeight + Math::randf(0.f, 10.f) );
	PS[4].setPosition( -Math::cosAng(Ang) * 220.f + HWidth + Math::randf(0.f, 10.f), -Math::sinAng(Ang) * 220.f + HHeight + Math::randf(0.f, 10.f) );

	for ( Uint32 i = 0; i < PS.size(); i++ )
		PS[i].draw();
}

#define GRABABLE_MASK_BIT (1<<31)
#define NOT_GRABABLE_MASK (~GRABABLE_MASK_BIT)

void EETest::createJointAndBody() {
	#ifndef EE_PLATFORM_TOUCH
	mMouseJoint	= NULL;
	mMouseBody	= Body::New( INFINITY, INFINITY );
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		mMouseJoint[i] = NULL;
		mMouseBody[i] = Body::New( INFINITY, INFINITY );
	}
	#endif
}

void EETest::demo1Create() {
	createJointAndBody();

	Shape::resetShapeIdCounter();

	mSpace = Physics::Space::New();
	mSpace->setGravity( cVectNew( 0, 100 ) );
	mSpace->setSleepTimeThreshold( 0.5f );

	Body *body, *statiBody = mSpace->getStaticBody();
	Shape * shape;

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( 0, mWindow->getHeight() ), cVectNew( mWindow->getWidth(), mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( mWindow->getWidth(), 0 ), cVectNew( mWindow->getWidth(), mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( 0, 0 ), cVectNew( 0, mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( 0, 0 ), cVectNew( mWindow->getWidth(), 0 ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	Float hw = mWindow->getWidth() / 2;

	for(int i=0; i<14; i++){
		for(int j=0; j<=i; j++){
			body = mSpace->addBody( Body::New( 1.0f, Moment::forBox( 1.0f, 30.0f, 30.0f ) ) );
			body->setPos( cVectNew( hw + j * 32 - i * 16, 100 + i * 32 ) );

			//shape = mSpace->AddShape( ShapePolySprite::New( body, 30.f, 30.f, mBoxSprite ) );
			shape = mSpace->addShape( ShapePoly::New( body, 30.f, 30.f ) );
			shape->setE( 0.0f );
			shape->setU( 0.8f );
		}
	}

	cpFloat radius = 15.0f;

	body = mSpace->addBody( Body::New( 10.0f, Moment::forCircle( 10.0f, 0.0f, radius, cVectZero ) ) );
	body->setPos( cVectNew( hw, mWindow->getHeight() - radius - 5 ) );

	//shape = mSpace->AddShape( ShapeCircleSprite::New( body, radius, cVectZero, mCircleSprite ) );
	shape = mSpace->addShape( ShapeCircle::New( body, radius, cVectZero ) );
	shape->setE( 0.0f );
	shape->setU( 0.9f );
}

void EETest::demo1Update() {

}

void EETest::destroyBody() {
	#ifndef EE_PLATFORM_TOUCH
	eeSAFE_DELETE( mMouseBody );
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		eeSAFE_DELETE( mMouseBody[i] );
	}
	#endif
}

void EETest::demo1Destroy() {
	destroyBody();

	eeSAFE_DELETE( mSpace );
}

cpBool EETest::blockerBegin( Arbiter *arb, Space *space, void *unused ) {
	Shape * a, * b;
	arb->getShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->getData();

	emitter->blocked++;

	return cpFalse; // Return values from sensors callbacks are ignored,
}

void EETest::blockerSeparate( Arbiter *arb, Space * space, void *unused ) {
	Shape * a, * b;
	arb->getShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->getData();

	emitter->blocked--;
}

void EETest::postStepRemove( Space *space, void * tshape, void * unused ) {
	Shape * shape = reinterpret_cast<Shape*>( tshape );

	#ifndef EE_PLATFORM_TOUCH
	if ( NULL != mMouseJoint && ( mMouseJoint->getA() == shape->getBody() || mMouseJoint->getB() == shape->getBody() ) ) {
		mSpace->removeConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( NULL != mMouseJoint[i] && ( mMouseJoint[i]->getA() == shape->getBody() || mMouseJoint[i]->getB() == shape->getBody() ) ) {
			mSpace->removeConstraint( mMouseJoint[i] );
			eeSAFE_DELETE( mMouseJoint[i] );
		}
	}
	#endif

	mSpace->removeBody( shape->getBody() );
	mSpace->removeShape( shape );
	Shape::Free( shape, true );
}

cpBool EETest::catcherBarBegin(Arbiter *arb, Physics::Space *space, void *unused) {
	Shape * a, * b;
	arb->getShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->getData();

	emitter->queue++;

	mSpace->addPostStepCallback( cb::Make3( this, &EETest::postStepRemove ), b, NULL );

	return cpFalse;
}

void EETest::demo2Create() {
	createJointAndBody();

	Shape::resetShapeIdCounter();

	mSpace = Physics::Space::New();
	mSpace->setIterations( 10 );
	mSpace->setGravity( cVectNew( 0, 100 ) );

	Body * statiBody = mSpace->getStaticBody();
	Shape * shape;

	emitterInstance.queue = 5;
	emitterInstance.blocked = 0;
	emitterInstance.position = cVectNew( mWindow->getWidth() / 2 , 150);

	shape = mSpace->addShape( ShapeCircle::New( statiBody, 15.0f, emitterInstance.position ) );
	shape->setSensor( 1 );
	shape->setCollisionType( BLOCKING_SENSOR_TYPE );
	shape->setData( &emitterInstance );

	// Create our catch sensor to requeue the balls when they reach the bottom of the screen
	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew(-4000, 600), cVectNew(4000, 600), 15.0f ) );
	shape->setSensor( 1 );
	shape->setCollisionType( CATCH_SENSOR_TYPE );
	shape->setData( &emitterInstance );

	Space::CollisionHandler handler;
	handler.a			= BLOCKING_SENSOR_TYPE;
	handler.b			= BALL_TYPE;
	handler.begin		= cb::Make3( this, &EETest::blockerBegin );
	handler.separate	= cb::Make3( this, &EETest::blockerSeparate );
	mSpace->addCollisionHandler( handler );

	handler.Reset(); // Reset all the values and the callbacks ( set the callbacks as !IsSet()

	handler.a			= CATCH_SENSOR_TYPE;
	handler.b			= BALL_TYPE;
	handler.begin		= cb::Make3( this, &EETest::catcherBarBegin );
	mSpace->addCollisionHandler( handler );
}

void EETest::demo2Update() {
	if( !emitterInstance.blocked && emitterInstance.queue ){
		emitterInstance.queue--;

		Body * body = mSpace->addBody( Body::New( 1.0f, Moment::forCircle(1.0f, 15.0f, 0.0f, cVectZero ) ) );
		body->setPos( emitterInstance.position );
		body->setVel( cVectNew( Math::randf(-1,1), Math::randf(-1,1) ) * (cpFloat)100 );

		Shape *shape = mSpace->addShape( ShapeCircle::New( body, 15.0f, cVectZero ) );
		shape->setCollisionType( BALL_TYPE );
	}
}

void EETest::demo2Destroy() {
	destroyBody();
	eeSAFE_DELETE( mSpace );
}

void EETest::changeDemo( Uint32 num ) {
	if ( num < mDemo.size() ) {
		if ( eeINDEX_NOT_FOUND != mCurDemo )
			mDemo[ mCurDemo ].destroy();

		mCurDemo = num;

		mDemo[ mCurDemo ].init();
	}
}

void EETest::physicsCreate() {
	PhysicsManager::createSingleton();
	PhysicsManager * PM = PhysicsManager::instance();
	PhysicsManager::DrawSpaceOptions * DSO = PM->getDrawOptions();

	DSO->DrawBBs			= false;
	DSO->DrawShapes			= true;
	DSO->CollisionPointSize	= 0;
	DSO->BodyPointSize		= 0;
	DSO->LineThickness		= 1;

	mDemo.clear();

	physicDemo demo;

	demo.init		= cb::Make0( this, &EETest::demo1Create );
	demo.update		= cb::Make0( this, &EETest::demo1Update );
	demo.destroy	= cb::Make0( this, &EETest::demo1Destroy );
	mDemo.push_back( demo );

	demo.init		= cb::Make0( this, &EETest::demo2Create );
	demo.update		= cb::Make0( this, &EETest::demo2Update );
	demo.destroy	= cb::Make0( this, &EETest::demo2Destroy );
	mDemo.push_back( demo );

	changeDemo( 0 );
}

void EETest::physicsUpdate() {
	#ifndef EE_PLATFORM_TOUCH
	mMousePoint = cVectNew( KM->getMousePosf().x, KM->getMousePosf().y );
	cVect newPoint = tovect( cpvlerp( tocpv( mMousePoint_last ), tocpv( mMousePoint ), 0.25 ) );
	mMouseBody->setPos( newPoint );
	mMouseBody->setVel( ( newPoint - mMousePoint_last ) * (cpFloat)mWindow->getFPS() );
	mMousePoint_last = newPoint;

	if ( KM->isMouseLeftPressed() ) {
		if ( NULL == mMouseJoint ) {
			cVect point = cVectNew( KM->getMousePosf().x, KM->getMousePosf().y );

			Shape * shape = mSpace->pointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

			if( NULL != shape ){
				mMouseJoint = eeNew( PivotJoint, ( mMouseBody, shape->getBody(), cVectZero, shape->getBody()->world2Local( point ) ) );

				mMouseJoint->setMaxForce( 50000.0f );
				mSpace->addConstraint( mMouseJoint );
			}
		}
	} else if ( NULL != mMouseJoint ) {
		mSpace->removeConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		InputFinger * Finger = KM->getFingerIndex(i);
		mMousePoint[i] = cVectNew( Finger->x, Finger->y );
		cVect newPoint = tovect( cpvlerp( tocpv( mMousePoint_last[i] ), tocpv( mMousePoint[i] ), 0.25 ) );
		mMouseBody[i]->setPos( newPoint );
		mMouseBody[i]->setVel( ( newPoint - mMousePoint_last[i] ) * (cpFloat)mWindow->getFPS() );
		mMousePoint_last[i] = newPoint;

		if ( Finger->isDown() ) {
			if ( NULL == mMouseJoint[i] ) {
				cVect point = cVectNew( Finger->x, Finger->y );

				Shape * shape = mSpace->pointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

				if( NULL != shape ){
					mMouseJoint[i] = eeNew( PivotJoint, ( mMouseBody[i], shape->getBody(), cVectZero, shape->getBody()->world2Local( point ) ) );

					mMouseJoint[i]->setMaxForce( 50000.0f );
					mSpace->addConstraint( mMouseJoint[i] );
				}
			}
		} else if ( NULL != mMouseJoint[i] ) {
			mSpace->removeConstraint( mMouseJoint[i] );
			eeSAFE_DELETE( mMouseJoint[i] );
		}
	}
	#endif

	mDemo[ mCurDemo ].update();
	mSpace->update();
	mSpace->draw();
}

void EETest::physicsDestroy() {
	mDemo[ mCurDemo ].destroy();
}

void EETest::end() {
	wait();

	physicsDestroy();

	eeSAFE_DELETE( Mus );
	eeSAFE_DELETE( mTGL );
	eeSAFE_DELETE( mFBO );
	eeSAFE_DELETE( mVBO );
	eeSAFE_DELETE( mBoxSprite );
	eeSAFE_DELETE( mCircleSprite );
	eeSAFE_DELETE( PakTest );

	Log::instance()->save();

	Engine::destroySingleton();
}

}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	Demo_Test::EETest * Test = eeNew( Demo_Test::EETest, () );

	Test->process();

	eeDelete( Test );

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
