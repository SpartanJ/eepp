#include "eetest.hpp"

Demo_Test::EETest * TestInstance = NULL;

static void MainLoop() {
	TestInstance->Update();
}

namespace Demo_Test {

void EETest::Init() {
	EE = Engine::instance();

	Log::instance()->liveWrite( true );
	Log::instance()->consoleOutput( true );

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

	mWindow = EE->createWindow( WinSettings, ConSettings );

	if ( NULL != mWindow && mWindow->created() ) {
		SetScreen( StartScreen );

		mWindow->caption( "eepp - Test Application" );
		mWindow->pushResizeCallback( cb::Make1( this, &EETest::OnWindowResize ) );

		TF = TextureFactory::instance();
		TF->allocate(40);

		Log		= Log::instance();
		KM		= mWindow->getInput();
		JM		= KM->getJoystickManager();

		PS.resize(5);

		Scenes[0] = cb::Make0( this, &EETest::PhysicsUpdate );
		Scenes[1] = cb::Make0( this, &EETest::Screen1 );
		Scenes[2] = cb::Make0( this, &EETest::Screen2 );
		Scenes[3] = cb::Make0( this, &EETest::Screen3 );
		Scenes[4] = cb::Make0( this, &EETest::Screen4 );
		Scenes[5] = cb::Make0( this, &EETest::Screen5 );

		//InBuf.Start();
		InBuf.supportNewLine( true );

		setRandomSeed( static_cast<Uint32>( Sys::getSystemTime() * 1000 ) );

		LoadTextures();

		LoadFonts();

		CreateShaders();

		if ( mMusEnabled ) {
			Mus = eeNew( Music, () );

			if ( Mus->openFromFile( MyPath + "sounds/music.ogg" ) ) {
				Mus->loop( true );
				Mus->play();
			}
		}

		WP.type( Ease::QuarticInOut );
		WP.addWaypoint( Vector2f(0,0), 100 );
		WP.addWaypoint( Vector2f(800,0), 100 );
		WP.addWaypoint( Vector2f(0,0), 100 );
		WP.addWaypoint( Vector2f(1024,768), 100 );
		WP.addWaypoint( Vector2f(0,600), 100 );
		WP.editWaypoint( 2, Vector2f(800,600), 100 );
		WP.eraseWaypoint( 3 );
		WP.loop(true);
		WP.setTotalTime( Milliseconds( 5000 ) );
		WP.start();

		Batch.allocVertexs( 2048 );
		Batch.setBlendMode( ALPHA_BLENDONE );

		mFBO = FrameBuffer::New( 256, 256, false );

		if ( NULL != mFBO )
			mFBO->clearColor( ColorAf( 0, 0, 0, 0.5f ) );

		Polygon2f Poly = Polygon2f::createRoundedRectangle( 0, 0, 256, 50 );

		mVBO = VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, DM_TRIANGLE_FAN );

		if ( NULL != mVBO ) {
			for ( Uint32 i = 0; i < Poly.size(); i++ ) {
				mVBO->addVertex( Poly[i] );
				mVBO->addColor( ColorA( 100 + i, 255 - i, 150 + i, 200 ) );
			}

			mVBO->compile();
		}

		PhysicsCreate();

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		launch();
#endif

	} else {
		Engine::destroySingleton();

		exit(0);
	}
}

void EETest::CreateUIThemeTextureAtlas() {
	#if !defined( EE_DEBUG ) || defined( EE_GLES )
	return;
	#endif

	std::string tgpath( MyPath + "ui/uitheme" );
	std::string Path( MyPath + "ui/uitheme" );

	if ( !FileSystem::fileExists( tgpath + EE_TEXTURE_ATLAS_EXTENSION ) ) {
		TexturePacker tp( 256, 256, true, 2 );
		tp.addTexturesPath( Path );
		tp.packTextures();
		tp.save( tgpath + ".png", SAVE_TYPE_PNG );
	} else {
		TextureAtlasLoader tgl;
		tgl.updateTextureAtlas( tgpath + EE_TEXTURE_ATLAS_EXTENSION, Path );
	}
}

void EETest::LoadFonts() {
	mFTE.restart();

	TextureLoader * tl = eeNew( TextureLoader, ( MyPath + "fonts/conchars.png" ) );
	tl->setColorKey( RGB(0,0,0) );

	mFontLoader.add( eeNew( TextureFontLoader, ( "conchars", tl, (unsigned int)32 ) ) );
	mFontLoader.add( eeNew( TextureFontLoader, ( "ProggySquareSZ", eeNew( TextureLoader, ( MyPath + "fonts/ProggySquareSZ.png" ) ), MyPath + "fonts/ProggySquareSZ.dat" ) ) );
	mFontLoader.add( eeNew( TTFFontLoader, ( "arial", MyPath + "fonts/arial.ttf", 12, TTF_STYLE_NORMAL, 256, RGB(255,255,255) ) ) );
	mFontLoader.add( eeNew( TTFFontLoader, ( "arialb", MyPath + "fonts/arial.ttf", 12, TTF_STYLE_NORMAL, 256, RGB(255,255,255), 1, RGB(0,0,0), true ) ) );

	mFontLoader.load( cb::Make1( this, &EETest::OnFontLoaded ) );
}

void EETest::OnFontLoaded( ResourceLoader * ObjLoaded ) {
	FF		= FontManager::instance()->getByName( "conchars" );
	FF2		= FontManager::instance()->getByName( "ProggySquareSZ" );
	TTF		= FontManager::instance()->getByName( "arial" );
	TTFB	= FontManager::instance()->getByName( "arialb" );

	eePRINTL( "Fonts loading time: %4.3f ms.", mFTE.elapsed().asMilliseconds() );

	eeASSERT( TTF != NULL );
	eeASSERT( TTFB != NULL );

	Con.create( FF, true );
	Con.ignoreCharOnPrompt( 186 ); // 'º'

	mBuda = String::fromUtf8( "El mono ve el pez en el agua y sufre. Piensa que su mundo es el único que existe, el mejor, el real. Sufre porque es bueno y tiene compasión, lo ve y piensa: \"Pobre se está ahogando no puede respirar\". Y lo saca, lo saca y se queda tranquilo, por fin lo salvé. Pero el pez se retuerce de dolor y muere. Por eso te mostré el sueño, es imposible meter el mar en tu cabeza, que es un balde." );

	CreateUI();

	mEEText.create( TTFB, "Entropia Engine++\nCTRL + Number to change Demo Screen\nRight click to see the PopUp Menu" );
	mFBOText.create( TTFB, "This is a VBO\nInside of a FBO" );
	mFBOText.color( ColorA(255,255,0,255), mFBOText.text().find( "VBO" ), mFBOText.text().find( "VBO" ) + 2 );
	mFBOText.color( ColorA(255,255,0,255), mFBOText.text().find( "FBO" ), mFBOText.text().find( "FBO" ) + 2 );

	mInfoText.create( FF, "", ColorA(255,255,255,150) );
}

void EETest::CreateShaders() {
	mUseShaders = mUseShaders && GLi->shadersSupported();

	mShaderProgram = NULL;

	if ( mUseShaders ) {
		mBlurFactor = 0.01f;
		mShaderProgram = ShaderProgram::New( MyPath + "shaders/blur.vert", MyPath + "shaders/blur.frag" );
	}
}

void EETest::OnWinMouseUp( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	UIControlAnim * CtrlAnim;

	if ( Event->Ctrl()->IsType( UI_TYPE_WINDOW ) ) {
		CtrlAnim = reinterpret_cast<UIControlAnim*>( Event->Ctrl() );
	} else {
		CtrlAnim = reinterpret_cast<UIControlAnim*>( Event->Ctrl()->Parent() );
	}

	if ( MEvent->Flags() & EE_BUTTON_WUMASK ) {
		CtrlAnim->Scale( CtrlAnim->Scale() + 0.1f );
	} else if ( MEvent->Flags() & EE_BUTTON_WDMASK ) {
		CtrlAnim->Scale( CtrlAnim->Scale() - 0.1f );
	}
}

void EETest::OnShowMenu( const UIEvent * Event ) {
	UIPushButton * PB = static_cast<UIPushButton*>( Event->Ctrl() );

	if ( Menu->Show() ) {
		Vector2i Pos = Vector2i( (Int32)PB->GetPolygon()[0].x, (Int32)PB->GetPolygon()[0].y - 2 );
		UIMenu::FixMenuPos( Pos , Menu );
		Menu->Pos( Pos );
	}
}

