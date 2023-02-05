#ifndef EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP
#define EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP

#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uitabwidget.hpp>

using namespace EE::UI::Doc;

namespace EE { namespace UI { namespace Tools {

class EE_API UICodeEditorSplitter {
  public:
	static const std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings();

	static const std::map<KeyBindings::Shortcut, std::string> getLocalDefaultKeybindings();

	enum class EE_API SplitDirection { Left, Right, Top, Bottom };

	class EE_API Client {
	  public:
		virtual ~Client(){};

		virtual void onCodeEditorCreated( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onCodeEditorFocusChange( UICodeEditor* editor ) = 0;

		virtual void onWidgetFocusChange( UIWidget* widget ) = 0;

		virtual void onDocumentStateChanged( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentModified( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentSelectionChange( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentCursorPosChange( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onColorSchemeChanged( const std::string& currentColorScheme ) = 0;

		virtual void onDocumentLoaded( UICodeEditor* codeEditor, const std::string& path ) = 0;
	};

	static std::vector<std::string> getUnlockedCommands();

	static UICodeEditorSplitter* New( UICodeEditorSplitter::Client* client, UISceneNode* sceneNode,
									  const std::vector<SyntaxColorScheme>& colorSchemes = {},
									  const std::string& initColorScheme = "" );

	virtual ~UICodeEditorSplitter();

	virtual bool tryTabClose( UIWidget* widget );

	void closeTab( UIWidget* widget );

	bool curEditorExistsAndFocused() const;

	UISplitter* split( const SplitDirection& direction, UIWidget* editor,
					   bool openCurEditor = true );

	void switchToTab( Int32 index );

	UITabWidget* findPreviousSplit( UIWidget* widget );

	void switchPreviousSplit( UIWidget* widget );

	UITabWidget* findNextSplit( UIWidget* widget );

	void switchNextSplit( UIWidget* widget );

	void setCurrentEditor( UICodeEditor* editor );

	UITabWidget* tabWidgetFromEditor( UICodeEditor* editor ) const;

	UITabWidget* tabWidgetFromWidget( UIWidget* widget ) const;

	UISplitter* splitterFromEditor( UICodeEditor* editor ) const;

	UISplitter* splitterFromWidget( UIWidget* widget ) const;

	std::pair<UITab*, UICodeEditor*> createCodeEditorInTabWidget( UITabWidget* tabWidget );

	std::pair<UITab*, UIWidget*> createWidgetInTabWidget( UITabWidget* tabWidget, UIWidget* widget,
														  const std::string& tabName,
														  bool focus = true );

	UICodeEditor* createCodeEditor();

	void focusSomeEditor( Node* searchFrom = nullptr );

	bool loadDocument( std::shared_ptr<TextDocument> doc, UICodeEditor* codeEditor = nullptr );

	std::pair<UITab*, UICodeEditor*> loadDocumentInNewTab( std::shared_ptr<TextDocument> doc );

	bool loadFileFromPath( const std::string& path, UICodeEditor* codeEditor = nullptr );

	void loadAsyncFileFromPath( const std::string& path, std::shared_ptr<ThreadPool> pool,
								UICodeEditor* codeEditor = nullptr,
								std::function<void( UICodeEditor*, const std::string& )> onLoaded =
									std::function<void( UICodeEditor*, const std::string& )>() );

	std::pair<UITab*, UICodeEditor*> loadFileFromPathInNewTab( const std::string& path );

	void loadAsyncFileFromPathInNewTab(
		const std::string& path, std::shared_ptr<ThreadPool> pool,
		std::function<void( UICodeEditor*, const std::string& )> onLoaded =
			std::function<void( UICodeEditor*, const std::string& )>() );

	void loadAsyncFileFromPathInNewTab(
		const std::string& path, std::shared_ptr<ThreadPool> pool,
		std::function<void( UICodeEditor*, const std::string& )> onLoaded, UITabWidget* tabWidget );

	void removeUnusedTab( UITabWidget* tabWidge, bool destroyOwnedNode = true,
						  bool immediateCloset = true );

	UITabWidget* createEditorWithTabWidget( Node* parent, bool openCurEditor = true );

	UITab* isDocumentOpen( std::string path, bool checkOnlyInCurrentTabWidget = false,
						   bool checkOpeningDocuments = false ) const;

	UICodeEditor* editorFromTab( UITab* tab ) const;

	UICodeEditor* findEditorFromPath( const std::string& path );

	void applyColorScheme( const SyntaxColorScheme& colorScheme );

	void forEachWidgetType( const UINodeType& nodeType,
							std::function<void( UIWidget* )> run ) const;

	void forEachWidgetTypeStoppable( const UINodeType& nodeType,
									 std::function<bool( UIWidget* )> run ) const;

	void forEachWidgetStoppable( std::function<bool( UIWidget* )> run ) const;

	void forEachWidget( std::function<void( UIWidget* )> run ) const;

	void forEachEditor( std::function<void( UICodeEditor* )> run ) const;

	void forEachDoc( std::function<void( TextDocument& doc )> run ) const;

	void forEachTabWidget( std::function<void( UITabWidget* )> run ) const;

	void zoomIn();

	void zoomOut();

	void zoomReset();

	void closeSplitter( UISplitter* splitter );

	void addRemainingTabWidgets( Node* widget );

	void closeTabWidgets( UISplitter* splitter );

	UICodeEditor* getCurEditor() const;

	const SyntaxColorScheme& getCurrentColorScheme() const;

	const std::string& getCurrentColorSchemeName() const;

	void setColorScheme( const std::string& name );

	const std::map<std::string, SyntaxColorScheme>& getColorSchemes() const;

	bool editorExists( UICodeEditor* editor ) const;

	bool isAnyEditorDirty();

	void forEachEditorStoppable( std::function<bool( UICodeEditor* )> run ) const;

	std::vector<UICodeEditor*> getAllEditors();

	void forEachDocStoppable( std::function<bool( TextDocument& )> run ) const;

	std::shared_ptr<TextDocument> findDocFromPath( const std::string& path );

	bool getHideTabBarOnSingleTab() const;

	void setHideTabBarOnSingleTab( bool hideTabBarOnSingleTab );

	const std::vector<UITabWidget*>& getTabWidgets() const;

	Node* getBaseLayout() const;

	UIWidget* getCurWidget() const;

	void setCurrentWidget( UIWidget* curWidget );

	bool curWidgetExists() const;

	UICodeEditor* getSomeEditor();

	size_t countEditorsOpeningDoc( const TextDocument& doc ) const;

	// T must implement setCommand( const std::string& command, const std::function<void()>& func )
	template <typename T> void registerSplitterCommands( T& t ) {
		t.setCommand( "switch-to-previous-split", [&] { switchPreviousSplit( mCurWidget ); } );
		t.setCommand( "switch-to-next-split", [&] { switchNextSplit( mCurWidget ); } );
		t.setCommand( "close-tab", [&] { tryTabClose( mCurWidget ); } );
		t.setCommand( "create-new", [&] {
			auto d = createCodeEditorInTabWidget( tabWidgetFromWidget( mCurWidget ) );
			d.first->getTabWidget()->setTabSelected( d.first );
		} );
		t.setCommand( "next-tab", [&] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget && tabWidget->getTabCount() > 1 ) {
				UITab* tab = (UITab*)mCurWidget->getData();
				Uint32 tabIndex = tabWidget->getTabIndex( tab );
				switchToTab( ( tabIndex + 1 ) % tabWidget->getTabCount() );
			}
		} );
		t.setCommand( "previous-tab", [&] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget && tabWidget->getTabCount() > 1 ) {
				UITab* tab = (UITab*)mCurWidget->getData();
				Uint32 tabIndex = tabWidget->getTabIndex( tab );
				Int32 newTabIndex = (Int32)tabIndex - 1;
				switchToTab( newTabIndex < 0 ? tabWidget->getTabCount() - newTabIndex
											 : newTabIndex );
			}
		} );
		for ( int i = 1; i <= 10; i++ )
			t.setCommand( String::format( "switch-to-tab-%d", i ),
						  [&, i] { switchToTab( i - 1 ); } );
		t.setCommand( "switch-to-first-tab", [&] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget && tabWidget->getTabCount() ) {
				switchToTab( 0 );
			}
		} );
		t.setCommand( "switch-to-last-tab", [&] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget && tabWidget->getTabCount() ) {
				switchToTab( tabWidget->getTabCount() - 1 );
			}
		} );
		t.setCommand( "split-right", [&] {
			split( SplitDirection::Right, mCurWidget, curEditorExistsAndFocused() );
		} );
		t.setCommand( "split-bottom", [&] {
			split( SplitDirection::Bottom, mCurWidget, curEditorExistsAndFocused() );
		} );
		t.setCommand( "split-left", [&] {
			split( SplitDirection::Left, mCurWidget, curEditorExistsAndFocused() );
		} );
		t.setCommand( "split-top", [&] {
			split( SplitDirection::Top, mCurWidget, curEditorExistsAndFocused() );
		} );
		t.setCommand( "split-swap", [&] {
			if ( UISplitter* splitter = splitterFromWidget( mCurWidget ) )
				splitter->swap();
		} );
	}

	UISceneNode* getUISceneNode() const;

	bool curEditorExists() const;

  protected:
	UISceneNode* mUISceneNode{ nullptr };
	UICodeEditor* mCurEditor{ nullptr };
	UIWidget* mCurWidget{ nullptr };
	std::map<std::string, SyntaxColorScheme> mColorSchemes;
	std::string mCurrentColorScheme;
	std::vector<UITabWidget*> mTabWidgets;
	Node* mBaseLayout{ nullptr };
	Client* mClient;
	bool mHideTabBarOnSingleTab{ true };
	bool mFirstCodeEditor{ true };
	UICodeEditor* mAboutToAddEditor{ nullptr };
	UIMessageBox* mTryCloseMsgBox{ nullptr };

	UICodeEditorSplitter( UICodeEditorSplitter::Client* client, UISceneNode* sceneNode,
						  const std::vector<SyntaxColorScheme>& colorSchemes,
						  const std::string& initColorScheme );

	bool checkEditorExists( UICodeEditor* ) const;

	virtual void onTabClosed( const TabEvent* tabEvent );
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP
