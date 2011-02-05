#include "../ee.h"

/**
@TODO Add some Surface Grid class, to create special effects ( waved texture, and stuff like that ).
@TODO Add Scripting support ( lua or squirrel ).
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

		virtual Uint32 OnFocus() {
			ToFront();

			return 1;
		}

		const std::vector<eeColorA>& OldColor() { return mOldColor; }
	protected:
		std::vector<eeColorA> mOldColor;
};

enum CollisionTypes {
	BALL_TYPE,
	BLOCKING_SENSOR_TYPE,
	CATCH_SENSOR_TYPE,
};

typedef struct Emitter {
	int queue;
	int blocked;
	cVect position;
} Emitter;

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

		std::vector<Graphics::cShape*> Tiles;

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

		cMusic * Mus;
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
		SceneCb Scenes[6];
		void Screen1();
		void Screen2();
		void Screen3();
		void Screen4();
		void Screen5();

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

		cFrameBuffer * mFBO;
		cVertexBuffer * mVBO;

		void ItemClick( const cUIEvent * Event );
		void MainClick( const cUIEvent * Event );
		void QuitClick( const cUIEvent * Event );
		void CloseClick( const cUIEvent * Event );
		void ButtonClick( const cUIEvent * Event );
		void OnValueChange( const cUIEvent * Event );
		void CreateDecoratedWindow();
		void CreateAquaTextureAtlas();

		cUIControlAnim * C;
		cUIScrollBar * mScrollBar;
		cUITextBox * mTextBoxValue;
		cUISlider * mSlider;
		cUIProgressBar * mProgressBar;
		cUIListBox * mListBox;
		cUIPopUpMenu * Menu;
		cUIWindow * mWindow;

		cTextCache mEEText;
		cTextCache mFBOText;
		cTextCache mInfoText;

		cSpace * mSpace;
		cBody * mMouseBody;
		cVect mMousePoint;
		cVect mMousePoint_last;
		cConstraint * mMouseJoint;
		void PhysicsCreate();
		void PhysicsUpdate();
		void PhysicsDestroy();

		void SetScreen( Uint32 num );

		cpBool blockerBegin( cArbiter *arb, cSpace *space, void *unused );
		void blockerSeparate( cArbiter *arb, cSpace *space, void *unused );
		void postStepRemove( cSpace *space, void * tshape, void *unused );
		cpBool catcherBarBegin( cArbiter *arb, cSpace *space, void *unused );

		void Demo1Create();
		void Demo1Update();
		void Demo1Destroy();

		void Demo2Create();
		void Demo2Update();
		void Demo2Destroy();

		Emitter emitterInstance;

		void ChangeDemo( Uint32 num );

		struct physicDemo {
			SceneCb init;
			SceneCb update;
			SceneCb destroy;
		};

		std::vector<physicDemo> mDemo;
		Int32					mCurDemo;
};

void cEETest::CreateAquaTextureAtlas() {
	std::string tgpath( MyPath + "data/aquatg/aqua" );
	std::string Path( MyPath + "data/aqua" );

	if ( !FileExists( tgpath + ".etg" ) ) {
		cTexturePacker tp( 512, 512, true, 2 );
		tp.AddTexturesPath( Path );
		tp.PackTextures();
		tp.Save( tgpath + ".png", EE_SAVE_TYPE_PNG );
	} else {
		cTextureGroupLoader tgl;
		tgl.UpdateTextureAtlas( tgpath + ".etg", Path );
	}
}

void cEETest::Init() {
	EE = cEngine::instance();

	SetScreen( 0 );
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
	Int32 GLVersion = Ini.GetValueI( "EEPP", "GLVersion", 2 );
	EEGL_version GLVer;

	if ( 3 == GLVersion )
		GLVer = GLv_3;
	else
		GLVer = GLv_2;

	run = EE->Init(mWidth, mHeight, BitColor, Windowed, Resizeable, VSync, true, false, false, GLVer );

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

		Scenes[0] = cb::Make0( this, &cEETest::PhysicsUpdate );
		Scenes[1] = cb::Make0( this, &cEETest::Screen1 );
		Scenes[2] = cb::Make0( this, &cEETest::Screen2 );
		Scenes[3] = cb::Make0( this, &cEETest::Screen3 );
		Scenes[4] = cb::Make0( this, &cEETest::Screen4 );
		Scenes[5] = cb::Make0( this, &cEETest::Screen5 );

		InBuf.Start();
		InBuf.SupportNewLine( true );

		SetRandomSeed();

		LoadTextures();

		LoadFonts();

		CreateShaders();

		Mus = eeNew( cMusic, () );
		if ( Mus->OpenFromPack( &PAK, "music.ogg" ) ) {
			Mus->Loop(true);
			Mus->Volume( 0.f );
			Mus->Play();
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
		Batch.SetPreBlendFunc( ALPHA_BLENDONE );

		mFBO = cFrameBuffer::CreateNew( 256, 256, false );

		if ( NULL != mFBO )
			mFBO->ClearColor( eeColorAf( 0, 0, 0, 0.5f ) );


		eePolygon2f Poly = CreateRoundedPolygon( 0.f, 0.f, 256.f, 50.f );

		mVBO = cVertexBuffer::Create( VERTEX_FLAG_GET( VERTEX_FLAG_POSITION ) | VERTEX_FLAG_GET( VERTEX_FLAG_COLOR ), DM_POLYGON );

		for ( Uint32 i = 0; i < Poly.Size(); i++ ) {
			mVBO->AddVertex( Poly[i] );
			mVBO->AddColor( eeColorA( 100 + i, 255 - i, 150 + i, 200 ) );
		}

		mVBO->Compile();

		PhysicsCreate();

		Launch();
	} else {
		std::cout << "Failed to start EE++" << std::endl;
		cEngine::DestroySingleton();
		exit(0);
	}
}

void cEETest::LoadFonts() {
	cTextureLoader * tl = eeNew( cTextureLoader, ( &PAK, "conchars.png" ) );
	tl->SetColorKey( eeColor(0,0,0) );

	mFontLoader.Add( eeNew( cTextureFontLoader, ( "conchars", tl, (eeUint)32 ) ) );
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

	Map.Font( FF );

	Con.Create( FF, true );
	Con.IgnoreCharOnPrompt( 186 ); // L'º'

	mBuda = L"El mono ve el pez en el agua y sufre. Piensa que su mundo es el único que existe, el mejor, el real. Sufre porque es bueno y tiene compasión, lo ve y piensa: \"Pobre se está ahogando no puede respirar\". Y lo saca, lo saca y se queda tranquilo, por fin lo salvé. Pero el pez se retuerce de dolor y muere. Por eso te mostré el sueño, es imposible meter el mar en tu cabeza, que es un balde.";

	cTimeElapsed TE;
	cUIManager::instance()->Init();
	CreateUI();
	cLog::instance()->Writef( "CreateUI time: %f", TE.ElapsedSinceStart() );

	mEEText.Create( TTFB, L"Entropia Engine++\nCTRL + Number to change Demo Screen" );
	mFBOText.Create( TTFB, L"This is a VBO\nInside of a FBO" );
	mInfoText.Create( FF, L"", eeColorA(255,255,255,150) );

	EE->Display();

	mFontsLoaded = true;
}

void cEETest::CreateShaders() {
	mUseShaders = mUseShaders && EE->ShadersSupported();

	mShaderProgram = NULL;

	if ( mUseShaders ) {
		mBlurFactor = 0.01f;

		if ( GLv_3 == GLi->Version() || GLv_ES2 == GLi->Version() )
			mShaderProgram = eeNew( cShaderProgram, ( MyPath + "data/shader/gl3_blur.vert", MyPath + "data/shader/gl3_blur.frag" ) );
		else
			mShaderProgram = eeNew( cShaderProgram, ( MyPath + "data/shader/blur.vert", MyPath + "data/shader/blur.frag" ) );
	}
}

void cEETest::CreateUI() {
	cUIControl::CreateParams Params( cUIManager::instance()->MainControl(), eeVector2i(0,0), eeSize( 530, 380 ), UI_FILL_BACKGROUND | UI_CLIP_ENABLE | UI_BORDER );

	//cUIThemeManager::instance()->Add( cUITheme::LoadFromPath( MyPath + "data/aqua/", "aqua", "aqua" ) );

	CreateAquaTextureAtlas();

	cTextureGroupLoader tgl( MyPath + "data/aquatg/aqua.etg" );
	TF->GetByName( "data/aquatg/aqua.png" )->TextureFilter( TEX_FILTER_NEAREST );
	cUIThemeManager::instance()->Add( cUITheme::LoadFromShapeGroup( cShapeGroupManager::instance()->GetByName( "aqua" ), "aqua", "aqua" ) );

	cUIThemeManager::instance()->DefaultEffectsEnabled( true );
	cUIThemeManager::instance()->DefaultFont( TTF );
	cUIThemeManager::instance()->DefaultTheme( "aqua" );

	Params.Border.Width( 2 );
	Params.Border.Color( 0xFF979797 );
	Params.Background.Colors( eeColorA( 0x66EDEDED ), eeColorA( 0xCCEDEDED ), eeColorA( 0xCCEDEDED ), eeColorA( 0x66EDEDED ) );
	C = eeNew( cUITest, ( Params ) );
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
	Button->TooltipText( L"Click and see what happens..." );

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

	Int32 wsize = 100;

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
	ComboParams.Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
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
	Menu->Add( L"Show Screen 4" );
	Menu->Add( L"Show Screen 5" );
	Menu->Add( L"Show Screen 6" );
	Menu->AddSeparator();
	Menu->AddCheckBox( L"Show Window" );
	Menu->Add( L"Show Window 2" );
	Menu->AddCheckBox( L"Multi Viewport" );

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
	Menu->AddSubMenu( L"Sub-Menu", NULL, Menu2 ) ;

	Menu->AddSeparator();
	Menu->Add( L"Quit" );

	Menu->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cEETest::ItemClick ) );
	Menu->GetItem( L"Quit" )->AddEventListener( cUIEvent::EventMouseUp, cb::Make1( this, &cEETest::QuitClick ) );
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
	TxtGfxParams.Shape = cGlobalShapeGroup::instance()->GetByName( "aqua_button_ok" );

	for ( Uint32 i = 0; i < 100; i++ ) {
		cUIGridCell * Cell			= eeNew( cUIGridCell, ( CellParams ) );
		cUITextBox * TxtBox			= eeNew( cUITextBox, ( TxtBoxParams ) );
		cUITextInput * TxtInput		= eeNew( cUITextInput, ( TxtInputParams ) );
		cUIGfx * TxtGfx				= eeNew( cUIGfx, ( TxtGfxParams )  );

		TxtBox->Text( L"Test " + toWStr( i+1 ) );

		Cell->Cell( 0, TxtBox );
		Cell->Cell( 1, TxtGfx );
		Cell->Cell( 2, TxtInput );

		mGenGrid->Add( Cell );
	}

	mGenGrid->CollumnWidth( 0, 50 );
	mGenGrid->CollumnWidth( 1, 24 );
	mGenGrid->CollumnWidth( 2, 100 );
/*
	//reinterpret_cast<cUIMenuCheckBox*> ( Menu->GetItem( L"Show Window" ) )->Active( true );
	C->Visible( true );
	C->Enabled( true );
	C->StartScaleAnim( 0.f, 1.f, 500.f, SINEOUT );
	C->StartAlphaAnim( 0.f, 255.f, 500.f );
	C->StartRotation( 0, 360, 500.f, SINEOUT );
*/
	C->Scale( 0 );

	CreateDecoratedWindow();
	//mWindow->Show();
}

