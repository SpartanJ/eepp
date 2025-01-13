#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace Scene {

SceneNode* SceneNode::New( EE::Window::Window* window ) {
	return eeNew( SceneNode, ( window ) );
}

SceneNode::SceneNode( EE::Window::Window* window ) :
	Node(),
	mWindow( window ),
	mActionManager( ActionManager::New() ),
	mFrameBuffer( NULL ),
	mEventDispatcher( NULL ),
	mFrameBufferBound( false ),
	mUseInvalidation( false ),
	mUseGlobalCursors( true ),
	mUpdateAllChilds( true ),
	mResizeCb( -1 ),
	mDrawDebugData( false ),
	mDrawBoxes( false ),
	mHighlightOver( false ),
	mHighlightFocus( false ),
	mHighlightInvalidation( false ),
	mHighlightFocusColor( 234, 195, 123, 255 ),
	mHighlightOverColor( 195, 123, 234, 255 ),
	mHighlightInvalidationColor( 220, 0, 0, 255 ) {
	mNodeFlags |= NODE_FLAG_SCENENODE;
	mSceneNode = this;

	enableReportSizeChangeToChilds();

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	mResizeCb = mWindow->pushResizeCallback( [this]( auto win ) { resizeNode( win ); } );

	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	int currentDisplayIndex = getWindow()->getCurrentDisplayIndex();
	Display* currentDisplay = displayManager->getDisplayIndex( currentDisplayIndex );
	mDPI = currentDisplay->getDPI();

	resizeNode( window );
}

SceneNode::~SceneNode() {
	if ( -1 != mResizeCb && NULL != Engine::existsSingleton() &&
		 Engine::instance()->existsWindow( mWindow ) ) {
		mWindow->popResizeCallback( mResizeCb );
	}

	onClose();

	eeSAFE_DELETE( mActionManager );

	if ( !mParentNode )
		eeSAFE_DELETE( mEventDispatcher );

	eeSAFE_DELETE( mFrameBuffer );
}

void SceneNode::enableFrameBuffer() {
	if ( NULL == mFrameBuffer )
		createFrameBuffer();
}

void SceneNode::disableFrameBuffer() {
	eeSAFE_DELETE( mFrameBuffer );

	writeNodeFlag( NODE_FLAG_FRAME_BUFFER, 0 );
}

bool SceneNode::ownsFrameBuffer() const {
	return 0 != ( mNodeFlags & NODE_FLAG_FRAME_BUFFER );
}

void SceneNode::draw() {
	GlobalBatchRenderer::instance()->draw();

	const View& prevView = mWindow->getView();

	mWindow->setView( mWindow->getDefaultView() );

	if ( mVisible && 0 != mAlpha ) {
		updateScreenPos();

		preDraw();

		ClippingMask* clippingMask = GLi->getClippingMask();

		const std::vector<Rectf>& clips = clippingMask->getPlanesClipped();

		if ( !clips.empty() )
			clippingMask->clipPlaneDisable();

		matrixSet();

		if ( NULL == mFrameBuffer || !usesInvalidation() || invalidated() ) {
			bool needsClipPlanes = isClipped() && isMeOrParentTreeScaledOrRotatedOrFrameBuffer();

			clipStart( needsClipPlanes );

			drawChilds();

			clipEnd( needsClipPlanes );
		}

		matrixUnset();

		if ( !clips.empty() )
			clippingMask->setPlanesClipped( clips );

		postDraw();

		writeNodeFlag( NODE_FLAG_VIEW_DIRTY, 0 );
	}

	mWindow->setView( prevView );

	GlobalBatchRenderer::instance()->draw();

	if ( mVerbose && mFirstFrame ) {
		mFirstFrame = false;
		Log::info( "First frame in SceneNode took %.2f ms",
				   mClock.getElapsedTime().asMilliseconds() );
	}
}

void SceneNode::update( const Time& time ) {
	mElapsed = time;

	mActionManager->update( time );

	if ( NULL == mParentNode && NULL != mEventDispatcher )
		mEventDispatcher->update( time );

	checkClose();

	if ( !mScheduledUpdateRemove.empty() ) {
		for ( auto it = mScheduledUpdateRemove.begin(); it != mScheduledUpdateRemove.end(); ++it )
			mScheduledUpdate.erase( *it );

		mScheduledUpdateRemove.clear();
	}

	if ( !mScheduledUpdate.empty() ) {
		for ( auto& node : mScheduledUpdate )
			node->scheduledUpdate( time );
	}

	if ( mUpdateAllChilds ) {
		Node::update( time );
	} else {
		for ( auto& nodeOver : mMouseOverNodes )
			nodeOver->writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 0 );
	}

	mMouseOverNodes.clear();
}

