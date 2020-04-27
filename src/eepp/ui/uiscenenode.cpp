#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
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
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/window.hpp>
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
#ifdef EE_DEBUG
	mVerbose( true ),
#else
	mVerbose( false ),
#endif
	mUIThemeManager( UIThemeManager::New() ) {
	// Update only UI elements that requires it.
	setUpdateAllChilds( false );

	mNodeFlags |= NODE_FLAG_UISCENENODE | NODE_FLAG_OVER_FIND_ALLOWED;

	setEventDispatcher( UIEventDispatcher::New( this ) );

	mRoot = UIWidget::NewWithTag( ":root" );
	mRoot->setParent( this )->setPosition( 0, 0 );
	mRoot->enableReportSizeChangeToChilds();

	resizeControl( mWindow );
}

UISceneNode::~UISceneNode() {
	eeSAFE_DELETE( mUIThemeManager );

	for ( auto& font : mFontFaces ) {
		FontManager::instance()->remove( font );
	}
}

void UISceneNode::resizeControl( EE::Window::Window* ) {
	setSize( eefloor( mWindow->getWidth() / PixelDensity::getPixelDensity() ),
			 eefloor( mWindow->getHeight() / PixelDensity::getPixelDensity() ) );
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

void UISceneNode::setTranslator( Translator translator ) {
	mTranslator = translator;
}

String UISceneNode::getTranslatorString( const std::string& str ) {
	if ( String::startsWith( str, "@string/" ) ) {
		String tstr = mTranslator.getString( str.substr( 8 ) );

		if ( !tstr.empty() )
			return tstr;
	}

	return String( str );
}

String UISceneNode::getTranslatorString( const std::string& str, const String& defaultValue ) {
	if ( String::startsWith( str, "@string/" ) ) {
		String tstr = mTranslator.getString( str.substr( 8 ) );

		if ( !tstr.empty() )
			return tstr;
	}

	return defaultValue;
}

void UISceneNode::setFocusLastWindow( UIWindow* window ) {
	if ( NULL != mEventDispatcher && !mWindowsList.empty() && window != mWindowsList.front() ) {
		mEventDispatcher->setFocusControl( mWindowsList.front() );
	}
}

void UISceneNode::windowAdd( UIWindow* win ) {
	if ( !windowExists( win ) ) {
		mWindowsList.push_front( win );
	} else {
		//! Send to front
		mWindowsList.remove( win );
		mWindowsList.push_front( win );
	}
}

void UISceneNode::windowRemove( UIWindow* win ) {
	if ( windowExists( win ) ) {
		mWindowsList.remove( win );
	}
}

bool UISceneNode::windowExists( UIWindow* win ) {
	return mWindowsList.end() != std::find( mWindowsList.begin(), mWindowsList.end(), win );
}

std::vector<UIWidget*> UISceneNode::loadNode( pugi::xml_node node, Node* parent ) {
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
				mTimes.push_back( std::make_pair<Float, std::string>(
					clock.getElapsedTime().asMilliseconds(), widget.name() ) );
			}

			if ( widget.first_child() ) {
				loadNode( widget.first_child(), uiwidget );
			}

			uiwidget->onWidgetCreated();
		} else if ( String::toLower( widget.name() ) == "style" ) {
			combineStyleSheet( widget.text().as_string(), false );
		}
	}

	return rootWidgets;
}

