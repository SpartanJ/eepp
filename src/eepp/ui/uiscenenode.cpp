#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/system/virtualfilesystem.hpp>
#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uiroot.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using namespace EE::Network;

namespace EE { namespace UI {

static void refreshWebViewDocumentLayoutAfterStyleChange( UIWidget* root ) {
	if ( !root )
		return;

	// A stylesheet combine is a document-level style mutation: inherited font metrics,
	// table sizing, percentage constraints, and root/body dimensions can all change after the
	// first HTML layout pass. UIWebView already performs this html/body reset when the viewport
	// size changes; deferred CSS needs the same document-boundary invalidation even if the
	// viewport did not move. Keeping it scoped to WebView documents avoids reopening the generic
	// RichText parent-recompute storm, while the final html dirty mark gives the normal layout
	// queue one coalesced pass from the document root.
	auto webViews = root->findAllByType( UI_TYPE_WEBVIEW );
	for ( auto webViewNode : webViews ) {
		auto* webView = webViewNode->asType<UIWebView>();
		webView->invalidateDocumentLayout(
			LayoutInvalidation::Document |
			toLayoutInvalidationFlags( LayoutInvalidationReason::Style ) );
	}

	auto htmls = root->findAllByType( UI_TYPE_HTML_HTML );
	for ( auto html : htmls ) {
		if ( html->isLayout() )
			html->asType<UILayout>()->setLayoutDirty( LayoutInvalidation::Document );
	}
}

UISceneNode* UISceneNode::New( EE::Window::Window* window ) {
	return eeNew( UISceneNode, ( window ) );
}

UISceneNode::UISceneNode( EE::Window::Window* window ) :
	SceneNode( window ),
	mRoot( NULL ),
	mIsLoading( false ),
	mUpdatingLayouts( false ),
	mUIThemeManager( UIThemeManager::New() ),
	mUIIconThemeManager( UIIconThemeManager::New()->setFallbackThemeManager( mUIThemeManager ) ),
	mAsyncResourceLoadState( std::make_shared<AsyncResourceLoadState>() ),
	mKeyBindings( mWindow->getInput() ) {
	// Reset size since the SceneNode already set it but needs to set the size from zero to emit
	// the required events to its children.
	mSize = Sizef();
	mDpSize = Sizef();

	// Update only UI elements that requires it.
	setUpdateAllChildren( false );

	mNodeFlags |= NODE_FLAG_UISCENENODE | NODE_FLAG_OVER_FIND_ALLOWED;

	setEventDispatcher( UIEventDispatcher::New( this ) );

	mRoot = UIRoot::New();
	mRoot->setParent( this )->setPosition( 0, 0 )->setId( "uiscenenode_root_node" );
	mRoot->enableReportSizeChangeToChildren();

	resizeNode( mWindow );
}

UISceneNode::~UISceneNode() {
	if ( mAsyncResourceLoadState ) {
		mAsyncResourceLoadState->alive = false;
		mAsyncResourceLoadState->generation++;
	}

	eeSAFE_DELETE( mUIThemeManager );
	eeSAFE_DELETE( mUIIconThemeManager );

	for ( auto& font : mFontFaces ) {
		FontManager::instance()->remove( font );
	}

	// UISceneNode can now destroy the ThreadPool shared to him. If that's the case,
	// We need to ensure that the children are destroyed before the thread pool,
	// since its children could be consuming it and need to uninitialize gracefully.
	childDeleteAll();

	if ( !mOwnsEventDispatcher )
		mEventDispatcher = nullptr;
}

void UISceneNode::resizeNode( EE::Window::Window* ) {
	if ( mParentNode )
		return;
	setPixelsSize( mWindow->getSize().asFloat() );
	onMediaChanged();
	sendMsg( this, NodeMessage::WindowResize );
}

void UISceneNode::resetTooltips( Node* node ) {
	if ( node->isWidget() ) {
		UIWidget* widget = node->asType<UIWidget>();

		if ( NULL != widget->getTooltip() ) {
			widget->getTooltip()->resetTextToStringBuffer();
			widget->getTooltip()->setVisible( false );
		}
	}

	Node* child = node->getFirstChild();

	while ( NULL != child ) {
		resetTooltips( child );
		child = child->getNextNode();
	}
}

void UISceneNode::onDrawDebugDataChange() {
	if ( !mDrawDebugData ) {
		resetTooltips( mRoot );
	}
}

Node* UISceneNode::setFocus( NodeFocusReason reason ) {
	if ( NULL != getEventDispatcher() )
		getEventDispatcher()->setFocusNode( mRoot, reason );
	return this;
}

void UISceneNode::nodeToWorldTranslation( Vector2f& Pos ) const {
	Node* ParentLoop = mParentNode;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->isUINode()
										? ParentLoop->asType<UINode>()->getPixelsPosition()
										: ParentLoop->getPosition();

		Pos += ParentPos;

		ParentLoop = ParentLoop->getParent();
	}
}

void UISceneNode::onParentChange() {
	SceneNode::onParentChange();

	if ( mCurParent && mCurOnSizeChangeListener )
		mCurParent->removeEventListener( mCurOnSizeChangeListener );
	mCurOnSizeChangeListener = 0;

	if ( !mCurParent && mOwnsEventDispatcher )
		eeSAFE_DELETE( mEventDispatcher );

	mCurParent = mParentNode;

	if ( !mParentNode ) {
		setEventDispatcher( UIEventDispatcher::New( this ) );
		mOwnsEventDispatcher = true;
		return;
	}

	initializeEmbeddedFromHost( getHostUISceneNode() );

	setDirty();
	updateParentSizeListener();
}

void UISceneNode::onSceneChange() {
	mSceneNode = this;
	eeASSERT( !removeFromCloseQueue( this ) );
	initializeEmbeddedFromHost( getHostUISceneNode() );

	Node* child = getFirstChild();
	while ( NULL != child ) {
		child->onSceneChange();
		child = child->getNextNode();
	}
}

UISceneNode* UISceneNode::getHostUISceneNode() const {
	const Node* node = getParent();
	while ( node ) {
		SceneNode* sceneNode = node->getSceneNode();
		if ( sceneNode && sceneNode->isUISceneNode() && sceneNode != this )
			return static_cast<UISceneNode*>( sceneNode );
		if ( node->isUISceneNode() && node != this )
			return const_cast<Node*>( node )->asType<UISceneNode>();
		node = node->getParent();
	}
	return nullptr;
}

void UISceneNode::initializeEmbeddedFromHost( UISceneNode* hostScene ) {
	if ( !hostScene || hostScene == this )
		return;

	mWindow = hostScene->getWindow();
	mDPI = hostScene->getDPI();
	mEventDispatcher = hostScene->getEventDispatcher();
	mOwnsEventDispatcher = false;
	mThreadPool = hostScene->getThreadPool();
	mColorSchemePreference = hostScene->getColorSchemePreference();
	mContrastPreference = hostScene->getContrastPreference();

	UIThemeManager* hostThemeManager = hostScene->getUIThemeManager();
	if ( hostThemeManager ) {
		mUIThemeManager->setDefaultFont( hostThemeManager->getDefaultFont() );
		mUIThemeManager->setDefaultFontSize( hostThemeManager->getDefaultFontSize() );
		mUIThemeManager->setDefaultTheme( hostThemeManager->getDefaultTheme() );
		mUIThemeManager->setAutoApplyDefaultTheme( hostThemeManager->getAutoApplyDefaultTheme() );
		mUIThemeManager->setDefaultEffectsEnabled( hostThemeManager->getDefaultEffectsEnabled() );
		mUIThemeManager->setWidgetsFadeInTime( hostThemeManager->getWidgetsFadeInTime() );
		mUIThemeManager->setWidgetsFadeOutTime( hostThemeManager->getWidgetsFadeOutTime() );
		mUIThemeManager->setTooltipTimeToShow( hostThemeManager->getTooltipTimeToShow() );
		mUIThemeManager->setTooltipFollowMouse( hostThemeManager->getTooltipFollowMouse() );
		mUIThemeManager->setCursorSize( hostThemeManager->getCursorSize() );
	}

	mUIIconThemeManager->setFallbackThemeManager( mUIThemeManager );
}

void UISceneNode::updateParentSizeListener() {
	if ( !mParentNode || !mFollowParentSize )
		return;

	setPixelsSize( getParent()->getPixelsSize() );
	mCurOnSizeChangeListener = getParent()->on( Event::OnSizeChange, [this]( const Event* ) {
		setDirty();
		setPixelsSize( getParent()->getPixelsSize() );
		onMediaChanged();
		sendMsg( this, NodeMessage::WindowResize );
	} );
}

void UISceneNode::setTranslator( Translator translator ) {
	mTranslator = translator;
}

void UISceneNode::setTranslator( Translator&& translator ) {
	mTranslator = std::move( translator );
}

const Translator& UISceneNode::getTranslator() const {
	return mTranslator;
}

Translator& UISceneNode::getTranslator() {
	return mTranslator;
}

String UISceneNode::getTranslatorString( const std::string& str ) {
	if ( str.size() >= 8 && String::startsWith( str, "@string" ) ) {
		if ( str[7] == '/' ) {
			String tstr = mTranslator.getString( str.substr( 8 ) );

			if ( !tstr.empty() )
				return tstr;
		} else if ( str[7] == '(' ) {
			FunctionString fun( FunctionString::parse( str ) );
			if ( !fun.isEmpty() ) {
				String tstr( mTranslator.getString( fun.getParameters()[0] ) );
				if ( !tstr.empty() )
					return tstr;
				if ( fun.getParameters().size() >= 2 )
					return fun.getParameters()[1];
			}
		}
	}
	return String( str );
}

String UISceneNode::getTranslatorString( const std::string& str, const String& defaultValue ) {
	if ( str.size() >= 8 && String::startsWith( str, "@string" ) ) {
		if ( str[7] == '/' ) {
			return mTranslator.getString( str.substr( 8 ), defaultValue );
		} else if ( str[7] == '(' ) {
			FunctionString fun( FunctionString::parse( str ) );
			if ( !fun.isEmpty() ) {
				if ( !defaultValue.empty() )
					return mTranslator.getString( fun.getParameters()[0], defaultValue );
				if ( fun.getParameters().size() >= 2 )
					return mTranslator.getString( fun.getParameters()[0], fun.getParameters()[1] );
			}
		}
	}
	return defaultValue;
}

String UISceneNode::getTranslatorStringFromKey( const std::string& key,
												const String& defaultValue ) {
	return mTranslator.getString( key, defaultValue );
}

String UISceneNode::i18n( const std::string& key, const String& defaultValue ) {
	return getTranslatorStringFromKey( key, defaultValue );
}

void UISceneNode::setFocusLastWindow( UIWindow* window ) {
	if ( NULL == mParentNode && NULL != mEventDispatcher && !mWindowsList.empty() &&
		 window != mWindowsList.front() ) {
		mEventDispatcher->setFocusNode( mWindowsList.front() );
	}
}

void UISceneNode::windowAdd( UIWindow* win ) {
	if ( !windowExists( win ) ) {
		mWindowsList.insert( mWindowsList.begin(), win );
		WindowEvent wevent( this, win, Event::OnWindowAdded );
		sendEvent( &wevent );
	} else {
		//! Send to front
		auto found = std::find( mWindowsList.begin(), mWindowsList.end(), win );
		if ( found != mWindowsList.end() ) {
			mWindowsList.erase( found );
			mWindowsList.insert( mWindowsList.begin(), win );
		}
	}
}

void UISceneNode::windowRemove( UIWindow* win ) {
	if ( windowExists( win ) ) {
		WindowEvent wevent( this, win, Event::OnWindowRemoved );
		sendEvent( &wevent );
		auto found = std::find( mWindowsList.begin(), mWindowsList.end(), win );
		if ( found != mWindowsList.end() )
			mWindowsList.erase( found );
	}
}

bool UISceneNode::windowExists( UIWindow* win ) {
	return mWindowsList.end() != std::find( mWindowsList.begin(), mWindowsList.end(), win );
}

std::vector<UIWidget*> UISceneNode::loadNode( pugi::xml_node node, Node* parent,
											  const Uint32& marker ) {
	Uint32 oldMarker = mCurrentMarker;
	mCurrentMarker = marker;

	std::vector<UIWidget*> rootWidgets;

	if ( NULL == parent )
		parent = this;

	Clock clock;
	for ( pugi::xml_node widget = node; widget; widget = widget.next_sibling() ) {
		clock.restart();

		if ( String::iequals( widget.name(), "style" ) ) {
			CSS::StyleSheetParser parser;
			parser.setBaseURI( mURI );
			std::string styleContent;
			for ( pugi::xml_node child = widget.first_child(); child;
				  child = child.next_sibling() ) {
				if ( child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata ) {
					styleContent += child.value();
				}
			}

			if ( parser.loadFromString( std::string_view{ styleContent } ) ) {
				parser.getStyleSheet().setMarker( marker );
				combineStyleSheet( parser.getStyleSheet(), false );
			}
			continue;
		} else if ( String::iequals( widget.name(), "link" ) ) {
			auto type = widget.attribute( "type" );
			auto href = widget.attribute( "href" );
			auto rel = widget.attribute( "rel" );
			auto defer = widget.attribute( "defer" );
			if ( !href.empty() &&
				 ( String::iequals( type.value(), "text/css" ) ||
				   String::icontains( std::string_view{ rel.value() }, "stylesheet" ) ) ) {
				loadCSS( href.as_string(), Milliseconds( defer.as_int() ) );
			}
			continue;
		} else if ( String::iequals( widget.name(), "meta" ) ) {
			// Ignored for now
			continue;
		} else if ( String::iequals( widget.name(), "title" ) ) {
			// Ignored for now
			continue;
		} else if ( String::iequals( widget.name(), "script" ) ) {
			// No plans to support it
			continue;
		}

		UIWidget* uiwidget = UIWidgetCreator::createFromName( widget.name() );

		if ( NULL != uiwidget ) {
			rootWidgets.push_back( uiwidget );

			uiwidget->setParent( parent );
			uiwidget->loadFromXmlNode( widget );
			uiwidget->getUIStyle()->applyInheritedProperties();

			if ( mVerbose ) {
				std::string name( widget.name() );
				pugi::xml_attribute idAttr( widget.attribute( "id" ) );
				pugi::xml_attribute classAttr( widget.attribute( "class" ) );

				if ( !idAttr.empty() ) {
					name += "#" + std::string( idAttr.as_string() );
				}

				if ( !classAttr.empty() ) {
					std::string classes( String::trim( std::string( classAttr.as_string() ) ) );
					String::replaceAll( classes, " ", "." );
					name += "." + classes;
				}

				mTimes.push_back( std::make_pair<Float, std::string>(
					clock.getElapsedTime().asMilliseconds(), std::string( name ) ) );
			}

			if ( widget.first_child() && !uiwidget->loadsItsChildren() ) {
				loadNode( widget.first_child(), uiwidget, marker );
			}

			uiwidget->onWidgetCreated();
		}
	}

	mCurrentMarker = oldMarker;
	return rootWidgets;
}

UIWidget* UISceneNode::loadLayoutNodes( pugi::xml_node node, Node* parent, const Uint32& marker ) {
	Clock clock;
	UISceneNode* prevUISceneNode = SceneManager::instance()->getUISceneNode();
	SceneManager::instance()->setCurrentUISceneNode( this );
	std::string id( node.attribute( "id" ).as_string() );
	mIsLoading = true;
	Clock innerClock;
	std::vector<UIWidget*> widgets = loadNode( node, parent, marker );

	if ( mVerbose ) {
		std::sort(
			mTimes.begin(), mTimes.end(),
			[]( const std::pair<Float, std::string>& left,
				const std::pair<Float, std::string>& right ) { return left.first < right.first; } );

		for ( auto& time : mTimes ) {
			Log::debug( "Widget %s created in %.2f ms", time.second.c_str(), time.first );
		}

		mTimes.clear();

		Log::debug( "UISceneNode::loadLayoutNodes loaded nodes%s in: %.2f ms",
					id.empty() ? "" : std::string( " (id=" + id + ")" ).c_str(),
					innerClock.getElapsedTimeAndReset().asMilliseconds() );
	}

	bool styleChangedDuringLoad = mStyleDuringLoad;
	if ( styleChangedDuringLoad ) {
		updateStyleSheet( false );
		mStyleDuringLoad = false;
	}

	for ( auto& widget : widgets )
		widget->reloadStyle( true, true, true );

	if ( styleChangedDuringLoad )
		refreshWebViewDocumentLayoutAfterStyleChange( mRoot );

	if ( mVerbose ) {
		Log::debug( "UISceneNode::loadLayoutNodes reloaded styles in: %.2f ms",
					innerClock.getElapsedTimeAndReset().asMilliseconds() );
	}

	mIsLoading = false;

	SceneManager::instance()->setCurrentUISceneNode( prevUISceneNode );

	if ( mVerbose ) {
		Log::debug( "UISceneNode::loadLayoutNodes loaded in: %.2f ms",
					clock.getElapsedTime().asMilliseconds() );
	}

	return widgets.empty() ? NULL : widgets[0];
}

void UISceneNode::setStyleSheet( const CSS::StyleSheet& styleSheet, bool loadStyle ) {
	mStyleSheet = styleSheet;
	processStyleSheetAtRules( styleSheet );
	if ( loadStyle ) {
		onMediaChanged();
		reloadStyle();
	}
}

void UISceneNode::setStyleSheet( const std::string& inlineStyleSheet ) {
	CSS::StyleSheetParser parser;

	if ( parser.loadFromString( inlineStyleSheet ) )
		setStyleSheet( parser.getStyleSheet() );
}

void UISceneNode::updateStyleSheet( bool forceReloadStyle ) {
	bool mediaChanged = false;
	if ( !mStyleSheet.isMediaQueryListEmpty() &&
		 mStyleSheet.updateMediaLists( getMediaFeatures() ) ) {
		mediaChanged = true;
	}

	if ( mRoot && mRoot->getUIStyle() )
		mRoot->getUIStyle()->resetGlobalDefinition();

	auto bodies = mRoot->findAllByType( UI_TYPE_HTML_BODY );
	for ( auto body : bodies )
		body->asType<UIWidget>()->getUIStyle()->resetGlobalDefinition();

	auto htmls = mRoot->findAllByType( UI_TYPE_HTML_HTML );
	for ( auto html : htmls )
		html->asType<UIWidget>()->getUIStyle()->resetGlobalDefinition();

	if ( mRoot && mediaChanged )
		mRoot->reportStyleStateChangeRecursive( false, false );

	if ( forceReloadStyle )
		reloadStyle();
}

void UISceneNode::combineStyleSheet( const CSS::StyleSheet& styleSheet, bool forceReloadStyle,
									 URI baseURI ) {
	mStyleSheet.combineStyleSheet( styleSheet );

	processStyleSheetAtRules( styleSheet, baseURI );

	if ( mIsLoading ) {
		mStyleDuringLoad = true;
		return;
	}

	updateStyleSheet( forceReloadStyle );
	if ( forceReloadStyle )
		refreshWebViewDocumentLayoutAfterStyleChange( mRoot );
}

void UISceneNode::combineStyleSheet( const std::string& inlineStyleSheet, bool forceReloadStyle,
									 const Uint32& marker, URI baseURI ) {
	CSS::StyleSheetParser parser;
	parser.setBaseURI( baseURI );

	if ( parser.loadFromString( inlineStyleSheet ) ) {
		parser.getStyleSheet().setMarker( marker );
		resolveStyleSheetRelativeURLs( parser.getStyleSheet(), baseURI.empty() ? mURI : baseURI );
		combineStyleSheet( parser.getStyleSheet(), forceReloadStyle, baseURI );
	}
}

CSS::StyleSheet& UISceneNode::getStyleSheet() {
	return mStyleSheet;
}

bool UISceneNode::hasStyleSheet() {
	return !mStyleSheet.isEmpty();
}

void UISceneNode::reloadStyle( bool disableAnimations, bool forceReApplyProperties,
							   bool resetPropertiesCache ) {
	if ( NULL != mChild ) {
		Node* child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				child->asType<UIWidget>()->reloadStyle(
					true, disableAnimations, true, forceReApplyProperties, resetPropertiesCache );
			}

			child = child->getNextNode();
		}
	}
}

