#include "test.hpp"

Demo_Test::EETest* TestInstance = NULL;

static void mainLoop() {
	TestInstance->update();
}

namespace Demo_Test {

class UIBlurredWindow : public UIWindow {
  public:
	static UIBlurredWindow* New( ShaderProgram* blurShader ) {
		return eeNew( UIBlurredWindow, ( blurShader ) );
	}

	explicit UIBlurredWindow( ShaderProgram* blurShader ) :
		UIWindow(), mBlurShader( blurShader ), mFboBlur( NULL ) {}

	~UIBlurredWindow() { eeSAFE_DELETE( mFboBlur ); }

  protected:
	ShaderProgram* mBlurShader;
	FrameBuffer* mFboBlur;

	void preDraw() {
		if ( !ownsFrameBuffer() )
			return;

		FrameBuffer* curFBO = getSceneNode()->getFrameBuffer();

		if ( NULL != curFBO && NULL != curFBO->getTexture() && NULL != mBlurShader ) {
			static int fboDiv = 2;

			if ( NULL == mFboBlur ) {
				mFboBlur = FrameBuffer::New( mSize.x / fboDiv, mSize.y / fboDiv );
			} else if ( mFboBlur->getSize().getWidth() != (int)( mSize.x / fboDiv ) ||
						mFboBlur->getSize().getHeight() != (int)( mSize.y / fboDiv ) ) {
				mFboBlur->resize( mSize.x / fboDiv, mSize.y / fboDiv );
			}

			TextureRegion textureRegion( curFBO->getTexture()->getTextureId(),
										 Rect( mScreenPos.x, mScreenPos.y, mScreenPos.x + mSize.x,
											   mScreenPos.y + mSize.y ) );

			RGB cc = getSceneNode()->getWindow()->getClearColor();
			mFboBlur->setClearColor( ColorAf( cc.r / 255.f, cc.g / 255.f, cc.b / 255.f, 0 ) );
			mFboBlur->bind();
			mFboBlur->clear();
			textureRegion.draw( Vector2f( 0, 0 ), mFboBlur->getSizef() );
			mFboBlur->unbind();

			mBlurShader->bind();

			mBlurShader->setUniform( "radius", 16.f );
			mBlurShader->setUniform( "dir", (Int32)0 );
			mBlurShader->setUniform( "textureRes", mFboBlur->getSizef() );

			mFboBlur->bind();
			mFboBlur->getTexture()->draw( Vector2f( 0, 0 ), mFboBlur->getSizef() );
			mFboBlur->unbind();

			mBlurShader->setUniform( "dir", (Int32)1 );
			mBlurShader->setUniform( "textureRes", mFboBlur->getSizef() );

			mFboBlur->bind();
			mFboBlur->getTexture()->draw( Vector2f( 0, 0 ), mFboBlur->getSizef() );
			mFboBlur->unbind();

			mBlurShader->unbind();

			mFboBlur->getTexture()->draw( Vector2f( mScreenPos.x, mScreenPos.y ),
										  Sizef( mSize.x, mSize.y ) );
		}
	}
};

void EETest::init() {
	EE = Engine::instance();

	Log::instance()->setLiveWrite( true );
	Log::instance()->setConsoleOutput( true );

	mTranslator.loadFromString( "<resources language='en'>"
								"		<string name='app_name'>eepp</string>"
								"		<string name='formatted'>Test %d %s</string>"
								"		<string name='test_item'>Test Item 2</string>"
								"</resources>" );

	DrawBack = false;
	MultiViewportMode = false;

	side = aside = true;
	ShowParticles = true;
	scale = 1.0f;
	Ang = ang = alpha = 0;
	lasttick = 0;
	AnimVal = 0.5f;
	mLastFPSLimit = 0;
	mWasMinimized = false;

	mAxisX = 0;
	mAxisY = 0;
	mCurDemo = eeINDEX_NOT_FOUND;
	mMapEditor = NULL;
	mETGEditor = NULL;
	mColorPicker = NULL;
	Mus = NULL;
	mUIWindow = NULL;
	mTerrainBut = NULL;
	mShowMenu = NULL;
	mTerrainUp = true;
	relLay = NULL;

	MyPath = Sys::getProcessPath() + "assets/";

	IniFile Ini( MyPath + "ee.ini" );

	PartsNum = Ini.getValueI( "EEPP", "ParticlesNum", 1000 );
	mUseShaders = Ini.getValueB( "EEPP", "UseShaders", false );
	mJoyEnabled = Ini.getValueB( "EEPP", "JoystickEnabled", false );
	mDebugUI = Ini.getValueB( "EEPP", "DebugUI", false );

#if defined( EE_PLATFORM_TOUCH )
	mJoyEnabled = false;
#endif

	mMusEnabled = Ini.getValueB( "EEPP", "Music", false );
	mLastFPSLimit = Ini.getValueI( "EEPP", "FrameRateLimit", 0 );
	Int32 StartScreen = Ini.getValueI( "EEPP", "StartScreen", 5 );

	WindowSettings WinSettings = EE->createWindowSettings( &Ini );
	ContextSettings ConSettings = EE->createContextSettings( &Ini );

	mWindow = EE->createWindow( WinSettings, ConSettings );

	if ( NULL != mWindow && mWindow->isOpen() ) {
		setScreen( StartScreen );

		mWindow->setTitle( "eepp - Test Application" );
		mWindow->pushResizeCallback( cb::Make1( this, &EETest::onWindowResize ) );

		TF = TextureFactory::instance();
		TF->allocate( 40 );

		Log = Log::instance();
		KM = mWindow->getInput();
		JM = KM->getJoystickManager();

		PS.resize( 5 );

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
			Mus = Music::New();

			if ( Mus->openFromFile( MyPath + "sounds/music.ogg" ) ) {
				Mus->setLoop( true );
				Mus->play();
			}
		}

		WP.setType( Ease::QuarticInOut );
		WP.add( Vector2f( 0, 0 ), Milliseconds( 100 ) );
		WP.add( Vector2f( 800, 0 ), Milliseconds( 100 ) );
		WP.add( Vector2f( 0, 0 ), Milliseconds( 100 ) );
		WP.add( Vector2f( 1024, 768 ), Milliseconds( 100 ) );
		WP.add( Vector2f( 0, 600 ), Milliseconds( 100 ) );
		WP.edit( 2, Vector2f( 800, 600 ), Milliseconds( 100 ) );
		WP.erase( 3 );
		WP.setLoop( true );
		WP.setDuration( Milliseconds( 5000 ) );
		WP.start();

		Batch.allocVertexs( 2048 );
		Batch.setBlendMode( BlendAdd );

		mFBO = FrameBuffer::New( 256, 256 );

		if ( NULL != mFBO )
			mFBO->setClearColor( ColorAf( 0, 0, 0, 0.5f ) );

		Polygon2f Poly = Polygon2f::createRoundedRectangle( 0, 0, 256, 50 );

		mVBO = VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_FAN );

		if ( NULL != mVBO ) {
			for ( Uint32 i = 0; i < Poly.getSize(); i++ ) {
				mVBO->addVertex( Poly[i] );
				mVBO->addColor( Color( 100 + i, 255 - i, 150 + i, 200 ) );
			}

			mVBO->compile();
		}

		physicsCreate();

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		launch();
#endif

	} else {
		Engine::destroySingleton();

		exit( 0 );
	}
}

void EETest::createUIThemeTextureAtlas() {
#if !defined( EE_DEBUG ) || defined( EE_GLES )
	return;
#endif

	std::string tgpath( MyPath + "ui/" + mThemeName );
	std::string Path( MyPath + "ui/" + mThemeName );

	if ( !FileSystem::fileExists( tgpath + EE_TEXTURE_ATLAS_EXTENSION ) ) {
		PixelDensitySize PD = PixelDensitySize::MDPI;
		if ( mThemeName.find( "2x" ) != std::string::npos )
			PD = PixelDensitySize::XHDPI;
		else if ( mThemeName.find( "1.5x" ) != std::string::npos )
			PD = PixelDensitySize::HDPI;

		TexturePacker tp( 2048, 2048, PixelDensity::toFloat( PD ), true, false, 2 );
		tp.addTexturesPath( Path );
		tp.packTextures();
		tp.save( tgpath + ".png", Image::SaveType::SAVE_TYPE_PNG );
	} else {
		TextureAtlasLoader tgl;
		tgl.updateTextureAtlas( tgpath + EE_TEXTURE_ATLAS_EXTENSION, Path );
	}
}

void EETest::loadFonts() {
	mFTE.restart();

	FontTrueType::New( "NotoSans-Regular", MyPath + "fonts/NotoSans-Regular.ttf" );
	FontTrueType::New( "monospace", MyPath + "fonts/DejaVuSansMono.ttf" );

	onFontLoaded();
}

void EETest::onFontLoaded() {
	TTF = FontManager::instance()->getByName( "NotoSans-Regular" );
	Font* monospace = FontManager::instance()->getByName( "monospace" );

	eePRINTL( "Fonts loading time: %4.3f ms.", mFTE.getElapsed().asMilliseconds() );

	eeASSERT( TTF != NULL );
	eeASSERT( monospace != NULL );

	Con.create( monospace, true );
	Con.ignoreCharOnPrompt( 186 ); // 'º'

	mBuda = String::fromUtf8(
		"El mono ve el pez en el agua y sufre. Piensa que su mundo es el único que existe, el "
		"mejor, el real. Sufre porque es bueno y tiene compasión, lo ve y piensa: \"Pobre se está "
		"ahogando no puede respirar\". Y lo saca, lo saca y se queda tranquilo, por fin lo salvé. "
		"Pero el pez se retuerce de dolor y muere. Por eso te mostré el sueño, es imposible meter "
		"el mar en tu cabeza, que es un balde." );

	createUI();

	mEEText.create( TTF, "Entropia Engine++\nCTRL + Number to change Demo Screen\nRight click to "
						 "see the PopUp Menu" );
	mEEText.setOutlineThickness( 1 );
	mEEText.setOutlineColor( Color( 0, 0, 0, 255 ) );
	mFBOText.create( TTF, "This is a VBO\nInside of a FBO" );
	mFBOText.setOutlineThickness( 1 );
	mFBOText.setOutlineColor( Color( 0, 0, 0, 255 ) );

	mInfoText.create( monospace, "", Color( 100, 100, 100, 255 ) );
	mInfoText.setOutlineThickness( 1 );
}

void EETest::createShaders() {
	mUseShaders = mUseShaders && GLi->shadersSupported();

	mShaderProgram = NULL;

	if ( mUseShaders ) {
		mBlurFactor = 0.01f;
		mShaderProgram = ShaderProgram::New( MyPath + "shaders/blur.vert",
											 MyPath + "shaders/blur.frag", "blur" );
		mBlur = ShaderProgram::New( MyPath + "shaders/blur.vert",
									MyPath + "shaders/gaussian_blur.frag", "gaussian_blur" );
	}
}

void EETest::onWinMouseUp( const Event* Event ) {
	const MouseEvent* MEvent = reinterpret_cast<const MouseEvent*>( Event );

	Node* CtrlAnim;

	if ( Event->getNode()->isType( UI_TYPE_WINDOW ) ) {
		CtrlAnim = reinterpret_cast<Node*>( Event->getNode() );
	} else {
		CtrlAnim = reinterpret_cast<Node*>( Event->getNode()->getParent() );
	}

	if ( MEvent->getFlags() & EE_BUTTON_WUMASK ) {
		CtrlAnim->setScale( CtrlAnim->getScale() + 0.1f );
	} else if ( MEvent->getFlags() & EE_BUTTON_WDMASK ) {
		CtrlAnim->setScale( CtrlAnim->getScale() - 0.1f );
	}
}

