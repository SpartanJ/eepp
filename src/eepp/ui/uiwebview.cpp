#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/network/cookiemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uiwebview.hpp>

namespace EE { namespace UI {

class UIWebViewDocumentContainer : public UILinearLayout {
  public:
	static UIWebViewDocumentContainer* New() { return eeNew( UIWebViewDocumentContainer, () ); }

	void clearDocumentChildren() { childDeleteAll(); }

  protected:
	UIWebViewDocumentContainer() : UILinearLayout( "webview::doc", UIOrientation::Vertical ) {}
};

static void expandDocumentContentExtent( Node* node, Vector2f offset, Sizef& extent,
										 bool hasClip = false,
										 const Sizef& clipExtent = Sizef::Zero ) {
	if ( !node || node->isClosing() || !node->isVisible() )
		return;

	Vector2f childOffset( offset );
	bool childHasClip = hasClip;
	Sizef childClipExtent = clipExtent;
	if ( node->isWidget() ) {
		UIWidget* widget = node->asType<UIWidget>();
		childOffset += widget->getPixelsPosition();
		Sizef size = widget->fitMinMaxSizePx( widget->getPixelsSize() );
		Float right = childOffset.x + size.getWidth();
		Float bottom = childOffset.y + size.getHeight();
		if ( hasClip ) {
			right = eemin( right, clipExtent.getWidth() );
			bottom = eemin( bottom, clipExtent.getHeight() );
		}
		if ( widget->getLayoutWidthPolicy() != SizePolicy::MatchParent )
			extent.x = eemax( extent.x, right );
		if ( widget->getLayoutHeightPolicy() != SizePolicy::MatchParent )
			extent.y = eemax( extent.y, bottom );

		if ( widget->getClipType() != ClipType::None ) {
			Float clipRight = childOffset.x + widget->getPixelsSize().getWidth();
			Float clipBottom = childOffset.y + widget->getPixelsSize().getHeight();
			if ( hasClip ) {
				clipRight = eemin( clipRight, clipExtent.getWidth() );
				clipBottom = eemin( clipBottom, clipExtent.getHeight() );
			}
			childClipExtent = Sizef( clipRight, clipBottom );
			childHasClip = true;
		}
	}

	Node* child = node->getFirstChild();
	while ( child ) {
		expandDocumentContentExtent( child, childOffset, extent, childHasClip, childClipExtent );
		child = child->getNextNode();
	}
}
static void expandWidgetContentExtent( UIWidget* widget, const Vector2f& offset, Sizef& extent ) {
	if ( !widget || widget->isClosing() )
		return;

	const Vector2f position = offset + widget->getPixelsPosition();
	Sizef size = widget->fitMinMaxSizePx( widget->getPixelsSize() );
	if ( widget->getLayoutWidthPolicy() != SizePolicy::MatchParent )
		extent.x = eemax( extent.x, position.x + size.getWidth() );
	if ( widget->getLayoutHeightPolicy() != SizePolicy::MatchParent )
		extent.y = eemax( extent.y, position.y + size.getHeight() );
}

static bool syncWidgetDocumentPixelsSize( UIWidget* widget, const Sizef& size ) {
	if ( !widget || widget->isClosing() )
		return false;

	Sizef next( size.getWidth(), size.getHeight() );
	if ( next == widget->getPixelsSize() )
		return false;

	widget->setPixelsSize( next );
	return true;
}

static Float getDocumentBodyPixelsHeight( UIWidget* body, const Sizef& extent ) {
	return extent.getHeight();
}

static void syncDocumentBoxesPixelsSize( UIWidget* container, const Sizef& extent ) {
	if ( !container || extent == Sizef::Zero )
		return;

	UIWidget* html = nullptr;
	UIWidget* body = nullptr;
	if ( auto htmlNode = container->findByType( UI_TYPE_HTML_HTML ) ) {
		if ( htmlNode->isWidget() )
			html = htmlNode->asType<UIWidget>();
	}
	if ( auto bodyNode = container->findByType( UI_TYPE_HTML_BODY ) ) {
		if ( bodyNode->isWidget() )
			body = bodyNode->asType<UIWidget>();
	}

	syncWidgetDocumentPixelsSize( container, extent );
	syncWidgetDocumentPixelsSize( html, extent );
	if ( body ) {
		if ( body->isType( UI_TYPE_HTML_BODY ) )
			body->asType<UIHTMLBody>()->setDocumentCanvasMinHeight(
				PixelDensity::pxToDp( extent.getHeight() ) );
		syncWidgetDocumentPixelsSize(
			body, { extent.getWidth(), getDocumentBodyPixelsHeight( body, extent ) } );
	}
}

static Sizef computeDocumentContentExtent( UIWidget* container, const Sizef& viewport ) {
	Sizef extent( viewport.getWidth(), viewport.getHeight() );
	if ( !container )
		return extent;

	if ( auto htmlNode = container->findByType( UI_TYPE_HTML_HTML ) ) {
		UIWidget* html = htmlNode->asType<UIWidget>();
		expandWidgetContentExtent( html, Vector2f::Zero, extent );
	}

	if ( auto bodyNode = container->findByType( UI_TYPE_HTML_BODY ) ) {
		UIWidget* body = bodyNode->asType<UIWidget>();
		expandWidgetContentExtent( body, Vector2f::Zero, extent );
		expandDocumentContentExtent( body, container->getPixelsPosition(), extent );
	}

	return extent;
}

static void resetViewportDependentDocumentWidths( UIWidget* container ) {
	if ( !container )
		return;

	auto nodes = container->findAllByType( UI_TYPE_HTML_WIDGET );
	for ( Node* node : nodes ) {
		if ( node == container || !node->isWidget() )
			continue;

		UIWidget* widget = node->asType<UIWidget>();
		Node* parent = widget->getParent();
		UIWidget* parentWidget =
			parent && parent->isWidget() ? parent->asType<UIWidget>() : nullptr;
		if ( !parentWidget )
			continue;

		bool normalFlow = true;
		Rectf margin = widget->getLayoutPixelsMargin();
		if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
			auto* htmlWidget = widget->asType<UIHTMLWidget>();
			normalFlow = !htmlWidget->isOutOfFlow();
			margin = htmlWidget->getNormalFlowLayoutPixelsMargin();
		}

		if ( normalFlow ) {
			widget->invalidateIntrinsicSize();
		}
	}
}

Sizef UIWebView::getDocumentViewportPixelsSize() const {
	Sizef viewport = getPixelsSize();
	const Rectf& padding = getPixelsPadding();
	viewport.x -= padding.Left + padding.Right;
	viewport.y -= padding.Top + padding.Bottom;

	if ( getViewType() == ScrollViewType::Outside ) {
		if ( getVerticalScrollBar()->isVisible() )
			viewport.x -= getVerticalScrollBar()->getPixelsSize().getWidth();
		if ( getHorizontalScrollBar()->isVisible() )
			viewport.y -= getHorizontalScrollBar()->getPixelsSize().getHeight();
	}

	viewport.x = eemax( 0.f, viewport.x );
	viewport.y = eemax( 0.f, viewport.y );
	return viewport;
}

UIWebView* UIWebView::New() {
	return eeNew( UIWebView, () );
}

UIWebView::UIWebView() : UIScrollView( "webview" ) {
	mNavigationLoadState = std::make_shared<NavigationLoadState>();
	mNavigationLoadState->owner = this;

	mDocumentLayout = UILinearLayout::NewVerticalWidthMatchParent( "webview::document_layout" );
	mDocumentLayout->setClipType( ClipType::None );
	mDocumentLayout->setFlags( UI_OWNS_CHILDREN_POSITION );
	mDocumentLayout->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mDocumentLayout->setParent( this );

	mDocumentScene = UISceneNode::New();
	mDocumentScene->setFollowParentSize( false );
	mDocumentScene->setParent( mDocumentLayout );

	mDocContainer = UIWebViewDocumentContainer::New();
	mDocContainer->setClipType( ClipType::None );
	mDocContainer->setFlags( UI_OWNS_CHILDREN_POSITION );
	mDocContainer->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	mDocContainer->setParent( mDocumentScene->getRoot() );
	mDocContainer->setBackgroundColor( Color::White );
	mScrollContainerSizeChangeCb = mContainer->on( Event::OnSizeChange, [this]( const Event* ) {
		onDocumentViewportGeometryChanged();
		updateScroll();
	} );
	mVerticalScrollVisibleChangeCb = mVScroll->on(
		Event::OnVisibleChange, [this]( const Event* ) { onDocumentViewportGeometryChanged(); } );
	mHorizontalScrollVisibleChangeCb = mHScroll->on(
		Event::OnVisibleChange, [this]( const Event* ) { onDocumentViewportGeometryChanged(); } );
	mStyleSheetDefaultMarker = String::hash( "html_defaults" );
}

UIWebView::~UIWebView() {
	if ( mNavigationLoadState ) {
		mNavigationLoadState->alive = false;
		mNavigationLoadState->owner = nullptr;
		mNavigationLoadState->generation++;
	}
	if ( mDocumentScene )
		mDocumentScene->invalidateAsyncResourceLoads();
}

Uint32 UIWebView::getType() const {
	return UI_TYPE_WEBVIEW;
}

bool UIWebView::isType( const Uint32& type ) const {
	return UIWebView::getType() == type || UIScrollView::isType( type );
}

void UIWebView::onSizeChange() {
	UIScrollView::onSizeChange();
	onDocumentViewportGeometryChanged();
}

void UIWebView::updateHTMLMinHeight( UIHTMLHtml* html, UIHTMLBody* body ) {
	Sizef viewport = mDocumentScene ? mDocumentScene->getViewportPixelsSize() : getPixelsSize();
	const Float h = PixelDensity::pxToDp( viewport.getHeight() );
	const Rectf bodyMargin = body->getLayoutPixelsMargin();
	const Float bodyMarginHeight = PixelDensity::pxToDp( bodyMargin.Top + bodyMargin.Bottom );
	html->setMinHeight( h );
	html->setPixelsSize( viewport );
	body->setPixelsSize( { viewport.getWidth(), body->getPixelsSize().getHeight() } );
	body->setDocumentViewportMinHeight( eemax( 0.f, h - bodyMarginHeight ) );
}

void UIWebView::onSceneChange() {
	UIScrollView::onSceneChange();
	if ( mDocumentScene )
		mDocumentScene->initializeEmbeddedFromHost( getUISceneNode() );
}

void UIWebView::scheduledUpdate( const Time& time ) {
	UITouchDraggableWidget::scheduledUpdate( time );
	if ( mDocumentScene ) {
		mDocumentScene->update( time );
		updateDocumentMetricsIfNeeded();
		mWebResourceCachePruneElapsed += time;
		if ( mWebResourceCachePruneElapsed >= Seconds( 1 ) ) {
			mWebResourceCachePruneElapsed = Time::Zero;
			if ( const auto& cache = mDocumentScene->getWebResourceCache() )
				cache->prune();
		}
	}
}

void UIWebView::onScrollViewSizeChange( const Event* event ) {
	UIScrollView::onScrollViewSizeChange( event );
	onDocumentViewportGeometryChanged();
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
	Uint64 generation = beginNavigationLoad();
	if ( mDocumentScene )
		mDocumentScene->beginDocumentNavigation( uri );
	mIsLoading = true;

	if ( !isHistoryNav )
		pushHistory( uri );

	NavigationEvent ev( this, Event::OnNavigationStarted, uri, true );
	sendEvent( &ev );

	if ( uri.getScheme() == "http" || uri.getScheme() == "https" ) {
		loadDocumentAsync( uri, method, body, headers );
	} else {
		std::string data;
		if ( uri.getScheme() == "file" || uri.getScheme().empty() ) {
			std::string path = uri.getScheme() == "file"
								   ? uri.getFSPath()
								   : ( uri.getPath().empty() ? uri.toString() : uri.getPath() );
			if ( path.empty() )
				path = uri.toString();
			FileSystem::fileGet( path, data );
		}
		loadDocumentData( uri, std::move( data ), generation );
	}
}

Uint64 UIWebView::beginNavigationLoad() {
	mNavigationGeneration++;
	if ( mNavigationLoadState )
		mNavigationLoadState->generation = mNavigationGeneration;
	if ( mDocumentScene )
		mDocumentScene->invalidateAsyncResourceLoads();
	return mNavigationGeneration;
}

bool UIWebView::isNavigationLoadCurrent( Uint64 generation ) const {
	return mNavigationLoadState && mNavigationLoadState->alive &&
		   mNavigationLoadState->generation == generation && mNavigationGeneration == generation;
}

UIWebView* UIWebView::resolveNavigationLoad( const std::weak_ptr<NavigationLoadState>& state,
											 Uint64 generation ) {
	auto locked = state.lock();
	if ( !locked || !locked->alive || locked->generation != generation )
		return nullptr;
	return locked->owner;
}

void UIWebView::loadDocumentAsync( const URI& url, const std::string& method,
								   const std::string& body,
								   const Http::Request::FieldTable& headers ) {
	Uint64 generation = mNavigationGeneration;
	std::weak_ptr<NavigationLoadState> loadState( mNavigationLoadState );
	auto ui = getDocumentSceneNode();
	if ( !ui ) {
		mIsLoading = false;
		return;
	}

	auto reqHeaders = headers;
	if ( !mUserAgent.empty() )
		reqHeaders["User-Agent"] = mUserAgent;

	WebResourceRequest request;
	request.uri = url;
	request.kind = WebResourceKind::Document;
	request.method = method == "POST" ? Http::Request::Method::Post : Http::Request::Method::Get;
	request.headers = std::move( reqHeaders );
	request.body = body;
	request.timeout = mDefaultTimeout;
	ui->requestWebResource(
		std::move( request ), [loadState, generation, url]( const WebResourceResult& result ) {
			UIWebView* self = resolveNavigationLoad( loadState, generation );
			if ( !self )
				return;
			if ( result.success && result.data ) {
				std::string data( *result.data );
				self->loadDocumentData( url, std::move( data ), generation );
			} else {
				if ( !self->isNavigationLoadCurrent( generation ) )
					return;
				self->mIsLoading = false;
				NavigationEvent ev( self, Event::OnNavigationError, url, false, result.error );
				self->sendEvent( &ev );
			}
		} );
}

void UIWebView::loadDocumentData( URI url, std::string data ) {
	loadDocumentData( std::move( url ), std::move( data ), mNavigationGeneration );
}

void UIWebView::loadDocumentData( URI url, std::string data, Uint64 generation ) {
	if ( !isNavigationLoadCurrent( generation ) )
		return;

	if ( data.empty() ) {
		if ( isNavigationLoadCurrent( generation ) )
			mIsLoading = false;
		NavigationEvent ev( this, Event::OnNavigationError, url, false, "Empty response body" );
		sendEvent( &ev );
		return;
	}

	std::weak_ptr<NavigationLoadState> loadState( mNavigationLoadState );
	ensureMainThread( [loadState, generation, url = std::move( url ),
					   data = std::move( data )]() mutable {
		UIWebView* self = resolveNavigationLoad( loadState, generation );
		if ( !self )
			return;

		auto ui = self->getDocumentSceneNode();
		if ( !ui || !self->isNavigationLoadCurrent( generation ) )
			return;

		self->getVerticalScrollBar()->setValue( 0 );
		self->getHorizontalScrollBar()->setValue( 0 );
		static_cast<UIWebViewDocumentContainer*>( self->mDocContainer )->clearDocumentChildren();
		ui->invalidateAsyncResourceLoads();
		ui->clearFontFaces();
		ui->getStyleSheet().removeAllWithoutMarker( self->mStyleSheetDefaultMarker );
		ui->setURIFromURL( url );

		auto hash = String::hash( url.toString() );
		ui->loadLayoutFromString( Tools::HTMLFormatter::HTMLtoXML( data ), self->mDocContainer,
								  hash );

		ui->setNavigationInterceptorCb( [loadState]( const NavigationRequest& request ) {
			auto locked = loadState.lock();
			if ( !locked || !locked->alive || locked->owner == nullptr )
				return true;
			UIWebView* self = locked->owner;
			UISceneNode* docScene = self->getDocumentSceneNode();
			if ( !docScene )
				return true;
			URI uri = docScene->solveRelativePath( request.uri );
			self->loadURI( uri, request.method != "GET", request.method, request.body,
						   request.extraHeaders );
			return true;
		} );

		if ( !self->isNavigationLoadCurrent( generation ) )
			return;
		self->mIsLoading = false;
		NavigationEvent ev( self, (Uint32)Event::OnNavigationCompleted, url, true );
		self->sendEvent( &ev );
		self->markDocumentExtentDirty( LayoutInvalidation::Document );
		self->updateDocumentMetricsIfNeeded();
	} );
}

void UIWebView::onDocumentViewportGeometryChanged() {
	markDocumentExtentDirty( toLayoutInvalidationFlags( LayoutInvalidationReason::Viewport ) );
}

void UIWebView::updateHTMLMinHeightForDocument() {
	auto ui = getDocumentSceneNode();
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

void UIWebView::refresh() {
	navigateToHistoryIndex( mHistoryIndex );
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

UISceneNode* UIWebView::getDocumentSceneNode() const {
	return mDocumentScene;
}

const WebResourceCachePtr& UIWebView::getWebResourceCache() const {
	return mDocumentScene->getWebResourceCache();
}

UIWebView* UIWebView::setWebResourceCache( WebResourceCachePtr cache, CachePartitionId partition ) {
	if ( mDocumentScene )
		mDocumentScene->setWebResourceCache( std::move( cache ), partition );
	return this;
}

bool UIWebView::updateDocumentViewportMetrics() {
	if ( !mDocumentScene || !mDocumentLayout || !mDocContainer || mUpdatingDocumentViewportMetrics )
		return false;

	mUpdatingDocumentViewportMetrics = true;

	Sizef viewport = getDocumentViewportPixelsSize();
	bool changed = viewport != Sizef::Zero && viewport != mDocumentScene->getViewportPixelsSize();

	if ( viewport != Sizef::Zero ) {
		mDocumentScene->setViewportPixelsSize( viewport );
		mDocumentScene->setLayoutViewportPixelsSize( viewport );
	}

	if ( mDocumentLayout->getPixelsSize() == Sizef::Zero && viewport != Sizef::Zero )
		mDocumentLayout->setPixelsSize( viewport );

	mUpdatingDocumentViewportMetrics = false;
	return changed;
}

void UIWebView::markDocumentExtentDirty( LayoutInvalidationFlags reasons ) {
	mDocumentExtentDirty = true;
	mDocumentExtentDirtyReasons |= reasons;
}

void UIWebView::updateDocumentMetricsIfNeeded() {
	if ( !mDocumentScene || !mDocumentLayout || !mDocContainer || mUpdatingDocumentContentExtent ||
		 !mDocumentExtentDirty )
		return;

	mUpdatingDocumentContentExtent = true;
	LayoutInvalidationFlags requestedReasons = mDocumentExtentDirtyReasons;
	mDocumentExtentDirty = false;
	mDocumentExtentDirtyReasons = 0;
	bool viewportChanged = !mUpdatingDocumentViewportMetrics && updateDocumentViewportMetrics();

	Sizef viewport = mDocumentScene->getViewportPixelsSize();
	if ( viewport == Sizef::Zero )
		viewport = mContainer ? mContainer->getPixelsSize() : getPixelsSize();

	const bool documentReset =
		requestedReasons & toLayoutInvalidationFlags( LayoutInvalidationReason::DocumentExtent );
	if ( !viewportChanged && !documentReset ) {
		mUpdatingDocumentContentExtent = false;
		return;
	}

	if ( viewportChanged || documentReset ) {
		updateHTMLMinHeightForDocument();
		if ( mDocContainer->isLayout() )
			mDocContainer->asType<UILayout>()->setLayoutDirty( LayoutInvalidation::Document );
		if ( auto htmlNode = mDocumentScene->findByType( UI_TYPE_HTML_HTML ) ) {
			if ( htmlNode->isWidget() )
				resetViewportDependentDocumentWidths( htmlNode->asType<UIWidget>() );
			if ( htmlNode->isLayout() )
				htmlNode->asType<UILayout>()->setLayoutDirty( LayoutInvalidation::Document );
		}
	}

	mDocumentScene->flushDirtyStyleAndLayout();

	auto applyDocumentExtent = [&]() {
		Sizef extent = computeDocumentContentExtent( mDocContainer, viewport );
		bool extentChanged = extent != Sizef::Zero && extent != mDocumentLayout->getPixelsSize();
		if ( extentChanged ) {
			mDocumentLayout->setPixelsSize( extent );
			mDocumentScene->setPixelsSize( extent );
			containerUpdate();
			updateScroll();
		}
		syncDocumentBoxesPixelsSize( mDocContainer, extent );
		return extentChanged;
	};

	if ( applyDocumentExtent() ) {
		mDocumentScene->flushDirtyStyleAndLayout();
		applyDocumentExtent();
	}

	mUpdatingDocumentContentExtent = false;
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

void UIWebView::refreshDocumentLayout() {
	invalidateDocumentLayout( LayoutInvalidation::Document |
							  toLayoutInvalidationFlags( LayoutInvalidationReason::Style ) );
}

void UIWebView::invalidateDocumentLayout( LayoutInvalidationFlags reasons ) {
	bool docExtent =
		reasons & ( toLayoutInvalidationFlags( LayoutInvalidationReason::DocumentExtent ) |
					toLayoutInvalidationFlags( LayoutInvalidationReason::Viewport ) );
	if ( docExtent ) {
		markDocumentExtentDirty( reasons );
	}
	if ( mDocContainer && mDocContainer->isLayout() )
		mDocContainer->asType<UILayout>()->setLayoutDirty( reasons );
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
