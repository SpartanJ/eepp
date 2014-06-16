#include "eetest.hpp"

Demo_Test::cEETest * TestInstance = NULL;

static void MainLoop() {
	TestInstance->Update();
}

namespace Demo_Test {

void cEETest::Init() {
	EE = Engine::instance();

	Log::instance()->LiveWrite( true );
	Log::instance()->ConsoleOutput( true );

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

	MyPath 				= Sys::GetProcessPath() + "assets/";

	IniFile Ini( MyPath + "ee.ini" );

	PartsNum			= Ini.GetValueI( "EEPP", "ParticlesNum", 1000 );
	mUseShaders			= Ini.GetValueB( "EEPP", "UseShaders", false );
	mJoyEnabled			= Ini.GetValueB( "EEPP", "JoystickEnabled", false );

#if defined( EE_PLATFORM_TOUCH )
	mJoyEnabled = false;
#endif

	mMusEnabled			= Ini.GetValueB( "EEPP", "Music", false );
	Int32 StartScreen	= Ini.GetValueI( "EEPP", "StartScreen", 0 );

	WindowSettings WinSettings	= EE->CreateWindowSettings( &Ini );
	ContextSettings ConSettings	= EE->CreateContextSettings( &Ini );

	mWindow = EE->CreateWindow( WinSettings, ConSettings );

	if ( NULL != mWindow && mWindow->Created() ) {
		SetScreen( StartScreen );

		mWindow->Caption( "eepp - Test Application" );
		mWindow->PushResizeCallback( cb::Make1( this, &cEETest::OnWindowResize ) );

		TF = cTextureFactory::instance();
		TF->Allocate(40);

		Log		= Log::instance();
		KM		= mWindow->GetInput();
		JM		= KM->GetJoystickManager();

		PS.resize(5);

		Scenes[0] = cb::Make0( this, &cEETest::PhysicsUpdate );
		Scenes[1] = cb::Make0( this, &cEETest::Screen1 );
		Scenes[2] = cb::Make0( this, &cEETest::Screen2 );
		Scenes[3] = cb::Make0( this, &cEETest::Screen3 );
		Scenes[4] = cb::Make0( this, &cEETest::Screen4 );
		Scenes[5] = cb::Make0( this, &cEETest::Screen5 );

		//InBuf.Start();
		InBuf.SupportNewLine( true );

		SetRandomSeed( static_cast<Uint32>( Sys::GetSystemTime() * 1000 ) );

		LoadTextures();

		LoadFonts();

		CreateShaders();

		if ( mMusEnabled ) {
			Mus = eeNew( Music, () );

			if ( Mus->OpenFromFile( MyPath + "sounds/music.ogg" ) ) {
				Mus->Loop( true );
				Mus->Play();
			}
		}

		WP.Type( Ease::QuarticInOut );
		WP.AddWaypoint( Vector2f(0,0), 100 );
		WP.AddWaypoint( Vector2f(800,0), 100 );
		WP.AddWaypoint( Vector2f(0,0), 100 );
		WP.AddWaypoint( Vector2f(1024,768), 100 );
		WP.AddWaypoint( Vector2f(0,600), 100 );
		WP.EditWaypoint( 2, Vector2f(800,600), 100 );
		WP.EraseWaypoint( 3 );
		WP.Loop(true);
		WP.SetTotalTime( Milliseconds( 5000 ) );
		WP.Start();

		Batch.AllocVertexs( 2048 );
		Batch.SetBlendMode( ALPHA_BLENDONE );

		mFBO = cFrameBuffer::New( 256, 256, false );

		if ( NULL != mFBO )
			mFBO->ClearColor( ColorAf( 0, 0, 0, 0.5f ) );

		Polygon2f Poly = Polygon2f::CreateRoundedRectangle( 0, 0, 256, 50 );

		mVBO = cVertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, DM_TRIANGLE_FAN );

		if ( NULL != mVBO ) {
			for ( Uint32 i = 0; i < Poly.Size(); i++ ) {
				mVBO->AddVertex( Poly[i] );
				mVBO->AddColor( ColorA( 100 + i, 255 - i, 150 + i, 200 ) );
			}

			mVBO->Compile();
		}

		PhysicsCreate();

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		Launch();
#endif

	} else {
		Engine::DestroySingleton();

		exit(0);
	}
}

void cEETest::CreateUIThemeTextureAtlas() {
	#if !defined( EE_DEBUG ) || defined( EE_GLES )
	return;
	#endif

	std::string tgpath( MyPath + "ui/uitheme" );
	std::string Path( MyPath + "ui/uitheme" );

	if ( !FileSystem::FileExists( tgpath + EE_TEXTURE_ATLAS_EXTENSION ) ) {
		cTexturePacker tp( 256, 256, true, 2 );
		tp.AddTexturesPath( Path );
		tp.PackTextures();
		tp.Save( tgpath + ".png", SAVE_TYPE_PNG );
	} else {
		cTextureAtlasLoader tgl;
		tgl.UpdateTextureAtlas( tgpath + EE_TEXTURE_ATLAS_EXTENSION, Path );
	}
}

void cEETest::LoadFonts() {
	mFTE.Restart();

	cTextureLoader * tl = eeNew( cTextureLoader, ( MyPath + "fonts/conchars.png" ) );
	tl->SetColorKey( RGB(0,0,0) );

	mFontLoader.Add( eeNew( cTextureFontLoader, ( "conchars", tl, (unsigned int)32 ) ) );
	mFontLoader.Add( eeNew( cTextureFontLoader, ( "ProggySquareSZ", eeNew( cTextureLoader, ( MyPath + "fonts/ProggySquareSZ.png" ) ), MyPath + "fonts/ProggySquareSZ.dat" ) ) );
	mFontLoader.Add( eeNew( cTTFFontLoader, ( "arial", MyPath + "fonts/arial.ttf", 12, TTF_STYLE_NORMAL, 256, RGB(255,255,255) ) ) );
	mFontLoader.Add( eeNew( cTTFFontLoader, ( "arialb", MyPath + "fonts/arial.ttf", 12, TTF_STYLE_NORMAL, 256, RGB(255,255,255), 1, RGB(0,0,0), true ) ) );

	mFontLoader.Load( cb::Make1( this, &cEETest::OnFontLoaded ) );
}

void cEETest::OnFontLoaded( ResourceLoader * ObjLoaded ) {
	FF		= cFontManager::instance()->GetByName( "conchars" );
	FF2		= cFontManager::instance()->GetByName( "ProggySquareSZ" );
	TTF		= cFontManager::instance()->GetByName( "arial" );
	TTFB	= cFontManager::instance()->GetByName( "arialb" );

	eePRINTL( "Fonts loading time: %4.3f ms.", mFTE.Elapsed().AsMilliseconds() );

	eeASSERT( TTF != NULL );
	eeASSERT( TTFB != NULL );

	Con.Create( FF, true );
	Con.IgnoreCharOnPrompt( 186 ); // 'º'

	mBuda = String::FromUtf8( "El mono ve el pez en el agua y sufre. Piensa que su mundo es el único que existe, el mejor, el real. Sufre porque es bueno y tiene compasión, lo ve y piensa: \"Pobre se está ahogando no puede respirar\". Y lo saca, lo saca y se queda tranquilo, por fin lo salvé. Pero el pez se retuerce de dolor y muere. Por eso te mostré el sueño, es imposible meter el mar en tu cabeza, que es un balde." );

	CreateUI();

	mEEText.Create( TTFB, "Entropia Engine++\nCTRL + Number to change Demo Screen\nRight click to see the PopUp Menu" );
	mFBOText.Create( TTFB, "This is a VBO\nInside of a FBO" );
	mFBOText.Color( ColorA(255,255,0,255), mFBOText.Text().find( "VBO" ), mFBOText.Text().find( "VBO" ) + 2 );
	mFBOText.Color( ColorA(255,255,0,255), mFBOText.Text().find( "FBO" ), mFBOText.Text().find( "FBO" ) + 2 );

	mInfoText.Create( FF, "", ColorA(255,255,255,150) );
}

void cEETest::CreateShaders() {
	mUseShaders = mUseShaders && GLi->ShadersSupported();

	mShaderProgram = NULL;

	if ( mUseShaders ) {
		mBlurFactor = 0.01f;
		mShaderProgram = cShaderProgram::New( MyPath + "shaders/blur.vert", MyPath + "shaders/blur.frag" );
	}
}

void cEETest::OnWinMouseUp( const cUIEvent * Event ) {
	const cUIEventMouse * MEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	cUIControlAnim * CtrlAnim;

	if ( Event->Ctrl()->IsType( UI_TYPE_WINDOW ) ) {
		CtrlAnim = reinterpret_cast<cUIControlAnim*>( Event->Ctrl() );
	} else {
		CtrlAnim = reinterpret_cast<cUIControlAnim*>( Event->Ctrl()->Parent() );
	}

	if ( MEvent->Flags() & EE_BUTTON_WUMASK ) {
		CtrlAnim->Scale( CtrlAnim->Scale() + 0.1f );
	} else if ( MEvent->Flags() & EE_BUTTON_WDMASK ) {
		CtrlAnim->Scale( CtrlAnim->Scale() - 0.1f );
	}
}