bool UISceneNode::hasThreadPool() const {
	return mThreadPool != nullptr;
}

std::shared_ptr<ThreadPool> UISceneNode::getThreadPool() {
	return mThreadPool;
}

void UISceneNode::setThreadPool( const std::shared_ptr<ThreadPool>& threadPool ) {
	mThreadPool = threadPool;
}

static std::string getErrorContext( size_t offset, std::string_view content ) {
	static constexpr std::size_t CONTEXT_LENGTH = 50;
	std::size_t minVal = offset >= CONTEXT_LENGTH ? offset - CONTEXT_LENGTH : 0;
	std::size_t maxVal = offset + CONTEXT_LENGTH;
	std::size_t left = std::max( static_cast<std::size_t>( 0ul ), minVal );
	std::size_t right = std::min( content.size(), maxVal );
	return std::string{ content.substr( left, right - left ) };
}

UIWidget* UISceneNode::loadLayoutFromFile( const std::string& layoutPath, Node* parent,
										   const Uint32& marker ) {
	if ( FileSystem::fileExists( layoutPath ) ) {
		pugi::xml_document doc;
		pugi::xml_parse_result result =
			doc.load_file( layoutPath.c_str(), pugi::parse_default | pugi::parse_ws_pcdata );

		if ( result ) {
			return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this, marker );
		} else {
			Log::error( "Couldn't load UI Layout: %s", layoutPath.c_str() );
			Log::error( "Error description: %s", result.description() );
			Log::error( "Error offset: %d", result.offset );
			std::string data;
			FileSystem::fileGet( layoutPath, data );
			Log::error( "Error context: %s", getErrorContext( result.offset, data ) );
		}
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string path( layoutPath );
		Pack* pack = PackManager::instance()->exists( path );

		if ( NULL != pack ) {
			return loadLayoutFromPack( pack, path, parent );
		}
	}

	return NULL;
}

