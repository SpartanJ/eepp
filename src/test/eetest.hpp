#ifndef EE_EETEST_HPP
#define EE_EETEST_HPP

#include <eepp/ee.hpp>

namespace Demo_Test {

class cUITest : public cUIControlAnim {
	public:
		cUITest( cUIControlAnim::CreateParams& Params ) : cUIControlAnim( Params ) 	{ mOldColor = mBackground->Colors(); }

		virtual Uint32 OnMouseEnter( const Vector2i& Pos, const Uint32 Flags )	{
			if ( 4 == mOldColor.size() ) {
				mBackground->Colors( ColorA( mOldColor[0].R(), mOldColor[0].G(), mOldColor[0].B(), 200 ),
									ColorA( mOldColor[1].R(), mOldColor[1].G(), mOldColor[1].B(), 200 ),
									ColorA( mOldColor[2].R(), mOldColor[2].G(), mOldColor[2].B(), 200 ),
									ColorA( mOldColor[3].R(), mOldColor[3].G(), mOldColor[3].B(), 200 )
								);
			} else {
				mBackground->Color( ColorA( mOldColor[0].R(), mOldColor[0].G(), mOldColor[0].B(), 200 ) );
			}

			return 1;
		}

		virtual Uint32 OnMouseExit( const Vector2i& Pos, const Uint32 Flags )	{
			if ( 4 == mOldColor.size() ) {
				mBackground->Colors( mOldColor[0], mOldColor[1], mOldColor[2], mOldColor[3] );
			} else {
				mBackground->Color( mOldColor[0] );
			}

			return 1;
		}

		virtual Uint32 OnMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
			cUIDragable::OnMouseUp( Pos, Flags );

			if ( cEngine::instance()->GetCurrentWindow()->GetInput()->MouseWheelUp() )
				Scale( Scale() + 0.1f );
			else if ( cEngine::instance()->GetCurrentWindow()->GetInput()->MouseWheelDown() )
				Scale( Scale() - 0.1f );

			return 1;
		}

		virtual Uint32 OnFocus() {
			ToFront();

			return 1;
		}

		const std::vector<ColorA>& OldColor() { return mOldColor; }
	protected:
		std::vector<ColorA> mOldColor;
};

enum CollisionTypes {
	BALL_TYPE,
	BLOCKING_SENSOR_TYPE,
	CATCH_SENSOR_TYPE
};

struct Emitter {
	int queue;
	int blocked;
	cVect position;
};

class cEETest : private Thread {
	public:
		typedef cb::Callback0<void> SceneCb;

		void Init();
		void Update();
		void End();
		void Process();
		void Render();
		void Input();
		void ParticlesCallback(cParticle* P, cParticleSystem* Me);

		void ParticlesThread();
		void Particles();
		void UpdateParticles();
		void LoadTextures();
		void CmdSetPartsNum ( const std::vector < String >& params );

		Clock cElapsed;
		Time PSElapsed;
	private:
		cEngine * EE;
		Window::cWindow * mWindow;
		cTextureFactory* TF;
		System::Log* Log;
		cInput* KM;
		cInputTextBuffer InBuf;

		bool side, aside;
		Float ang, scale, alpha, Ang;
		Time et;
		Int32 x, y;
		Uint32 lasttick;

		std::vector<Uint32> TN;
		std::vector<cTexture *> TNP;

		std::vector<cSubTexture*> Tiles;
		std::vector<cParticleSystem> PS;

		Vector2i Mouse;
		Vector2f Mousef;

		cSprite SP;
		cSprite CL1, CL2;
		cFont * FF;
		cFont * FF2;
		cFont * TTF;
		cFont * TTFB;

		cPrimitives PR;
		bool iL1, iL2;
		Float HWidth, HHeight;

		Music * Mus;
		SoundManager SndMng;

		bool DrawBack;

		cConsole Con;
		virtual void Run();

		Vector2f Point;