void EETest::OnWindowResize(EE::Window::Window * win) {
	Map.ViewSize( win->size() );
}

void EETest::CreateUI() {
	Clock TE;

	CreateUIThemeTextureAtlas();

	eePRINTL( "Texture Atlas Loading Time: %4.3f ms.", TE.elapsed().asMilliseconds() );

	UIManager::instance()->Init(); //UI_MANAGER_HIGHLIGHT_FOCUS | UI_MANAGER_HIGHLIGHT_OVER

	//mTheme = UITheme::LoadFromPath( eeNew( UIDefaultTheme, ( "uitheme", "uitheme" ) ), MyPath + "uitheme/" );

	TextureAtlasLoader tgl( MyPath + "ui/uitheme" + EE_TEXTURE_ATLAS_EXTENSION );

	mTheme = UITheme::LoadFromTextureAtlas( eeNew( UIDefaultTheme, ( "uitheme", "uitheme" ) ), TextureAtlasManager::instance()->getByName( "uitheme" ) );

	UIThemeManager::instance()->add( mTheme );
	UIThemeManager::instance()->DefaultEffectsEnabled( true );
	UIThemeManager::instance()->DefaultFont( TTF );
	UIThemeManager::instance()->DefaultTheme( "uitheme" );

	UIControl::CreateParams Params( UIManager::instance()->MainControl(), Vector2i(0,0), Sizei( 530, 380 ), UI_FILL_BACKGROUND | UI_CLIP_ENABLE | UI_BORDER );

	Params.Border.Width( 2 );
	Params.Border.Color( 0x979797CC );
	Params.Background.Colors( ColorA( 0xEDEDED66 ), ColorA( 0xEDEDEDCC ), ColorA( 0xEDEDEDCC ), ColorA( 0xEDEDED66 ) );

	UIWindow * tWin = mTheme->CreateWindow( NULL, Sizei( 530, 405 ), Vector2i( 320, 240 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DRAGABLE_CONTAINER , Sizei( 530, 405 ), 200 );
	C = tWin->Container();

	tWin->Title( "Controls Test" );

	tWin->AddEventListener( UIEvent::EventMouseUp, cb::Make1( this, &EETest::OnWinMouseUp ) );
	C->AddEventListener( UIEvent::EventMouseUp, cb::Make1( this, &EETest::OnWinMouseUp ) );

	Params.Flags &= ~UI_CLIP_ENABLE;
	Params.Background.Corners(0);
	Params.Background.Colors( ColorA( 0x00FF0077 ), ColorA( 0x00CC0077 ), ColorA( 0x00CC0077 ), ColorA( 0x00FF0077 ) );
	Params.Parent( C );
	Params.Size = Sizei( 50, 50 );
	UITest * Child = eeNew( UITest, ( Params ) );
	Child->Pos( 240, 130 );
	Child->Visible( true );
	Child->Enabled( true );
	Child->StartRotation( 0.f, 360.f, Milliseconds( 5000.f ) );
	Child->RotationInterpolation()->loop( true );

	Params.Background.Colors( ColorA( 0xFFFF0077 ), ColorA( 0xCCCC0077 ), ColorA( 0xCCCC0077 ), ColorA( 0xFFFF0077 ) );
	Params.Parent( Child );
	Params.Size = Sizei( 25, 25 );
	UITest * Child2 = eeNew( UITest, ( Params ) );
	Child2->Pos( 15, 15 );
	Child2->Visible( true );
	Child2->Enabled( true );
	Child2->StartRotation( 0.f, 360.f, Milliseconds( 5000.f ) );
	Child2->RotationInterpolation()->loop( true );

	mTheme->CreateSprite( eeNew( Sprite, ( "gn" ) ), C, Sizei(), Vector2i( 160, 100 ) );

	UITextBox::CreateParams TextParams;
	TextParams.Parent( C );
	TextParams.PosSet( 0, 0 );
	TextParams.Size = Sizei( 320, 240 );
	TextParams.Flags = UI_VALIGN_TOP | UI_HALIGN_RIGHT;
	UITextBox * Text = eeNew( UITextBox, ( TextParams ) );
	Text->Visible( true );
	Text->Enabled( false );
	Text->Text( "Turn around\nJust Turn Around\nAround!" );

	UITextInput::CreateParams InputParams;
	InputParams.Parent( C );
	InputParams.PosSet( 20, 216 );
	InputParams.Size = Sizei( 200, 22 );
	InputParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED;
	UITextInput * Input = eeNew( UITextInput, ( InputParams ) );
	Input->Visible( true );
	Input->Enabled( true );

	UIPushButton::CreateParams ButtonParams;
	ButtonParams.Parent( C );
	ButtonParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE;
	ButtonParams.PosSet( 225, 216 );
	ButtonParams.Size = Sizei( 90, 0 );
	ButtonParams.SetIcon( mTheme->GetIconByName( "ok" ) );
	UIPushButton * Button = eeNew( UIPushButton, ( ButtonParams ) );
	Button->Visible( true );
	Button->Enabled( true );
	Button->Text( "Click Me" );
	Button->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &EETest::ButtonClick ) );
	Button->TooltipText( "Click and see what happens..." );

	TextParams.PosSet( 130, 20 );
	TextParams.Size = Sizei( 80, 22 );
	TextParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	UICheckBox * Checkbox = eeNew( UICheckBox, ( TextParams ) );
	Checkbox->Visible( true );
	Checkbox->Text( "Check Me" );
	Checkbox->Enabled( true );

	TextParams.PosSet( 130, 40 );
	UIRadioButton * RadioButton = eeNew( UIRadioButton, ( TextParams ) );
	RadioButton->Visible( true );
	RadioButton->Text( "Check Me" );
	RadioButton->Enabled( true );

	TextParams.PosSet( 130, 60 );
	RadioButton = eeNew( UIRadioButton, ( TextParams ) );
	RadioButton->Visible( true );
	RadioButton->Text( "Check Me 2" );
	RadioButton->Enabled( true );

	UISlider::CreateParams SliderParams;
	SliderParams.Parent( C );
	SliderParams.PosSet( 220, 80 );
	SliderParams.Size = Sizei( 80, 24 );
	mSlider = eeNew( UISlider, ( SliderParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );
	mSlider->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &EETest::OnSliderValueChange ) );

	SliderParams.PosSet( 40, 110 );
	SliderParams.Size = Sizei( 24, 80 );
	SliderParams.VerticalSlider = true;
	mSlider = eeNew( UISlider, ( SliderParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );

	SliderParams.PosSet( 60, 110 );
	mSlider = eeNew( UISlider, ( SliderParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );

	UISpinBox::CreateParams SpinBoxParams;
	SpinBoxParams.Parent( C );
	SpinBoxParams.PosSet( 80, 150 );
	SpinBoxParams.Size = Sizei( 80, 24 );
	SpinBoxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_CLIP_ENABLE;
	SpinBoxParams.AllowDotsInNumbers = true;
	UISpinBox * mSpinBox = eeNew( UISpinBox, ( SpinBoxParams ) );
	mSpinBox->Visible( true );
	mSpinBox->Enabled( true );

	UIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent( C );
	ScrollBarP.PosSet( 0, 0 );
	ScrollBarP.Size = Sizei( 15, 240 );
	ScrollBarP.Flags = UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar = true;
	mScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );
	mScrollBar->Visible( true );
	mScrollBar->Enabled( true );
	mScrollBar->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &EETest::OnValueChange ) );

	mProgressBar = mTheme->CreateProgressBar( C, Sizei( 200, 20 ), Vector2i( 20, 190 ) );

	TextParams.PosSet( 20, 5 );
	mTextBoxValue = eeNew( UITextBox, ( TextParams ) );
	mTextBoxValue->Visible( true );
	OnValueChange( NULL );

	UIListBox::CreateParams LBParams;
	LBParams.Parent( C );
	LBParams.PosSet( 325, 8 );
	LBParams.Size = Sizei( 200, 240-16 );
	LBParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TOUCH_DRAG_ENABLED; // | UI_MULTI_SELECT
	mListBox = eeNew( UIListBox, ( LBParams ) );
	mListBox->Visible( true );
	mListBox->Enabled( true );

	Int32 wsize = 100;

	if ( wsize ) {
		std::vector<String> str(wsize);

		for ( Int32 i = 1; i <= wsize; i++ )
			str[i-1] = "Test ListBox " + String::toStr(i) + " testing it right now!";

		mListBox->AddListBoxItems( str );
	}

	UIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( C );
	DDLParams.PosSet( 20, 55 );
	DDLParams.Size = Sizei( 100, 21 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_TOUCH_DRAG_ENABLED;
	UIDropDownList * mDropDownList = eeNew( UIDropDownList, ( DDLParams ) );
	mDropDownList->Visible( true );
	mDropDownList->Enabled( true );

	std::vector<String> combostrs;
	combostrs.push_back( "Plane" );
	combostrs.push_back( "Car" );
	combostrs.push_back( "Bus" );
	combostrs.push_back( "Train" );
	combostrs.push_back( "Overcraft" );
	combostrs.push_back( "Spaceship" );
	combostrs.push_back( "Bike" );
	combostrs.push_back( "Motorbike" );

	mDropDownList->ListBox()->AddListBoxItems( combostrs );
	mDropDownList->ListBox()->SetSelected( 0 );

	UIComboBox::CreateParams ComboParams;
	ComboParams.Parent( C );
	ComboParams.PosSet( 20, 80 );
	ComboParams.Size = Sizei( 100, 1 );
	ComboParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_AUTO_SIZE | UI_TOUCH_DRAG_ENABLED | UI_TEXT_SELECTION_ENABLED;
	UIComboBox * mComboBox = eeNew( UIComboBox, ( ComboParams ) );
	mComboBox->Visible( true );
	mComboBox->Enabled( true );

	mComboBox->ListBox()->AddListBoxItems( combostrs );
	mComboBox->ListBox()->SetSelected( 0 );

	Menu = mTheme->CreatePopUpMenu();
	Menu->Add( "New", mTheme->GetIconByName( "document-new" ) );

	Menu->Add( "Open...", mTheme->GetIconByName( "document-open" ) );
	Menu->AddSeparator();
	Menu->Add( "Map Editor" );
	Menu->Add( "Texture Atlas Editor" );
	Menu->AddSeparator();
	Menu->Add( "Show Screen 1" );
	Menu->Add( "Show Screen 2" );
	Menu->Add( "Show Screen 3" );
	Menu->Add( "Show Screen 4" );
	Menu->Add( "Show Screen 5" );
	Menu->Add( "Show Screen 6" );
	Menu->AddSeparator();
	Menu->Add( "Show Console" );
	Menu->AddSeparator();
	Menu->AddCheckBox( "Show Window" );
	Menu->Add( "Show Window 2" );
	Menu->AddCheckBox( "Multi Viewport" );

	UIPopUpMenu * Menu3 = mTheme->CreatePopUpMenu();
	Menu3->Add( "Hello World 1" );
	Menu3->Add( "Hello World 2" );
	Menu3->Add( "Hello World 3" );
	Menu3->Add( "Hello World 4" );

	UIPopUpMenu * Menu2 = mTheme->CreatePopUpMenu();
	Menu2->Add( "Test 1" );
	Menu2->Add( "Test 2" );
	Menu2->Add( "Test 3" );
	Menu2->Add( "Test 4" );
	Menu2->AddSubMenu( "Hello World", NULL, Menu3 );

	Menu->AddSeparator();
	Menu->AddSubMenu( "Sub-Menu", NULL, Menu2 ) ;

	Menu->AddSeparator();
	Menu->Add( "Quit" );

	Menu->AddEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &EETest::ItemClick ) );
	Menu->GetItem( "Quit" )->AddEventListener( UIEvent::EventMouseUp, cb::Make1( this, &EETest::QuitClick ) );
	UIManager::instance()->MainControl()->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &EETest::MainClick ) );

	UITextEdit::CreateParams TEParams;
	TEParams.Parent( C );
	TEParams.PosSet( 5, 245 );
	TEParams.Size	= Sizei( 315, 130 );
	TEParams.Flags = UI_AUTO_PADDING | UI_CLIP_ENABLE | UI_TEXT_SELECTION_ENABLED;
	UITextEdit * TextEdit = eeNew( UITextEdit, ( TEParams ) );
	TextEdit->Visible( true );
	TextEdit->Enabled( true );
	TextEdit->Text( mBuda );

	UIGenericGrid::CreateParams GridParams;
	GridParams.Parent( C );
	GridParams.PosSet( 325, 245 );
	GridParams.SizeSet( 200, 130 );
	GridParams.Flags = UI_AUTO_PADDING | UI_TOUCH_DRAG_ENABLED;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 3;
	UIGenericGrid * mGenGrid = eeNew( UIGenericGrid, ( GridParams ) );
	mGenGrid->Visible( true );
	mGenGrid->Enabled( true );

	UIGridCell::CreateParams CellParams;
	CellParams.Parent( mGenGrid->Container() );

	UITextBox::CreateParams TxtBoxParams;
	UITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_TEXT_SELECTION_ENABLED;

	UIGfx::CreateParams TxtGfxParams;
	TxtGfxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	TxtGfxParams.SubTexture = mTheme->GetIconByName( "ok" );

	for ( Uint32 i = 0; i < 100; i++ ) {
		UIGridCell * Cell			= eeNew( UIGridCell, ( CellParams ) );
		UITextBox * TxtBox			= eeNew( UITextBox, ( TxtBoxParams ) );
		UITextInput * TxtInput		= eeNew( UITextInput, ( TxtInputParams ) );
		UIGfx * TxtGfx				= eeNew( UIGfx, ( TxtGfxParams )  );

		TxtBox->Text( "Test " + String::toStr( i+1 ) );

		Cell->Cell( 0, TxtBox );
		Cell->Cell( 1, TxtGfx );
		Cell->Cell( 2, TxtInput );

		mGenGrid->Add( Cell );
	}

	mGenGrid->CollumnWidth( 0, 50 );
	mGenGrid->CollumnWidth( 1, 24 );
	mGenGrid->CollumnWidth( 2, 100 );