void EETest::onShowMenu( const Event* Event ) {
	UIPushButton* PB = static_cast<UIPushButton*>( Event->getNode() );

	if ( Menu->show() ) {
		Vector2f pos( Vector2f( PB->getSize().getWidth(), 0 ) );
		PB->nodeToWorld( pos );
		UIMenu::fixMenuPos( pos, Menu );
		Menu->setPixelsPosition( pos );
	}
}

void EETest::onWindowResize( EE::Window::Window* win ) {
	Map.setViewSize( win->getSize().asFloat() );
}

static std::vector<String> getTestStringArr() {
	Int32 wsize = 15;
	std::vector<String> str( wsize );

	if ( wsize ) {
		for ( Int32 i = 1; i <= wsize; i++ )
			str[i - 1] = "Test ListBox " + String::toStr( i ) + " testing it right now!";
	}

	return str;
}

void EETest::createBaseUI() {
	std::vector<String> str = getTestStringArr();

	// ActionManager test
	UINode::New()
		->setSize( 10, 10 )
		->runAction( Actions::Fade::New( 0, 255, Seconds( 5 ) ) )
		->close();

	/**/
	UIWindow* tWin = UIWindow::New();
	tWin->setSize( 530, 405 )->setPosition( 320, 240 );
	UIWindow::StyleConfig windowStyleConfig = tWin->getStyleConfig();
	windowStyleConfig.WinFlags = UI_WIN_DRAGABLE_CONTAINER | UI_WIN_SHADOW | UI_WIN_FRAME_BUFFER;
	windowStyleConfig.MinWindowSize = Sizef( 530, 405 );
	windowStyleConfig.BaseAlpha = 200;
	tWin->setStyleConfig( windowStyleConfig );

	C = static_cast<UINode*>( tWin->getContainer() );
	tWin->setVisible( false )->setEnabled( false );

	tWin->setTitle( "Controls Test" );

	tWin->addEventListener( Event::MouseUp, cb::Make1( this, &EETest::onWinMouseUp ) );
	C->addEventListener( Event::MouseUp, cb::Make1( this, &EETest::onWinMouseUp ) );

	UISprite* sprite = UISprite::New();
	sprite->setFlags( UI_AUTO_SIZE );
	sprite->setSprite( Sprite::New( "gn" ) );
	sprite->setParent( C );
	sprite->setPosition( 160, 100 );
	sprite->setIsSpriteOwner( true );

	UITextView* Text = UITextView::New();
	Text->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
		->setHorizontalAlign( UI_HALIGN_RIGHT )
		->setVerticalAlign( UI_VALIGN_TOP )
		->setParent( C )
		->setEnabled( false )
		->setSize( 320, 240 );
	Text->setText( "Turn around\nJust Turn Around\nAround!" );

	UITextInput::New()->setParent( C )->setPosition( 20, 216 )->setSize( 200, 0 );

	UIPushButton* Button = UIPushButton::New();
	Button->setParent( C )->setPosition( 225, 215 )->setSize( 90, 0 );
	Button->setIcon( mTheme->getIconByName( "ok" ) );
	Button->setText( "Click Me" );
	Button->addEventListener( Event::MouseClick, cb::Make1( this, &EETest::onButtonClick ) );
	Button->setTooltipText( "Click and see what happens..." );

	UICheckBox* Checkbox = UICheckBox::New();
	Checkbox->setParent( C )->setPosition( 130, 20 )->setSize( 80, 22 );
	Checkbox->setText( "Check Me" );

	UIRadioButton* RadioButton = UIRadioButton::New();
	RadioButton->setParent( C )->setPosition( 130, 40 )->setSize( 80, 22 );
	RadioButton->setText( "Check Me" );

	RadioButton = UIRadioButton::New();
	RadioButton->setParent( C )->setPosition( 130, 60 )->setSize( 80, 22 );
	RadioButton->setText( "Check Me 2" );

	mSlider = UISlider::New();
	mSlider->setOrientation( UIOrientation::Horizontal )
		->setParent( C )
		->setPosition( 220, 80 )
		->setSize( 80, 24 );
	mSlider->addEventListener( Event::OnValueChange,
							   cb::Make1( this, &EETest::onSliderValueChange ) );

	UISlider::New()
		->setOrientation( UIOrientation::Vertical )
		->setParent( C )
		->setPosition( 40, 110 )
		->setSize( 24, 80 );

	UISlider::New()
		->setOrientation( UIOrientation::Horizontal )
		->setParent( C )
		->setPosition( 60, 110 )
		->setSize( 80, 24 );

	UISpinBox::New()->setAllowOnlyNumbers( true )->setParent( C )->setPosition( 80, 150 )->setSize(
		80, 24 );

	mScrollBar = UIScrollBar::New();
	mScrollBar->setParent( C )->setSize( 0, 240 );
	mScrollBar->addEventListener( Event::OnValueChange, cb::Make1( this, &EETest::onValueChange ) );

	mProgressBar = UIProgressBar::New();
	mProgressBar->setParent( C )->setSize( 200, 24 )->setPosition( 20, 190 );

	mTextBoxValue = UITextView::New();
	mTextBoxValue->setParent( C )->setPosition( 20, 0 );
	mTextBoxValue->setFlags( UI_AUTO_SIZE )->setVisible( true );
	onValueChange( NULL );

	mListBox = UIListBox::New();
	mListBox->setParent( C )->setPosition( 325, 8 )->setSize( 200, 224 );
	mListBox->setFlags( UI_TOUCH_DRAG_ENABLED );
	mListBox->addListBoxItems( str );

	UIDropDownList* dropDownList = UIDropDownList::New();
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

	UIComboBox* comboBox = UIComboBox::New();
	comboBox->setParent( C )->setPosition( 20, 80 )->setSize( 100, 1 );
	comboBox->getListBox()->addListBoxItems( combostrs );
	comboBox->getListBox()->setSelected( 0 );

	UITextEdit* TextEdit = UITextEdit::New();
	TextEdit->setFlags( UI_WORD_WRAP );
	TextEdit->setParent( C )->setPosition( 5, 245 )->setSize( 315, 130 );
	TextEdit->setText( mBuda );

	UITable* genGrid = UITable::New();
	genGrid->setSmoothScroll( true )->setFlags( UI_TOUCH_DRAG_ENABLED );
	genGrid->setParent( C )->setPosition( 325, 245 )->setSize( 200, 130 );
	genGrid->setCollumnsCount( 3 )->setRowHeight( 24 );

	for ( Uint32 i = 0; i < 15; i++ ) {
		UITableCell* Cell = UITableCell::New();
		UITextView* TxtBox = UITextView::New();
		UITextInput* TxtInput = UITextInput::New();
		UIImage* TxtGfx = UIImage::New();

		Cell->setParent( genGrid->getContainer() );

		TxtGfx->setVerticalAlign( UI_VALIGN_CENTER );
		TxtGfx->setDrawable( mTheme->getIconByName( "ok" ) );
		TxtBox->setText( "Test " + String::toStr( i + 1 ) );

		Cell->setCell( 0, TxtBox );
		Cell->setCell( 1, TxtGfx );
		Cell->setCell( 2, TxtInput );

		genGrid->add( Cell );
	}

	genGrid->setCollumnWidth( 0, 50 );
	genGrid->setCollumnWidth( 1, 24 );
	genGrid->setCollumnWidth( 2, 100 );

	UIWidget* w = UIWidget::New();
	w->setParent( C )->setSize( 20, 20 )->setPosition( 260, 130 );
	w->setBackgroundColor( Color::Green );
	w->setRotation( 45 );
	// w->setBackgroundColor( UIState::StateFlagHover, Color::Yellow );
	// w->setBackgroundColor( UIState::StateFlagPressed, Color::Red );

	C = C->getParent()->asType<UINode>();

	Menu = UIPopUpMenu::New();
	Menu->add( "New", mTheme->getIconByName( "document-new" ) );

	Menu->add( "Open...", mTheme->getIconByName( "document-open" ) );
	Menu->addSeparator();
	Menu->add( "Map Editor" );
	Menu->add( "Texture Atlas Editor" );
	Menu->add( "Color Picker" );
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

	UIPopUpMenu* Menu3 = UIPopUpMenu::New();
	Menu3->add( "Hello World 1" );
	Menu3->add( "Hello World 2" );
	Menu3->add( "Hello World 3" );
	Menu3->add( "Hello World 4" );

	UIPopUpMenu* Menu2 = UIPopUpMenu::New();
	Menu2->add( "Test 1" );
	Menu2->add( "Test 2" );
	Menu2->add( "Test 3" );
	Menu2->add( "Test 4" );
	Menu2->addSubMenu( "Hello World", NULL, Menu3 );

	Menu->addSeparator();
	Menu->addSubMenu( "Sub-Menu", NULL, Menu2 );

	Menu->addSeparator();
	Menu->add( "Quit" );

	Menu->addEventListener( Event::OnItemClicked, cb::Make1( this, &EETest::onItemClick ) );
	Menu->getItem( "Quit" )->addEventListener( Event::MouseUp,
											   cb::Make1( this, &EETest::onQuitClick ) );

	SceneManager::instance()->getUISceneNode()->getRoot()->addEventListener(
		Event::MouseClick, cb::Make1( this, &EETest::onMainClick ) );

#ifdef EE_PLATFORM_TOUCH
	UISkin nSkin( "button-te" );
	nSkin.setStateDrawable(
		UIState::getStateNumber( "normal" ),
		TF->getTexture( TF->loadFromFile( MyPath + "sprites/button-te_normal.png" ) ) );
	nSkin.setStateDrawable(
		UIState::getStateNumber( "pressed" ),
		TF->getTexture( TF->loadFromFile( MyPath + "sprites/button-te_mdown.png" ) ) );
	Sizef screenSize = SceneManager::instance()->getUISceneNode()->getSize();

	mShowMenu = UIPushButton::New();
	mShowMenu->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
	mShowMenu->setPadding( Rectf( 16, 0, 16, 0 ) );
	mShowMenu->setSkin( nSkin );
	mShowMenu->setText( "Show Menu" );
	mShowMenu->setPosition( screenSize.getWidth() - mShowMenu->getSize().getWidth() - 32,
							screenSize.getHeight() - mShowMenu->getSize().getHeight() - 9 );
	mShowMenu->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mShowMenu->addEventListener( Event::MouseClick, cb::Make1( this, &EETest::onShowMenu ) );
#endif
}

void EETest::createUI() {
	Clock TE;

	mThemeName = "uitheme";

	if ( PixelDensity::getPixelDensity() > 1.5 ) {
		mThemeName = "uitheme2x";
	} else if ( PixelDensity::getPixelDensity() >= 1.1 ) {
		mThemeName = "uitheme1.5x";
	}

	createUIThemeTextureAtlas();

	eePRINTL( "Texture Atlas Loading Time: %4.3f ms.", TE.getElapsed().asMilliseconds() );

	UISceneNode* sceneNode = UISceneNode::New();

	sceneNode->enableDrawInvalidation();
	sceneNode->enableFrameBuffer();
	sceneNode->setVerbose( true );

	if ( mDebugUI ) {
		sceneNode->setDrawBoxes( true );
		sceneNode->setDrawDebugData( true );
		sceneNode->setHighlightFocus( true );
		sceneNode->setHighlightOver( true );
		sceneNode->setHighlightInvalidation( true );
	}

	sceneNode->setTranslator( mTranslator );

	SceneManager::instance()->add( sceneNode );

	eePRINTL( "Node size: %d", sizeof( Node ) );
	eePRINTL( "UINode size: %d", sizeof( UINode ) );
	eePRINTL( "UIWidget size: %d", sizeof( UIWidget ) );

	mTheme = UITheme::load( mThemeName, mThemeName,
							MyPath + "ui/" + mThemeName + EE_TEXTURE_ATLAS_EXTENSION, TTF,
							MyPath + "ui/uitheme.css" );

	/*mTheme = UITheme::load( mThemeName, mThemeName, "", TTF, MyPath + "ui/breeze.css" );*/

	sceneNode->combineStyleSheet( mTheme->getStyleSheet() );

	UIThemeManager* uiThemeManager = sceneNode->getUIThemeManager();
	uiThemeManager->add( mTheme );
	uiThemeManager->setDefaultEffectsEnabled( true )->setDefaultFont( TTF )->setDefaultTheme(
		mThemeName );

	createBaseUI();
	createNewUI();

	eePRINTL( "CreateUI time: %4.3f ms.", TE.getElapsed().asMilliseconds() );
}

