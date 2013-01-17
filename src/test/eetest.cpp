#include "eetest.hpp"

namespace Demo_Test {

void cEETest::Init() {
	EE = cEngine::instance();

	cLog::instance()->LiveWrite( true );
	cLog::instance()->ConsoleOutput( true );

	run 				= false;
	DrawBack 			= false;
	MultiViewportMode 	= false;

	side = aside 		= true;
	ShowParticles 		= true;
	scale 				= 1.0f;
	Ang = ang = alpha 	= 0;
	lasttick 			= 0;
	Wireframe 			= false;
	TreeTilingCreated 	= false;
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

	cIniFile Ini( MyPath + "ee.ini" );
	Ini.ReadFile();

	PartsNum			= Ini.GetValueI( "EEPP", "ParticlesNum", 1000);
	mUseShaders			= Ini.GetValueB( "EEPP", "UseShaders", false );
	mJoyEnabled			= Ini.GetValueB( "EEPP", "JoystickEnabled", false );
	mMusEnabled			= Ini.GetValueB( "EEPP", "Music", false );
	Int32 StartScreen	= Ini.GetValueI( "EEPP", "StartScreen", 0 );

	WindowSettings WinSettings	= EE->CreateWindowSettings( &Ini );
	ContextSettings ConSettings	= EE->CreateContextSettings( &Ini );

	PAK = eeNew( cZip, () );
	PAK->Open( MyPath + "ee.zip" );

	mWindow = EE->CreateWindow( WinSettings, ConSettings );

	run = ( NULL != mWindow && mWindow->Created() && PAK->IsOpen() );

	if ( run ) {
		SetScreen( StartScreen );

		mWindow->Caption( "EE++ Test Application" );

		TF = cTextureFactory::instance();
		TF->Allocate(40);

		Log		= cLog::instance();
		KM		= mWindow->GetInput();
		JM		= KM->GetJoystickManager();

		PS.resize(5);

		Scenes[0] = cb::Make0( this, &cEETest::PhysicsUpdate );
		Scenes[1] = cb::Make0( this, &cEETest::Screen1 );
		Scenes[2] = cb::Make0( this, &cEETest::Screen2 );
		Scenes[3] = cb::Make0( this, &cEETest::Screen3 );
		Scenes[4] = cb::Make0( this, &cEETest::Screen4 );
		Scenes[5] = cb::Make0( this, &cEETest::Screen5 );

		InBuf.Start();
		InBuf.SupportNewLine( true );

		SetRandomSeed( static_cast<Uint32>( Sys::GetSystemTime() * 1000 ) );

		LoadTextures();

		LoadFonts();

		CreateShaders();

		if ( mMusEnabled ) {
			Mus = eeNew( cMusic, () );

			if ( Mus->OpenFromPack( PAK, "music.ogg" ) ) {
				Mus->Loop( true );
				Mus->Play();
			}
		}

		WP.Type( QUARTICINOUT );
		WP.AddWaypoint( eeVector2f(0,0), 100 );
		WP.AddWaypoint( eeVector2f(800,0), 100 );
		WP.AddWaypoint( eeVector2f(0,0), 100 );
		WP.AddWaypoint( eeVector2f(1024,768), 100 );
		WP.AddWaypoint( eeVector2f(0,600), 100 );
		WP.EditWaypoint( 2, eeVector2f(800,600), 100 );
		WP.EraseWaypoint( 3 );
		WP.Loop(true);
		WP.SetTotalTime(5000);
		WP.Start();

		Batch.AllocVertexs( 2048 );
		Batch.SetBlendMode( ALPHA_BLENDONE );

		mFBO = cFrameBuffer::New( 256, 256, false );

		if ( NULL != mFBO )
			mFBO->ClearColor( eeColorAf( 0, 0, 0, 0.5f ) );

		eePolygon2f Poly = eePolygon2f::CreateRoundedPolygon( 0, 0, 256, 50 );

		mVBO = cVertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, DM_TRIANGLE_FAN );

        if ( NULL != mVBO ) {
            for ( Uint32 i = 0; i < Poly.Size(); i++ ) {
                mVBO->AddVertex( Poly[i] );
                mVBO->AddColor( eeColorA( 100 + i, 255 - i, 150 + i, 200 ) );
            }

            mVBO->Compile();
        }

		PhysicsCreate();

		Launch();
	} else {
		cEngine::DestroySingleton();

		exit(0);
	}
}

void cEETest::CreateAquaTextureAtlas() {
	#if !defined( EE_DEBUG ) || defined( EE_GLES )
	return;
	#endif

	std::string tgpath( MyPath + "aquata/aqua" );
	std::string Path( MyPath + "aqua" );

	if ( !FileSystem::FileExists( tgpath + EE_TEXTURE_ATLAS_EXTENSION ) ) {
		cTexturePacker tp( 256, 256, true, 2 );
		tp.AddTexturesPath( Path );
		tp.PackTextures();
		tp.Save( tgpath + ".png", EE_SAVE_TYPE_PNG );
	} else {
		cTextureAtlasLoader tgl;
		tgl.UpdateTextureAtlas( tgpath + EE_TEXTURE_ATLAS_EXTENSION, Path );
	}
}

void cEETest::LoadFonts() {
	mFTE.Reset();

	cTextureLoader * tl = eeNew( cTextureLoader, ( PAK, "conchars.png" ) );
	tl->SetColorKey( eeColor(0,0,0) );

	mFontLoader.Add( eeNew( cTextureFontLoader, ( "conchars", tl, (eeUint)32 ) ) );
	mFontLoader.Add( eeNew( cTextureFontLoader, ( "ProggySquareSZ", eeNew( cTextureLoader, ( PAK, "ProggySquareSZ.png" ) ), PAK, "ProggySquareSZ.dat" ) ) );
	mFontLoader.Add( eeNew( cTTFFontLoader, ( "arial", PAK, "arial.ttf", 12, EE_TTF_STYLE_NORMAL, false, 256, eeColor(255,255,255) ) ) );
	mFontLoader.Add( eeNew( cTTFFontLoader, ( "arialb", PAK, "arial.ttf", 12, EE_TTF_STYLE_NORMAL, false, 256, eeColor(255,255,255), 1, eeColor(0,0,0), true ) ) );

	mFontLoader.Load( cb::Make1( this, &cEETest::OnFontLoaded ) );
}

