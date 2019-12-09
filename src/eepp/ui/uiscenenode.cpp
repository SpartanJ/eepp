#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/window.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <pugixml/pugixml.hpp>
#include <algorithm>

namespace EE { namespace UI {

UISceneNode * UISceneNode::New( EE::Window::Window * window ) {
	return eeNew( UISceneNode, ( window ) );
}

UISceneNode::UISceneNode( EE::Window::Window * window ) :
	SceneNode( window ),
	mIsLoading( false ),
	mUIThemeManager( UIThemeManager::New() )
{
	// Update only UI elements that requires it.
	setUpdateAllChilds( false );

	mNodeFlags |= NODE_FLAG_UISCENENODE | NODE_FLAG_OVER_FIND_ALLOWED;

	setEventDispatcher( UIEventDispatcher::New( this ) );

	resizeControl( mWindow );
}

UISceneNode::~UISceneNode() {
	eeSAFE_DELETE( mUIThemeManager );
}

void UISceneNode::resizeControl( EE::Window::Window * ) {
	setSize( eefloor( mWindow->getWidth() / PixelDensity::getPixelDensity() ), eefloor(mWindow->getHeight() / PixelDensity::getPixelDensity()) );
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

void UISceneNode::setFocusLastWindow( UIWindow * window ) {
	if ( NULL != mEventDispatcher && !mWindowsList.empty() && window != mWindowsList.front() ) {
		mEventDispatcher->setFocusControl( mWindowsList.front() );
	}
}

void UISceneNode::windowAdd( UIWindow * win ) {
	if ( !windowExists( win ) ) {
		mWindowsList.push_front( win );
	} else {
		//! Send to front
		mWindowsList.remove( win );
		mWindowsList.push_front( win );
	}
}

void UISceneNode::windowRemove( UIWindow * win ) {
	if ( windowExists( win ) ) {
		mWindowsList.remove( win );
	}
}

bool UISceneNode::windowExists( UIWindow * win ) {
	return mWindowsList.end() != std::find( mWindowsList.begin(), mWindowsList.end(), win );
}

UIWidget * UISceneNode::loadLayoutNodes( pugi::xml_node node, Node * parent ) {
	mIsLoading = true;
	UIWidget * firstWidget = NULL;

	if ( NULL == parent )
		parent = this;

	for ( pugi::xml_node widget = node; widget; widget = widget.next_sibling() ) {
		UIWidget * uiwidget = UIWidgetCreator::createFromName( widget.name() );

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

	reloadStyle();
}

void UISceneNode::setStyleSheet( const std::string& inlineStyleSheet ) {
	CSS::StyleSheetParser parser;

	if ( parser.loadFromString( inlineStyleSheet ) )
		setStyleSheet( parser.getStyleSheet() );
}

void UISceneNode::combineStyleSheet( const CSS::StyleSheet& styleSheet ) {
	mStyleSheet.combineStyleSheet( styleSheet );

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
		Node * ChildLoop = mChild;

		while ( NULL != ChildLoop ) {
			if ( ChildLoop->isWidget() )
				static_cast<UIWidget*>( ChildLoop )->reloadStyle();

			ChildLoop = ChildLoop->getNextNode();
		}
	}
}

UIWidget * UISceneNode::loadLayoutFromFile( const std::string& layoutPath, Node * parent ) {
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
		Pack * pack = PackManager::instance()->exists( path );

		if ( NULL != pack ) {
			return loadLayoutFromPack( pack, path, parent );
		}
	}

	return NULL;
}

UIWidget * UISceneNode::loadLayoutFromString( const std::string& layoutString, Node * parent ) {
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

UIWidget * UISceneNode::loadLayoutFromMemory( const void * buffer, Int32 bufferSize, Node * parent ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( buffer, bufferSize);

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from buffer" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget * UISceneNode::loadLayoutFromStream( IOStream& stream, Node * parent ) {
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

UIWidget * UISceneNode::loadLayoutFromPack( Pack * pack, const std::string& FilePackPath, Node * parent ) {
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

Node * UISceneNode::setSize( const Sizef & Size ) {
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

Node * UISceneNode::setSize(const Float & Width, const Float & Height) {
	return setSize( Vector2f( Width, Height ) );
}

const Sizef &UISceneNode::getSize() const {
	return mDpSize;
}

const bool& UISceneNode::isLoading() const {
	return mIsLoading;
}

UIThemeManager* UISceneNode::getUIThemeManager() const {
	return mUIThemeManager;
}

}}
