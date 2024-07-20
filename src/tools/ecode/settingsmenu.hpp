#ifndef SETTINGSMENU_HPP
#define SETTINGSMENU_HPP

#include "ecode.hpp"
#include <eepp/ee.hpp>

namespace ecode {

class SettingsMenu {
  public:
	void createSettingsMenu( App* app, UIMenuBar* menuBar );

	String i18n( const std::string& key, const String& def );

	std::string getKeybind( const std::string& command );

	Drawable* findIcon( const std::string& name );

	void runCommand( const std::string& command );

	UIMenu* createFileTypeMenu();

	UIMenu* createColorSchemeMenu();

	UIMenu* createDocumentMenu();

	UIMenu* createTerminalMenu();

	UIMenu* createEditMenu();

	UIMenu* createWindowMenu();

	UIMenu* createRendererMenu();

	UIMenu* createViewMenu();

	UIPopUpMenu* createToolsMenu();

	UIMenu* createHelpMenu();

	UIMenu* createThemesMenu();

	UIMenu* createLanguagesMenu();

	void updateTerminalMenu();

	void updateProjectSettingsMenu();

	void toggleSettingsMenu();

	void updateDocumentMenu();

	void updateViewMenu();

	void updateGlobalDocumentSettingsMenu();

	void showProjectTreeMenu();

	void createProjectTreeMenu();

	void createProjectTreeMenu( const FileInfo& file );

	void updateColorSchemeMenu();

	void updateCurrentFileType();

	void updatedReopenClosedFileState();

	UIPopUpMenu* getViewMenu() const;

	UIPopUpMenu* getWindowMenu() const;

	UIPopUpMenu* getSettingsMenu() const;

	UIPopUpMenu* getToolsMenu() const;

	UIPopUpMenu* getProjectMenu() const;

	UIPopUpMenu* getTerminalMenu() const;

	UIPopUpMenu* getDocMenu() const;

	UIPopUpMenu* getEditMenu() const;

	UIPopUpMenu* getHelpMenu() const;

	void updateRecentFolders();

	void deleteFileDialog( const FileInfo& file );

	void createProjectMenu();

	void updateMenu();

  protected:
	App* mApp{ nullptr };
	UIPopUpMenu* mSettingsMenu{ nullptr };
	UIPopUpMenu* mRecentFilesMenu{ nullptr };
	UIWidget* mSettingsButton{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };
	UIPopUpMenu* mDocMenu{ nullptr };
	UIPopUpMenu* mGlobalMenu{ nullptr };
	UIPopUpMenu* mTerminalMenu{ nullptr };
	UIPopUpMenu* mViewMenu{ nullptr };
	UIPopUpMenu* mWindowMenu{ nullptr };
	UIPopUpMenu* mRendererMenu{ nullptr };
	UIPopUpMenu* mToolsMenu{ nullptr };
	UIPopUpMenu* mProjectTreeMenu{ nullptr };
	UIPopUpMenu* mProjectDocMenu{ nullptr };
	UIPopUpMenu* mProjectMenu{ nullptr };
	UIPopUpMenu* mEditMenu{ nullptr };
	UIPopUpMenu* mHelpMenu{ nullptr };
	UIPopUpMenu* mLineWrapMenu{ nullptr };
	UIPopUpMenu* mCodeFoldingMenu{ nullptr };
	UIMenuBar* mMenuBar{ nullptr };
	std::vector<UIPopUpMenu*> mFileTypeMenues;
	Float mFileTypeMenuesCreatedWithHeight{ 0 };
	std::vector<UIPopUpMenu*> mColorSchemeMenues;
	Float mColorSchemeMenuesCreatedWithHeight{ 0 };
};

} // namespace ecode

#endif // SETTINGSMENU_HPP