void EETest::createNewUI() {
	std::vector<String> str = getTestStringArr();

	relLay = UIRelativeLayout::New();
	relLay->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );

	UIWidget* container = UIWidget::New();
	container->setSize( relLay->getParent()->getSize() - 32.f );

	UIScrollView* scrollView = UIScrollView::New();
	scrollView->setTouchDragEnabled( true );
	scrollView->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent )
		->setParent( relLay );
	scrollView->getContainer()->addEventListener( Event::MouseClick,
												  cb::Make1( this, &EETest::onMainClick ) );
	container->setParent( scrollView );
	container->addEventListener( Event::MouseClick, cb::Make1( this, &EETest::onMainClick ) );

	UILoader* loader = UILoader::New();
	loader->setOutlineThickness( 4 )
		->setRadius( 25 )
		->setPosition( 800, 0 )
		->setSize( 100, 100 )
		->setParent( container );
	loader->setBackgroundColor( 0xCCCCCCCC );

	UIRadioButton* ctrl = UIRadioButton::New();
	ctrl->setId( "happy_radio" );
	ctrl->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
		->setPosition( 50, 100 )
		->setSize( 200, 32 )
		->setParent( container );
	ctrl->setBackgroundColor( 0x33333333 );
	ctrl->setBorderColor( 0x66666666 );
	ctrl->setText( "Happy RadioButon :)" );
	ctrl->setFontColor( Color::Black );

	UICheckBox* cbox = UICheckBox::New();
	cbox->setId( "happy_check" );
	cbox->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
		->setPosition( 50, 164 )
		->setSize( 200, 32 )
		->setParent( container );
	cbox->setBackgroundColor( 0x33333333 );
	cbox->setBorderColor( 0x66666666 );
	cbox->setText( "Happy CheckBox :)" );
	cbox->setFontColor( Color::Black );

	SceneManager::instance()->getUISceneNode()->combineStyleSheet( R"css(
		#happy_check,
		#happy_radio {
			color: black;
		}
	)css" );

	UIImage* gfx = UIImage::New();
	gfx->setPosition( 50, 140 )->setSize( 16, 16 )->setParent( container );
	gfx->setBackgroundColor( 0x33333333 );
	gfx->setDrawable( mTheme->getIconByName( "ok" ) );

	UISlider* slider = UISlider::New();
	slider->setOrientation( UIOrientation::Horizontal )
		->setPosition( 50, 0 )
		->setSize( 100, 100 )
		->setParent( container );
	slider->setAllowHalfSliderOut( true );

	UISlider* slider2 = UISlider::New();
	slider2->setOrientation( UIOrientation::Vertical )
		->setPosition( 100, 0 )
		->setSize( 100, 100 )
		->setParent( container );
	slider2->setAllowHalfSliderOut( true );

	UITextInput* textInput = UITextInput::New();
	textInput->setPosition( 50, 210 )->setSize( 200, 0 )->setParent( container );

	UITextInputPassword* textInputPass = UITextInputPassword::New();
	textInputPass->setPosition( 50, 245 )->setSize( 200, 0 )->setParent( container );

	UIListBox* listBox = UIListBox::New();
	listBox->setPosition( 50, 360 )->setSize( 200, 160 )->setParent( container );
	listBox->addListBoxItems( str );

	UIProgressBar* progressBar = UIProgressBar::New();
	progressBar->setPosition( 50, 530 )->setSize( 200, 0 )->setParent( container );
	progressBar->setProgress( 60.f );
	progressBar->setDisplayPercent( true );

	UIPushButton* pushButton = UIPushButton::New();
	pushButton->setPosition( 50, 560 )->setSize( 200, 0 )->setParent( container );
	pushButton->setText( "PushButton" );
	pushButton->setIcon( mTheme->getIconByName( "ok" ) );
	pushButton->addEventListener( Event::MouseClick, [&, pushButton]( const Event* event ) {
		if ( static_cast<const MouseEvent*>( event )->getFlags() & EE_BUTTON_LMASK )
			createColorPicker( pushButton );
	} );

	UISprite* sprite = UISprite::New();
	sprite->setFlags( UI_AUTO_SIZE );
	sprite->setPosition( 50, 600 )->setParent( container );
	sprite->setSprite( &SP );

	UIScrollBar* scrollBar = UIScrollBar::New();
	scrollBar->setOrientation( UIOrientation::Horizontal )
		->setPosition( 200, 0 )
		->setSize( 100, 0 )
		->setParent( container );

	UIScrollBar* scrollBar2 = UIScrollBar::New();
	scrollBar2->setOrientation( UIOrientation::Vertical )
		->setPosition( 300, 0 )
		->setSize( 0, 100 )
		->setParent( container );

	UIDropDownList* dropdownList = UIDropDownList::New();
	dropdownList->setPosition( 50, 320 )->setSize( 200, 100 )->setParent( container );
	dropdownList->getListBox()->addListBoxItem( "Test 1" );
	dropdownList->getListBox()->addListBoxItem( "Test 2" );
	dropdownList->getListBox()->addListBoxItem( "Test 3" );

	UIComboBox* comboBox = UIComboBox::New();
	comboBox->setPosition( 50, 280 )->setSize( 200, 0 )->setParent( container );
	comboBox->getListBox()->addListBoxItem( "Test 1234" );
	comboBox->getListBox()->addListBoxItem( "Test 2345" );
	comboBox->getListBox()->addListBoxItem( "Test 3567" );
	comboBox->getListBox()->setSelected( 0 );

	UITextEdit* textEdit = UITextEdit::New();
	textEdit->setFlags( UI_WORD_WRAP );
	textEdit->setPosition( 350, 4 )->setSize( 200, 200 )->setParent( container );
	textEdit->setText( mBuda );

	UISpinBox* spinBox = UISpinBox::New();
	spinBox->setPosition( 350, 210 )->setSize( 200, 0 )->setParent( container );

	UITable* genGrid = UITable::New();
	genGrid->setSmoothScroll( true );
	genGrid->setPosition( 350, 250 )->setSize( 200, 130 )->setParent( container );
	genGrid->setCollumnsCount( 3 )->setRowHeight( 24 );
	genGrid->setCollumnWidth( 0, 50 );
	genGrid->setCollumnWidth( 1, 24 );
	genGrid->setCollumnWidth( 2, 100 );

	for ( Uint32 i = 0; i < 15; i++ ) {
		UITableCell* Cell = UITableCell::New();
		UITextView* TxtBox = UITextView::New();
		UITextInput* TxtInput = UITextInput::New();
		UIImage* TxtGfx = UIImage::New();
		TxtGfx->unsetFlags( UI_AUTO_SIZE );

		Cell->setParent( genGrid->getContainer() );

		Cell->setCell( 0, TxtBox );
		Cell->setCell( 1, TxtGfx );
		Cell->setCell( 2, TxtInput );

		TxtGfx->setDrawable( mTheme->getIconByName( "ok" ) );
		TxtBox->setText( "Test " + String::toStr( i + 1 ) );

		genGrid->add( Cell );
	}

	UITabWidget* TabWidget = UITabWidget::New();
	TabWidget->setPosition( 350, 530 )->setSize( 200, 64 )->setParent( container );

	TabWidget->add( "Tab 1", UIWidget::New()->setThemeSkin( "winback" ),
					mTheme->getIconByName( "ok" ) );
	TabWidget->add( "Tab 2", UIWidget::New()->setThemeSkin( "winback" ),
					mTheme->getIconByName( "go-up" ) );
	TabWidget->add( "Tab 3", UIWidget::New()->setThemeSkin( "winback" ),
					mTheme->getIconByName( "add" ) );

	UIWindow* MenuCont = UIWindow::New();
	MenuCont->setPosition( 350, 390 )->setSize( 200, 115 );

	UIMenuBar* MenuBar = UIMenuBar::New();
	MenuBar->setParent( MenuCont->getContainer() );

	UIPopUpMenu* PopMenu = UIPopUpMenu::New();
	PopMenu->add( "File" );
	PopMenu->add( "Open" );
	PopMenu->add( "Close" );
	PopMenu->add( "Quit" );

	UIPopUpMenu* PopMenu2 = UIPopUpMenu::New();
	PopMenu2->add( "Bla" );
	PopMenu2->add( "Bla 2" );
	PopMenu2->add( "Bla 3" );
	PopMenu2->add( "Bla 4" );

	MenuBar->addMenuButton( "File", PopMenu );
	MenuBar->addMenuButton( "Edit", PopMenu2 );

	UIWindow* win = UIWindow::New();
	win->setSize( 500, 500 );
	win->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_RESIZEABLE | UI_WIN_MAXIMIZE_BUTTON );

	UILinearLayout* layWin = UILinearLayout::NewVertical();
	layWin->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	layWin->setParent( win );

	UILinearLayout* layPar = UILinearLayout::NewHorizontal();
	layPar->setParent( layWin );
	layPar->setLayoutMargin( Rect( 10, 10, 10, 10 ) );
	layPar->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	layPar->setLayoutGravity( UI_VALIGN_CENTER | UI_HALIGN_CENTER );
	layPar->setBackgroundColor( 0x999999FF );

	UILinearLayout* lay = UILinearLayout::NewVertical();
	lay->setLayoutGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
	lay->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	lay->setBackgroundColor( 0x333333FF );
	lay->setLayoutWeight( 0.7f );

	UITextView::New()
		->setText( "Text on test 1" )
		->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setParent( lay );
	UITextView::New()
		->setText( "Text on test 2" )
		->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( lay );
	UICheckBox::New()
		->setText( "Checkbox" )
		->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( lay );
	UITextView::New()
		->setText( "Text on test 3" )
		->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( lay );
	UITextView::New()
		->setText( "Text on test 4" )
		->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( lay );
	UITextInput::New()
		->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( lay );

	UILinearLayout* lay2 = UILinearLayout::NewVertical();
	lay2->setLayoutGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
	lay2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
	lay2->setBackgroundColor( Color::Black );
	lay2->setLayoutWeight( 0.3f );

	UIPushButton::New()
		->setText( "PushButton" )
		->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_VALIGN_CENTER )
		->setParent( lay2 );
	UIListBox* lbox = UIListBox::New();
	lbox->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed )
		->setSize( 0, 105 )
		->setParent( lay2 );
	lbox->addListBoxItems( {"This", "is", "a", "ListBox"} );
	lay2->setParent( layPar );
	lay->setParent( layPar );

	UIDropDownList* drop = UIDropDownList::New();
	drop->setLayoutMargin( Rect( 10, 10, 10, 10 ) )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( layWin );
	drop->getListBox()->addListBoxItems( {"Car", "Bus", "Plane", "Submarine"} );
	drop->getListBox()->setSelected( 0 );
	win->show();

	UIWindow* win2 = UIWindow::New();
	win2->setSize( 500, 500 );
	win2->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_RESIZEABLE | UI_WIN_MAXIMIZE_BUTTON );

	UIRelativeLayout* rlay = UIRelativeLayout::New();
	rlay->setParent( win2 );
	rlay->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	rlay->setLayoutMargin( Rect( 16, 16, 16, 16 ) );
	rlay->setBackgroundColor( 0x333333CC );

	UIPushButton* ofBut = UIPushButton::New();
	ofBut->setText( "OK" )
		->setLayoutGravity( UI_VALIGN_BOTTOM | UI_HALIGN_RIGHT )
		->setLayoutMargin( Rect( 0, 0, 16, 16 ) )
		->setParent( rlay );

	UIPushButton::New()
		->setText( "Cancel" )
		->setLayoutGravity( UI_VALIGN_BOTTOM | UI_HALIGN_RIGHT )
		->setLayoutMargin( Rect( 0, 0, 8, 0 ) )
		->setLayoutPositionPolicy( PositionPolicy::LeftOf, ofBut )
		->setParent( rlay );

	win2->show();

	SceneManager::instance()->getUISceneNode()->loadLayoutFromString( R"xml(
		<window layout_width="300dp" layout_height="300dp" winflags="default|maximize">
			<LinearLayout id="testlayout" orientation="vertical" layout_width="match_parent" layout_height="match_parent" layout_margin="8dp">
				<TextView text="Hello World!" gravity="center" layout_gravity="center_horizontal" layout_width="match_parent" layout_height="wrap_content" backgroundColor="black" />
				<PushButton text="OK!" textSize="16dp" icon="ok" gravity="center" layout_gravity="center_horizontal" layout_width="match_parent" layout_height="wrap_content" />
				<Image src="thecircle" layout_width="match_parent" layout_height="32dp" flags="clip" />
				<Sprite src="gn" />
				<TextInput text="test" layout_width="match_parent" layout_height="wrap_content" />
				<DropDownList layout_width="match_parent" layout_height="wrap_content" selectedIndex="0">
					<item>Test Item</item>
					<item>@string/test_item</item>
				</DropDownList>
				<ListBox layout_width="match_parent" layout_height="match_parent" layout_weight="1">
					<item>Hello!</item>
					<item>World!</item>
				</ListBox>
			</LinearLayout>
		</window>
	)xml" );

	SceneManager::instance()->getUISceneNode()->loadLayoutFromString( R"xml(
		<window layout_width="800dp" layout_height="600dp" winflags="default|maximize">
			<LinearLayout layout_width="match_parent" layout_height="match_parent">
				<ScrollView layout_width="match_parent" layout_height="match_parent" touchdrag="true">
					<GridLayout columnMode="size" rowMode="size" columnWidth="200dp" rowHeight="200dp" layout_width="match_parent" layout_height="wrap_content" id="gridlayout" clip="false" />
				</ScrollView>
			</LinearLayout>
		</window>
	)xml" );

	UIGridLayout* gridLayout = NULL;
	SceneManager::instance()->getUISceneNode()->bind( "gridlayout", gridLayout );

	if ( NULL != gridLayout ) {
		std::vector<Texture*> textures = TextureFactory::instance()->getTextures();

		if ( textures.size() > 0 ) {
			for ( std::size_t i = 0; i < textures.size(); i++ ) {
				UIImage* img = UIImage::New();
				img->setDrawable( textures[i] )
					->setScaleType( UIScaleType::FitInside )
					->setGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER )
					->setEnabled( false )
					->setParent( gridLayout );

				img->setBackgroundColor( Color::fromPointer( textures[i] ) );
			}
		}
	}

	if ( NULL != mShowMenu )
		mShowMenu->toFront();
}

void EETest::createMapEditor() {
	if ( NULL != mMapEditor )
		return;

	UIWindow* tWin = UIWindow::New();
	tWin->setSizeWithDecoration( 1024, 768 )->setPosition( 0, 0 );
	UIWindow::StyleConfig windowStyleConfig = tWin->getStyleConfig();
	windowStyleConfig.WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON |
								 UI_WIN_DRAGABLE_CONTAINER | UI_WIN_SHADOW | UI_WIN_FRAME_BUFFER;
	windowStyleConfig.MinWindowSize = tWin->getSizeWithoutDecoration();
	tWin->setStyleConfig( windowStyleConfig );

	Clock mapEditorTime;
	mMapEditor = MapEditor::New( tWin, cb::Make0( this, &EETest::onMapEditorClose ) );
	eePRINTL( "Map Editor created in: %s.", mapEditorTime.getElapsedTime().toString().c_str() );
	tWin->center();
	tWin->show();
}

void EETest::onMapEditorClose() {
	mMapEditor = NULL;
}

void EETest::createETGEditor() {
	UIWindow* tWin = UIWindow::New();
	tWin->setSizeWithDecoration( 1024, 768 )->setPosition( 0, 0 );
	UIWindow::StyleConfig windowStyleConfig = tWin->getStyleConfig();
	windowStyleConfig.WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON |
								 UI_WIN_DRAGABLE_CONTAINER | UI_WIN_SHADOW | UI_WIN_FRAME_BUFFER;
	windowStyleConfig.MinWindowSize = tWin->getSizeWithoutDecoration();
	tWin->setStyleConfig( windowStyleConfig );

	mETGEditor = Tools::TextureAtlasEditor::New( tWin, [&] { mETGEditor = NULL; } );
	tWin->center();
	tWin->show();
}

void EETest::createColorPicker( Node* node ) {
	mColorPicker = Tools::UIColorPicker::NewModal( node, [&]( Color color ) {
		UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK, color.toHexString() );
		msgBox->center();
		msgBox->show();
	} );
	// mColorPicker->getUIWindow()->center();
}

void EETest::createCommonDialog() {
	UICommonDialog* CDialog = UICommonDialog::New();
	CDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON );
	CDialog->addFilePattern( "*.hpp;*.cpp", true );
	CDialog->center();
	CDialog->show();
}

static void onWinDragStart( const Event* event ) {
	UINode* ctrl = static_cast<UINode*>( event->getNode() );
	UIWindow* window = ctrl->isType( UI_TYPE_WINDOW )
						   ? static_cast<UIWindow*>( ctrl )
						   : static_cast<UIWindow*>( ctrl->getWindowContainer()->getParent() );
	window->runAction( Actions::Fade::New( window->getAlpha(), 100, Seconds( 0.2f ) ) );
}

static void onWinDragStop( const Event* event ) {
	UINode* ctrl = static_cast<UINode*>( event->getNode() );
	UIWindow* window = ctrl->isType( UI_TYPE_WINDOW )
						   ? static_cast<UIWindow*>( ctrl )
						   : static_cast<UIWindow*>( ctrl->getWindowContainer()->getParent() );
	window->runAction( Actions::Fade::New( window->getAlpha(), 255, Seconds( 0.2f ) ) );
}

void EETest::createDecoratedWindow() {
	mUIWindow = UIBlurredWindow::New( mBlur );
	mUIWindow
		->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_SHADOW |
					   UI_WIN_FRAME_BUFFER )
		->setMinWindowSize( 530, 350 )
		->setPosition( 200, 50 );

	mUIWindow->addEventListener( Event::OnWindowClose, cb::Make1( this, &EETest::onCloseClick ) );
	mUIWindow->setTitle( "Test Window" );
	mUIWindow->addEventListener( Event::OnDragStart, cb::Make1( &onWinDragStart ) );
	mUIWindow->addEventListener( Event::OnDragStop, cb::Make1( &onWinDragStop ) );

	UILinearLayout* lay = UILinearLayout::NewVertical();
	lay->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	lay->setParent( mUIWindow->getContainer() );

	UIMenuBar* MenuBar = UIMenuBar::New();
	MenuBar->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( lay );

	UIPopUpMenu* PopMenu = UIPopUpMenu::New();
	PopMenu->add( "Hide Border" );
	PopMenu->add( "Close" );
	PopMenu->addEventListener( Event::OnItemClicked, []( const Event* Event ) {
		if ( !Event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;

		Node* node = Event->getNode();
		UIWindow* win = NULL;

		while ( NULL != node && NULL == win ) {
			if ( node->isWindow() ) {
				win = static_cast<UIWindow*>( node );
			} else {
				node = node->getParent();
			}
		}

		if ( NULL == win )
			return;

		UIMenuItem* menuItem = Event->getNode()->asType<UIMenuItem>();
		const String& txt = menuItem->getText();

		if ( "Hide Border" == txt ) {
			win->setWinFlags( win->getWinFlags() | UI_WIN_NO_DECORATION );
			menuItem->setText( "Show Border" );
		} else if ( "Show Border" == txt ) {
			win->setWinFlags( win->getWinFlags() & ~UI_WIN_NO_DECORATION );
			menuItem->setText( "Hide Border" );
		} else if ( "Close" == txt ) {
			win->closeWindow();
		}
	} );

	UIPopUpMenu* PopMenu2 = UIPopUpMenu::New();
	PopMenu2->add( "Bla" );
	PopMenu2->add( "Bla 2" );
	PopMenu2->add( "Bla 3" );
	PopMenu2->add( "Bla 4" );

	MenuBar->addMenuButton( "File", PopMenu );
	MenuBar->addMenuButton( "Edit", PopMenu2 );

	UIPushButton* Button = UIPushButton::New();
	Button->setLayoutMargin( Rect( 5, 5, 5, 5 ) );
	Button->setText( "Click Me" );
	Button->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setParent( lay );
	Button->addEventListener( Event::MouseClick, cb::Make1( this, &EETest::onButtonClick ) );

	mUIWindow->addShortcut( KEY_C, KEYMOD_ALT, Button );

	UITabWidget* TabWidget = UITabWidget::New();
	TabWidget->setLayoutMargin( Rect( 5, 5, 5, 5 ) )
		->setLayoutWeight( 1 )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
		->setFlags( UI_HALIGN_CENTER | UI_VALIGN_CENTER )
		->setParent( lay );

	UITextEdit* TEdit = UITextEdit::New();
	TEdit->setFlags( UI_WORD_WRAP );
	TEdit->setParent( TabWidget );
	TEdit->setText( mBuda );
	TabWidget->add( "TextEdit", TEdit );

	UITextInput* Txt = UITextInput::New();
	Txt->setFlags( UI_WORD_WRAP );
	Txt->setParent( TabWidget );
	Txt->setText( mBuda );
	TabWidget->add( "TextInput", Txt );

	UITextView* txtBox = UITextView::New();
	txtBox->resetFlags( UI_HALIGN_LEFT | UI_VALIGN_TOP | UI_AUTO_PADDING | UI_WORD_WRAP |
						UI_TEXT_SELECTION_ENABLED );
	txtBox->setParent( TabWidget );
	txtBox->setText( mBuda );
	TabWidget->add( "TextBox", txtBox );

	UICodeEditor* codeEditor = UICodeEditor::New();
	codeEditor->setParent( TabWidget );
	codeEditor->getDocument().textInput( mBuda );
	TabWidget->add( "CodeEditor", codeEditor );
}

void EETest::onCloseClick( const Event* ) {
	mUIWindow = NULL;
}

void EETest::onItemClick( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = event->getNode()->asType<UIMenuItem>()->getText();

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
		UIMenuCheckBox* Chk = event->getNode()->asType<UIMenuCheckBox>();

		C->toFront();
		C->setVisible( true );
		C->setEnabled( true );

		if ( Chk->isActive() ) {
			if ( C->getScale() == 1.f )
				C->setScale( 0.f );

			C->runAction( Actions::Spawn::New(
				Actions::Scale::New( C->getScale(), Vector2f::One, Milliseconds( 500.f ),
									 Ease::SineOut ),
				Actions::Fade::New( C->getAlpha(), 255.f, Milliseconds( 500.f ) ),
				Actions::Rotate::New( 0, 360, Milliseconds( 500.f ), Ease::SineOut ) ) );
		} else {
			C->runAction( Actions::Spawn::New(
				Actions::Scale::New( C->getScale(), Vector2f::Zero, Milliseconds( 500.f ),
									 Ease::SineIn ),
				Actions::Sequence::New( Actions::FadeOut::New( Milliseconds( 500 ) ),
										Actions::Disable::New() ),
				Actions::Rotate::New( 0, 360, Milliseconds( 500.f ), Ease::SineIn ) ) );
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
	} else if ( "Color Picker" == txt ) {
		createColorPicker( event->getNode() );
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

void EETest::onValueChange( const Event* ) {
	mTextBoxValue->setText( "Scroll Value:\n" + String::toStr( mScrollBar->getValue() ) );

	mProgressBar->setProgress( mScrollBar->getValue() * 100.f );
}

void EETest::onSliderValueChange( const Event* Event ) {
	UISlider* slider = static_cast<UISlider*>( Event->getNode() );

	C->setRotation( slider->getValue() * 90.f );
}

void EETest::onQuitClick( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mWindow->close();
	}
}

void EETest::showMenu() {
	if ( NULL != Menu && Menu->show() ) {
		Vector2f Pos = KM->getMousePosf();
		UIMenu::fixMenuPos( Pos, Menu );
		Menu->setPixelsPosition( Pos );
	}
}

void EETest::onMainClick( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTON_RMASK ) {
		showMenu();
	}
}

