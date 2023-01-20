#ifndef SETTINGSMENU_HPP
#define SETTINGSMENU_HPP

#include "ecode.hpp"
#include <eepp/ee.hpp>

namespace ecode {

class SettingsMenu {
  public:
	void createSettingsMenu( App* app );

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

	void updateTerminalMenu();

	void updateProjectSettingsMenu();

	void toggleSettingsMenu();

	void updateDocumentMenu();

	void showProjectTreeMenu();

	void createProjectTreeMenu();

	void createProjectTreeMenu( const FileInfo& file );

	void updateColorSchemeMenu();

	void updateCurrentFileType();

	void updatedReopenClosedFileState();

	UIPopUpMenu* getWindowMenu() const;

	UIPopUpMenu* getSettingsMenu() const;

	void updateRecentFolders();

  protected:
	App* mApp{ nullptr };
	UIPopUpMenu* mSettingsMenu{ nullptr };
	UIPopUpMenu* mRecentFilesMenu{ nullptr };
	UITextView* mSettingsButton{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };
	UIPopUpMenu* mDocMenu{ nullptr };
	UIPopUpMenu* mTerminalMenu{ nullptr };
	UIPopUpMenu* mViewMenu{ nullptr };
	UIPopUpMenu* mWindowMenu{ nullptr };
	UIPopUpMenu* mRendererMenu{ nullptr };
	UIPopUpMenu* mToolsMenu{ nullptr };
	UIPopUpMenu* mProjectTreeMenu{ nullptr };
	UIPopUpMenu* mProjectMenu{ nullptr };
	std::vector<UIPopUpMenu*> mFileTypeMenues;
	Float mFileTypeMenuesCreatedWithHeight{ 0 };
	std::vector<UIPopUpMenu*> mColorSchemeMenues;
	Float mColorSchemeMenuesCreatedWithHeight{ 0 };
};

} // namespace ecode

#endif // SETTINGSMENU_HPP
