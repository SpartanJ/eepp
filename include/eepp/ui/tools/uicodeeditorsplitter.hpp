#ifndef EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP
#define EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP

#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uitabwidget.hpp>

using namespace EE::UI::Doc;

namespace EE { namespace UI { namespace Tools {

class EE_API UICodeEditorSplitter {
  public:
	static const std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings();

	static const std::map<KeyBindings::Shortcut, std::string> getLocalDefaultKeybindings();

	enum class SplitDirection { Left, Right, Top, Bottom };

	class EE_API Client {
	  public:
		virtual ~Client(){};

		virtual void onCodeEditorCreated( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onCodeEditorFocusChange( UICodeEditor* editor ) = 0;

		virtual void onDocumentStateChanged( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentModified( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentSelectionChange( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onColorSchemeChanged( const std::string& currentColorScheme ) = 0;

		virtual void onDocumentLoaded( UICodeEditor* codeEditor, const std::string& path ) = 0;
	};

	static UICodeEditorSplitter* New( UICodeEditorSplitter::Client* client, UISceneNode* sceneNode,
									  const std::vector<SyntaxColorScheme>& colorSchemes = {},
									  const std::string& initColorScheme = "" );

	virtual ~UICodeEditorSplitter();

	virtual bool tryTabClose( UICodeEditor* editor );

	void closeEditorTab( UICodeEditor* editor );

	void splitEditor( const SplitDirection& direction, UICodeEditor* editor );

	void switchToTab( Int32 index );

	UITabWidget* findPreviousSplit( UICodeEditor* editor );

	void switchPreviousSplit( UICodeEditor* editor );

	UITabWidget* findNextSplit( UICodeEditor* editor );

	void switchNextSplit( UICodeEditor* editor );

	void setCurrentEditor( UICodeEditor* editor );

	UITabWidget* tabWidgetFromEditor( UICodeEditor* editor );

	UISplitter* splitterFromEditor( UICodeEditor* editor );

	std::pair<UITab*, UICodeEditor*> createCodeEditorInTabWidget( UITabWidget* tabWidget );

	UICodeEditor* createCodeEditor();

	void focusSomeEditor( Node* searchFrom = nullptr );

	bool loadFileFromPath( const std::string& path, UICodeEditor* codeEditor = nullptr );

	void loadFileFromPathInNewTab( const std::string& path );

	void removeUnusedTab( UITabWidget* tabWidget );

	UITabWidget* createEditorWithTabWidget( Node* parent );

	void applyColorScheme( const SyntaxColorScheme& colorScheme );

	void forEachEditor( std::function<void( UICodeEditor* )> run );

	void zoomIn();

	void zoomOut();

	void zoomReset();

	void closeSplitter( UISplitter* splitter );

	void addRemainingTabWidgets( Node* widget );

	void closeTabWidgets( UISplitter* splitter );

	UICodeEditor* getCurEditor() const;

	const std::string& getCurrentColorScheme() const;

	void setColorScheme( const std::string& name );

	const std::map<std::string, SyntaxColorScheme>& getColorSchemes() const;

	bool editorExists( UICodeEditor* editor ) const;

  protected:
	UISceneNode* mUISceneNode{nullptr};
	UICodeEditor* mCurEditor{nullptr};
	std::map<std::string, SyntaxColorScheme> mColorSchemes;
	std::string mCurrentColorScheme;
	std::vector<UITabWidget*> mTabWidgets;
	Client* mClient;

	UICodeEditorSplitter( UICodeEditorSplitter::Client* client, UISceneNode* sceneNode,
						  const std::vector<SyntaxColorScheme>& colorSchemes,
						  const std::string& initColorScheme );

	virtual void onTabClosed( const TabEvent* tabEvent );
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP
