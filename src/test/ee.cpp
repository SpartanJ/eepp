#include "../ee.h"

/**
@TODO Create a asynchronous resource loader ( at least for textures ).
@TODO Create a basic UI system ( add basic controls, add skinning support ).
@TODO Add support for Joysticks.
@TODO Create a Vertex Buffer Object class ( with and without GL_ARB_vertex_buffer_object ).
@TODO Add support for Frame Buffer Object and to switch rendering to FBO and Screen.
@TODO Support multitexturing.
@TODO Create a texture packer tool ( pack various textures into one texture ).
@TODO Encapsulate SDL and OpenGL ( and remove unnecessary dependencies ).
@TODO Add some Surface Grid class, to create special effects ( waved texture, and stuff like that ).
@TODO Support color cursors ( not only black and white cursors, that really sucks ) - Imposible with SDL 1.2
*/

class cUITest : public cUIControlAnim {
	public:
		cUITest( cUIControlAnim::CreateParams& Params ) : cUIControlAnim( Params ) 	{ mOldColor = mBackground.Colors(); }

		virtual Uint32 OnMouseEnter( const eeVector2i& Pos, const Uint32 Flags )	{
			if ( 4 == mOldColor.size() ) {
				mBackground.Colors( eeColorA( mOldColor[0].R(), mOldColor[0].G(), mOldColor[0].B(), 200 ),
									eeColorA( mOldColor[1].R(), mOldColor[1].G(), mOldColor[1].B(), 200 ),
									eeColorA( mOldColor[2].R(), mOldColor[2].G(), mOldColor[2].B(), 200 ),
									eeColorA( mOldColor[3].R(), mOldColor[3].G(), mOldColor[3].B(), 200 )
								);
			} else {
				mBackground.Color( eeColorA( mOldColor[0].R(), mOldColor[0].G(), mOldColor[0].B(), 200 ) );
			}

			return 1;
		}