#ifdef EE_PLATFORM_TOUCH
	TextureAtlas * SG = GlobalTextureAtlas::instance();

	Texture * butTex = TF->getTexture( TF->Load( MyPath + "sprites/button-te_normal.png" ) );

	SG->Add( butTex->Id(), "button-te_normal" );
	SG->Add( TF->Load( MyPath + "sprites/button-te_mdown.png" ), "button-te_mdown" );

	UISkinSimple nSkin( "button-te" );

	mShowMenu = mTheme->CreatePushButton( NULL, butTex->Size(), Vector2i( mWindow->GetWidth() - butTex->Width() - 20, mWindow->GetHeight() - butTex->Height() - 10 ), UI_CONTROL_ALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mShowMenu->SetSkin( nSkin );
	mShowMenu->Text( "Show Menu" );
	mShowMenu->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &EETest::OnShowMenu ) );
#endif

	C = reinterpret_cast<UIControlAnim*> ( C->Parent() );

	eePRINTL( "CreateUI time: %4.3f ms.", TE.elapsed().asMilliseconds() );
}

void EETest::CreateMapEditor() {
	if ( NULL != mMapEditor )
		return;

	UIWindow * tWin = mTheme->CreateWindow( NULL, Sizei( 1024, 768 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, Sizei( 1024, 768 ) );
	mMapEditor = eeNew( MapEditor, ( tWin, cb::Make0( this, &EETest::OnMapEditorClose ) ) );
	tWin->Center();
	tWin->Show();
}

void EETest::OnMapEditorClose() {
	mMapEditor = NULL;
}

void EETest::CreateETGEditor() {
	UIWindow * tWin = mTheme->CreateWindow( NULL, Sizei( 1024, 768 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, Sizei( 1024, 768 ) );
	mETGEditor = eeNew ( Tools::TextureAtlasEditor, ( tWin, cb::Make0( this, &EETest::OnETGEditorClose ) ) );
	tWin->Center();
	tWin->Show();
}

void EETest::OnETGEditorClose() {
	mETGEditor = NULL;
}

void EETest::CreateCommonDialog() {
	UICommonDialog * CDialog = mTheme->CreateCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON );
	CDialog->AddFilePattern( "*.hpp;*.cpp", true );
	CDialog->Center();
	CDialog->Show();
}

void EETest::CreateWinMenu() {
	UIWinMenu * WinMenu = mTheme->CreateWinMenu( mUIWindow->Container() );

	UIPopUpMenu * PopMenu = mTheme->CreatePopUpMenu();
	PopMenu->Add( "File" );
	PopMenu->Add( "Open" );
	PopMenu->Add( "Close" );
	PopMenu->Add( "Quit" );

	UIPopUpMenu * PopMenu2 = mTheme->CreatePopUpMenu();
	PopMenu2->Add( "Bla" );
	PopMenu2->Add( "Bla 2" );
	PopMenu2->Add( "Bla 3" );
	PopMenu2->Add( "Bla 4" );

	WinMenu->AddMenuButton( "File", PopMenu );
	WinMenu->AddMenuButton( "Edit", PopMenu2 );
}

void EETest::CreateDecoratedWindow() {
	mUIWindow = mTheme->CreateWindow( NULL, Sizei( 530, 350 ), Vector2i( 200, 50 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON, Sizei( 100, 200 ) );

	mUIWindow->AddEventListener( UIEvent::EventOnWindowCloseClick, cb::Make1( this, &EETest::CloseClick ) );
	mUIWindow->Title( "Test Window" );
	mUIWindow->ToBack();

	UIPushButton * Button = mTheme->CreatePushButton( mUIWindow->Container(), Sizei( 510, 22 ), Vector2i( 10, 28 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_ANCHOR_RIGHT );
	Button->Text( "Click Me" );
	Button->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &EETest::ButtonClick ) );

	mUIWindow->AddShortcut( KEY_C, KEYMOD_ALT, Button );

	UITabWidget * TabWidget = mTheme->CreateTabWidget( mUIWindow->Container(), Sizei( 510, 250 ), Vector2i( 10, 55 ), UI_HALIGN_CENTER | UI_VALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT | UI_ANCHOR_TOP );

	UITextEdit * TEdit = mTheme->CreateTextEdit( TabWidget, Sizei(), Vector2i() );
	TEdit->Text( mBuda );
	TabWidget->Add( "TextEdit", TEdit );

	UITextInput * Txt = mTheme->CreateTextInput( TabWidget, Sizei(), Vector2i(), UI_AUTO_PADDING | UI_AUTO_SHRINK_TEXT | UI_TEXT_SELECTION_ENABLED );
	Txt->Text( mBuda );
	TabWidget->Add( "TextInput", Txt );

	TabWidget->Add( "TextBox", mTheme->CreateTextBox( mBuda, TabWidget, Sizei(), Vector2i(), UI_AUTO_PADDING | UI_AUTO_SHRINK_TEXT | UI_TEXT_SELECTION_ENABLED ) );

	CreateWinMenu();
}

void EETest::CloseClick( const UIEvent * Event ) {
	mUIWindow = NULL;
}

void EETest::ItemClick( const UIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "Show Screen 1" == txt ) {
		SetScreen( 0 );
	} else if ( "Show Screen 2" == txt ) {
		SetScreen( 1 );
	} else if ( "Show Screen 3" == txt ) {
		SetScreen( 2 );
	} else if ( "Show Screen 4" == txt ) {
		SetScreen( 3 );
	} else if ( "Show Screen 5" == txt ) {
		SetScreen( 4 );
	} else if ( "Show Screen 6" == txt ) {
		SetScreen( 5 );
	} else if ( "Show Console" == txt ) {
		Con.toggle();
		InBuf.active( !Con.active() );

		if ( Con.active() ) {
			mWindow->startTextInput();
		} else {
			mWindow->stopTextInput();
		}
	} else if ( "Show Window" == txt ) {
		UIMenuCheckBox * Chk = reinterpret_cast<UIMenuCheckBox*> ( Event->Ctrl() );

		C->Visible( true );
		C->Enabled( true );

		if ( Chk->Active() ) {
			if ( C->Scale() == 1.f ) C->Scale( 0.f );
			C->StartScaleAnim( C->Scale(), Vector2f::One, Milliseconds( 500.f ), Ease::SineOut );
			C->StartAlphaAnim( C->Alpha(), 255.f, Milliseconds( 500.f ) );
			C->StartRotation( 0, 360, Milliseconds( 500.f ), Ease::SineOut );
		} else {
			C->StartScaleAnim( C->Scale(), Vector2f::Zero, Milliseconds( 500.f ), Ease::SineIn );
			C->StartAlphaAnim( C->Alpha(), 0.f, Milliseconds( 500.f ) );
			C->StartRotation( 0, 360, Milliseconds( 500.f ), Ease::SineIn );
		}
	} else if ( "Show Window 2" == txt ) {
		if ( NULL == mUIWindow ) {
			CreateDecoratedWindow();
		}

		mUIWindow->Show();
	} else if ( "Map Editor" == txt ) {
		CreateMapEditor();
	} else if ( "Texture Atlas Editor" == txt ) {
		CreateETGEditor();
	} else if ( "Multi Viewport" == txt ) {
		MultiViewportMode = !MultiViewportMode;
	} else if ( "Open..." == txt ) {
		CreateCommonDialog();
	} else if ( "New" == txt ) {
		if ( 0 == Screen ) {
			ChangeDemo( 0 );
		}
	}
}

