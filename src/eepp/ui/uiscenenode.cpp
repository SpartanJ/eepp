#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/virtualfilesystem.hpp>
#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uiroot.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/window.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using namespace EE::Network;

namespace EE { namespace UI {

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
	mKeyBindings( mWindow->getInput() ) {
	// Reset size since the SceneNode already set it but needs to set the size from zero to emmit
	// the required events to its childs.
	mSize = Sizef();
	mDpSize = Sizef();

	// Update only UI elements that requires it.
	setUpdateAllChilds( false );

	mNodeFlags |= NODE_FLAG_UISCENENODE | NODE_FLAG_OVER_FIND_ALLOWED;

	setEventDispatcher( UIEventDispatcher::New( this ) );

	mRoot = UIRoot::New();
	mRoot->setParent( this )->setPosition( 0, 0 )->setId( "uiscenenode_root_node" );
	mRoot->enableReportSizeChangeToChilds();

	resizeNode( mWindow );
}

UISceneNode::~UISceneNode() {
	eeSAFE_DELETE( mUIThemeManager );
	eeSAFE_DELETE( mUIIconThemeManager );

	for ( auto& font : mFontFaces ) {
		FontManager::instance()->remove( font );
	}

	// UISceneNode can now destroy the ThreadPool shared to him. If that's the case,
	// We need to ensure that the childs are destroyed before the thread pool,
	// since its childs could be consuming it and need to uninitialize gracefully.
	childDeleteAll();
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

	if ( !mCurParent )
		eeSAFE_DELETE( mEventDispatcher );

	mCurParent = mParentNode;

	if ( !mParentNode ) {
		setEventDispatcher( UIEventDispatcher::New( this ) );
		return;
	}

	mEventDispatcher = getParent()->asType<UINode>()->getUISceneNode()->getEventDispatcher();

	setDirty();
	setPixelsSize( getParent()->getPixelsSize() );

	mCurOnSizeChangeListener =
		getParent()->addEventListener( Event::OnSizeChange, [this]( const Event* ) {
			setDirty();
			setPixelsSize( getParent()->getPixelsSize() );
			onMediaChanged();
			sendMsg( this, NodeMessage::WindowResize );
		} );
}

void UISceneNode::setTranslator( Translator translator ) {
	mTranslator = translator;
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
	std::vector<UIWidget*> rootWidgets;

	if ( NULL == parent )
		parent = this;

	Clock clock;
	for ( pugi::xml_node widget = node; widget; widget = widget.next_sibling() ) {
		clock.restart();

		UIWidget* uiwidget = UIWidgetCreator::createFromName( widget.name() );

		if ( NULL != uiwidget ) {
			rootWidgets.push_back( uiwidget );

			uiwidget->setParent( parent );
			uiwidget->loadFromXmlNode( widget );

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

			if ( widget.first_child() ) {
				loadNode( widget.first_child(), uiwidget, marker );
			}

			uiwidget->onWidgetCreated();
		} else if ( String::toLower( std::string( widget.name() ) ) == "style" ) {
			CSS::StyleSheetParser parser;

			if ( parser.loadFromString( std::string_view{ widget.text().as_string() } ) ) {
				parser.getStyleSheet().setMarker( marker );
				combineStyleSheet( parser.getStyleSheet(), false );
			}
		}
	}

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

	for ( auto& widget : widgets )
		widget->reloadStyle( true, true, true );

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

void UISceneNode::setStyleSheet( const CSS::StyleSheet& styleSheet ) {
	mStyleSheet = styleSheet;
	processStyleSheetAtRules( styleSheet );
	onMediaChanged();
	reloadStyle();
}

void UISceneNode::setStyleSheet( const std::string& inlineStyleSheet ) {
	CSS::StyleSheetParser parser;

	if ( parser.loadFromString( inlineStyleSheet ) )
		setStyleSheet( parser.getStyleSheet() );
}

void UISceneNode::combineStyleSheet( const CSS::StyleSheet& styleSheet,
									 const bool& forceReloadStyle ) {
	mStyleSheet.combineStyleSheet( styleSheet );
	processStyleSheetAtRules( styleSheet );
	onMediaChanged();
	if ( forceReloadStyle )
		reloadStyle();
}

void UISceneNode::combineStyleSheet( const std::string& inlineStyleSheet,
									 const bool& forceReloadStyle, const Uint32& marker ) {
	CSS::StyleSheetParser parser;

	if ( parser.loadFromString( inlineStyleSheet ) ) {
		parser.getStyleSheet().setMarker( marker );

		combineStyleSheet( parser.getStyleSheet(), forceReloadStyle );
	}
}

CSS::StyleSheet& UISceneNode::getStyleSheet() {
	return mStyleSheet;
}

bool UISceneNode::hasStyleSheet() {
	return !mStyleSheet.isEmpty();
}

void UISceneNode::reloadStyle( bool disableAnimations, bool forceReApplyProperties ) {
	if ( NULL != mChild ) {
		Node* child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				child->asType<UIWidget>()->reloadStyle( true, disableAnimations, true,
														forceReApplyProperties );
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

UIWidget* UISceneNode::loadLayoutFromFile( const std::string& layoutPath, Node* parent,
										   const Uint32& marker ) {
	if ( FileSystem::fileExists( layoutPath ) ) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( layoutPath.c_str() );

		if ( result ) {
			return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this, marker );
		} else {
			Log::error( "Couldn't load UI Layout: %s", layoutPath.c_str() );
			Log::error( "Error description: %s", result.description() );
			Log::error( "Error offset: %d", result.offset );
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
	pugi::xml_parse_result result = doc.load_string( layoutString );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this, marker );
	} else {
		Log::error( "Couldn't load UI Layout from string: %s", layoutString );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
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
	pugi::xml_parse_result result = doc.load_buffer( buffer, bufferSize );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this, marker );
	} else {
		Log::error( "Couldn't load UI Layout from buffer" );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
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
	pugi::xml_parse_result result = doc.load_buffer( scopedBuffer.get(), scopedBuffer.length() );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this, marker );
	} else {
		Log::error( "Couldn't load UI Layout from stream" );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
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
		sendCommonEvent( Event::OnSizeChange );
		invalidateDraw();
	}
}

Node* UISceneNode::setSize( const Sizef& Size ) {
	if ( Size != mDpSize ) {
		Vector2f sizeChange( Size.x - mDpSize.x, Size.y - mDpSize.y );

		setInternalSize( Size );

		onSizeChange();

		if ( reportSizeChangeToChilds() ) {
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

		onSizeChange();

		if ( reportSizeChangeToChilds() ) {
			sendParentSizeChange( PixelDensity::pxToDp( sizeChange ) );
		}
	}

	return this;
}

UISceneNode* UISceneNode::setPixelsSize( const Float& x, const Float& y ) {
	return setPixelsSize( Sizef( x, y ) );
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
	// that creates childs dynamically only when they are visible.
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

const bool& UISceneNode::isLoading() const {
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

	if ( mDirtyStyle.count( node ) > 0 ) {
		if ( !tryReinsert )
			return;
		else
			mDirtyStyle.erase( node );
	}

	for ( auto& dirtyNode : mDirtyStyle )
		if ( NULL != dirtyNode && dirtyNode->isParentOf( node ) )
			return;

	std::vector<UIWidget*> eraseList;

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

	if ( mDirtyStyleState.count( node ) > 0 ) {
		if ( !tryReinsert )
			return;
		else
			mDirtyStyleState.erase( node );
	}

	for ( auto& dirtyNode : mDirtyStyleState )
		if ( NULL != dirtyNode && dirtyNode->isParentOf( node ) )
			return;

	std::vector<UIWidget*> eraseList;

	for ( auto widget : mDirtyStyleState )
		if ( NULL == widget || node->isParentOf( widget ) )
			eraseList.push_back( widget );

	for ( auto widget : eraseList )
		mDirtyStyleState.erase( widget );

	mDirtyStyleState.insert( node );
	mDirtyStyleStateCSSAnimations[node] = disableCSSAnimations;
}

void UISceneNode::invalidateLayout( UILayout* node ) {
	eeASSERT( NULL != node );

	if ( node->isClosing() )
		return;

	if ( mDirtyLayouts.count( node ) > 0 )
		return;

	if ( node->getParent()->isLayout() ) {
		for ( auto& dirtyNode : mDirtyLayouts )
			if ( NULL != dirtyNode && dirtyNode->isParentOf( node ) )
				return;

		std::vector<UILayout*> eraseList;

		for ( auto layout : mDirtyLayouts )
			if ( NULL == layout ||
				 ( node->isParentOf( layout ) && layout->getParent()->isLayout() ) )
				eraseList.push_back( layout );

		for ( auto layout : eraseList )
			mDirtyLayouts.erase( layout );
	}

	mDirtyLayouts.insert( node );
}

void UISceneNode::setIsLoading( bool isLoading ) {
	mIsLoading = isLoading;
}

void UISceneNode::updateDirtyLayouts() {
	if ( !mDirtyLayouts.empty() ) {
		Clock clock;
		mUpdatingLayouts = true;

		for ( UILayout* layout : mDirtyLayouts ) {
			layout->updateLayoutTree();
		}

		mDirtyLayouts.clear();
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

const bool& UISceneNode::isUpdatingLayouts() const {
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
	media.type = media_type_screen;
	media.width = mWindow->getWidth();
	media.height = mWindow->getHeight();
	media.deviceWidth = mWindow->getDesktopResolution().getWidth();
	media.deviceHeight = mWindow->getDesktopResolution().getHeight();
	media.color = 8;
	media.monochrome = 0;
	media.colorIndex = 256;
	media.resolution = static_cast<int>( getDPI() );
	media.pixelDensity = PixelDensity::getPixelDensity();
	media.prefersColorScheme =
		mColorSchemePreference == ColorSchemePreference::Dark ? "dark" : "light";
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

void UISceneNode::processStyleSheetAtRules( const StyleSheet& styleSheet ) {
	loadFontFaces( styleSheet.getStyleSheetStyleByAtRule( AtRuleType::FontFace ) );
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
				if ( String::fromString( value, buffer, std::hex ) )
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

void UISceneNode::loadFontFaces( const StyleSheetStyleVector& styles ) {
	auto loadFont = [this]( const std::string& familyName, const CSS::StyleSheetProperty& srcProp,
							Font* fontFamily, Uint32 fontStyle ) {
		auto trySetFontFamily = []( Font* fontFamily, Uint32 fontStyle, FontTrueType* font ) {
			if ( fontFamily && fontFamily->getType() == FontType::TTF && fontStyle ) {
				FontTrueType* ttf = static_cast<FontTrueType*>( fontFamily );
				if ( fontStyle == ( Text::Italic | Text::Bold ) ) {
					ttf->setBoldItalicFont( font );
				} else if ( fontStyle == Text::Italic ) {
					ttf->setItalicFont( font );
				} else if ( fontStyle == Text::Bold ) {
					ttf->setBoldFont( font );
				}
			}
		};

		std::string path( srcProp.getValue() );
		FunctionString func( FunctionString::parse( path ) );

		if ( !func.getParameters().empty() && func.getName() == "url" )
			path = func.getParameters().at( 0 );

		if ( String::startsWith( path, "file://" ) ) {
			std::string filePath( path.substr( 7 ) );

			FontTrueType* font = FontTrueType::New( familyName );

			font->loadFromFile( filePath );
			trySetFontFamily( fontFamily, fontStyle, font );

			mFontFaces.push_back( font );
			runOnMainThread( [this] { mRoot->reloadFontFamily(); } );
		} else if ( String::startsWith( path, "http://" ) ||
					String::startsWith( path, "https://" ) ) {
			Http::getAsync(
				[this, fontStyle, familyName, fontFamily,
				 trySetFontFamily]( const Http&, Http::Request&, Http::Response& response ) {
					FontTrueType* font = FontTrueType::New( familyName );

					if ( !response.getBody().empty() ) {
						font->loadFromMemory( &response.getBody()[0], response.getBody().size() );
						trySetFontFamily( fontFamily, fontStyle, font );
						mFontFaces.push_back( font );
						runOnMainThread( [this] { mRoot->reloadFontFamily(); } );
					}
				},
				URI( path ), Seconds( 5 ) );
		} else if ( VFS::instance()->fileExists( path ) ) {
			FontTrueType* font = FontTrueType::New( familyName );

			IOStream* stream = VFS::instance()->getFileFromPath( path );
			font->loadFromStream( *stream );
			trySetFontFamily( fontFamily, fontStyle, font );

			mFontFaces.push_back( font );
			runOnMainThread( [this] { mRoot->reloadFontFamily(); } );
		}
	};

	for ( auto& style : styles ) {
		auto family = style->getPropertyById( PropertyId::FontFamily );
		auto src = style->getPropertyById( PropertyId::Src );
		if ( src == nullptr || family == nullptr )
			return;
		auto fontStyleProp = style->getPropertyById( PropertyId::FontStyle );
		Uint32 fontStyle = fontStyleProp ? fontStyleProp->asFontStyle() : 0;

		CSS::StyleSheetProperty familyProp( *family );
		CSS::StyleSheetProperty srcProp( *src );

		if ( familyProp.isEmpty() || srcProp.isEmpty() )
			return;

		Font* fontSearch = FontManager::instance()->getByName( familyProp.getValue() );
		std::string fontFamily( String::trim( familyProp.getValue(), '"' ) );
		if ( fontStyle )
			fontFamily += "#" + Text::styleFlagToString( fontStyle );

		if ( nullptr == fontSearch ) {
			loadFont( fontFamily, srcProp, nullptr, fontStyle );
		} else if ( fontSearch->isRegular() && fontSearch->getFontStyle() != fontStyle ) {
			loadFont( fontFamily, srcProp, fontSearch, fontStyle );
		}
	}
}

void UISceneNode::setInternalPixelsSize( const Sizef& size ) {
	Sizef s( size );
	if ( s != mSize ) {
		mDpSize = PixelDensity::pxToDp( s ).ceil();
		mSize = s;
		mNodeFlags |= NODE_FLAG_POLYGON_DIRTY;
		updateCenter();
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

void UISceneNode::setColorSchemePreference( const ColorSchemePreference& colorSchemePreference ) {
	if ( mColorSchemePreference != colorSchemePreference ) {
		mColorSchemePreference = colorSchemePreference;
		if ( !mStyleSheet.isMediaQueryListEmpty() ) {
			if ( mStyleSheet.updateMediaLists( getMediaFeatures() ) ) {
				mStyleSheet.invalidateCache();
				mRoot->reloadStyle( true, true, true, true );
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

}} // namespace EE::UI