void cEETest::OnShowMenu( const cUIEvent * Event ) {
	cUIPushButton * PB = static_cast<cUIPushButton*>( Event->Ctrl() );

	if ( Menu->Show() ) {
		Vector2i Pos = Vector2i( (Int32)PB->GetPolygon()[0].x, (Int32)PB->GetPolygon()[0].y - 2 );
		cUIMenu::FixMenuPos( Pos , Menu );
		Menu->Pos( Pos );
	}
}

void cEETest::OnWindowResize(EE::Window::Window * win) {
	Map.ViewSize( win->Size() );
}

void cEETest::CreateUI() {
	Clock TE;

	CreateUIThemeTextureAtlas();

	eePRINTL( "Texture Atlas Loading Time: %4.3f ms.", TE.Elapsed().AsMilliseconds() );

	cUIManager::instance()->Init(); //UI_MANAGER_HIGHLIGHT_FOCUS | UI_MANAGER_HIGHLIGHT_OVER

	//mTheme = cUITheme::LoadFromPath( eeNew( cUIDefaultTheme, ( "uitheme", "uitheme" ) ), MyPath + "uitheme/" );

	cTextureAtlasLoader tgl( MyPath + "ui/uitheme" + EE_TEXTURE_ATLAS_EXTENSION );

	mTheme = cUITheme::LoadFromTextureAtlas( eeNew( cUIDefaultTheme, ( "uitheme", "uitheme" ) ), cTextureAtlasManager::instance()->GetByName( "uitheme" ) );

	cUIThemeManager::instance()->Add( mTheme );
	cUIThemeManager::instance()->DefaultEffectsEnabled( true );
	cUIThemeManager::instance()->DefaultFont( TTF );
	cUIThemeManager::instance()->DefaultTheme( "uitheme" );

	cUIControl::CreateParams Params( cUIManager::instance()->MainControl(), Vector2i(0,0), Sizei( 530, 380 ), UI_FILL_BACKGROUND | UI_CLIP_ENABLE | UI_BORDER );

	Params.Border.Width( 2 );
	Params.Border.Color( 0x979797CC );
	Params.Background.Colors( ColorA( 0xEDEDED66 ), ColorA( 0xEDEDEDCC ), ColorA( 0xEDEDEDCC ), ColorA( 0xEDEDED66 ) );

	cUIWindow * tWin = mTheme->CreateWindow( NULL, Sizei( 530, 405 ), Vector2i( 320, 240 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DRAGABLE_CONTAINER , Sizei( 530, 405 ), 200 );
	C = tWin->Container();

	tWin->Title( "Controls Test" );

	tWin->AddEventListener( cUIEvent::EventMouseUp, cb::Make1( this, &cEETest::OnWinMouseUp ) );
	C->AddEventListener( cUIEvent::EventMouseUp, cb::Make1( this, &cEETest::OnWinMouseUp ) );

	Params.Flags &= ~UI_CLIP_ENABLE;
	Params.Background.Corners(0);
	Params.Background.Colors( ColorA( 0x00FF0077 ), ColorA( 0x00CC0077 ), ColorA( 0x00CC0077 ), ColorA( 0x00FF0077 ) );
	Params.Parent( C );
	Params.Size = Sizei( 50, 50 );
	cUITest * Child = eeNew( cUITest, ( Params ) );
	Child->Pos( 240, 130 );
	Child->Visible( true );
	Child->Enabled( true );
	Child->StartRotation( 0.f, 360.f, Milliseconds( 5000.f ) );
	Child->RotationInterpolation()->Loop( true );

	Params.Background.Colors( ColorA( 0xFFFF0077 ), ColorA( 0xCCCC0077 ), ColorA( 0xCCCC0077 ), ColorA( 0xFFFF0077 ) );
	Params.Parent( Child );
	Params.Size = Sizei( 25, 25 );
	cUITest * Child2 = eeNew( cUITest, ( Params ) );
	Child2->Pos( 15, 15 );
	Child2->Visible( true );
	Child2->Enabled( true );
	Child2->StartRotation( 0.f, 360.f, Milliseconds( 5000.f ) );
	Child2->RotationInterpolation()->Loop( true );

	mTheme->CreateSprite( eeNew( cSprite, ( "gn" ) ), C, Sizei(), Vector2i( 160, 100 ) );

	cUITextBox::CreateParams TextParams;
	TextParams.Parent( C );
	TextParams.PosSet( 0, 0 );
	TextParams.Size = Sizei( 320, 240 );
	TextParams.Flags = UI_VALIGN_TOP | UI_HALIGN_RIGHT;
	cUITextBox * Text = eeNew( cUITextBox, ( TextParams ) );
	Text->Visible( true );
	Text->Enabled( false );
	Text->Text( "Turn around\nJust Turn Around\nAround!" );

	cUITextInput::CreateParams InputParams;
	InputParams.Parent( C );
	InputParams.PosSet( 20, 216 );
	InputParams.Size = Sizei( 200, 22 );
	InputParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED;
	cUITextInput * Input = eeNew( cUITextInput, ( InputParams ) );
	Input->Visible( true );
	Input->Enabled( true );

	cUIPushButton::CreateParams ButtonParams;
	ButtonParams.Parent( C );
	ButtonParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE;
	ButtonParams.PosSet( 225, 216 );
	ButtonParams.Size = Sizei( 90, 0 );
	ButtonParams.SetIcon( mTheme->GetIconByName( "ok" ) );
	cUIPushButton * Button = eeNew( cUIPushButton, ( ButtonParams ) );
	Button->Visible( true );
	Button->Enabled( true );
	Button->Text( "Click Me" );
	Button->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::ButtonClick ) );
	Button->TooltipText( "Click and see what happens..." );

	TextParams.PosSet( 130, 20 );
	TextParams.Size = Sizei( 80, 22 );
	TextParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	cUICheckBox * Checkbox = eeNew( cUICheckBox, ( TextParams ) );
	Checkbox->Visible( true );
	Checkbox->Text( "Check Me" );
	Checkbox->Enabled( true );

	TextParams.PosSet( 130, 40 );
	cUIRadioButton * RadioButton = eeNew( cUIRadioButton, ( TextParams ) );
	RadioButton->Visible( true );
	RadioButton->Text( "Check Me" );
	RadioButton->Enabled( true );

	TextParams.PosSet( 130, 60 );
	RadioButton = eeNew( cUIRadioButton, ( TextParams ) );
	RadioButton->Visible( true );
	RadioButton->Text( "Check Me 2" );
	RadioButton->Enabled( true );

	cUISlider::CreateParams SliderParams;
	SliderParams.Parent( C );
	SliderParams.PosSet( 220, 80 );
	SliderParams.Size = Sizei( 80, 24 );
	mSlider = eeNew( cUISlider, ( SliderParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );
	mSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cEETest::OnSliderValueChange ) );

	SliderParams.PosSet( 40, 110 );
	SliderParams.Size = Sizei( 24, 80 );
	SliderParams.VerticalSlider = true;
	mSlider = eeNew( cUISlider, ( SliderParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );

	SliderParams.PosSet( 60, 110 );
	mSlider = eeNew( cUISlider, ( SliderParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );

	cUISpinBox::CreateParams SpinBoxParams;
	SpinBoxParams.Parent( C );
	SpinBoxParams.PosSet( 80, 150 );
	SpinBoxParams.Size = Sizei( 80, 24 );
	SpinBoxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_CLIP_ENABLE;
	SpinBoxParams.AllowDotsInNumbers = true;
	cUISpinBox * mSpinBox = eeNew( cUISpinBox, ( SpinBoxParams ) );
	mSpinBox->Visible( true );
	mSpinBox->Enabled( true );

	cUIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent( C );
	ScrollBarP.PosSet( 0, 0 );
	ScrollBarP.Size = Sizei( 15, 240 );
	ScrollBarP.Flags = UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar = true;
	mScrollBar = eeNew( cUIScrollBar, ( ScrollBarP ) );
	mScrollBar->Visible( true );
	mScrollBar->Enabled( true );
	mScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cEETest::OnValueChange ) );

	mProgressBar = mTheme->CreateProgressBar( C, Sizei( 200, 20 ), Vector2i( 20, 190 ) );

	TextParams.PosSet( 20, 5 );
	mTextBoxValue = eeNew( cUITextBox, ( TextParams ) );
	mTextBoxValue->Visible( true );
	OnValueChange( NULL );

	cUIListBox::CreateParams LBParams;
	LBParams.Parent( C );
	LBParams.PosSet( 325, 8 );
	LBParams.Size = Sizei( 200, 240-16 );
	LBParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TOUCH_DRAG_ENABLED; // | UI_MULTI_SELECT
	mListBox = eeNew( cUIListBox, ( LBParams ) );
	mListBox->Visible( true );
	mListBox->Enabled( true );

	Int32 wsize = 100;

	if ( wsize ) {
		std::vector<String> str(wsize);

		for ( Int32 i = 1; i <= wsize; i++ )
			str[i-1] = "Test ListBox " + String::ToStr(i) + " testing it right now!";

		mListBox->AddListBoxItems( str );
	}

	cUIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( C );
	DDLParams.PosSet( 20, 55 );
	DDLParams.Size = Sizei( 100, 21 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_TOUCH_DRAG_ENABLED;
	cUIDropDownList * mDropDownList = eeNew( cUIDropDownList, ( DDLParams ) );
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

	cUIComboBox::CreateParams ComboParams;
	ComboParams.Parent( C );
	ComboParams.PosSet( 20, 80 );
	ComboParams.Size = Sizei( 100, 1 );
	ComboParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_AUTO_SIZE | UI_TOUCH_DRAG_ENABLED | UI_TEXT_SELECTION_ENABLED;
	cUIComboBox * mComboBox = eeNew( cUIComboBox, ( ComboParams ) );
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

	cUIPopUpMenu * Menu3 = mTheme->CreatePopUpMenu();
	Menu3->Add( "Hello World 1" );
	Menu3->Add( "Hello World 2" );
	Menu3->Add( "Hello World 3" );
	Menu3->Add( "Hello World 4" );

	cUIPopUpMenu * Menu2 = mTheme->CreatePopUpMenu();
	Menu2->Add( "Test 1" );
	Menu2->Add( "Test 2" );
	Menu2->Add( "Test 3" );
	Menu2->Add( "Test 4" );
	Menu2->AddSubMenu( "Hello World", NULL, Menu3 );

	Menu->AddSeparator();
	Menu->AddSubMenu( "Sub-Menu", NULL, Menu2 ) ;

	Menu->AddSeparator();
	Menu->Add( "Quit" );

	Menu->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cEETest::ItemClick ) );
	Menu->GetItem( "Quit" )->AddEventListener( cUIEvent::EventMouseUp, cb::Make1( this, &cEETest::QuitClick ) );
	cUIManager::instance()->MainControl()->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::MainClick ) );

	cUITextEdit::CreateParams TEParams;
	TEParams.Parent( C );
	TEParams.PosSet( 5, 245 );
	TEParams.Size	= Sizei( 315, 130 );
	TEParams.Flags = UI_AUTO_PADDING | UI_CLIP_ENABLE | UI_TEXT_SELECTION_ENABLED;
	cUITextEdit * TextEdit = eeNew( cUITextEdit, ( TEParams ) );
	TextEdit->Visible( true );
	TextEdit->Enabled( true );
	TextEdit->Text( mBuda );

	cUIGenericGrid::CreateParams GridParams;
	GridParams.Parent( C );
	GridParams.PosSet( 325, 245 );
	GridParams.SizeSet( 200, 130 );
	GridParams.Flags = UI_AUTO_PADDING | UI_TOUCH_DRAG_ENABLED;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 3;
	cUIGenericGrid * mGenGrid = eeNew( cUIGenericGrid, ( GridParams ) );
	mGenGrid->Visible( true );
	mGenGrid->Enabled( true );

	cUIGridCell::CreateParams CellParams;
	CellParams.Parent( mGenGrid->Container() );

	cUITextBox::CreateParams TxtBoxParams;
	cUITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_TEXT_SELECTION_ENABLED;

	cUIGfx::CreateParams TxtGfxParams;
	TxtGfxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	TxtGfxParams.SubTexture = mTheme->GetIconByName( "ok" );

	for ( Uint32 i = 0; i < 100; i++ ) {
		cUIGridCell * Cell			= eeNew( cUIGridCell, ( CellParams ) );
		cUITextBox * TxtBox			= eeNew( cUITextBox, ( TxtBoxParams ) );
		cUITextInput * TxtInput		= eeNew( cUITextInput, ( TxtInputParams ) );
		cUIGfx * TxtGfx				= eeNew( cUIGfx, ( TxtGfxParams )  );

		TxtBox->Text( "Test " + String::ToStr( i+1 ) );

		Cell->Cell( 0, TxtBox );
		Cell->Cell( 1, TxtGfx );
		Cell->Cell( 2, TxtInput );

		mGenGrid->Add( Cell );
	}

	mGenGrid->CollumnWidth( 0, 50 );
	mGenGrid->CollumnWidth( 1, 24 );
	mGenGrid->CollumnWidth( 2, 100 );

	C = reinterpret_cast<cUIControlAnim*> ( C->Parent() );

	eePRINTL( "CreateUI time: %4.3f ms.", TE.Elapsed().AsMilliseconds() );
}

