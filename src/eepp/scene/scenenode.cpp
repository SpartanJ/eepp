#include <eepp/scene/scenenode.hpp>
#include <eepp/window/window.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>

namespace EE { namespace Scene {

SceneNode * SceneNode::New( EE::Window::Window * window ) {
	return eeNew( SceneNode, ( window ) );
}

SceneNode::SceneNode( EE::Window::Window * window ) :
	Node(),
	mWindow( window ),
	mFrameBuffer( NULL ),
	mEventDispatcher( NULL ),
	mFrameBufferBound( false ),
	mUseInvalidation( false ),
	mUseGlobalCursors( true ),
	mResizeCb( -1 ),
	mDrawDebugData( false ),
	mDrawBoxes( false ),
	mHighlightOver( false ),
	mHighlightFocus( false ),
	mHighlightInvalidation( false ),
	mHighlightFocusColor( 234, 195, 123, 255 ),
	mHighlightOverColor( 195, 123, 234, 255 ),
	mHighlightInvalidationColor( 220, 0, 0, 255 )
{
	mNodeFlags |= NODE_FLAG_SCENENODE;
	mSceneNode = this;

	enableReportSizeChangeToChilds();

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	mResizeCb = mWindow->pushResizeCallback( cb::Make1( this, &SceneNode::resizeControl ) );

	resizeControl( window );
}

SceneNode::~SceneNode() {
	if ( -1 != mResizeCb && NULL != Engine::existsSingleton() && Engine::instance()->existsWindow( mWindow ) ) {
		mWindow->popResizeCallback( mResizeCb );
	}

	onClose();

	eeSAFE_DELETE( mEventDispatcher );

	eeSAFE_DELETE( mFrameBuffer );
}

void SceneNode::enableFrameBuffer() {
	if ( NULL == mFrameBuffer )
		createFrameBuffer();
}

void SceneNode::disableFrameBuffer() {
	eeSAFE_DELETE( mFrameBuffer );

	writeCtrlFlag( NODE_FLAG_FRAME_BUFFER, 0 );
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

		ClippingMask * clippingMask = GLi->getClippingMask();

		std::list<Rectf> clips = clippingMask->getPlanesClipped();

		if ( !clips.empty() )
			clippingMask->clipPlaneDisable();

		matrixSet();

		if ( NULL == mFrameBuffer || !usesInvalidation() || invalidated() ) {
			clipStart();

			drawChilds();

			clipEnd();
		}

		matrixUnset();

		if ( !clips.empty() )
			clippingMask->setPlanesClipped( clips );

		postDraw();

		writeCtrlFlag( NODE_FLAG_VIEW_DIRTY, 0 );
	}

	mWindow->setView( prevView );