UIWidget* UISceneNode::loadLayoutFromString( const char* layoutString, Node* parent,
											 const Uint32& marker ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result =
		doc.load_string( layoutString, pugi::parse_default | pugi::parse_ws_pcdata );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this, marker );
	} else {
		Log::error( "Couldn't load UI Layout from string: %s", layoutString );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
		Log::error( "Error context: %s", getErrorContext( result.offset, layoutString ) );
	}

	return NULL;
}

UIWidget* UISceneNode::loadLayoutFromString( const std::string& layoutString, Node* parent,
											 const Uint32& marker ) {
	return loadLayoutFromString( layoutString.c_str(), parent, marker );
}

UIWidget* UISceneNode::loadLayoutFromMemory( const void* buffer, Int32 bufferSize, Node* parent,
											 const Uint32& marker ) {
	pugi::xml_document doc;
	std::string_view layoutString( static_cast<const char*>( buffer ), bufferSize );
	pugi::xml_parse_result result =
		doc.load_buffer( buffer, bufferSize, pugi::parse_default | pugi::parse_ws_pcdata );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this, marker );
	} else {
		Log::error( "Couldn't load UI Layout from memory: %s", layoutString.data() );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
		Log::error( "Error context: %s",
					getErrorContext( result.offset,
									 std::string_view{ static_cast<const char*>( buffer ),
													   static_cast<std::size_t>( bufferSize ) } ) );
	}

	return NULL;
}

UIWidget* UISceneNode::loadLayoutFromStream( IOStream& stream, Node* parent,
											 const Uint32& marker ) {
	if ( !stream.isOpen() )
		return NULL;

	ios_size bufferSize = stream.getSize();
	TScopedBuffer<char> scopedBuffer( bufferSize );
	stream.read( scopedBuffer.get(), scopedBuffer.length() );

	pugi::xml_document doc;
	std::string_view layoutString( scopedBuffer.get(), scopedBuffer.length() );
	std::string_view contents;
	pugi::xml_parse_result result = doc.load_buffer( scopedBuffer.get(), scopedBuffer.length(),
													 pugi::parse_default | pugi::parse_ws_pcdata );
	contents = std::string_view( scopedBuffer.get(), scopedBuffer.length() );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this, marker );
	} else {
		Log::error( "Couldn't load UI Layout from stream: %s", layoutString.data() );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
		Log::error( "Error context: %s", getErrorContext( result.offset, contents ) );
	}

	return NULL;
}

UIWidget* UISceneNode::loadLayoutFromPack( Pack* pack, const std::string& FilePackPath,
										   Node* parent ) {
	ScopedBuffer buffer;

	if ( pack->isOpen() && pack->extractFileToMemory( FilePackPath, buffer ) ) {
		return loadLayoutFromMemory( buffer.get(), buffer.length(), parent );
	}

	return NULL;
}

void UISceneNode::setInternalSize( const Sizef& size ) {
	if ( size != mDpSize ) {
		mDpSize = size;
		mSize = PixelDensity::dpToPx( size );
		updateCenter();
		onSizeChange();
		sendCommonEvent( Event::OnSizeChange );
		invalidateDraw();
	}
}

Node* UISceneNode::setSize( const Sizef& Size ) {
	if ( Size != mDpSize ) {
		Vector2f sizeChange( Size.x - mDpSize.x, Size.y - mDpSize.y );

		setInternalSize( Size );

		if ( reportSizeChangeToChildren() ) {
			sendParentSizeChange( sizeChange );
		}
	}

	return this;
}

Node* UISceneNode::setSize( const Float& Width, const Float& Height ) {
	return setSize( Vector2f( Width, Height ) );
}

const Sizef& UISceneNode::getSize() const {
	return mDpSize;
}

