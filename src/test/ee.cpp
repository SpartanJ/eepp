#include "../ee.h"

/**
@TODO Create a basic UI system ( add basic controls, add skinning support ).
@TODO Add some Surface Grid class, to create special effects ( waved texture, and stuff like that ).
@TODO Add Scripting support ( lua or squirrel ).
@TODO Add 2D physics support ( Box2D or Chipmunk wrapper ).
@TODO Encapsulate SDL and OpenGL ( and remove unnecessary dependencies ).
@TODO Support color cursors ( not only black and white cursors, that really sucks ) - Imposible with SDL 1.2
*/

class cUITest : public cUIControlAnim {
	public:
		cUITest( cUIControlAnim::CreateParams& Params ) : cUIControlAnim( Params ) 	{ mOldColor = mBackground->Colors(); }

		virtual Uint32 OnMouseEnter( const eeVector2i& Pos, const Uint32 Flags )	{
			if ( 4 == mOldColor.size() ) {
				mBackground->Colors( eeColorA( mOldColor[0].R(), mOldColor[0].G(), mOldColor[0].B(), 200 ),
									eeColorA( mOldColor[1].R(), mOldColor[1].G(), mOldColor[1].B(), 200 ),
									eeColorA( mOldColor[2].R(), mOldColor[2].G(), mOldColor[2].B(), 200 ),
									eeColorA( mOldColor[3].R(), mOldColor[3].G(), mOldColor[3].B(), 200 )
								);
			} else {
				mBackground->Color( eeColorA( mOldColor[0].R(), mOldColor[0].G(), mOldColor[0].B(), 200 ) );
			}

			return 1;
		}

		virtual Uint32 OnMouseExit( const eeVector2i& Pos, const Uint32 Flags )	{
			if ( 4 == mOldColor.size() ) {
				mBackground->Colors( mOldColor[0], mOldColor[1], mOldColor[2], mOldColor[3] );
			} else {
				mBackground->Color( mOldColor[0] );
			}

			return 1;
		}

		virtual Uint32 OnMouseUp( const eeVector2i& Pos, const Uint32 Flags ) {
			cUIDragable::OnMouseUp( Pos, Flags );

			if ( cInput::instance()->MouseWheelUp() )
				Scale( Scale() + 0.1f );
			else if ( cInput::instance()->MouseWheelDown() )
				Scale( Scale() - 0.1f );

			return 1;
		}

		const std::vector<eeColorA>& OldColor() { return mOldColor; }
	protected:
		std::vector<eeColorA> mOldColor;
};


class cEETest : private cThread {
	public:
		typedef cb::Callback0<void> SceneCb;

		void Init();
		void End();
		void Process();
		void Render();
		void Input();
		void ParticlesCallback(cParticle* P, cParticleSystem* Me);

		void ParticlesThread();
		void Particles();
		void LoadTextures();
		void CmdSetPartsNum ( const std::vector < std::wstring >& params );

		std::vector<cParticleSystem, eeAllocator< cParticleSystem > > PS;

		cTimeElapsed cElapsed;
		eeFloat PSElapsed;
	private:
		cEngine* EE;
		cTextureFactory* TF;
		cLog* Log;
		cInput* KM;
		cInputTextBuffer InBuf;

		bool run, side, aside;
		eeFloat ang, scale, alpha, Ang;
		eeFloat et;
		Int32 x, y;
		Uint32 lasttick;

		std::vector<Uint32> TN;
		std::vector<cTexture *> TNP;

		std::vector<Uint32> Tiles;

		eeVector2i Mouse;
		eeVector2f Mousef;

		cSprite SP;
		cSprite CL1, CL2;
		cTextureFont * FF;
		cTextureFont * FF2;
		cTTFFont * TTF;
		cTTFFont * TTFB;

		cPrimitives PR;
		bool iL1, iL2;
		eeFloat HWidth, HHeight;

		cMusic Mus;
		cSoundManager SndMng;

		bool DrawBack;

		cConsole Con;
		virtual void Run();

		eeVector2f Point;

		std::string MyPath;
		bool ShowParticles;

		cIsoMap Map;

		bool Wireframe;

		void CreateTiling( const bool& Wire );
		void RandomizeHeights();
		cPerlinNoise PerlinNoise;
		bool TreeTilingCreated;

		eeFloat H;
		Int32 NH;

		Uint8 Screen;
		SceneCb Scenes[3];
		void Screen1();
		void Screen2();
		void Screen3();

		cZip PAK;
		cZip PakTest;

		std::vector<Uint8> tmpv;
		std::vector<Uint8> MySong;

		cWaypoints WP;
		Int32 PartsNum;
		Uint32 Cursor[2];
		cTexture * CursorP[2];
		std::string mInfo;

		bool MultiViewportMode;

		cBatchRenderer Batch;
		eeFloat AnimVal;
		bool AnimSide;

		cView Views[2];

		cShaderProgram * mShaderProgram;
    	eeFloat mBlurFactor;
    	bool mUseShaders;

    	Uint32 mLastFPSLimit;
    	bool mWasMinimized;

		eeInt mWidth;
		eeInt mHeight;

		std::wstring mBuda;
		cTextCache mBudaTC;

		bool mTextureLoaded;
		cResourceLoader mResLoad;
		void OnTextureLoaded( cResourceLoader * ObjLoaded );

		void CreateUI();
		void CreateShaders();

		void LoadFonts();

		bool mFontsLoaded;
		cResourceLoader mFontLoader;
		void OnFontLoaded( cResourceLoader * ObjLoaded );

		cJoystickManager * JM;
		eeFloat mAxisX;
		eeFloat mAxisY;

		cTextureGroupLoader * mTGL;
		cSprite mBlindy;
		cSprite * mBlindyPtr;

		cFrameBuffer * mFB;
		cVertexBuffer * mVBO;

		void ItemClick( const cUIEvent * Event );
		void MainClick( const cUIEvent * Event );
		void QuitClick( const cUIEvent * Event );
		void ButtonClick( const cUIEvent * Event );
		void OnValueChange( const cUIEvent * Event );
		void CreateAquaTextureAtlas();

		cUIControlAnim * C;
		cUIScrollBar * mScrollBar;
		cUITextBox * mTextBoxValue;
		cUISlider * mSlider;
		cUIProgressBar * mProgressBar;
		cUIListBox * mListBox;
		cUIPopUpMenu * Menu;

		cTextCache mEEText;
		cTextCache mFBOText;
		cTextCache mInfoText;
};