void cEETest::OnFontLoaded( cResourceLoader * ObjLoaded ) {
	FF		= cFontManager::instance()->GetByName( "conchars" );
	FF2		= cFontManager::instance()->GetByName( "ProggySquareSZ" );
	TTF		= cFontManager::instance()->GetByName( "arial" );
	TTFB	= cFontManager::instance()->GetByName( "arialb" );

	Log->Writef( "Fonts loading time: %f ms.", mFTE.Elapsed() );

	eeASSERT( TTF != NULL );
	eeASSERT( TTFB != NULL );

	Map.Font( FF );

	Con.Create( FF, true );
	Con.IgnoreCharOnPrompt( 186 ); // 'º'

	mBuda = String::FromUtf8( "El mono ve el pez en el agua y sufre. Piensa que su mundo es el único que existe, el mejor, el real. Sufre porque es bueno y tiene compasión, lo ve y piensa: \"Pobre se está ahogando no puede respirar\". Y lo saca, lo saca y se queda tranquilo, por fin lo salvé. Pero el pez se retuerce de dolor y muere. Por eso te mostré el sueño, es imposible meter el mar en tu cabeza, que es un balde." );

	CreateUI();

	mEEText.Create( TTFB, "Entropia Engine++\nCTRL + Number to change Demo Screen\nRight click to see the PopUp Menu" );
	mFBOText.Create( TTFB, "This is a VBO\nInside of a FBO" );
	mFBOText.Color( eeColorA(255,255,0,255), 10, 13 );
	mFBOText.Color( eeColorA(255,255,0,255), 25, 28 );

	mInfoText.Create( FF, "", eeColorA(255,255,255,150) );
}

void cEETest::CreateShaders() {
	mUseShaders = mUseShaders && GLi->ShadersSupported();

	mShaderProgram = NULL;

	if ( mUseShaders ) {
		mBlurFactor = 0.01f;
		mShaderProgram = cShaderProgram::New( MyPath + "shader/blur.vert", MyPath + "shader/blur.frag" );
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

void cEETest::OnTerrainMouse( const cUIEvent * Event ) {
	cUIPushButton * PB = static_cast<cUIPushButton*>( Event->Ctrl() );

	if ( PB->Text() == "Terrain Up" ) {
		PB->Text( "Terrain Down" );
		mTerrainUp = false;
	} else {
		PB->Text( "Terrain Up" );
		mTerrainUp = true;
	}
}

void cEETest::OnShowMenu( const cUIEvent * Event ) {
	cUIPushButton * PB = static_cast<cUIPushButton*>( Event->Ctrl() );

	if ( Menu->Show() ) {
		eeVector2i Pos = eeVector2i( (Int32)PB->GetPolygon()[0].x, (Int32)PB->GetPolygon()[0].y - 2 );
		cUIMenu::FixMenuPos( Pos , Menu );
		Menu->Pos( Pos );
	}
}


void cEETest::CreateUI() {
	cTimeElapsed TE;

	CreateAquaTextureAtlas();

	Log->Writef( "Texture Atlas Loading Time: %f ms.", TE.ElapsedSinceStart() );

	cUIManager::instance()->Init(); //UI_MANAGER_HIGHLIGHT_FOCUS

	//mTheme = cUITheme::LoadFromPath( eeNew( cUIAquaTheme, ( "aqua", "aqua" ) ), MyPath + "aqua/" );

	cTextureAtlasLoader tgl( MyPath + "aquata/aqua" + EE_TEXTURE_ATLAS_EXTENSION );

	mTheme = cUITheme::LoadFromTextureAtlas( eeNew( cUIAquaTheme, ( "aqua", "aqua" ) ), cTextureAtlasManager::instance()->GetByName( "aqua" ) );

	cUIThemeManager::instance()->Add( mTheme );
	cUIThemeManager::instance()->DefaultEffectsEnabled( true );
	cUIThemeManager::instance()->DefaultFont( TTF );
	cUIThemeManager::instance()->DefaultTheme( "aqua" );

	cUIControl::CreateParams Params( cUIManager::instance()->MainControl(), eeVector2i(0,0), eeSize( 530, 380 ), UI_FILL_BACKGROUND | UI_CLIP_ENABLE | UI_BORDER );

	Params.Border.Width( 2 );
	Params.Border.Color( 0xFF979797 );
	Params.Background.Colors( eeColorA( 0x66EDEDED ), eeColorA( 0xCCEDEDED ), eeColorA( 0xCCEDEDED ), eeColorA( 0x66EDEDED ) );

	cUIWindow * tWin = mTheme->CreateWindow( NULL, eeSize( 530, 405 ), eeVector2i( 320, 240 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DRAGABLE_CONTAINER , eeSize( 530, 405 ), 200 );
	C = tWin->Container();

	tWin->Title( "Controls Test" );

	tWin->AddEventListener( cUIEvent::EventMouseUp, cb::Make1( this, &cEETest::OnWinMouseUp ) );
	C->AddEventListener( cUIEvent::EventMouseUp, cb::Make1( this, &cEETest::OnWinMouseUp ) );

	Params.Flags &= ~UI_CLIP_ENABLE;
	Params.Background.Corners(0);
	Params.Background.Colors( eeColorA( 0x7700FF00 ), eeColorA( 0x7700CC00 ), eeColorA( 0x7700CC00 ), eeColorA( 0x7700FF00 ) );
	Params.Parent( C );
	Params.Size = eeSize( 50, 50 );
	cUITest * Child = eeNew( cUITest, ( Params ) );
	Child->Pos( 240, 130 );
	Child->Visible( true );
	Child->Enabled( true );
	Child->StartRotation( 0.f, 360.f, 5000.f );
	Child->AngleInterpolation()->Loop( true );

	Params.Background.Colors( eeColorA( 0x77FFFF00 ), eeColorA( 0x77CCCC00 ), eeColorA( 0x77CCCC00 ), eeColorA( 0x77FFFF00 ) );
	Params.Parent( Child );
	Params.Size = eeSize( 25, 25 );
	cUITest * Child2 = eeNew( cUITest, ( Params ) );
	Child2->Pos( 15, 15 );
	Child2->Visible( true );
	Child2->Enabled( true );
	Child2->StartRotation( 0.f, 360.f, 5000.f );
	Child2->AngleInterpolation()->Loop( true );

	mTheme->CreateSprite( eeNew( cSprite, ( "gn" ) ), C, eeSize(), eeVector2i( 160, 100 ) );

	cUITextBox::CreateParams TextParams;
	TextParams.Parent( C );
	TextParams.PosSet( 0, 0 );
	TextParams.Size = eeSize( 320, 240 );
	TextParams.Flags = UI_VALIGN_TOP | UI_HALIGN_RIGHT;
	cUITextBox * Text = eeNew( cUITextBox, ( TextParams ) );
	Text->Visible( true );
	Text->Enabled( false );
	Text->Text( "Turn around\nJust Turn Around\nAround!" );

	cUITextInput::CreateParams InputParams;
	InputParams.Parent( C );
	InputParams.PosSet( 20, 216 );
	InputParams.Size = eeSize( 200, 22 );
	InputParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_CLIP_ENABLE | UI_AUTO_PADDING;
	cUITextInput * Input = eeNew( cUITextInput, ( InputParams ) );
	Input->Visible( true );
	Input->Enabled( true );

	cUIPushButton::CreateParams ButtonParams;
	ButtonParams.Parent( C );
	ButtonParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE;
	ButtonParams.PosSet( 225, 216 );
	ButtonParams.Size = eeSize( 90, 0 );
	ButtonParams.SetIcon( mTheme->GetIconByName( "ok" ) );
	cUIPushButton * Button = eeNew( cUIPushButton, ( ButtonParams ) );
	Button->Visible( true );
	Button->Enabled( true );
	Button->Text( "Click Me" );
	Button->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::ButtonClick ) );
	Button->TooltipText( "Click and see what happens..." );

	TextParams.PosSet( 130, 20 );
	TextParams.Size = eeSize( 80, 22 );
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
	SliderParams.Size = eeSize( 80, 24 );
	mSlider = eeNew( cUISlider, ( SliderParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );

	SliderParams.PosSet( 40, 110 );
	SliderParams.Size = eeSize( 24, 80 );
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
	SpinBoxParams.Size = eeSize( 80, 24 );
	SpinBoxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_CLIP_ENABLE;
	SpinBoxParams.AllowDotsInNumbers = true;
	cUISpinBox * mSpinBox = eeNew( cUISpinBox, ( SpinBoxParams ) );
	mSpinBox->Visible( true );
	mSpinBox->Enabled( true );

	cUIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent( C );
	ScrollBarP.PosSet( 0, 0 );
	ScrollBarP.Size = eeSize( 15, 240 );
	ScrollBarP.Flags = UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar = true;
	mScrollBar = eeNew( cUIScrollBar, ( ScrollBarP ) );
	mScrollBar->Visible( true );
	mScrollBar->Enabled( true );
	mScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cEETest::OnValueChange ) );

	cUIProgressBar::CreateParams PBParams;
	PBParams.Parent( C );
	PBParams.PosSet( 20, 197 );
	PBParams.Size = eeSize( 200, 16 );
	PBParams.DisplayPercent = true;
	PBParams.MovementSpeed = eeVector2f( -64, 0 );
	mProgressBar = eeNew( cUIProgressBar, ( PBParams ) );
	mProgressBar->Visible( true );
	mProgressBar->Enabled( true );

	TextParams.PosSet( 20, 5 );
	mTextBoxValue = eeNew( cUITextBox, ( TextParams ) );
	mTextBoxValue->Visible( true );
	OnValueChange( NULL );

	cUIListBox::CreateParams LBParams;
	LBParams.Parent( C );
	LBParams.PosSet( 325, 8 );
	LBParams.Size = eeSize( 200, 240-16 );
	LBParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING; // | UI_MULTI_SELECT
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
	DDLParams.Size = eeSize( 100, 21 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	cUIDropDownList * mDropDownList = eeNew( cUIDropDownList, ( DDLParams ) );
	mDropDownList->Visible( true );
	mDropDownList->Enabled( true );

	std::vector<String> combostrs;
	combostrs.push_back( "Plane" );
	combostrs.push_back( "Car" );
	combostrs.push_back( "Bus" );
	combostrs.push_back( "Train" );

	mDropDownList->ListBox()->AddListBoxItems( combostrs );
	mDropDownList->ListBox()->SetSelected( 0 );

	cUIComboBox::CreateParams ComboParams;
	ComboParams.Parent( C );
	ComboParams.PosSet( 20, 80 );
	ComboParams.Size = eeSize( 100, 1 );
	ComboParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_AUTO_SIZE;
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
	TEParams.Size	= eeSize( 315, 130 );
	TEParams.Flags = UI_AUTO_PADDING | UI_CLIP_ENABLE;
	cUITextEdit * TextEdit = eeNew( cUITextEdit, ( TEParams ) );
	TextEdit->Visible( true );
	TextEdit->Enabled( true );
	TextEdit->Text( mBuda );

	cUIGenericGrid::CreateParams GridParams;
	GridParams.Parent( C );
	GridParams.PosSet( 325, 245 );
	GridParams.SizeSet( 200, 130 );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 3;
	cUIGenericGrid * mGenGrid = eeNew( cUIGenericGrid, ( GridParams ) );
	mGenGrid->Visible( true );
	mGenGrid->Enabled( true );

	cUIGridCell::CreateParams CellParams;
	CellParams.Parent( mGenGrid->Container() );

	cUITextBox::CreateParams TxtBoxParams;
	cUITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER;

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

#ifdef EE_PLATFORM_TOUCH
	cTextureAtlas * SG = cGlobalTextureAtlas::instance();

	cTexture * butTex = TF->GetTexture( TF->Load( MyPath + "extra/button-te_normal.png" ) );

	SG->Add( butTex->Id(), "button-te_normal" );
	SG->Add( TF->Load( MyPath + "extra/button-te_mdown.png" ), "button-te_mdown" );

	cUISkinSimple nSkin( "button-te" );

	mShowMenu = mTheme->CreatePushButton( NULL, butTex->Size(), eeVector2i( mWindow->GetWidth() - butTex->Width() - 20, mWindow->GetHeight() - butTex->Height() - 10 ), UI_CONTROL_ALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mShowMenu->SetSkin( nSkin );
	mShowMenu->Text( "Show Menu" );
	mShowMenu->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::OnShowMenu ) );

	mTerrainBut = mTheme->CreatePushButton( NULL, butTex->Size(), eeVector2i( mShowMenu->Pos().x - butTex->Width() - 20, mWindow->GetHeight() - butTex->Height() - 10 ), UI_CONTROL_ALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mTerrainBut->SetSkin( nSkin );
	mTerrainBut->Text( "Terrain Up" );
	mTerrainBut->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::OnTerrainMouse ) );
	mTerrainBut->Visible( 1 == Screen );