void cEETest::CreateMapEditor() {
	if ( NULL != mMapEditor )
		return;

	cUIWindow * tWin = mTheme->CreateWindow( NULL, Sizei( 1024, 768 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, Sizei( 1024, 768 ) );
	mMapEditor = eeNew( cMapEditor, ( tWin, cb::Make0( this, &cEETest::OnMapEditorClose ) ) );
	tWin->Center();
	tWin->Show();
}

void cEETest::OnMapEditorClose() {
	mMapEditor = NULL;
}

void cEETest::CreateETGEditor() {
	cUIWindow * tWin = mTheme->CreateWindow( NULL, Sizei( 1024, 768 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, Sizei( 1024, 768 ) );
	mETGEditor = eeNew ( Tools::cTextureAtlasEditor, ( tWin, cb::Make0( this, &cEETest::OnETGEditorClose ) ) );
	tWin->Center();
	tWin->Show();
}

void cEETest::OnETGEditorClose() {
	mETGEditor = NULL;
}

void cEETest::CreateCommonDialog() {
	cUICommonDialog * CDialog = mTheme->CreateCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON );
	CDialog->AddFilePattern( "*.hpp;*.cpp", true );
	CDialog->Center();
	CDialog->Show();
}

void cEETest::CreateWinMenu() {
	cUIWinMenu * WinMenu = mTheme->CreateWinMenu( mUIWindow->Container() );

	cUIPopUpMenu * PopMenu = mTheme->CreatePopUpMenu();
	PopMenu->Add( "File" );
	PopMenu->Add( "Open" );
	PopMenu->Add( "Close" );
	PopMenu->Add( "Quit" );

	cUIPopUpMenu * PopMenu2 = mTheme->CreatePopUpMenu();
	PopMenu2->Add( "Bla" );
	PopMenu2->Add( "Bla 2" );
	PopMenu2->Add( "Bla 3" );
	PopMenu2->Add( "Bla 4" );

	WinMenu->AddMenuButton( "File", PopMenu );
	WinMenu->AddMenuButton( "Edit", PopMenu2 );
}

void cEETest::CreateDecoratedWindow() {
	mUIWindow = mTheme->CreateWindow( NULL, Sizei( 530, 350 ), Vector2i( 200, 50 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON, Sizei( 100, 200 ) );

	mUIWindow->AddEventListener( cUIEvent::EventOnWindowCloseClick, cb::Make1( this, &cEETest::CloseClick ) );
	mUIWindow->Title( "Test Window" );
	mUIWindow->ToBack();

	cUIPushButton * Button = mTheme->CreatePushButton( mUIWindow->Container(), Sizei( 510, 22 ), Vector2i( 10, 28 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_ANCHOR_RIGHT );
	Button->Text( "Click Me" );
	Button->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::ButtonClick ) );

	mUIWindow->AddShortcut( KEY_C, KEYMOD_ALT, Button );

	cUITabWidget * TabWidget = mTheme->CreateTabWidget( mUIWindow->Container(), Sizei( 510, 250 ), Vector2i( 10, 55 ), UI_HALIGN_CENTER | UI_VALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT | UI_ANCHOR_TOP );

	cUITextEdit * TEdit = mTheme->CreateTextEdit( TabWidget, Sizei(), Vector2i() );
	TEdit->Text( mBuda );
	TabWidget->Add( "TextEdit", TEdit );

	cUITextInput * Txt = mTheme->CreateTextInput( TabWidget, Sizei(), Vector2i(), UI_AUTO_PADDING | UI_AUTO_SHRINK_TEXT | UI_TEXT_SELECTION_ENABLED );
	Txt->Text( mBuda );
	TabWidget->Add( "TextInput", Txt );

	TabWidget->Add( "TextBox", mTheme->CreateTextBox( mBuda, TabWidget, Sizei(), Vector2i(), UI_AUTO_PADDING | UI_AUTO_SHRINK_TEXT | UI_TEXT_SELECTION_ENABLED ) );

	CreateWinMenu();
}

void cEETest::CloseClick( const cUIEvent * Event ) {
	mUIWindow = NULL;
}

void cEETest::ItemClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

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
		Con.Toggle();
		InBuf.Active( !Con.Active() );

		if ( Con.Active() ) {
			mWindow->StartTextInput();
		} else {
			mWindow->StopTextInput();
		}
	} else if ( "Show Window" == txt ) {
		cUIMenuCheckBox * Chk = reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() );

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