UISceneNode* UISceneNode::setPixelsSize( const Sizef& size ) {
	if ( size != mSize ) {
		Vector2f sizeChange( size.x - mSize.x, size.y - mSize.y );

		setInternalPixelsSize( size );

		if ( reportSizeChangeToChildren() ) {
			sendParentSizeChange( PixelDensity::pxToDp( sizeChange ) );
		}
	}

	return this;
}

UISceneNode* UISceneNode::setPixelsSize( const Float& x, const Float& y ) {
	return setPixelsSize( Sizef( x, y ) );
}

void UISceneNode::setViewportPixelsSize( const Sizef& size ) {
	if ( mHasViewportPixelsSize && mViewportPixelsSize == size )
		return;

	mViewportPixelsSize = size;
	mHasViewportPixelsSize = true;
	mRoot->setPixelsSize( getPixelsSize() );
	onViewportPixelsSizeChange();
}

void UISceneNode::clearViewportPixelsSize() {
	if ( !mHasViewportPixelsSize )
		return;

	mHasViewportPixelsSize = false;
	mRoot->setPixelsSize( getPixelsSize() );
	onViewportPixelsSizeChange();
}

void UISceneNode::onViewportPixelsSizeChange() {
	onMediaChanged();
	reloadStyle( true, true, true );
	sendMsg( this, NodeMessage::WindowResize );
}

const Sizef& UISceneNode::getViewportPixelsSize() const {
	return mHasViewportPixelsSize ? mViewportPixelsSize : getPixelsSize();
}

void UISceneNode::setFollowParentSize( bool followParentSize ) {
	if ( mFollowParentSize == followParentSize )
		return;

	mFollowParentSize = followParentSize;
	if ( mCurParent && mCurOnSizeChangeListener ) {
		mCurParent->removeEventListener( mCurOnSizeChangeListener );
		mCurOnSizeChangeListener = 0;
	}
	updateParentSizeListener();
}

bool UISceneNode::followsParentSize() const {
	return mFollowParentSize;
}

void UISceneNode::update( const Time& elapsed ) {
	UISceneNode* uiSceneNode = SceneManager::instance()->getUISceneNode();

	if ( mFirstUpdate && mVerbose ) {
		mClock.restart();
	}

	SceneManager::instance()->setCurrentUISceneNode( this );

	updateDirtyStyles();
	updateDirtyStyleStates();
	updateDirtyLayouts();

	if ( mFirstUpdate && mVerbose ) {
		Log::debug( "UISceneNode::update first update dirty took: %.2f ms",
					mClock.getElapsedTime().asMilliseconds() );
	}

	SceneNode::update( elapsed );

	if ( mFirstUpdate && mVerbose ) {
		Log::debug( "UISceneNode::update first SceneNode::update update took: %.2f ms",
					mClock.getElapsedTime().asMilliseconds() );
	}

	// We process again all the dirty states since the update could have created new dirty states
	// that we want to process BEFORE drawing the scene, since we can avoid some resizes/animations
	// glitches. Also after the SceneNode::update (having run updated the actions, responded to
	// events, and updating the nodes means that new nodes could have been added and need to be
	// ready before being drawn. Also the reverse case could happen, we need to have the styles and
	// layouts updated before and after the update to avoid weird issues. The cost of doing this is
	// minimal and the benefit is huge and simplifies implementation.
	// invalidationDepth allows to retry to apply any pending state as many times as set.
	// This is required in some very edge cases where widgets are being created during the update
	// of any of these 3 steps. Usually during the layout update, this could trigger resizes that
	// provokes the creation of dynamic elements. This is the case of the UIListBox for example
	// that creates children dynamically only when they are visible.
	int invalidationDepth = mMaxInvalidationDepth;
	while ( ( !mDirtyStyle.empty() || !mDirtyStyleState.empty() || !mDirtyLayouts.empty() ) &&
			invalidationDepth > 0 ) {
		updateDirtyStyles();
		updateDirtyStyleStates();
		updateDirtyLayouts();
		invalidationDepth--;
	}

	SceneManager::instance()->setCurrentUISceneNode( uiSceneNode );

	if ( mFirstUpdate && mVerbose ) {
		mFirstUpdate = false;
		Log::debug( "UISceneNode::update first update took: %.2f ms",
					mClock.getElapsedTime().asMilliseconds() );
	}
}

void UISceneNode::onWidgetDelete( Node* node ) {
	if ( node->isWidget() ) {
		UIWidget* widget = node->asType<UIWidget>();

		if ( node->isLayout() ) {
			mDirtyLayouts.erase( node->asType<UILayout>() );
		}

		mDirtyStyle.erase( widget );

		mDirtyStyleState.erase( widget );
	}
}

bool UISceneNode::isLoading() const {
	return mIsLoading;
}

UIThemeManager* UISceneNode::getUIThemeManager() const {
	return mUIThemeManager;
}

void UISceneNode::setTheme( UITheme* theme ) {
	setTheme( theme, mRoot );
}

void UISceneNode::setTheme( UITheme* theme, Node* to ) {
	to->forEachChild( [theme, this]( Node* node ) {
		if ( node->isWidget() )
			node->asType<UIWidget>()->setTheme( theme );
		setTheme( theme, node );
	} );
}

UIWidget* UISceneNode::getRoot() const {
	return mRoot;
}

void UISceneNode::invalidateStyle( UIWidget* node, bool tryReinsert ) {
	eeASSERT( NULL != node );

	if ( node->isClosing() )
		return;

	bool alreadyExists = ( mDirtyStyle.count( node ) > 0 );
	if ( alreadyExists && !tryReinsert )
		return;

	// Any parent dirty?
	Node* parent = node->getParent();
	while ( parent != nullptr ) {
		if ( parent->isWidget() && mDirtyStyle.count( parent->asType<UIWidget>() ) > 0 )
			return;
		parent = parent->getParent();
	}

	// Now that we know we aren't early-outing, handle the reinsertion erase
	if ( alreadyExists && tryReinsert )
		mDirtyStyle.erase( node );

	SmallVector<UIWidget*> eraseList;

	// Any child in list? remove it
	for ( auto widget : mDirtyStyle )
		if ( NULL == widget || node->isParentOf( widget ) )
			eraseList.push_back( widget );

	for ( auto widget : eraseList )
		mDirtyStyle.erase( widget );

	mDirtyStyle.insert( node );
}

void UISceneNode::invalidateStyleState( UIWidget* node, bool disableCSSAnimations,
										bool tryReinsert ) {
	eeASSERT( NULL != node );

	if ( node->isClosing() )
		return;

	// Already invalidated?
	if ( mDirtyStyleState.count( node ) > 0 ) {
		if ( !tryReinsert )
			return;
		else
			mDirtyStyleState.erase( node );
	}

	// Any parent dirty?
	Node* parent = node->getParent();
	while ( parent != nullptr ) {
		if ( parent->isWidget() && mDirtyStyleState.count( parent->asType<UIWidget>() ) > 0 )
			return;
		parent = parent->getParent();
	}

	SmallVector<UIWidget*> eraseList;

	// Any child in list? remove it
	for ( auto widget : mDirtyStyleState )
		if ( NULL == widget || node->isParentOf( widget ) )
			eraseList.push_back( widget );

	for ( auto widget : eraseList ) {
		mDirtyStyleState.erase( widget );
		mDirtyStyleStateCSSAnimations.erase( widget );
	}

	mDirtyStyleState.insert( node );
	mDirtyStyleStateCSSAnimations[node] = disableCSSAnimations;
}

void UISceneNode::invalidateLayout( UILayout* node, LayoutInvalidationFlags reasons ) {
	eeASSERT( NULL != node );

	if ( node->isClosing() )
		return;

	if ( mDirtyLayouts.count( node ) > 0 ) {
		node->mDirtyReasons |= reasons;
		return;
	}

	// 1. Walk UP the tree.
	// If any ancestor is already dirty AND the path to it is entirely layouts,
	// merge the descendant reasons into the ancestor and skip adding this node.
	Node* ancestorIt = node->getParent();
	while ( ancestorIt != nullptr ) {
		if ( !ancestorIt->isLayout() ) {
			// The invalidation path is broken by a non-layout node.
			// Any dirty layouts above this will not automatically update this node.
			break;
		}

		if ( mDirtyLayouts.count( ancestorIt->asType<UILayout>() ) > 0 ) {
			ancestorIt->asType<UILayout>()->mDirtyReasons |= reasons;
			return;
		}

		ancestorIt = ancestorIt->getParent();
	}

	// 2. Walk DOWN the dirty list.
	// Remove any already-dirty layouts that will be naturally updated by THIS node,
	// merging their reasons into this node.
	SmallVector<UILayout*> eraseList;

	for ( auto layout : mDirtyLayouts ) {
		if ( NULL == layout ) {
			eraseList.push_back( layout );
			continue;
		}

		// Traverse up from the already-dirty layout to the new node. Coalescing is valid only when
		// every intermediate node is a layout, because updateLayoutTree() recursively walks layout
		// children but does not cross arbitrary widget boundaries.
		Node* it = layout->getParent();
		bool isValidPath = false;

		while ( it != nullptr ) {
			if ( it == node ) {
				// We reached node, and every node in between was a layout.
				isValidPath = true;
				break;
			}
			if ( !it->isLayout() ) {
				// The invalidation path is broken, or node is not an ancestor.
				break;
			}
			it = it->getParent();
		}

		if ( isValidPath ) {
			reasons |= layout->mDirtyReasons;
			eraseList.push_back( layout );
		}
	}

	for ( auto layout : eraseList )
		mDirtyLayouts.erase( layout );

	// 3. Insert the coalesced layout after preserving any descendant reasons removed above.
	node->mDirtyReasons |= reasons;
	mDirtyLayouts.insert( node );
}