UIWidget* UISceneNode::loadLayoutNodes( pugi::xml_node node, Node* parent ) {
	Clock clock;
	UISceneNode* prevUISceneNode = SceneManager::instance()->getUISceneNode();
	SceneManager::instance()->setCurrentUISceneNode( this );
	std::string id( node.attribute( "id" ).as_string() );
	mIsLoading = true;
	Clock innerClock;
	std::vector<UIWidget*> widgets = loadNode( node, parent );

	if ( mVerbose ) {
		std::sort(
			mTimes.begin(), mTimes.end(),
			[]( const std::pair<Float, std::string>& left,
				const std::pair<Float, std::string>& right ) { return left.first < right.first; } );

		for ( auto& time : mTimes ) {
			eePRINTL( "Widget %s created in %.2f ms", time.second.c_str(), time.first );
		}

		mTimes.clear();

		eePRINTL( "UISceneNode::loadLayoutNodes loaded nodes%s in: %.2f ms",
				  id.empty() ? "" : std::string( " (id=" + id + ")" ).c_str(),
				  innerClock.getElapsed().asMilliseconds() );
	}

	for ( auto& widget : widgets )
		widget->reloadStyle( true, true, true );

	if ( mVerbose ) {
		eePRINTL( "UISceneNode::loadLayoutNodes reloaded styles in: %.2f ms",
				  innerClock.getElapsed().asMilliseconds() );
	}

	mIsLoading = false;
	SceneManager::instance()->setCurrentUISceneNode( prevUISceneNode );

	if ( mVerbose ) {
		eePRINTL( "UISceneNode::loadLayoutNodes loaded in: %.2f ms",
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
									 const bool& forceReloadStyle ) {
	CSS::StyleSheetParser parser;

	if ( parser.loadFromString( inlineStyleSheet ) )
		combineStyleSheet( parser.getStyleSheet(), forceReloadStyle );
}

CSS::StyleSheet& UISceneNode::getStyleSheet() {
	return mStyleSheet;
}

bool UISceneNode::hasStyleSheet() {
	return !mStyleSheet.isEmpty();
}

void UISceneNode::reloadStyle( const bool& disableAnimations ) {
	if ( NULL != mChild ) {
		Node* child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				child->asType<UIWidget>()->reloadStyle( true, disableAnimations );
			}

			child = child->getNextNode();
		}
	}
}

