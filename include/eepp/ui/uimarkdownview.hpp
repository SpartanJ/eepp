#ifndef EE_UI_UIMARKDOWNVIEW_HPP
#define EE_UI_UIMARKDOWNVIEW_HPP

#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uitextselectioncontroller.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>
#include <vector>

namespace EE { namespace UI {

class EE_API UIMarkdownView : public UILinearLayout {
  public:
	/** Emitted for every link before default handling. Accept to implement a custom flow.
	 * request.uri is resolved against this view's document, and target includes click modifiers. */
	struct LinkOpenEvent : Scene::Event {
		NavigationRequest request;
		mutable bool handled{ false };

		LinkOpenEvent( Node* node, NavigationRequest request ) :
			Scene::Event( node, Event::OnLinkOpenRequested ), request( std::move( request ) ) {}

		void accept() const { handled = true; }
	};

	struct NavigationEvent : Scene::Event {
		URI uri;
		bool success;

		NavigationEvent( Node* node, Uint32 type, URI uri, bool success ) :
			Scene::Event( node, type ), uri( std::move( uri ) ), success( success ) {}
	};

	static UIMarkdownView* New();

	UIMarkdownView();

	virtual ~UIMarkdownView();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual UITextSelectionController* getTextSelectionController();

	virtual const UITextSelectionController* getTextSelectionController() const;

	/** documentPath supplies the local source filename for relative links. An empty path uses
	 * the scene's base URI. Replacing the document cancels pending file navigation and emits
	 * OnDocumentChanged after loading the new content, including string updates. */
	void loadFromString( std::string_view markdown );

	void loadFromString( std::string_view markdown, const std::string& documentPath );

	/** Loads a local document, using the scene's worker pool when available. Failed reads preserve
	 * the current document and emit OnNavigationError; successful loads emit OnNavigationCompleted.
	 */
	void loadFromFile( const std::string& path );

	const std::string& getDocumentPath() const;

	UIMarkdownView* setFollowLocalLinks( bool follow );

	bool getFollowLocalLinks() const;

	/** Resolves a link using this view's source path, without changing the shared scene URI.
	 * Also accepts the relative file://docs/page.md spelling used by local Markdown documents. */
	URI resolveLink( URI uri ) const;

	/** Returns true when this view handles the request. Unhandled links use scene navigation.
	 * New-tab requests fall back to the same view unless a LinkOpenEvent listener accepts them. */
	bool navigate( const NavigationRequest& request );

	virtual void loadFromXmlNode( const pugi::xml_node& node );

  protected:
	UITextSelectionController mTextSelectionController;
	std::string mDocumentPath;
	struct NavigationLoadState {
		UIMarkdownView* owner;
		Uint64 generation{ 0 };
	};
	// Allocated only when file navigation is used, not for ordinary rendered Markdown strings.
	std::shared_ptr<NavigationLoadState> mNavigationLoadState;
	UISceneNode* mNavigationScene{ nullptr };
	bool mFollowLocalLinks{ true };

	void updateNavigationInterceptor();

	void loadDocument( std::string_view markdown, std::string documentPath );

	virtual void onSceneChange();

	virtual void scheduledUpdate( const Time& time );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onMessage( const NodeMessage* message );
};

/** Scrollable Markdown document with configurable command shortcuts. The owned Markdown view
 * exposes document loading, link policy, selection, and navigation events through
 * getMarkdownView(). Successful file navigation resets scrolling; string updates and failed loads
 * preserve it. */
class EE_API UIScrollableMarkdownView : public UIScrollView, public WidgetCommandExecuter {
  public:
	static UIScrollableMarkdownView* New();

	UIScrollableMarkdownView();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UIMarkdownView* getMarkdownView() const;

	/** History navigation is disabled by default. Enabling starts at the current named document
	 * and registers history commands and shortcuts. Disabling clears history and unregisters them;
	 * document loading and local link following remain available. */
	void setHistoryNavigationEnabled( bool enabled );

	bool isHistoryNavigationEnabled() const;

	/** Navigate named documents when history is enabled. Failed loads leave its position unchanged.
	 * Commands go-back and go-forward default to mod+[ and mod+], respectively. */
	void goHistoryBack();

	void goHistoryForward();

	bool canGoBack() const;

	bool canGoForward() const;

	const std::vector<std::string>& getHistory() const;

	Int32 getHistoryIndex() const;

	/** Loads inline Markdown text or CDATA into the owned view. Layout attributes apply to the
	 * scroll container; style the Markdown child separately with the MarkdownView selector. */
	virtual void loadFromXmlNode( const pugi::xml_node& node );

  protected:
	UIMarkdownView* mMarkdownView;
	std::vector<std::string> mHistory;
	EventConnection mHistoryMenuConnection;
	Int32 mHistoryIndex{ -1 };
	Int32 mPendingHistoryIndex{ -1 };
	bool mHistoryNavigationEnabled{ false };

	/** Override to load an application-owned buffer instead of reading the history path from disk.
	 * The replacement must use the same source path so OnDocumentChanged commits the traversal. */
	virtual void loadHistoryDocument( const std::string& path );

	void navigateToHistoryIndex( Int32 index );

	void updateHistory();

	void resetScrollPosition();

	void onCreateContextMenu( const Event* event );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onMessage( const NodeMessage* message );
};

}} // namespace EE::UI

#endif