void EETest::OnValueChange( const UIEvent * Event ) {
	mTextBoxValue->Text( "Scroll Value:\n" + String::toStr( mScrollBar->Value() ) );

	mProgressBar->Progress( mScrollBar->Value() * 100.f );
}

void EETest::OnSliderValueChange( const UIEvent * Event ) {
	UISlider * slider = static_cast<UISlider*>( Event->Ctrl() );

	C->Angle( slider->Value() * 90.f );
}

void EETest::QuitClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		mWindow->close();
	}
}

void EETest::ShowMenu() {
	if ( Menu->Show() ) {
		Vector2i Pos = mWindow->getInput()->getMousePos();
		UIMenu::FixMenuPos( Pos , Menu );
		Menu->Pos( Pos );
	}
}

void EETest::MainClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_RMASK ) {
		ShowMenu();
	}
}

void EETest::ButtonClick( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTONS_LRM ) {
		UIGfx::CreateParams GfxParams;
		GfxParams.Parent( UIManager::instance()->MainControl() );
		GfxParams.SubTexture = mTheme->GetIconByName( "ok" );
		UIGfx * Gfx = eeNew( UIGfx, ( GfxParams ) );
		Gfx->Visible( true );
		Gfx->Enabled( false );

		Gfx->StartRotation( 0, 2500, Milliseconds( 2500 ) );
		Gfx->StartMovement( Vector2i( Math::randi( 0, mWindow->getWidth() ), -64 ), Vector2i( Math::randi( 0, mWindow->getWidth() ), mWindow->getHeight() + 64 ), Milliseconds( 2500 ) );
		Gfx->CloseFadeOut( Milliseconds( 3500 ) );

		mListBox->AddListBoxItem( "Test ListBox " + String::toStr( mListBox->Count() + 1 ) + " testing it right now!" );
	}
}

void EETest::SetScreen( Uint32 num ) {
	if ( NULL != mTerrainBut ) mTerrainBut->Visible( 1 == num );

	if ( 0 == num || 5 == num )
		mWindow->backColor( RGB( 240, 240, 240 ) );
	else
		mWindow->backColor( RGB( 0, 0, 0 ) );

	if ( num < 6 )
		Screen = num;
}