void SceneNode::onSizeChange() {
	if ( NULL != mFrameBuffer && ( mFrameBuffer->getWidth() < mSize.getWidth() ||
								   mFrameBuffer->getHeight() < mSize.getHeight() ) ) {
		if ( NULL == mFrameBuffer ) {
			createFrameBuffer();
		} else {
			Sizei fboSize( getFrameBufferSize() );
			mFrameBuffer->resize( fboSize.getWidth(), fboSize.getHeight() );
		}
	}

	Node::onSizeChange();
}

void SceneNode::addToCloseQueue( Node* node ) {
	eeASSERT( NULL != node );

	Node* itNode = NULL;

	if ( mCloseList.count( node ) > 0 )
		return;

	// If the parent is closing all his children, we skip all the verifications and add it to the
	// close list
	if ( node->getParent() && node->getParent()->isClosingChildren() ) {
		mCloseList.insert( node );
		return;
	}

	for ( auto& closeNode : mCloseList ) {
		if ( closeNode && closeNode->isParentOf( node ) ) {
			// If a parent will be removed, means that the node
			// that we are trying to queue will be removed by the father
			// so we skip it
			return;
		}
	}

	std::vector<CloseList::iterator> itEraseList;

	for ( auto it = mCloseList.begin(); it != mCloseList.end(); ++it ) {
		itNode = *it;

		if ( NULL == itNode || node->isParentOf( itNode ) ) {
			// if the node added is parent of another node already added,
			// we remove the already added node because it will be deleted
			// by its parent
			itEraseList.push_back( it );
		}
	}

	// We delete all the nodes that don't need to be deleted
	// because of the new added node to the queue
	for ( auto ite = itEraseList.begin(); ite != itEraseList.end(); ++ite ) {
		mCloseList.erase( *ite );
	}

	mCloseList.insert( node );
}

bool SceneNode::removeFromCloseQueue( Node* node ) {
	auto it = mCloseList.find( node );
	if ( it != mCloseList.end() ) {
		mCloseList.erase( it );
		return true;
	}
	return false;
}

void SceneNode::checkClose() {
	if ( !mCloseList.empty() ) {
		// First we need to create a temporal copy of the close list because it can change its
		// content while deleting the elements, since the elements can call to any node close()
		// at any moment (and in this case during the deletion of a node).
		// Once copied we need to clear the close list and start the new close list for the next
		// check.
		CloseList closeListCopy( mCloseList );
		mCloseList.clear();

		for ( Node* node : closeListCopy )
			eeSAFE_DELETE( node );
	}
}

Sizei SceneNode::getFrameBufferSize() {
	return mSize.ceil().asInt();
}

void SceneNode::createFrameBuffer() {
	writeNodeFlag( NODE_FLAG_FRAME_BUFFER, 1 );
	eeSAFE_DELETE( mFrameBuffer );
	Sizei fboSize( getFrameBufferSize() );
	if ( fboSize.getWidth() < 1 )
		fboSize.setWidth( 1 );
	if ( fboSize.getHeight() < 1 )
		fboSize.setHeight( 1 );
	mFrameBuffer =
		FrameBuffer::New( fboSize.getWidth(), fboSize.getHeight(), true, false, false, 4, mWindow );

	// Frame buffer failed to create?
	if ( !mFrameBuffer->created() ) {
		eeSAFE_DELETE( mFrameBuffer );
	}
}

void SceneNode::drawFrameBuffer() {
	if ( NULL != mFrameBuffer ) {
		if ( mFrameBuffer->hasColorBuffer() ) {
			mFrameBuffer->draw( Rect( 0, 0, mSize.getWidth(), mSize.getHeight() ),
								Rect( mScreenPos.x, mScreenPos.y, mScreenPos.x + mSize.getWidth(),
									  mScreenPos.y + mSize.getHeight() ) );
		} else {
			Rect r = Rect( 0, 0, mSize.getWidth(), mSize.getHeight() );
			TextureRegion textureRegion( mFrameBuffer->getTexture(), r, r.getSize().asFloat() );
			textureRegion.draw( mScreenPosi.x, mScreenPosi.y, Color::White, getRotation(),
								getScale() );
		}
	}
}

void SceneNode::enableDrawInvalidation() {
	mUseInvalidation = true;
}

void SceneNode::disableDrawInvalidation() {
	mUseInvalidation = false;
}

EE::Window::Window* SceneNode::getWindow() {
	return mWindow;
}

void SceneNode::matrixSet() {
	if ( NULL != mFrameBuffer ) {
		if ( !mUseInvalidation || invalidated() ) {
			mFrameBufferBound = true;

			mFrameBuffer->bind();

			mFrameBuffer->clear();
		}

		if ( 0.f != mScreenPos ) {
			GLi->pushMatrix();
			GLi->translatef( -mScreenPos.x, -mScreenPos.y, 0.f );
		}
	} else {
		Node::matrixSet();
	}
}

void SceneNode::matrixUnset() {
	if ( NULL != mFrameBuffer ) {
		GlobalBatchRenderer::instance()->draw();

		if ( 0.f != mScreenPos )
			GLi->popMatrix();

		if ( mFrameBufferBound ) {
			mFrameBuffer->unbind();

			mFrameBufferBound = false;
		}

		drawFrameBuffer();
	} else {
		Node::matrixUnset();
	}
}