void cEETest::CreateAquaTextureAtlas() {
	std::string Path( MyPath + "data/aqua" );

	if ( !FileExists( Path + ".etg" ) ) {
		cTexturePacker tp( 512, 512, true, 2 );
		tp.AddTexturesPath( Path );
		tp.PackTextures();
		tp.Save( Path + ".png", EE_SAVE_TYPE_PNG );
	} else {
		cTextureGroupLoader tgl;
		tgl.UpdateTextureAtlas( Path + ".etg", Path );
	}
}

void cEETest::Init() {
	EE = cEngine::instance();

	Screen 				= 0;
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

	mFontsLoaded		= false;
	mTextureLoaded		= false;
	mAxisX				= 0;
	mAxisY				= 0;

	MyPath 				= AppPath();

	cIniFile Ini( MyPath + "data/ee.ini" );
	Ini.ReadFile();

	mWidth 			= Ini.GetValueI( "EEPP", "Width", 800 );
	mHeight 		= Ini.GetValueI( "EEPP", "Height", 600 );
	int BitColor 	= Ini.GetValueI( "EEPP", "BitColor", 32);
	bool Windowed 	= Ini.GetValueB( "EEPP", "Windowed", true );
	bool Resizeable = Ini.GetValueB( "EEPP", "Resizeable", true );
	bool VSync 		= Ini.GetValueB( "EEPP", "VSync", true );
	PartsNum 		= Ini.GetValueI( "EEPP", "ParticlesNum", 1000);
	mUseShaders 	= Ini.GetValueB( "EEPP", "UseShaders", false );

	run = EE->Init(mWidth, mHeight, BitColor, Windowed, Resizeable, VSync);

	PAK.Open( MyPath + "data/ee.zip" );

	run = ( run && PAK.IsOpen() );

	if ( run ) {
		EE->SetWindowCaption( "EE++ Test Application" );
		TF = cTextureFactory::instance();
		TF->Allocate(40);

		Log = cLog::instance();
		KM = cInput::instance();
		JM = cJoystickManager::instance();

		PS.resize(5);

		Scenes[0] = cb::Make0( this, &cEETest::Screen1 );
		Scenes[1] = cb::Make0( this, &cEETest::Screen2 );
		Scenes[2] = cb::Make0( this, &cEETest::Screen3 );

		InBuf.Start();

		SetRandomSeed();

		LoadTextures();

		LoadFonts();

		CreateShaders();

		if ( Mus.OpenFromPack( &PAK, "music.ogg" ) ) {
			Mus.Loop(true);
			//Mus.Volume( 0.f );
			Mus.Play();
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

		Batch.AllocVertexs( 1024 );
		Batch.SetPreBlendFunc( ALPHA_BLENDONE );

		mFB = cFrameBuffer::CreateNew( 256, 256, false );

		if ( NULL != mFB )
			mFB->ClearColor( eeColorAf( 0, 0, 0, 0.5f ) );


		eePolygon2f Poly = CreateRoundedPolygon( 0.f, 0.f, 256.f, 50.f );

		mVBO = cVertexBuffer::Create( VERTEX_FLAG_GET( VERTEX_FLAG_POSITION ) | VERTEX_FLAG_GET( VERTEX_FLAG_COLOR ), DM_POLYGON );

		for ( Uint32 i = 0; i < Poly.Size(); i++ ) {
			mVBO->AddVertex( Poly[i] );
			mVBO->AddColor( eeColorA( 100 + i, 255 - i, 150 + i, 200 ) );
		}

		mVBO->Compile();

		Launch();
	} else {
		std::cout << "Failed to start EE++" << std::endl;
		cEngine::DestroySingleton();
		exit(0);
	}
}

void cEETest::LoadFonts() {
	mFontLoader.Add( eeNew( cTextureFontLoader, ( "conchars", eeNew( cTextureLoader, ( &PAK, "conchars.png", false, eeRGB(0,0,0) ) ), (eeUint)32 ) ) );
	mFontLoader.Add( eeNew( cTextureFontLoader, ( "ProggySquareSZ", eeNew( cTextureLoader, ( &PAK, "ProggySquareSZ.png" ) ), &PAK, "ProggySquareSZ.dat" ) ) );
	mFontLoader.Add( eeNew( cTTFFontLoader, ( "arial", &PAK, "arial.ttf", 12, EE_TTF_STYLE_NORMAL, false, 256, eeColor(255,255,255) ) ) );
	mFontLoader.Add( eeNew( cTTFFontLoader, ( "arialb", &PAK, "arial.ttf", 12, EE_TTF_STYLE_NORMAL, false, 256, eeColor(255,255,255), 1, eeColor(0,0,0), true ) ) );

	mFontLoader.Load( cb::Make1( this, &cEETest::OnFontLoaded ) );
}

void cEETest::OnFontLoaded( cResourceLoader * ObjLoaded ) {
	FF 		= reinterpret_cast<cTextureFont*> ( cFontManager::instance()->GetByName( "conchars" ) );
	FF2 	= reinterpret_cast<cTextureFont*> ( cFontManager::instance()->GetByName( "ProggySquareSZ" ) );
	TTF 	= reinterpret_cast<cTTFFont*> ( cFontManager::instance()->GetByName( "arial" ) );
	TTFB 	= reinterpret_cast<cTTFFont*> ( cFontManager::instance()->GetByName( "arialb" ) );

	eeASSERT( TTF != NULL );
	eeASSERT( TTFB != NULL );

	Con.Create( FF, true );
	Con.IgnoreCharOnPrompt( 186 ); // L'º'

	CreateUI();

	EE->Display();

	mFontsLoaded = true;
}

void cEETest::CreateShaders() {
	mUseShaders = mUseShaders && EE->ShadersSupported();

	mShaderProgram = NULL;

	if ( mUseShaders ) {
		mBlurFactor = 0.01f;
		mShaderProgram = eeNew( cShaderProgram, ( &PAK, "shader/blur.vert", "shader/blur.frag" ) );
	}
}

void cEETest::CreateUI() {
	cUIManager::instance()->Init();

	cUIControl::CreateParams Params( cUIManager::instance()->MainControl(), eeVector2i(0,0), eeSize( 530, 240 ), UI_FILL_BACKGROUND | UI_CLIP_ENABLE | UI_BORDER );

	cUIThemeManager::instance()->Add( cUITheme::LoadFromPath( MyPath + "data/aqua/", "aqua", "aqua" ) );
	CreateAquaTextureAtlas();

/*
	cTextureGroupLoader tgl( MyPath + "data/aqua.etg" );
	TF->GetByName( "data/aqua.png" )->TextureFilter( TEX_FILTER_NEAREST );
	cUIThemeManager::instance()->Add( cUITheme::LoadFromShapeGroup( cShapeGroupManager::instance()->GetByName( "aqua" ), "aqua", "aqua" ) );
*/

	cUIThemeManager::instance()->DefaultEffectsEnabled( true );
	cUIThemeManager::instance()->DefaultFont( TTF );
	cUIThemeManager::instance()->DefaultTheme( "aqua" );

	Params.Border.Width( 2 );
	Params.Border.Color( 0xFF979797 );
	Params.Background.Colors( eeColorA( 0x66EDEDED ), eeColorA( 0xCCEDEDED ), eeColorA( 0xCCEDEDED ), eeColorA( 0x66EDEDED ) );
	C = eeNew( cUITest, ( Params ) );
	C->Visible( true );
	C->Enabled( true );
	C->Pos( 320, 240 );
	C->DragEnable( true );

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

/*
	cUIGfx::CreateParams GfxParams;
	GfxParams.Parent( C );
	GfxParams.PosSet( 160, 100 );
	GfxParams.Flags |= UI_CLIP_ENABLE;
	GfxParams.Size = eeSize( 64, 64 );
	GfxParams.Shape = cGlobalShapeGroup::instance()->Add( TN[2] );
	cUIGfx * Gfx = eeNew( cUIGfx, ( GfxParams ) );
	Gfx->Angle( 45.f );
	Gfx->Visible( true );
	Gfx->Enabled( true );
	Gfx->StartAlphaAnim( 100.f, 255.f, 1000.f );
	Gfx->AlphaInterpolation()->Loop( true );
	Gfx->AlphaInterpolation()->SetTotalTime( 1000.f );
*/

	mBlindyPtr = eeNew( cSprite, () );
	mBlindyPtr->AddFramesByPattern( "gn" );

	cUISprite::CreateParams SpriteParams;
	SpriteParams.Parent( C );
	SpriteParams.PosSet( 160, 100 );
	SpriteParams.Flags |= UI_AUTO_SIZE;
	SpriteParams.Sprite = mBlindyPtr;
	cUISprite * Spr = eeNew( cUISprite, ( SpriteParams ) );
	Spr->Visible( true );
	Spr->Enabled( true );

	cUITextBox::CreateParams TextParams;
	TextParams.Parent( C );
	TextParams.PosSet( 0, 0 );
	TextParams.Size = eeSize( 320, 240 );
	TextParams.Flags = UI_VALIGN_TOP | UI_HALIGN_RIGHT;
	cUITextBox * Text = eeNew( cUITextBox, ( TextParams ) );
	Text->Visible( true );
	Text->Enabled( false );
	Text->Text( L"Turn around\nJust Turn Around\nAround!" );

	cUITextInput::CreateParams InputParams;
	InputParams.Parent( C );
	InputParams.Border.Color(0xFF979797);
	InputParams.Background.Colors( eeColorA(0x99AAAAAA), eeColorA(0x99CCCCCC), eeColorA(0x99CCCCCC), eeColorA(0x99AAAAAA) );
	InputParams.PosSet( 20, 216 );
	InputParams.Size = eeSize( 200, 22 );
	InputParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_CLIP_ENABLE | UI_AUTO_PADDING;
	cUITextInput * Input = eeNew( cUITextInput, ( InputParams ) );
	Input->Visible( true );
	Input->Enabled( true );

	cUIPushButton::CreateParams ButtonParams;
	ButtonParams.Parent( C );
	ButtonParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	ButtonParams.PosSet( 225, 216 );
	ButtonParams.Size = eeSize( 90, 22 );
	ButtonParams.SetIcon( cGlobalShapeGroup::instance()->GetByName( "aqua_button_ok" ) );
	cUIPushButton * Button = eeNew( cUIPushButton, ( ButtonParams ) );
	Button->Visible( true );
	Button->Enabled( true );
	Button->Text( L"Click Me" );
	Button->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::ButtonClick ) );

	TextParams.PosSet( 130, 20 );
	TextParams.Size = eeSize( 80, 22 );
	TextParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	cUICheckBox * Checkbox = eeNew( cUICheckBox, ( TextParams ) );
	Checkbox->Visible( true );
	Checkbox->Text( L"Check Me" );
	Checkbox->Enabled( true );

	TextParams.PosSet( 130, 40 );
	cUIRadioButton * RadioButton = eeNew( cUIRadioButton, ( TextParams ) );
	RadioButton->Visible( true );
	RadioButton->Text( L"Check Me" );
	RadioButton->Enabled( true );

	TextParams.PosSet( 130, 60 );
	RadioButton = eeNew( cUIRadioButton, ( TextParams ) );
	RadioButton->Visible( true );
	RadioButton->Text( L"Check Me 2" );
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
	LBParams.FontSelectedColor = eeColorA( 255, 255, 255, 255 );
	mListBox = eeNew( cUIListBox, ( LBParams ) );
	mListBox->Visible( true );
	mListBox->Enabled( true );

	Int32 wsize = 10000;

	if ( wsize ) {
		std::vector<std::wstring> wstr(wsize);

		for ( Int32 i = 1; i <= wsize; i++ )
			wstr[i-1] = L"Test ListBox " + toWStr(i) + L" testing it right now!";

		mListBox->AddListBoxItems( wstr );
	}

	cUIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( C );
	DDLParams.PosSet( 20, 55 );
	DDLParams.Size = eeSize( 100, 19 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	cUIDropDownList * mDropDownList = eeNew( cUIDropDownList, ( DDLParams ) );
	mDropDownList->Visible( true );
	mDropDownList->Enabled( true );

	std::vector<std::wstring> combostrs;
	combostrs.push_back( L"Plane" );
	combostrs.push_back( L"Car" );
	combostrs.push_back( L"Bus" );
	combostrs.push_back( L"Train" );

	mDropDownList->ListBox()->AddListBoxItems( combostrs );
	mDropDownList->ListBox()->SetSelected( 0 );

	cUIComboBox::CreateParams ComboParams;
	ComboParams.Parent( C );
	ComboParams.PosSet( 20, 80 );
	ComboParams.Size = eeSize( 100, 19 );
	DDLParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	cUIComboBox * mComboBox = eeNew( cUIComboBox, ( ComboParams ) );
	mComboBox->Visible( true );
	mComboBox->Enabled( true );

	mComboBox->ListBox()->AddListBoxItems( combostrs );
	mComboBox->ListBox()->SetSelected( 0 );

	cUIPopUpMenu::CreateParams MenuParams;
	MenuParams. Parent( cUIManager::instance()->MainControl() );
	MenuParams.Flags = UI_AUTO_SIZE | UI_AUTO_PADDING;
	MenuParams.Size = eeSize( 0, 200 );
	MenuParams.MinWidth = 100;
	MenuParams.MinSpaceForIcons = 16;
	MenuParams.PosSet( 0, 0 );
	MenuParams.FontSelectedColor = eeColorA( 255, 255, 255, 255 );
	MenuParams.MinRightMargin = 8;
	Menu = eeNew( cUIPopUpMenu, ( MenuParams ) );
	Menu->Add( L"New", cGlobalShapeGroup::instance()->GetByName( "aqua_button_ok" ) );
	Menu->Add( L"Open..." );
	Menu->AddSeparator();
	Menu->Add( L"Show Screen 1" );
	Menu->Add( L"Show Screen 2" );
	Menu->Add( L"Show Screen 3" );
	Menu->AddSeparator();
	Menu->AddCheckBox( L"Show Window" );
	Menu->AddCheckBox( L"Multi Viewport" );
	reinterpret_cast<cUIMenuCheckBox*> ( Menu->GetItem( L"Show Window" ) )->Active( true );

	cUIPopUpMenu * Menu3 = eeNew( cUIPopUpMenu, ( MenuParams ) );
	Menu3->Add( L"Hello World 1" );
	Menu3->Add( L"Hello World 2" );
	Menu3->Add( L"Hello World 3" );
	Menu3->Add( L"Hello World 4" );

	cUIPopUpMenu * Menu2 = eeNew( cUIPopUpMenu, ( MenuParams ) );
	Menu2->Add( L"Test 1" );
	Menu2->Add( L"Test 2" );
	Menu2->Add( L"Test 3" );
	Menu2->Add( L"Test 4" );
	Menu2->AddSubMenu( L"Hello World", NULL, Menu3 );

	Menu->AddSeparator();
	Menu->AddSubMenu( L"Teh Menuh", NULL, Menu2 ) ;

	Menu->AddSeparator();
	Menu->Add( L"Quit" );

	Menu->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cEETest::ItemClick ) );
	Menu->GetItem( L"Quit" )->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::QuitClick ) );
	cUIManager::instance()->MainControl()->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cEETest::MainClick ) );

	C->StartScaleAnim( 0.f, 1.f, 500.f, SINEOUT );
	C->StartAlphaAnim( 0.f, 255.f, 500.f );

	mBuda = L"El mono ve el pez en el agua y sufre. Piensa que su mundo es el único que existe, el mejor, el real. Sufre porque es bueno y tiene compasión, lo ve y piensa: \"Pobre se está ahogando no puede respirar\". Y lo saca, lo saca y se queda tranquilo, por fin lo salvé. Pero el pez se retuerce de dolor y muere. Por eso te mostré el sueño, es imposible meter el mar en tu cabeza, que es un balde.";
	TTFB->ShrinkText( mBuda, 400 );

	mBudaTC.Create( TTFB, mBuda, eeColorA(255,255,255,255) );
	mEEText.Create( TTFB, L"Entropia Engine++\nCTRL + 1 = Screen 1 - CTRL + 2 = Screen 2\nCTRL + 3 = Screen 3" );
	mFBOText.Create( TTFB, L"This is a VBO\nInside of a FBO" );
	mInfoText.Create( FF, L"", eeColorA(255,255,255,150) );
}

void cEETest::ItemClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const std::wstring& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( L"Show Screen 1" == txt ) {
		Screen = 0;
	} else if ( L"Show Screen 2" == txt ) {
		Screen = 1;
	} else if ( L"Show Screen 3" == txt ) {
		Screen = 2;
	} else if ( L"Show Window" == txt ) {
		cUIMenuCheckBox * Chk = reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() );

		if ( !Chk->Active() ) {
			C->StartScaleAnim( C->Scale(), 1.f, 500.f, SINEOUT );
			C->StartAlphaAnim( C->Alpha(), 255.f, 500.f );
		} else {
			C->StartScaleAnim( C->Scale(), 0.f, 500.f, SINEIN );
			C->StartAlphaAnim( C->Alpha(), 0.f, 500.f );
		}
	} else if ( L"Multi Viewport" == txt ) {
		MultiViewportMode = !MultiViewportMode;
	}
}

void cEETest::OnValueChange( const cUIEvent * Event ) {
	mTextBoxValue->Text( L"Scroll Value:\n" + toWStr( mScrollBar->Value() ) );

	mProgressBar->Progress( mScrollBar->Value() * 100.f );
}

void cEETest::QuitClick( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		EE->Running(false);
	}
}

void cEETest::MainClick( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTON_RMASK ) {
		if ( Menu->Show() ) {
			eeVector2i Pos = MouseEvent->Pos();
			cUIMenu::FixMenuPos( Pos , Menu );
			Menu->Pos( Pos );
		}
	}
}

void cEETest::ButtonClick( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( MouseEvent->Flags() & EE_BUTTONS_LRM ) {
		cUIGfx::CreateParams GfxParams;
		GfxParams.Parent( cUIManager::instance()->MainControl() );
		GfxParams.Shape = cShapeGroupManager::instance()->GetShapeByName( "aqua_button_ok" );
		cUIGfx * Gfx = eeNew( cUIGfx, ( GfxParams ) );
		Gfx->Visible( true );
		Gfx->Enabled( false );

		Gfx->StartRotation( 0, 2500, 2500 );
		Gfx->StartMovement( eeVector2i( eeRandi( 0, EE->GetWidth() ), -64 ), eeVector2i( eeRandi( 0, EE->GetWidth() ), EE->GetHeight() + 64 ), 2500 );
		Gfx->CloseFadeOut( 3500 );

		mListBox->AddListBoxItem( L"Test ListBox " + toWStr( mListBox->Count() + 1 ) + L" testing it right now!" );
	}
}

void cEETest::CmdSetPartsNum ( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = 0;

			bool Res = fromWString<Int32>( tInt, params[1] );

			if ( Res && ( tInt >= 0 && tInt <= 100000 ) ) {
				PS[2].Create(WormHole, tInt, TN[5], EE->GetWidth() * 0.5f, EE->GetHeight() * 0.5f, 32, true);
				Con.PushText( L"Wormhole Particles Number Changed to: " + toWStr(tInt) );
			} else
				Con.PushText( L"Valid parameters are between 0 and 100000 (0 = no limit)." );
		} catch (...) {
			Con.PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	}
}

