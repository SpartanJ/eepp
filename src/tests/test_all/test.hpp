#ifndef EE_EETEST_HPP
#define EE_EETEST_HPP

#include <eepp/ee.hpp>
#include <eepp/maps/maps.hpp>
#include <eepp/physics/physics.hpp>

namespace Demo_Test {

enum CollisionTypes { BALL_TYPE, BLOCKING_SENSOR_TYPE, CATCH_SENSOR_TYPE };

struct Emitter {
	int queue;
	int blocked;
	cVect position;
};

class EETest : private Thread {
  public:
	typedef std::function<void()> SceneCb;

	void init();
	void update();
	void end();
	void process();
	void render();
	void input();
	void particlesCallback( Particle* P, ParticleSystem* Me );

	void particlesThread();
	void particles();
	void updateParticles();
	void loadTextures();
	void cmdSetPartsNum( const std::vector<String>& params );

	Clock cElapsed;
	Time PSElapsed;

  private:
	Engine* EE;
	EE::Window::Window* mWindow;
	TextureFactory* TF;
	System::Log* Log;
	EE::Window::Input* KM;

	bool side, aside;
	Float ang, scale, alpha, Ang;
	Time et;
	Int32 x, y;
	Uint32 lasttick;

	std::vector<Uint32> TN;
	std::vector<Texture*> TNP;

	std::vector<TextureRegion*> Tiles;
	std::vector<ParticleSystem> PS;

	Vector2i Mouse;
	Vector2f Mousef;

	Sprite SP;
	Sprite CL1, CL2;
	Font* TTF;

	Primitives PR;
	bool iL1, iL2;
	Float HWidth, HHeight;

	Music* Mus;
	SoundManager SndMng;

	bool DrawBack;

	UIConsole* Con;
	virtual void run();

	Vector2f Point;

	std::string MyPath;
	bool ShowParticles;

	TileMap Map;

	Uint8 Screen;
	SceneCb Scenes[6];
	void screen1();
	void screen2();
	void screen3();
	void screen4();
	void screen5();

	Zip* PakTest;

	std::vector<Uint8> tmpv;
	std::vector<Uint8> MySong;

	Interpolation2d WP;
	Int32 PartsNum;
	Uint32 Cursor[1];
	Texture* CursorP[1];
	std::string mInfo;

	bool MultiViewportMode;

	BatchRenderer Batch;
	Float AnimVal;
	bool AnimSide;

	View Views[2];

	ShaderProgram* mShaderProgram;
	ShaderProgram* mBlur;

	Float mBlurFactor;
	bool mUseShaders;
	bool mJoyEnabled;
	bool mMusEnabled;
	bool mDebugUI;

	Uint32 mLastFPSLimit;
	bool mWasMinimized;

	String mBuddha;

	ResourceLoader mResLoad;
	void onTextureLoaded( ResourceLoader* ObjLoaded );

	void createUI();
	void createShaders();

	void loadFonts();

	void onFontLoaded();

	JoystickManager* JM;
	Float mAxisX;
	Float mAxisY;

	TextureAtlasLoader* mTGL;
	Sprite mMonster;

	FrameBuffer* mFBO;
	VertexBuffer* mVBO;

	Clock mFTE;
	Translator mTranslator;

	void createFileDialog();
	void onItemClick( const Event* Event );
	void onMainClick( const Event* Event );
	void onQuitClick( const Event* Event );
	void onCloseClick( const Event* Event );
	void onButtonClick( const Event* Event );
	void onValueChange( const Event* Event );
	void onSliderValueChange( const Event* Event );
	void onWinMouseUp( const Event* Event );
	void createDecoratedWindow();
	void createUIThemeTextureAtlas();

	void addFlyingIcon();

	UINode* C;
	UIScrollBar* mScrollBar;
	UITextView* mTextBoxValue;
	UISlider* mSlider;
	UIProgressBar* mProgressBar;
	UIListBox* mListBox;
	UIPopUpMenu* Menu;
	UIWindow* mUIWindow;
	MapEditor* mMapEditor;
	TextureAtlasEditor* mETGEditor;
	UIColorPicker* mColorPicker;

	Text mEEText;
	Text mFBOText;
	Text mInfoText;

	std::string mThemeName;

	Space* mSpace;

#ifndef EE_PLATFORM_TOUCH
	Body* mMouseBody;
	cVect mMousePoint;
	cVect mMousePoint_last;
	Constraint* mMouseJoint;
#else
	Body* mMouseBody[EE_MAX_FINGERS];
	cVect mMousePoint[EE_MAX_FINGERS];
	cVect mMousePoint_last[EE_MAX_FINGERS];
	Constraint* mMouseJoint[EE_MAX_FINGERS];
#endif

	void physicsCreate();
	void physicsUpdate();
	void physicsDestroy();

	void setScreen( Uint32 num );

	cpBool blockerBegin( Arbiter* arb, Space* space, void* unused );
	void blockerSeparate( Arbiter* arb, Space* space, void* unused );
	void postStepRemove( Space* space, void* tshape, void* unused );
	cpBool catcherBarBegin( Arbiter* arb, Space* space, void* unused );

	void demo1Create();
	void demo1Update();
	void demo1Destroy();

	void demo2Create();
	void demo2Update();
	void demo2Destroy();

	void showMenu();

	Emitter emitterInstance;

	void changeDemo( Uint32 num );

	struct physicDemo {
		SceneCb init;
		SceneCb update;
		SceneCb destroy;
	};

	std::vector<physicDemo> mDemo;
	Uint32 mCurDemo;
	Sprite* mBoxSprite;
	Sprite* mCircleSprite;

	UITheme* mTheme;
	UISceneNode* mSceneNode;

	bool mTerrainUp;
	UIPushButton* mShowMenu;
	UIPushButton* mTerrainBut;
	UIRelativeLayout* relLay;

	DrawableGroup drawableGroup;

	void createMapEditor();

	void createColorPicker( Node* node );

	void onMapEditorClose();

	void createETGEditor();

	void createJointAndBody();

	void destroyBody();

	void onShowMenu( const Event* Event );

	void onWindowResize( EE::Window::Window* win );

	void createBaseUI();

	void createNewUI();
};

} // namespace Demo_Test
#endif
