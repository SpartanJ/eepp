#pragma once

#include <eepp/core/string.hpp>
#include <eepp/ui/doc/textrange.hpp>
#include <string>

namespace EE {

namespace Window {
class Window;
}

namespace Graphics {
class Drawable;
class Font;
} // namespace Graphics

namespace UI {

class UISplitter;
class UITabWidget;
class UISceneNode;
class UICodeEditor;

namespace Tools {
class UICodeEditorSplitter;
}

} // namespace UI

} // namespace EE

using namespace EE;
using namespace EE::Graphics;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace EE::UI::Doc;

namespace ecode {

class AppConfig;
class UIStatusBar;
class TerminalManager;
class UniversalLocator;
class GlobalSearchController;
class StatusTerminalController;
class StatusBuildOutputController;
class StatusAppOutputController;
class ProjectBuildManager;
class NotificationCenter;
class ProjectDirectoryTree;
struct TerminalConfig;
class UIMainLayout;

class PluginContextProvider {
  public:
	virtual UIStatusBar* getStatusBar() const = 0;

	virtual UISplitter* getMainSplitter() const = 0;

	virtual void hideGlobalSearchBar() = 0;

	virtual void hideSearchBar() = 0;

	virtual void hideLocateBar() = 0;

	virtual std::string getKeybind( const std::string& command ) = 0;

	virtual UniversalLocator* getUniversalLocator() const = 0;

	virtual TerminalManager* getTerminalManager() const = 0;

	virtual UICodeEditorSplitter* getSplitter() const = 0;

	virtual GlobalSearchController* getGlobalSearchController() const = 0;

	virtual StatusTerminalController* getStatusTerminalController() const = 0;

	virtual StatusBuildOutputController* getStatusBuildOutputController() const = 0;

	virtual StatusAppOutputController* getStatusAppOutputController() const = 0;

	virtual ProjectBuildManager* getProjectBuildManager() const = 0;

	virtual UITabWidget* getSidePanel() const = 0;

	virtual String i18n( const std::string& key, const String& def ) = 0;

	virtual const std::string& getWindowTitle() const = 0;

	virtual EE::Window::Window* getWindow() const = 0;

	virtual UISceneNode* getUISceneNode() const = 0;

	virtual NotificationCenter* getNotificationCenter() const = 0;

	virtual bool
	loadFileFromPath( std::string path, bool inNewTab = true, UICodeEditor* codeEditor = nullptr,
					  std::function<void( UICodeEditor*, const std::string& )> onLoaded =
						  std::function<void( UICodeEditor*, const std::string& )>(),
					  bool openBinaryAsDocument = false ) = 0;

	virtual ProjectDirectoryTree* getDirTree() const = 0;

	virtual Drawable* findIcon( const std::string& name ) = 0;

	virtual Drawable* findIcon( const std::string& name, const size_t iconSize ) = 0;

	virtual TerminalConfig& termConfig() = 0;

	virtual Font* getTerminalFont() const = 0;

	virtual Font* getFontMono() const = 0;

	virtual Font* getFallbackFont() const = 0;

	virtual const Float& getDisplayDPI() const = 0;

	virtual const std::string& getCurrentProject() const = 0;

	virtual std::string getCurrentWorkingDir() const = 0;

	virtual void focusOrLoadFile( const std::string& path, const TextRange& range = {},
								  bool searchInSameContext = false ) = 0;

	virtual void runCommand( const std::string& command ) = 0;

	virtual bool commandExists( const std::string& command ) const = 0;

	virtual UIMainLayout* getMainLayout() const = 0;

	virtual std::string getDefaultFileDialogFolder() const = 0;

	virtual AppConfig& getConfig() = 0;
};

} // namespace ecode