		virtual Uint32 OnMouseExit( const eeVector2i& Pos, const Uint32 Flags )	{
			if ( 4 == mOldColor.size() ) {
				mBackground.Colors( mOldColor[0], mOldColor[1], mOldColor[2], mOldColor[3] );
			} else {
				mBackground.Color( mOldColor[0] );
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
		typedef boost::function0<void> SceneCb;

		void Init();
		void End();
		void Process();
		void Render();
		void Input();
		void InputCallback(EE_Event* event);
		void ParticlesCallback(cParticle* P, cParticleSystem* Me);

		void ParticlesThread();
		void Particles();
		void LoadTextures();
		void CmdSetPartsNum ( const std::vector < std::wstring >& params );

		std::vector<cParticleSystem> PS;

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
		cTextureFont FF, FF2;
		cTTFFont TTF;
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

		#ifdef EE_SHADERS
		cShaderProgram* mShaderProgram;
    	eeFloat mBlurFactor;
    	bool mUseShaders;
    	#endif

    	Uint32 mLastFPSLimit;
    	bool mWasMinimized;

		eeInt mWidth;
		eeInt mHeight;

		std::wstring mBuda;

		cTextureLoader * mTexLoader;
};


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

		Log = cLog::instance();
		KM = cInput::instance();

		PS.resize(5);

		KM->PushCallback( std::bind1st( std::mem_fun( &cEETest::InputCallback ), this) );

		Scenes[0] = boost::bind( &cEETest::Screen1, this );
		Scenes[1] = boost::bind( &cEETest::Screen2, this );
		Scenes[2] = boost::bind( &cEETest::Screen3, this );

		InBuf.Start();

		SetRandomSeed();

		LoadTextures();

		if ( SndMng.LoadFromPack( "mysound", &PAK, "sound.ogg" ) )
			SndMng["mysound"].Play();


		if ( Mus.OpenFromPack( &PAK, "music.ogg" ) ) {
			Mus.Loop(true);
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
		Batch.SetBlendFunc( ALPHA_BLENDONE );

		#ifdef EE_SHADERS
		mUseShaders = mUseShaders && EE->ShadersSupported();

		mShaderProgram = NULL;

		if ( mUseShaders ) {
			mBlurFactor = 0.01f;
			mShaderProgram = new cShaderProgram( MyPath + "data/shader/blur.vert", MyPath + "data/shader/blur.frag" );
		}
		#endif

		cUIManager::instance()->Init();

		cUIControl::CreateParams Params( cUIManager::instance()->MainControl(), eeVector2i(0,0), eeSize( 320, 240 ), UI_FILL_BACKGROUND | UI_CLIP_ENABLE | UI_BORDER );

		Params.Border.Width( 2.f );
		Params.Border.Color( 0xFF979797 );
		//Params.Background.Corners(5);
		Params.Background.Colors( eeColorA( 0x66FAFAFA ), eeColorA( 0xCCFAFAFA ), eeColorA( 0xCCFAFAFA ), eeColorA( 0x66FAFAFA ) );
		cUIControlAnim * C = new cUITest( Params );
		C->Visible( true );
		C->Enabled( true );
		C->Pos( 320, 240 );
		C->DragEnable( true );
		C->StartRotation( 0.f, 360.f, 2500.f );

		Params.Flags &= ~UI_CLIP_ENABLE;
		Params.Background.Corners(0);
		Params.Background.Colors( eeColorA( 0x7700FF00 ), eeColorA( 0x7700CC00 ), eeColorA( 0x7700CC00 ), eeColorA( 0x7700FF00 ) );
		Params.Parent( C );
		Params.Size = eeSize( 50, 50 );
		cUITest * Child = new cUITest( Params );
		Child->Pos( 25, 50 );
		Child->Visible( true );
		Child->Enabled( true );
		Child->StartRotation( 0.f, 360.f * 10.f, 5000.f * 10.f );

		Params.Background.Colors( eeColorA( 0x77FFFF00 ), eeColorA( 0x77CCCC00 ), eeColorA( 0x77CCCC00 ), eeColorA( 0x77FFFF00 ) );
		Params.Parent( Child );
		Params.Size = eeSize( 25, 25 );
		cUITest * Child2 = new cUITest( Params );
		Child2->Pos( 15, 15 );
		Child2->Visible( true );
		Child2->Enabled( true );
		Child2->StartRotation( 0.f, 360.f * 10.f, 5000.f * 10.f );

		cUIGfx::CreateParams GfxParams;
		GfxParams.Parent( C );
		GfxParams.PosSet( 160, 100 );
		GfxParams.Flags |= UI_CLIP_ENABLE;
		GfxParams.Size = eeSize( 64, 64 );
		GfxParams.Shape = cShapeManager::instance()->Add( TN[2] );
		cUIGfx * Gfx = new cUIGfx( GfxParams );
		Gfx->Angle( 45.f );
		Gfx->Visible( true );
		Gfx->Enabled( true );
		Gfx->StartAlphaAnim( 100.f, 255.f, 1000.f );
		Gfx->AlphaInterpolation()->Loop( true );
		Gfx->AlphaInterpolation()->SetTotalTime( 1000.f );

		cUITextBox::CreateParams TextParams;
		TextParams.Parent( C );
		TextParams.PosSet( 0, 0 );
		TextParams.Size = eeSize( 320, 240 );
		TextParams.Flags = UI_VALIGN_TOP | UI_HALIGN_RIGHT;
		TextParams.Font = &TTF;
		cUITextBox * Text = new cUITextBox( TextParams );
		Text->Visible( true );
		Text->Enabled( false );
		Text->Text( L"Turn around\nJust Turn Around\nAround!" );

		cUITextInput::CreateParams InputParams;
		InputParams.Parent( C );
		//InputParams.Background.Corners(6);
		InputParams.Border.Color(0xFF979797);
		InputParams.Background.Colors( eeColorA(0x99AAAAAA), eeColorA(0x99CCCCCC), eeColorA(0x99CCCCCC), eeColorA(0x99AAAAAA) );
		InputParams.PosSet( 10, 220 );
		InputParams.Size = eeSize( 300, 18 );
		InputParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_FILL_BACKGROUND | UI_CLIP_ENABLE | UI_BORDER;
		InputParams.Font = &TTF;
		InputParams.SupportNewLine = false;
		cUITextInput * Input = new cUITextInput( InputParams );
		Input->Visible( true );
		Input->Enabled( true );

		mBuda = L"El mono ve el pez en el agua y sufre. Piensa que su mundo es el único que existe, el mejor, el real. Sufre porque es bueno y tiene compasión, lo ve y piensa: \"Pobre se está ahogando no puede respirar\". Y lo saca, lo saca y se queda tranquilo, por fin lo salvé. Pero el pez se retuerce de dolor y muere. Por eso te mostré el sueño, es imposible meter el mar en tu cabeza, que es un balde.\nPowered by Text Shrinker =)";
		TTF.ShrinkText( mBuda, 400 );

		Launch();
	} else {
		cout << "Failed to start EE++" << endl;
		cEngine::DestroySingleton();
		exit(0);
	}
}

void cEETest::CmdSetPartsNum ( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );
			if ( tInt >= 0 && tInt <= 10000 ) {
				PS[2].Create(WormHole, tInt, TN[5], EE->GetWidth() * 0.5f, EE->GetHeight() * 0.5f, 32, true);
				Con.PushText( L"Wormhole Particles Number Changed to: " + toWStr(tInt) );
			} else
				Con.PushText( L"Valid parameters are between 0 and 10000 (0 = no limit)." );
		} catch (boost::bad_lexical_cast&) {
			Con.PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	}

}