#endif

	C = reinterpret_cast<cUIControlAnim*> ( C->Parent() );

	Log->Writef( "CreateUI time: %f ms.", TE.ElapsedSinceStart() );
}

void cEETest::CreateMapEditor() {
	if ( NULL != mMapEditor )
		return;

	cUIWindow * tWin = mTheme->CreateWindow( NULL, eeSize( 1024, 768 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, eeSize( 1024, 768 ) );
	mMapEditor = eeNew( cMapEditor, ( tWin, cb::Make0( this, &cEETest::OnMapEditorClose ) ) );
	tWin->Center();
	tWin->Show();
}

void cEETest::OnMapEditorClose() {
	mMapEditor = NULL;
}

void cEETest::CreateETGEditor() {
	cUIWindow * tWin = mTheme->CreateWindow( NULL, eeSize( 1024, 768 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER, eeSize( 1024, 768 ) );
	mETGEditor = eeNew ( Tools::cTextureAtlasEditor, ( tWin, cb::Make0( this, &cEETest::OnETGEditorClose ) ) );
	tWin->Center();
	tWin->Show();
}

void cEETest::OnETGEditorClose() {
	mETGEditor = NULL;
}

void cEETest::CreateCommonDialog() {
	cUICommonDialog * CDialog = mTheme->CreateCommonDialog( NULL, eeSize(), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON );
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
	mUIWindow = mTheme->CreateWindow( NULL, eeSize( 530, 350 ), eeVector2i( 200, 50 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON, eeSize( 100, 200 ) );

	mUIWindow->AddEventListener( cUIEvent::EventOnWindowCloseClick, cb::Make1( this, &cEETest::CloseClick ) );
	mUIWindow->Title( "Test Window" );
	mUIWindow->ToBack();

	cUIPushButton * Button = mTheme->CreatePushButton( mUIWindow->Container(), eeSize( 510, 22 ), eeVector2i( 10, 28 ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_ANCHOR_RIGHT );
	Button->Text( "Click Me" );
	Button->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::ButtonClick ) );

	mUIWindow->AddShortcut( KEY_C, KEYMOD_ALT, Button );

	cUITabWidget * TabWidget = mTheme->CreateTabWidget( mUIWindow->Container(), eeSize( 510, 250 ), eeVector2i( 10, 55 ), UI_HALIGN_CENTER | UI_VALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT | UI_ANCHOR_TOP );

	cUITextEdit * TEdit = mTheme->CreateTextEdit( TabWidget, eeSize(), eeVector2i(), UI_AUTO_PADDING );
	TEdit->Text( mBuda );
	TabWidget->Add( "TextEdit", TEdit );

	cUITextBox * Txt = mTheme->CreateTextInput( TabWidget, eeSize(), eeVector2i(), UI_AUTO_PADDING | UI_AUTO_SHRINK_TEXT );
	Txt->Text( mBuda );
	TabWidget->Add( "TextInput", Txt );

	TabWidget->Add( "TextBox", mTheme->CreateTextBox( mBuda, TabWidget, eeSize(), eeVector2i(), UI_AUTO_PADDING | UI_AUTO_SHRINK_TEXT ) );

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
	} else if ( "Show Window" == txt ) {
		cUIMenuCheckBox * Chk = reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() );

		C->Visible( true );
		C->Enabled( true );

		if ( Chk->Active() ) {
			if ( C->Scale() == 1.f ) C->Scale( 0.f );
			C->StartScaleAnim( C->Scale(), 1.f, 500.f, SINEOUT );
			C->StartAlphaAnim( C->Alpha(), 255.f, 500.f );
			C->StartRotation( 0, 360, 500.f, SINEOUT );
		} else {
			C->StartScaleAnim( C->Scale(), 0.f, 500.f, SINEIN );
			C->StartAlphaAnim( C->Alpha(), 0.f, 500.f );
			C->StartRotation( 0, 360, 500.f, SINEIN );
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

void cEETest::QuitClick( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		mWindow->Close();
	}
}

void cEETest::ShowMenu() {
	if ( Menu->Show() ) {
		eeVector2i Pos = mWindow->GetInput()->GetMousePos();
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

		Gfx->StartRotation( 0, 2500, 2500 );
		Gfx->StartMovement( eeVector2i( Math::Randi( 0, mWindow->GetWidth() ), -64 ), eeVector2i( Math::Randi( 0, mWindow->GetWidth() ), mWindow->GetHeight() + 64 ), 2500 );
		Gfx->CloseFadeOut( 3500 );

		mListBox->AddListBoxItem( "Test ListBox " + String::ToStr( mListBox->Count() + 1 ) + " testing it right now!" );
	}
}

void cEETest::SetScreen( Uint32 num ) {
	if ( NULL != mTerrainBut ) mTerrainBut->Visible( 1 == num );

	if ( 0 == num || 5 == num )
		mWindow->BackColor( eeColor( 240, 240, 240 ) );
	else
		mWindow->BackColor( eeColor( 0, 0, 0 ) );

	if ( num < 6 )
		Screen = num;
}

void cEETest::CmdSetPartsNum ( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt >= 0 && tInt <= 100000 ) ) {
			PS[2].Create( PSE_WormHole, tInt, TN[5], eeVector2f( mWindow->GetWidth() * 0.5f, mWindow->GetHeight() * 0.5f ), 32, true );
			Con.PushText( "Wormhole Particles Number Changed to: " + String::ToStr(tInt) );
		} else
			Con.PushText( "Valid parameters are between 0 and 100000 (0 = no limit)." );
	}
}

