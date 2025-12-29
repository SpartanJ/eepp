#ifndef EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP
#define EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP

#include <eepp/ui/splitdirection.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uitabwidget.hpp>

#include <eepp/system/log.hpp>

using namespace EE::UI::Doc;

namespace EE { namespace UI { namespace Tools {

class EE_API UICodeEditorSplitter {
  public:
	static const std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings();

	static const std::map<KeyBindings::Shortcut, std::string> getLocalDefaultKeybindings();

	class EE_API Client {
	  public:
		virtual ~Client() {};

		virtual void onTabCreated( UITab* tab, UIWidget* widget ) = 0;

		virtual void onCodeEditorCreated( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onCodeEditorFocusChange( UICodeEditor* editor ) = 0;

		virtual void onWidgetFocusChange( UIWidget* widget ) = 0;

		virtual void onDocumentStateChanged( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentModified( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentSelectionChange( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentCursorPosChange( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onDocumentUndoRedo( UICodeEditor* editor, TextDocument& doc ) = 0;

		virtual void onColorSchemeChanged( const std::string& currentColorScheme ) = 0;

		virtual void onDocumentLoaded( UICodeEditor* codeEditor, const std::string& path ) = 0;
	};

	static std::vector<std::string> getUnlockedCommands();

	static UICodeEditorSplitter* New( UICodeEditorSplitter::Client* client, UISceneNode* sceneNode,
									  std::shared_ptr<ThreadPool> = nullptr,
									  const std::vector<SyntaxColorScheme>& colorSchemes = {},
									  const std::string& initColorScheme = "" );

	virtual ~UICodeEditorSplitter();

	virtual bool tryTabClose( UIWidget* widget, UITabWidget::FocusTabBehavior focusTabBehavior,
							  std::function<void()> onMsgBoxCloseCb = {} );

	virtual bool tryCodeEditorClose( UICodeEditor* editor,
									 UITabWidget::FocusTabBehavior focusTabBehavior,
									 std::function<void()> onMsgBoxCloseCb = {} );

	virtual bool tryCloseAllTabs( UIWidget* widget,
								  UITabWidget::FocusTabBehavior focusTabBehavior );

	virtual bool tryCloseOtherTabs( UIWidget* widget,
									UITabWidget::FocusTabBehavior focusTabBehavior );

	virtual bool tryCloseCleanTabs( UIWidget* widget,
									UITabWidget::FocusTabBehavior focusTabBehavior );

	virtual bool tryCloseTabsToDirection( UIWidget* widget,
										  UITabWidget::FocusTabBehavior focusTabBehavior,
										  bool toTheRight );

	void closeTab( UIWidget* widget, UITabWidget::FocusTabBehavior focusTabBehavior );

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

	UITab* tabFromEditor( UICodeEditor* editor ) const;

	UITabWidget* tabWidgetFromWidget( UIWidget* widget ) const;

	UISplitter* splitterFromEditor( UICodeEditor* editor ) const;

	UISplitter* splitterFromWidget( UIWidget* widget ) const;

	std::pair<UITab*, UICodeEditor*> createCodeEditorInTabWidget( UITabWidget* tabWidget );

	std::pair<UITab*, UIWidget*> createWidgetInTabWidget( UITabWidget* tabWidget, UIWidget* widget,
														  const std::string& tabName,
														  bool focus = true );

	std::pair<UITab*, UIWidget*> createWidget( UIWidget* widget, const std::string& tabName,
											   bool focus = true );

	std::vector<std::pair<UITab*, UITabWidget*>> getTabFromOwnedWidgetId( const std::string& id );

	bool ownedWidgetExists( UIWidget* widget );

	bool removeTabWithOwnedWidgetId( const std::string& id, bool destroyOwnedNode = true,
									 bool immediateClose = false );

	UICodeEditor* createCodeEditor();

	void focusSomeEditor( Node* searchFrom = nullptr );

	bool loadDocument( std::shared_ptr<TextDocument> doc, UICodeEditor* codeEditor = nullptr );

	std::pair<UITab*, UICodeEditor*> createEditorInNewTab();

	std::pair<UITab*, UICodeEditor*> loadDocumentInNewTab( std::shared_ptr<TextDocument> doc );

	bool loadFileFromPath( const std::string& path, UICodeEditor* codeEditor = nullptr );

	void loadAsyncFileFromPath( const std::string& path, UICodeEditor* codeEditor = nullptr,
								std::function<void( UICodeEditor*, const std::string& )> onLoaded =
									std::function<void( UICodeEditor*, const std::string& )>() );

	std::pair<UITab*, UICodeEditor*> loadFileFromPathInNewTab( const std::string& path );

	void loadAsyncFileFromPathInNewTab(
		const std::string& path, std::function<void( UICodeEditor*, const std::string& )> onLoaded =
									 std::function<void( UICodeEditor*, const std::string& )>() );

	void loadAsyncFileFromPathInNewTab(
		const std::string& path, std::function<void( UICodeEditor*, const std::string& )> onLoaded,
		UITabWidget* tabWidget );

	void removeUnusedTab( UITabWidget* tabWidge, bool destroyOwnedNode = true,
						  bool immediateClose = true );

	UITabWidget* createEditorWithTabWidget( Node* parent, bool openCurEditor = true );

	UITab* isDocumentOpen( const std::string& path, bool checkOnlyInCurrentTabWidget = false,
						   bool checkOpeningDocuments = false ) const;

	UITab* isDocumentOpen( const URI& uri, bool checkOnlyInCurrentTabWidget = false,
						   bool checkOpeningDocuments = false ) const;

	UICodeEditor* editorFromTab( UITab* tab ) const;

	UICodeEditor* findEditorFromPath( const std::string& path );

	void applyColorScheme( const SyntaxColorScheme& colorScheme );

	void forEachWidgetClass( const std::string& className,
							 std::function<void( UIWidget* )> run ) const;

	void forEachWidgetClassStoppable( const std::string& className,
									  std::function<bool( UIWidget* )> run ) const;

	void forEachWidgetType( const UINodeType& nodeType,
							std::function<void( UIWidget* )> run ) const;

	void forEachWidgetTypeStoppable( const UINodeType& nodeType,
									 std::function<bool( UIWidget* )> run ) const;

	void forEachWidgetStoppable( std::function<bool( UIWidget* )> run ) const;

	void forEachWidget( std::function<void( UIWidget* )> run ) const;

	void forEachEditor( std::function<void( UICodeEditor* )> run ) const;

	void forEachDoc( std::function<void( TextDocument& doc )> run ) const;

	void forEachDocSharedPtr( std::function<void( std::shared_ptr<TextDocument> doc )> run ) const;

	void forEachTabWidget( std::function<void( UITabWidget* )> run ) const;

	void forEachTabWidgetStoppable( std::function<bool( UITabWidget* )> run ) const;

	void forEachTab( std::function<void( UITab* )> run ) const;

	void zoomIn();

	void zoomOut();

	void zoomReset();

	void closeSplitter( UISplitter* splitter );

	void addRemainingTabWidgets( Node* widget );

	void closeTabWidgets( UISplitter* splitter );

	UICodeEditor* getCurEditor() const;

	bool curEditorIsNotNull() const;

	const SyntaxColorScheme& getCurrentColorScheme() const;

	const std::string& getCurrentColorSchemeName() const;

	void setColorScheme( const std::string& name );

	const std::map<std::string, SyntaxColorScheme>& getColorSchemes() const;

	bool editorExists( UICodeEditor* editor );

	bool isAnyEditorDirty();

	bool allEditorsEmpty();

	void forEachEditorStoppable( std::function<bool( UICodeEditor* )> run ) const;

	std::vector<UICodeEditor*> getAllEditors();

	void forEachDocStoppable( std::function<bool( TextDocument& )> run ) const;

	std::shared_ptr<TextDocument> findDocFromPath( const std::string& path );

	std::shared_ptr<TextDocument> findDocFromURI( const URI& uri );

	bool getHideTabBarOnSingleTab() const;

	void setHideTabBarOnSingleTab( bool hideTabBarOnSingleTab );

	void setHideTabBar( bool hideTabBar );

	const std::vector<UITabWidget*>& getTabWidgets() const;

	Node* getBaseLayout() const;

	UIWidget* getCurWidget() const;

	void setCurrentWidget( UIWidget* curWidget );

	bool curWidgetExists() const;

	bool isCurEditor( UICodeEditor* editor );

	UICodeEditor* getSomeEditor();

	size_t countEditorsOpeningDoc( const TextDocument& doc ) const;

	// T must implement setCommand( const std::string& command, const std::function<void()>& func )
	template <typename T> void registerSplitterCommands( T& t ) {
		t.setCommand( "switch-to-previous-split", [this] { switchPreviousSplit( mCurWidget ); } );
		t.setCommand( "switch-to-next-split", [this] { switchNextSplit( mCurWidget ); } );
		t.setCommand( "close-tab", [this] {
			if ( tryTabClose( mCurWidget, UITabWidget::FocusTabBehavior::Default ) )
				closeTab( mCurWidget, UITabWidget::FocusTabBehavior::Default );
		} );
		t.setCommand( "close-other-tabs", [this] {
			tryCloseOtherTabs( mCurWidget, UITabWidget::FocusTabBehavior::Default );
		} );
		t.setCommand( "close-all-tabs", [this] {
			tryCloseAllTabs( mCurWidget, UITabWidget::FocusTabBehavior::Default );
		} );
		t.setCommand( "close-clean-tabs", [this] {
			tryCloseCleanTabs( mCurWidget, UITabWidget::FocusTabBehavior::Default );
		} );
		t.setCommand( "close-tabs-to-the-left", [this] {
			tryCloseTabsToDirection( mCurWidget, UITabWidget::FocusTabBehavior::Default, false );
		} );
		t.setCommand( "close-tabs-to-the-right", [this] {
			tryCloseTabsToDirection( mCurWidget, UITabWidget::FocusTabBehavior::Default, true );
		} );
		t.setCommand( "create-new", [this] {
			auto d = createCodeEditorInTabWidget( getPreferredTabWidget() );
			if ( d.first != nullptr && d.second != nullptr ) {
				d.first->getTabWidget()->setTabSelected( d.first );
			} else if ( !mTabWidgets.empty() ) {
				d = createCodeEditorInTabWidget( mTabWidgets[0] );
			}
			if ( d.first == nullptr || d.second == nullptr )
				Log::error( "Couldn't createCodeEditorInTabWidget in create-new command" );
		} );
		t.setCommand( "next-tab", [this] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget )
				tabWidget->focusNextTab();
		} );
		t.setCommand( "previous-tab", [this] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget )
				tabWidget->focusPreviousTab();
		} );
		for ( int i = 1; i <= 10; i++ )
			t.setCommand( String::format( "switch-to-tab-%d", i ),
						  [this, i] { switchToTab( i - 1 ); } );
		t.setCommand( "switch-to-first-tab", [this] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget && tabWidget->getTabCount() ) {
				switchToTab( 0 );
			}
		} );
		t.setCommand( "switch-to-last-tab", [this] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget && tabWidget->getTabCount() ) {
				switchToTab( tabWidget->getTabCount() - 1 );
			}
		} );
		t.setCommand( "split-right", [this] {
			split( SplitDirection::Right, mCurWidget, curEditorExistsAndFocused() );
		} );
		t.setCommand( "split-bottom", [this] {
			split( SplitDirection::Bottom, mCurWidget, curEditorExistsAndFocused() );
		} );
		t.setCommand( "split-left", [this] {
			split( SplitDirection::Left, mCurWidget, curEditorExistsAndFocused() );
		} );
		t.setCommand( "split-top", [this] {
			split( SplitDirection::Top, mCurWidget, curEditorExistsAndFocused() );
		} );
		t.setCommand( "split-swap", [this] {
			if ( UISplitter* splitter = splitterFromWidget( mCurWidget ) )
				splitter->swap();
		} );
	}

