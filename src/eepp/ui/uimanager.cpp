#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <algorithm>

namespace EE { namespace UI {

SINGLETON_DECLARE_IMPLEMENTATION(UIManager)

UIManager::UIManager() :
	mWindow( NULL ),
	mInput( NULL ),
	mControl( NULL ),
	mFocusControl( NULL ),
	mOverControl( NULL ),
	mDownControl( NULL ),
	mLossFocusControl( NULL ),
	mCbId(-1),
	mResizeCb(0),
	mFlags( 0 ),
	mHighlightFocusColor( 234, 195, 123, 255 ),
	mHighlightOverColor( 195, 123, 234, 255 ),
	mHighlightInvalidationColor( 220, 0, 0, 255 ),
	mInit( false ),
	mFirstPress( false ),
	mShootingDown( false ),
	mControlDragging( false ),
	mUseGlobalCursors( true )
{
}

UIManager::~UIManager() {
	shutdown();
}

void UIManager::init( Uint32 Flags, EE::Window::Window * window ) {
	if ( mInit )
		shutdown();

	mWindow		= window;
	mFlags		= Flags;

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	mInput				= mWindow->getInput();

	mInit			= true;

	UIWindowStyleConfig windowStyleConfig;
	windowStyleConfig.WinFlags = UI_WIN_NO_BORDER | UI_WIN_RESIZEABLE;

	if ( isMainControlInFrameBuffer() )
		windowStyleConfig.WinFlags |= UI_WIN_FRAME_BUFFER;

	windowStyleConfig.MinWindowSize = Sizef( 0, 0 );
	windowStyleConfig.DecorationSize = Sizei( 0, 0 );
	windowStyleConfig.DecorationAutoSize = false;
	mControl = UIWindow::New();
	mControl->enableReportSizeChangeToChilds();
	mControl->setStyleConfig( windowStyleConfig );
	mControl->setSize( (Float)mWindow->getWidth() / PixelDensity::getPixelDensity(), (Float)mWindow->getHeight() / PixelDensity::getPixelDensity() );
	mControl->setVisible( true );
	mControl->setEnabled( true );
	mControl->getContainer()->setEnabled( false );
	mControl->getContainer()->setVisible( false );

	if ( mControl->ownsFrameBuffer() ) {
		mControl->getFrameBuffer()->setName( "uimain" );
		RGB cc = mWindow->getClearColor();
		mControl->getFrameBuffer()->setClearColor( ColorAf( cc.r / 255.f, cc.g / 255.f, cc.b / 255.f, 0 ) );
	}

	mFocusControl	= mControl;
	mOverControl	= mControl;

	mCbId = mInput->pushCallback( cb::Make1( this, &UIManager::inputCallback ) );
	mResizeCb = mWindow->pushResizeCallback( cb::Make1( this, &UIManager::resizeControl ) );

	mClock.restart();
}

void UIManager::shutdown() {
	if ( mInit ) {
		if ( -1 != mCbId &&
			NULL != Engine::existsSingleton() &&
			Engine::instance()->existsWindow( mWindow )
		)
		{
			mInput->popCallback( mCbId );
			mWindow->popResizeCallback( mResizeCb );
		}

		mShootingDown = true;

		eeSAFE_DELETE( mControl );

		mShootingDown = false;

		mOverControl = NULL;
		mFocusControl = NULL;

		mInit = false;
	}

	UIThemeManager::destroySingleton();
}

void UIManager::inputCallback( InputEvent * Event ) {
	switch( Event->Type ) {
		case InputEvent::KeyUp:
			sendKeyUp( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );
			break;
		case InputEvent::KeyDown:
			sendKeyDown( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );

			checkTabPress( Event->key.keysym.sym );
			break;
		case InputEvent::SysWM:
		case InputEvent::VideoResize:
		case InputEvent::VideoExpose:
		{
			if ( NULL != mControl )
				mControl->invalidate();
		}
	}
}

void UIManager::resizeControl( EE::Window::Window * win ) {
	mControl->setSize( (Float)mWindow->getWidth() / PixelDensity::getPixelDensity(), (Float)mWindow->getHeight() / PixelDensity::getPixelDensity() );
	sendMsg( mControl, NodeMessage::WindowResize );

	std::list<UIWindow*>::iterator it;

	for ( it = mWindowsList.begin(); it != mWindowsList.end(); ++it ) {
		sendMsg( *it, NodeMessage::WindowResize );
	}
}

void UIManager::sendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	KeyEvent	keyEvent	= KeyEvent( mFocusControl, Event::KeyUp, KeyCode, Char, Mod );
	Node * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->isEnabled() && CtrlLoop->onKeyUp( keyEvent ) )
			break;