using namespace EE::Scene::Actions;

void EETest::onButtonClick( const Event* event ) {
	const MouseEvent* mouseEvent = reinterpret_cast<const MouseEvent*>( event );

	if ( mouseEvent->getFlags() & EE_BUTTONS_LRM ) {
		UIImage* Gfx = UIImage::New();
		Gfx->setDrawable( mTheme->getIconByName( "ok" ) );
		Gfx->setEnabled( false );

		Gfx->runAction( Spawn::New(
			Move::New( Vector2f( Math::randi( 0, mWindow->getWidth() ), -64 ),
					   Vector2f( Math::randi( 0, mWindow->getWidth() ), mWindow->getHeight() + 64 ),
					   Milliseconds( 2500 ) ),
			Rotate::New( 0.f, 2500.f, Milliseconds( 2500 ) ),
			Sequence::New(
				Scale::New( Vector2f( 1.f, 1.f ), Vector2f( 2.f, 2.f ), Seconds( 0.5f ) ),
				Scale::New( Vector2f( 2.f, 2.f ), Vector2f( 1.f, 1.f ), Seconds( 0.5f ) ) ),
			Sequence::New( FadeOut::New( Milliseconds( 3500 ) ), Close::New() ) ) );
	}
}

void EETest::setScreen( Uint32 num ) {
	if ( NULL != mTerrainBut )
		mTerrainBut->setVisible( 1 == num );

	if ( 0 == num || 5 == num )
		mWindow->setClearColor( RGB( 240, 240, 240 ) );
	else
		mWindow->setClearColor( RGB( 0, 0, 0 ) );

	if ( num < 6 )
		Screen = num;

	if ( NULL != relLay )
		relLay->setVisible( Screen == 5 );
}

void EETest::cmdSetPartsNum( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt >= 0 && tInt <= 100000 ) ) {
			PS[2].create( ParticleEffect::WormHole, tInt, TN[5],
						  Vector2f( mWindow->getWidth() * 0.5f, mWindow->getHeight() * 0.5f ), 32,
						  true );
			Con.pushText( "Wormhole Particles Number Changed to: " + String::toStr( tInt ) );
		} else
			Con.pushText( "Valid parameters are between 0 and 100000 (0 = no limit)." );
	}
}

void EETest::onTextureLoaded( ResourceLoader* ) {
	SndMng.play( "mysound" );
}