	UISceneNode* getUISceneNode() const;

	bool curEditorExists() const;

	bool hasSplit() const;

	UIOrientation getMainSplitOrientation() const;

	void addCurrentPositionToNavigationHistory();

	void addEditorPositionToNavigationHistory( UICodeEditor* editor );

	void updateCurrentPositionInNavigationHistory();

	void goBackInNavigationHistory();

	void goForwardInNavigationHistory();

	void clearNavigationHistory();

	std::shared_ptr<ThreadPool> getThreadPool() const;

	void setThreadPool( const std::shared_ptr<ThreadPool>& threadPool );

	bool checkEditorExists( UICodeEditor* ) const;

	bool checkWidgetExists( UIWidget* ) const;

	void setOnTabWidgetCreateCb( std::function<void( UITabWidget* )> cb );

	bool getVisualSplitting() const;

	void setVisualSplitting( bool visualSplitting );

	Float getVisualSplitEdgePercent() const;

	void setVisualSplitEdgePercent( Float visualSplitEdgePercent );

	UITabWidget* splitTabWidget( SplitDirection, UITabWidget* );

	std::shared_ptr<TextDocument> getTextDocumentRef( TextDocument* doc );

	// @return True if can be removed
	typedef std::function<bool( UIWidget*, UITabWidget::FocusTabBehavior,
								std::function<void()> onMsgBoxCloseCb )>
		TabTryCloseCallback;