void cEETest::CreateDecoratedWindow() {
	cUIWindow::CreateParams WinParams;
	WinParams.Flags = UI_HALIGN_CENTER;
	WinParams.WinFlags |= cUIWindow::UI_WIN_MAXIMIZE_BUTTON;
	WinParams.PosSet( 200, 50 );
	WinParams.Size = eeSize( 530, 380 );
	WinParams.ButtonsPositionFixer.x = -4;
	WinParams.ButtonsPositionFixer.y = -2;
	WinParams.BaseAlpha = 240;
	//WinParams.BorderAutoSize = false;
	//WinParams.BorderSize = eeSize( 8, 8 );

	mWindow = eeNew( cUIWindow, ( WinParams ) );
	mWindow->AddEventListener( cUIEvent::EventOnWindowCloseClick, cb::Make1( this, &cEETest::CloseClick ) );
	mWindow->Title( L"Test Window" );
	mWindow->ToBack();
}

void cEETest::CloseClick( const cUIEvent * Event ) {
	mWindow = NULL;
}

void cEETest::ItemClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const std::wstring& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( L"Show Screen 1" == txt ) {
		SetScreen( 0 );
	} else if ( L"Show Screen 2" == txt ) {
		SetScreen( 1 );
	} else if ( L"Show Screen 3" == txt ) {
		SetScreen( 2 );
	} else if ( L"Show Screen 4" == txt ) {
		SetScreen( 3 );
	} else if ( L"Show Screen 5" == txt ) {
		SetScreen( 4 );
	} else if ( L"Show Screen 6" == txt ) {
		SetScreen( 5 );
	} else if ( L"Show Window" == txt ) {
		cUIMenuCheckBox * Chk = reinterpret_cast<cUIMenuCheckBox*> ( Event->Ctrl() );

		C->Visible( true );
		C->Enabled( true );

		if ( Chk->Active() ) {
			C->StartScaleAnim( C->Scale(), 1.f, 500.f, SINEOUT );
			C->StartAlphaAnim( C->Alpha(), 255.f, 500.f );
			C->StartRotation( 0, 360, 500.f, SINEOUT );
		} else {
			C->StartScaleAnim( C->Scale(), 0.f, 500.f, SINEIN );
			C->StartAlphaAnim( C->Alpha(), 0.f, 500.f );
			C->StartRotation( 0, 360, 500.f, SINEIN );
		}
	} else if ( L"Show Window 2" == txt ) {
		if ( NULL == mWindow ) {
			CreateDecoratedWindow();
		}

		mWindow->Show();
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

void cEETest::SetScreen( Uint32 num ) {
	if ( 0 == num )
		EE->SetBackColor( eeColor( 240, 240, 240 ) );
	else
		EE->SetBackColor( eeColor( 0, 0, 0 ) );

	if ( num < 6 )
		Screen = num;
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

	for ( i = 0; i <= 7; i++ ) {
		TN[i] = TF->LoadFromPack( &PAK, "t" + toStr(i+1) + ".png", ( (i+1) == 7 ) ? true : false, ( (i+1) == 4 ) ? EE_CLAMP_REPEAT : EE_CLAMP_TO_EDGE );
		TNP[i] = TF->GetTexture( TN[i] );
	}

	Tiles.resize(10);

	cTextureGroupLoader tgl( &PAK, "tiles.etg" );
	cShapeGroup * SG = cShapeGroupManager::instance()->GetByName( "tiles" );

	for ( i = 0; i < 6; i++ ) {
		Tiles[i] = SG->GetByName( toStr( i+1 ) );
	}

	Tiles[6] = SG->Add( TF->LoadFromPack( &PAK, "objects/1.png" ), "7" );
	Tiles[7] = SG->Add( TF->LoadFromPack( &PAK, "objects/2.png" ), "8" );
	Tiles[7]->GetTexture()->CreateMaskFromColor( eeColorA(0,0,0,255), 0 );

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
	CL1.Position( 500, 400 );
	CL1.Scale( 0.5f );

	CL2.AddFrame(TN[0], 96, 96);
	CL2.Color( eeColorA( 255, 255, 255, 255 ) );

	mTGL = eeNew( cTextureGroupLoader, ( MyPath + "data/bnb/bnb.etg" ) );

	mBlindy.AddFramesByPattern( "rn" );
	mBlindy.Position( 320.f, 0.f );

	Map.Create( 100, 100, 2, 128, 64, eeColor(175,175,175) );

	RandomizeHeights();

	TreeTilingCreated = false;
	CreateTiling(Wireframe);

	cGlobalShapeGroup::instance()->Add( eeNew( Graphics::cShape, ( TF->Load( MyPath + "data/aqua/aqua_button_ok.png" ), "aqua_button_ok" ) ) );
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
		if ( MultiViewportMode || Screen == 2 ) {
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

	SP.Position( alpha, alpha );
	SP.Draw();

	CL1.SetRenderType( RN_ISOMETRIC );

	if (IntersectRectCircle( CL1.GetAABB(), Mousef.x, Mousef.y, 80.f ))
		CL1.Color( eeColorA(255, 0, 0, 200) );
	else
		CL1.Color( eeColorA(255, 255, 255, 200) );

	if ( IntersectQuad2( CL1.GetQuad() , CL2.GetQuad() ) ) {
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

	PR.DrawRectangle( CL1.GetAABB(), 0.0f, 1.0f, EE_DRAW_LINE );

	PR.DrawQuad( CL1.GetQuad(), EE_DRAW_LINE );

	Ang = Ang + EE->Elapsed() * 0.1f;
	if (Ang > 360.f) Ang = 1.f;

	if ( ShowParticles )
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

void cEETest::Screen4() {
	if ( NULL != mFBO ) {
		mFBO->Bind();
		mFBO->Clear();

		mBlindy.Position( 128-16, 128-16 );
		mBlindy.Draw();

		mVBO->Bind();
		mVBO->Draw();
		mVBO->Unbind();

		mFBOText.Draw( 128.f - (eeFloat)(Int32)( mFBOText.GetTextWidth() * 0.5f ), 25.f - (eeFloat)(Int32)( mFBOText.GetTextHeight() * 0.5f ), FONT_DRAW_CENTER );

		mFBO->Unbind();

		if ( NULL != mFBO->GetTexture() ) {
			mFBO->GetTexture()->Draw( (eeFloat)EE->GetWidth() * 0.5f - (eeFloat)mFBO->GetWidth() * 0.5f, (eeFloat)EE->GetHeight() * 0.5f - (eeFloat)mFBO->GetHeight() * 0.5f, Ang );
			cGlobalBatchRenderer::instance()->Draw();
		}
	}
}

void cEETest::Screen5() {

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
					(eeFloat)EE->GetHeight() - (eeFloat)mEEText.GetNumLines() * (eeFloat)mEEText.Font()->GetFontSize(),
					mEEText.GetTextWidth(),
					mEEText.GetTextHeight(),
					ColRR1, ColRR2, ColRR3, ColRR4
	);

	mEEText.Draw( 0.f, (eeFloat)EE->GetHeight() - mEEText.GetTextHeight(), FONT_DRAW_CENTER, 1.f, Ang );

	mInfoText.Draw( 6.f, 6.f );

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

	cUIManager::instance()->Update();
	cUIManager::instance()->Draw();

	Con.Draw();

	if ( Screen == 1 )
		CursorP[ 0 ]->Draw( Mousef.x, Mousef.y );
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

		if ( Mus->State() == SOUND_PLAYING )
			Mus->Pause();

	} else {
		if ( mLastFPSLimit != EE->GetFrameRateLimit() && !mWasMinimized )
			mLastFPSLimit = EE->GetFrameRateLimit();

		if ( mWasMinimized ) {
			mWasMinimized = false;

			if ( !EE->Windowed() )
				KM->GrabInput( true );
		}

		EE->SetFrameRateLimit( mLastFPSLimit );

		if ( Mus->State() == SOUND_PAUSED )
			Mus->Play();
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
		if ( KM->AltPressed() && KM->IsKeyDown( KEY_TAB ) ) {
			EE->MinimizeWindow();

			if ( KM->GrabInput() )
				KM->GrabInput( false );
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

	cJoystick * Joy = JM->GetJoystick(0);

	if ( NULL != Joy ) {
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
				Map.BaseLight().Radius( Map.BaseLight().Radius() - EE->Elapsed() * 0.2f );

			if ( KM->IsKeyDown(KEY_KP_PLUS) )
				Map.BaseLight().Radius( Map.BaseLight().Radius() + EE->Elapsed() * 0.2f );

			if ( KM->IsKeyUp(KEY_F6) ) {
				Wireframe = !Wireframe;
				eeSleep(1);
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
				Map.SetTileHeight( P.x, P.y );
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

	PhysicsDestroy();

	Mus->Stop();
	eeSAFE_DELETE( Mus );
	eeSAFE_DELETE( mTGL );
	eeSAFE_DELETE( mFBO );
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

#define GRABABLE_MASK_BIT (1<<31)
#define NOT_GRABABLE_MASK (~GRABABLE_MASK_BIT)

void cEETest::Demo1Create() {
	mMouseJoint	= NULL;
	mMouseBody	= eeNew( cBody, ( INFINITY, INFINITY ) );

	Physics::cShape::ResetShapeIdCounter();

	mSpace = Physics::cSpace::New();
	//mSpace->Iterations( 30 );
	//mSpace->ResizeStaticHash( 40.f, 1000 );
	//mSpace->ResizeActiveHash( 40.f, 1000 );
	mSpace->Gravity( cVectNew( 0, 100 ) );
	mSpace->SleepTimeThreshold( 0.5f );

	cBody *body, *staticBody = mSpace->StaticBody();
	Physics::cShape * shape;

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, EE->GetHeight() ), cVectNew( EE->GetWidth(), EE->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( EE->GetWidth(), 0 ), cVectNew( EE->GetWidth(), EE->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, 0 ), cVectNew( 0, EE->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, 0 ), cVectNew( EE->GetWidth(), 0 ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	eeFloat hw = EE->GetWidth() / 2;

	for(int i=0; i<14; i++){
		for(int j=0; j<=i; j++){
			body = mSpace->AddBody( cBody::New( 1.0f, Moment::ForBox( 1.0f, 30.0f, 30.0f ) ) );
			body->Pos( cVectNew( hw + j * 32 - i * 16, 100 + i * 32 ) );

			shape = mSpace->AddShape( cShapePoly::New( body, 30.f, 30.f ) );
			shape->e( 0.0f );
			shape->u( 0.8f );
		}
	}

	cpFloat radius = 15.0f;

	body = mSpace->AddBody( cBody::New( 10.0f, Moment::ForCircle( 10.0f, 0.0f, radius, cVectZero ) ) );
	body->Pos( cVectNew( hw, EE->GetHeight() - radius - 5 ) );

	shape = mSpace->AddShape( cShapeCircle::New( body, radius, cVectZero ) );
	shape->e( 0.0f );
	shape->u( 0.9f );
}

void cEETest::Demo1Update() {

}

void cEETest::Demo1Destroy() {
	eeSAFE_DELETE( mMouseBody );
	eeSAFE_DELETE( mSpace );
}

cpBool cEETest::blockerBegin( cArbiter *arb, cSpace *space, void *unused ) {
	Physics::cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->blocked++;

	return cpFalse; // Return values from sensors callbacks are ignored,
}

void cEETest::blockerSeparate( cArbiter *arb, cSpace * space, void *unused ) {
	Physics::cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->blocked--;
}

void cEETest::postStepRemove( cSpace *space, void * tshape, void * unused ) {
	Physics::cShape * shape = reinterpret_cast<Physics::cShape*>( tshape );

	if ( NULL != mMouseJoint && ( mMouseJoint->A() == shape->Body() || mMouseJoint->B() == shape->Body() ) ) {
		mSpace->RemoveConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}

	mSpace->RemoveBody( shape->Body() );
	mSpace->RemoveShape( shape );
	Physics::cShape::Free( shape, true );
}

cpBool cEETest::catcherBarBegin(cArbiter *arb, Physics::cSpace *space, void *unused) {
	Physics::cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->queue++;

	mSpace->AddPostStepCallback( cb::Make3( this, &cEETest::postStepRemove ), b, NULL );

	return cpFalse;
}

static cpFloat frand_unit(){return 2.0f*((cpFloat)rand()/(cpFloat)RAND_MAX) - 1.0f;}

void cEETest::Demo2Create() {
	mMouseJoint	= NULL;
	mMouseBody	= eeNew( cBody, ( INFINITY, INFINITY ) );

	Physics::cShape::ResetShapeIdCounter();

	mSpace = Physics::cSpace::New();
	mSpace->Iterations( 10 );
	mSpace->Gravity( cVectNew( 0, 100 ) );

	cBody * staticBody = mSpace->StaticBody();
	Physics::cShape * shape;

	emitterInstance.queue = 5;
	emitterInstance.blocked = 0;
	emitterInstance.position = cVectNew( EE->GetWidth() / 2 , 150);

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
		body->Vel( cVectNew( frand_unit(), frand_unit() ) * (cpFloat)100 );

		Physics::cShape *shape = mSpace->AddShape( cShapeCircle::New( body, 15.0f, cVectZero ) );
		shape->CollisionType( BALL_TYPE );
	}
}

void cEETest::Demo2Destroy() {
	eeSAFE_DELETE( mMouseBody );
	eeSAFE_DELETE( mSpace );
}

void cEETest::ChangeDemo( Uint32 num ) {
	if ( num < mDemo.size() ) {
		mDemo[ mCurDemo ].destroy();

		mCurDemo = num;

		mDemo[ mCurDemo ].init();
	}
}

void cEETest::PhysicsCreate() {
	cPhysicsManager::CreateSingleton();
	cPhysicsManager::instance()->CollisionSlop( 0.2 );
	cPhysicsManager * PM = cPhysicsManager::instance();
	cPhysicsManager::cDrawSpaceOptions * DSO = PM->GetDrawOptions();

	DSO->DrawHash			= false;
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
	mMousePoint = cVectNew( KM->GetMousePosf().x, KM->GetMousePosf().y );
	cVect newPoint = tovect( cpvlerp( tocpv( mMousePoint_last ), tocpv( mMousePoint ), 0.25 ) );
	mMouseBody->Pos( newPoint );
	mMouseBody->Vel( ( newPoint - mMousePoint_last ) * (cpFloat)EE->FPS() );
	mMousePoint_last = newPoint;

	if ( KM->MouseLeftPressed() ) {
		if ( NULL == mMouseJoint ) {
			cVect point = cVectNew( KM->GetMousePosf().x, KM->GetMousePosf().y );

			Physics::cShape * shape = mSpace->PointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

			if( NULL != shape ){
				mMouseJoint = eeNew( cPivotJoint, ( mMouseBody, shape->Body(), cVectZero, shape->Body()->World2Local( point ) ) );

				mMouseJoint->MaxForce( 50000.0f );
				mMouseJoint->BiasCoef( 0.15f );
				mSpace->AddConstraint( mMouseJoint );
			}
		}
	} else if ( NULL != mMouseJoint ) {
		mSpace->RemoveConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}

	mDemo[ mCurDemo ].update();
	mSpace->Update();
	mSpace->Draw();
}

void cEETest::PhysicsDestroy() {
	mDemo[ mCurDemo ].destroy();
	cPhysicsManager::DestroySingleton();
}
/*
static void defineVertexArrayObject(GLuint vaoId, size_t NbytesV, size_t NbytesC, GLint size, GLenum type, GLfloat *vertices, GLfloat *colors, GLfloat * texCoords, GLuint shader_id ) {
	//enable vertex array object to be defined
	glBindVertexArray(vaoId);

	//generate VBO foreach 'in'; dgl_Vertex dgl_Color dgl_TexCoord
	GLuint m_vboId[3];
	glGenBuffers(3, &m_vboId[0]);

	//"in		 vec2 dgl_Vertex;",
	glBindBuffer(GL_ARRAY_BUFFER, m_vboId[0] );											// enable the 1st VBO
	glBufferData(GL_ARRAY_BUFFER, NbytesV, vertices, GL_STATIC_DRAW);					// fill the VBO with vertices data
	const GLuint index_mPosition = glGetAttribLocation( shader_id, "dgl_Vertex" );		// get ID for "dgl_Vertex"
	glEnableVertexAttribArray( index_mPosition );										// enable VBO vertex attribute ("dgl_Vertex")
	glVertexAttribPointer( index_mPosition, 2, type, GL_FALSE, 0, 0 );					// VBO point to the "dgl_Vertex" attribute

	//"in		 vec4 dgl_Color;",
	glBindBuffer( GL_ARRAY_BUFFER, m_vboId[1] );										// enable the 2nd VBO
	glBufferData( GL_ARRAY_BUFFER, NbytesC, colors, GL_STATIC_DRAW );					// fill the 2nd VBO with colors data
	const GLuint index_mcolor = glGetAttribLocation( shader_id,"dgl_Color" );			// get ID for "dgl_Color"
	glEnableVertexAttribArray( index_mcolor );											// enable VBO vertex attribute ("dgl_Color")
	glVertexAttribPointer( index_mcolor, 4, type, GL_FALSE, 0, 0 );						// VBO point to the "dgl_Color" attribute

	//"in		 vec2 dgl_TexCoord;",
	glBindBuffer( GL_ARRAY_BUFFER, m_vboId[2] );										// enable the 3rd VBO
	glBufferData( GL_ARRAY_BUFFER, NbytesV, texCoords, GL_STATIC_DRAW );				// fill the 3nd VBO with tex coords data
	const GLuint index_mcoords = glGetAttribLocation( shader_id,"dgl_TexCoord" );		// get ID for "dgl_TexCoords"
	glEnableVertexAttribArray( index_mcoords );											// enable VBO vertex attribute ("dgl_TexCoords")
	glVertexAttribPointer( index_mcoords, 2, type, GL_FALSE, 0, 0 );					// VBO point to the "dgl_TexCoords" attribute
}
*/
int main (int argc, char * argv []) {
	cEETest * Test = eeNew( cEETest, () );

	Test->Process();

	eeDelete( Test );
/*
	cEngine *			EE		= cEngine::instance();
	EE->Init( 800, 600, 32, true, true, true, true, false, false, GLv_3 );
	EE->SetBackColor( eeColor( 255, 255, 255 ) );

	cInput *			KM		= cInput::instance();
	cTextureFactory *	TF		= cTextureFactory::instance();
	cTexture *			Tex		= TF->GetTexture( TF->Load( AppPath() + "data/bnb/bnb.png" ) );

	cRendererGL3 * Ren = reinterpret_cast<cRendererGL3 *>( GLi );

	GLfloat vertices0[] = { // dgl_Vertex
		0.0		, 0.0, // xy
		0.0		, 600,
		800.0	, 600.0,
		800.0	, 0.0
	};

	size_t Nbytes_vertices0=sizeof(vertices0);

	GLfloat colors0[] = { // dgl_Color
		1.0, 1.0, 1.0, 0.5, //rgba
		1.0, 1.0, 1.0, 0.5,
		1.0, 1.0, 1.0, 0.5,
		1.0, 1.0, 1.0, 0.5
	};
	size_t Nbytes_colors0=sizeof(colors0);

	GLfloat texCoords0[] = { // dgl_TexCoord
		0.0		, 0.0, // xy
		0.0		, 1.0,
		1.0		, 1.0,
		1.0		, 0.0
	};

	GLuint				vao_id;

	glGenVertexArrays( 1, &vao_id );
	glBindVertexArray( vao_id );

	GLuint vao_elementcount = Nbytes_vertices0 / 2 / sizeof(GLfloat);

	defineVertexArrayObject( vao_id, Nbytes_vertices0, Nbytes_colors0, 4, GL_FLOAT, vertices0, colors0, texCoords0, Ren->BaseShaderId() );

	eeFloat ang = 0;

	while( EE->Running() ) {
		KM->Update();

		ang += EE->Elapsed() * 0.1;

		TF->Bind( Tex );
		//glBindVertexArray( vao_id );

		GLi->DrawArrays( GL_TRIANGLE_FAN, 0, vao_elementcount );

		GLi->PushMatrix();

		GLi->Translatef( 400, 300, 0 );
		GLi->Rotatef( ang, 0, 0, 1 );
		GLi->Translatef( -400, -300, 0 );

		GLi->DrawArrays( GL_TRIANGLE_FAN, 0, vao_elementcount );

		GLi->PopMatrix();

		EE->Display();

		if ( KM->IsKeyDown( KEY_ESCAPE ) ) EE->Running( false );
	};

	cEngine::DestroySingleton();
*/
	EE::MemoryManager::LogResults();

	return 0;
}