void cEETest::OnTextureLoaded( cResourceLoader * ResLoaded ) {
	mTextureLoaded = true;
	SndMng.Play( "mysound" );
}

void cEETest::LoadTextures() {
	Uint32 i;

	mTextureLoaded = false;

	PakTest.Open( MyPath + "data/test.zip" );

	std::vector<std::string> files = PakTest.GetFileList();

	for ( i = 0; i < files.size(); i++ ) {
		std::string name( files[i] );

		if ( "jpg" == FileExtension( name ) ) {
			mResLoad.Add( eeNew( cTextureLoader, ( &PakTest, name ) ) );
		}
	}

	mResLoad.Add( eeNew( cSoundLoader, ( &SndMng, "mysound", &PAK, "sound.ogg" ) ) );

	mResLoad.Load( cb::Make1( this, &cEETest::OnTextureLoaded ) );

	TN.resize(12);
	TNP.resize(12);

	Tiles.resize(10);

	for ( i = 0; i <= 7; i++ ) {
		TN[i] = TF->LoadFromPack( &PAK, toStr(i+1) + ".png", ( (i+1) == 7 ) ? true : false, eeRGB(true), ( (i+1) == 4 ) ? EE_CLAMP_REPEAT : EE_CLAMP_TO_EDGE );
		TNP[i] = TF->GetTexture( TN[i] );
	}

	for ( i = 0; i <= 6; i++ ) {
		Tiles[i] = TF->LoadFromPack( &PAK, "tiles/" + toStr(i+1) + ".png", true );
	}

	Tiles[7] = TF->LoadFromPack( &PAK, "tiles/8.png", false, eeRGB(0,0,0) );

	eeInt w, h;

	for ( Int32 my = 0; my < 4; my++ )
		for( Int32 mx = 0; mx < 8; mx++ )
			SP.AddFrame( TN[4], 0, 0, 0, 0, eeRecti( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );

	PS[0].SetCallbackReset( cb::Make2( this, &cEETest::ParticlesCallback ) );
	PS[0].Create(Callback, 500, TN[5], 0, 0, 16, true);
	PS[1].Create(Heal, 250, TN[5], EE->GetWidth() * 0.5f, EE->GetHeight() * 0.5f, 16, true);

	PS[2].Create(WormHole, PartsNum, TN[5], EE->GetWidth() * 0.5f, EE->GetHeight() * 0.5f, 32, true);
	Con.AddCommand( L"setparticlesnum", cb::Make1( this, &cEETest::CmdSetPartsNum ) );

	PS[3].Create(Fire, 350, TN[5], -50.f, -50.f, 32, true);
	PS[4].Create(Fire, 350, TN[5], -50.f, -50.f, 32, true);

	cTexture * Tex = TF->GetTexture(TN[2]);

	Tex->Lock();
	w = (eeInt)Tex->Width();
	h = (eeInt)Tex->Height();

	for ( y = 0; y < h; y++) {
		for ( x = 0; x < w; x++) {
			eeColorA C = Tex->GetPixel(x, y);

			if ( C.R() > 200 && C.G() > 200 && C.B() > 200 )
				Tex->SetPixel(x, y, eeColorA( eeRandi(0, 255), eeRandi(0, 255), eeRandi(0, 255), C.A() ) );
			else
				Tex->SetPixel(x, y, eeColorA( eeRandi(200, 255), eeRandi(200, 255), eeRandi(200, 255), C.A() ) );
		}
	}
	Tex->Unlock(false, true);

	Cursor[0] = TF->LoadFromPack( &PAK, "cursor.png" );
	CursorP[0] = TF->GetTexture( Cursor[0] );
	Cursor[1] = TF->LoadFromPack( &PAK, "cursor.tga" );
	CursorP[1] = TF->GetTexture( Cursor[1] );

	EE->ShowCursor(false);

	CL1.AddFrame(TN[2]);
	CL1.UpdatePos(500,400);
	CL1.Scale(0.5f);

	CL2.AddFrame(TN[0], 96, 96);
	CL2.Color( eeRGBA( 255, 255, 255, 255 ) );

	mTGL = eeNew( cTextureGroupLoader, ( MyPath + "data/bnb/bnb.etg" ) );

	mBlindy.AddFramesByPattern( "rn" );
	mBlindy.UpdatePos( 320.f, 0.f );

	Map.myFont = reinterpret_cast<cFont*> ( &FF );

	Map.Create( 100, 100, 2, 128, 64, eeColor(175,175,175) );

	RandomizeHeights();

	TreeTilingCreated = false;
	CreateTiling(Wireframe);

	cGlobalShapeGroup::instance()->Add( eeNew( cShape, ( TF->Load( MyPath + "data/aqua/aqua_button_ok.png" ), "aqua_button_ok" ) ) );
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
	while ( cEngine::instance()->Running() ) {
		if ( MultiViewportMode || Screen == 1 ) {
			PSElapsed = (eeFloat)cElapsed.Elapsed();

			for ( Uint8 i = 0; i < PS.size(); i++ )
				PS[i].Update( PSElapsed );
		}
		eeSleep(10);
	}
}

void cEETest::Screen1() {
	Map.Draw();
}

void cEETest::Screen2() {
	if ( mTextureLoaded ) {
		cTexture * TexLoaded = TF->GetByName( "1.jpg" );

		if ( NULL != TexLoaded )
			TexLoaded->Draw( 0, 0 );
	}

	if ( KM->MouseLeftPressed() )
		TNP[3]->DrawEx( 0.f, 0.f, (eeFloat)EE->GetWidth(), (eeFloat)EE->GetHeight() );

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

	if ( mUseShaders ) {
		mBlurFactor = scale * 0.01f;
		mShaderProgram->Bind();
		mShaderProgram->SetUniform( "blurfactor" , mBlurFactor );
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
	TNP[1]->DrawEx( (eeFloat)EE->GetWidth() - 128.f, (eeFloat)EE->GetHeight() - 128.f, 128.f, 128.f, ang, 1, Col, Col, Col, Col, ALPHA_BLENDONE, RN_FLIPMIRROR);

	SP.UpdatePos(alpha,alpha);
	SP.Draw();

	CL1.SetRenderType( RN_ISOMETRIC );

	if (IntersectRectCircle( CL1.SprDestRectf(), Mousef.x, Mousef.y, 80.f ))
		CL1.Color( eeRGBA(255, 0, 0, 200) );
	else
		CL1.Color( eeRGBA(255, 255, 255, 200) );

	if ( IntersectQuad2( CL1.GetQuad() , CL2.GetQuad() ) ) {
		CL1.Color( eeRGBA(0, 255, 0, 255) );
		CL2.Color( eeRGBA(0, 255, 0, 255) );
	} else
		CL2.Color( eeRGBA(255, 255, 255, 255) );

	CL1.Angle(ang);
	CL1.Scale(scale * 0.5f);

	CL2.UpdatePos( (eeFloat)Mousef.x - 64.f, (eeFloat)Mousef.y + 128.f );
	CL2.Angle(-ang);

	CL1.Draw();
	CL2.Draw();

	PR.DrawRectangle( CL1.GetAABB(), 0.0f, 1.0f, EE_DRAW_LINE );

	PR.DrawQuad( CL1.GetQuad(), EE_DRAW_LINE );

	Ang = Ang + EE->Elapsed() * 0.1f;
	if (Ang > 360.f) Ang = 1.f;

	if (ShowParticles)
		Particles();
	PR.SetColor( eeColorA(0, 255, 0, 50) );
	if (IntersectLines( eeVector2f(0.f, 0.f), eeVector2f( (eeFloat)EE->GetWidth(), (eeFloat)EE->GetHeight() ), eeVector2f(Mousef.x - 80.f, Mousef.y - 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y + 80.f) ) )
		iL1 = true; else iL1 = false;

	if (IntersectLines( eeVector2f((eeFloat)EE->GetWidth(), 0.f), eeVector2f( 0.f, (eeFloat)EE->GetHeight() ), eeVector2f(Mousef.x - 80.f, Mousef.y + 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y - 80.f) ) )
		iL2 = true; else iL2 = false;

	if (iL1 && iL2)
		PR.SetColor( eeColorA(255, 0, 0, 255) );
	else if (iL1)
		PR.SetColor( eeColorA(0, 0, 255, 255) );
	else if (iL2)
		PR.SetColor( eeColorA(255, 255, 0, 255) );

	PR.DrawCircle(Mousef.x, Mousef.y, 80.f, (Uint32)(Ang/3), EE_DRAW_LINE);

	PR.DrawTriangle( eeVector2f( Mousef.x, Mousef.y - 10.f ), eeVector2f( Mousef.x - 10.f, Mousef.y + 10.f ), eeVector2f( Mousef.x + 10.f, Mousef.y + 10.f ), EE_DRAW_LINE );
	PR.DrawLine( eeVector2f(Mousef.x - 80.f, Mousef.y - 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y + 80.f) );
	PR.DrawLine( eeVector2f(Mousef.x - 80.f, Mousef.y + 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y - 80.f) );
	PR.DrawLine( eeVector2f((eeFloat)EE->GetWidth(), 0.f), eeVector2f( 0.f, (eeFloat)EE->GetHeight() ) );
	PR.DrawQuad( eeVector2f(0.f, 0.f), eeVector2f(0.f, 100.f), eeVector2f(150.f, 150.f), eeVector2f(200.f, 150.f), eeColorA(220, 240, 0, 125), eeColorA(100, 0, 240, 125), eeColorA(250, 50, 25, 125), eeColorA(50, 150, 150, 125) );
	PR.DrawRectangle(Mousef.x - 80.f, Mousef.y - 80.f, 160.f, 160.f, 45.f, 1.f, EE_DRAW_LINE);
	PR.DrawLine( eeVector2f(0.f, 0.f), eeVector2f( (eeFloat)EE->GetWidth(), (eeFloat)EE->GetHeight() ) );

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
		Batch.BatchLineLoop( HWidth + 350 * sinAng(j), HHeight + 350 * cosAng(j), HWidth + AnimVal * sinAng(j+1), HHeight + AnimVal * cosAng(j+1) );
	}
	Batch.Draw();
}

void cEETest::Render() {
	if ( !mTextureLoaded )
		mResLoad.Update();

	HWidth = EE->GetWidth() * 0.5f;
	HHeight = EE->GetHeight() * 0.5f;

	if ( eeGetTicks() - lasttick >= 50 ) {
		lasttick = eeGetTicks();
		#ifdef EE_DEBUG
		mInfo = StrFormated( "EE - FPS: %d Elapsed Time: %4.8f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s\nApp Memory Usage: %s\nApp Peak Memory Usage: %s",
							EE->FPS(),
							et,
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							SizeToString( TF->MemorySize() ).c_str(),
							SizeToString( (Uint32)MemoryManager::GetTotalMemoryUsage() ).c_str(),
							SizeToString( (Uint32)MemoryManager::GetPeakMemoryUsage() ).c_str()
						);
		#else
		mInfo = StrFormated( "EE - FPS: %d Elapsed Time: %4.8f\nMouse X: %d Mouse Y: %d\nTexture Memory Usage: %s",
							EE->FPS(),
							et,
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							SizeToString( TF->MemorySize() ).c_str()
						);
		#endif

		mInfoText.Text( mInfo );
	}

	if ( !MultiViewportMode ) {
		Scenes[ Screen ]();
	} else {
		Views[0].SetView( 0, 0, EE->GetWidth(), static_cast<Uint32>( HHeight ) );
		Views[1].SetView( 0, static_cast<Int32> ( HHeight ), EE->GetWidth(), static_cast<Uint32>( HHeight ) );

		EE->SetView( Views[1] );
		Mouse = KM->GetMousePosFromView( Views[1] );
		Mousef = eeVector2f( (eeFloat)Mouse.x, (eeFloat)Mouse.y );
		Screen2();

		EE->SetView( Views[0] );
		Mouse = KM->GetMousePosFromView( Views[0] );
		Mousef = eeVector2f( (eeFloat)Mouse.x, (eeFloat)Mouse.y );
		Screen1();

		EE->SetView( EE->GetDefaultView() );
		EE->ClipEnable( (Int32)HWidth - 320, (Int32)HHeight - 240, 640, 480 );
		Screen3();
		EE->ClipDisable();
	}

	eeColorA ColRR1( 150, 150, 150, 220 );
	eeColorA ColRR4( 150, 150, 150, 220 );
	eeColorA ColRR2( 100, 100, 100, 220 );
	eeColorA ColRR3( 100, 100, 100, 220 );

	PR.SetColor( eeColorA(150, 150, 150, 220) );

	PR.DrawRectangle(
					0.f,
					(eeFloat)EE->GetHeight() - (eeFloat)mEEText.GetNumLines() * (eeFloat)mEEText.GetFont()->GetFontSize(),
					mEEText.GetTextWidth(),
					mEEText.GetTextHeight(),
					ColRR1, ColRR2, ColRR3, ColRR4
	);

	mEEText.Draw( 0.f, (eeFloat)EE->GetHeight() - mEEText.GetTextHeight(), FONT_DRAW_CENTER, 1.f, Ang );

	mInfoText.Draw( 6.f, 6.f );

	mBudaTC.Draw( 5.f, 60.f );

	Uint32 NLPos = 0;
	Uint32 LineNum = InBuf.GetCurPosLinePos( NLPos );
	if ( InBuf.CurPos() == (eeInt)InBuf.Buffer().size() && !LineNum ) {
		FF2->Draw( L"_", 6.f + FF2->GetTextWidth(), 180.f );
	} else {
		FF2->SetText( InBuf.Buffer().substr( NLPos, InBuf.CurPos() - NLPos ) );
		FF2->Draw( L"_", 6.f + FF2->GetTextWidth(), 180.f + (eeFloat)LineNum * (eeFloat)FF2->GetFontSize() );
	}

	FF2->SetText( L"FPS: " + toWStr( EE->FPS() ) );
	FF2->Draw( EE->GetWidth() - FF2->GetTextWidth() - 15, 0 );

	FF2->SetText( InBuf.Buffer() );
	FF2->Draw( 6, 180, FONT_DRAW_SHADOW );

	if ( NULL != mFB ) {
		mFB->Bind();
		mFB->Clear();

		mBlindy.UpdatePos( 128-16, 128-16 );
		mBlindy.Draw();

		mVBO->Bind();
		mVBO->Draw();
		mVBO->Unbind();

		mFBOText.Draw( 128.f - (eeFloat)(Int32)( mFBOText.GetTextWidth() * 0.5f ), 25.f - (eeFloat)(Int32)( mFBOText.GetTextHeight() * 0.5f ), FONT_DRAW_CENTER );

		mFB->Unbind();

		if ( NULL != mFB->GetTexture() )
			mFB->GetTexture()->Draw( (eeFloat)EE->GetWidth() - 256.f, 240.f, Ang );
	}

	cUIManager::instance()->Update();
	cUIManager::instance()->Draw();

	Con.Draw();

	if ( Screen < 2 )
		CursorP[ Screen ]->Draw( Mousef.x, Mousef.y );
	else
		CursorP[ 1 ]->Draw( Mousef.x, Mousef.y );
}

void cEETest::Input() {
	KM->Update();
	JM->Update();

	Mouse = KM->GetMousePos();
	Mousef = eeVector2f( (eeFloat)Mouse.x, (eeFloat)Mouse.y );

	if ( !EE->WindowVisible() ) {
		mWasMinimized = true;

		EE->SetFrameRateLimit( 10 );

		if ( Mus.State() == SOUND_PLAYING )
			Mus.Pause();

	} else {
		if ( mLastFPSLimit != EE->GetFrameRateLimit() && !mWasMinimized )
			mLastFPSLimit = EE->GetFrameRateLimit();

		if ( mWasMinimized )
			mWasMinimized = false;

		EE->SetFrameRateLimit( mLastFPSLimit );

		if ( Mus.State() == SOUND_PAUSED )
			Mus.Play();
	}

	if ( KM->IsKeyDown(KEY_ESCAPE) )
		EE->Running(false);

	if ( KM->IsKeyUp(KEY_F1) )
		MultiViewportMode = !MultiViewportMode;

	if ( KM->AltPressed() && KM->IsKeyUp(KEY_M) && !Con.Active() )
		EE->MaximizeWindow();

	if ( KM->IsKeyUp(KEY_F4) )
		TF->ReloadAllTextures();

	if ( KM->AltPressed() && KM->IsKeyUp(KEY_RETURN) ) {
		if ( EE->Windowed() ) {
			EE->ChangeRes( EE->GetDeskWidth(), EE->GetDeskHeight(), false );
			KM->GrabInput(true);
		} else {
			EE->ToggleFullscreen();
			KM->GrabInput(false);
		}
	}

	if ( KM->GrabInput() ) {
		if ( KM->AltPressed() && KM->IsKeyDown(KEY_TAB) ) {
			EE->MinimizeWindow();

			if ( KM->GrabInput() )
				KM->GrabInput(false);
		}
	}

	if ( KM->ControlPressed() && KM->IsKeyUp(KEY_G) )
		KM->GrabInput(  !KM->GrabInput() );

	if ( KM->IsKeyUp(KEY_F3) || ( KM->AltPressed() && KM->IsKeyUp(KEY_C) ) || KM->IsKeyUp( KEY_WORLD_26 ) ) {
		Con.Toggle();
		if ( Con.Active() )
			InBuf.Active( false );
		else
			InBuf.Active( true );
	}

	if ( InBuf.Active() && ( ( KM->ControlPressed() && KM->IsKeyUp(KEY_V) ) || ( KM->ShiftPressed() && KM->IsKeyUp(KEY_INSERT) ) ) ) {
		std::wstring tmp = InBuf.Buffer();
		InBuf.Buffer( tmp + EE->GetClipboardTextWStr() );
	}

	if ( KM->IsKeyUp(KEY_1) && KM->ControlPressed() )
		Screen = 0;

	if ( KM->IsKeyUp(KEY_2) && KM->ControlPressed() )
		Screen = 1;

	if ( KM->IsKeyUp(KEY_3) && KM->ControlPressed() )
		Screen = 2;

	cJoystick * Joy = JM->GetJoystick(0);

	if ( NULL != Joy ) {
		if ( Joy->IsButtonDown(0) )		KM->InjectButtonPress(EE_BUTTON_LEFT);
		if ( Joy->IsButtonDown(1) )		KM->InjectButtonPress(EE_BUTTON_RIGHT);
		if ( Joy->IsButtonDown(2) )		KM->InjectButtonPress(EE_BUTTON_MIDDLE);
		if ( Joy->IsButtonUp(0) )		KM->InjectButtonRelease(EE_BUTTON_LEFT);
		if ( Joy->IsButtonUp(1) )		KM->InjectButtonRelease(EE_BUTTON_RIGHT);
		if ( Joy->IsButtonUp(2) )		KM->InjectButtonRelease(EE_BUTTON_WHEELUP);
		if ( Joy->IsButtonUp(3) )		KM->InjectButtonRelease(EE_BUTTON_WHEELDOWN);
		if ( Joy->IsButtonUp(4) )		Screen = 0;
		if ( Joy->IsButtonUp(5) )		Screen = 1;
		if ( Joy->IsButtonUp(6) )		Screen = 2;
		if ( Joy->IsButtonUp(7) )		KM->InjectButtonRelease(EE_BUTTON_MIDDLE);

		Int16 aX = Joy->GetAxis( AXIS_X );
		Int16 aY = Joy->GetAxis( AXIS_Y );

		if ( 0 != aX || 0 != aY ) {
			eeFloat rE = EE->Elapsed();

			if ( aX < 0 )	mAxisX -= ( (eeFloat)aX / (eeFloat)AXIS_MIN ) * rE;
			else 			mAxisX += ( (eeFloat)aX / (eeFloat)AXIS_MAX ) * rE;

			if ( aY < 0 )	mAxisY -= ( (eeFloat)aY / (eeFloat)AXIS_MIN ) * rE;
			else 			mAxisY += ( (eeFloat)aY / (eeFloat)AXIS_MAX ) * rE;
		}

		if ( ( mAxisX != 0 && ( mAxisX >= 1.f || mAxisX <= -1.f ) ) || ( mAxisY != 0 && ( mAxisY >= 1.f || mAxisY <= -1.f )  ) ) {
			eeFloat nmX = Mousef.x + mAxisX;
			eeFloat nmY = Mousef.y + mAxisY;

			KM->InjectMousePos( (Int32)nmX, (Int32)nmY );

			mAxisX 		= 0;
			mAxisY	 	= 0;
		}
	}

	switch (Screen) {
		case 0:
			if ( NULL != Joy ) {
				Uint8 hat = Joy->GetHat();

				if ( HAT_LEFT == hat || HAT_LEFTDOWN == hat || HAT_LEFTUP == hat )
					Map.Move( (EE->Elapsed() * 0.2f), 0 );

				if ( HAT_RIGHT == hat || HAT_RIGHTDOWN == hat || HAT_RIGHTUP == hat )
					Map.Move( -EE->Elapsed() * 0.2f, 0 );

				if ( HAT_UP == hat || HAT_LEFTUP == hat || HAT_RIGHTUP == hat )
					Map.Move( 0, (EE->Elapsed() * 0.2f) );

				if ( HAT_DOWN == hat || HAT_LEFTDOWN == hat || HAT_RIGHTDOWN == hat )
					Map.Move( 0, -EE->Elapsed() * 0.2f );
			}

			if ( KM->IsKeyDown(KEY_LEFT) )
				Map.Move( (EE->Elapsed() * 0.2f), 0 );

			if ( KM->IsKeyDown(KEY_RIGHT) )
				Map.Move( -EE->Elapsed() * 0.2f, 0 );

			if ( KM->IsKeyDown(KEY_UP) )
				Map.Move( 0, (EE->Elapsed() * 0.2f) );

			if ( KM->IsKeyDown(KEY_DOWN) )
				Map.Move( 0, -EE->Elapsed() * 0.2f );

			if ( KM->IsKeyDown(KEY_KP_MINUS) )
				Map.Light.Radio( Map.Light.Radio() - EE->Elapsed() * 0.2f );

			if ( KM->IsKeyDown(KEY_KP_PLUS) )
				Map.Light.Radio( Map.Light.Radio() + EE->Elapsed() * 0.2f );

			if ( KM->IsKeyUp(KEY_F6) ) {
				Wireframe = !Wireframe;
				eeSleep(1);
				CreateTiling(Wireframe);
			}

			if ( KM->IsKeyUp(KEY_F7) )
				Map.DrawFont = !Map.DrawFont;

			if ( KM->IsKeyUp(KEY_F8) )
				Map.Reset();

			if ( KM->IsKeyUp(KEY_F9) )
				RandomizeHeights();

			if ( KM->MouseLeftClick() ) {
				eeVector2i P = Map.GetMouseTilePos();
				Map.SetTileHeight( P.x, P.y );
			}

			if ( KM->MouseRightClick() ) {
				eeVector2i P = Map.GetMouseTilePos();
				Map.SetTileHeight( P.x, P.y, 1, false );
			}
			break;
		case 1:
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
			et = EE->Elapsed();

			Input();

			if ( mFontsLoaded )
				Render();
			else
				mFontLoader.Update();

			if ( KM->IsKeyUp(KEY_F12) ) EE->TakeScreenshot( MyPath + "data/screenshots/" ); //After render and before Display

			EE->Display();
		} while( EE->Running() );
	}

	End();
}