	void setTabTryCloseCallback( TabTryCloseCallback cb );

	bool isWidgetInAnyWidget( UIWidget* ) const;

	void setOpenDocumentsInMainSplit( bool open ) { mOpenDocumentsInMainSplit = open; }

	bool openDocumentsInMainSplit() const { return mOpenDocumentsInMainSplit; }

	UITabWidget* getFirstTabWidget() const;

	UITabWidget* getPreferredTabWidget() const;

  protected:
	UISceneNode* mUISceneNode{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	UICodeEditor* mCurEditor{ nullptr };
	UIWidget* mCurWidget{ nullptr };
	std::map<std::string, SyntaxColorScheme> mColorSchemes;
	std::string mCurrentColorScheme;
	std::vector<UITabWidget*> mTabWidgets;
	Node* mBaseLayout{ nullptr };
	Client* mClient;
	bool mHideTabBar{ false };
	bool mHideTabBarOnSingleTab{ true };
	bool mFirstCodeEditor{ true };
	bool mVisualSplitting{ true };
	bool mOpenDocumentsInMainSplit{ false };
	UICodeEditor* mAboutToAddEditor{ nullptr };
	UIMessageBox* mTryCloseMsgBox{ nullptr };
	Mutex mTabWidgetMutex;
	struct NavigationRecord {
		std::string path;
		TextPosition pos;
	};
	size_t mNavigationHistoryMaxSize{ 100 };
	std::vector<NavigationRecord> mNavigationHistory;
	size_t mNavigationHistoryPos{ std::numeric_limits<size_t>::max() };
	std::function<void( UITabWidget* )> mOnTabWidgetCreateCb;
	Float mVisualSplitEdgePercent{ 0.1 };
	TabTryCloseCallback mTabTryCloseCb;

	UICodeEditorSplitter( UICodeEditorSplitter::Client* client, UISceneNode* sceneNode,
						  std::shared_ptr<ThreadPool> threadPool,
						  const std::vector<SyntaxColorScheme>& colorSchemes,
						  const std::string& initColorScheme );

	virtual void onTabClosed( const TabEvent* tabEvent );

	void closeAllTabs( std::vector<UITab*> tabs, UITabWidget::FocusTabBehavior focusTabBehavior );

	UITabWidget* createTabWidget( Node* parent );

	void updateTabWidgetVisualSplitting();
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UICODEEDITORSPLITTER_HPP