void UISceneNode::setIsLoading( bool isLoading ) {
	mIsLoading = isLoading;
}

void UISceneNode::updateDirtyLayouts() {
	if ( !mDirtyLayouts.empty() ) {
		Clock clock;

		// Process a snapshot instead of iterating mDirtyLayouts directly. Layout is allowed to
		// invalidate more layouts while this pass is running: size changes can happen as children
		// are measured, async resources can resolve, and HTML/RichText can discover that a parent
		// needs another pass. Those new invalidations must remain in mDirtyLayouts for the outer
		// invalidation-depth loop to process next. If we iterate the live set and clear it at the
		// end, any invalidation created during layout is silently lost; if we force everything to
		// update synchronously instead, RichText/block layout can re-enter the same parent many
		// times and rebuild the same inline stream repeatedly.
		//
		// Keep the snapshot as reusable storage instead of moving mDirtyLayouts into a temporary
		// set. Moving the set transfers its bucket/node allocation and frees it at the end of this
		// function, which makes large documents rebuild the dirty-set allocation on every layout
		// wave. Copying only layout pointers into a SmallVector keeps common passes inline, lets
		// large bursts grow once and retain capacity, and mDirtyLayouts.clear() preserves the set
		// buckets for invalidations produced by the current pass.
		mDirtyLayoutsSnapshot.clear();
		mDirtyLayoutsSnapshot.reserve( mDirtyLayouts.size() );
		for ( auto layout : mDirtyLayouts )
			mDirtyLayoutsSnapshot.push_back( layout );
		mDirtyLayouts.clear();

		mUpdatingLayouts = true;

		for ( UILayout* layout : mDirtyLayoutsSnapshot ) {
			layout->updateLayoutTree();
		}

		mUpdatingLayouts = false;

		if ( mVerbose )
			Log::debug( "Layout tree updated in %.2f ms", clock.getElapsedTime().asMilliseconds() );
	}
}

void UISceneNode::updateDirtyStyles() {
	if ( !mDirtyStyle.empty() ) {
		Clock clock;
		for ( auto& node : mDirtyStyle ) {
			node->reloadStyle( true, false, false );
		}
		mDirtyStyle.clear();

		if ( mVerbose )
			Log::info( "CSS Styles Reloaded in %.2f ms", clock.getElapsedTime().asMilliseconds() );
	}
}

void UISceneNode::updateDirtyStyleStates() {
	if ( !mDirtyStyleState.empty() ) {
		Clock clock;
		for ( auto& node : mDirtyStyleState ) {
			node->reportStyleStateChangeRecursive( mDirtyStyleStateCSSAnimations[node] );
		}
		mDirtyStyleState.clear();
		mDirtyStyleStateCSSAnimations.clear();

		if ( mVerbose )
			Log::debug( "CSS Style State Invalidated, reapplied state in %.2f ms",
						clock.getElapsedTime().asMilliseconds() );
	}
}

bool UISceneNode::isUpdatingLayouts() const {
	return mUpdatingLayouts;
}

UIIconThemeManager* UISceneNode::getUIIconThemeManager() const {
	return mUIIconThemeManager;
}

UIIcon* UISceneNode::findIcon( const std::string& iconName ) {
	return getUIIconThemeManager()->findIcon( iconName );
}

Drawable* UISceneNode::findIconDrawable( const std::string& iconName, const size_t& drawableSize ) {
	UIIcon* icon = findIcon( iconName );
	if ( icon )
		return icon->getSize( drawableSize );
	return nullptr;
}

CSS::MediaFeatures UISceneNode::getMediaFeatures() const {
	CSS::MediaFeatures media;
	const Sizef& viewportSize = getViewportPixelsSize();
	media.type = media_type_screen;
	media.width = PixelDensity::pxToDp( viewportSize.getWidth() );
	media.height = PixelDensity::pxToDp( viewportSize.getHeight() );
	media.deviceWidth = PixelDensity::pxToDp( mWindow->getDesktopResolution().getWidth() );
	media.deviceHeight = PixelDensity::pxToDp( mWindow->getDesktopResolution().getHeight() );
	media.color = 8;
	media.monochrome = 0;
	media.colorIndex = 256;
	media.resolution = static_cast<int>( getDPI() );
	media.pixelDensity = PixelDensity::getPixelDensity();
	media.prefersColorScheme =
		mColorSchemePreference == ColorSchemePreference::Dark ? "dark" : "light";
	media.prefersContrast = ContrastPreferences::toString( mContrastPreference );
	return media;
}

bool UISceneNode::onMediaChanged( bool forceReApplyStyles ) {
	if ( !mStyleSheet.isMediaQueryListEmpty() ) {
		if ( mStyleSheet.updateMediaLists( getMediaFeatures() ) ) {
			mRoot->reportStyleStateChangeRecursive( false, forceReApplyStyles );
			return true;
		}
	}
	return false;
}

void UISceneNode::onChildCountChange( Node* child, const bool& removed ) {
	if ( !removed && child != mRoot ) {
		child->setParent( mRoot );
	}
}

void UISceneNode::onSizeChange() {
	SceneNode::onSizeChange();

	mRoot->setPixelsSize( getPixelsSize() );
}

void UISceneNode::processStyleSheetAtRules( const StyleSheet& styleSheet, URI baseURI ) {
	loadFontFaces( styleSheet.getStyleSheetStyleByAtRule( AtRuleType::FontFace ), baseURI );
	loadGlyphIcon( styleSheet.getStyleSheetStyleByAtRule( AtRuleType::GlyphIcon ) );
}

void UISceneNode::loadGlyphIcon( const StyleSheetStyleVector& styles ) {
	for ( auto& style : styles ) {
		auto family = style->getPropertyById( PropertyId::FontFamily );
		auto name = style->getPropertyById( PropertyId::Name );
		auto glyph = style->getPropertyById( PropertyId::Glyph );
		if ( name == nullptr || family == nullptr || glyph == nullptr )
			return;
		CSS::StyleSheetProperty familyProp( *family );
		CSS::StyleSheetProperty nameProp( *name );
		CSS::StyleSheetProperty glyphProp( *glyph );

		if ( !familyProp.isEmpty() && !nameProp.isEmpty() && !glyphProp.isEmpty() ) {
			Font* fontSearch = FontManager::instance()->getByName( familyProp.getValue() );

			if ( nullptr == fontSearch )
				continue;

			if ( nullptr == getUIIconThemeManager()->getCurrentTheme() ||
				 fontSearch->getType() != FontType::TTF )
				break;

			Uint32 codePoint = 0;
			std::string buffer( glyphProp.asString() );
			Uint32 value;
			if ( String::startsWith( buffer, "0x" ) ) {
				if ( String::fromString( value, buffer, 16 ) )
					codePoint = value;
			} else if ( String::fromString( value, buffer ) ) {
				codePoint = value;
			}

			if ( codePoint )
				getUIIconThemeManager()->getCurrentTheme()->add( UIGlyphIcon::New(
					nameProp.asString(), static_cast<FontTrueType*>( fontSearch ), codePoint ) );
		}
	}
}

static void resolvePropertyValueURLs( CSS::StyleSheetProperty& prop, const URI& baseURI,
									  UISceneNode* node ) {
	const std::string& value = prop.getValue();
	if ( value.find( "url(" ) == std::string::npos )
		return;

	FunctionString func = FunctionString::parse( value );
	if ( func.getName() != "url" || func.getParameters().empty() )
		return;

	std::string param = func.getParameters()[0];
	if ( !param.empty() && param.front() == '\'' && param.back() == '\'' )
		param = param.substr( 1, param.size() - 2 );
	else if ( !param.empty() && param.front() == '"' && param.back() == '"' )
		param = param.substr( 1, param.size() - 2 );

	if ( param.empty() || param[0] == '@' || String::startsWith( param, "data:image/" ) )
		return;

	URI resolved = node->solveRelativePath( param, baseURI );
	if ( resolved != URI( param ) )
		prop.setValue( "url(" + resolved.toString() + ")", true );
}

