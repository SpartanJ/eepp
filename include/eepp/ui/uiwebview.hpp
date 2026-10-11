#ifndef EE_UIWEBVIEW_HPP
#define EE_UIWEBVIEW_HPP

#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/scene/event.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/layoutinvalidation.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uitextselectioncontroller.hpp>
#include <eepp/ui/webresourcecache.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace EE::Network;

namespace EE { namespace Network {
class CookieManager;
}} // namespace EE::Network

namespace EE { namespace UI {

class UIHTMLHtml;
class UIHTMLBody;
class UILayout;
class UISceneNode;

class EE_API UIWebView : public UIScrollView {
  public:
	/** The icon is empty when the replacement document has no favicon yet. */
	struct FaviconEvent : Scene::Event {
		TexturePtr icon;

		FaviconEvent( Node* node, TexturePtr icon = {} ) :
			Scene::Event( node, Event::OnFaviconChanged ), icon( std::move( icon ) ) {}
	};

	struct TitleEvent : Scene::Event {
		std::string title;

		TitleEvent( Node* node, std::string title ) :
			Scene::Event( node, Event::OnTitleChanged ), title( std::move( title ) ) {}
	};

	struct LinkOpenEvent : Scene::Event {
		URI uri;
		mutable bool handled{ false };

		LinkOpenEvent( Node* node, URI uri ) :
			Scene::Event( node, Event::OnLinkOpenRequested ), uri( std::move( uri ) ) {}

		void accept() const { handled = true; }
	};

	struct NavigationEvent : Scene::Event {
		URI uri;
		bool success{ false };
		std::string error;

		NavigationEvent( Node* node, const Uint32& eventType, const URI& ruri, bool succ = false,
						 std::string err = "" ) :
			Scene::Event( node, eventType ),
			uri( ruri ),
			success( succ ),
			error( std::move( err ) ) {}
	};

	static UIWebView* New();

	virtual ~UIWebView();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual UITextSelectionController* getTextSelectionController();

	virtual const UITextSelectionController* getTextSelectionController() const;

	/** Loads an embedded <html> child into the isolated document scene. Relative document
	 * resources use the containing scene's URI. Other children are not loaded as host widgets. */
	virtual void loadFromXmlNode( const pugi::xml_node& node );

	void loadURI( URI uri );

	void loadURI( URI uri, const std::string& method, const std::string& body,
				  const Http::Request::FieldTable& headers );

	void goHistoryBack();

	void goHistoryForward();

	void refresh();

	bool canGoBack() const;

	bool canGoForward() const;

	const std::vector<URI>& getHistory() const;

	int getHistoryIndex() const;

	const URI& getCurrentURI() const;

	const std::string& getTitle() const { return mTitle; }

	void reload();

	UIWidget* getDocumentContainer() const;

	UISceneNode* getDocumentSceneNode() const;

	const WebResourceCachePtr& getWebResourceCache() const;

	UIWebView* setWebResourceCache( WebResourceCachePtr cache, CachePartitionId partition = 0 );

	/** Share a cookie jar with other WebViews. Set before starting navigation. */
	UIWebView* setCookieManager( std::shared_ptr<EE::Network::CookieManager> manager );

	void setStyleSheetDefaultMarker( Uint32 marker );

	void setUserAgent( const std::string& userAgent );

	const std::string& getUserAgent() const;

	void setDefaultTimeout( const Time& timeout );

	void refreshDocumentLayout();

	void invalidateDocumentLayout( LayoutInvalidationFlags reasons );

	Uint32 onNavigationStarted( std::function<void( const URI& )> cb );

	Uint32 onNavigationCompleted( std::function<void( const URI& )> cb );

	Uint32 onNavigationError( std::function<void( const URI&, const std::string& )> cb );

	Uint32 onTitleChanged( std::function<void( const std::string& )> cb );

  protected:
	UIWebView();

	UISceneNode* mDocumentScene{ nullptr };
	UILayout* mDocumentLayout{ nullptr };
	UIWidget* mDocContainer{ nullptr };
	UITextSelectionController mTextSelectionController;
	Uint32 mScrollContainerSizeChangeCb{ 0 };
	Uint32 mVerticalScrollVisibleChangeCb{ 0 };
	Uint32 mHorizontalScrollVisibleChangeCb{ 0 };
	bool mUpdatingDocumentViewportMetrics{ false };
	bool mUpdatingDocumentContentExtent{ false };
	bool mDocumentExtentDirty{ true };
	LayoutInvalidationFlags mDocumentExtentDirtyReasons{ 0 };
	struct NavigationLoadState {
		UIWebView* owner{ nullptr };
		bool alive{ true };
		Uint64 generation{ 0 };
	};
	std::shared_ptr<NavigationLoadState> mNavigationLoadState;
	std::vector<URI> mHistory;
	int mHistoryIndex{ -1 };
	bool mIsLoading{ false };
	Uint64 mNavigationGeneration{ 0 };
	std::string mUserAgent;
	std::string mTitle;
	Time mDefaultTimeout{ Seconds( 30 ) };
	Time mWebResourceCachePruneElapsed;
	Uint32 mStyleSheetDefaultMarker{ 0 };

	void loadURI( URI uri, bool isHistoryNav );

	void loadURI( URI uri, bool isHistoryNav, const std::string& method, const std::string& body,
				  const Http::Request::FieldTable& headers );

	virtual void onSizeChange();
	virtual void onSceneChange();
	virtual void scheduledUpdate( const Time& time );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onMessage( const NodeMessage* message );
	virtual void onScrollViewSizeChange( const Event* event );

	void loadDocumentData( URI url, std::string data );

	void loadDocumentData( URI url, std::string data, Uint64 generation );

	void loadDocumentData( URI url, std::string data, Uint64 generation, bool documentIsXML );

	void
	loadDocumentAsync( const URI& url, const std::string& method = "GET",
					   const std::string& body = "",
					   const Http::Request::FieldTable& headers = Http::Request::FieldTable() );
	Uint64 beginNavigationLoad();
	bool isNavigationLoadCurrent( Uint64 generation ) const;
	static UIWebView* resolveNavigationLoad( const std::weak_ptr<NavigationLoadState>& state,
											 Uint64 generation );
	void pushHistory( const URI& url );
	void navigateToHistoryIndex( int index );
	Sizef getDocumentViewportPixelsSize() const;
	void onDocumentViewportGeometryChanged();
	void updateHTMLMinHeight( UIHTMLHtml* html, UIHTMLBody* body );
	void updateHTMLMinHeightForDocument();
	void markDocumentExtentDirty( LayoutInvalidationFlags reasons );
	bool updateDocumentViewportMetrics();
	void updateDocumentMetricsIfNeeded();
};

}} // namespace EE::UI

#endif