void cEETest::OnValueChange( const cUIEvent * Event ) {
	mTextBoxValue->Text( "Scroll Value:\n" + String::ToStr( mScrollBar->Value() ) );

	mProgressBar->Progress( mScrollBar->Value() * 100.f );
}

void cEETest::OnSliderValueChange( const cUIEvent * Event ) {
	cUISlider * slider = static_cast<cUISlider*>( Event->Ctrl() );

	C->Angle( slider->Value() * 90.f );
}

void cEETest::QuitClick( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		mWindow->Close();
	}
}

void cEETest::ShowMenu() {
	if ( Menu->Show() ) {
		Vector2i Pos = mWindow->GetInput()->GetMousePos();
		cUIMenu::FixMenuPos( Pos , Menu );
		Menu->Pos( Pos );
	}
}

void cEETest::MainClick( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_RMASK ) {
		ShowMenu();
	}
}

void cEETest::ButtonClick( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTONS_LRM ) {
		cUIGfx::CreateParams GfxParams;
		GfxParams.Parent( cUIManager::instance()->MainControl() );
		GfxParams.SubTexture = mTheme->GetIconByName( "ok" );
		cUIGfx * Gfx = eeNew( cUIGfx, ( GfxParams ) );
		Gfx->Visible( true );
		Gfx->Enabled( false );

		Gfx->StartRotation( 0, 2500, Milliseconds( 2500 ) );
		Gfx->StartMovement( Vector2i( Math::Randi( 0, mWindow->GetWidth() ), -64 ), Vector2i( Math::Randi( 0, mWindow->GetWidth() ), mWindow->GetHeight() + 64 ), Milliseconds( 2500 ) );
		Gfx->CloseFadeOut( Milliseconds( 3500 ) );

		mListBox->AddListBoxItem( "Test ListBox " + String::ToStr( mListBox->Count() + 1 ) + " testing it right now!" );
	}
}

void cEETest::SetScreen( Uint32 num ) {
	if ( NULL != mTerrainBut ) mTerrainBut->Visible( 1 == num );

	if ( 0 == num || 5 == num )
		mWindow->BackColor( RGB( 240, 240, 240 ) );
	else
		mWindow->BackColor( RGB( 0, 0, 0 ) );

	if ( num < 6 )
		Screen = num;
}

void cEETest::CmdSetPartsNum ( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt >= 0 && tInt <= 100000 ) ) {
			PS[2].Create( PSE_WormHole, tInt, TN[5], Vector2f( mWindow->GetWidth() * 0.5f, mWindow->GetHeight() * 0.5f ), 32, true );
			Con.PushText( "Wormhole Particles Number Changed to: " + String::ToStr(tInt) );
		} else
			Con.PushText( "Valid parameters are between 0 and 100000 (0 = no limit)." );
	}
}

void cEETest::OnTextureLoaded( ResourceLoader * ResLoaded ) {
	SndMng.Play( "mysound" );
}