void SceneNode::sendMsg( Node* node, const Uint32& msg, const Uint32& flags ) {
	NodeMessage tMsg( node, msg, flags );
	node->messagePost( &tMsg );
}

void SceneNode::resizeNode( EE::Window::Window* ) {
	setSize( (Float)mWindow->getWidth(), (Float)mWindow->getHeight() );
	sendMsg( this, NodeMessage::WindowResize );
}

FrameBuffer* SceneNode::getFrameBuffer() const {
	return mFrameBuffer;
}

void SceneNode::setEventDispatcher( EventDispatcher* eventDispatcher ) {
	mEventDispatcher = eventDispatcher;
}

EventDispatcher* SceneNode::getEventDispatcher() const {
	return mEventDispatcher;
}

void SceneNode::setDrawDebugData( bool debug ) {
	if ( mDrawDebugData != debug ) {
		mDrawDebugData = debug;

		onDrawDebugDataChange();
	}
}

bool SceneNode::getDrawDebugData() const {
	return mDrawDebugData;
}

void SceneNode::setDrawBoxes( bool draw ) {
	mDrawBoxes = draw;
	invalidateDraw();
}

bool SceneNode::getDrawBoxes() const {
	return mDrawBoxes;
}

void SceneNode::setHighlightOver( bool Highlight ) {
	mHighlightOver = Highlight;
	invalidateDraw();
}

bool SceneNode::getHighlightOver() const {
	return mHighlightOver;
}

void SceneNode::setHighlightFocus( bool Highlight ) {
	mHighlightFocus = Highlight;
	invalidateDraw();
}

bool SceneNode::getHighlightFocus() const {
	return mHighlightFocus;
}

void SceneNode::setHighlightInvalidation( bool Highlight ) {
	mHighlightInvalidation = Highlight;
	invalidateDraw();
}

bool SceneNode::getHighlightInvalidation() const {
	return mHighlightInvalidation;
}

void SceneNode::setHighlightOverColor( const Color& color ) {
	mHighlightOverColor = color;
	invalidateDraw();
}

const Color& SceneNode::getHighlightOverColor() const {
	return mHighlightOverColor;
}

void SceneNode::setHighlightFocusColor( const Color& color ) {
	mHighlightFocusColor = color;
	invalidateDraw();
}

const Color& SceneNode::getHighlightFocusColor() const {
	return mHighlightFocusColor;
}

void SceneNode::setHighlightInvalidationColor( const Color& color ) {
	mHighlightInvalidationColor = color;
	invalidateDraw();
}

const Color& SceneNode::getHighlightInvalidationColor() const {
	return mHighlightInvalidationColor;
}

const Time& SceneNode::getElapsed() const {
	return mElapsed;
}

bool SceneNode::usesInvalidation() const {
	return mUseInvalidation;
}

void SceneNode::setUseGlobalCursors( const bool& use ) {
	mUseGlobalCursors = use;
}

const bool& SceneNode::getUseGlobalCursors() {
	return mUseGlobalCursors;
}

void SceneNode::setCursor( Cursor::Type cursor ) {
	if ( mUseGlobalCursors ) {
		mWindow->getCursorManager()->set( cursor );
	}
}

bool SceneNode::isDrawInvalidator() const {
	return true;
}

ActionManager* SceneNode::getActionManager() const {
	return mActionManager;
}

void SceneNode::preDraw() {}

void SceneNode::postDraw() {}

void SceneNode::onDrawDebugDataChange() {}

void SceneNode::subscribeScheduledUpdate( Node* node ) {
	mScheduledUpdate.insert( node );
	auto it = mScheduledUpdateRemove.find( node );
	if ( it != mScheduledUpdateRemove.end() )
		mScheduledUpdateRemove.erase( it );
}

void SceneNode::unsubscribeScheduledUpdate( Node* node ) {
	mScheduledUpdateRemove.insert( node );
}

bool SceneNode::isSubscribedForScheduledUpdate( Node* node ) {
	return mScheduledUpdate.count( node ) > 0;
}

void SceneNode::addMouseOverNode( Node* node ) {
	mMouseOverNodes.insert( node );
}

void SceneNode::removeMouseOverNode( Node* node ) {
	mMouseOverNodes.erase( node );
}

const bool& SceneNode::getUpdateAllChilds() const {
	return mUpdateAllChilds;
}

void SceneNode::setUpdateAllChilds( const bool& updateAllChilds ) {
	mUpdateAllChilds = updateAllChilds;
}

const Float& SceneNode::getDPI() const {
	return mDPI;
}

bool SceneNode::getVerbose() const {
	return mVerbose;
}

void SceneNode::setVerbose( bool verbose ) {
	mVerbose = verbose;
}

}} // namespace EE::Scene