		CtrlLoop = CtrlLoop->getParent();
	}
}

void UIManager::sendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	KeyEvent	keyEvent	= KeyEvent( mFocusControl, Event::KeyDown, KeyCode, Char, Mod );
	Node * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->isEnabled() && CtrlLoop->onKeyDown( keyEvent ) )
			break;

		CtrlLoop = CtrlLoop->getParent();
	}
}

Node * UIManager::getFocusControl() const {
	return mFocusControl;
}

Node * UIManager::getLossFocusControl() const {
	return mLossFocusControl;
}

void UIManager::setFocusControl( Node * Ctrl ) {
	if ( NULL != mFocusControl && NULL != Ctrl && Ctrl != mFocusControl ) {
		mLossFocusControl = mFocusControl;

		mFocusControl = Ctrl;

		mLossFocusControl->onFocusLoss();
		sendMsg( mLossFocusControl, NodeMessage::FocusLoss );

		mFocusControl->onFocus();
		sendMsg( mFocusControl, NodeMessage::Focus );
	}
}

Node * UIManager::getOverControl() const {
	return mOverControl;
}

void UIManager::setOverControl( Node * Ctrl ) {
	mOverControl = Ctrl;
}

void UIManager::sendMsg( Node * Ctrl, const Uint32& Msg, const Uint32& Flags ) {
	NodeMessage tMsg( Ctrl, Msg, Flags );

	Ctrl->messagePost( &tMsg );
}

void UIManager::update() {
	update( mClock.getElapsed() );
}

void UIManager::update( const Time& elapsed ) {
	mElapsed = elapsed;

	bool wasDraggingControl = isControlDragging();

	mControl->update( elapsed );

	mMousePos = mInput->getMousePosFromView( mWindow->getDefaultView() );
	mMousePosi = mMousePos.asInt();

	Node * pOver = mControl->overFind( mMousePos );

	if ( pOver != mOverControl ) {
		if ( NULL != mOverControl ) {
			sendMsg( mOverControl, NodeMessage::MouseExit );
			mOverControl->onMouseExit( mMousePosi, 0 );
		}

		mOverControl = pOver;

		if ( NULL != mOverControl ) {
			sendMsg( mOverControl, NodeMessage::MouseEnter );
			mOverControl->onMouseEnter( mMousePosi, 0 );
		}
	} else {
		if ( NULL != mOverControl )
			mOverControl->onMouseMove( mMousePosi, mInput->getPressTrigger() );
	}

	if ( mInput->getPressTrigger() ) {
		if ( NULL != mOverControl ) {
			mOverControl->onMouseDown( mMousePosi, mInput->getPressTrigger() );
			sendMsg( mOverControl, NodeMessage::MouseDown, mInput->getPressTrigger() );
		}

		if ( !mFirstPress ) {
			mDownControl = mOverControl;
			mMouseDownPos = mMousePosi;

			mFirstPress = true;
		}
	}

	if ( mInput->getReleaseTrigger() ) {
		if ( NULL != mFocusControl ) {
			if ( !wasDraggingControl || mMousePos == mLastMousePos ) {
				if ( mOverControl != mFocusControl )
					setFocusControl( mOverControl );

				// The focused control can change after the MouseUp ( since the control can call "setFocus()" on other control
				// And the Click would be received by the new focused control instead of the real one
				Node * lastFocusControl = mFocusControl;

				lastFocusControl->onMouseUp( mMousePosi, mInput->getReleaseTrigger() );
				sendMsg( lastFocusControl, NodeMessage::MouseUp, mInput->getReleaseTrigger() );

				if ( mInput->getClickTrigger() ) {
					sendMsg( lastFocusControl, NodeMessage::Click, mInput->getClickTrigger() );
					lastFocusControl->onMouseClick( mMousePosi, mInput->getClickTrigger() );

					if ( mInput->getDoubleClickTrigger() ) {
						sendMsg( lastFocusControl, NodeMessage::DoubleClick, mInput->getDoubleClickTrigger() );
						lastFocusControl->onMouseDoubleClick( mMousePosi, mInput->getDoubleClickTrigger() );
					}
				}
			}
		}

		mFirstPress = false;
	}

	mLastMousePos = mMousePos;

	checkClose();
}

Node * UIManager::getDownControl() const {
	return mDownControl;
}

void UIManager::draw() {
	GlobalBatchRenderer::instance()->draw();

	const View& prevView = mWindow->getView();

	mWindow->setView( mWindow->getDefaultView() );

	mControl->internalDraw();

	mWindow->setView( prevView );

	GlobalBatchRenderer::instance()->draw();
}

UIWindow * UIManager::getMainControl() const {
	return mControl;
}

const Time& UIManager::getElapsed() const {
	return mElapsed;
}

Vector2i UIManager::getMousePos() {
	return mMousePosi;
}

Vector2f UIManager::getMousePosf() {
	return mMousePos;
}

Input * UIManager::getInput() const {
	return mInput;
}

const Uint32& UIManager::getPressTrigger() const {
	return mInput->getPressTrigger();
}

const Uint32& UIManager::getLastPressTrigger() const {
	return mInput->getLastPressTrigger();
}

void UIManager::clipSmartEnable(Node * ctrl, const Int32 & x, const Int32 & y, const Uint32 & Width, const Uint32 & Height) {
	if ( ctrl->isMeOrParentTreeScaledOrRotatedOrFrameBuffer() ) {
		GLi->getClippingMask()->clipPlaneEnable( x, y, Width, Height );
	} else {
		GLi->getClippingMask()->clipEnable( x, y, Width, Height );
	}
}

void UIManager::clipSmartDisable(Node * ctrl) {
	if ( ctrl->isMeOrParentTreeScaledOrRotatedOrFrameBuffer() ) {
		GLi->getClippingMask()->clipPlaneDisable();
	} else {
		GLi->getClippingMask()->clipDisable();
	}
}

void UIManager::setHighlightFocus( bool Highlight ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_FOCUS, Highlight ? 1 : 0 );
}

bool UIManager::getHighlightFocus() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_FOCUS );
}

void UIManager::setHighlightInvalidation( bool Invalidation ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_INVALIDATION, Invalidation ? 1 : 0 );
}

bool UIManager::getHighlightInvalidation() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_INVALIDATION );
}

void UIManager::setDrawDebugData( bool debug ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_DRAW_DEBUG_DATA, debug ? 1 : 0 );
}

bool UIManager::getDrawDebugData() const {
	return 0 != ( mFlags & UI_MANAGER_DRAW_DEBUG_DATA );
}

void UIManager::setDrawBoxes( bool draw ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_DRAW_BOXES, draw ? 1 : 0 );
}

bool UIManager::getDrawBoxes() const {
	return 0 != ( mFlags & UI_MANAGER_DRAW_BOXES );
}

void UIManager::setHighlightFocusColor( const Color& Color ) {
	mHighlightFocusColor = Color;
}

bool UIManager::usesInvalidation() {
	return 0 != ( mFlags & UI_MANAGER_USE_DRAW_INVALIDATION );
}

void UIManager::setUseInvalidation( const bool& use ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_USE_DRAW_INVALIDATION, use ? 1 : 0 );
}

