#ifndef EE_UIEDITOR_HPP
#define EE_UIEDITOR_HPP

#include <eepp/ui/tools/uicodeeditorsplitter.hpp>

#include <eepp/ee.hpp>
#include <efsw/efsw.hpp>

using namespace EE::UI;
using namespace EE::UI::Tools;

namespace uieditor {

class UpdateListener;

enum InvalidationType : Uint32 { None, FileSystem, Memory };

class App : public UICodeEditorSplitter::Client {
  public:
	App();

	~App();

	void init( const Float& pixelDensityConf, const bool& useAppTheme, const std::string& cssFile,
			   const std::string& xmlFile, const std::string& projectFile );

	virtual void onCodeEditorCreated( UICodeEditor*, TextDocument& );

	virtual void onCodeEditorFocusChange( UICodeEditor* );

	virtual void onWidgetFocusChange( UIWidget* ){};

	virtual void onDocumentStateChanged( UICodeEditor*, TextDocument& ){};

	virtual void onDocumentModified( UICodeEditor*, TextDocument& );

	virtual void onDocumentUndoRedo( UICodeEditor*, TextDocument& );

	virtual void onDocumentSelectionChange( UICodeEditor*, TextDocument& );

	virtual void onDocumentCursorPosChange( UICodeEditor*, TextDocument& ){};

	virtual void onColorSchemeChanged( const std::string& ){};

	virtual void onDocumentLoaded( UICodeEditor*, const std::string& );

	virtual void onTabCreated( UITab*, UIWidget* ) {}

	void updateLayoutFunc( const InvalidationType& invalidator );
	void updateStyleSheetFunc( const InvalidationType& invalidator );
	void updateBaseStyleSheetFunc( const InvalidationType& invalidator );
	const std::string& getCurrentLayout() const;
	const std::string& getCurrentStyleSheet() const;
	const std::string& getBaseStyleSheet() const;
	void loadImage( std::string path );
	void loadFont( std::string path );
	void loadImagesFromFolder( std::string folderPath );
	void loadFontsFromFolder( std::string folderPath );
	void loadLayoutsFromFolder( std::string folderPath );
	void setUserDefaultTheme();
	void loadStyleSheet( std::string cssPath, bool updateCurrentStyleSheet = true );
	std::pair<UITab*, UICodeEditor*> loadLayout( std::string file,
												 bool updateCurrentLayout = true );
	void refreshLayout();
	void refreshStyleSheet();
	void onRecentProjectClick( const Event* event );
	void onRecentFilesClick( const Event* event );

	void loadConfig();
	void saveConfig();
	void unloadFonts();
	void unloadImages();

	void closeProject();
	void updateRecentProjects();
	void updateRecentFiles();
	void loadProject( std::string projectPath );
	void loadLayoutFile( std::string layoutPath );

	void resizeCb();
	void resizeWindowToLayout();

	UIWidget* createWidget( std::string widgetName );
	std::string pathFix( std::string path );
	void loadUITheme( std::string themePath );
	void onLayoutSelected( const Event* event );

	void refreshLayoutList();
	void loadProjectNodes( pugi::xml_node node );

	bool onCloseRequestCallback( EE::Window::Window* );

	void mainLoop();

	void imagePathOpen( const Event* event );
	void fontPathOpen( const Event* event );
	void styleSheetPathOpen( const Event* event );
	void layoutOpen( const Event* event );
	void projectOpen( const Event* event );

	void showFileDialog( const String& title, const std::function<void( const Event* )>& cb,
						 const std::string& filePattern = "*",
						 const Uint32& dialogFlags = UIFileDialog::Flags::FoldersFirst |
													 UIFileDialog::Flags::SortAlphabetically |
													 UIFileDialog::Flags::ShowHidden );

	void fileMenuClick( const Event* event );
	void createAppMenu();

	FontTrueType* loadFont( const std::string& name, std::string fontPath,
							const std::string& fallback = "" );

	void createWidgetInspector();

	std::string titleFromEditor( UICodeEditor* editor );

	void updateEditorTabTitle( UICodeEditor* editor );

	void updateEditorTitle( UICodeEditor* editor );

	void tryUpdateEditorTitle( UICodeEditor* editor );

	void closeEditors();

	void saveTmpDocument( TextDocument& doc, std::function<void( const std::string& )> action );

	void reloadLayout();

	void reloadStyleSheet();

	void reloadBaseStyleSheet();

	void toggleEditor();

	void showEditor( bool show );

	void createNewLayout();

	UIFileDialog* saveFileDialog( UICodeEditor* editor, bool focusOnClose = true );

	String i18n( const std::string& key, const String& def );

	void updateEditorState();

	void saveDoc();

	void saveAll();

	void saveAllProcess();

	void tryUpdateWatch( const std::string& file );

	void loadBaseStyleSheet();

  protected:
	EE::Window::Window* mWindow{ nullptr };
	UIMessageBox* mMsgBox{ nullptr };
	efsw::FileWatcher* mFileWatcher{ nullptr };
	UITheme* mTheme{ nullptr };
	UIWindow* mUIContainer{ nullptr };
	UIMenuBar* mUIMenuBar{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	UISceneNode* mAppUISceneNode{ nullptr };
	std::string mCurrentLayout;
	std::string mCurrentStyleSheet;
	std::string mBaseStyleSheet;
	Float mDisplayDPI{ 96.f };
	size_t mMenuIconSize{ 16 };
	bool mLayoutExpanded{ true };
	bool mUpdateLayout{ false };
	bool mUpdateStyleSheet{ false };
	bool mUpdateBaseStyleSheet{ false };
	bool mUseDefaultTheme{ false };
	bool mIsBundledApp{ false };
	InvalidationType mInvalidationLayout{ InvalidationType::None };
	InvalidationType mInvalidationStyleSheet{ InvalidationType::None };
	InvalidationType mInvalidationBaseStyleSheet{ InvalidationType::None };
	Sizef mProjectScreenSize;
	Clock mWaitClock;
	Clock mCssWaitClock;
	Clock mCssBaseWaitClock;
	efsw::WatchID mWatch{ 0 };
	efsw::WatchID mStyleSheetWatch{ 0 };
	efsw::WatchID mBaseStyleSheetWatch{ 0 };
	std::map<std::string, std::string> mWidgetRegistered;
	std::string mResPath;
	std::string mBasePath;
	UIConsole* mConsole{ nullptr };
	std::map<std::string, std::string> mLayouts;
	std::vector<std::string> mRecentProjects;
	std::vector<std::string> mRecentFiles;
	IniFile mIni;
	Uint32 mRecentProjectEventClickId{ 0xFFFFFFFF };
	Uint32 mRecentFilesEventClickId{ 0xFFFFFFFF };
	std::map<Uint32, TextureRegion*> mImagesLoaded;
	std::map<Font*, std::string> mFontsLoaded;
	UpdateListener* mListener{ nullptr };
	std::string mConfigPath;
	std::string mColorSchemesPath;
	UISplitter* mProjectSplitter{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };
	UILayout* mBaseLayout{ nullptr };
	UILayout* mPreviewLayout{ nullptr };
	UIWidget* mSidePanel{ nullptr };
	std::unordered_set<Doc::TextDocument*> mTmpDocs;

	Drawable* findIcon( const std::string& icon );
};

} // namespace uieditor

#endif // EE_UIEDITOR_HPP
