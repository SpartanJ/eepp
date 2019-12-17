#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/virtualfilesystem.hpp>
#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
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

UIWidget* UISceneNode::loadLayoutNodes( pugi::xml_node node, Node* parent ) {
	mIsLoading = true;
	UIWidget* firstWidget = NULL;

	if ( NULL == parent )
		parent = this;

	for ( pugi::xml_node widget = node; widget; widget = widget.next_sibling() ) {
		UIWidget* uiwidget = UIWidgetCreator::createFromName( widget.name() );

		if ( NULL != uiwidget ) {
			if ( NULL == firstWidget ) {
				firstWidget = uiwidget;
			}

			uiwidget->setParent( parent );
			uiwidget->loadFromXmlNode( widget );

			if ( widget.first_child() ) {
				loadLayoutNodes( widget.first_child(), uiwidget );
			}

			uiwidget->reloadStyle( false );
			uiwidget->onWidgetCreated();
		} else if ( String::toLower( widget.name() ) == "style" ) {
			combineStyleSheet( widget.text().as_string() );
		}
	}

	mIsLoading = false;

	return firstWidget;
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

void UISceneNode::combineStyleSheet( const CSS::StyleSheet& styleSheet ) {
	mStyleSheet.combineStyleSheet( styleSheet );
	processStyleSheetAtRules( styleSheet );
	onMediaChanged();
	reloadStyle();
}

void UISceneNode::combineStyleSheet( const std::string& inlineStyleSheet ) {
	CSS::StyleSheetParser parser;

	if ( parser.loadFromString( inlineStyleSheet ) )
		combineStyleSheet( parser.getStyleSheet() );
}

CSS::StyleSheet& UISceneNode::getStyleSheet() {
	return mStyleSheet;
}

bool UISceneNode::hasStyleSheet() {
	return !mStyleSheet.isEmpty();
}

void UISceneNode::reloadStyle() {
	if ( NULL != mChild ) {
		Node* ChildLoop = mChild;

		while ( NULL != ChildLoop ) {
			if ( ChildLoop->isWidget() )
				ChildLoop->asType<UIWidget>()->reloadStyle();

			ChildLoop = ChildLoop->getNextNode();
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

const bool& UISceneNode::isLoading() const {
	return mIsLoading;
}

UIThemeManager* UISceneNode::getUIThemeManager() const {
	return mUIThemeManager;
}

UIWidget* UISceneNode::getRoot() const {
	return mRoot;
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
			reloadStyle();
			return true;
		}
	}

	return false;
}

void UISceneNode::onChildCountChange() {
	Node* child = mChild;
	bool found = false;

	while ( NULL != child ) {
		if ( child != mRoot ) {
			found = true;
			break;
		}

		child = child->getNextNode();
	}

	if ( found ) {
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
		CSS::StyleSheetProperty familyProp( style.getPropertyById( PropertyId::FontFamily ) );
		CSS::StyleSheetProperty srcProp( style.getPropertyById( PropertyId::Src ) );

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
				} else if ( String::startsWith( path, "http://" ) ||
							String::startsWith( path, "https://" ) ) {

					FontTrueType* font =
						FontTrueType::New( String::trim( familyProp.getValue(), '"' ) );

					Http::getAsync(
						[&, font]( const Http&, Http::Request&, Http::Response& response ) {
							if ( !response.getBody().empty() ) {
								font->loadFromMemory( &response.getBody()[0],
													  response.getBody().size() );
								mFontFaces.push_back( font );
								runOnMainThread( [&] { reloadStyle(); } );
							}
						},
						URI( path ), Seconds( 5 ) );
				} else if ( VFS::instance()->fileExists( path ) ) {
					FontTrueType* font =
						FontTrueType::New( String::trim( familyProp.getValue(), '"' ) );

					IOStream * stream = VFS::instance()->getFileFromPath( path );

					font->loadFromStream( *stream );

					mFontFaces.push_back( font );
				}
			}
		}
	}
}

}} // namespace EE::UI