const Color& UIManager::getHighlightFocusColor() const {
	return mHighlightFocusColor;
}

void UIManager::setHighlightOver( bool Highlight ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_OVER, Highlight ? 1 : 0 );
}

bool UIManager::getHighlightOver() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_OVER );
}

void UIManager::setHighlightOverColor( const Color& Color ) {
	mHighlightOverColor = Color;
}

void UIManager::setMainControlInFrameBuffer(const bool& set) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_MAIN_CONTROL_IN_FRAME_BUFFER, set ? 1 : 0 );

	if ( NULL != mControl ) {
		mControl->setWinFlags( mControl->getWinFlags() | ( set ? UI_WIN_FRAME_BUFFER : 0 ) );
	}
}

bool UIManager::isMainControlInFrameBuffer() const {
	return 0 != ( mFlags & UI_MANAGER_MAIN_CONTROL_IN_FRAME_BUFFER );
}

void UIManager::setMainControlInColorBuffer(const bool& set) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_MAIN_CONTROL_IN_COLOR_BUFFER, set ? 1 : 0 );

	if ( NULL != mControl ) {
		mControl->setWinFlags( mControl->getWinFlags() | ( set ? UI_WIN_COLOR_BUFFER : 0 ) );
	}
}

bool UIManager::isMainControlInColorBuffer() const {
	return 0 != ( mFlags & UI_MANAGER_MAIN_CONTROL_IN_COLOR_BUFFER );
}

const Color& UIManager::getHighlightOverColor() const {
	return mHighlightOverColor;
}

void UIManager::checkTabPress( const Uint32& KeyCode ) {
	eeASSERT( NULL != mFocusControl );

	if ( KeyCode == KEY_TAB && mFocusControl->isUINode() ) {
		Node * Ctrl = static_cast<UINode*>( mFocusControl )->getNextWidget();

		if ( NULL != Ctrl )
			Ctrl->setFocus();
	}
}

void UIManager::sendMouseClick( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::Click, Flags );
	ToCtrl->onMouseClick( Pos, Flags );
}

void UIManager::sendMouseUp( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::MouseUp, Flags );
	ToCtrl->onMouseUp( Pos, Flags );
}

void UIManager::sendMouseDown( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::MouseDown, Flags );
	ToCtrl->onMouseDown( Pos, Flags );
}

EE::Window::Window * UIManager::getWindow() const {
	return mWindow;
}

void UIManager::setFocusLastWindow( UIWindow * window ) {
	if ( !mWindowsList.empty() && window != mWindowsList.front() ) {
		setFocusControl( mWindowsList.front() );
	}
}

void UIManager::windowAdd( UIWindow * win ) {
	if ( !windowExists( win ) ) {
		mWindowsList.push_front( win );
	} else {
		//! Send to front
		mWindowsList.remove( win );
		mWindowsList.push_front( win );
	}
}

void UIManager::windowRemove( UIWindow * win ) {
	if ( windowExists( win ) ) {
		mWindowsList.remove( win );
	}
}

bool UIManager::windowExists( UIWindow * win ) {
	return mWindowsList.end() != std::find( mWindowsList.begin(), mWindowsList.end(), win );
}

const bool& UIManager::isShootingDown() const {
	return mShootingDown;
}

const Vector2i &UIManager::getMouseDownPos() const {
	return mMouseDownPos;
}

