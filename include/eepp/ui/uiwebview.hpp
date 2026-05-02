#ifndef EE_UIWEBVIEW_HPP
#define EE_UIWEBVIEW_HPP

#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/scene/event.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/uiscrollview.hpp>

#include <functional>
#include <string>
#include <vector>

using namespace EE::Network;

namespace EE { namespace UI {

class UIHTMLHtml;
class UIHTMLBody;

class EE_API UIWebView : public UIScrollView {
  public:
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

	void loadURI( URI uri );

	void loadURI( URI uri, const std::string& method, const std::string& body,
				  const Http::Request::FieldTable& headers );

	void goHistoryBack();

	void goHistoryForward();

	bool canGoBack() const;

	bool canGoForward() const;

	const std::vector<URI>& getHistory() const;

	int getHistoryIndex() const;

	const URI& getCurrentURI() const;

	void reload();

	UIWidget* getDocumentContainer() const;

	void setStyleSheetDefaultMarker( Uint32 marker );

	void setUserAgent( const std::string& userAgent );

	const std::string& getUserAgent() const;

	void setDefaultTimeout( const Time& timeout );

	Uint32 onNavigationStarted( std::function<void( const URI& )> cb );

	Uint32 onNavigationCompleted( std::function<void( const URI& )> cb );

	Uint32 onNavigationError( std::function<void( const URI&, const std::string& )> cb );

	Uint32 onTitleChanged( std::function<void( const std::string& )> cb );

  protected:
	UIWebView();

	UIWidget* mDocContainer{ nullptr };
	std::vector<URI> mHistory;
	int mHistoryIndex{ -1 };
	bool mIsLoading{ false };
	std::string mUserAgent;
	Time mDefaultTimeout{ Seconds( 30 ) };
	Uint32 mStyleSheetDefaultMarker{ 0 };

	void loadURI( URI uri, bool isHistoryNav );

	void loadURI( URI uri, bool isHistoryNav, const std::string& method, const std::string& body,
				  const Http::Request::FieldTable& headers );

	virtual void onSizeChange();

	void loadDocumentData( URI url, std::string data );
	void
	loadDocumentAsync( const URI& url, const std::string& method = "GET",
					   const std::string& body = "",
					   const Http::Request::FieldTable& headers = Http::Request::FieldTable() );
	void pushHistory( const URI& url );
	void navigateToHistoryIndex( int index );
	void updateHTMLMinHeight( UIHTMLHtml* html, UIHTMLBody* body );
	void updateHTMLMinHeightForDocument();
};

}} // namespace EE::UI

#endif