void cEETest::LoadTextures() {
	Clock TE;

	Uint32 i;

	PakTest = eeNew( Zip, () );

	#ifndef EE_GLES

	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX
	Engine::instance()->EnableSharedGLContext();
	#endif

	PakTest->Open( MyPath + "test.zip" );

	std::vector<std::string> files = PakTest->GetFileList();

	for ( i = 0; i < files.size(); i++ ) {
		std::string name( files[i] );

		if ( "jpg" == FileSystem::FileExtension( name ) ) {
			mResLoad.Add( eeNew( cTextureLoader, ( PakTest, name ) ) );
		}
	}
	#endif

	mResLoad.Add( eeNew( SoundLoader, ( &SndMng, "mysound", MyPath + "sounds/sound.ogg" ) ) );

	mResLoad.Load( cb::Make1( this, &cEETest::OnTextureLoaded ) );

	TN.resize(12);
	TNP.resize(12);

	for ( i = 0; i <= 6; i++ ) {
		TN[i] = TF->Load( MyPath + "sprites/" + String::ToStr(i+1) + ".png", (i+1) == 7 ? true : false, ( (i+1) == 4 ) ? CLAMP_REPEAT : CLAMP_TO_EDGE );
		TNP[i] = TF->GetTexture( TN[i] );
	}

	Tiles.resize(10);

	cTextureAtlasLoader tgl( MyPath + "atlases/tiles.eta" );
	cTextureAtlas * SG = cTextureAtlasManager::instance()->GetByName( "tiles" );

	if ( NULL != SG ) {
		for ( i = 0; i < 6; i++ ) {
			Tiles[i] = SG->GetByName( String::ToStr( i+1 ) );
		}

		Tiles[6] = SG->Add( TF->Load( MyPath + "sprites/objects/1.png" ), "7" );

		#ifdef EE_GLES
		cImage tImg( MyPath + "sprites/objects/2.png", 4 );
		tImg.CreateMaskFromColor( ColorA(0,0,0,255), 0 );
		Tiles[7] = SG->Add( TF->LoadFromPixels( tImg.GetPixelsPtr(), tImg.Width(), tImg.Height(), tImg.Channels() ), "8" );
		#else
		Tiles[7] = SG->Add( TF->Load( MyPath + "sprites/objects/2.png" ), "8" );
		Tiles[7]->GetTexture()->CreateMaskFromColor( ColorA(0,0,0,255), 0 );
		#endif
	}

	int w, h;

	for ( Int32 my = 0; my < 4; my++ )
		for( Int32 mx = 0; mx < 8; mx++ )
			SP.AddFrame( TN[4], Sizef( 0, 0 ), Vector2i( 0, 0 ), Recti( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );

	PS[0].SetCallbackReset( cb::Make2( this, &cEETest::ParticlesCallback ) );
	PS[0].Create( PSE_Callback, 500, TN[5], Vector2f( 0, 0 ), 16, true );
	PS[1].Create( PSE_Heal, 250, TN[5], Vector2f( mWindow->GetWidth() * 0.5f, mWindow->GetHeight() * 0.5f ), 16, true );
	PS[2].Create( PSE_WormHole, PartsNum, TN[5], Vector2f( mWindow->GetWidth() * 0.5f, mWindow->GetHeight() * 0.5f ), 32, true );
	PS[3].Create( PSE_Fire, 350, TN[5], Vector2f( -50.f, -50.f ), 32, true );
	PS[4].Create( PSE_Fire, 350, TN[5], Vector2f( -50.f, -50.f ), 32, true );

	Con.AddCommand( "setparticlesnum", cb::Make1( this, &cEETest::CmdSetPartsNum ) );

	cTexture * Tex = TNP[2];

	if ( NULL != Tex && Tex->Lock() ) {
		w = (int)Tex->Width();
		h = (int)Tex->Height();

		for ( y = 0; y < h; y++) {
			for ( x = 0; x < w; x++) {
				ColorA C = Tex->GetPixel(x, y);

				if ( C.R() > 200 && C.G() > 200 && C.B() > 200 )
					Tex->SetPixel(x, y, ColorA( Math::Randi(0, 255), Math::Randi(0, 255), Math::Randi(0, 255), C.A() ) );
				else
					Tex->SetPixel(x, y, ColorA( Math::Randi(200, 255), Math::Randi(200, 255), Math::Randi(200, 255), C.A() ) );
			}
		}

		Tex->Unlock(false, true);
	}

	Cursor[0] = TF->Load( MyPath + "cursors/cursor.tga" );
	CursorP[0] = TF->GetTexture( Cursor[0] );

	CursorManager * CurMan = mWindow->GetCursorManager();
	CurMan->Visible( false );
	CurMan->Visible( true );
	CurMan->Set( Window::SYS_CURSOR_HAND );
	CurMan->SetGlobalCursor( EE_CURSOR_ARROW, CurMan->Add( CurMan->Create( CursorP[0], Vector2i( 1, 1 ), "cursor_special" ) ) );
	CurMan->Set( EE_CURSOR_ARROW );

	CL1.AddFrame( TN[2] );
	CL1.Position( 500, 400 );
	CL1.Scale( 0.5f );

	CL2.AddFrame(TN[0], Sizef(96, 96) );
	CL2.Color( ColorA( 255, 255, 255, 255 ) );

	mTGL = eeNew( cTextureAtlasLoader, ( MyPath + "atlases/bnb" + EE_TEXTURE_ATLAS_EXTENSION ) );

	mBlindy.AddFramesByPattern( "rn" );
	mBlindy.Position( 320.f, 0.f );

	mBoxSprite = eeNew( cSprite, ( cGlobalTextureAtlas::instance()->Add( eeNew( cSubTexture, ( TN[3], "ilmare" ) ) ) ) );
	mCircleSprite = eeNew( cSprite, ( cGlobalTextureAtlas::instance()->Add( eeNew( cSubTexture, ( TN[1], "thecircle" ) ) ) ) );

	eePRINTL( "Textures loading time: %4.3f ms.", TE.Elapsed().AsMilliseconds() );

	Map.Load( MyPath + "maps/test.eem" );
	Map.DrawGrid( false );
	Map.ClipedArea( false );
	Map.DrawBackground( false );
	Map.ViewSize( mWindow->Size() );

	eePRINTL( "Map creation time: %4.3f ms.", TE.Elapsed().AsMilliseconds() );
}

void cEETest::Run() {
	ParticlesThread();
}

void cEETest::ParticlesThread() {
	while ( mWindow->Running() ) {
		UpdateParticles();
		Sys::Sleep(10);
	}
}

void cEETest::UpdateParticles() {
	if ( MultiViewportMode || Screen == 2 ) {
		PSElapsed = cElapsed.Elapsed();

		for ( Uint8 i = 0; i < PS.size(); i++ )
			PS[i].Update( PSElapsed );
	}
}

void cEETest::Screen1() {
	Map.Update();
	Map.Draw();
}

void cEETest::Screen2() {
	if ( mResLoad.IsLoaded() ) {
		cTexture * TexLoaded = TF->GetByName( "1.jpg" );

		if ( NULL != TexLoaded )
			TexLoaded->Draw( 0, 0 );
	}

	if ( KM->MouseLeftPressed() )
		TNP[3]->DrawEx( 0.f, 0.f, (Float)mWindow->GetWidth(), (Float)mWindow->GetHeight() );

	Batch.SetTexture( TNP[2] );
	Batch.QuadsBegin();
	Batch.QuadsSetColor( ColorA(150,150,150,100) );
	Batch.QuadsSetSubset( 0.0f, 0.0f, 0.5f, 0.5f );

	Batch.BatchRotation( ang );
	Batch.BatchScale( scale );
	Batch.BatchCenter( Vector2f( HWidth, HHeight ) );

	Float aX = HWidth - 256.f;
	Float aY = HHeight - 256.f;
	Quad2f TmpQuad(
		Vector2f( aX	   , aY 		),
		Vector2f( aX	   , aY + 32.f  ),
		Vector2f( aX + 32.f, aY + 32.f  ),
		Vector2f( aX + 32.f, aY 		)
	);
	TmpQuad.Rotate( ang, Vector2f( aX + 16.f, aY + 16.f ) );

	for ( Uint32 z = 0; z < 16; z++ ) {
		for ( Uint32 y = 0; y < 16; y++ ) {
			Float tmpx = (Float)z * 32.f;
			Float tmpy = (Float)y * 32.f;

			Batch.BatchQuadFree( TmpQuad[0].x + tmpx, TmpQuad[0].y + tmpy, TmpQuad[1].x + tmpx, TmpQuad[1].y + tmpy, TmpQuad[2].x + tmpx, TmpQuad[2].y + tmpy, TmpQuad[3].x + tmpx, TmpQuad[3].y + tmpy );
		}
	}

	Batch.Draw();

	Batch.BatchRotation( 0.0f );
	Batch.BatchScale( 1.0f );
	Batch.BatchCenter( Vector2f( 0, 0 ) );

	Float PlanetX = HWidth  - TNP[6]->Width() * 0.5f;
	Float PlanetY = HHeight - TNP[6]->Height() * 0.5f;

	ang+=et.AsMilliseconds() * 0.1f;
	ang = (ang>=360) ? 0 : ang;

	if (scale>=1.5f) {
		scale = 1.5f;
		side = true;
	} else if (scale<=0.5f) {
		side = false;
		scale = 0.5f;
	}
	scale = (!side) ? scale+et.AsMilliseconds() * 0.00025f : scale-et.AsMilliseconds() * 0.00025f;

	if ( mUseShaders ) {
		mBlurFactor = ( 1.5f * 0.01f ) - ( scale * 0.01f );
		mShaderProgram->Bind();
		mShaderProgram->SetUniform( "blurfactor" , (float)mBlurFactor );
	}

	TNP[6]->DrawFast( PlanetX, PlanetY, ang, Vector2f(scale,scale));

	if ( mUseShaders )
		mShaderProgram->Unbind();

	TNP[3]->Draw( HWidth - 128, HHeight, 0, Vector2f::One, ColorA(255,255,255,150), ALPHA_NORMAL, RN_ISOMETRIC);
	TNP[3]->Draw( HWidth - 128, HHeight - 128, 0, Vector2f::One, ColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRIC);
	TNP[3]->Draw( HWidth - 128, HHeight, 0, Vector2f::One, ColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRICVERTICAL);
	TNP[3]->Draw( HWidth, HHeight, 0, Vector2f::One, ColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRICVERTICALNEGATIVE);

	alpha = (!aside) ? alpha+et.AsMilliseconds() * 0.1f : alpha-et.AsMilliseconds() * 0.1f;
	if (alpha>=255) {
		aside = true;
		alpha = 255;
	} else if (alpha<=0) {
		alpha = 0;
		aside = false;
	}

	ColorA Col(255,255,255,(int)alpha);
	TNP[1]->DrawEx( (Float)mWindow->GetWidth() - 128.f, (Float)mWindow->GetHeight() - 128.f, 128.f, 128.f, ang, Vector2f::One, Col, Col, Col, Col, ALPHA_BLENDONE, RN_FLIPMIRROR);

	SP.Position( alpha, alpha );
	SP.Draw();

	#ifndef EE_GLES
	CL1.RenderMode( RN_ISOMETRIC );

	if ( CL1.GetAABB().IntersectCircle( Mousef, 80.f ) )
		CL1.Color( ColorA(255, 0, 0, 200) );
	else
		CL1.Color( ColorA(255, 255, 255, 200) );

	if ( Polygon2f::IntersectQuad2( CL1.GetQuad() , CL2.GetQuad() ) ) {
		CL1.Color( ColorA(0, 255, 0, 255) );
		CL2.Color( ColorA(0, 255, 0, 255) );
	} else
		CL2.Color( ColorA(255, 255, 255, 255) );

	CL1.Angle(ang);
	CL1.Scale(scale * 0.5f);

	CL2.Position( (Float)Mousef.x - 64.f, (Float)Mousef.y + 128.f );
	CL2.Angle(-ang);

	CL1.Draw();
	CL2.Draw();

	PR.FillMode( DRAW_LINE );
	PR.DrawRectangle( CL1.GetAABB() );

	PR.DrawQuad( CL1.GetQuad() );
	#endif

	Ang = Ang + mWindow->Elapsed().AsMilliseconds() * 0.1f;
	if (Ang > 360.f) Ang = 1.f;

	if ( ShowParticles )
		Particles();

	PR.SetColor( ColorA(0, 255, 0, 50) );

	Line2f Line( Vector2f(0.f, 0.f), Vector2f( (Float)mWindow->GetWidth(), (Float)mWindow->GetHeight() ) );
	Line2f Line2( Vector2f(Mousef.x - 80.f, Mousef.y - 80.f), Vector2f(Mousef.x + 80.f, Mousef.y + 80.f) );
	Line2f Line3( Vector2f((Float)mWindow->GetWidth(), 0.f), Vector2f( 0.f, (Float)mWindow->GetHeight() ) );
	Line2f Line4( Vector2f(Mousef.x - 80.f, Mousef.y + 80.f), Vector2f(Mousef.x + 80.f, Mousef.y - 80.f) );

	if ( Line.Intersect( Line2 ) )
		iL1 = true;
	else
		iL1 = false;

	if ( Line3.Intersect( Line4 ) )
		iL2 = true;
	else
		iL2 = false;

	if (iL1 && iL2)
		PR.SetColor( ColorA(255, 0, 0, 255) );
	else if (iL1)
		PR.SetColor( ColorA(0, 0, 255, 255) );
	else if (iL2)
		PR.SetColor( ColorA(255, 255, 0, 255) );

	PR.FillMode( DRAW_LINE );
	PR.DrawCircle( Vector2f( Mousef.x, Mousef.y ), 80.f, (Uint32)(Ang/3) );
	PR.DrawTriangle( Triangle2f( Vector2f( Mousef.x, Mousef.y - 10.f ), Vector2f( Mousef.x - 10.f, Mousef.y + 10.f ), Vector2f( Mousef.x + 10.f, Mousef.y + 10.f ) ) );
	PR.DrawLine( Line2f( Vector2f(Mousef.x - 80.f, Mousef.y - 80.f), Vector2f(Mousef.x + 80.f, Mousef.y + 80.f) ) );
	PR.DrawLine( Line2f( Vector2f(Mousef.x - 80.f, Mousef.y + 80.f), Vector2f(Mousef.x + 80.f, Mousef.y - 80.f) ) );
	PR.DrawLine( Line2f( Vector2f((Float)mWindow->GetWidth(), 0.f), Vector2f( 0.f, (Float)mWindow->GetHeight() ) ) );
	PR.FillMode( DRAW_FILL );
	PR.DrawQuad( Quad2f( Vector2f(0.f, 0.f), Vector2f(0.f, 100.f), Vector2f(150.f, 150.f), Vector2f(200.f, 150.f) ), ColorA(220, 240, 0, 125), ColorA(100, 0, 240, 125), ColorA(250, 50, 25, 125), ColorA(50, 150, 150, 125) );
	PR.FillMode( DRAW_LINE );
	PR.DrawRectangle( Rectf( Vector2f( Mousef.x - 80.f, Mousef.y - 80.f ), Sizef( 160.f, 160.f ) ), 45.f );
	PR.DrawLine( Line2f( Vector2f(0.f, 0.f), Vector2f( (Float)mWindow->GetWidth(), (Float)mWindow->GetHeight() ) ) );

	TNP[3]->DrawQuadEx( Quad2f( Vector2f(0.f, 0.f), Vector2f(0.f, 100.f), Vector2f(150.f, 150.f), Vector2f(200.f, 150.f) ), Vector2f(), ang, Vector2f(scale,scale), ColorA(220, 240, 0, 125), ColorA(100, 0, 240, 125), ColorA(250, 50, 25, 125), ColorA(50, 150, 150, 125) );

	WP.Update( et );
	PR.SetColor( ColorA(0, 255, 0, 255) );
	PR.DrawPoint( WP.GetPos(), 10.f );
}

void cEETest::Screen3() {
	if (AnimVal>=300.0f) {
		AnimVal = 300.0f;
		AnimSide = true;
	} else if (AnimVal<=0.5f) {
		AnimVal = 0.5f;
		AnimSide = false;
	}
	AnimVal = (!AnimSide) ? AnimVal+et.AsMilliseconds() * 0.1f : AnimVal-et.AsMilliseconds() * 0.1f;

	Batch.SetTexture( TNP[3] );
	Batch.LineLoopBegin();
	for ( Float j = 0; j < 360; j++ ) {
		Batch.BatchLineLoop( HWidth + 350 * Math::sinAng(j), HHeight + 350 * Math::cosAng(j), HWidth + AnimVal * Math::sinAng(j+1), HHeight + AnimVal * Math::cosAng(j+1) );
	}
	Batch.Draw();
}

void cEETest::Screen4() {
	if ( NULL != mFBO ) {
		mFBO->Bind();
		mFBO->Clear();
	}

	if ( NULL != mVBO ) {
		mBlindy.Position( 128-16, 128-16 );
		mBlindy.Draw();

		mVBO->Bind();
		mVBO->Draw();
		mVBO->Unbind();

		mFBOText.Flags( FONT_DRAW_CENTER );
		mFBOText.Draw( 128.f - (Float)(Int32)( mFBOText.GetTextWidth() * 0.5f ), 25.f - (Float)(Int32)( mFBOText.GetTextHeight() * 0.5f ) );
	}

	if ( NULL != mFBO ) {
		mFBO->Unbind();

		if ( NULL != mFBO->GetTexture() ) {
			mFBO->GetTexture()->Draw( (Float)mWindow->GetWidth() * 0.5f - (Float)mFBO->GetWidth() * 0.5f, (Float)mWindow->GetHeight() * 0.5f - (Float)mFBO->GetHeight() * 0.5f, Ang );
			cGlobalBatchRenderer::instance()->Draw();
		}
	}
}

void cEETest::Screen5() {

}

void cEETest::Render() {
	HWidth = mWindow->GetWidth() * 0.5f;
	HHeight = mWindow->GetHeight() * 0.5f;

	if ( Sys::GetTicks() - lasttick >= 50 ) {
		lasttick = Sys::GetTicks();
		#ifdef EE_DEBUG
		mInfo = String::StrFormated( "EE - FPS: %d Elapsed Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s\nApp Memory Usage: %s\nApp Peak Memory Usage: %s",
							mWindow->FPS(),
							et.AsMilliseconds(),
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							FileSystem::SizeToString( TF->MemorySize() ).c_str(),
							FileSystem::SizeToString( (Uint32)MemoryManager::GetTotalMemoryUsage() ).c_str(),
							FileSystem::SizeToString( (Uint32)MemoryManager::GetPeakMemoryUsage() ).c_str()
						);
		#else
		mInfo = String::StrFormated( "EE - FPS: %d Elapsed Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s",
							mWindow->FPS(),
							et.AsMilliseconds(),
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							FileSystem::SizeToString( TF->MemorySize() ).c_str()
						);
		#endif

		mInfoText.Text( mInfo );
	}

	if ( !MultiViewportMode ) {
		Scenes[ Screen ]();
	} else {
		Views[0].SetView( 0, 0, mWindow->GetWidth(), static_cast<Uint32>( HHeight ) );
		Views[1].SetView( 0, static_cast<Int32> ( HHeight ), mWindow->GetWidth(), static_cast<Uint32>( HHeight ) );

		mWindow->SetView( Views[1] );
		Mouse = KM->GetMousePosFromView( Views[1] );
		Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );
		Screen2();

		mWindow->SetView( Views[0] );
		Mouse = KM->GetMousePosFromView( Views[0] );
		Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );
		Screen1();

		mWindow->SetView( mWindow->GetDefaultView() );
		mWindow->ClipEnable( (Int32)HWidth - 320, (Int32)HHeight - 240, 640, 480 );
		Screen3();
		mWindow->ClipDisable();
	}

	ColorA ColRR1( 150, 150, 150, 220 );
	ColorA ColRR4( 150, 150, 150, 220 );
	ColorA ColRR2( 100, 100, 100, 220 );
	ColorA ColRR3( 100, 100, 100, 220 );

	mEEText.Flags( FONT_DRAW_CENTER );

	PR.SetColor( ColorA(150, 150, 150, 220) );
	PR.FillMode( DRAW_FILL );
	PR.DrawRectangle(
				Rectf(
					Vector2f(
						0.f,
						(Float)mWindow->GetHeight() - mEEText.GetTextHeight()
					),
					Vector2f(
						mEEText.GetTextWidth(),
						mEEText.GetTextHeight()
					)
				),
				ColRR1, ColRR2, ColRR3, ColRR4
	);

	mEEText.Draw( 0.f, (Float)mWindow->GetHeight() - mEEText.GetTextHeight() );

	mInfoText.Draw( 6.f, 6.f );

	if ( InBuf.Active() ) {
		Uint32 NLPos = 0;
		Uint32 LineNum = InBuf.GetCurPosLinePos( NLPos );
		if ( InBuf.CurPos() == (int)InBuf.Buffer().size() && !LineNum ) {
			FF2->Draw( "_", 6.f + FF2->GetTextWidth(), 180.f );
		} else {
			FF2->SetText( InBuf.Buffer().substr( NLPos, InBuf.CurPos() - NLPos ) );
			FF2->Draw( "_", 6.f + FF2->GetTextWidth(), 180.f + (Float)LineNum * (Float)FF2->GetFontHeight() );
		}

		FF2->SetText( "FPS: " + String::ToStr( mWindow->FPS() ) );
		FF2->Draw( mWindow->GetWidth() - FF2->GetTextWidth() - 15, 0 );

		FF2->SetText( InBuf.Buffer() );
		FF2->Draw( 6, 180, FONT_DRAW_SHADOW );
	}

	cUIManager::instance()->Draw();
	cUIManager::instance()->Update();


	Con.Draw();
}