void cEETest::OnTextureLoaded( cResourceLoader * ResLoaded ) {
	SndMng.Play( "mysound" );
}

void cEETest::LoadTextures() {
	cTimeElapsed te;

	Uint32 i;

	PakTest = eeNew( cZip, () );

	#ifndef EE_GLES
	PakTest->Open( MyPath + "test.zip" );

	std::vector<std::string> files = PakTest->GetFileList();

	for ( i = 0; i < files.size(); i++ ) {
		std::string name( files[i] );

		if ( "jpg" == FileSystem::FileExtension( name ) ) {
			mResLoad.Add( eeNew( cTextureLoader, ( PakTest, name ) ) );
		}
	}
	#endif

	mResLoad.Add( eeNew( cSoundLoader, ( &SndMng, "mysound", PAK, "sound.ogg" ) ) );

	mResLoad.Load( cb::Make1( this, &cEETest::OnTextureLoaded ) );

	TN.resize(12);
	TNP.resize(12);

	for ( i = 0; i <= 7; i++ ) {
		TN[i] = TF->LoadFromPack( PAK, "t" + String::ToStr(i+1) + ".png", ( (i+1) == 7 ) ? true : false, ( (i+1) == 4 ) ? EE_CLAMP_REPEAT : EE_CLAMP_TO_EDGE );
		TNP[i] = TF->GetTexture( TN[i] );
	}

	Tiles.resize(10);

	cTextureAtlasLoader tgl( PAK, "tiles.etg" );
	cTextureAtlas * SG = cTextureAtlasManager::instance()->GetByName( "tiles" );

	if ( NULL != SG ) {
		for ( i = 0; i < 6; i++ ) {
			Tiles[i] = SG->GetByName( String::ToStr( i+1 ) );
		}

		Tiles[6] = SG->Add( TF->LoadFromPack( PAK, "objects/1.png" ), "7" );
		Tiles[7] = SG->Add( TF->LoadFromPack( PAK, "objects/2.png" ), "8" );

		Tiles[7]->GetTexture()->CreateMaskFromColor( eeColorA(0,0,0,255), 0 );
	}

	eeInt w, h;

	for ( Int32 my = 0; my < 4; my++ )
		for( Int32 mx = 0; mx < 8; mx++ )
			SP.AddFrame( TN[4], 0, 0, 0, 0, eeRecti( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );

	PS[0].SetCallbackReset( cb::Make2( this, &cEETest::ParticlesCallback ) );
	PS[0].Create( PSE_Callback, 500, TN[5], eeVector2f( 0, 0 ), 16, true );
	PS[1].Create( PSE_Heal, 250, TN[5], eeVector2f( mWindow->GetWidth() * 0.5f, mWindow->GetHeight() * 0.5f ), 16, true );
	PS[2].Create( PSE_WormHole, PartsNum, TN[5], eeVector2f( mWindow->GetWidth() * 0.5f, mWindow->GetHeight() * 0.5f ), 32, true );
	PS[3].Create( PSE_Fire, 350, TN[5], eeVector2f( -50.f, -50.f ), 32, true );
	PS[4].Create( PSE_Fire, 350, TN[5], eeVector2f( -50.f, -50.f ), 32, true );

	Con.AddCommand( "setparticlesnum", cb::Make1( this, &cEETest::CmdSetPartsNum ) );

	cTexture * Tex = TF->GetTexture( TN[2] );

	if ( NULL != Tex && Tex->Lock() ) {
		w = (eeInt)Tex->Width();
		h = (eeInt)Tex->Height();

		for ( y = 0; y < h; y++) {
			for ( x = 0; x < w; x++) {
				eeColorA C = Tex->GetPixel(x, y);

				if ( C.R() > 200 && C.G() > 200 && C.B() > 200 )
					Tex->SetPixel(x, y, eeColorA( Math::Randi(0, 255), Math::Randi(0, 255), Math::Randi(0, 255), C.A() ) );
				else
					Tex->SetPixel(x, y, eeColorA( Math::Randi(200, 255), Math::Randi(200, 255), Math::Randi(200, 255), C.A() ) );
			}
		}

		Tex->Unlock(false, true);
	}

	Cursor[0] = TF->LoadFromPack( PAK, "cursor.png" );
	CursorP[0] = TF->GetTexture( Cursor[0] );
	Cursor[1] = TF->LoadFromPack( PAK, "cursor.tga" );
	CursorP[1] = TF->GetTexture( Cursor[1] );

	cCursorManager * CurMan = mWindow->GetCursorManager();
	CurMan->Visible( false );
	CurMan->Visible( true );
	CurMan->Set( Window::Cursor::SYS_CURSOR_LINK );
	CurMan->Set( CurMan->Add( CurMan->Create( CursorP[1], eeVector2i( 1, 1 ), "cursor_special" ) ) );

	CL1.AddFrame(TN[2]);
	CL1.Position( 500, 400 );
	CL1.Scale( 0.5f );

	CL2.AddFrame(TN[0], 96, 96);
	CL2.Color( eeColorA( 255, 255, 255, 255 ) );

	int x, y, c;

	if ( cImage::GetInfo( MyPath + "extra/bnb.png", &x, &y, &c ) )
	{
		mTGL = eeNew( cTextureAtlasLoader, ( MyPath + "extra/bnb" + EE_TEXTURE_ATLAS_EXTENSION ) );
	}

	mBlindy.AddFramesByPattern( "rn" );
	mBlindy.Position( 320.f, 0.f );

	mBoxSprite = eeNew( cSprite, ( cGlobalTextureAtlas::instance()->Add( eeNew( cSubTexture, ( TN[3], "ilmare" ) ) ) ) );
	mCircleSprite = eeNew( cSprite, ( cGlobalTextureAtlas::instance()->Add( eeNew( cSubTexture, ( TN[1], "thecircle" ) ) ) ) );

	Log->Writef( "Textures loading time: %f ms.", te.Elapsed() );

	Map.Create( 100, 100, 2, 128, 64, eeColor(175,175,175) );
	RandomizeHeights();

	TreeTilingCreated = false;
	CreateTiling(Wireframe);

	Log->Writef( "Map creation time: %f ms.", te.Elapsed() );
}

void cEETest::RandomizeHeights() {
	Map.Reset();
	PerlinNoise.Octaves(7);
	PerlinNoise.Persistence(0.25f);
	PerlinNoise.Frequency(0.015f);
	PerlinNoise.Amplitude(1);

	for ( x = 0; x < static_cast<Int32>( Map.Width() ); x++ ) {
		for ( y = 0; y < static_cast<Int32>( Map.Height() ); y++ ) {
			H = PerlinNoise.PerlinNoise2D((eeFloat)x,(eeFloat)y);
			if (H < 0 ) H *= -1;

			eeFloat nh = (eeFloat)( (Int32)( H * 100 ) % 255 );
			eeFloat nf = (4 * nh / 25) / 2;
			NH = Int32(nf);

			Map.SetTileHeight( x, y, NH );
		}
	}
}

void cEETest::CreateTiling( const bool& Wire ) {
	cMTRand Rand( 0xFF00FF00 );

	for ( x = 0; x < static_cast<Int32>( Map.Width() ); x++ ) {
		for ( y = 0; y < static_cast<Int32>( Map.Height() ); y++ ) {
			if ( Wire )
				Map.Layer(x, y, 0, Tiles[6] );
			else
				Map.Layer(x, y, 0, Tiles[ Rand.RandRange( 0, 5 ) ] );

			if ( !TreeTilingCreated )
				Map.Layer(x, y, 1, 0);
		}
	}

	if ( !TreeTilingCreated ) {
		for ( x = 0; x < 100; x++ )
			Map.Layer( Rand.RandRange( 0, Map.Width() - 1 ), Rand.RandRange( 0, Map.Height() -1 ), 1, Tiles[7] );

		TreeTilingCreated = true;
	}
}

void cEETest::Run() {
	ParticlesThread();
}

void cEETest::ParticlesThread() {
	while ( mWindow->Running() ) {
		if ( MultiViewportMode || Screen == 2 ) {
			PSElapsed = (eeFloat)cElapsed.Elapsed();

			for ( Uint8 i = 0; i < PS.size(); i++ )
				PS[i].Update( PSElapsed );
		}
		Sys::Sleep(10);
	}
}

void cEETest::Screen1() {
	Map.Draw();
}

void cEETest::Screen2() {
	if ( mResLoad.IsLoaded() ) {
		cTexture * TexLoaded = TF->GetByName( "1.jpg" );

		if ( NULL != TexLoaded )
			TexLoaded->Draw( 0, 0 );
	}

	if ( KM->MouseLeftPressed() )
		TNP[3]->DrawEx( 0.f, 0.f, (eeFloat)mWindow->GetWidth(), (eeFloat)mWindow->GetHeight() );

	Batch.SetTexture( TNP[2] );
	Batch.QuadsBegin();
	Batch.QuadsSetColor( eeColorA(150,150,150,100) );
	Batch.QuadsSetSubset( 0.0f, 0.0f, 0.5f, 0.5f );

	Batch.BatchRotation( ang );
	Batch.BatchScale( scale );
	Batch.BatchCenter( eeVector2f( HWidth, HHeight ) );

	eeFloat aX = HWidth - 256.f;
	eeFloat aY = HHeight - 256.f;
	eeQuad2f TmpQuad(
		eeVector2f( aX	   , aY 		),
		eeVector2f( aX	   , aY + 32.f  ),
		eeVector2f( aX + 32.f, aY + 32.f  ),
		eeVector2f( aX + 32.f, aY 		)
	);
	TmpQuad.Rotate( ang, eeVector2f( aX + 16.f, aY + 16.f ) );

	for ( Uint32 z = 0; z < 16; z++ ) {
		for ( Uint32 y = 0; y < 16; y++ ) {
			eeFloat tmpx = (eeFloat)z * 32.f;
			eeFloat tmpy = (eeFloat)y * 32.f;

			Batch.BatchQuadFree( TmpQuad[0].x + tmpx, TmpQuad[0].y + tmpy, TmpQuad[1].x + tmpx, TmpQuad[1].y + tmpy, TmpQuad[2].x + tmpx, TmpQuad[2].y + tmpy, TmpQuad[3].x + tmpx, TmpQuad[3].y + tmpy );
		}
	}

	Batch.Draw();

	Batch.BatchRotation( 0.0f );
	Batch.BatchScale( 1.0f );
	Batch.BatchCenter( eeVector2f( 0, 0 ) );

	eeFloat PlanetX = HWidth  - TNP[6]->Width() * 0.5f;
	eeFloat PlanetY = HHeight - TNP[6]->Height() * 0.5f;

	ang+=et * 0.1f;
	ang = (ang>=360) ? 0 : ang;

	if (scale>=1.5f) {
		scale = 1.5f;
		side = true;
	} else if (scale<=0.5f) {
		side = false;
		scale = 0.5f;
	}
	scale = (!side) ? scale+et * 0.00025f : scale-et * 0.00025f;

	if ( mUseShaders ) {
		mBlurFactor = ( 1.5f * 0.01f ) - ( scale * 0.01f );
		mShaderProgram->Bind();
		mShaderProgram->SetUniform( "blurfactor" , (float)mBlurFactor );
	}

	TNP[6]->DrawFast( PlanetX, PlanetY, ang, scale);

	if ( mUseShaders )
		mShaderProgram->Unbind();

	TNP[3]->Draw( HWidth - 128, HHeight, 0, 1, eeColorA(255,255,255,150), ALPHA_NORMAL, RN_ISOMETRIC);
	TNP[3]->Draw( HWidth - 128, HHeight - 128, 0, 1, eeColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRIC);
	TNP[3]->Draw( HWidth - 128, HHeight, 0, 1, eeColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRICVERTICAL);
	TNP[3]->Draw( HWidth, HHeight, 0, 1, eeColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRICVERTICALNEGATIVE);

	alpha = (!aside) ? alpha+et * 0.1f : alpha-et * 0.1f;
	if (alpha>=255) {
		aside = true;
		alpha = 255;
	} else if (alpha<=0) {
		alpha = 0;
		aside = false;
	}

	eeColorA Col(255,255,255,(int)alpha);
	TNP[1]->DrawEx( (eeFloat)mWindow->GetWidth() - 128.f, (eeFloat)mWindow->GetHeight() - 128.f, 128.f, 128.f, ang, 1, Col, Col, Col, Col, ALPHA_BLENDONE, RN_FLIPMIRROR);

	SP.Position( alpha, alpha );
	SP.Draw();

	#ifndef EE_GLES
	CL1.RenderMode( RN_ISOMETRIC );

	if ( CL1.GetAABB().IntersectCircle( Mousef, 80.f ) )
		CL1.Color( eeColorA(255, 0, 0, 200) );
	else
		CL1.Color( eeColorA(255, 255, 255, 200) );

	if ( eePolygon2f::IntersectQuad2( CL1.GetQuad() , CL2.GetQuad() ) ) {
		CL1.Color( eeColorA(0, 255, 0, 255) );
		CL2.Color( eeColorA(0, 255, 0, 255) );
	} else
		CL2.Color( eeColorA(255, 255, 255, 255) );

	CL1.Angle(ang);
	CL1.Scale(scale * 0.5f);

	CL2.Position( (eeFloat)Mousef.x - 64.f, (eeFloat)Mousef.y + 128.f );
	CL2.Angle(-ang);

	CL1.Draw();
	CL2.Draw();

	PR.FillMode( EE_DRAW_LINE );
	PR.DrawRectangle( CL1.GetAABB() );

	PR.DrawQuad( CL1.GetQuad() );
	#endif

	Ang = Ang + mWindow->Elapsed() * 0.1f;
	if (Ang > 360.f) Ang = 1.f;

	if ( ShowParticles )
		Particles();

	PR.SetColor( eeColorA(0, 255, 0, 50) );

	eeLine2f Line( eeVector2f(0.f, 0.f), eeVector2f( (eeFloat)mWindow->GetWidth(), (eeFloat)mWindow->GetHeight() ) );
	eeLine2f Line2( eeVector2f(Mousef.x - 80.f, Mousef.y - 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y + 80.f) );
	eeLine2f Line3( eeVector2f((eeFloat)mWindow->GetWidth(), 0.f), eeVector2f( 0.f, (eeFloat)mWindow->GetHeight() ) );
	eeLine2f Line4( eeVector2f(Mousef.x - 80.f, Mousef.y + 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y - 80.f) );

	if ( Line.Intersect( Line2 ) )
		iL1 = true;
	else
		iL1 = false;

	if ( Line3.Intersect( Line4 ) )
		iL2 = true;
	else
		iL2 = false;

	if (iL1 && iL2)
		PR.SetColor( eeColorA(255, 0, 0, 255) );
	else if (iL1)
		PR.SetColor( eeColorA(0, 0, 255, 255) );
	else if (iL2)
		PR.SetColor( eeColorA(255, 255, 0, 255) );

	PR.FillMode( EE_DRAW_LINE );
	PR.DrawCircle( eeVector2f( Mousef.x, Mousef.y ), 80.f, (Uint32)(Ang/3) );

	PR.DrawTriangle( eeTriangle2f( eeVector2f( Mousef.x, Mousef.y - 10.f ), eeVector2f( Mousef.x - 10.f, Mousef.y + 10.f ), eeVector2f( Mousef.x + 10.f, Mousef.y + 10.f ) ) );
	PR.DrawLine( eeLine2f( eeVector2f(Mousef.x - 80.f, Mousef.y - 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y + 80.f) ) );
	PR.DrawLine( eeLine2f( eeVector2f(Mousef.x - 80.f, Mousef.y + 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y - 80.f) ) );
	PR.DrawLine( eeLine2f( eeVector2f((eeFloat)mWindow->GetWidth(), 0.f), eeVector2f( 0.f, (eeFloat)mWindow->GetHeight() ) ) );
	PR.DrawQuad( eeQuad2f( eeVector2f(0.f, 0.f), eeVector2f(0.f, 100.f), eeVector2f(150.f, 150.f), eeVector2f(200.f, 150.f) ), eeColorA(220, 240, 0, 125), eeColorA(100, 0, 240, 125), eeColorA(250, 50, 25, 125), eeColorA(50, 150, 150, 125) );
	PR.DrawRectangle( eeRectf( eeVector2f( Mousef.x - 80.f, Mousef.y - 80.f ), eeSizef( 160.f, 160.f ) ), 45.f );
	PR.DrawLine( eeLine2f( eeVector2f(0.f, 0.f), eeVector2f( (eeFloat)mWindow->GetWidth(), (eeFloat)mWindow->GetHeight() ) ) );

	TNP[3]->DrawQuadEx( eeQuad2f( eeVector2f(0.f, 0.f), eeVector2f(0.f, 100.f), eeVector2f(150.f, 150.f), eeVector2f(200.f, 150.f) ), 0.0f, 0.0f, ang, scale, eeColorA(220, 240, 0, 125), eeColorA(100, 0, 240, 125), eeColorA(250, 50, 25, 125), eeColorA(50, 150, 150, 125) );

	WP.Update( et );
	PR.SetColor( eeColorA(0, 255, 0, 255) );
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
	AnimVal = (!AnimSide) ? AnimVal+et * 0.1f : AnimVal-et * 0.1f;

	Batch.SetTexture( TNP[3] );
	Batch.LineLoopBegin();
	for ( eeFloat j = 0; j < 360; j++ ) {
		Batch.BatchLineLoop( HWidth + 350 * Math::sinAng(j), HHeight + 350 * Math::cosAng(j), HWidth + AnimVal * Math::sinAng(j+1), HHeight + AnimVal * Math::cosAng(j+1) );
	}
	Batch.Draw();
}

void cEETest::Screen4() {
	if ( NULL != mFBO ) {
		mFBO->Bind();
		mFBO->Clear();

		mBlindy.Position( 128-16, 128-16 );
		mBlindy.Draw();

        if ( NULL != mVBO ) {
            mVBO->Bind();
            mVBO->Draw();
            mVBO->Unbind();
        }

		mFBOText.Draw( 128.f - (eeFloat)(Int32)( mFBOText.GetTextWidth() * 0.5f ), 25.f - (eeFloat)(Int32)( mFBOText.GetTextHeight() * 0.5f ), FONT_DRAW_CENTER );

		mFBO->Unbind();

		if ( NULL != mFBO->GetTexture() ) {
			mFBO->GetTexture()->Draw( (eeFloat)mWindow->GetWidth() * 0.5f - (eeFloat)mFBO->GetWidth() * 0.5f, (eeFloat)mWindow->GetHeight() * 0.5f - (eeFloat)mFBO->GetHeight() * 0.5f, Ang );
			cGlobalBatchRenderer::instance()->Draw();
		}
	}
}

void cEETest::Screen5() {
}

void cEETest::Render() {
	mResLoad.Update();

	HWidth = mWindow->GetWidth() * 0.5f;
	HHeight = mWindow->GetHeight() * 0.5f;

	if ( Sys::GetTicks() - lasttick >= 50 ) {
		lasttick = Sys::GetTicks();
		#ifdef EE_DEBUG
		mInfo = String::StrFormated( "EE - FPS: %d Elapsed Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s\nApp Memory Usage: %s\nApp Peak Memory Usage: %s",
							mWindow->FPS(),
							et,
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							FileSystem::SizeToString( TF->MemorySize() ).c_str(),
							FileSystem::SizeToString( (Uint32)MemoryManager::GetTotalMemoryUsage() ).c_str(),
							FileSystem::SizeToString( (Uint32)MemoryManager::GetPeakMemoryUsage() ).c_str()
						);
		#else
		mInfo = String::StrFormated( "EE - FPS: %d Elapsed Time: %4.2f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s",
							mWindow->FPS(),
							et,
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
		Mousef = eeVector2f( (eeFloat)Mouse.x, (eeFloat)Mouse.y );
		Screen2();

		mWindow->SetView( Views[0] );
		Mouse = KM->GetMousePosFromView( Views[0] );
		Mousef = eeVector2f( (eeFloat)Mouse.x, (eeFloat)Mouse.y );
		Screen1();

		mWindow->SetView( mWindow->GetDefaultView() );
		mWindow->ClipEnable( (Int32)HWidth - 320, (Int32)HHeight - 240, 640, 480 );
		Screen3();
		mWindow->ClipDisable();
	}

	eeColorA ColRR1( 150, 150, 150, 220 );
	eeColorA ColRR4( 150, 150, 150, 220 );
	eeColorA ColRR2( 100, 100, 100, 220 );
	eeColorA ColRR3( 100, 100, 100, 220 );

	PR.SetColor( eeColorA(150, 150, 150, 220) );
	PR.FillMode( EE_DRAW_FILL );
	PR.DrawRectangle(
				eeRectf(
					eeVector2f(
						0.f,
						(eeFloat)mWindow->GetHeight() - mEEText.GetTextHeight()
					),
					eeVector2f(
						mEEText.GetTextWidth(),
						mEEText.GetTextHeight()
					)
				),
				ColRR1, ColRR2, ColRR3, ColRR4
	);

	mEEText.Draw( 0.f, (eeFloat)mWindow->GetHeight() - mEEText.GetTextHeight(), FONT_DRAW_CENTER );

	mInfoText.Draw( 6.f, 6.f );

	Uint32 NLPos = 0;
	Uint32 LineNum = InBuf.GetCurPosLinePos( NLPos );
	if ( InBuf.CurPos() == (eeInt)InBuf.Buffer().size() && !LineNum ) {
		FF2->Draw( "_", 6.f + FF2->GetTextWidth(), 180.f );
	} else {
		FF2->SetText( InBuf.Buffer().substr( NLPos, InBuf.CurPos() - NLPos ) );
		FF2->Draw( "_", 6.f + FF2->GetTextWidth(), 180.f + (eeFloat)LineNum * (eeFloat)FF2->GetFontHeight() );
	}

	FF2->SetText( "FPS: " + String::ToStr( mWindow->FPS() ) );
	FF2->Draw( mWindow->GetWidth() - FF2->GetTextWidth() - 15, 0 );

	FF2->SetText( InBuf.Buffer() );
	FF2->Draw( 6, 180, FONT_DRAW_SHADOW );

	cUIManager::instance()->Update();
	cUIManager::instance()->Draw();

	Con.Draw();
}

void cEETest::Input() {
	KM->Update();
	JM->Update();

	Mouse = KM->GetMousePos();
	Mousef = eeVector2f( (eeFloat)Mouse.x, (eeFloat)Mouse.y );

	if ( KM->IsKeyUp( KEY_F1 ) )
		Graphics::cShaderProgramManager::instance()->Reload();

	if ( !mWindow->Visible() ) {
		mWasMinimized = true;

		mWindow->FrameRateLimit( 10 );

		if ( mMusEnabled && Mus->State() == cSound::Playing )
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

		if ( mMusEnabled && Mus->State() == cSound::Paused )
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

	#ifdef EE_PLATFORM_TOUCH
	std::list<cInputFinger*> Fingers = KM->GetFingersDown();
	std::list<cInputFinger*> FingersDown = KM->GetFingersWasDown();

	if ( Fingers.size() == 1 && FingersDown.size() == 1 ) {
		ShowMenu();
	}
	#endif

	cJoystick * Joy = JM->GetJoystick(0);

	if ( mJoyEnabled && NULL != Joy ) {
		if ( Joy->IsButtonDown(0) )		KM->InjectButtonPress(EE_BUTTON_LEFT);
		if ( Joy->IsButtonDown(1) )		KM->InjectButtonPress(EE_BUTTON_RIGHT);
		if ( Joy->IsButtonDown(2) )		KM->InjectButtonPress(EE_BUTTON_MIDDLE);
		if ( Joy->IsButtonUp(0) )		KM->InjectButtonRelease(EE_BUTTON_LEFT);
		if ( Joy->IsButtonUp(1) )		KM->InjectButtonRelease(EE_BUTTON_RIGHT);
		if ( Joy->IsButtonUp(2) )		KM->InjectButtonRelease(EE_BUTTON_WHEELUP);
		if ( Joy->IsButtonUp(3) )		KM->InjectButtonRelease(EE_BUTTON_WHEELDOWN);
		if ( Joy->IsButtonUp(4) )		SetScreen( 0 );
		if ( Joy->IsButtonUp(5) )		SetScreen( 1 );
		if ( Joy->IsButtonUp(6) )		SetScreen( 2 );
		if ( Joy->IsButtonUp(7) )		KM->InjectButtonRelease(EE_BUTTON_MIDDLE);

		eeFloat aX = Joy->GetAxis( AXIS_X );
		eeFloat aY = Joy->GetAxis( AXIS_Y );

		if ( 0 != aX || 0 != aY ) {
			eeFloat rE = mWindow->Elapsed();
			mAxisX += aX * rE;
			mAxisY += aY * rE;
		}

		if ( ( mAxisX != 0 && ( mAxisX >= 1.f || mAxisX <= -1.f ) ) || ( mAxisY != 0 && ( mAxisY >= 1.f || mAxisY <= -1.f )  ) ) {
			eeFloat nmX = Mousef.x + mAxisX;
			eeFloat nmY = Mousef.y + mAxisY;

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
					Map.Move( (mWindow->Elapsed() * 0.2f), 0 );

				if ( HAT_RIGHT == hat || HAT_RIGHTDOWN == hat || HAT_RIGHTUP == hat )
					Map.Move( -mWindow->Elapsed() * 0.2f, 0 );

				if ( HAT_UP == hat || HAT_LEFTUP == hat || HAT_RIGHTUP == hat )
					Map.Move( 0, (mWindow->Elapsed() * 0.2f) );

				if ( HAT_DOWN == hat || HAT_LEFTDOWN == hat || HAT_RIGHTDOWN == hat )
					Map.Move( 0, -mWindow->Elapsed() * 0.2f );
			}

			if ( KM->IsKeyDown(KEY_LEFT) ) {
				Map.Move( mWindow->Elapsed() * 0.2f, 0 );
			}

			if ( KM->IsKeyDown(KEY_RIGHT) ) {
				Map.Move( -mWindow->Elapsed() * 0.2f, 0 );
			}

			if ( KM->IsKeyDown(KEY_UP) ) {
				Map.Move( 0, mWindow->Elapsed() * 0.2f );
			}

			if ( KM->IsKeyDown(KEY_DOWN) ) {
				Map.Move( 0, -mWindow->Elapsed() * 0.2f );
			}

			if ( KM->IsKeyDown(KEY_KP_MINUS) )
				Map.BaseLight().Radius( Map.BaseLight().Radius() - mWindow->Elapsed() * 0.2f );

			if ( KM->IsKeyDown(KEY_KP_PLUS) )
				Map.BaseLight().Radius( Map.BaseLight().Radius() + mWindow->Elapsed() * 0.2f );

			if ( KM->IsKeyUp(KEY_F6) ) {
				Wireframe = !Wireframe;
				Sys::Sleep(1);
				CreateTiling(Wireframe);
			}

			if ( KM->IsKeyUp(KEY_F7) )
				Map.DrawFont( !Map.DrawFont() );

			if ( KM->IsKeyUp(KEY_F8) )
				Map.Reset();

			if ( KM->IsKeyUp(KEY_F9) )
				RandomizeHeights();

			if ( KM->MouseLeftClick() ) {
				eeVector2i P = Map.GetMouseTilePos();

				if ( NULL != mTerrainBut ) {
					if ( !mTerrainBut->GetPolygon().PointInside( KM->GetMousePosf() ) ) {
						if ( mTerrainUp ) {
							Map.SetTileHeight( P.x, P.y );
						} else {
							Map.SetTileHeight( P.x, P.y, 1, false );
						}
					}
				} else {
					if ( mTerrainUp ) {
						Map.SetTileHeight( P.x, P.y );
					} else {
						Map.SetTileHeight( P.x, P.y, 1, false );
					}
				}
			}

			if ( KM->MouseRightClick() ) {
				eeVector2i P = Map.GetMouseTilePos();
				Map.SetTileHeight( P.x, P.y, 1, false );
			}
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

void cEETest::Process() {
	Init();

	if ( run ) {
		do {
			et = mWindow->Elapsed();

			Input();

			if ( mFontLoader.IsLoaded() ) {
				Render();
			} else {
				mFontLoader.Update();
			}

			if ( KM->IsKeyUp(KEY_F12) ) mWindow->TakeScreenshot( MyPath + "screenshots/" ); //After render and before Display

			mWindow->Display();
		} while( mWindow->Running() );
	}

	End();
}

void cEETest::ParticlesCallback( cParticle * P, cParticleSystem * Me ) {
	eeFloat x, y, radio;
	eeVector2f MePos( Me->Position() );

	radio = (Math::Randf(1.f, 1.2f) + sin( 20.0f / P->Id() )) * 24;
	x = MePos.x + radio * cos( (eeFloat)P->Id() );
	y = MePos.y + radio * sin( (eeFloat)P->Id() );
	P->Reset(x, y, Math::Randf(-10.f, 10.f), Math::Randf(-10.f, 10.f), Math::Randf(-10.f, 10.f), Math::Randf(-10.f, 10.f));
	P->Color( eeColorAf(1.f, 0.6f, 0.3f, 1.f), 0.02f + Math::Randf() * 0.3f );
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

	eeFloat hw = mWindow->GetWidth() / 2;

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
		cInputFinger * Finger = KM->GetFingerIndex(i);
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
	eeSAFE_DELETE( PAK );
	eeSAFE_DELETE( PakTest );

	cLog::instance()->Save();

	cEngine::DestroySingleton();
}

}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	Demo_Test::cEETest * Test = eeNew( Demo_Test::cEETest, () );

	Test->Process();

	eeDelete( Test );

	EE::MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