void EETest::loadTextures() {
	if ( !FileSystem::fileExists( MyPath + "atlases/bnb.eta" ) ) {
		TexturePacker tp( 1024, 512, PixelDensity::toFloat( PixelDensitySize::MDPI ), true, false,
						  1 );
		tp.addTexturesPath( MyPath + "atlases/bnb/" );
		tp.save( MyPath + "atlases/bnb.png" );
	}

	if ( !FileSystem::fileExists( MyPath + "atlases/tiles.eta" ) ) {
		TexturePacker tp( 256, 32, PixelDensity::toFloat( PixelDensitySize::MDPI ), true, false,
						  0 );
		tp.addTexturesPath( MyPath + "atlases/tiles/" );
		tp.save( MyPath + "atlases/tiles.png" );
	}

	Clock TE;

	Uint32 i;

#ifndef EE_GLES

	PakTest = Zip::New();
	PakTest->open( MyPath + "test.zip" );

	std::vector<std::string> files = PakTest->getFileList();

	for ( i = 0; i < files.size(); i++ ) {
		std::string name( files[i] );

		if ( "jpg" == FileSystem::fileExtension( name ) ) {
			mResLoad.add( [=] { TextureFactory::instance()->loadFromPack( PakTest, name ); } );
		}
	}
#endif

	SndMng.loadFromFile( "mysound", MyPath + "sounds/sound.ogg" );

	mResLoad.setThreaded( EE->isSharedGLContextEnabled() );
	mResLoad.load( cb::Make1( this, &EETest::onTextureLoaded ) );

	TN.resize( 12 );
	TNP.resize( 12 );

	for ( i = 0; i <= 6; i++ ) {
		TN[i] = TF->loadFromFile( MyPath + "sprites/" + String::toStr( i + 1 ) + ".png",
								  ( i + 1 ) == 7 ? true : false,
								  ( ( i + 1 ) == 4 ) ? Texture::ClampMode::ClampRepeat
													 : Texture::ClampMode::ClampToEdge );
		TNP[i] = TF->getTexture( TN[i] );
	}

	Tiles.resize( 10 );

	TextureAtlasLoader tgl( MyPath + "atlases/tiles.eta" );
	TextureAtlas* SG = TextureAtlasManager::instance()->getByName( "tiles" );

	if ( NULL != SG ) {
		for ( i = 0; i < 6; i++ ) {
			Tiles[i] = SG->getByName( String::toStr( i + 1 ) );
		}

		Tiles[6] = SG->add( TF->loadFromFile( MyPath + "sprites/objects/1.png" ), "7" );

#ifdef EE_GLES
		Image tImg( MyPath + "sprites/objects/2.png", 4 );
		tImg.createMaskFromColor( ColorA( 0, 0, 0, 255 ), 0 );
		Tiles[7] = SG->add( TF->loadFromPixels( tImg.getPixelsPtr(), tImg.getWidth(),
												tImg.getHeight(), tImg.getChannels() ),
							"8" );
#else
		Tiles[7] = SG->add( TF->loadFromFile( MyPath + "sprites/objects/2.png" ), "8" );
		Tiles[7]->getTexture()->createMaskFromColor( Color( 0, 0, 0, 255 ), 0 );
#endif
	}

	for ( Int32 my = 0; my < 4; my++ )
		for ( Int32 mx = 0; mx < 8; mx++ )
			SP.addFrame( TN[4], Sizef( 0, 0 ), Vector2i( 0, 0 ),
						 Rect( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );

	PS[0].setCallbackReset( cb::Make2( this, &EETest::particlesCallback ) );
	PS[0].create( ParticleEffect::Callback, 500, TN[5], Vector2f( 0, 0 ), 16, true );
	PS[1].create( ParticleEffect::Heal, 250, TN[5],
				  Vector2f( mWindow->getWidth() * 0.5f, mWindow->getHeight() * 0.5f ), 16, true );
	PS[2].create( ParticleEffect::WormHole, PartsNum, TN[5],
				  Vector2f( mWindow->getWidth() * 0.5f, mWindow->getHeight() * 0.5f ), 32, true );
	PS[3].create( ParticleEffect::Fire, 350, TN[5], Vector2f( -50.f, -50.f ), 32, true );
	PS[4].create( ParticleEffect::Fire, 350, TN[5], Vector2f( -50.f, -50.f ), 32, true );

	Con.addCommand( "setparticlesnum", cb::Make1( this, &EETest::cmdSetPartsNum ) );

	Texture* Tex = TNP[2];

	if ( NULL != Tex && Tex->lock() ) {
		int w = (int)Tex->getWidth();
		int h = (int)Tex->getHeight();

		for ( y = 0; y < h; y++ ) {
			for ( x = 0; x < w; x++ ) {
				Color C = Tex->getPixel( x, y );

				if ( C.r > 200 && C.g > 200 && C.b > 200 )
					Tex->setPixel( x, y,
								   Color( Math::randi( 0, 255 ), Math::randi( 0, 255 ),
										  Math::randi( 0, 255 ), C.a ) );
				else
					Tex->setPixel( x, y,
								   Color( Math::randi( 200, 255 ), Math::randi( 200, 255 ),
										  Math::randi( 200, 255 ), C.a ) );
			}
		}

		Tex->unlock( false, true );
	}

	Cursor[0] = TF->loadFromFile( MyPath + "cursors/cursor.tga" );
	CursorP[0] = TF->getTexture( Cursor[0] );

	CursorManager* CurMan = mWindow->getCursorManager();
	CurMan->setVisible( false );
	CurMan->setVisible( true );
	CurMan->set( Cursor::SysHand );
	CurMan->setGlobalCursor(
		Cursor::Arrow,
		CurMan->add( CurMan->create( CursorP[0], Vector2i( 1, 1 ), "cursor_special" ) ) );
	CurMan->set( Cursor::Arrow );

	CL1.addFrame( TN[2] );
	CL1.setPosition( Vector2f( 500, 400 ) );
	CL1.setScale( 0.5f );

	CL2.addFrame( TN[0], Sizef( 96, 96 ) );
	CL2.setColor( Color( 255, 255, 255, 255 ) );

	mTGL = TextureAtlasLoader::New( MyPath + "atlases/bnb" + EE_TEXTURE_ATLAS_EXTENSION );

	mBlindy.addFramesByPattern( "rn" );
	mBlindy.setPosition( Vector2f( 320.f, 0.f ) );

	mBoxSprite =
		Sprite::New( GlobalTextureAtlas::instance()->add( TextureRegion::New( TN[3], "ilmare" ) ) );
	mCircleSprite = Sprite::New(
		GlobalTextureAtlas::instance()->add( TextureRegion::New( TN[1], "thecircle" ) ) );

	eePRINTL( "Textures loading time: %4.3f ms.", TE.getElapsed().asMilliseconds() );

	Map.loadFromFile( MyPath + "maps/test.eem" );
	Map.setDrawGrid( false );
	Map.setClipedArea( false );
	Map.setDrawBackground( false );
	Map.setViewSize( mWindow->getSize().asFloat() );

	eePRINTL( "Map creation time: %4.3f ms.", TE.getElapsed().asMilliseconds() );
}

void EETest::run() {
	particlesThread();
}

void EETest::particlesThread() {
	while ( mWindow->isRunning() ) {
		updateParticles();
		Sys::sleep( Milliseconds( 10 ) );
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
		Texture* TexLoaded = TF->getByName( "1.jpg" );

		if ( NULL != TexLoaded )
			TexLoaded->draw( 0, 0 );
	}

	if ( KM->isMouseLeftPressed() )
		TNP[3]->drawEx( 0.f, 0.f, (Float)mWindow->getWidth(), (Float)mWindow->getHeight() );

	ang += et.asMilliseconds() * 0.1f;
	ang = ( ang >= 360 ) ? 0 : ang;

	ConvexShapeDrawable shape;
	Polygon2f polygon( Polygon2f::createRoundedRectangle( 0, 0, 50, 50, 8 ) );
	polygon.rotate( ang, polygon.getBounds().getCenter() );
	shape.setPolygon( polygon );
	shape.setPosition( Vector2f( 150, 150 ) );
	shape.setColor( Color::Fuchsia );
	shape.setAlpha( 100 );
	shape.draw();

	Batch.setTexture( TNP[2] );
	Batch.quadsBegin();
	Batch.quadsSetColor( Color( 150, 150, 150, 100 ) );
	Batch.quadsSetTexCoord( 0.0f, 0.0f, 0.5f, 0.5f );

	Batch.setBatchRotation( ang );
	Batch.setBatchScale( scale );
	Batch.setBatchCenter( Vector2f( HWidth, HHeight ) );

	Float aX = HWidth - 256.f;
	Float aY = HHeight - 256.f;
	Quad2f TmpQuad( Vector2f( aX, aY ), Vector2f( aX, aY + 32.f ), Vector2f( aX + 32.f, aY + 32.f ),
					Vector2f( aX + 32.f, aY ) );
	TmpQuad.rotate( ang, Vector2f( aX + 16.f, aY + 16.f ) );

	for ( Uint32 z = 0; z < 16; z++ ) {
		for ( Uint32 y = 0; y < 16; y++ ) {
			Float tmpx = (Float)z * 32.f;
			Float tmpy = (Float)y * 32.f;

			Batch.batchQuadFree( TmpQuad[0].x + tmpx, TmpQuad[0].y + tmpy, TmpQuad[1].x + tmpx,
								 TmpQuad[1].y + tmpy, TmpQuad[2].x + tmpx, TmpQuad[2].y + tmpy,
								 TmpQuad[3].x + tmpx, TmpQuad[3].y + tmpy );
		}
	}

	Batch.draw();

	Batch.setBatchRotation( 0.0f );
	Batch.setBatchScale( 1.0f );
	Batch.setBatchCenter( Vector2f( 0, 0 ) );

	Float PlanetX = HWidth - TNP[6]->getWidth() * 0.5f;
	Float PlanetY = HHeight - TNP[6]->getHeight() * 0.5f;

	if ( scale >= 1.5f ) {
		scale = 1.5f;
		side = true;
	} else if ( scale <= 0.5f ) {
		side = false;
		scale = 0.5f;
	}
	scale =
		( !side ) ? scale + et.asMilliseconds() * 0.00025f : scale - et.asMilliseconds() * 0.00025f;

	if ( mUseShaders ) {
		mBlurFactor = ( 1.5f * 0.01f ) - ( scale * 0.01f );
		mShaderProgram->bind();
		mShaderProgram->setUniform( "blurfactor", (float)mBlurFactor );
	}

	TNP[6]->drawFast( PlanetX, PlanetY, ang, Vector2f( scale, scale ) );

	if ( mUseShaders )
		mShaderProgram->unbind();

	TNP[3]->draw( HWidth - 128, HHeight, 0, Vector2f::One, Color( 255, 255, 255, 150 ), BlendAlpha,
				  RENDER_ISOMETRIC );
	TNP[3]->draw( HWidth - 128, HHeight - 128, 0, Vector2f::One, Color( 255, 255, 255, 50 ),
				  BlendAlpha, RENDER_ISOMETRIC );
	TNP[3]->draw( HWidth - 128, HHeight, 0, Vector2f::One, Color( 255, 255, 255, 50 ), BlendAlpha,
				  RENDER_ISOMETRIC_VERTICAL );
	TNP[3]->draw( HWidth, HHeight, 0, Vector2f::One, Color( 255, 255, 255, 50 ), BlendAlpha,
				  RENDER_ISOMETRIC_VERTICAL_NEGATIVE );

	alpha = ( !aside ) ? alpha + et.asMilliseconds() * 0.1f : alpha - et.asMilliseconds() * 0.1f;
	if ( alpha >= 255 ) {
		aside = true;
		alpha = 255;
	} else if ( alpha <= 0 ) {
		alpha = 0;
		aside = false;
	}

	Color Col( 255, 255, 255, (int)alpha );
	TNP[1]->drawEx( (Float)mWindow->getWidth() - 128.f, (Float)mWindow->getHeight() - 128.f, 128.f,
					128.f, ang, Vector2f::One, Col, Col, Col, Col, BlendAdd,
					RENDER_FLIPPED_MIRRORED );

	SP.setPosition( Vector2f( alpha, alpha ) );
	SP.draw();

#ifndef EE_GLES
	CL1.setRenderMode( RENDER_ISOMETRIC );

	if ( CL1.getAABB().intersectCircle( Mousef, 80.f ) )
		CL1.setColor( Color( 255, 0, 0, 200 ) );
	else
		CL1.setColor( Color( 255, 255, 255, 200 ) );

	if ( Polygon2f::intersectQuad2( CL1.getQuad(), CL2.getQuad() ) ) {
		CL1.setColor( Color( 0, 255, 0, 255 ) );
		CL2.setColor( Color( 0, 255, 0, 255 ) );
	} else
		CL2.setColor( Color( 255, 255, 255, 255 ) );

	CL1.setRotation( ang );
	CL1.setScale( scale * 0.5f );

	CL2.setPosition( Vector2f( (Float)Mousef.x - 64.f, (Float)Mousef.y + 128.f ) );
	CL2.setRotation( -ang );

	CL1.draw();
	CL2.draw();

	PR.setFillMode( DRAW_LINE );
	PR.drawRectangle( CL1.getAABB() );

	PR.drawQuad( CL1.getQuad() );
#endif

	Ang = Ang + mWindow->getElapsed().asMilliseconds() * 0.1f;
	if ( Ang > 360.f )
		Ang = 1.f;

	if ( ShowParticles )
		particles();

	PR.setColor( Color( 0, 255, 0, 50 ) );

	Line2f Line( Vector2f( 0.f, 0.f ),
				 Vector2f( (Float)mWindow->getWidth(), (Float)mWindow->getHeight() ) );
	Line2f Line2( Vector2f( Mousef.x - 80.f, Mousef.y - 80.f ),
				  Vector2f( Mousef.x + 80.f, Mousef.y + 80.f ) );
	Line2f Line3( Vector2f( (Float)mWindow->getWidth(), 0.f ),
				  Vector2f( 0.f, (Float)mWindow->getHeight() ) );
	Line2f Line4( Vector2f( Mousef.x - 80.f, Mousef.y + 80.f ),
				  Vector2f( Mousef.x + 80.f, Mousef.y - 80.f ) );

	if ( Line.intersect( Line2 ) )
		iL1 = true;
	else
		iL1 = false;

	if ( Line3.intersect( Line4 ) )
		iL2 = true;
	else
		iL2 = false;

	if ( iL1 && iL2 )
		PR.setColor( Color( 255, 0, 0, 255 ) );
	else if ( iL1 )
		PR.setColor( Color( 0, 0, 255, 255 ) );
	else if ( iL2 )
		PR.setColor( Color( 255, 255, 0, 255 ) );

	PR.setFillMode( DRAW_LINE );
	PR.drawCircle( Vector2f( Mousef.x, Mousef.y ), 80.f, ( Uint32 )( Ang / 3 ) );
	PR.drawTriangle( Triangle2f( Vector2f( Mousef.x, Mousef.y - 10.f ),
								 Vector2f( Mousef.x - 10.f, Mousef.y + 10.f ),
								 Vector2f( Mousef.x + 10.f, Mousef.y + 10.f ) ) );
	PR.drawLine( Line2f( Vector2f( Mousef.x - 80.f, Mousef.y - 80.f ),
						 Vector2f( Mousef.x + 80.f, Mousef.y + 80.f ) ) );
	PR.drawLine( Line2f( Vector2f( Mousef.x - 80.f, Mousef.y + 80.f ),
						 Vector2f( Mousef.x + 80.f, Mousef.y - 80.f ) ) );
	PR.drawLine( Line2f( Vector2f( (Float)mWindow->getWidth(), 0.f ),
						 Vector2f( 0.f, (Float)mWindow->getHeight() ) ) );
	PR.setFillMode( DRAW_FILL );
	PR.drawQuad( Quad2f( Vector2f( 0.f, 0.f ), Vector2f( 0.f, 100.f ), Vector2f( 150.f, 150.f ),
						 Vector2f( 200.f, 150.f ) ),
				 Color( 220, 240, 0, 125 ), Color( 100, 0, 240, 125 ), Color( 250, 50, 25, 125 ),
				 Color( 50, 150, 150, 125 ) );
	PR.setFillMode( DRAW_LINE );
	PR.drawRectangle( Rectf( Vector2f( Mousef.x - 80.f, Mousef.y - 80.f ), Sizef( 160.f, 160.f ) ),
					  45.f );
	PR.drawLine( Line2f( Vector2f( 0.f, 0.f ),
						 Vector2f( (Float)mWindow->getWidth(), (Float)mWindow->getHeight() ) ) );

	TNP[3]->drawQuadEx( Quad2f( Vector2f( 0.f, 0.f ), Vector2f( 0.f, 100.f ),
								Vector2f( 150.f, 150.f ), Vector2f( 200.f, 150.f ) ),
						Vector2f(), ang, Vector2f( scale, scale ), Color( 220, 240, 0, 125 ),
						Color( 100, 0, 240, 125 ), Color( 250, 50, 25, 125 ),
						Color( 50, 150, 150, 125 ) );

	WP.update( et );
	PR.setColor( Color( 0, 255, 0, 255 ) );
	PR.drawPoint( WP.getPosition(), 10.f );
}

void EETest::screen3() {
	if ( AnimVal >= 300.0f ) {
		AnimVal = 300.0f;
		AnimSide = true;
	} else if ( AnimVal <= 0.5f ) {
		AnimVal = 0.5f;
		AnimSide = false;
	}
	AnimVal =
		( !AnimSide ) ? AnimVal + et.asMilliseconds() * 0.1f : AnimVal - et.asMilliseconds() * 0.1f;

	Batch.setTexture( TNP[3] );
	Batch.lineLoopBegin();
	for ( Float j = 0; j < 360; j++ ) {
		Batch.batchLineLoop( HWidth + 350 * Math::sinAng( j ), HHeight + 350 * Math::cosAng( j ),
							 HWidth + AnimVal * Math::sinAng( j + 1 ),
							 HHeight + AnimVal * Math::cosAng( j + 1 ) );
	}
	Batch.draw();
}

void EETest::screen4() {
	if ( NULL != mFBO ) {
		mFBO->bind();
		mFBO->clear();
	}

	if ( NULL != mVBO ) {
		mBlindy.setPosition( Vector2f( 128 - 16, 128 - 16 ) );
		mBlindy.draw();

		mVBO->bind();
		mVBO->draw();
		mVBO->unbind();

		mFBOText.setAlign( TEXT_ALIGN_CENTER );
		mFBOText.draw( 128.f - ( Float )( Int32 )( mFBOText.getTextWidth() * 0.5f ),
					   25.f - ( Float )( Int32 )( mFBOText.getTextHeight() * 0.5f ) );
	}

	Vector2f center( mFBO->getWidth() * 0.5f, mFBO->getHeight() * 0.5f );
	CircleDrawable r;
	r.setPosition( center );
	r.setRadius( 18 );
	GLi->getClippingMask()->setMaskMode( ClippingMask::Exclusive );
	GLi->getClippingMask()->clearMasks();
	GLi->getClippingMask()->appendMask( r );
	GLi->getClippingMask()->stencilMaskEnable();
	Primitives p;
	p.setFillMode( DRAW_FILL );
	p.setColor( Color( 100, 200, 100, 150 ) );
	p.drawCircle( center, 32 );
	GLi->getClippingMask()->stencilMaskDisable();

	if ( NULL != mFBO ) {
		mFBO->unbind();

		if ( NULL != mFBO->getTexture() ) {
			mFBO->getTexture()->draw(
				(Float)mWindow->getWidth() * 0.5f - (Float)mFBO->getWidth() * 0.5f,
				(Float)mWindow->getHeight() * 0.5f - (Float)mFBO->getHeight() * 0.5f, Ang );
			GlobalBatchRenderer::instance()->draw();
		}
	}
}

void EETest::screen5() {
	Color col( 0x000000CC );

	if ( drawableGroup.getDrawableCount() == 0 ) {
		ArcDrawable* arc = ArcDrawable::New();
		arc->setPosition( Vector2f( 60, 60 ) );
		arc->setArcStartAngle( 90 );
		arc->setArcAngle( 180 );
		arc->setRadius( 60 );
		arc->setColor( col );

		RectangleDrawable* rect = RectangleDrawable::New();
		rect->setPosition( Vector2f( 0, 60 ) );
		rect->setSize( Sizef( 120, 60 ) );
		rect->setColor( col );

		ArcDrawable* arc2 = ArcDrawable::New();
		arc2->setPosition( Vector2f( 60, 120 ) );
		arc2->setArcStartAngle( -90 );
		arc2->setArcAngle( 180 );
		arc2->setRadius( 60 );
		arc2->setColor( col );

		ConvexShapeDrawable* poly = ConvexShapeDrawable::New();
		poly->setPosition( Vector2f( 60, 90 ) );
		poly->addPoint( Vector2f( -10, -10 ) );
		poly->addPoint( Vector2f( -10, 10 ) );
		poly->addPoint( Vector2f( 10, 10 ) );
		poly->addPoint( Vector2f( 10, -10 ) );
		poly->setColor( col );
		poly->setLineWidth( PixelDensity::getPixelDensity() );
		poly->setFillMode( DRAW_LINE );

		drawableGroup.addDrawable( arc );
		drawableGroup.addDrawable( rect );
		drawableGroup.addDrawable( arc2 );
		drawableGroup.addDrawable( poly );
		drawableGroup.setPosition( Vector2f( 800 * 2, 400 * 2 ) );
	}

	drawableGroup.draw();
}

void EETest::render() {
	HWidth = mWindow->getWidth() * 0.5f;
	HHeight = mWindow->getHeight() * 0.5f;

	if ( Sys::getTicks() - lasttick >= 50 ) {
		lasttick = Sys::getTicks();
#ifdef EE_DEBUG
		mInfo = String::format(
			"EE - FPS: %d Frame Time: %4.2f Sleep Time: %4.2f Render Time: %4.2f\nMouse X: %d "
			"Mouse Y: %d\nTexture Memory Usage: %s\nApp Memory Usage: %s\nApp Peak Memory Usage: "
			"%s",
			mWindow->getFPS(), et.asMilliseconds(),
			mWindow->getSleepTimePerSecond().asMilliseconds(),
			mWindow->getRenderTimePerSecond().asMilliseconds(), (Int32)Mouse.x, (Int32)Mouse.y,
			FileSystem::sizeToString( TF->getTextureMemorySize() ).c_str(),
			FileSystem::sizeToString( (Uint32)MemoryManager::getTotalMemoryUsage() ).c_str(),
			FileSystem::sizeToString( (Uint32)MemoryManager::getPeakMemoryUsage() ).c_str() );
#else
		mInfo = String::format(
			"EE - FPS: %d Elapsed Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s",
			mWindow->getFPS(), et.asMilliseconds(), (Int32)Mouse.x, (Int32)Mouse.y,
			FileSystem::sizeToString( TF->getTextureMemorySize() ).c_str() );
#endif

		mInfoText.setString( mInfo );

		if ( mWindow->getClearColor().r == 0 ) {
			mInfoText.setFillColor( Color::White );
			mInfoText.setOutlineColor( Color::Black );
		} else {
			mInfoText.setFillColor( Color::Black );
			mInfoText.setOutlineColor( Color::White );
		}
	}

	if ( !MultiViewportMode ) {
		Scenes[Screen]();
	} else {
		Views[0].reset( Rectf( 0, 0, mWindow->getWidth(), HHeight ) );
		Views[0].setViewport( Rectf( 0, 0, 1, 0.5f ) );

		mWindow->setView( Views[0] );
		Mousef = KM->getMousePosFromView( Views[0] );
		Mouse = Vector2i( Mousef.x, Mousef.y );
		screen1();

		Views[1].reset( Rectf( 0, 0, mWindow->getWidth(), HHeight ) );
		Views[1].setViewport( Rectf( 0, 0.5f, 1, 0.5f ) );

		mWindow->setView( Views[1] );
		Mousef = KM->getMousePosFromView( Views[1] );
		Mouse = Vector2i( Mousef.x, Mousef.y );
		screen2();

		mWindow->setView( mWindow->getDefaultView() );
		GLi->getClippingMask()->clipEnable( (Int32)HWidth - 320, (Int32)HHeight - 240, 640, 480 );
		screen3();
		GLi->getClippingMask()->clipDisable();
	}

	Color ColRR1( 150, 150, 150, 220 );
	Color ColRR4( 150, 150, 150, 220 );
	Color ColRR2( 100, 100, 100, 220 );
	Color ColRR3( 100, 100, 100, 220 );

	mEEText.setAlign( TEXT_ALIGN_CENTER );

	PR.setColor( Color( 150, 150, 150, 220 ) );
	PR.setFillMode( DRAW_FILL );
	PR.drawRectangle( Rectf( Vector2f( 0.f, (Float)mWindow->getHeight() - mEEText.getTextHeight() ),
							 Vector2f( mEEText.getTextWidth(), mEEText.getTextHeight() ) ),
					  ColRR1, ColRR2, ColRR3, ColRR4 );

	mEEText.draw( 0.f, (Float)mWindow->getHeight() - mEEText.getTextHeight() );

	mInfoText.draw( 6.f, 6.f );

	SceneManager::instance()->draw();
	Con.draw();
}

void EETest::input() {
	KM->update();
	JM->update();

	Mouse = KM->getMousePos();
	Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );

	if ( KM->isKeyUp( KEY_F1 ) )
		Graphics::ShaderProgramManager::instance()->reload();

	UISceneNode* uiSceneNode = SceneManager::instance()->getUISceneNode();

	if ( KM->isKeyUp( KEY_F6 ) ) {
		uiSceneNode->setHighlightFocus( !uiSceneNode->getHighlightFocus() );
		uiSceneNode->setHighlightOver( !uiSceneNode->getHighlightOver() );
	}

	if ( KM->isKeyUp( KEY_F7 ) )
		uiSceneNode->setDrawBoxes( !uiSceneNode->getDrawBoxes() );

	if ( KM->isKeyUp( KEY_F8 ) )
		uiSceneNode->setDrawDebugData( !uiSceneNode->getDrawDebugData() );

	if ( !mWindow->isVisible() ) {
		mWasMinimized = true;

		mWindow->setFrameRateLimit( 10 );

		if ( mMusEnabled && Mus->getStatus() == Sound::Playing )
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

		if ( mMusEnabled && Mus->getStatus() == Sound::Paused )
			Mus->play();
	}

	if ( KM->isKeyDown( KEY_ESCAPE ) )
		mWindow->close();

	if ( KM->isKeyUp( KEY_F1 ) )
		MultiViewportMode = !MultiViewportMode;

	if ( KM->isAltPressed() && KM->isKeyUp( KEY_C ) )
		mWindow->centerToDisplay();

	if ( KM->isAltPressed() && KM->isKeyUp( KEY_M ) && !Con.isActive() ) {
		if ( !mWindow->isMaximized() )
			mWindow->maximize();
	}

	if ( KM->isKeyUp( KEY_F4 ) )
		TF->reloadAllTextures();

	if ( KM->isAltPressed() && KM->isKeyUp( KEY_RETURN ) ) {
		mWindow->toggleFullscreen();
	}

	if ( KM->grabInput() ) {
		if ( KM->isAltPressed() && KM->isKeyDown( KEY_TAB ) ) {
			mWindow->minimize();

			if ( KM->grabInput() )
				KM->grabInput( false );
		}
	}

	if ( KM->isControlPressed() && KM->isKeyUp( KEY_G ) )
		KM->grabInput( !KM->grabInput() );

	if ( KM->isKeyUp( KEY_F3 ) || KM->isKeyUp( KEY_BACKSLASH ) ) {
		Con.toggle();
	}

	if ( KM->isKeyUp( KEY_1 ) && KM->isControlPressed() )
		setScreen( 0 );

	if ( KM->isKeyUp( KEY_2 ) && KM->isControlPressed() )
		setScreen( 1 );

	if ( KM->isKeyUp( KEY_3 ) && KM->isControlPressed() )
		setScreen( 2 );

	if ( KM->isKeyUp( KEY_4 ) && KM->isControlPressed() )
		setScreen( 3 );

	if ( KM->isKeyUp( KEY_5 ) && KM->isControlPressed() )
		setScreen( 4 );

	if ( KM->isKeyUp( KEY_6 ) && KM->isControlPressed() )
		setScreen( 5 );

	Joystick* Joy = JM->getJoystick( 0 );

	if ( mJoyEnabled && NULL != Joy ) {
		if ( Joy->isButtonDown( 0 ) )
			KM->injectButtonPress( EE_BUTTON_LEFT );
		if ( Joy->isButtonDown( 1 ) )
			KM->injectButtonPress( EE_BUTTON_RIGHT );
		if ( Joy->isButtonDown( 2 ) )
			KM->injectButtonPress( EE_BUTTON_MIDDLE );
		if ( Joy->isButtonUp( 0 ) )
			KM->injectButtonRelease( EE_BUTTON_LEFT );
		if ( Joy->isButtonUp( 1 ) )
			KM->injectButtonRelease( EE_BUTTON_RIGHT );
		if ( Joy->isButtonUp( 2 ) )
			KM->injectButtonRelease( EE_BUTTON_MIDDLE );
		if ( Joy->isButtonUp( 3 ) )
			KM->injectButtonRelease( EE_BUTTON_WHEELUP );
		if ( Joy->isButtonUp( 7 ) )
			KM->injectButtonRelease( EE_BUTTON_WHEELDOWN );
		if ( Joy->isButtonUp( 4 ) )
			setScreen( 0 );
		if ( Joy->isButtonUp( 5 ) )
			setScreen( 1 );
		if ( Joy->isButtonUp( 6 ) )
			setScreen( 2 );

		Float aX = Joy->getAxis( AXIS_X );
		Float aY = Joy->getAxis( AXIS_Y );

		if ( 0 != aX || 0 != aY ) {
			double rE = mWindow->getElapsed().asMilliseconds();
			mAxisX += aX * rE;
			mAxisY += aY * rE;
		}

		if ( ( mAxisX != 0 && ( mAxisX >= 1.f || mAxisX <= -1.f ) ) ||
			 ( mAxisY != 0 && ( mAxisY >= 1.f || mAxisY <= -1.f ) ) ) {
			Float nmX = Mousef.x + mAxisX;
			Float nmY = Mousef.y + mAxisY;

			nmX = eemax<Float>( nmX, 0 );
			nmY = eemax<Float>( nmY, 0 );
			nmX = eemin( nmX, (Float)EE->getCurrentWindow()->getWidth() );
			nmY = eemin( nmY, (Float)EE->getCurrentWindow()->getHeight() );

			KM->injectMousePos( (Int32)nmX, (Int32)nmY );

			nmX -= (Int32)nmX;
			nmY -= (Int32)nmY;

			mAxisX = nmX;
			mAxisY = nmY;
		}
	}

	switch ( Screen ) {
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
					Map.move( ( mWindow->getElapsed().asMilliseconds() * 0.2f ), 0 );

				if ( HAT_RIGHT == hat || HAT_RIGHTDOWN == hat || HAT_RIGHTUP == hat )
					Map.move( -mWindow->getElapsed().asMilliseconds() * 0.2f, 0 );

				if ( HAT_UP == hat || HAT_LEFTUP == hat || HAT_RIGHTUP == hat )
					Map.move( 0, ( mWindow->getElapsed().asMilliseconds() * 0.2f ) );

				if ( HAT_DOWN == hat || HAT_LEFTDOWN == hat || HAT_RIGHTDOWN == hat )
					Map.move( 0, -mWindow->getElapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyDown( KEY_LEFT ) ) {
				Map.move( mWindow->getElapsed().asMilliseconds() * 0.2f, 0 );
			}

			if ( KM->isKeyDown( KEY_RIGHT ) ) {
				Map.move( -mWindow->getElapsed().asMilliseconds() * 0.2f, 0 );
			}

			if ( KM->isKeyDown( KEY_UP ) ) {
				Map.move( 0, mWindow->getElapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyDown( KEY_DOWN ) ) {
				Map.move( 0, -mWindow->getElapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyUp( KEY_F8 ) )
				Map.reset();

			break;
		case 2:
			if ( KM->isKeyUp( KEY_S ) )
				SP.setRepetitions( 1 );

			if ( KM->isKeyUp( KEY_A ) )
				SP.setRepetitions( -1 );

			if ( KM->isKeyUp( KEY_D ) )
				SP.setReverseAnimation( !SP.getReverseAnimation() );

			if ( KM->isMouseRightPressed() )
				DrawBack = true;
			else
				DrawBack = false;

			if ( KM->isKeyUp( KEY_P ) )
				SndMng.play( "mysound" );

			if ( KM->isControlPressed() && KM->isKeyUp( KEY_P ) ) {
				ShowParticles = !ShowParticles;
			}

			break;
	}
}

void EETest::update() {
	et = mWindow->getElapsed();

	SceneManager::instance()->update();

	input();

	mWindow->clear();

	render();

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	updateParticles();
#endif

	if ( KM->isKeyUp( KEY_F12 ) )
		mWindow->takeScreenshot( MyPath + "screenshots/" ); // After render and before Display

	mWindow->display( false );
}

void EETest::process() {
	init();

	if ( NULL != mWindow && mWindow->isOpen() ) {
		TestInstance = this;

		mWindow->runMainLoop( &mainLoop, mLastFPSLimit );
	}

	end();
}

void EETest::particlesCallback( Particle* P, ParticleSystem* Me ) {
	Float x, y, radio;
	Vector2f MePos( Me->getPosition() );

	radio = ( Math::randf( 1.f, 1.2f ) + sin( 20.0f / P->getId() ) ) * 24;
	x = MePos.x + radio * cos( (Float)P->getId() );
	y = MePos.y + radio * sin( (Float)P->getId() );
	P->reset( x, y, Math::randf( -10.f, 10.f ), Math::randf( -10.f, 10.f ),
			  Math::randf( -10.f, 10.f ), Math::randf( -10.f, 10.f ) );
	P->setColor( ColorAf( 1.f, 0.6f, 0.3f, 1.f ), 0.02f + Math::randf() * 0.3f );
}

void EETest::particles() {
	PS[0].setPosition( Mousef );

	if ( DrawBack )
		PS[1].setPosition( Mousef );

	PS[2].setPosition( HWidth, HHeight );
	PS[3].setPosition( Math::cosAng( Ang ) * 220.f + HWidth + Math::randf( 0.f, 10.f ),
					   Math::sinAng( Ang ) * 220.f + HHeight + Math::randf( 0.f, 10.f ) );
	PS[4].setPosition( -Math::cosAng( Ang ) * 220.f + HWidth + Math::randf( 0.f, 10.f ),
					   -Math::sinAng( Ang ) * 220.f + HHeight + Math::randf( 0.f, 10.f ) );

	for ( Uint32 i = 0; i < PS.size(); i++ )
		PS[i].draw();
}

#define GRABABLE_MASK_BIT ( 1u << 31u )
#define NOT_GRABABLE_MASK ( ~GRABABLE_MASK_BIT )

void EETest::createJointAndBody() {
#ifndef EE_PLATFORM_TOUCH
	mMouseJoint = NULL;
	mMouseBody = Body::New( INFINITY, INFINITY );
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
	Shape* shape;

	shape = mSpace->addShape(
		ShapeSegment::New( statiBody, cVectNew( 0, mWindow->getHeight() ),
						   cVectNew( mWindow->getWidth(), mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape(
		ShapeSegment::New( statiBody, cVectNew( mWindow->getWidth(), 0 ),
						   cVectNew( mWindow->getWidth(), mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( 0, 0 ),
												 cVectNew( 0, mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( 0, 0 ),
												 cVectNew( mWindow->getWidth(), 0 ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	Float hw = mWindow->getWidth() / 2;

	for ( int i = 0; i < 14; i++ ) {
		for ( int j = 0; j <= i; j++ ) {
			body = mSpace->addBody( Body::New( 1.0f, Moment::forBox( 1.0f, 30.0f, 30.0f ) ) );
			body->setPos( cVectNew( hw + j * 32 - i * 16, 100 + i * 32 ) );

			// shape = mSpace->AddShape( ShapePolySprite::New( body, 30.f, 30.f, mBoxSprite ) );
			shape = mSpace->addShape( ShapePoly::New( body, 30.f, 30.f ) );
			shape->setE( 0.0f );
			shape->setU( 0.8f );
		}
	}

	cpFloat radius = 15.0f;

	body =
		mSpace->addBody( Body::New( 10.0f, Moment::forCircle( 10.0f, 0.0f, radius, cVectZero ) ) );
	body->setPos( cVectNew( hw, mWindow->getHeight() - radius - 5 ) );

	// shape = mSpace->AddShape( ShapeCircleSprite::New( body, radius, cVectZero, mCircleSprite ) );
	shape = mSpace->addShape( ShapeCircle::New( body, radius, cVectZero ) );
	shape->setE( 0.0f );
	shape->setU( 0.9f );
}

void EETest::demo1Update() {}

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

cpBool EETest::blockerBegin( Arbiter* arb, Space*, void* ) {
	Shape *a, *b;
	arb->getShapes( &a, &b );

	Emitter* emitter = reinterpret_cast<Emitter*>( a->getData() );

	emitter->blocked++;

	return cpFalse; // Return values from sensors callbacks are ignored,
}

void EETest::blockerSeparate( Arbiter* arb, Space*, void* ) {
	Shape *a, *b;
	arb->getShapes( &a, &b );

	Emitter* emitter = reinterpret_cast<Emitter*>( a->getData() );

	emitter->blocked--;
}

void EETest::postStepRemove( Space*, void* tshape, void* ) {
	Shape* shape = reinterpret_cast<Shape*>( tshape );

#ifndef EE_PLATFORM_TOUCH
	if ( NULL != mMouseJoint &&
		 ( mMouseJoint->getA() == shape->getBody() || mMouseJoint->getB() == shape->getBody() ) ) {
		mSpace->removeConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}
#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( NULL != mMouseJoint[i] && ( mMouseJoint[i]->getA() == shape->getBody() ||
										 mMouseJoint[i]->getB() == shape->getBody() ) ) {
			mSpace->removeConstraint( mMouseJoint[i] );
			eeSAFE_DELETE( mMouseJoint[i] );
		}
	}
#endif

	mSpace->removeBody( shape->getBody() );
	mSpace->removeShape( shape );
	Shape::Free( shape, true );
}

cpBool EETest::catcherBarBegin( Arbiter* arb, Physics::Space*, void* ) {
	Shape *a, *b;
	arb->getShapes( &a, &b );

	Emitter* emitter = reinterpret_cast<Emitter*>( a->getData() );

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

	Body* statiBody = mSpace->getStaticBody();
	Shape* shape;

	emitterInstance.queue = 5;
	emitterInstance.blocked = 0;
	emitterInstance.position = cVectNew( mWindow->getWidth() / 2, 150 );

	shape = mSpace->addShape( ShapeCircle::New( statiBody, 15.0f, emitterInstance.position ) );
	shape->setSensor( 1 );
	shape->setCollisionType( BLOCKING_SENSOR_TYPE );
	shape->setData( &emitterInstance );

	// Create our catch sensor to requeue the balls when they reach the bottom of the screen
	shape = mSpace->addShape(
		ShapeSegment::New( statiBody, cVectNew( -4000, 600 ), cVectNew( 4000, 600 ), 15.0f ) );
	shape->setSensor( 1 );
	shape->setCollisionType( CATCH_SENSOR_TYPE );
	shape->setData( &emitterInstance );

	Space::CollisionHandler handler;
	handler.a = BLOCKING_SENSOR_TYPE;
	handler.b = BALL_TYPE;
	handler.begin = cb::Make3( this, &EETest::blockerBegin );
	handler.separate = cb::Make3( this, &EETest::blockerSeparate );
	mSpace->addCollisionHandler( handler );

	handler.reset(); // Reset all the values and the callbacks ( set the callbacks as !IsSet()

	handler.a = CATCH_SENSOR_TYPE;
	handler.b = BALL_TYPE;
	handler.begin = cb::Make3( this, &EETest::catcherBarBegin );
	mSpace->addCollisionHandler( handler );
}

void EETest::demo2Update() {
	if ( !emitterInstance.blocked && emitterInstance.queue ) {
		emitterInstance.queue--;

		Body* body =
			mSpace->addBody( Body::New( 1.0f, Moment::forCircle( 1.0f, 15.0f, 0.0f, cVectZero ) ) );
		body->setPos( emitterInstance.position );
		body->setVel( cVectNew( Math::randf( -1, 1 ), Math::randf( -1, 1 ) ) * (cpFloat)100 );

		Shape* shape = mSpace->addShape( ShapeCircle::New( body, 15.0f, cVectZero ) );
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
			mDemo[mCurDemo].destroy();

		mCurDemo = num;

		mDemo[mCurDemo].init();
	}
}

void EETest::physicsCreate() {
	PhysicsManager::createSingleton();
	PhysicsManager* PM = PhysicsManager::instance();
	PhysicsManager::DrawSpaceOptions* DSO = PM->getDrawOptions();

	DSO->DrawBBs = false;
	DSO->DrawShapes = true;
	DSO->CollisionPointSize = 0;
	DSO->BodyPointSize = 0;
	DSO->LineThickness = 1;

	mDemo.clear();

	physicDemo demo;

	demo.init = cb::Make0( this, &EETest::demo1Create );
	demo.update = cb::Make0( this, &EETest::demo1Update );
	demo.destroy = cb::Make0( this, &EETest::demo1Destroy );
	mDemo.push_back( demo );

	demo.init = cb::Make0( this, &EETest::demo2Create );
	demo.update = cb::Make0( this, &EETest::demo2Update );
	demo.destroy = cb::Make0( this, &EETest::demo2Destroy );
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

			Shape* shape = mSpace->pointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

			if ( NULL != shape ) {
				mMouseJoint = eeNew( PivotJoint, ( mMouseBody, shape->getBody(), cVectZero,
												   shape->getBody()->world2Local( point ) ) );

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
		InputFinger* Finger = KM->getFingerIndex( i );
		mMousePoint[i] = cVectNew( Finger->x, Finger->y );
		cVect newPoint =
			tovect( cpvlerp( tocpv( mMousePoint_last[i] ), tocpv( mMousePoint[i] ), 0.25 ) );
		mMouseBody[i]->setPos( newPoint );
		mMouseBody[i]->setVel( ( newPoint - mMousePoint_last[i] ) * (cpFloat)mWindow->getFPS() );
		mMousePoint_last[i] = newPoint;

		if ( Finger->isDown() ) {
			if ( NULL == mMouseJoint[i] ) {
				cVect point = cVectNew( Finger->x, Finger->y );

				Shape* shape = mSpace->pointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

				if ( NULL != shape ) {
					mMouseJoint[i] =
						eeNew( PivotJoint, ( mMouseBody[i], shape->getBody(), cVectZero,
											 shape->getBody()->world2Local( point ) ) );

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

	mDemo[mCurDemo].update();
	mSpace->update();
	mSpace->draw();
}

void EETest::physicsDestroy() {
	mDemo[mCurDemo].destroy();
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
}

} // namespace Demo_Test

EE_MAIN_FUNC int main( int, char*[] ) {
	Demo_Test::EETest* Test = eeNew( Demo_Test::EETest, () );

	Test->process();

	eeDelete( Test );

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