void cEETest::Input() {
	KM->Update();
	JM->Update();

	Mouse = KM->GetMousePos();
	Mousef = Vector2f( (Float)Mouse.x, (Float)Mouse.y );

	if ( KM->IsKeyUp( KEY_F1 ) )
		Graphics::cShaderProgramManager::instance()->Reload();

	if ( !mWindow->Visible() ) {
		mWasMinimized = true;

		mWindow->FrameRateLimit( 10 );

		if ( mMusEnabled && Mus->State() == Sound::Playing )
			Mus->Pause();

	} else {
		if ( mLastFPSLimit != mWindow->FrameRateLimit() && !mWasMinimized )
			mLastFPSLimit = mWindow->FrameRateLimit();

		if ( mWasMinimized ) {
			mWasMinimized = false;

			if ( !mWindow->Windowed() )
				KM->GrabInput( true );
		}

		mWindow->FrameRateLimit( mLastFPSLimit );

		if ( mMusEnabled && Mus->State() == Sound::Paused )
			Mus->Play();
	}

	if ( KM->IsKeyDown( KEY_ESCAPE ) )
		mWindow->Close();

	if ( KM->IsKeyUp( KEY_F1 ) )
		MultiViewportMode = !MultiViewportMode;

	if ( KM->AltPressed() && KM->IsKeyUp( KEY_C ) )
		mWindow->Center();

	if ( KM->AltPressed() && KM->IsKeyUp( KEY_M ) && !Con.Active() ) {
		if ( !mWindow->IsMaximized() )
			mWindow->Maximize();
	}

	if ( KM->IsKeyUp(KEY_F4) )
		TF->ReloadAllTextures();

	if ( KM->AltPressed() && KM->IsKeyUp( KEY_RETURN ) ) {
		if ( mWindow->Windowed() ) {
			mWindow->Size( mWindow->GetDesktopResolution().Width(), mWindow->GetDesktopResolution().Height(), false );
		} else {
			mWindow->ToggleFullscreen();
		}
	}

	if ( KM->GrabInput() ) {
		if ( KM->AltPressed() && KM->IsKeyDown( KEY_TAB ) ) {
			mWindow->Minimize();

			if ( KM->GrabInput() )
				KM->GrabInput( false );
		}
	}

	if ( KM->ControlPressed() && KM->IsKeyUp(KEY_G) )
		KM->GrabInput(  !KM->GrabInput() );

	if ( KM->IsKeyUp( KEY_F3 ) || KM->IsKeyUp( KEY_WORLD_26 ) || KM->IsKeyUp( KEY_BACKSLASH ) ) {
		Con.Toggle();
		InBuf.Active( !Con.Active() );
	}

	if ( KM->IsKeyUp(KEY_1) && KM->ControlPressed() )
		SetScreen( 0 );

	if ( KM->IsKeyUp(KEY_2) && KM->ControlPressed() )
		SetScreen( 1 );

	if ( KM->IsKeyUp(KEY_3) && KM->ControlPressed() )
		SetScreen( 2 );

	if ( KM->IsKeyUp(KEY_4) && KM->ControlPressed() )
		SetScreen( 3 );

	if ( KM->IsKeyUp(KEY_5) && KM->ControlPressed() )
		SetScreen( 4 );

	if ( KM->IsKeyUp(KEY_6) && KM->ControlPressed() )
		SetScreen( 5 );

	Joystick * Joy = JM->GetJoystick(0);

	if ( mJoyEnabled && NULL != Joy ) {
		if ( Joy->IsButtonDown(0) )		KM->InjectButtonPress(EE_BUTTON_LEFT);
		if ( Joy->IsButtonDown(1) )		KM->InjectButtonPress(EE_BUTTON_RIGHT);
		if ( Joy->IsButtonDown(2) )		KM->InjectButtonPress(EE_BUTTON_MIDDLE);
		if ( Joy->IsButtonUp(0) )		KM->InjectButtonRelease(EE_BUTTON_LEFT);
		if ( Joy->IsButtonUp(1) )		KM->InjectButtonRelease(EE_BUTTON_RIGHT);
		if ( Joy->IsButtonUp(2) )		KM->InjectButtonRelease(EE_BUTTON_MIDDLE);
		if ( Joy->IsButtonUp(3) )		KM->InjectButtonRelease(EE_BUTTON_WHEELUP);
		if ( Joy->IsButtonUp(7) )		KM->InjectButtonRelease(EE_BUTTON_WHEELDOWN);
		if ( Joy->IsButtonUp(4) )		SetScreen( 0 );
		if ( Joy->IsButtonUp(5) )		SetScreen( 1 );
		if ( Joy->IsButtonUp(6) )		SetScreen( 2 );

		Float aX = Joy->GetAxis( AXIS_X );
		Float aY = Joy->GetAxis( AXIS_Y );

		if ( 0 != aX || 0 != aY ) {
			double rE = mWindow->Elapsed().AsMilliseconds();
			mAxisX += aX * rE;
			mAxisY += aY * rE;
		}

		if ( ( mAxisX != 0 && ( mAxisX >= 1.f || mAxisX <= -1.f ) ) || ( mAxisY != 0 && ( mAxisY >= 1.f || mAxisY <= -1.f )  ) ) {
			Float nmX = Mousef.x + mAxisX;
			Float nmY = Mousef.y + mAxisY;

			nmX = eemax<Float>( nmX, 0 );
			nmY = eemax<Float>( nmY, 0 );
			nmX = eemin( nmX, (Float)EE->GetWidth() );
			nmY = eemin( nmY, (Float)EE->GetHeight() );

			KM->InjectMousePos( (Int32)nmX, (Int32)nmY );

			nmX -= (Int32)nmX;
			nmY -= (Int32)nmY;

			mAxisX 		= nmX;
			mAxisY	 	= nmY;
		}
	}

	switch (Screen) {
		case 0:
			if ( KM->IsKeyUp( KEY_R ) ) {
				PhysicsDestroy();
				PhysicsCreate();
			}

			if ( KM->IsKeyUp( KEY_1 ) )
				ChangeDemo( 0 );

			if ( KM->IsKeyUp( KEY_2 ) )
				ChangeDemo( 1 );
		case 1:
			if ( NULL != Joy ) {
				Uint8 hat = Joy->GetHat();

				if ( HAT_LEFT == hat || HAT_LEFTDOWN == hat || HAT_LEFTUP == hat )
					Map.Move( (mWindow->Elapsed().AsMilliseconds() * 0.2f), 0 );

				if ( HAT_RIGHT == hat || HAT_RIGHTDOWN == hat || HAT_RIGHTUP == hat )
					Map.Move( -mWindow->Elapsed().AsMilliseconds() * 0.2f, 0 );

				if ( HAT_UP == hat || HAT_LEFTUP == hat || HAT_RIGHTUP == hat )
					Map.Move( 0, (mWindow->Elapsed().AsMilliseconds() * 0.2f) );

				if ( HAT_DOWN == hat || HAT_LEFTDOWN == hat || HAT_RIGHTDOWN == hat )
					Map.Move( 0, -mWindow->Elapsed().AsMilliseconds() * 0.2f );
			}

			if ( KM->IsKeyDown(KEY_LEFT) ) {
				Map.Move( mWindow->Elapsed().AsMilliseconds() * 0.2f, 0 );
			}

			if ( KM->IsKeyDown(KEY_RIGHT) ) {
				Map.Move( -mWindow->Elapsed().AsMilliseconds() * 0.2f, 0 );
			}

			if ( KM->IsKeyDown(KEY_UP) ) {
				Map.Move( 0, mWindow->Elapsed().AsMilliseconds() * 0.2f );
			}

			if ( KM->IsKeyDown(KEY_DOWN) ) {
				Map.Move( 0, -mWindow->Elapsed().AsMilliseconds() * 0.2f );
			}

			if ( KM->IsKeyUp(KEY_F8) )
				Map.Reset();

			break;
		case 2:
			if ( KM->IsKeyUp(KEY_S) )
				SP.SetRepeations(1);

			if ( KM->IsKeyUp(KEY_A) )
				SP.SetRepeations(-1);

			if ( KM->IsKeyUp(KEY_D) )
				SP.ReverseAnim( !SP.ReverseAnim() );

			if ( KM->MouseRightPressed() )
				DrawBack = true;
			else
				DrawBack = false;

			if ( KM->IsKeyUp( KEY_P ) )
				SndMng.Play( "mysound" );

			if ( KM->ControlPressed() && KM->IsKeyUp(KEY_P) ) {
				ShowParticles = !ShowParticles;
			}

			break;
	}
}

