#ifndef EE_UIEDITOR_HPP
#define EE_UIEDITOR_HPP

#include <eepp/ui/iconmanager.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uiapplication.hpp>

#include <eepp/ee.hpp>
#include <efsw/efsw.hpp>

using namespace EE::UI;
using namespace EE::UI::Tools;

namespace uieditor {

class UpdateListener;

class App : public UICodeEditorSplitter::Client {
  public:
	App();

	~App();

	int init( const Float& pixelDensityConf, const bool& useAppTheme, std::string cssFile,
			  std::string xmlFile, std::string projectFile, const std::string& colorScheme );

	virtual void onCodeEditorCreated( UICodeEditor*, TextDocument& );

	virtual void onCodeEditorFocusChange( UICodeEditor* );

	virtual void onWidgetFocusChange( UIWidget* ) {};

	virtual void onDocumentStateChanged( UICodeEditor*, TextDocument& ) {};

	virtual void onDocumentModified( UICodeEditor*, TextDocument& );

	virtual void onDocumentUndoRedo( UICodeEditor*, TextDocument& );

	virtual void onDocumentSelectionChange( UICodeEditor*, TextDocument& );

	virtual void onDocumentCursorPosChange( UICodeEditor*, TextDocument& ) {};

	virtual void onColorSchemeChanged( const std::string& ) {};

	virtual void onDocumentLoaded( UICodeEditor*, const std::string& );

	virtual void onTabCreated( UITab*, UIWidget* ) {}

	void loadImage( std::string path );

	void loadFont( std::string path );

	void loadImagesFromFolder( std::string folderPath );

	void loadFontsFromFolder( std::string folderPath );

	void loadLayoutsFromFolder( std::string folderPath );

	void loadStyleSheet( std::string cssPath );

	std::pair<UITab*, UICodeEditor*> loadLayout( std::string file );

	void loadConfig();

	void saveConfig();

	void closeProject();

	void updateRecentMenu( bool projects );

	void loadProject( std::string projectPath );

	void loadLayoutFile( std::string layoutPath );

	void resizeCb();

	void resizeWindowToLayout();

	UIWidget* createWidget( const std::string& widgetName );
	std::string pathFix( std::string path );

	void loadUITheme( std::string themePath );

	void onLayoutSelected( const Event* event );

	void refreshLayoutList();

	void loadProjectNodes( pugi::xml_node node );

	bool onCloseRequestCallback( EE::Window::Window* );

	void update();

	void releaseEditor();

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

	void executeCommand( const std::string& command );

	void createKeyBindings();

	void createWidgetInspector();

	std::string titleFromEditor( UICodeEditor* editor );

	void updateEditorTabTitle( UICodeEditor* editor );

	void updateEditorTitle( UICodeEditor* editor );

	void tryUpdateEditorTitle( UICodeEditor* editor );

	void closeEditors();

	void toggleEditor();

	void showEditor( bool show );

	void createNewLayout();

	UIFileDialog* saveFileDialog( UICodeEditor* editor, bool focusOnClose = true );

	String i18n( const std::string& key, const String& def );

	void updateEditorState();

	void saveDoc();

	void saveAll();

	void saveAllProcess();

	void updateWatches();

	void invalidatePreview();

	void refreshPreview();

	void clearPreviewDocument();

	bool readSource( const std::string& path, std::string& source );

	std::pair<UITab*, UICodeEditor*> openDocument( const std::string& path );

	void loadBaseStyleSheet();

	void toggleAppTheme();

  protected:
	std::unique_ptr<UIApplication> mUIApplication;
	EE::Window::Window* mWindow{ nullptr };
	UIMessageBox* mMsgBox{ nullptr };
	efsw::FileWatcher* mFileWatcher{ nullptr };
	UpdateListener* mListener{ nullptr };
	UISceneNode* mAppUISceneNode{ nullptr };
	UISceneNode* mPreviewScene{ nullptr };
	UIWidget* mPreviewHost{ nullptr };
	UIMenuBar* mUIMenuBar{ nullptr };
	UIConsole* mConsole{ nullptr };
	UISplitter* mProjectSplitter{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };
	UILayout* mBaseLayout{ nullptr };
	UILayout* mPreviewLayout{ nullptr };
	UIWidget* mSidePanel{ nullptr };
	UIThemePtr mTheme;
	SmallVector<UIThemePtr, 2> mProjectThemes;
	SmallVector<std::string, 2> mRegisteredCallbacks;
	UnorderedMap<std::string, efsw::WatchID> mWatches;
	UnorderedMap<std::string, std::string> mWidgetRegistered;
	UnorderedMap<std::string, std::string> mLayouts;
	UnorderedSet<Doc::TextDocument*> mTmpDocs;
	std::vector<std::string> mRecentProjects;
	std::vector<std::string> mRecentFiles;
	std::string mCurrentLayout;
	std::string mCurrentStyleSheet;
	std::string mBaseStyleSheet;
	std::string mResPath;
	std::string mBasePath;
	std::string mConfigPath;
	std::string mColorSchemesPath;
	IniFile mIni;
	Clock mReloadClock;
	size_t mMenuIconSize{ 16 };
	Sizef mProjectScreenSize;
	Float mDisplayDPI{ 96.f };
	Uint32 mRecentProjectEventClickId{ 0xFFFFFFFF };
	Uint32 mRecentFilesEventClickId{ 0xFFFFFFFF };
	ColorSchemeExtPreference mUIColorScheme{ ColorSchemeExtPreference::System };
	bool mPreviewDirty{ false };
	bool mUseDefaultTheme{ true };
	bool mUseProjectViewport{ false };

	DrawablePtr findIcon( const std::string& icon );
};

} // namespace uieditor

#endif // EE_UIEDITOR_HPP