		std::string MyPath;
		bool ShowParticles;

		cMap Map;

		Uint8 Screen;
		SceneCb Scenes[6];
		void Screen1();
		void Screen2();
		void Screen3();
		void Screen4();
		void Screen5();

		Zip * PakTest;

		std::vector<Uint8> tmpv;
		std::vector<Uint8> MySong;

		Waypoints WP;
		Int32 PartsNum;
		Uint32 Cursor[1];
		cTexture * CursorP[1];
		std::string mInfo;

		bool MultiViewportMode;

		cBatchRenderer Batch;
		Float AnimVal;
		bool AnimSide;

		cView Views[2];

		cShaderProgram * mShaderProgram;

		Float mBlurFactor;
		bool mUseShaders;
		bool mJoyEnabled;
		bool mMusEnabled;

		Uint32 mLastFPSLimit;
		bool mWasMinimized;

		String mBuda;

		ResourceLoader mResLoad;
		void OnTextureLoaded( ResourceLoader * ObjLoaded );

		void CreateUI();
		void CreateShaders();

		void LoadFonts();

		ResourceLoader mFontLoader;
		void OnFontLoaded( ResourceLoader * ObjLoaded );

		cJoystickManager * JM;
		Float mAxisX;
		Float mAxisY;

		cTextureAtlasLoader * mTGL;
		cSprite mBlindy;

		cFrameBuffer * mFBO;
		cVertexBuffer * mVBO;

		Clock	mFTE;

		void CreateCommonDialog();
		void ItemClick( const cUIEvent * Event );
		void MainClick( const cUIEvent * Event );
		void QuitClick( const cUIEvent * Event );
		void CloseClick( const cUIEvent * Event );
		void ButtonClick( const cUIEvent * Event );
		void OnValueChange( const cUIEvent * Event );
		void OnSliderValueChange( const cUIEvent * Event );
		void OnWinMouseUp( const cUIEvent * Event );
		void CreateDecoratedWindow();
		void CreateWinMenu();
		void CreateUIThemeTextureAtlas();

		cUIControlAnim * C;
		cUIScrollBar * mScrollBar;
		cUITextBox * mTextBoxValue;
		cUISlider * mSlider;
		cUIProgressBar * mProgressBar;
		cUIListBox * mListBox;
		cUIPopUpMenu * Menu;
		cUIWindow * mUIWindow;
		cMapEditor * mMapEditor;
		cTextureAtlasEditor * mETGEditor;

		cTextCache mEEText;
		cTextCache mFBOText;
		cTextCache mInfoText;

		cSpace * mSpace;

		#ifndef EE_PLATFORM_TOUCH
		cBody * mMouseBody;
		cVect mMousePoint;
		cVect mMousePoint_last;
		cConstraint * mMouseJoint;
		#else
		cBody * mMouseBody[ EE_MAX_FINGERS ];
		cVect mMousePoint[ EE_MAX_FINGERS ];
		cVect mMousePoint_last[ EE_MAX_FINGERS ];
		cConstraint * mMouseJoint[ EE_MAX_FINGERS ];
		#endif

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

		void ShowMenu();

		Emitter emitterInstance;

		void ChangeDemo( Uint32 num );

		struct physicDemo {
			SceneCb init;
			SceneCb update;
			SceneCb destroy;
		};

		std::vector<physicDemo> mDemo;
		Uint32					mCurDemo;
		cSprite *				mBoxSprite;
		cSprite *				mCircleSprite;

		cUITheme *				mTheme;

		bool					mTerrainUp;
		cUIPushButton *			mShowMenu;
		cUIPushButton *			mTerrainBut;

		void CreateMapEditor();

		void OnMapEditorClose();

		void OnETGEditorClose();

		void CreateETGEditor();

		void CreateJointAndBody();

		void DestroyBody();

		void OnShowMenu( const cUIEvent * Event );

		void OnWindowResize( cWindow * win );
};

}
#endif