static void resolvePropertyRelativeURLs( CSS::StyleSheetProperty& prop, const URI& baseURI,
										 UISceneNode* node ) {
	size_t indexCount = prop.getPropertyIndexCount();
	if ( indexCount > 0 ) {
		std::string newValue;
		for ( size_t i = 0; i < indexCount; i++ ) {
			auto* indexed = prop.getPropertyIndexRef( i );
			if ( indexed ) {
				resolvePropertyValueURLs( *indexed, baseURI, node );
				if ( i > 0 )
					newValue += ", ";
				newValue += indexed->getValue();
			}
		}
		prop.setValue( newValue, true );
	} else {
		resolvePropertyValueURLs( prop, baseURI, node );
	}
}

void UISceneNode::resolveStyleSheetRelativeURLs( CSS::StyleSheet& styleSheet, URI baseURI ) {
	if ( baseURI.empty() )
		return;

	for ( auto& stylePtr : styleSheet.getStyles() ) {
		auto& props = stylePtr->getPropertiesRef();
		for ( auto& it : props )
			resolvePropertyRelativeURLs( it.second, baseURI, this );
	}
}

static std::string normalizeFontFaceFamily( std::string_view family ) {
	std::string normalized{ String::trim( family, ' ' ) };
	normalized = String::trim( normalized, '\'' );
	normalized = String::trim( normalized, '"' );
	return String::toLower( normalized );
}

static std::string makeFontFaceAliasKey( std::string_view family, Uint32 fontStyle,
										 FontWeight weight ) {
	FontWeight resolvedWeight =
		weight != FontWeight::Normal
			? weight
			: ( ( fontStyle & Text::Bold ) ? FontWeight::Bold : FontWeight::Normal );
	return String::format( "%s#%u#%u", normalizeFontFaceFamily( family ).c_str(), fontStyle,
						   static_cast<Uint32>( resolvedWeight ) );
}

void UISceneNode::registerFontFaceAlias( std::string_view family, Uint32 fontStyle,
										 FontWeight weight, Font* font ) {
	if ( family.empty() || font == nullptr || !font->loaded() )
		return;

	mFontFaceAliases[makeFontFaceAliasKey( family, fontStyle, weight )] = font;
}

Font* UISceneNode::getFontFaceAlias( std::string_view family, Uint32 fontStyle,
									 FontWeight weight ) const {
	auto aliasIt = mFontFaceAliases.find( makeFontFaceAliasKey( family, fontStyle, weight ) );
	if ( aliasIt != mFontFaceAliases.end() )
		return aliasIt->second;

	if ( weight != FontWeight::Normal ) {
		aliasIt =
			mFontFaceAliases.find( makeFontFaceAliasKey( family, fontStyle, FontWeight::Normal ) );
		if ( aliasIt != mFontFaceAliases.end() )
			return aliasIt->second;
	}

	if ( fontStyle != 0 ) {
		aliasIt = mFontFaceAliases.find( makeFontFaceAliasKey( family, 0, weight ) );
		if ( aliasIt != mFontFaceAliases.end() )
			return aliasIt->second;
	}

	return nullptr;
}

void UISceneNode::loadFontFaces( const StyleSheetStyleVector& styles, URI baseURI ) {
	auto loadFont = [this, baseURI]( const std::string& authorFamily,
									 const CSS::StyleSheetProperty& srcProp, Uint32 fontStyle,
									 FontWeight fontWeight ) {
		auto makeInternalFontName = [this]( const std::string& familyName, Uint32 fontStyle,
											FontWeight fontWeight ) {
			return String::format( "__eepp_font_face_%p_%zu_%s_%u_%u", this,
								   mFontFaces.size() + mFontFaceAliases.size(), familyName.c_str(),
								   fontStyle, static_cast<Uint32>( fontWeight ) );
		};
		auto registerLoadedFont = [this, authorFamily, fontStyle,
								   fontWeight]( FontTrueType* font ) {
			if ( font == nullptr || !font->loaded() )
				return false;
			registerFontFaceAlias( authorFamily, fontStyle, fontWeight, font );
			mFontFaces.push_back( font );
			mRoot->reloadFontFamily();
			return true;
		};
		std::string path( srcProp.getValue() );
		FunctionString func( FunctionString::parse( path ) );

		if ( !func.getParameters().empty() && func.getName() == "url" ) {
			path = func.getParameters().at( 0 );

			if ( !path.empty() && path.front() == '\'' && path.back() == '\'' )
				String::trimInPlace( path, '\'' );
			else if ( !path.empty() && path.front() == '"' && path.back() == '"' )
				String::trimInPlace( path, '"' );
		}

		if ( String::startsWith( path, "data:" ) ) {
			size_t commaPos = path.find( ',' );
			if ( commaPos != std::string::npos ) {
				std::string header = path.substr( 5, commaPos - 5 );
				std::string data = path.substr( commaPos + 1 );
				bool isBase64 = header.find( ";base64" ) != std::string::npos;

				if ( isBase64 && !data.empty() ) {
					std::string decoded;
					Base64::decode( data, decoded );
					FontTrueType* font = FontTrueType::New(
						makeInternalFontName( authorFamily, fontStyle, fontWeight ) );
					if ( font->loadFromMemory( &decoded[0], decoded.size() ) ) {
						registerLoadedFont( font );
					} else
						eeSAFE_DELETE( font );
				}
			}
			return;
		}

		path = solveRelativePath( path, baseURI ).toString();

		if ( String::startsWith( path, "file://" ) ) {
			std::string filePath( path.substr( 7 ) );

			FontTrueType* font =
				FontTrueType::New( makeInternalFontName( authorFamily, fontStyle, fontWeight ) );

			if ( font->loadFromFile( filePath ) ) {
				registerLoadedFont( font );
				runOnMainThread( [this] { mRoot->reloadFontFamily(); } );
			} else
				eeSAFE_DELETE( font );
		} else if ( String::startsWith( path, "http://" ) ||
					String::startsWith( path, "https://" ) ) {
			std::string internalFontName(
				makeInternalFontName( authorFamily, fontStyle, fontWeight ) );
			auto resourceState = mAsyncResourceLoadState;
			Uint64 resourceGeneration = resourceState ? resourceState->generation : 0;
			Http::getAsync(
				[this, resourceState, resourceGeneration, internalFontName, registerLoadedFont,
				 path]( const Http&, Http::Request&, Http::Response& response ) {
					if ( !resourceState || !resourceState->alive ||
						 resourceState->generation != resourceGeneration )
						return;
					FontTrueType* font = FontTrueType::New( internalFontName );
					if ( response.isOK() && !response.getBody().empty() &&
						 font->loadFromMemory( &response.getBody()[0],
											   response.getBody().size() ) ) {
						runOnMainThread( [resourceState, resourceGeneration, registerLoadedFont,
										  font]() mutable {
							if ( resourceState && resourceState->alive &&
								 resourceState->generation == resourceGeneration ) {
								registerLoadedFont( font );
							} else {
								eeSAFE_DELETE( font );
							}
						} );
					} else {
						eeSAFE_DELETE( font );
						Log::error( "UISceneNode::loadFontFaces: Failed to load font \"%s\", from: "
									"%s. Request response status code: %d (%s)",
									internalFontName, path, response.getStatus(),
									response.getStatusDescription() );
					}
				},
				URI( path ), Seconds( 5 ) );
		} else if ( VFS::instance()->fileExists( path ) ) {
			FontTrueType* font =
				FontTrueType::New( makeInternalFontName( authorFamily, fontStyle, fontWeight ) );

			IOStream* stream = VFS::instance()->getFileFromPath( path );
			if ( font->loadFromStream( *stream ) ) {
				registerLoadedFont( font );
			} else
				eeSAFE_DELETE( font );
		}
	};

	for ( auto& style : styles ) {
		auto family = style->getPropertyById( PropertyId::FontFamily );
		auto src = style->getPropertyById( PropertyId::Src );
		if ( src == nullptr || family == nullptr )
			return;
		auto fontStyleProp = style->getPropertyById( PropertyId::FontStyle );
		Uint32 fontStyle = fontStyleProp ? fontStyleProp->asFontStyle() : 0;
		auto fontWeightProp = style->getPropertyById( PropertyId::FontWeight );
		fontStyle |= fontWeightProp ? fontWeightProp->asFontStyle() : 0;
		FontWeight fontWeight = fontWeightProp
									? Text::stringToFontWeight( fontWeightProp->getValue() )
									: FontWeight::Normal;

		CSS::StyleSheetProperty familyProp( *family );
		CSS::StyleSheetProperty srcProp( *src );

		if ( familyProp.isEmpty() || srcProp.isEmpty() )
			return;

		std::string fontFamily( String::trim( familyProp.getValue(), '"' ) );
		String::trimInPlace( fontFamily, "'" );

		auto unicodeRange = style->getPropertyById( PropertyId::UnicodeRange );

		// We don't support unicode ranges yet
		if ( unicodeRange && unicodeRange->value().find( "U+0000-00FF" ) == std::string::npos ) {
			continue;
		}

		loadFont( fontFamily, srcProp, fontStyle, fontWeight );
	}
}

