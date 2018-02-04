#include <eepp/scene/scenenode.hpp>
#include <eepp/window/window.hpp>
#include <eepp/window/engine.hpp>
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
	mFrameBufferBound( false ),
	mUseInvalidation( false )
{
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}
}

SceneNode::~SceneNode() {
	onClose();

	eeSAFE_DELETE( mFrameBuffer );
}

void SceneNode::enableFrameBuffer() {
	if ( NULL == mFrameBuffer )
		createFrameBuffer();
}

void SceneNode::disableFrameBuffer() {
	eeSAFE_DELETE( mFrameBuffer );
}

void SceneNode::draw() {
	GlobalBatchRenderer::instance()->draw();

	const View& prevView = mWindow->getView();

	mWindow->setView( mWindow->getDefaultView() );

	if ( mVisible ) {
		if ( mNodeFlags & NODE_FLAG_POSITION_DIRTY )
			updateScreenPos();

		matrixSet();

		clipStart();

		draw();

		drawChilds();

		clipEnd();

		matrixUnset();
	}

	mWindow->setView( prevView );

	GlobalBatchRenderer::instance()->draw();
}

void SceneNode::update( const Time& elapsed ) {

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
	eeSAFE_DELETE( mFrameBuffer );
	Sizei fboSize( getFrameBufferSize() );
	if ( fboSize.getWidth() < 1 ) fboSize.setWidth(1);
	if ( fboSize.getHeight() < 1 ) fboSize.setHeight(1);
	mFrameBuffer = FrameBuffer::New( fboSize.getWidth(), fboSize.getHeight(), true, false, true );
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

bool SceneNode::invalidated() {
	return 0 != ( mNodeFlags & NODE_FLAG_VIEW_DIRTY );
}

void SceneNode::enableDrawInvalidation() {
	mUseInvalidation = true;
}

void SceneNode::disableDrawInvalidation() {
	mUseInvalidation = false;
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

}}