void cEETest::Update() {
	mWindow->Clear();

	et = mWindow->Elapsed();

	Input();

	mResLoad.Update();

	if ( mFontLoader.IsLoaded() ) {
		Render();
	} else {
		mFontLoader.Update();
	}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	UpdateParticles();
#endif

	if ( KM->IsKeyUp(KEY_F12) ) mWindow->TakeScreenshot( MyPath + "screenshots/" ); //After render and before Display

	mWindow->Display(false);
}

void cEETest::Process() {
	Init();

	if ( NULL != mWindow && mWindow->Created() ) {
		TestInstance = this;

		mWindow->RunMainLoop( &MainLoop );
	}

	End();
}

void cEETest::ParticlesCallback( cParticle * P, cParticleSystem * Me ) {
	Float x, y, radio;
	Vector2f MePos( Me->Position() );

	radio = (Math::Randf(1.f, 1.2f) + sin( 20.0f / P->Id() )) * 24;
	x = MePos.x + radio * cos( (Float)P->Id() );
	y = MePos.y + radio * sin( (Float)P->Id() );
	P->Reset(x, y, Math::Randf(-10.f, 10.f), Math::Randf(-10.f, 10.f), Math::Randf(-10.f, 10.f), Math::Randf(-10.f, 10.f));
	P->Color( ColorAf(1.f, 0.6f, 0.3f, 1.f), 0.02f + Math::Randf() * 0.3f );
}

void cEETest::Particles() {
	PS[0].Position( Mousef );

	if ( DrawBack )
		PS[1].Position( Mousef );

	PS[2].Position( HWidth, HHeight );
	PS[3].Position(  Math::cosAng(Ang) * 220.f + HWidth + Math::Randf(0.f, 10.f),  Math::sinAng(Ang) * 220.f + HHeight + Math::Randf(0.f, 10.f) );
	PS[4].Position( -Math::cosAng(Ang) * 220.f + HWidth + Math::Randf(0.f, 10.f), -Math::sinAng(Ang) * 220.f + HHeight + Math::Randf(0.f, 10.f) );

	for ( Uint32 i = 0; i < PS.size(); i++ )
		PS[i].Draw();
}

#define GRABABLE_MASK_BIT (1<<31)
#define NOT_GRABABLE_MASK (~GRABABLE_MASK_BIT)

void cEETest::CreateJointAndBody() {
	#ifndef EE_PLATFORM_TOUCH
	mMouseJoint	= NULL;
	mMouseBody	= eeNew( cBody, ( INFINITY, INFINITY ) );
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		mMouseJoint[i] = NULL;
		mMouseBody[i] = eeNew( cBody, ( INFINITY, INFINITY ) );
	}
	#endif
}