URI UISceneNode::solveRelativePath( URI uri, URI baseURI ) {
	URI base = baseURI.empty() ? mURI : baseURI;

	// Automatically handles absolute URLs, protocol-relative URLs,
	// directory merging, and ".." segment collapsing!
	base.resolve( uri );

	// If after resolution the scheme is still empty:
	// - For plain relative paths (no authority), default to "file"
	// - For protocol-relative URLs (has authority via //), default to "https"
	if ( base.getScheme().empty() ) {
		if ( base.getAuthority().empty() )
			base.setScheme( "file" );
		else
			base.setScheme( "https" );
	}

	return base;
}

void UISceneNode::loadCSS( URI uri, std::optional<Time> defer ) {
	uri = solveRelativePath( uri );
	std::string url = uri.toString();
	Log::debug( "UISceneNode::loadCSS: %s", url );

	if ( "file" == uri.getScheme() ||
		 ( uri.getScheme().empty() && FileSystem::fileExists( uri.getFSPath() ) ) ) {
		if ( defer && mThreadPool ) {
			mThreadPool->run( [this, uri, url, defer] {
				Clock c;
				std::string filePath( uri.getFSPath() );
				std::string css;
				if ( FileSystem::fileExists( filePath ) && FileSystem::fileGet( filePath, css ) ) {
					CSS::StyleSheetParser parser;
					if ( parser.loadFromString( css ) ) {
						parser.getStyleSheet().setMarker( String::hash( url ) );
						auto baseURL = getURIFromURL( url );
						auto delay = defer.has_value() ? *defer - c.getElapsedTime() : Time::Zero;
						if ( delay < Time::Zero )
							delay = Time::Zero;
						runOnMainThread(
							[this, url, baseURL = std::move( baseURL ),
							 parser = std::move( parser )]() mutable {
								combineStyleSheet( parser.getStyleSheet(), true, baseURL );
								Log::debug( "UISceneNode::loadCSS: Loaded - %s", url );
							},
							delay );
					}
				}
			} );
		} else {
			std::string filePath( uri.getFSPath() );
			std::string css;
			if ( FileSystem::fileExists( filePath ) && FileSystem::fileGet( filePath, css ) ) {
				combineStyleSheet( css, true, String::hash( url ), getURIFromURL( url ) );
				Log::debug( "UISceneNode::loadCSS: Loaded - %s", url );
			}
		}
	} else if ( "http" == uri.getScheme() || "https" == uri.getScheme() ) {
		auto resourceState = mAsyncResourceLoadState;
		Uint64 resourceGeneration = resourceState ? resourceState->generation : 0;
		Http::getAsync(
			[this, resourceState, resourceGeneration, url]( const Http&, Http::Request&,
															Http::Response& response ) {
				if ( !resourceState || !resourceState->alive ||
					 resourceState->generation != resourceGeneration )
					return;
				if ( !response.getBody().empty() &&
					 response.getStatus() == Http::Response::Status::Ok ) {
					std::string css( response.getBody() );
					runOnMainThread( [resourceState, resourceGeneration, css = std::move( css ),
									  url = std::move( url ), this] {
						if ( !resourceState || !resourceState->alive ||
							 resourceState->generation != resourceGeneration )
							return;
						combineStyleSheet( css, true, String::hash( url ), getURIFromURL( url ) );
						Log::debug( "UISceneNode::loadCSS: Loaded - %s", url );
					} );
				} else {
					Log::debug( "UISceneNode::loadCSS: Failed to load %s - %s", url,
								response.getStatusDescription() );
				}
			},
			uri, Seconds( 5 ) );
	} else if ( VFS::instance()->fileExists( uri.getPath() ) ) {
		IOStream* stream = VFS::instance()->getFileFromPath( uri.getPath() );
		CSS::StyleSheetParser parser;
		if ( parser.loadFromStream( *stream ) ) {
			parser.getStyleSheet().setMarker( String::hash( url ) );
			combineStyleSheet( parser.getStyleSheet() );
			Log::debug( "UISceneNode::loadCSS: Loaded - %s", url );
		}
	} else {
		Log::debug( "UISceneNode::loadCSS: Failed to load %s - Unknown scheme", url );
	}
}

void UISceneNode::setInternalPixelsSize( const Sizef& size ) {
	Sizef s( size );
	if ( s != mSize ) {
		mDpSize = PixelDensity::pxToDp( s ).ceil();
		mSize = s;
		mNodeFlags |= NODE_FLAG_POLYGON_DIRTY;
		updateCenter();
		onSizeChange();
		sendCommonEvent( Event::OnSizeChange );
		invalidateDraw();
	}
}

Uint32 UISceneNode::onKeyDown( const KeyEvent& event ) {
	std::string cmd = mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
	if ( !cmd.empty() ) {
		executeKeyBindingCommand( cmd );
		return 0;
	}
	return SceneNode::onKeyDown( event );
}

KeyBindings& UISceneNode::getKeyBindings() {
	return mKeyBindings;
}

void UISceneNode::setKeyBindings( const KeyBindings& keyBindings ) {
	mKeyBindings = keyBindings;
}

void UISceneNode::addKeyBindingString( const std::string& shortcut, const std::string& command ) {
	mKeyBindings.addKeybindString( shortcut, command );
}

void UISceneNode::addKeyBinding( const KeyBindings::Shortcut& shortcut,
								 const std::string& command ) {
	mKeyBindings.addKeybind( shortcut, command );
}

void UISceneNode::replaceKeyBindingString( const std::string& shortcut,
										   const std::string& command ) {
	mKeyBindings.replaceKeybindString( shortcut, command );
}

void UISceneNode::replaceKeyBinding( const KeyBindings::Shortcut& shortcut,
									 const std::string& command ) {
	mKeyBindings.replaceKeybind( shortcut, command );
}

void UISceneNode::addKeyBindsString( const std::map<std::string, std::string>& binds ) {
	mKeyBindings.addKeybindsString( binds );
}

void UISceneNode::addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds ) {
	mKeyBindings.addKeybinds( binds );
}

void UISceneNode::setKeyBindingCommand( const std::string& command,
										UISceneNode::KeyBindingCommand func ) {
	mKeyBindingCommands[command] = func;
}

void UISceneNode::executeKeyBindingCommand( const std::string& command ) {
	auto cmdIt = mKeyBindingCommands.find( command );
	if ( cmdIt != mKeyBindingCommands.end() ) {
		cmdIt->second();
	}
}

UIEventDispatcher* UISceneNode::getUIEventDispatcher() const {
	return static_cast<UIEventDispatcher*>( mEventDispatcher );
}

ColorSchemePreference UISceneNode::getColorSchemePreference() const {
	return mColorSchemePreference;
}

void UISceneNode::setColorSchemePreference(
	const ColorSchemeExtPreference& colorSchemePreference ) {
	switch ( colorSchemePreference ) {
		case ColorSchemeExtPreference::Light:
			setColorSchemePreference( ColorSchemePreference::Light );
			break;
		case ColorSchemeExtPreference::Dark:
			setColorSchemePreference( ColorSchemePreference::Dark );
			break;
		case ColorSchemeExtPreference::System:
			setColorSchemePreference( Sys::isOSUsingDarkColorScheme()
										  ? ColorSchemePreference::Dark
										  : ColorSchemePreference::Light );
			break;
	}
}

void UISceneNode::setColorSchemePreference( const ColorSchemePreference& colorSchemePreference ) {
	if ( mColorSchemePreference != colorSchemePreference ) {
		mColorSchemePreference = colorSchemePreference;
		if ( !mStyleSheet.isMediaQueryListEmpty() ) {
			if ( mStyleSheet.updateMediaLists( getMediaFeatures() ) ) {
				mStyleSheet.invalidateCache();
				mRoot->reloadStyle( true, true, true, true, true );
			}
		}
	}
}

ContrastPreference UISceneNode::getContrastPreference() const {
	return mContrastPreference;
}