UIWidget* UISceneNode::loadLayoutFromFile( const std::string& layoutPath, Node* parent ) {
	if ( FileSystem::fileExists( layoutPath ) ) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( layoutPath.c_str() );

		if ( result ) {
			return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
		} else {
			eePRINTL( "Error: Couldn't load UI Layout: %s", layoutPath.c_str() );
			eePRINTL( "Error description: %s", result.description() );
			eePRINTL( "Error offset: %d", result.offset );
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

UIWidget* UISceneNode::loadLayoutFromString( const std::string& layoutString, Node* parent ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string( layoutString.c_str() );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from string: %s", layoutString.c_str() );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget* UISceneNode::loadLayoutFromMemory( const void* buffer, Int32 bufferSize, Node* parent ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( buffer, bufferSize );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from buffer" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget* UISceneNode::loadLayoutFromStream( IOStream& stream, Node* parent ) {
	if ( !stream.isOpen() )
		return NULL;

	ios_size bufferSize = stream.getSize();
	TScopedBuffer<char> scopedBuffer( bufferSize );
	stream.read( scopedBuffer.get(), scopedBuffer.length() );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( scopedBuffer.get(), scopedBuffer.length() );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from stream" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
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
	mDpSize = size;
	mSize = PixelDensity::dpToPx( size );
	updateCenter();
	sendCommonEvent( Event::OnSizeChange );
	invalidateDraw();
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

void UISceneNode::update( const Time& elapsed ) {
	UISceneNode* uiSceneNode = SceneManager::instance()->getUISceneNode();

	SceneManager::instance()->setCurrentUISceneNode( this );

	SceneNode::update( elapsed );

	updateDirtyStyles();

	updateDirtyStyleStates();

	SceneManager::instance()->setCurrentUISceneNode( uiSceneNode );
}

void UISceneNode::onWidgetDelete( Node* node ) {
	if ( node->isWidget() ) {
		UIWidget* widget = node->asType<UIWidget>();

		if ( mDirtyStyle.find( widget ) != mDirtyStyle.end() ) {
			mDirtyStyle.erase( widget );
		}

		if ( mDirtyStyleState.find( widget ) != mDirtyStyleState.end() ) {
			mDirtyStyleState.erase( widget );
		}
	}
}

const bool& UISceneNode::isLoading() const {
	return mIsLoading;
}

UIThemeManager* UISceneNode::getUIThemeManager() const {
	return mUIThemeManager;
}

UIWidget* UISceneNode::getRoot() const {
	return mRoot;
}

bool UISceneNode::getVerbose() const {
	return mVerbose;
}

void UISceneNode::setVerbose( bool verbose ) {
	mVerbose = verbose;
}

void UISceneNode::invalidateStyle( UIWidget* node ) {
	eeASSERT( NULL != node );

	if ( node->isClosing() )
		return;

	Node* itNode = NULL;

	if ( mDirtyStyle.count( node ) > 0 )
		return;

	for ( auto& dirtyCtrl : mDirtyStyle ) {
		if ( NULL != dirtyCtrl && dirtyCtrl->isParentOf( node ) ) {
			return;
		}
	}

	std::vector<std::unordered_set<UIWidget*>::iterator> itEraseList;

	for ( auto it = mDirtyStyle.begin(); it != mDirtyStyle.end(); ++it ) {
		itNode = *it;

		if ( NULL != itNode && node->isParentOf( itNode ) ) {
			itEraseList.push_back( it );
		} else if ( NULL == itNode ) {
			itEraseList.push_back( it );
		}
	}

	for ( auto ite = itEraseList.begin(); ite != itEraseList.end(); ++ite ) {
		mDirtyStyle.erase( *ite );
	}

	mDirtyStyle.insert( node );
}

void UISceneNode::invalidateStyleState( UIWidget* node, bool disableCSSAnimations ) {
	eeASSERT( NULL != node );

	if ( node->isClosing() )
		return;

	Node* itNode = NULL;

	if ( mDirtyStyleState.count( node ) > 0 )
		return;

	for ( auto& dirtyCtrl : mDirtyStyleState ) {
		if ( NULL != dirtyCtrl && dirtyCtrl->isParentOf( node ) ) {
			return;
		}
	}

	std::vector<std::unordered_set<UIWidget*>::iterator> itEraseList;

	for ( auto it = mDirtyStyleState.begin(); it != mDirtyStyleState.end(); ++it ) {
		itNode = *it;

		if ( NULL != itNode && node->isParentOf( itNode ) ) {
			itEraseList.push_back( it );
		} else if ( NULL == itNode ) {
			itEraseList.push_back( it );
		}
	}

	for ( auto ite = itEraseList.begin(); ite != itEraseList.end(); ++ite ) {
		mDirtyStyleState.erase( *ite );
	}

	mDirtyStyleState.insert( node );
	mDirtyStyleStateCSSAnimations[node] = disableCSSAnimations;
}

void UISceneNode::setIsLoading( bool isLoading ) {
	mIsLoading = isLoading;
}

void UISceneNode::updateDirtyStyles() {
	if ( !mDirtyStyle.empty() ) {
		Clock clock;
		for ( auto& node : mDirtyStyle ) {
			node->reloadStyle( true, false, false );
		}
		mDirtyStyle.clear();
		eePRINTL( "CSS Styles Reloaded in %.2f ms", clock.getElapsedTime().asMilliseconds() );
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
		eePRINTL( "CSS Style State Invalidated, reapplied state in %.2f ms",
				  clock.getElapsedTime().asMilliseconds() );
	}
}

bool UISceneNode::onMediaChanged() {
	if ( !mStyleSheet.isMediaQueryListEmpty() ) {
		MediaFeatures media;
		media.type = media_type_screen;
		media.width = mWindow->getWidth();
		media.height = mWindow->getHeight();
		media.deviceWidth = mWindow->getDesktopResolution().getWidth();
		media.deviceHeight = mWindow->getDesktopResolution().getHeight();
		media.color = 8;
		media.monochrome = 0;
		media.colorIndex = 256;
		media.resolution = static_cast<int>( getDPI() );

		if ( mStyleSheet.updateMediaLists( media ) ) {
			mRoot->reportStyleStateChangeRecursive();
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
}

void UISceneNode::loadFontFaces( const StyleSheetStyleVector& styles ) {
	for ( auto& style : styles ) {
		CSS::StyleSheetProperty familyProp( *style->getPropertyById( PropertyId::FontFamily ) );
		CSS::StyleSheetProperty srcProp( *style->getPropertyById( PropertyId::Src ) );

		if ( !familyProp.isEmpty() && !srcProp.isEmpty() ) {
			Font* fontSearch = FontManager::instance()->getByName( familyProp.getValue() );

			if ( NULL == fontSearch ) {
				std::string path( srcProp.getValue() );
				FunctionString func( FunctionString::parse( path ) );

				if ( !func.getParameters().empty() && func.getName() == "url" ) {
					path = func.getParameters().at( 0 );
				}

				if ( String::startsWith( path, "file://" ) ) {
					std::string filePath( path.substr( 7 ) );

					FontTrueType* font =
						FontTrueType::New( String::trim( familyProp.getValue(), '"' ) );

					font->loadFromFile( filePath );

					mFontFaces.push_back( font );
					runOnMainThread( [&] { mRoot->reloadFontFamily(); } );
				} else if ( String::startsWith( path, "http://" ) ||
							String::startsWith( path, "https://" ) ) {
					std::string familyName = familyProp.getValue();
					Http::getAsync(
						[&, familyName]( const Http&, Http::Request&, Http::Response& response ) {
							FontTrueType* font =
								FontTrueType::New( String::trim( familyName, '"' ) );

							if ( !response.getBody().empty() ) {
								font->loadFromMemory( &response.getBody()[0],
													  response.getBody().size() );
								mFontFaces.push_back( font );
								runOnMainThread( [&] { mRoot->reloadFontFamily(); } );
							}
						},
						URI( path ), Seconds( 5 ) );
				} else if ( VFS::instance()->fileExists( path ) ) {
					FontTrueType* font =
						FontTrueType::New( String::trim( familyProp.getValue(), '"' ) );

					IOStream* stream = VFS::instance()->getFileFromPath( path );

					font->loadFromStream( *stream );

					mFontFaces.push_back( font );
					runOnMainThread( [&] { mRoot->reloadFontFamily(); } );
				}
			}
		}
	}
}

Uint32 UISceneNode::onKeyDown( const KeyEvent& Event ) {
	checkShortcuts( Event.getKeyCode(), Event.getMod() );

	return SceneNode::onKeyDown( Event );
}

void UISceneNode::checkShortcuts( const Uint32& KeyCode, const Uint32& Mod ) {
	if ( NULL == getEventDispatcher() )
		return;

	for ( auto& kb : mKbShortcuts ) {
		if ( KeyCode == kb.KeyCode && ( Mod & kb.Mod ) ) {
			getEventDispatcher()->sendMouseUp( kb.Widget, Vector2i( -1, -1 ), EE_BUTTON_LMASK );
			getEventDispatcher()->sendMouseClick( kb.Widget, Vector2i( -1, -1 ), EE_BUTTON_LMASK );
		}
	}
}

KeyboardShortcuts::iterator UISceneNode::existsShortcut( const Uint32& KeyCode,
														 const Uint32& Mod ) {
	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); ++it ) {
		if ( ( *it ).KeyCode == KeyCode && ( *it ).Mod == Mod )
			return it;
	}

	return mKbShortcuts.end();
}

bool UISceneNode::addShortcut( const Uint32& KeyCode, const Uint32& Mod, UIWidget* Widget ) {
	if ( inParentTreeOf( Widget ) && mKbShortcuts.end() == existsShortcut( KeyCode, Mod ) ) {
		mKbShortcuts.push_back( KeyboardShortcut( KeyCode, Mod, Widget ) );

		return true;
	}

	return false;
}

bool UISceneNode::removeShortcut( const Uint32& KeyCode, const Uint32& Mod ) {
	KeyboardShortcuts::iterator it = existsShortcut( KeyCode, Mod );

	if ( mKbShortcuts.end() != it ) {
		mKbShortcuts.erase( it );

		return true;
	}

	return false;
}

}} // namespace EE::UI