void cEETest::Demo1Create() {
	CreateJointAndBody();

	cShape::ResetShapeIdCounter();

	mSpace = Physics::cSpace::New();
	mSpace->Gravity( cVectNew( 0, 100 ) );
	mSpace->SleepTimeThreshold( 0.5f );

	cBody *body, *staticBody = mSpace->StaticBody();
	cShape * shape;

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, mWindow->GetHeight() ), cVectNew( mWindow->GetWidth(), mWindow->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( mWindow->GetWidth(), 0 ), cVectNew( mWindow->GetWidth(), mWindow->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, 0 ), cVectNew( 0, mWindow->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, 0 ), cVectNew( mWindow->GetWidth(), 0 ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	Float hw = mWindow->GetWidth() / 2;

	for(int i=0; i<14; i++){
		for(int j=0; j<=i; j++){
			body = mSpace->AddBody( cBody::New( 1.0f, Moment::ForBox( 1.0f, 30.0f, 30.0f ) ) );
			body->Pos( cVectNew( hw + j * 32 - i * 16, 100 + i * 32 ) );

			//shape = mSpace->AddShape( cShapePolySprite::New( body, 30.f, 30.f, mBoxSprite ) );
			shape = mSpace->AddShape( cShapePoly::New( body, 30.f, 30.f ) );
			shape->e( 0.0f );
			shape->u( 0.8f );
		}
	}

	cpFloat radius = 15.0f;

	body = mSpace->AddBody( cBody::New( 10.0f, Moment::ForCircle( 10.0f, 0.0f, radius, cVectZero ) ) );
	body->Pos( cVectNew( hw, mWindow->GetHeight() - radius - 5 ) );

	//shape = mSpace->AddShape( cShapeCircleSprite::New( body, radius, cVectZero, mCircleSprite ) );
	shape = mSpace->AddShape( cShapeCircle::New( body, radius, cVectZero ) );
	shape->e( 0.0f );
	shape->u( 0.9f );
}

void cEETest::Demo1Update() {

}

void cEETest::DestroyBody() {
	#ifndef EE_PLATFORM_TOUCH
	eeSAFE_DELETE( mMouseBody );
	#else
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		eeSAFE_DELETE( mMouseBody[i] );
	}
	#endif
}

void cEETest::Demo1Destroy() {
	DestroyBody();

	eeSAFE_DELETE( mSpace );
}

cpBool cEETest::blockerBegin( cArbiter *arb, cSpace *space, void *unused ) {
	cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->blocked++;

	return cpFalse; // Return values from sensors callbacks are ignored,
}

void cEETest::blockerSeparate( cArbiter *arb, cSpace * space, void *unused ) {
	cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->blocked--;
}

void cEETest::postStepRemove( cSpace *space, void * tshape, void * unused ) {
	cShape * shape = reinterpret_cast<cShape*>( tshape );

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
	cShape::Free( shape, true );
}

cpBool cEETest::catcherBarBegin(cArbiter *arb, Physics::cSpace *space, void *unused) {
	cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->queue++;

	mSpace->AddPostStepCallback( cb::Make3( this, &cEETest::postStepRemove ), b, NULL );

	return cpFalse;
}

void cEETest::Demo2Create() {
	CreateJointAndBody();

	cShape::ResetShapeIdCounter();

	mSpace = Physics::cSpace::New();
	mSpace->Iterations( 10 );
	mSpace->Gravity( cVectNew( 0, 100 ) );

	cBody * staticBody = mSpace->StaticBody();
	cShape * shape;

	emitterInstance.queue = 5;
	emitterInstance.blocked = 0;
	emitterInstance.position = cVectNew( mWindow->GetWidth() / 2 , 150);

	shape = mSpace->AddShape( cShapeCircle::New( staticBody, 15.0f, emitterInstance.position ) );
	shape->Sensor( 1 );
	shape->CollisionType( BLOCKING_SENSOR_TYPE );
	shape->Data( &emitterInstance );

	// Create our catch sensor to requeue the balls when they reach the bottom of the screen
	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew(-4000, 600), cVectNew(4000, 600), 15.0f ) );
	shape->Sensor( 1 );
	shape->CollisionType( CATCH_SENSOR_TYPE );
	shape->Data( &emitterInstance );

	cSpace::cCollisionHandler handler;
	handler.a			= BLOCKING_SENSOR_TYPE;
	handler.b			= BALL_TYPE;
	handler.begin		= cb::Make3( this, &cEETest::blockerBegin );
	handler.separate	= cb::Make3( this, &cEETest::blockerSeparate );
	mSpace->AddCollisionHandler( handler );

	handler.Reset(); // Reset all the values and the callbacks ( set the callbacks as !IsSet()

	handler.a			= CATCH_SENSOR_TYPE;
	handler.b			= BALL_TYPE;
	handler.begin		= cb::Make3( this, &cEETest::catcherBarBegin );
	mSpace->AddCollisionHandler( handler );
}

void cEETest::Demo2Update() {
	if( !emitterInstance.blocked && emitterInstance.queue ){
		emitterInstance.queue--;

		cBody * body = mSpace->AddBody( cBody::New( 1.0f, Moment::ForCircle(1.0f, 15.0f, 0.0f, cVectZero ) ) );
		body->Pos( emitterInstance.position );
		body->Vel( cVectNew( Math::Randf(-1,1), Math::Randf(-1,1) ) * (cpFloat)100 );

		cShape *shape = mSpace->AddShape( cShapeCircle::New( body, 15.0f, cVectZero ) );
		shape->CollisionType( BALL_TYPE );
	}
}

void cEETest::Demo2Destroy() {
	DestroyBody();
	eeSAFE_DELETE( mSpace );
}

void cEETest::ChangeDemo( Uint32 num ) {
	if ( num < mDemo.size() ) {
		if ( eeINDEX_NOT_FOUND != mCurDemo )
			mDemo[ mCurDemo ].destroy();

		mCurDemo = num;

		mDemo[ mCurDemo ].init();
	}
}

void cEETest::PhysicsCreate() {
	cPhysicsManager::CreateSingleton();
	cPhysicsManager * PM = cPhysicsManager::instance();
	cPhysicsManager::cDrawSpaceOptions * DSO = PM->GetDrawOptions();

	DSO->DrawBBs			= false;
	DSO->DrawShapes			= true;
	DSO->CollisionPointSize	= 0;
	DSO->BodyPointSize		= 0;
	DSO->LineThickness		= 1;

	mDemo.clear();

	physicDemo demo;

	demo.init		= cb::Make0( this, &cEETest::Demo1Create );
	demo.update		= cb::Make0( this, &cEETest::Demo1Update );
	demo.destroy	= cb::Make0( this, &cEETest::Demo1Destroy );
	mDemo.push_back( demo );

	demo.init		= cb::Make0( this, &cEETest::Demo2Create );
	demo.update		= cb::Make0( this, &cEETest::Demo2Update );
	demo.destroy	= cb::Make0( this, &cEETest::Demo2Destroy );
	mDemo.push_back( demo );

	ChangeDemo( 0 );
}

void cEETest::PhysicsUpdate() {	
	#ifndef EE_PLATFORM_TOUCH
	mMousePoint = cVectNew( KM->GetMousePosf().x, KM->GetMousePosf().y );
	cVect newPoint = tovect( cpvlerp( tocpv( mMousePoint_last ), tocpv( mMousePoint ), 0.25 ) );
	mMouseBody->Pos( newPoint );
	mMouseBody->Vel( ( newPoint - mMousePoint_last ) * (cpFloat)mWindow->FPS() );
	mMousePoint_last = newPoint;

	if ( KM->MouseLeftPressed() ) {
		if ( NULL == mMouseJoint ) {
			cVect point = cVectNew( KM->GetMousePosf().x, KM->GetMousePosf().y );

			cShape * shape = mSpace->PointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

			if( NULL != shape ){
				mMouseJoint = eeNew( cPivotJoint, ( mMouseBody, shape->Body(), cVectZero, shape->Body()->World2Local( point ) ) );

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

				cShape * shape = mSpace->PointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

				if( NULL != shape ){
					mMouseJoint[i] = eeNew( cPivotJoint, ( mMouseBody[i], shape->Body(), cVectZero, shape->Body()->World2Local( point ) ) );

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

void cEETest::PhysicsDestroy() {
	mDemo[ mCurDemo ].destroy();
}

void cEETest::End() {
	Wait();

	PhysicsDestroy();

	eeSAFE_DELETE( Mus );
	eeSAFE_DELETE( mTGL );
	eeSAFE_DELETE( mFBO );
	eeSAFE_DELETE( mVBO );
	eeSAFE_DELETE( mBoxSprite );
	eeSAFE_DELETE( mCircleSprite );
	eeSAFE_DELETE( PakTest );

	Log::instance()->Save();

	Engine::DestroySingleton();
}

}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	Demo_Test::cEETest * Test = eeNew( Demo_Test::cEETest, () );

	Test->Process();

	eeDelete( Test );

	MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