void EETest::CmdSetPartsNum ( const std::vector < String >& params ) {
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

void EETest::OnTextureLoaded( ResourceLoader * ResLoaded ) {
	SndMng.play( "mysound" );
}

void EETest::LoadTextures() {
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

	mResLoad.load( cb::Make1( this, &EETest::OnTextureLoaded ) );

	TN.resize(12);
	TNP.resize(12);

	for ( i = 0; i <= 6; i++ ) {
		TN[i] = TF->load( MyPath + "sprites/" + String::toStr(i+1) + ".png", (i+1) == 7 ? true : false, ( (i+1) == 4 ) ? CLAMP_REPEAT : CLAMP_TO_EDGE );
		TNP[i] = TF->getTexture( TN[i] );
	}

	Tiles.resize(10);

	TextureAtlasLoader tgl( MyPath + "atlases/tiles.eta" );
	TextureAtlas * SG = TextureAtlasManager::instance()->getByName( "tiles" );

	if ( NULL != SG ) {
		for ( i = 0; i < 6; i++ ) {
			Tiles[i] = SG->getByName( String::toStr( i+1 ) );
		}

		Tiles[6] = SG->add( TF->load( MyPath + "sprites/objects/1.png" ), "7" );

		#ifdef EE_GLES
		Image tImg( MyPath + "sprites/objects/2.png", 4 );
		tImg.CreateMaskFromColor( ColorA(0,0,0,255), 0 );
		Tiles[7] = SG->Add( TF->loadFromPixels( tImg.GetPixelsPtr(), tImg.Width(), tImg.Height(), tImg.Channels() ), "8" );
		#else
		Tiles[7] = SG->add( TF->load( MyPath + "sprites/objects/2.png" ), "8" );
		Tiles[7]->getTexture()->createMaskFromColor( ColorA(0,0,0,255), 0 );
		#endif
	}

	int w, h;

	for ( Int32 my = 0; my < 4; my++ )
		for( Int32 mx = 0; mx < 8; mx++ )
			SP.addFrame( TN[4], Sizef( 0, 0 ), Vector2i( 0, 0 ), Recti( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );

	PS[0].setCallbackReset( cb::Make2( this, &EETest::ParticlesCallback ) );
	PS[0].create( PSE_Callback, 500, TN[5], Vector2f( 0, 0 ), 16, true );
	PS[1].create( PSE_Heal, 250, TN[5], Vector2f( mWindow->getWidth() * 0.5f, mWindow->getHeight() * 0.5f ), 16, true );
	PS[2].create( PSE_WormHole, PartsNum, TN[5], Vector2f( mWindow->getWidth() * 0.5f, mWindow->getHeight() * 0.5f ), 32, true );
	PS[3].create( PSE_Fire, 350, TN[5], Vector2f( -50.f, -50.f ), 32, true );
	PS[4].create( PSE_Fire, 350, TN[5], Vector2f( -50.f, -50.f ), 32, true );

	Con.addCommand( "setparticlesnum", cb::Make1( this, &EETest::CmdSetPartsNum ) );

	Texture * Tex = TNP[2];

	if ( NULL != Tex && Tex->lock() ) {
		w = (int)Tex->width();
		h = (int)Tex->height();

		for ( y = 0; y < h; y++) {
			for ( x = 0; x < w; x++) {
				ColorA C = Tex->getPixel(x, y);

				if ( C.r() > 200 && C.g() > 200 && C.b() > 200 )
					Tex->setPixel(x, y, ColorA( Math::randi(0, 255), Math::randi(0, 255), Math::randi(0, 255), C.a() ) );
				else
					Tex->setPixel(x, y, ColorA( Math::randi(200, 255), Math::randi(200, 255), Math::randi(200, 255), C.a() ) );
			}
		}

		Tex->unlock(false, true);
	}

	Cursor[0] = TF->load( MyPath + "cursors/cursor.tga" );
	CursorP[0] = TF->getTexture( Cursor[0] );

	CursorManager * CurMan = mWindow->getCursorManager();
	CurMan->visible( false );
	CurMan->visible( true );
	CurMan->set( EE::Window::SYS_CURSOR_HAND );
	CurMan->setGlobalCursor( EE_CURSOR_ARROW, CurMan->add( CurMan->create( CursorP[0], Vector2i( 1, 1 ), "cursor_special" ) ) );
	CurMan->set( EE_CURSOR_ARROW );

	CL1.addFrame( TN[2] );
	CL1.position( 500, 400 );
	CL1.scale( 0.5f );

	CL2.addFrame(TN[0], Sizef(96, 96) );
	CL2.color( ColorA( 255, 255, 255, 255 ) );

	mTGL = eeNew( TextureAtlasLoader, ( MyPath + "atlases/bnb" + EE_TEXTURE_ATLAS_EXTENSION ) );

	mBlindy.addFramesByPattern( "rn" );
	mBlindy.position( 320.f, 0.f );

	mBoxSprite = eeNew( Sprite, ( GlobalTextureAtlas::instance()->add( eeNew( SubTexture, ( TN[3], "ilmare" ) ) ) ) );
	mCircleSprite = eeNew( Sprite, ( GlobalTextureAtlas::instance()->add( eeNew( SubTexture, ( TN[1], "thecircle" ) ) ) ) );

	eePRINTL( "Textures loading time: %4.3f ms.", TE.elapsed().asMilliseconds() );

	Map.Load( MyPath + "maps/test.eem" );
	Map.DrawGrid( false );
	Map.ClipedArea( false );
	Map.DrawBackground( false );
	Map.ViewSize( mWindow->size() );

	eePRINTL( "Map creation time: %4.3f ms.", TE.elapsed().asMilliseconds() );
}

void EETest::run() {
	ParticlesThread();
}

void EETest::ParticlesThread() {
	while ( mWindow->isRunning() ) {
		UpdateParticles();
		Sys::sleep(10);
	}
}

void EETest::UpdateParticles() {
	if ( MultiViewportMode || Screen == 2 ) {
		PSElapsed = cElapsed.elapsed();

		for ( Uint8 i = 0; i < PS.size(); i++ )
			PS[i].update( PSElapsed );
	}
}

void EETest::Screen1() {
	Map.Update();
	Map.Draw();
}

void EETest::Screen2() {
	if ( mResLoad.isLoaded() ) {
		Texture * TexLoaded = TF->getByName( "1.jpg" );

		if ( NULL != TexLoaded )
			TexLoaded->draw( 0, 0 );
	}

	if ( KM->mouseLeftPressed() )
		TNP[3]->drawEx( 0.f, 0.f, (Float)mWindow->getWidth(), (Float)mWindow->getHeight() );

	Batch.setTexture( TNP[2] );
	Batch.quadsBegin();
	Batch.quadsSetColor( ColorA(150,150,150,100) );
	Batch.quadsSetSubset( 0.0f, 0.0f, 0.5f, 0.5f );

	Batch.batchRotation( ang );
	Batch.batchScale( scale );
	Batch.batchCenter( Vector2f( HWidth, HHeight ) );

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

	Batch.batchRotation( 0.0f );
	Batch.batchScale( 1.0f );
	Batch.batchCenter( Vector2f( 0, 0 ) );

	Float PlanetX = HWidth  - TNP[6]->width() * 0.5f;
	Float PlanetY = HHeight - TNP[6]->height() * 0.5f;

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

	SP.position( alpha, alpha );
	SP.draw();

	#ifndef EE_GLES
	CL1.renderMode( RN_ISOMETRIC );

	if ( CL1.getAABB().intersectCircle( Mousef, 80.f ) )
		CL1.color( ColorA(255, 0, 0, 200) );
	else
		CL1.color( ColorA(255, 255, 255, 200) );

	if ( Polygon2f::intersectQuad2( CL1.getQuad() , CL2.getQuad() ) ) {
		CL1.color( ColorA(0, 255, 0, 255) );
		CL2.color( ColorA(0, 255, 0, 255) );
	} else
		CL2.color( ColorA(255, 255, 255, 255) );

	CL1.angle(ang);
	CL1.scale(scale * 0.5f);

	CL2.position( (Float)Mousef.x - 64.f, (Float)Mousef.y + 128.f );
	CL2.angle(-ang);

	CL1.draw();
	CL2.draw();

	PR.fillMode( DRAW_LINE );
	PR.drawRectangle( CL1.getAABB() );

	PR.drawQuad( CL1.getQuad() );
	#endif

	Ang = Ang + mWindow->elapsed().asMilliseconds() * 0.1f;
	if (Ang > 360.f) Ang = 1.f;

	if ( ShowParticles )
		Particles();

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

	PR.fillMode( DRAW_LINE );
	PR.drawCircle( Vector2f( Mousef.x, Mousef.y ), 80.f, (Uint32)(Ang/3) );
	PR.drawTriangle( Triangle2f( Vector2f( Mousef.x, Mousef.y - 10.f ), Vector2f( Mousef.x - 10.f, Mousef.y + 10.f ), Vector2f( Mousef.x + 10.f, Mousef.y + 10.f ) ) );
	PR.drawLine( Line2f( Vector2f(Mousef.x - 80.f, Mousef.y - 80.f), Vector2f(Mousef.x + 80.f, Mousef.y + 80.f) ) );
	PR.drawLine( Line2f( Vector2f(Mousef.x - 80.f, Mousef.y + 80.f), Vector2f(Mousef.x + 80.f, Mousef.y - 80.f) ) );
	PR.drawLine( Line2f( Vector2f((Float)mWindow->getWidth(), 0.f), Vector2f( 0.f, (Float)mWindow->getHeight() ) ) );
	PR.fillMode( DRAW_FILL );
	PR.drawQuad( Quad2f( Vector2f(0.f, 0.f), Vector2f(0.f, 100.f), Vector2f(150.f, 150.f), Vector2f(200.f, 150.f) ), ColorA(220, 240, 0, 125), ColorA(100, 0, 240, 125), ColorA(250, 50, 25, 125), ColorA(50, 150, 150, 125) );
	PR.fillMode( DRAW_LINE );
	PR.drawRectangle( Rectf( Vector2f( Mousef.x - 80.f, Mousef.y - 80.f ), Sizef( 160.f, 160.f ) ), 45.f );
	PR.drawLine( Line2f( Vector2f(0.f, 0.f), Vector2f( (Float)mWindow->getWidth(), (Float)mWindow->getHeight() ) ) );

	TNP[3]->drawQuadEx( Quad2f( Vector2f(0.f, 0.f), Vector2f(0.f, 100.f), Vector2f(150.f, 150.f), Vector2f(200.f, 150.f) ), Vector2f(), ang, Vector2f(scale,scale), ColorA(220, 240, 0, 125), ColorA(100, 0, 240, 125), ColorA(250, 50, 25, 125), ColorA(50, 150, 150, 125) );

	WP.update( et );
	PR.setColor( ColorA(0, 255, 0, 255) );
	PR.drawPoint( WP.getPos(), 10.f );
}

void EETest::Screen3() {
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

void EETest::Screen4() {
	if ( NULL != mFBO ) {
		mFBO->bind();
		mFBO->clear();
	}

	if ( NULL != mVBO ) {
		mBlindy.position( 128-16, 128-16 );
		mBlindy.draw();

		mVBO->bind();
		mVBO->draw();
		mVBO->unbind();

		mFBOText.flags( FONT_DRAW_CENTER );
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

void EETest::Screen5() {

}

void EETest::Render() {
	HWidth = mWindow->getWidth() * 0.5f;
	HHeight = mWindow->getHeight() * 0.5f;

	if ( Sys::getTicks() - lasttick >= 50 ) {
		lasttick = Sys::getTicks();
		#ifdef EE_DEBUG
		mInfo = String::strFormated( "EE - FPS: %d Elapsed Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s\nApp Memory Usage: %s\nApp Peak Memory Usage: %s",
							mWindow->FPS(),
							et.asMilliseconds(),
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							FileSystem::sizeToString( TF->memorySize() ).c_str(),
							FileSystem::sizeToString( (Uint32)MemoryManager::getTotalMemoryUsage() ).c_str(),
							FileSystem::sizeToString( (Uint32)MemoryManager::getPeakMemoryUsage() ).c_str()
						);
		#else
		mInfo = String::strFormated( "EE - FPS: %d Elapsed Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s",
							mWindow->FPS(),
							et.AsMilliseconds(),
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							FileSystem::sizeToString( TF->MemorySize() ).c_str()
						);
		#endif

		mInfoText.text( mInfo );
	}

	if ( !MultiViewportMode ) {
		Scenes[ Screen ]();
	} else {
		Views[0].setView( 0, 0, mWindow->getWidth(), static_cast<Uint32>( HHeight ) );
		Views[1].setView( 0, static_cast<Int32> ( HHeight ), mWindow->getWidth(), static_cast<Uint32>( HHeight ) );

		mWindow->setView( Views[1] );
		Mouse = KM->getMousePosFromView( Views[1] );
		Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );
		Screen2();

		mWindow->setView( Views[0] );
		Mouse = KM->getMousePosFromView( Views[0] );
		Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );
		Screen1();

		mWindow->setView( mWindow->getDefaultView() );
		mWindow->clipEnable( (Int32)HWidth - 320, (Int32)HHeight - 240, 640, 480 );
		Screen3();
		mWindow->clipDisable();
	}

	ColorA ColRR1( 150, 150, 150, 220 );
	ColorA ColRR4( 150, 150, 150, 220 );
	ColorA ColRR2( 100, 100, 100, 220 );
	ColorA ColRR3( 100, 100, 100, 220 );

	mEEText.flags( FONT_DRAW_CENTER );

	PR.setColor( ColorA(150, 150, 150, 220) );
	PR.fillMode( DRAW_FILL );
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

	if ( InBuf.active() ) {
		Uint32 NLPos = 0;
		Uint32 LineNum = InBuf.getCurPosLinePos( NLPos );
		if ( InBuf.curPos() == (int)InBuf.buffer().size() && !LineNum ) {
			FF2->draw( "_", 6.f + FF2->getTextWidth(), 180.f );
		} else {
			FF2->setText( InBuf.buffer().substr( NLPos, InBuf.curPos() - NLPos ) );
			FF2->draw( "_", 6.f + FF2->getTextWidth(), 180.f + (Float)LineNum * (Float)FF2->getFontHeight() );
		}

		FF2->setText( "FPS: " + String::toStr( mWindow->FPS() ) );
		FF2->draw( mWindow->getWidth() - FF2->getTextWidth() - 15, 0 );

		FF2->setText( InBuf.buffer() );
		FF2->draw( 6, 180, FONT_DRAW_SHADOW );
	}

	UIManager::instance()->Draw();
	UIManager::instance()->Update();


	Con.draw();
}

void EETest::Input() {
	KM->update();
	JM->update();

	Mouse = KM->getMousePos();
	Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );

	if ( KM->isKeyUp( KEY_F1 ) )
		Graphics::ShaderProgramManager::instance()->reload();

	if ( !mWindow->visible() ) {
		mWasMinimized = true;

		mWindow->frameRateLimit( 10 );

		if ( mMusEnabled && Mus->state() == Sound::Playing )
			Mus->pause();

	} else {
		if ( mLastFPSLimit != mWindow->frameRateLimit() && !mWasMinimized )
			mLastFPSLimit = mWindow->frameRateLimit();

		if ( mWasMinimized ) {
			mWasMinimized = false;

			if ( !mWindow->isWindowed() )
				KM->grabInput( true );
		}

		mWindow->frameRateLimit( mLastFPSLimit );

		if ( mMusEnabled && Mus->state() == Sound::Paused )
			Mus->play();
	}

	if ( KM->isKeyDown( KEY_ESCAPE ) )
		mWindow->close();

	if ( KM->isKeyUp( KEY_F1 ) )
		MultiViewportMode = !MultiViewportMode;

	if ( KM->altPressed() && KM->isKeyUp( KEY_C ) )
		mWindow->center();

	if ( KM->altPressed() && KM->isKeyUp( KEY_M ) && !Con.active() ) {
		if ( !mWindow->isMaximized() )
			mWindow->maximize();
	}

	if ( KM->isKeyUp(KEY_F4) )
		TF->reloadAllTextures();

	if ( KM->altPressed() && KM->isKeyUp( KEY_RETURN ) ) {
		if ( mWindow->isWindowed() ) {
			mWindow->size( mWindow->getDesktopResolution().width(), mWindow->getDesktopResolution().height(), false );
		} else {
			mWindow->toggleFullscreen();
		}
	}

	if ( KM->grabInput() ) {
		if ( KM->altPressed() && KM->isKeyDown( KEY_TAB ) ) {
			mWindow->minimize();

			if ( KM->grabInput() )
				KM->grabInput( false );
		}
	}

	if ( KM->controlPressed() && KM->isKeyUp(KEY_G) )
		KM->grabInput(  !KM->grabInput() );

	if ( KM->isKeyUp( KEY_F3 ) || KM->isKeyUp( KEY_WORLD_26 ) || KM->isKeyUp( KEY_BACKSLASH ) ) {
		Con.toggle();
		InBuf.active( !Con.active() );
	}

	if ( KM->isKeyUp(KEY_1) && KM->controlPressed() )
		SetScreen( 0 );

	if ( KM->isKeyUp(KEY_2) && KM->controlPressed() )
		SetScreen( 1 );

	if ( KM->isKeyUp(KEY_3) && KM->controlPressed() )
		SetScreen( 2 );

	if ( KM->isKeyUp(KEY_4) && KM->controlPressed() )
		SetScreen( 3 );

	if ( KM->isKeyUp(KEY_5) && KM->controlPressed() )
		SetScreen( 4 );

	if ( KM->isKeyUp(KEY_6) && KM->controlPressed() )
		SetScreen( 5 );

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
		if ( Joy->isButtonUp(4) )		SetScreen( 0 );
		if ( Joy->isButtonUp(5) )		SetScreen( 1 );
		if ( Joy->isButtonUp(6) )		SetScreen( 2 );

		Float aX = Joy->getAxis( AXIS_X );
		Float aY = Joy->getAxis( AXIS_Y );

		if ( 0 != aX || 0 != aY ) {
			double rE = mWindow->elapsed().asMilliseconds();
			mAxisX += aX * rE;
			mAxisY += aY * rE;
		}

		if ( ( mAxisX != 0 && ( mAxisX >= 1.f || mAxisX <= -1.f ) ) || ( mAxisY != 0 && ( mAxisY >= 1.f || mAxisY <= -1.f )  ) ) {
			Float nmX = Mousef.x + mAxisX;
			Float nmY = Mousef.y + mAxisY;

			nmX = eemax<Float>( nmX, 0 );
			nmY = eemax<Float>( nmY, 0 );
			nmX = eemin( nmX, (Float)EE->getWidth() );
			nmY = eemin( nmY, (Float)EE->getHeight() );

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
				PhysicsDestroy();
				PhysicsCreate();
			}

			if ( KM->isKeyUp( KEY_1 ) )
				ChangeDemo( 0 );

			if ( KM->isKeyUp( KEY_2 ) )
				ChangeDemo( 1 );
		case 1:
			if ( NULL != Joy ) {
				Uint8 hat = Joy->getHat();

				if ( HAT_LEFT == hat || HAT_LEFTDOWN == hat || HAT_LEFTUP == hat )
					Map.Move( (mWindow->elapsed().asMilliseconds() * 0.2f), 0 );

				if ( HAT_RIGHT == hat || HAT_RIGHTDOWN == hat || HAT_RIGHTUP == hat )
					Map.Move( -mWindow->elapsed().asMilliseconds() * 0.2f, 0 );

				if ( HAT_UP == hat || HAT_LEFTUP == hat || HAT_RIGHTUP == hat )
					Map.Move( 0, (mWindow->elapsed().asMilliseconds() * 0.2f) );

				if ( HAT_DOWN == hat || HAT_LEFTDOWN == hat || HAT_RIGHTDOWN == hat )
					Map.Move( 0, -mWindow->elapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyDown(KEY_LEFT) ) {
				Map.Move( mWindow->elapsed().asMilliseconds() * 0.2f, 0 );
			}

			if ( KM->isKeyDown(KEY_RIGHT) ) {
				Map.Move( -mWindow->elapsed().asMilliseconds() * 0.2f, 0 );
			}

			if ( KM->isKeyDown(KEY_UP) ) {
				Map.Move( 0, mWindow->elapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyDown(KEY_DOWN) ) {
				Map.Move( 0, -mWindow->elapsed().asMilliseconds() * 0.2f );
			}

			if ( KM->isKeyUp(KEY_F8) )
				Map.Reset();

			break;
		case 2:
			if ( KM->isKeyUp(KEY_S) )
				SP.setRepetitions(1);

			if ( KM->isKeyUp(KEY_A) )
				SP.setRepetitions(-1);

			if ( KM->isKeyUp(KEY_D) )
				SP.reverseAnim( !SP.reverseAnim() );

			if ( KM->mouseRightPressed() )
				DrawBack = true;
			else
				DrawBack = false;

			if ( KM->isKeyUp( KEY_P ) )
				SndMng.play( "mysound" );

			if ( KM->controlPressed() && KM->isKeyUp(KEY_P) ) {
				ShowParticles = !ShowParticles;
			}

			break;
	}
}

void EETest::Update() {
	mWindow->clear();

	et = mWindow->elapsed();

	Input();

	mResLoad.update();

	if ( mFontLoader.isLoaded() ) {
		Render();
	} else {
		mFontLoader.update();
	}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	UpdateParticles();
#endif

	if ( KM->isKeyUp(KEY_F12) ) mWindow->takeScreenshot( MyPath + "screenshots/" ); //After render and before Display

	mWindow->display(false);
}

void EETest::Process() {
	Init();

	if ( NULL != mWindow && mWindow->created() ) {
		TestInstance = this;

		mWindow->runMainLoop( &MainLoop );
	}

	End();
}

void EETest::ParticlesCallback( Particle * P, ParticleSystem * Me ) {
	Float x, y, radio;
	Vector2f MePos( Me->position() );

	radio = (Math::randf(1.f, 1.2f) + sin( 20.0f / P->id() )) * 24;
	x = MePos.x + radio * cos( (Float)P->id() );
	y = MePos.y + radio * sin( (Float)P->id() );
	P->reset(x, y, Math::randf(-10.f, 10.f), Math::randf(-10.f, 10.f), Math::randf(-10.f, 10.f), Math::randf(-10.f, 10.f));
	P->Color( ColorAf(1.f, 0.6f, 0.3f, 1.f), 0.02f + Math::randf() * 0.3f );
}

void EETest::Particles() {
	PS[0].position( Mousef );

	if ( DrawBack )
		PS[1].position( Mousef );

	PS[2].position( HWidth, HHeight );
	PS[3].position(  Math::cosAng(Ang) * 220.f + HWidth + Math::randf(0.f, 10.f),  Math::sinAng(Ang) * 220.f + HHeight + Math::randf(0.f, 10.f) );
	PS[4].position( -Math::cosAng(Ang) * 220.f + HWidth + Math::randf(0.f, 10.f), -Math::sinAng(Ang) * 220.f + HHeight + Math::randf(0.f, 10.f) );

	for ( Uint32 i = 0; i < PS.size(); i++ )
		PS[i].draw();
}

#define GRABABLE_MASK_BIT (1<<31)
#define NOT_GRABABLE_MASK (~GRABABLE_MASK_BIT)

void EETest::CreateJointAndBody() {
	#ifndef EE_PLATFORM_TOUCH
	mMouseJoint	= NULL;
	mMouseBody	= eeNew( Body, ( INFINITY, INFINITY ) );
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		mMouseJoint[i] = NULL;
		mMouseBody[i] = eeNew( Body, ( INFINITY, INFINITY ) );
	}
	#endif
}

void EETest::Demo1Create() {
	CreateJointAndBody();

	Shape::ResetShapeIdCounter();

	mSpace = Physics::Space::New();
	mSpace->Gravity( cVectNew( 0, 100 ) );
	mSpace->SleepTimeThreshold( 0.5f );

	Body *body, *statiBody = mSpace->StatiBody();
	Shape * shape;

	shape = mSpace->AddShape( ShapeSegment::New( statiBody, cVectNew( 0, mWindow->getHeight() ), cVectNew( mWindow->getWidth(), mWindow->getHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( ShapeSegment::New( statiBody, cVectNew( mWindow->getWidth(), 0 ), cVectNew( mWindow->getWidth(), mWindow->getHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( ShapeSegment::New( statiBody, cVectNew( 0, 0 ), cVectNew( 0, mWindow->getHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( ShapeSegment::New( statiBody, cVectNew( 0, 0 ), cVectNew( mWindow->getWidth(), 0 ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	Float hw = mWindow->getWidth() / 2;

	for(int i=0; i<14; i++){
		for(int j=0; j<=i; j++){
			body = mSpace->AddBody( Body::New( 1.0f, Moment::ForBox( 1.0f, 30.0f, 30.0f ) ) );
			body->Pos( cVectNew( hw + j * 32 - i * 16, 100 + i * 32 ) );

			//shape = mSpace->AddShape( ShapePolySprite::New( body, 30.f, 30.f, mBoxSprite ) );
			shape = mSpace->AddShape( ShapePoly::New( body, 30.f, 30.f ) );
			shape->e( 0.0f );
			shape->u( 0.8f );
		}
	}

	cpFloat radius = 15.0f;

	body = mSpace->AddBody( Body::New( 10.0f, Moment::ForCircle( 10.0f, 0.0f, radius, cVectZero ) ) );
	body->Pos( cVectNew( hw, mWindow->getHeight() - radius - 5 ) );

	//shape = mSpace->AddShape( ShapeCircleSprite::New( body, radius, cVectZero, mCircleSprite ) );
	shape = mSpace->AddShape( ShapeCircle::New( body, radius, cVectZero ) );
	shape->e( 0.0f );
	shape->u( 0.9f );
}

void EETest::Demo1Update() {

}

void EETest::DestroyBody() {
	#ifndef EE_PLATFORM_TOUCH
	eeSAFE_DELETE( mMouseBody );
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		eeSAFE_DELETE( mMouseBody[i] );
	}
	#endif
}

void EETest::Demo1Destroy() {
	DestroyBody();

	eeSAFE_DELETE( mSpace );
}

cpBool EETest::blockerBegin( Arbiter *arb, Space *space, void *unused ) {
	Shape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->blocked++;

	return cpFalse; // Return values from sensors callbacks are ignored,
}

void EETest::blockerSeparate( Arbiter *arb, Space * space, void *unused ) {
	Shape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->blocked--;
}

void EETest::postStepRemove( Space *space, void * tshape, void * unused ) {
	Shape * shape = reinterpret_cast<Shape*>( tshape );

	#ifndef EE_PLATFORM_TOUCH
	if ( NULL != mMouseJoint && ( mMouseJoint->A() == shape->Body() || mMouseJoint->B() == shape->Body() ) ) {
		mSpace->RemoveConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( NULL != mMouseJoint[i] && ( mMouseJoint[i]->A() == shape->Body() || mMouseJoint[i]->B() == shape->Body() ) ) {
			mSpace->RemoveConstraint( mMouseJoint[i] );
			eeSAFE_DELETE( mMouseJoint[i] );
		}
	}
	#endif

	mSpace->RemoveBody( shape->Body() );
	mSpace->RemoveShape( shape );
	Shape::Free( shape, true );
}

cpBool EETest::catcherBarBegin(Arbiter *arb, Physics::Space *space, void *unused) {
	Shape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->queue++;

	mSpace->AddPostStepCallback( cb::Make3( this, &EETest::postStepRemove ), b, NULL );

	return cpFalse;
}

void EETest::Demo2Create() {
	CreateJointAndBody();

	Shape::ResetShapeIdCounter();

	mSpace = Physics::Space::New();
	mSpace->Iterations( 10 );
	mSpace->Gravity( cVectNew( 0, 100 ) );

	Body * statiBody = mSpace->StatiBody();
	Shape * shape;

	emitterInstance.queue = 5;
	emitterInstance.blocked = 0;
	emitterInstance.position = cVectNew( mWindow->getWidth() / 2 , 150);

	shape = mSpace->AddShape( ShapeCircle::New( statiBody, 15.0f, emitterInstance.position ) );
	shape->Sensor( 1 );
	shape->CollisionType( BLOCKING_SENSOR_TYPE );
	shape->Data( &emitterInstance );

	// Create our catch sensor to requeue the balls when they reach the bottom of the screen
	shape = mSpace->AddShape( ShapeSegment::New( statiBody, cVectNew(-4000, 600), cVectNew(4000, 600), 15.0f ) );
	shape->Sensor( 1 );
	shape->CollisionType( CATCH_SENSOR_TYPE );
	shape->Data( &emitterInstance );

	Space::cCollisionHandler handler;
	handler.a			= BLOCKING_SENSOR_TYPE;
	handler.b			= BALL_TYPE;
	handler.begin		= cb::Make3( this, &EETest::blockerBegin );
	handler.separate	= cb::Make3( this, &EETest::blockerSeparate );
	mSpace->AddCollisionHandler( handler );

	handler.Reset(); // Reset all the values and the callbacks ( set the callbacks as !IsSet()

	handler.a			= CATCH_SENSOR_TYPE;
	handler.b			= BALL_TYPE;
	handler.begin		= cb::Make3( this, &EETest::catcherBarBegin );
	mSpace->AddCollisionHandler( handler );
}

void EETest::Demo2Update() {
	if( !emitterInstance.blocked && emitterInstance.queue ){
		emitterInstance.queue--;

		Body * body = mSpace->AddBody( Body::New( 1.0f, Moment::ForCircle(1.0f, 15.0f, 0.0f, cVectZero ) ) );
		body->Pos( emitterInstance.position );
		body->Vel( cVectNew( Math::randf(-1,1), Math::randf(-1,1) ) * (cpFloat)100 );

		Shape *shape = mSpace->AddShape( ShapeCircle::New( body, 15.0f, cVectZero ) );
		shape->CollisionType( BALL_TYPE );
	}
}

void EETest::Demo2Destroy() {
	DestroyBody();
	eeSAFE_DELETE( mSpace );
}

void EETest::ChangeDemo( Uint32 num ) {
	if ( num < mDemo.size() ) {
		if ( eeINDEX_NOT_FOUND != mCurDemo )
			mDemo[ mCurDemo ].destroy();

		mCurDemo = num;

		mDemo[ mCurDemo ].init();
	}
}

void EETest::PhysicsCreate() {
	PhysicsManager::createSingleton();
	PhysicsManager * PM = PhysicsManager::instance();
	PhysicsManager::DrawSpaceOptions * DSO = PM->GetDrawOptions();

	DSO->DrawBBs			= false;
	DSO->DrawShapes			= true;
	DSO->CollisionPointSize	= 0;
	DSO->BodyPointSize		= 0;
	DSO->LineThickness		= 1;

	mDemo.clear();

	physicDemo demo;

	demo.init		= cb::Make0( this, &EETest::Demo1Create );
	demo.update		= cb::Make0( this, &EETest::Demo1Update );
	demo.destroy	= cb::Make0( this, &EETest::Demo1Destroy );
	mDemo.push_back( demo );

	demo.init		= cb::Make0( this, &EETest::Demo2Create );
	demo.update		= cb::Make0( this, &EETest::Demo2Update );
	demo.destroy	= cb::Make0( this, &EETest::Demo2Destroy );
	mDemo.push_back( demo );

	ChangeDemo( 0 );
}

void EETest::PhysicsUpdate() {
	#ifndef EE_PLATFORM_TOUCH
	mMousePoint = cVectNew( KM->getMousePosf().x, KM->getMousePosf().y );
	cVect newPoint = tovect( cpvlerp( tocpv( mMousePoint_last ), tocpv( mMousePoint ), 0.25 ) );
	mMouseBody->Pos( newPoint );
	mMouseBody->Vel( ( newPoint - mMousePoint_last ) * (cpFloat)mWindow->FPS() );
	mMousePoint_last = newPoint;

	if ( KM->mouseLeftPressed() ) {
		if ( NULL == mMouseJoint ) {
			cVect point = cVectNew( KM->getMousePosf().x, KM->getMousePosf().y );

			Shape * shape = mSpace->PointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

			if( NULL != shape ){
				mMouseJoint = eeNew( PivotJoint, ( mMouseBody, shape->Body(), cVectZero, shape->Body()->World2Local( point ) ) );

				mMouseJoint->MaxForce( 50000.0f );
				mSpace->AddConstraint( mMouseJoint );
			}
		}
	} else if ( NULL != mMouseJoint ) {
		mSpace->RemoveConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		InputFinger * Finger = KM->GetFingerIndex(i);
		mMousePoint[i] = cVectNew( Finger->x, Finger->y );
		cVect newPoint = tovect( cpvlerp( tocpv( mMousePoint_last[i] ), tocpv( mMousePoint[i] ), 0.25 ) );
		mMouseBody[i]->Pos( newPoint );
		mMouseBody[i]->Vel( ( newPoint - mMousePoint_last[i] ) * (cpFloat)mWindow->FPS() );
		mMousePoint_last[i] = newPoint;

		if ( Finger->IsDown() ) {
			if ( NULL == mMouseJoint[i] ) {
				cVect point = cVectNew( Finger->x, Finger->y );

				Shape * shape = mSpace->PointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

				if( NULL != shape ){
					mMouseJoint[i] = eeNew( PivotJoint, ( mMouseBody[i], shape->Body(), cVectZero, shape->Body()->World2Local( point ) ) );

					mMouseJoint[i]->MaxForce( 50000.0f );
					mSpace->AddConstraint( mMouseJoint[i] );
				}
			}
		} else if ( NULL != mMouseJoint[i] ) {
			mSpace->RemoveConstraint( mMouseJoint[i] );
			eeSAFE_DELETE( mMouseJoint[i] );
		}
	}
	#endif

	mDemo[ mCurDemo ].update();
	mSpace->Update();
	mSpace->Draw();
}

void EETest::PhysicsDestroy() {
	mDemo[ mCurDemo ].destroy();
}

void EETest::End() {
	wait();

	PhysicsDestroy();

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

	Test->Process();

	eeDelete( Test );

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