void UIManager::addToCloseQueue( Node * Ctrl ) {
	eeASSERT( NULL != Ctrl );

	std::list<Node*>::iterator it;
	Node * itCtrl = NULL;

	for ( it = mCloseList.begin(); it != mCloseList.end(); ++it ) {
		itCtrl = *it;

		if ( NULL != itCtrl && itCtrl->isParentOf( Ctrl ) ) {
			// If a parent will be removed, means that the control
			// that we are trying to queue will be removed by the father
			// so we skip it
			return;
		}
	}

	std::list< std::list<Node*>::iterator > itEraseList;

	for ( it = mCloseList.begin(); it != mCloseList.end(); ++it ) {
		itCtrl = *it;

		if ( NULL != itCtrl && Ctrl->isParentOf( itCtrl ) ) {
			// if the control added is parent of another control already added,
			// we remove the already added control because it will be deleted
			// by its parent
			itEraseList.push_back( it );
		} else if ( NULL == itCtrl ) {
			itEraseList.push_back( it );
		}
	}

	// We delete all the controls that don't need to be deleted
	// because of the new added control to the queue
	std::list< std::list<Node*>::iterator >::iterator ite;

	for ( ite = itEraseList.begin(); ite != itEraseList.end(); ++ite ) {
		mCloseList.erase( *ite );
	}

	mCloseList.push_back( Ctrl );
}

void UIManager::checkClose() {
	if ( !mCloseList.empty() ) {
		for ( std::list<Node*>::iterator it = mCloseList.begin(); it != mCloseList.end(); ++it ) {
			eeDelete( *it );
		}

		mCloseList.clear();
	}
}

void UIManager::setControlDragging( bool dragging ) {
	mControlDragging = dragging;
}

const bool& UIManager::isControlDragging() const {
	return mControlDragging;
}

void UIManager::setUseGlobalCursors( const bool& use ) {
	mUseGlobalCursors = use;
}

const bool& UIManager::getUseGlobalCursors() {
	return mUseGlobalCursors;
}

void UIManager::setCursor( EE_CURSOR_TYPE cursor ) {
	if ( mUseGlobalCursors ) {
		mWindow->getCursorManager()->set( cursor );
	}
}

void UIManager::setTranslator( Translator translator ) {
	mTranslator = translator;
}

String UIManager::getTranslatorString( const std::string & str ) {
	if ( String::startsWith( str, "@string/" ) ) {
		String tstr = mTranslator.getString( str.substr( 8 ) );

		if ( !tstr.empty() )
			return tstr;
	}

	return String( str );
}

const Color& UIManager::getHighlightInvalidationColor() const {
	return mHighlightInvalidationColor;
}

void UIManager::setHighlightInvalidationColor(const Color & highlightInvalidationColor) {
	mHighlightInvalidationColor = highlightInvalidationColor;
}

UIWidget * UIManager::loadLayoutNodes( pugi::xml_node node, Node * parent ) {
	UIWidget * firstWidget = NULL;

	if ( NULL == parent )
		parent = getMainControl();

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

			uiwidget->onWidgetCreated();
		}
	}

	return firstWidget;
}

UIWidget * UIManager::loadLayoutFromFile( const std::string& layoutPath, Node * parent ) {
	if ( FileSystem::fileExists( layoutPath ) ) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( layoutPath.c_str() );

		if ( result ) {
			return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : getMainControl() );
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

UIWidget * UIManager::loadLayoutFromString( const std::string& layoutString, Node * parent ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string( layoutString.c_str() );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : getMainControl() );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from string: %s", layoutString.c_str() );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget * UIManager::loadLayoutFromMemory( const void * buffer, Int32 bufferSize, Node * parent ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( buffer, bufferSize);

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : getMainControl() );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from buffer" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget * UIManager::loadLayoutFromStream( IOStream& stream, Node * parent ) {
	if ( !stream.isOpen() )
		return NULL;

	ios_size bufferSize = stream.getSize();
	SafeDataPointer safeDataPointer( eeNewArray( Uint8, bufferSize ), bufferSize );
	stream.read( reinterpret_cast<char*>( safeDataPointer.data ), safeDataPointer.size );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( safeDataPointer.data, safeDataPointer.size );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : getMainControl() );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from stream" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget * UIManager::loadLayoutFromPack( Pack * pack, const std::string& FilePackPath, Node * parent ) {
	SafeDataPointer PData;

	if ( pack->isOpen() && pack->extractFileToMemory( FilePackPath, PData ) ) {
		return loadLayoutFromMemory( PData.data, PData.size, parent );
	}

	return NULL;
}

}}
