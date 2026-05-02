#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/network/cookiemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiwebview.hpp>

namespace EE { namespace UI {

UIWebView* UIWebView::New() {
	return eeNew( UIWebView, () );
}

UIWebView::UIWebView() : UIScrollView( "webview" ) {
	mDocContainer = UILinearLayout::NewVerticalWidthMatchParent( "webview::doc" );
	mDocContainer->setClipType( ClipType::None );
	mDocContainer->setFlags( UI_OWNS_CHILDREN_POSITION );
	mDocContainer->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	mDocContainer->setParent( this );
}

UIWebView::~UIWebView() {}

Uint32 UIWebView::getType() const {
	return UI_TYPE_WEBVIEW;
}

bool UIWebView::isType( const Uint32& type ) const {
	return UIWebView::getType() == type || UIScrollView::isType( type );
}

void UIWebView::onSizeChange() {
	UIScrollView::onSizeChange();

	auto ui = getUISceneNode();
	if ( !ui )
		return;

	auto htmlNode = ui->findByType( UI_TYPE_HTML_HTML );
	auto bodyNode = ui->findByType( UI_TYPE_HTML_BODY );
	if ( htmlNode && bodyNode ) {
		auto html = htmlNode->asType<UIHTMLHtml>();
		auto body = bodyNode->asType<UIHTMLBody>();
		updateHTMLMinHeight( html, body );
	}
}

void UIWebView::updateHTMLMinHeight( UIHTMLHtml* html, UIHTMLBody* body ) {
	Float h = PixelDensity::pxToDp( getPixelsSize().getHeight() );
	html->setMinHeight( h );
	body->setMinHeight( h );
	body->setPixelsSize( { html->getPixelsSize().getWidth(), 0 } );
	html->setPixelsSize( { html->getPixelsSize().getWidth(), 0 } );
}

void UIWebView::loadURI( URI uri ) {
	loadURI( uri, false, "GET", {}, {} );
}

void UIWebView::loadURI( URI uri, bool isHistoryNav ) {
	loadURI( uri, isHistoryNav, "GET", {}, {} );
}

void UIWebView::loadURI( URI uri, const std::string& method, const std::string& body,
						 const Http::Request::FieldTable& headers ) {
	loadURI( uri, false, method, body, headers );
}

void UIWebView::loadURI( URI uri, bool isHistoryNav, const std::string& method,
						 const std::string& body, const Http::Request::FieldTable& headers ) {
	if ( mIsLoading )
		return;
	mIsLoading = true;

	if ( !isHistoryNav )
		pushHistory( uri );

	NavigationEvent ev( this, Event::OnNavigationStarted, uri, true );
	sendEvent( &ev );

	if ( uri.getScheme() == "http" || uri.getScheme() == "https" ) {
		loadDocumentAsync( uri, method, body, headers );
	} else {
		std::string data;
		std::string path = uri.getPath().empty() ? uri.getFSPath() : uri.getPath();
		if ( uri.getScheme() == "file" || uri.getScheme().empty() ) {
			if ( path.empty() )
				path = uri.toString();
			FileSystem::fileGet( path, data );
		}
		loadDocumentData( uri, std::move( data ) );
	}
}

void UIWebView::loadDocumentAsync( const URI& url, const std::string& method,
								   const std::string& body,
								   const Http::Request::FieldTable& headers ) {
	auto ui = getUISceneNode();
	if ( !ui ) {
		mIsLoading = false;
		return;
	}

	auto reqHeaders = headers;
	auto& cookieManager = ui->getCookieManager();
	if ( cookieManager.hasCookie( url.getAuthority() ) ) {
		std::string cookieHeader = cookieManager.getCookieHeader( url.getAuthority() );
		if ( !cookieHeader.empty() )
			reqHeaders["Cookie"] = cookieHeader;
	}

	if ( !mUserAgent.empty() )
		reqHeaders["User-Agent"] = mUserAgent;

	Http::AsyncResponseCallback responseCb = [this, url, ui]( const Http&, Http::Request&,
															  Http::Response& response ) {
		if ( response.isOK() ) {
			std::string data = response.getBody();
			if ( response.hasField( "set-cookie" ) ) {
				ui->getCookieManager().storeCookiesFromHeader( url.getAuthority(),
															   response.getField( "set-cookie" ) );
			}
			loadDocumentData( url, std::move( data ) );
		} else {
			mIsLoading = false;
			NavigationEvent ev( this, Event::OnNavigationError, url, false,
								response.getStatusDescription() );
			sendEvent( &ev );
		}
	};

	Http::Request::ProgressCallback progressCb =
		[url, ui]( const Http&, const Http::Request&, const Http::Response& response,
				   const Http::Request::Status& status, std::size_t, std::size_t ) {
			if ( status == Http::Request::Status::Redirect && response.hasField( "set-cookie" ) ) {
				ui->getCookieManager().storeCookiesFromHeader( url.getAuthority(),
															   response.getField( "set-cookie" ) );
			}
			return true;
		};

	Http::requestAsync( responseCb, url, mDefaultTimeout,
						method == "POST" ? Http::Request::Method::Post : Http::Request::Method::Get,
						progressCb, reqHeaders, body, true, {} );
}

void UIWebView::loadDocumentData( URI url, std::string data ) {
	if ( data.empty() ) {
		mIsLoading = false;
		NavigationEvent ev( this, Event::OnNavigationError, url, false, "Empty response body" );
		sendEvent( &ev );
		return;
	}

	ensureMainThread( [this, url = std::move( url ), data = std::move( data )]() mutable {
		auto ui = getUISceneNode();
		if ( !ui ) {
			mIsLoading = false;
			return;
		}

		getVerticalScrollBar()->setValue( 0 );
		mDocContainer->closeAllChildren();

		ui->getStyleSheet().removeAllWithoutMarker( mStyleSheetDefaultMarker );
		ui->setURIFromURL( url );

		auto hash = String::hash( url.toString() );
		ui->loadLayoutFromString( Tools::HTMLFormatter::HTMLtoXML( data ), mDocContainer, hash );

		ui->setNavigationInterceptorCb( [this]( const NavigationRequest& request ) {
			URI uri = getUISceneNode()->solveRelativePath( request.uri );
			loadURI( uri, request.method != "GET", request.method, request.body,
					 request.extraHeaders );
			return true;
		} );

		mIsLoading = false;
		NavigationEvent ev( this, (Uint32)Event::OnNavigationCompleted, url, true );
		sendEvent( &ev );
		updateHTMLMinHeightForDocument();
	} );
}

void UIWebView::updateHTMLMinHeightForDocument() {
	auto ui = getUISceneNode();
	if ( !ui )
		return;

	auto htmlNode = ui->findByType( UI_TYPE_HTML_HTML );
	auto bodyNode = ui->findByType( UI_TYPE_HTML_BODY );
	if ( htmlNode && bodyNode ) {
		auto html = htmlNode->asType<UIHTMLHtml>();
		auto body = bodyNode->asType<UIHTMLBody>();
		updateHTMLMinHeight( html, body );
	}
}

void UIWebView::pushHistory( const URI& url ) {
	if ( mHistoryIndex >= 0 && mHistoryIndex < static_cast<int>( mHistory.size() ) - 1 )
		mHistory.resize( mHistoryIndex + 1 );

	if ( mHistory.empty() || mHistory.back().toString() != url.toString() ) {
		mHistory.push_back( url );
		mHistoryIndex = static_cast<int>( mHistory.size() ) - 1;
	}
}

void UIWebView::goHistoryBack() {
	if ( canGoBack() )
		navigateToHistoryIndex( mHistoryIndex - 1 );
}

void UIWebView::goHistoryForward() {
	if ( canGoForward() )
		navigateToHistoryIndex( mHistoryIndex + 1 );
}

bool UIWebView::canGoBack() const {
	return mHistoryIndex > 0;
}

bool UIWebView::canGoForward() const {
	return mHistoryIndex < static_cast<int>( mHistory.size() ) - 1;
}

const std::vector<URI>& UIWebView::getHistory() const {
	return mHistory;
}

int UIWebView::getHistoryIndex() const {
	return mHistoryIndex;
}

const URI& UIWebView::getCurrentURI() const {
	static URI empty;
	return mHistoryIndex >= 0 && mHistoryIndex < static_cast<int>( mHistory.size() )
			   ? mHistory[mHistoryIndex]
			   : empty;
}

void UIWebView::reload() {
	if ( mHistoryIndex >= 0 && mHistoryIndex < static_cast<int>( mHistory.size() ) )
		loadURI( mHistory[mHistoryIndex] );
}

UIWidget* UIWebView::getDocumentContainer() const {
	return mDocContainer;
}

void UIWebView::setStyleSheetDefaultMarker( Uint32 marker ) {
	mStyleSheetDefaultMarker = marker;
}

void UIWebView::setUserAgent( const std::string& userAgent ) {
	mUserAgent = userAgent;
}

const std::string& UIWebView::getUserAgent() const {
	return mUserAgent;
}

void UIWebView::setDefaultTimeout( const Time& timeout ) {
	mDefaultTimeout = timeout;
}

void UIWebView::navigateToHistoryIndex( int index ) {
	mHistoryIndex = index;
	loadURI( mHistory[mHistoryIndex], true );
}

Uint32 UIWebView::onNavigationStarted( std::function<void( const URI& )> cb ) {
	return on( Event::OnNavigationStarted,
			   [cb]( const Event* e ) { cb( static_cast<const NavigationEvent*>( e )->uri ); } );
}

Uint32 UIWebView::onNavigationCompleted( std::function<void( const URI& )> cb ) {
	return on( Event::OnNavigationCompleted,
			   [cb]( const Event* e ) { cb( static_cast<const NavigationEvent*>( e )->uri ); } );
}

Uint32 UIWebView::onNavigationError( std::function<void( const URI&, const std::string& )> cb ) {
	return on( Event::OnNavigationError, [cb]( const Event* e ) {
		auto ne = static_cast<const NavigationEvent*>( e );
		cb( ne->uri, ne->error );
	} );
}

Uint32 UIWebView::onTitleChanged( std::function<void( const std::string& )> cb ) {
	return on( Event::OnTitleChanged,
			   [cb]( const Event* e ) { cb( static_cast<const NavigationEvent*>( e )->error ); } );
}

}} // namespace EE::UI