void cEETest::End() {
	Wait();

	Mus.Stop();

	TN.clear();
	Tiles.clear();
	tmpv.clear();
	MySong.clear();

	eeSAFE_DELETE( mTGL );
	eeSAFE_DELETE( mFB );
	eeSAFE_DELETE( mVBO );
	eeSAFE_DELETE( mBlindyPtr );

	cLog::instance()->Save();

	cEngine::DestroySingleton();
}

void cEETest::ParticlesCallback(cParticle* P, cParticleSystem* Me) {
	eeFloat x, y, radio;
	radio = (eeRandf(1.f, 1.2f) + sin( 20.0f / P->Id() )) * 24;
	x = Me->X() + radio * cos( (eeFloat)P->Id() );
	y = Me->Y() + radio * sin( (eeFloat)P->Id() );
	P->Reset(x, y, eeRandf(-10.f, 10.f), eeRandf(-10.f, 10.f), eeRandf(-10.f, 10.f), eeRandf(-10.f, 10.f));
	P->SetColor( eeColorAf(1.f, 0.6f, 0.3f, 1.f), 0.02f + eeRandf() * 0.3f );
}

void cEETest::Particles() {
	PS[0].UpdatePos(Mousef.x, Mousef.y);

	if ( DrawBack )
		PS[1].UpdatePos(Mousef.x, Mousef.y);

	PS[2].UpdatePos( HWidth, HHeight );
	PS[3].UpdatePos(  cosAng(Ang) * 220.f + HWidth + eeRandf(0.f, 10.f),  sinAng(Ang) * 220.f + HHeight + eeRandf(0.f, 10.f) );
	PS[4].UpdatePos( -cosAng(Ang) * 220.f + HWidth + eeRandf(0.f, 10.f), -sinAng(Ang) * 220.f + HHeight + eeRandf(0.f, 10.f) );

	for ( Uint8 i = 0; i < PS.size(); i++ )
		PS[i].Draw();
}

int main (int argc, char * argv []) {
	cEETest * Test = eeNew( cEETest, () );

	Test->Process();

	eeDelete( Test );

	EE::MemoryManager::LogResults();

	return 0;
}