	GlobalBatchRenderer::instance()->draw();
}

void SceneNode::update( const Time& elapsed ) {
	mElapsed = elapsed;

	if ( NULL != mEventDispatcher )
		mEventDispatcher->update( elapsed );

	checkClose();

	Node::update( elapsed );
}

void SceneNode::onSizeChange() {
	if ( NULL != mFrameBuffer && ( mFrameBuffer->getWidth() < mSize.getWidth() || mFrameBuffer->getHeight() < mSize.getHeight() ) ) {
		if ( NULL == mFrameBuffer ) {
			createFrameBuffer();
		} else {
			Sizei fboSize( getFrameBufferSize() );
			mFrameBuffer->resize( fboSize.getWidth(), fboSize.getHeight() );
		}
	}

	Node::onSizeChange();
}

void SceneNode::addToCloseQueue( Node * Ctrl ) {
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

void SceneNode::checkClose() {
	if ( !mCloseList.empty() ) {
		for ( std::list<Node*>::iterator it = mCloseList.begin(); it != mCloseList.end(); ++it ) {
			eeDelete( *it );
		}

		mCloseList.clear();
	}
}

Sizei SceneNode::getFrameBufferSize() {
	return mSize.ceil().asInt();
}

void SceneNode::createFrameBuffer() {
	writeCtrlFlag( NODE_FLAG_FRAME_BUFFER, 1 );
	eeSAFE_DELETE( mFrameBuffer );
	Sizei fboSize( getFrameBufferSize() );
	if ( fboSize.getWidth() < 1 ) fboSize.setWidth(1);
	if ( fboSize.getHeight() < 1 ) fboSize.setHeight(1);
	mFrameBuffer = FrameBuffer::New( fboSize.getWidth(), fboSize.getHeight(), true, false, false, 4, mWindow );
}

void SceneNode::drawFrameBuffer() {
	if ( NULL != mFrameBuffer ) {
		if ( mFrameBuffer->hasColorBuffer() ) {
			mFrameBuffer->draw( Rect( 0, 0, mSize.getWidth(), mSize.getHeight() ), Rect( mScreenPos.x, mScreenPos.y, mScreenPos.x + mSize.getWidth(), mScreenPos.y + mSize.getHeight() ) );
		} else {
			TextureRegion textureRegion( mFrameBuffer->getTexture()->getId(), Rect( 0, 0, mSize.getWidth(), mSize.getHeight() ) );
			textureRegion.draw( mScreenPosi.x, mScreenPosi.y, Color::White, getRotation(), getScale() );
		}
	}
}

void SceneNode::enableDrawInvalidation() {
	mUseInvalidation = true;
}

void SceneNode::disableDrawInvalidation() {
	mUseInvalidation = false;
}

EE::Window::Window * SceneNode::getWindow() {
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
			GLi->translatef( -mScreenPos.x , -mScreenPos.y, 0.f );
		}
	}  else {
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

void SceneNode::sendMsg( Node * Ctrl, const Uint32& Msg, const Uint32& Flags ) {
	NodeMessage tMsg( Ctrl, Msg, Flags );
	Ctrl->messagePost( &tMsg );
}

void SceneNode::resizeControl( EE::Window::Window * win ) {
	setSize( (Float)mWindow->getWidth() , (Float)mWindow->getHeight() );
	sendMsg( this, NodeMessage::WindowResize );
}

void SceneNode::invalidate() {
	if ( mVisible && mAlpha != 0.f ) {
		writeCtrlFlag( NODE_FLAG_VIEW_DIRTY, 1 );
	}
}

bool SceneNode::invalidated() {
	return 0 != ( mNodeFlags & NODE_FLAG_VIEW_DIRTY );
}

FrameBuffer * SceneNode::getFrameBuffer() const {
	return mFrameBuffer;
}

void SceneNode::setEventDispatcher( EventDispatcher * eventDispatcher ) {
	mEventDispatcher = eventDispatcher;
}

EventDispatcher * SceneNode::getEventDispatcher() const {
	return mEventDispatcher;
}

void SceneNode::setDrawDebugData( bool debug ) {
	mDrawDebugData = debug;
}

bool SceneNode::getDrawDebugData() const {
	return mDrawDebugData;
}

void SceneNode::setDrawBoxes( bool draw ) {
	mDrawBoxes = draw;
}

bool SceneNode::getDrawBoxes() const {
	return mDrawBoxes;
}

void SceneNode::setHighlightOver( bool Highlight ) {
	mHighlightOver = Highlight;
}

bool SceneNode::getHighlightOver() const {
	return mHighlightOver;
}

void SceneNode::setHighlightFocus( bool Highlight ) {
	mHighlightFocus = Highlight;
}

bool SceneNode::getHighlightFocus() const {
	return mHighlightFocus;
}

void SceneNode::setHighlightInvalidation( bool Highlight ) {
	mHighlightInvalidation = Highlight;
}

bool SceneNode::getHighlightInvalidation() const {
	return mHighlightInvalidation;
}

void SceneNode::setHighlightOverColor( const Color& color ) {
	mHighlightOverColor = color;
}

const Color& SceneNode::getHighlightOverColor() const {
	return mHighlightOverColor;
}

void SceneNode::setHighlightFocusColor( const Color& color ) {
	mHighlightFocusColor = color;
}

const Color& SceneNode::getHighlightFocusColor() const {
	return mHighlightFocusColor;
}

void SceneNode::setHighlightInvalidationColor( const Color& color ) {
	mHighlightInvalidationColor = color;
}

const Color& SceneNode::getHighlightInvalidationColor() const {
	return mHighlightInvalidationColor;
}

const Time &SceneNode::getElapsed() const {
	return mElapsed;
}

bool SceneNode::usesInvalidation() {
	return mUseInvalidation;
}

void SceneNode::setUseGlobalCursors( const bool& use ) {
	mUseGlobalCursors = use;
}

const bool& SceneNode::getUseGlobalCursors() {
	return mUseGlobalCursors;
}

void SceneNode::setCursor( EE_CURSOR_TYPE cursor ) {
	if ( mUseGlobalCursors ) {
		mWindow->getCursorManager()->set( cursor );
	}
}

bool SceneNode::isDrawInvalidator() {
	return NULL != mFrameBuffer;
}

void SceneNode::preDraw() {
}

void SceneNode::postDraw() {
}

}}