void cEETest::LoadTextures() {
	Uint32 i;

	TF->Allocate(40);

	mTexLoader = new cTextureLoader( MyPath + "data/test.jpg" );
	mTexLoader->Threaded(true);
	mTexLoader->Load();

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

	SP.CreateAnimation();

	for ( Int32 my = 0; my < 4; my++ )
		for( Int32 mx = 0; mx < 8; mx++ )
			SP.AddFrame( TN[4], 0, 0, 0, 0, eeRecti( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );

	TN[8] = TF->LoadFromPack( &PAK, "conchars.png", false, eeRGB(0,0,0) );
	TNP[8] = TF->GetTexture( TN[8] );

	FF.Load( TN[8], 32 );

	TN[9] = TF->LoadFromPack( &PAK, "ProggySquareSZ.png" );
	TNP[9] = TF->GetTexture( TN[9] );

	FF2.LoadFromPack( &PAK, "ProggySquareSZ.dat", TN[9] );

	TTF.LoadFromPack( &PAK, "arial.ttf", 12, EE_TTF_STYLE_NORMAL, false, 512, eeColor(255,255,255), 1, eeColor(0,0,0) );
	TF->GetTexture( TTF.GetTexId() )->SetTextureFilter( TEX_NEAREST );

	PS[0].SetCallbackReset( boost::bind( &cEETest::ParticlesCallback, this, _1, _2) );
	PS[0].Create(Callback, 500, TN[5], 0, 0, 16, true);
	PS[1].Create(Heal, 250, TN[5], EE->GetWidth() * 0.5f, EE->GetHeight() * 0.5f, 16, true);

	PS[2].Create(WormHole, PartsNum, TN[5], EE->GetWidth() * 0.5f, EE->GetHeight() * 0.5f, 32, true);
	Con.AddCommand( L"setparticlesnum", boost::bind( &cEETest::CmdSetPartsNum, this, _1) );

	PS[3].Create(Fire, 350, TN[5], -50.f, -50.f, 32, true);
	PS[4].Create(Fire, 350, TN[5], -50.f, -50.f, 32, true);

	cTexture * Tex = TF->GetTexture(TN[2]);
	
	Tex->Lock();
	eeInt w = Tex->Width();
	eeInt h = Tex->Height();
	
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

	CL1.CreateStatic(TN[2]);
	CL1.UpdatePos(500,400);
	CL1.Scale(0.5f);

	CL2.CreateStatic(TN[0], 96, 96);
	CL2.Color( eeRGBA( 255, 255, 255, 255 ) );

	Con.Create( &FF, true );
	Con.IgnoreCharOnPrompt( 186 ); // L'º'

	Map.myFont = reinterpret_cast<cFont*> ( &FF );

	Map.Create( 100, 100, 2, 128, 64, eeColor(175,175,175) );

	RandomizeHeights();

	TreeTilingCreated = false;
	CreateTiling(Wireframe);
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
	for ( x = 0; x < static_cast<Int32>( Map.Width() ); x++ )
		for ( y = 0; y < static_cast<Int32>( Map.Height() ); y++ ) {
			if (Wire)
				Map.Layer(x, y, 0, Tiles[6] );
			else
				Map.Layer(x, y, 0, Tiles[ eeRandi( 0, 5 ) ] );

			if ( !TreeTilingCreated )
				Map.Layer(x, y, 1, 0);
		}

	if ( !TreeTilingCreated ) {
		for ( x = 0; x < 100; x++ )
			Map.Layer( eeRandi(Map.Width()-1), eeRandi(Map.Height()-1), 1, Tiles[7] );

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

void cEETest::Screen2() {
	eeFloat PlanetX = HWidth  - TNP[6]->Width() * 0.5f;
	eeFloat PlanetY = HHeight - TNP[6]->Height() * 0.5f;

	ang+=et * 0.1f;
	ang = (ang>=360) ? ang = 0 : ang;

	if (scale>=1.5f) {
		scale = 1.5f;
		side = true;
	} else if (scale<=0.5f) {
		side = false;
		scale = 0.5f;
	}
	scale = (!side) ? scale+=et * 0.00025f : scale -=et * 0.00025f;

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

	#ifdef EE_SHADERS
	if ( mUseShaders ) {
		mBlurFactor = scale * 0.01f;
		mShaderProgram->Bind();
		mShaderProgram->SetUniform( "blurfactor" , mBlurFactor );
	}
	#endif

	TNP[6]->DrawFast( PlanetX, PlanetY, ang, scale);

	#ifdef EE_SHADERS
	if ( mUseShaders )
		mShaderProgram->Unbind();
	#endif

	TNP[3]->Draw( HWidth - 128, HHeight, 0, 1, eeColorA(255,255,255,150), ALPHA_NORMAL, RN_ISOMETRIC);
	TNP[3]->Draw( HWidth - 128, HHeight - 128, 0, 1, eeColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRIC);
	TNP[3]->Draw( HWidth - 128, HHeight, 0, 1, eeColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRICVERTICAL);
	TNP[3]->Draw( HWidth, HHeight, 0, 1, eeColorA(255,255,255,50), ALPHA_NORMAL, RN_ISOMETRICVERTICALNEGATIVE);

	alpha = (!aside) ? alpha+=et * 0.1f : alpha -=et * 0.1f;
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

	PR.DrawRectangle( CL1.GetAABB(), 0.0f, 1.0f, DRAW_LINE );

	PR.DrawQuad( CL1.GetQuad(), DRAW_LINE );

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

	PR.DrawCircle(Mousef.x, Mousef.y, 80.f, (Uint32)(Ang/3), DRAW_LINE);

	PR.DrawTriangle( eeVector2f( Mousef.x, Mousef.y - 10.f ), eeVector2f( Mousef.x - 10.f, Mousef.y + 10.f ), eeVector2f( Mousef.x + 10.f, Mousef.y + 10.f ), DRAW_LINE );
	PR.DrawLine( eeVector2f(Mousef.x - 80.f, Mousef.y - 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y + 80.f) );
	PR.DrawLine( eeVector2f(Mousef.x - 80.f, Mousef.y + 80.f), eeVector2f(Mousef.x + 80.f, Mousef.y - 80.f) );
	PR.DrawLine( eeVector2f((eeFloat)EE->GetWidth(), 0.f), eeVector2f( 0.f, (eeFloat)EE->GetHeight() ) );
	PR.DrawQuad( eeVector2f(0.f, 0.f), eeVector2f(0.f, 100.f), eeVector2f(150.f, 150.f), eeVector2f(200.f, 150.f), eeColorA(220, 240, 0, 125), eeColorA(100, 0, 240, 125), eeColorA(250, 50, 25, 125), eeColorA(50, 150, 150, 125) );
	PR.DrawRectangle(Mousef.x - 80.f, Mousef.y - 80.f, 160.f, 160.f, 45.f, 1.f, DRAW_LINE);
	PR.DrawLine( eeVector2f(0.f, 0.f), eeVector2f( (eeFloat)EE->GetWidth(), (eeFloat)EE->GetHeight() ) );

	TNP[3]->DrawQuadEx( eeQuad2f( eeVector2f(0.f, 0.f), eeVector2f(0.f, 100.f), eeVector2f(150.f, 150.f), eeVector2f(200.f, 150.f) ), 0.0f, 0.0f, ang, scale, eeColorA(220, 240, 0, 125), eeColorA(100, 0, 240, 125), eeColorA(250, 50, 25, 125), eeColorA(50, 150, 150, 125) );

	WP.Update( et );
	PR.SetColor( eeColorA(0, 255, 0, 255) );
	PR.DrawPoint( WP.GetPos(), 10.f );
}

void cEETest::Screen1() {
	Map.Draw();
}

void cEETest::Screen3() {
	if (AnimVal>=300.0f) {
		AnimVal = 300.0f;
		AnimSide = true;
	} else if (AnimVal<=0.5f) {
		AnimVal = 0.5f;
		AnimSide = false;
	}
	AnimVal = (!AnimSide) ? AnimVal+=et * 0.1f : AnimVal -=et * 0.1f;

	Batch.SetTexture( TNP[3] );
	Batch.LineLoopBegin();
	for ( eeFloat j = 0; j < 360; j++ ) {
		Batch.BatchLineLoop( HWidth + 350 * sinAng(j), HHeight + 350 * cosAng(j), HWidth + AnimVal * sinAng(j+1), HHeight + AnimVal * cosAng(j+1) );
	}
	Batch.Draw();
}

void cEETest::Render() {
	mTexLoader->Update();

	if ( mTexLoader->IsLoaded() ) {
		cTexture * TexLoaded = TF->GetTexture( mTexLoader->TexId() );
		
		if ( NULL != TexLoaded )
			TexLoaded->Draw( 0, 0 );
	}

	HWidth = EE->GetWidth() * 0.5f;
	HHeight = EE->GetHeight() * 0.5f;

	if ( eeGetTicks() - lasttick >= 100 ) {
		lasttick = eeGetTicks();
		mInfo = StrFormated( "EE - FPS: %d Elapsed Time: %4.8f\nMouse X: %d Mouse Y: %d\nTexture Memory Size: %d",
							EE->FPS(),
							et,
							(Int32)Mouse.x,
							(Int32)Mouse.y,
							(Int32)TF->MemorySize()
						);
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

	TTF.SetText( L"Entropia Engine++\nEE++ Support TTF Fonts and they look beautifull. :)\nCTRL + 1 = Screen 1 - CTRL + 2 = Screen 2" );

	eeColorA ColRR1( 150, 150, 150, 220 );
	eeColorA ColRR4( 150, 150, 150, 220 );
	eeColorA ColRR2( 100, 100, 100, 220 );
	eeColorA ColRR3( 100, 100, 100, 220 );

	PR.SetColor( eeColorA(150, 150, 150, 220) );
	PR.DrawRectangle(
					0.f,
					(eeFloat)EE->GetHeight() - (eeFloat)TTF.GetNumLines() * (eeFloat)TTF.GetFontSize(),
					(eeFloat)TTF.GetTextWidth(),
					(eeFloat)TTF.GetNumLines() * (eeFloat)TTF.GetFontSize(),
					ColRR1, ColRR2, ColRR3, ColRR4
	);

	TTF.Draw( 0.f, (eeFloat)EE->GetHeight() - TTF.GetTextHeight(), FONT_DRAW_CENTER, 1.f, Ang );

	FF.Color( eeColorA(255,255,255,200) );
	FF.Draw( mInfo, 6, 6 );

	FF2.SetText( InBuf.Buffer() );
	FF2.Draw( 6, 24, FONT_DRAW_SHADOW );

	Uint32 NLPos = 0;
	Uint32 LineNum = InBuf.GetCurPosLinePos( NLPos );
	if ( InBuf.CurPos() == (eeInt)InBuf.Buffer().size() && !LineNum ) {
		FF2.Draw( L"_", 6.f + FF2.GetTextWidth(), 24.f );
	} else {
		FF2.SetText( InBuf.Buffer().substr( NLPos, InBuf.CurPos() - NLPos ) );
		FF2.Draw( L"_", 6.f + FF2.GetTextWidth(), 24.f + (eeFloat)LineNum * (eeFloat)FF2.GetFontSize() );
	}

	TTF.SetText( mBuda );
	TTF.Draw( 0.f, 50.f );

	FF2.SetText( L"FPS: " + toWStr( EE->FPS() ) );
	FF2.Draw( EE->GetWidth() - FF2.GetTextWidth() - 15, 0 );

	cUIManager::instance()->Update();
	cUIManager::instance()->Draw();

	Con.Draw();

	if ( Screen < 2 )
		CursorP[ Screen ]->Draw( Mousef.x, Mousef.y );
}

void cEETest::Input() {
	KM->Update();
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

	if ( KM->IsKeyUp(KEY_F1) )
		MultiViewportMode = !MultiViewportMode;

	if ( KM->AltPressed() && KM->IsKeyUp(KEY_M) && !Con.Active() )
		EE->MaximizeWindow();

	if ( KM->IsKeyDown(KEY_ESCAPE) )
		EE->Running(false);

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

	switch (Screen) {
		case 0:
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

			if ( KM->MouseLeftPressed() )
				TNP[3]->DrawEx( 0.f, 0.f, (eeFloat)EE->GetWidth(), (eeFloat)EE->GetHeight() );

			if ( !KM->MouseRightPressed() )
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
			Render();

			if ( KM->IsKeyUp(KEY_F12) ) EE->TakeScreenshot( MyPath + "data/screenshots/" ); //After render and before Display

			EE->Display();
		} while( EE->Running() );
	}
	End();
}

void cEETest::End() {
	delete mTexLoader;

	Wait();

	Mus.Stop();

	TN.clear();
	Tiles.clear();
	tmpv.clear();
	MySong.clear();

	cLog::instance()->Save();
	cEngine::DestroySingleton();
	cUIManager::DestroySingleton();
}

void cEETest::InputCallback(EE_Event* event) {
	switch(event->type) {
		case SDL_MOUSEBUTTONDOWN:
			if (event->button.button == SDL_BUTTON_RIGHT)
				DrawBack = true;
			break;
	}
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
	cEETest Test;
	Test.Process();

	return 0;
}