void UISceneNode::setContrastPreference( const ContrastExtPreference& contrastPreference ) {
	setContrastPreference( ContrastPreferences::fromExt( contrastPreference ) );
}

void UISceneNode::setContrastPreference( const ContrastPreference& contrastPreference ) {
	if ( mContrastPreference != contrastPreference ) {
		mContrastPreference = contrastPreference;
		if ( !mStyleSheet.isMediaQueryListEmpty() ) {
			if ( mStyleSheet.updateMediaLists( getMediaFeatures() ) ) {
				mStyleSheet.invalidateCache();
				mRoot->reloadStyle( true, true, true, true, true );
			}
		}
	}
}

const Uint32& UISceneNode::getMaxInvalidationDepth() const {
	return mMaxInvalidationDepth;
}

void UISceneNode::setMaxInvalidationDepth( const Uint32& maxInvalidationDepth ) {
	mMaxInvalidationDepth = maxInvalidationDepth;
}

void UISceneNode::setURI( const URI& uri ) {
	mURI = uri;
}

URI UISceneNode::getURIFromURL( const URI& url ) const {
	URI baseURI( url );
	std::string path = baseURI.getPath();

	// If the path isn't empty and doesn't end with a directory slash...
	if ( !path.empty() && path.back() != '/' ) {
		size_t lastSlash = path.find_last_of( '/' );

		if ( lastSlash != std::string::npos ) {
			// Keep everything up to and including the last '/'
			// Example: "/assets/css/styles.html" -> "/assets/css/"
			baseURI.setPath( path.substr( 0, lastSlash + 1 ) );
		} else {
			// Fallback if there are no slashes in the path at all
			baseURI.setPath( "/" );
		}
	}

	// Clear any query strings (?foo=bar) or fragments (#section) from the base URI,
	// as they shouldn't be inherited by relative paths.
	baseURI.setQuery( "" );
	baseURI.setFragment( "" );

	return baseURI;
}

void UISceneNode::setURIFromURL( const URI& url ) {
	setURI( getURIFromURL( url ) );
	mReferer = url;
}

void UISceneNode::openURL( URI uri ) {
	navigate( NavigationRequest{ std::move( uri ) } );
}

void UISceneNode::navigate( const NavigationRequest& request ) {
	if ( mNavigationInterceptorCb && mNavigationInterceptorCb( request ) )
		return;
	Engine::instance()->openURI( request.uri.toString() );
}

void UISceneNode::invalidateAsyncResourceLoads() {
	if ( mAsyncResourceLoadState )
		mAsyncResourceLoadState->generation++;
}

void UISceneNode::invalidate( Node* invalidator ) {
	Node::invalidate( invalidator );

	if ( mParentNode ) {
		UISceneNode* hostScene = getHostUISceneNode();
		if ( hostScene && hostScene != this )
			hostScene->invalidate( invalidator );
	}
}

Font* UISceneNode::getFontFromNamesList( std::string_view names, Uint32 fontStyle,
										 FontWeight weight ) const {
	FontManager* fm = FontManager::instance();
	Font* font = nullptr;
	String::readBySeparatorStoppable(
		names,
		[&]( std::string_view name ) {
			name = String::trim( name, ' ' );
			name = String::trim( name, '\'' );
			name = String::trim( name, '"' );

			std::string fontFamily{ name };
			font = getFontFaceAlias( fontFamily, fontStyle, weight );
			if ( font )
				return true;

			size_t size = fontFamily.size();
			if ( fontStyle )
				fontFamily += "#" + Text::styleFlagToString( fontStyle );

			font = fm->getByName( fontFamily );

			// Remove the font style part (ex: `Arial#bold` to `Arial`)
			// We need this for SystemFontResolver::genericFamilyFromName
			if ( fontStyle )
				fontFamily.resize( size );

			if ( font == nullptr &&
				 SystemFontResolver::genericFamilyFromName( fontFamily ) != GenericFamily::None ) {
				FontQuery query;
				query.family = fontFamily;
				query.italic = fontStyle & Text::Italic;
				query.weight =
					weight != FontWeight::Normal
						? weight
						: ( ( fontStyle & Text::Bold ) ? FontWeight::Bold : FontWeight::Normal );
				fontFamily = SystemFontResolver::instance()->resolve( query ).family;

				if ( fontStyle )
					fontFamily += "#" + Text::styleFlagToString( fontStyle );

				font = fm->getByName( fontFamily );
			}

			if ( font == nullptr && SystemFontResolver::isEnabled() ) {
				FontWeight resolvedWeight =
					weight != FontWeight::Normal
						? weight
						: ( ( fontStyle & Text::Bold ) ? FontWeight::Bold : FontWeight::Normal );
				FontDesc desc = SystemFontResolver::instance()->resolveFromNamesList(
					std::string{ names }, resolvedWeight, fontStyle & Text::Italic );
				if ( !desc.path.empty() ) {
					std::string family = desc.family;
					if ( fontStyle )
						family += "#" + Text::styleFlagToString( fontStyle );

					if ( ( font = fm->getByName( family ) ) )
						return true;

					FontTrueType* ttf = FontTrueType::New( family, desc.path, desc.faceIndex );
					if ( ttf && ttf->loaded() ) {
						font = ttf;
						Uint32 weightStyle = fontStyle & ( Text::Bold | Text::Italic );
						if ( weightStyle ) {
							Font* regular = fm->getByName( desc.family );
							if ( regular && regular != font &&
								 regular->getType() == FontType::TTF ) {
								auto* regularFT = static_cast<FontTrueType*>( regular );
								if ( weightStyle == Text::Bold )
									regularFT->setBoldFont( ttf );
								else if ( weightStyle == Text::Italic )
									regularFT->setItalicFont( ttf );
								else
									regularFT->setBoldItalicFont( ttf );
							}
						}
					}
				}
			}

			return font != nullptr;
		},
		',' );

	return font;
}

Font* UISceneNode::reevaluateFontStyle( Font* currentFont, Uint32 fontStyle,
										FontWeight weight ) const {
	if ( !currentFont || !SystemFontResolver::isEnabled() )
		return nullptr;

	if ( currentFont->getType() != FontType::TTF )
		return nullptr;

	std::string name = currentFont->getName();
	size_t pos = name.find( '#' );
	if ( pos != std::string::npos )
		name = name.substr( 0, pos );

	Uint32 weightStyle = fontStyle & ( Text::Bold | Text::Italic );
	Font* newFont = getFontFromNamesList( name, weightStyle, weight );
	if ( newFont && newFont != currentFont )
		return newFont;

	return nullptr;
}

void UISceneNode::loadFontStyleVariants( Font* font, const std::string& family ) const {
	if ( !font || !SystemFontResolver::isEnabled() )
		return;
	if ( font->getType() != FontType::TTF )
		return;
	auto* ft = static_cast<FontTrueType*>( font );

	auto loadVariant = [family]( FontWeight weight, bool italic ) -> FontTrueType* {
		Uint32 style = 0;
		if ( italic )
			style |= Text::Italic;
		if ( weight == FontWeight::Bold )
			style |= Text::Bold;
		std::string queryFamily = family;
		if ( style )
			queryFamily += "#" + Text::styleFlagToString( style );
		Font* existing = FontManager::instance()->getByName( queryFamily );
		if ( existing && existing->getType() == FontType::TTF )
			return static_cast<FontTrueType*>( existing );

		FontDesc desc = SystemFontResolver::instance()->resolveGeneric(
			SystemFontResolver::genericFamilyFromName( family ), weight, italic );
		if ( desc.path.empty() ) {
			FontQuery query;
			query.family = family;
			query.weight = weight;
			query.italic = italic;
			desc = SystemFontResolver::instance()->resolve( query );
		}
		if ( desc.path.empty() )
			return nullptr;

		auto* ttf = FontTrueType::New( queryFamily, desc.path, desc.faceIndex );
		if ( !ttf || !ttf->loaded() ) {
			eeSAFE_DELETE( ttf );
			return nullptr;
		}
		return ttf;
	};

	FontTrueType* boldFont = loadVariant( FontWeight::Bold, false );
	if ( boldFont )
		ft->setBoldFont( boldFont );

	FontTrueType* italicFont = loadVariant( FontWeight::Normal, true );
	if ( italicFont )
		ft->setItalicFont( italicFont );

	FontTrueType* boldItalicFont = loadVariant( FontWeight::Bold, true );
	if ( boldItalicFont )
		ft->setBoldItalicFont( boldItalicFont );
}

void UISceneNode::loadHTMLBaseCSS() {
	// Load HTML base defaults (idempotent - marker check prevents duplicates)
	UIWidgetCreator::loadHTMLBaseDefaults( mStyleSheet, String::hash( "html_defaults" ) );
}

}} // namespace EE::UI
