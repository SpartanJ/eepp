#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>

namespace EE { namespace Scene {

Node* Node::New() {
	return eeNew( Node, () );
}

Node::Node() :
	mIdHash( 0 ),
	mSize( 0, 0 ),
	mData( 0 ),
	mParentNode( NULL ),
	mSceneNode( NULL ),
	mNodeDrawInvalidator( NULL ),
	mChild( NULL ),
	mChildLast( NULL ),
	mNext( NULL ),
	mPrev( NULL ),
	mNodeFlags( NODE_FLAG_POSITION_DIRTY | NODE_FLAG_POLYGON_DIRTY ),
	mBlend( BlendMode::Alpha() ),
	mNumCallBacks( 0 ),
	mVisible( true ),
	mEnabled( true ),
	mAlpha( 255.f ) {}

Node::~Node() {
	if ( !SceneManager::instance()->isShuttingDown() && NULL != mSceneNode ) {
		if ( mSceneNode != this && NULL != mSceneNode->getActionManager() )
			mSceneNode->getActionManager()->removeAllActionsFromTarget( this );

		if ( mNodeFlags & NODE_FLAG_SCHEDULED_UPDATE )
			mSceneNode->unsubscribeScheduledUpdate( this );

		if ( isMouseOverMeOrChilds() )
			mSceneNode->removeMouseOverNode( this );
	}

	childDeleteAll();

	if ( NULL != mParentNode )
		mParentNode->childRemove( this );

	EventDispatcher* eventDispatcher = getEventDispatcher();

	if ( NULL != eventDispatcher ) {
		if ( eventDispatcher->getFocusNode() == this && mSceneNode != this ) {
			eventDispatcher->setFocusNode( mSceneNode );
		}

		if ( eventDispatcher->getLastFocusNode() == this && mSceneNode != this ) {
			eventDispatcher->setLastFocusNode( mSceneNode );
		}

		if ( eventDispatcher->getMouseOverNode() == this && mSceneNode != this ) {
			eventDispatcher->setMouseOverNode( mSceneNode );
		}

		if ( eventDispatcher->getMouseDownNode() == this ) {
			eventDispatcher->resetMouseDownNode();
		}
	}
}

void Node::worldToNodeTranslation( Vector2f& Pos ) const {
	Node* ParentLoop = mParentNode;

	Pos -= mPosition;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->getPosition();

		Pos -= ParentPos;

		ParentLoop = ParentLoop->getParent();
	}
}

void Node::nodeToWorldTranslation( Vector2f& Pos ) const {
	Node* ParentLoop = mParentNode;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->getPosition();

		Pos += ParentPos;

		ParentLoop = ParentLoop->getParent();
	}
}

Uint32 Node::getType() const {
	return 0;
}

bool Node::isType( const Uint32& type ) const {
	return Node::getType() == type;
}

void Node::messagePost( const NodeMessage* Msg ) {
	Node* node = this;
	while ( NULL != node ) {
		if ( node->onMessage( Msg ) )
			break;
		node = node->getParent();
	}
}

Uint32 Node::onMessage( const NodeMessage* ) {
	return 0;
}

void Node::setInternalPosition( const Vector2f& Pos ) {
	Transformable::setPosition( Vector2f( Pos.x, Pos.y ) );
	setDirty();
}

void Node::setPosition( const Vector2f& Pos ) {
	if ( Pos != getPosition() ) {
		setInternalPosition( Pos );
		onPositionChange();
	}
}

Node* Node::setPosition( const Float& x, const Float& y ) {
	setPosition( Vector2f( x, y ) );
	return this;
}

void Node::setInternalSize( const Sizef& size ) {
	mSize = size;
	mNodeFlags |= NODE_FLAG_POLYGON_DIRTY;
	updateCenter();
	sendCommonEvent( Event::OnSizeChange );
	invalidateDraw();
}

void Node::scheduledUpdate( const Time& ) {}

Node* Node::setSize( const Sizef& Size ) {
	if ( Size != mSize ) {
		setInternalSize( Size );

		onSizeChange();
	}

	return this;
}

Node* Node::setSize( const Float& Width, const Float& Height ) {
	return setSize( Vector2f( Width, Height ) );
}

void Node::setInternalWidth( const Float& width ) {
	setInternalSize( Sizef( width, getSize().getHeight() ) );
}

void Node::setInternalHeight( const Float& height ) {
	setInternalSize( Sizef( getSize().getWidth(), height ) );
}

const Sizef& Node::getSize() const {
	return mSize;
}

const Sizef& Node::getPixelsSize() const {
	return mSize;
}

Node* Node::setVisible( const bool& visible ) {
	if ( mVisible != visible ) {
		mVisible = visible;
		onVisibilityChange();
	}
	return this;
}

bool Node::isVisible() const {
	return mVisible;
}

bool Node::isHided() const {
	return !mVisible;
}

Node* Node::setEnabled( const bool& enabled ) {
	if ( mEnabled != enabled ) {
		mEnabled = enabled;
		onEnabledChange();
	}
	return this;
}

bool Node::isEnabled() const {
	return mEnabled;
}

bool Node::isDisabled() const {
	return !mEnabled;
}

Node* Node::getParent() const {
	return mParentNode;
}

void Node::updateDrawInvalidator( bool force ) {
	mNodeDrawInvalidator = getDrawInvalidator();

	if ( NULL != mChild && ( mChild->mNodeDrawInvalidator != mNodeDrawInvalidator || force ) ) {
		Node* child = mChild;
		while ( NULL != child ) {
			child->updateDrawInvalidator();
			child = child->mNext;
		}
	}
}

void Node::subscribeScheduledUpdate() {
	if ( NULL != mSceneNode ) {
		mSceneNode->subscribeScheduledUpdate( this );
		writeNodeFlag( NODE_FLAG_SCHEDULED_UPDATE, 1 );
	}
}

void Node::unsubscribeScheduledUpdate() {
	if ( NULL != mSceneNode ) {
		mSceneNode->unsubscribeScheduledUpdate( this );
		writeNodeFlag( NODE_FLAG_SCHEDULED_UPDATE, 0 );
	}
}

bool Node::isSubscribedForScheduledUpdate() {
	return 0 != ( mNodeFlags & NODE_FLAG_SCHEDULED_UPDATE );
}

Node* Node::setParent( Node* parent ) {
	eeASSERT( NULL != parent );

	if ( parent == mParentNode )
		return this;

	if ( NULL != mParentNode )
		mParentNode->childRemove( this );

	mParentNode = parent;

	updateDrawInvalidator();

	if ( NULL != mParentNode )
		mParentNode->childAdd( this );

	setDirty();

	if ( mSceneNode != findSceneNode() )
		onSceneChange();

	onParentChange();

	return this;
}

bool Node::isParentOf( const Node* node ) const {
	eeASSERT( NULL != node );
	Node* tParent = node->mParentNode;
	while ( NULL != tParent ) {
		if ( this == tParent )
			return true;
		tParent = tParent->mParentNode;
	}
	return false;
}

void Node::close() {
	mNodeFlags |= NODE_FLAG_CLOSE;

	if ( NULL != mSceneNode ) {
		mSceneNode->addToCloseQueue( this );
	}
}

void Node::draw() {}

void Node::update( const Time& time ) {
	Node* childLoop = mChild;

	while ( NULL != childLoop ) {
		childLoop->update( time );
		childLoop = childLoop->mNext;
	}

	writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 0 );
}

void Node::sendMouseEvent( const Uint32& event, const Vector2i& pos, const Uint32& flags ) {
	MouseEvent MouseEvent( this, event, pos, flags );
	sendEvent( &MouseEvent );
}

void Node::sendCommonEvent( const Uint32& event ) {
	Event CommonEvent( this, event );
	sendEvent( &CommonEvent );
}

void Node::sendTextEvent( const Uint32& event, const std::string& text ) {
	TextEvent tevent( this, event, text );
	sendEvent( &tevent );
}

Uint32 Node::onTextInput( const TextInputEvent& event ) {
	sendEvent( &event );
	return 0;
}

Uint32 Node::onTextEditing( const TextEditingEvent& event ) {
	sendEvent( &event );
	return 0;
}

Uint32 Node::onKeyDown( const KeyEvent& Event ) {
	sendEvent( &Event );
	return 0;
}

Uint32 Node::onKeyUp( const KeyEvent& Event ) {
	sendEvent( &Event );
	return 0;
}

Uint32 Node::onMouseMove( const Vector2i& Pos, const Uint32& Flags ) {
	sendMouseEvent( Event::MouseMove, Pos, Flags );
	return 1;
}

Uint32 Node::onMouseDown( const Vector2i& Pos, const Uint32& Flags ) {
	sendMouseEvent( Event::MouseDown, Pos, Flags );
	return 1;
}

Uint32 Node::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	sendMouseEvent( Event::MouseUp, Pos, Flags );
	return 1;
}

Uint32 Node::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	sendMouseEvent( Event::MouseClick, Pos, Flags );
	return 1;
}

bool Node::isMouseOver() const {
	return 0 != ( mNodeFlags & NODE_FLAG_MOUSEOVER );
}

bool Node::isMouseOverMeOrChilds() const {
	return 0 != ( mNodeFlags & NODE_FLAG_MOUSEOVER_ME_OR_CHILD );
}

Uint32 Node::onMouseDoubleClick( const Vector2i& Pos, const Uint32& Flags ) {
	sendMouseEvent( Event::MouseDoubleClick, Pos, Flags );
	return 1;
}

Uint32 Node::onMouseOver( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != mParentNode && mParentNode->isMouseOverMeOrChilds() )
		mParentNode->onMouseOver( Pos, Flags );

	writeNodeFlag( NODE_FLAG_MOUSEOVER, 1 );

	EventDispatcher* eventDispatcher = NULL != mSceneNode ? mSceneNode->getEventDispatcher() : NULL;

	if ( NULL != eventDispatcher && eventDispatcher->getMouseOverNode() == this )
		sendMouseEvent( Event::MouseEnter, Pos, Flags );

	sendMouseEvent( Event::MouseOver, Pos, Flags );

	return 1;
}

Uint32 Node::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != mParentNode && !mParentNode->isMouseOverMeOrChilds() )
		mParentNode->onMouseLeave( Pos, Flags );

	writeNodeFlag( NODE_FLAG_MOUSEOVER, 0 );

	EventDispatcher* eventDispatcher = NULL != mSceneNode ? mSceneNode->getEventDispatcher() : NULL;

	if ( NULL != eventDispatcher && eventDispatcher->getMouseOverNode() != this )
		sendMouseEvent( Event::MouseLeave, Pos, Flags );

	sendMouseEvent( Event::MouseOut, Pos, Flags );

	return 1;
}

Uint32 Node::onCalculateDrag( const Vector2f&, const Uint32& ) {
	return 1;
}

void Node::onClose() {
	sendCommonEvent( Event::OnClose );
}

Node* Node::getNextNode() const {
	return mNext;
}

Node* Node::getPrevNode() const {
	return mPrev;
}

Node* Node::getNextNodeLoop() const {
	if ( NULL == mNext )
		return getParent()->getFirstChild();
	else
		return mNext;
}

Node* Node::setData( const UintPtr& data ) {
	mData = data;
	return this;
}

const UintPtr& Node::getData() const {
	return mData;
}

Node* Node::setBlendMode( const BlendMode& blend ) {
	mBlend = blend;
	invalidateDraw();
	return this;
}

const BlendMode& Node::getBlendMode() const {
	return mBlend;
}

Node* Node::toFront() {
	if ( NULL != mParentNode && mParentNode->mChildLast != this ) {
		mParentNode->childRemove( this );
		mParentNode->childAdd( this );
	}
	return this;
}

Node* Node::toBack() {
	if ( NULL != mParentNode ) {
		mParentNode->childAddAt( this, 0 );
	}
	return this;
}

void Node::toPosition( const Uint32& Pos ) {
	if ( NULL != mParentNode ) {
		mParentNode->childAddAt( this, Pos );
	}
}

void Node::onVisibilityChange() {
	sendCommonEvent( Event::OnVisibleChange );
	invalidateDraw();
}

void Node::onEnabledChange() {
	EventDispatcher* eventDispatcher = NULL != mSceneNode ? mSceneNode->getEventDispatcher() : NULL;

	if ( !isEnabled() && NULL != eventDispatcher && NULL != eventDispatcher->getFocusNode() ) {
		if ( isChild( eventDispatcher->getFocusNode() ) ) {
			eventDispatcher->setFocusNode( NULL );
		}
	}

	sendCommonEvent( Event::OnEnabledChange );
	invalidateDraw();
}

void Node::onPositionChange() {
	sendCommonEvent( Event::OnPositionChange );
	invalidateDraw();
}

void Node::onSizeChange() {
	updateOriginPoint();

	invalidateDraw();
}

Rectf Node::getScreenBounds() {
	return Rectf( Vector2f( mScreenPosi.x, mScreenPosi.y ),
				  Sizef( (Float)(int)mSize.getWidth(), (Float)(int)mSize.getHeight() ) );
}

Rectf Node::getLocalBounds() const {
	return Rectf( 0, 0, mSize.getWidth(), mSize.getHeight() );
}

const Uint32& Node::getNodeFlags() const {
	return mNodeFlags;
}

void Node::setNodeFlags( const Uint32& flags ) {
	mNodeFlags = flags;
}

void Node::drawChilds() {
	if ( isReverseDraw() ) {
		Node* child = mChildLast;

		while ( NULL != child ) {
			if ( child->mVisible ) {
				child->nodeDraw();
			}

			child = child->mPrev;
		}
	} else {
		Node* child = mChild;

		while ( NULL != child ) {
			if ( child->mVisible ) {
				child->nodeDraw();
			}

			child = child->mNext;
		}
	}
}

void Node::nodeDraw() {
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
}

Uint32 Node::forceKeyDown( const KeyEvent& event ) {
	return onKeyDown( event );
}

Uint32 Node::foceKeyUp( const KeyEvent& event ) {
	return onKeyUp( event );
}

Uint32 Node::forceTextInput( const TextInputEvent& event ) {
	return onTextInput( event );
}

const Vector2f& Node::getScreenPos() const {
	return mScreenPos;
}

Rectf Node::getScreenRect() const {
	return Rectf( getScreenPos(), getPixelsSize() );
}

void Node::clipStart() {
	if ( mVisible && isClipped() ) {
		clipSmartEnable( mScreenPos.x, mScreenPos.y, mSize.getWidth(), mSize.getHeight() );
	}
}

void Node::clipEnd() {
	if ( mVisible && isClipped() ) {
		clipSmartDisable();
	}
}

void Node::matrixSet() {
	if ( getScale() != 1.f || getRotation() != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->pushMatrix();

		Vector2f scaleCenter = getScaleCenter();
		GLi->translatef( scaleCenter.x, scaleCenter.y, 0.f );
		GLi->scalef( getScale().x, getScale().y, 1.0f );
		GLi->translatef( -scaleCenter.x, -scaleCenter.y, 0.f );

		Vector2f rotationCenter = getRotationCenter();
		GLi->translatef( rotationCenter.x, rotationCenter.y, 0.f );
		GLi->rotatef( getRotation(), 0.0f, 0.0f, 1.0f );
		GLi->translatef( -rotationCenter.x, -rotationCenter.y, 0.f );
	}
}

void Node::matrixUnset() {
	if ( getScale() != 1.f || getRotation() != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->popMatrix();
	}
}

void Node::childDeleteAll() {
	while ( NULL != mChild ) {
		eeDelete( mChild );
	}
}

void Node::childAdd( Node* node ) {
	if ( NULL == mChild ) {
		mChild = node;
		mChildLast = node;
		mChild->mPrev = NULL;
		mChild->mNext = NULL;
	} else {
		mChildLast->mNext = node;
		node->mPrev = mChildLast;
		node->mNext = NULL;
		mChildLast = node;
	}

	eeASSERT( !( NULL == mChildLast && NULL != mChild ) );

	onChildCountChange( node, false );
}

void Node::childAddAt( Node* node, Uint32 index ) {
	eeASSERT( NULL != node );

	Node* nodeLoop = mChild;

	node->setParent( this );

	childRemove( node );

	node->mParentNode = this;
	node->mSceneNode = node->findSceneNode();

	if ( nodeLoop == NULL ) {
		mChild = node;
		mChildLast = node;
		node->mNext = NULL;
		node->mPrev = NULL;
	} else {
		if ( index == 0 ) {
			if ( NULL != mChild ) {
				mChild->mPrev = node;
			}

			node->mNext = mChild;
			node->mPrev = NULL;
			mChild = node;

			if ( mChild->mNext == NULL )
				mChildLast = mChild;
		} else {
			Uint32 i = 0;

			while ( NULL != nodeLoop->mNext && ++i < index ) {
				nodeLoop = nodeLoop->mNext;
			}

			Node* ChildTmp = nodeLoop->mNext;
			nodeLoop->mNext = node;
			node->mPrev = nodeLoop;
			node->mNext = ChildTmp;

			if ( NULL != ChildTmp ) {
				ChildTmp->mPrev = node;
			} else {
				mChildLast = node;
			}
		}
	}

	eeASSERT( !( NULL == mChildLast && NULL != mChild ) );

	onChildCountChange( node, false );
}

void Node::childRemove( Node* node ) {
	if ( node == mChild ) {
		mChild = mChild->mNext;

		if ( NULL != mChild ) {
			mChild->mPrev = NULL;

			if ( node == mChildLast )
				mChildLast = mChild;
		} else {
			mChildLast = NULL;
		}
	} else {
		if ( mChildLast == node )
			mChildLast = mChildLast->mPrev;

		if ( node->mPrev )
			node->mPrev->mNext = node->mNext;

		if ( NULL != node->mNext ) {
			node->mNext->mPrev = node->mPrev;
			node->mNext = NULL;
		}

		node->mPrev = NULL;
	}

	eeASSERT( !( NULL == mChildLast && NULL != mChild ) );

	onChildCountChange( node, true );
}

void Node::childsCloseAll() {
	Node* childLoop = mChild;
	writeNodeFlag( NODE_FLAG_CLOSING_CHILDREN, 1 );
	while ( NULL != childLoop ) {
		childLoop->close();
		childLoop = childLoop->mNext;
	}
	writeNodeFlag( NODE_FLAG_CLOSING_CHILDREN, 0 );
}

const std::string& Node::getId() const {
	return mId;
}

Node* Node::setId( const std::string& id ) {
	mId = id;
	mIdHash = String::hash( id );
	onIdChange();
	return this;
}

void Node::onIdChange() {
	sendCommonEvent( Event::OnIdChange );
}

bool Node::isClosing() const {
	return 0 != ( mNodeFlags & NODE_FLAG_CLOSE );
}

bool Node::isClosingChildren() const {
	return 0 != ( mNodeFlags & NODE_FLAG_CLOSING_CHILDREN );
}

const String::HashType& Node::getIdHash() const {
	return mIdHash;
}

Node* Node::findIdHash( const String::HashType& idHash ) const {
	if ( !isClosing() && mIdHash == idHash ) {
		return const_cast<Node*>( this );
	} else {
		Node* child = mChild;

		while ( NULL != child ) {
			Node* foundNode = child->findIdHash( idHash );

			if ( NULL != foundNode )
				return foundNode;

			child = child->mNext;
		}
	}

	return NULL;
}

Node* Node::find( const std::string& id ) const {
	return findIdHash( String::hash( id ) );
}

Node* Node::hasChildHash( const String::HashType& idHash ) const {
	Node* child = mChild;
	while ( NULL != child ) {
		if ( child->getIdHash() == idHash )
			return child;
		child = child->mNext;
	}
	return nullptr;
}

Node* Node::hasChild( const std::string& id ) const {
	return hasChildHash( String::hash( id ) );
}

Node* Node::findByType( const Uint32& type ) const {
	if ( !isClosing() && isType( type ) ) {
		return const_cast<Node*>( this );
	} else {
		Node* child = mChild;
		while ( NULL != child ) {
			Node* foundNode = child->findByType( type );
			if ( NULL != foundNode )
				return foundNode;
			child = child->mNext;
		}
	}

	return NULL;
}

bool Node::inNodeTree( Node* node ) const {
	if ( this == node ) {
		return true;
	} else {
		Node* child = mChild;
		while ( NULL != child ) {
			if ( child->inNodeTree( node ) )
				return true;
			child = child->mNext;
		}
	}

	return false;
}

bool Node::isChild( Node* child ) const {
	Node* childLoop = mChild;

	while ( NULL != childLoop ) {
		if ( child == childLoop )
			return true;

		childLoop = childLoop->mNext;
	}

	return false;
}

bool Node::inParentTreeOf( Node* child ) const {
	Node* node = child->mParentNode;
	while ( NULL != node ) {
		if ( node == this )
			return true;
		node = node->mParentNode;
	}
	return false;
}

void Node::setLoadingState( bool loading ) {
	writeNodeFlag( NODE_FLAG_LOADING, loading ? 1 : 0 );
}

bool Node::isLoadingState() const {
	return 0 != ( mNodeFlags & NODE_FLAG_LOADING );
}

Uint32 Node::getChildCount() const {
	Node* child = mChild;
	Uint32 count = 0;
	while ( NULL != child ) {
		child = child->mNext;
		count++;
	}
	return count;
}

Uint32 Node::getChildOfTypeCount( const Uint32& type ) const {
	Node* childLoop = mChild;
	Uint32 count = 0;

	while ( NULL != childLoop ) {
		if ( childLoop->getType() == type ) {
			count++;
		}
		childLoop = childLoop->mNext;
	}

	return count;
}

Node* Node::getChildAt( Uint32 index ) const {
	Node* child = mChild;

	while ( NULL != child && index ) {
		child = child->mNext;
		index--;
	}

	return child;
}

Uint32 Node::getNodeIndex() const {
	Uint32 nodeIndex = 0;
	if ( NULL != mParentNode ) {
		Node* parentChild = mParentNode->mChild;
		while ( parentChild != NULL ) {
			if ( parentChild == this )
				return nodeIndex;
			parentChild = parentChild->mNext;
			nodeIndex++;
		};
	}
	return nodeIndex;
}

Uint32 Node::getNodeOfTypeIndex() const {
	Uint32 nodeIndex = 0;
	Uint32 type = getType();
	if ( NULL != mParentNode ) {
		Node* parentChild = mParentNode->mChild;
		while ( parentChild != NULL ) {
			if ( parentChild == this )
				return nodeIndex;
			if ( parentChild->getType() == type )
				nodeIndex++;
			parentChild = parentChild->mNext;
		};
	}
	return nodeIndex;
}

Node* Node::getFirstChild() const {
	return mChild;
}

Node* Node::getLastChild() const {
	return mChildLast;
}

Node* Node::overFind( const Vector2f& point ) {
	Node* pOver = NULL;

	if ( ( mNodeFlags & NODE_FLAG_OVER_FIND_ALLOWED ) && mEnabled && mVisible ) {
		updateWorldPolygon();

		if ( mWorldBounds.contains( point ) && mPoly.pointInside( point ) ) {
			writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );
			mSceneNode->addMouseOverNode( this );

			Node* child = mChildLast;

			while ( NULL != child ) {
				Node* childOver = child->overFind( point );

				if ( NULL != childOver ) {
					pOver = childOver;

					break; // Search from top to bottom, so the first over will be the topmost
				}

				child = child->mPrev;
			}

			if ( NULL == pOver )
				pOver = this;
		}
	}

	return pOver;
}

void Node::detach() {
	if ( mParentNode ) {
		mParentNode->childRemove( this );
		mParentNode = NULL;
	}
}

void Node::forEachNode( std::function<void( Node* )> func ) {
	func( this );
	Node* node = mChild;
	while ( node ) {
		node->forEachNode( func );
		node = node->getNextNode();
	}
}

void Node::forEachChild( std::function<void( Node* )> func ) {
	Node* node = mChild;
	while ( node ) {
		func( node );
		node = node->getNextNode();
	}
}

void Node::onSceneChange() {
	mSceneNode = findSceneNode();
	eeASSERT( !mSceneNode->removeFromCloseQueue( this ) );

	Node* child = mChild;

	while ( NULL != child ) {
		child->onSceneChange();
		child = child->mNext;
	}
}

bool Node::isWidget() const {
	return 0 != ( mNodeFlags & NODE_FLAG_WIDGET );
}

bool Node::isWindow() const {
	return 0 != ( mNodeFlags & NODE_FLAG_WINDOW );
}

bool Node::isLayout() const {
	return 0 != ( mNodeFlags & NODE_FLAG_LAYOUT );
}

bool Node::isClipped() const {
	return 0 != ( mNodeFlags & NODE_FLAG_CLIP_ENABLE );
}

bool Node::isRotated() const {
	return 0 != ( mNodeFlags & NODE_FLAG_ROTATED );
}

bool Node::isScaled() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SCALED );
}

bool Node::isFrameBuffer() const {
	return 0 != ( mNodeFlags & NODE_FLAG_FRAME_BUFFER );
}

bool Node::isSceneNode() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SCENENODE );
}

bool Node::isUISceneNode() const {
	return 0 != ( mNodeFlags & NODE_FLAG_UISCENENODE );
}

bool Node::isUINode() const {
	return 0 != ( mNodeFlags & NODE_FLAG_UINODE );
}

bool Node::isMeOrParentTreeVisible() const {
	const Node* node = this;
	while ( NULL != node ) {
		if ( !node->isVisible() )
			return false;
		node = node->getParent();
	}
	return true;
}

bool Node::isMeOrParentTreeRotated() const {
	const Node* node = this;
	while ( NULL != node ) {
		if ( node->isRotated() )
			return true;
		node = node->getParent();
	}
	return false;
}

bool Node::isMeOrParentTreeScaled() const {
	const Node* node = this;
	while ( NULL != node ) {
		if ( node->isScaled() )
			return true;
		node = node->getParent();
	}
	return false;
}

bool Node::isMeOrParentTreeScaledOrRotated() const {
	const Node* node = this;
	while ( NULL != node ) {
		if ( node->isScaled() || node->isRotated() )
			return true;
		node = node->getParent();
	}
	return false;
}

bool Node::isMeOrParentTreeScaledOrRotatedOrFrameBuffer() const {
	const Node* node = this;
	while ( NULL != node ) {
		if ( node->isScaled() || node->isRotated() || node->isFrameBuffer() )
			return true;
		node = node->getParent();
	}
	return false;
}

const Polygon2f& Node::getWorldPolygon() {
	if ( mNodeFlags & NODE_FLAG_POLYGON_DIRTY )
		updateWorldPolygon();

	return mPoly;
}

const Rectf& Node::getWorldBounds() {
	if ( mNodeFlags & NODE_FLAG_POLYGON_DIRTY )
		updateWorldPolygon();

	return mWorldBounds;
}

void Node::updateWorldPolygon() {
	if ( !( mNodeFlags & NODE_FLAG_POLYGON_DIRTY ) )
		return;

	if ( mNodeFlags & NODE_FLAG_POSITION_DIRTY )
		updateScreenPos();

	mPoly = Polygon2f( Rectf( mScreenPos.x, mScreenPos.y, mScreenPos.x + mSize.getWidth(),
							  mScreenPos.y + mSize.getHeight() ) );

	mPoly.rotate( getRotation(), getRotationCenter() );
	mPoly.scale( getScale(), getScaleCenter() );

	Node* tParent = getParent();

	while ( tParent ) {
		mPoly.rotate( tParent->getRotation(), tParent->getRotationCenter() );
		mPoly.scale( tParent->getScale(), tParent->getScaleCenter() );

		tParent = tParent->getParent();
	};

	mWorldBounds = mPoly.getBounds();

	mNodeFlags &= ~NODE_FLAG_POLYGON_DIRTY;
}

void Node::updateCenter() {
	mCenter = Vector2f( mScreenPos.x + (Float)mSize.getWidth() * 0.5f,
						mScreenPos.y + (Float)mSize.getHeight() * 0.5f );
}

Uint32 Node::addEventListener( const Uint32& eventType, const EventCallback& callback ) {
	mEvents[eventType][++mNumCallBacks] = callback;
	return mNumCallBacks;
}

Uint32 Node::on( const Uint32& eventType, const EventCallback& callback ) {
	mEvents[eventType][++mNumCallBacks] = callback;
	return mNumCallBacks;
}

Uint32 Node::onClick( const std::function<void( const MouseEvent* )>& callback,
					  const MouseButton& button ) {
	return on( Event::MouseClick, [callback, button]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & ( EE_BUTTON_MASK( button ) ) ) {
			callback( event->asMouseEvent() );
		}
	} );
}

bool Node::hasEventsOfType( const Uint32& eventType ) const {
	return mEvents.find( eventType ) != mEvents.end();
}

void Node::removeEventsOfType( const Uint32& eventType ) {
	auto it = mEvents.find( eventType );
	if ( it != mEvents.end() )
		mEvents.erase( it );
}

void Node::removeEventListener( const Uint32& callbackId ) {
	EventsMap::iterator it;
	for ( it = mEvents.begin(); it != mEvents.end(); ++it ) {
		auto& event = it->second;
		if ( event.erase( callbackId ) > 0 )
			break;
	}
}

void Node::removeEventListener( const std::vector<Uint32>& callbacksIds ) {
	for ( auto& event : mEvents ) {
		auto& events = event.second;
		for ( auto& cbId : callbacksIds ) {
			auto it = events.find( cbId );
			if ( it != events.end() ) {
				events.erase( it );
			}
		}
	}
}

void Node::clearEventListener() {
	mEvents.clear();
}

void Node::sendEvent( const Event* event ) {
	if ( 0 != mEvents.count( event->getType() ) ) {
		std::map<Uint32, EventCallback> eventMap = mEvents[event->getType()];
		if ( eventMap.begin() != eventMap.end() ) {
			std::map<Uint32, EventCallback>::iterator it;
			for ( it = eventMap.begin(); it != eventMap.end(); ++it ) {
				const_cast<Event*>( event )->mCallbackId = it->first;
				it->second( event );
			}
		}
	}
}

void Node::onParentChange() {
	invalidateDraw();
}

void Node::updateScreenPos() {
	if ( !( mNodeFlags & NODE_FLAG_POSITION_DIRTY ) )
		return;

	Vector2f Pos( mPosition );

	nodeToWorldTranslation( Pos );

	mScreenPos = Pos;
	mScreenPosi = Vector2i( Pos.x, Pos.y );

	updateCenter();

	mNodeFlags &= ~NODE_FLAG_POSITION_DIRTY;

	sendCommonEvent( Event::OnUpdateScreenPosition );
}

void Node::writeNodeFlag( const Uint32& Flag, const Uint32& Val ) {
	BitOp::setBitFlagValue( &mNodeFlags, Flag, Val );
}

void Node::onParentSizeChange( const Vector2f& ) {
	sendCommonEvent( Event::OnParentSizeChange );
	invalidateDraw();
}

void Node::onChildCountChange( Node*, const bool& ) {
	sendCommonEvent( Event::OnChildCountChanged );
	invalidateDraw();
}

void Node::worldToNode( Vector2i& pos ) const {
	Vector2f toPos( convertToNodeSpace( Vector2f( pos.x, pos.y ) ) );
	pos = Vector2i( toPos.x, toPos.y );
}

void Node::nodeToWorld( Vector2i& pos ) const {
	Vector2f toPos( convertToWorldSpace( Vector2f( pos.x, pos.y ) ) );
	pos = Vector2i( toPos.x, toPos.y );
}

void Node::worldToNode( Vector2f& pos ) const {
	Vector2f toPos( convertToNodeSpace( pos ) );
	pos = Vector2f( toPos.x, toPos.y );
}

void Node::nodeToWorld( Vector2f& pos ) const {
	Vector2f toPos( convertToWorldSpace( Vector2f( pos.x, pos.y ) ) );
	pos = Vector2f( toPos.x, toPos.y );
}

bool Node::isReverseDraw() const {
	return 0 != ( mNodeFlags & NODE_FLAG_REVERSE_DRAW );
}

void Node::setReverseDraw( bool reverseDraw ) {
	writeNodeFlag( NODE_FLAG_REVERSE_DRAW, reverseDraw ? 1 : 0 );
	invalidateDraw();
}

void Node::invalidateDraw() {
	if ( NULL != mNodeDrawInvalidator ) {
		mNodeDrawInvalidator->invalidate( this );
	}
}

SceneNode* Node::getSceneNode() const {
	return mSceneNode;
}

SceneNode* Node::findSceneNode() {
	Node* node = mParentNode;
	while ( node != NULL ) {
		if ( node->isSceneNode() )
			return static_cast<SceneNode*>( node );
		node = node->getParent();
	}
	return isSceneNode() ? reinterpret_cast<SceneNode*>( this ) : NULL;
}

EventDispatcher* Node::getEventDispatcher() const {
	return NULL != mSceneNode ? mSceneNode->getEventDispatcher() : NULL;
}

void Node::updateOriginPoint() {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			mRotationOriginPoint.x = mSize.x * 0.5f;
			mRotationOriginPoint.y = mSize.y * 0.5f;
			break;
		case OriginPoint::OriginTopLeft:
			mRotationOriginPoint.x = mRotationOriginPoint.y = 0;
			break;
		default: {
		}
	}

	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			mScaleOriginPoint.x = mSize.x * 0.5f;
			mScaleOriginPoint.y = mSize.y * 0.5f;
			break;
		case OriginPoint::OriginTopLeft:
			mScaleOriginPoint.x = mScaleOriginPoint.y = 0;
			break;
		default: {
		}
	}

	setDirty();
}

void Node::setDirty() {
	if ( ( mNodeFlags & NODE_FLAG_POSITION_DIRTY ) && ( mNodeFlags & NODE_FLAG_POLYGON_DIRTY ) )
		return;

	mNodeFlags |= NODE_FLAG_POSITION_DIRTY | NODE_FLAG_POLYGON_DIRTY;

	setChildsDirty();
}

void Node::setChildsDirty() {
	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->setDirty();

		ChildLoop = ChildLoop->mNext;
	}
}

void Node::onAngleChange() {
	sendCommonEvent( Event::OnAngleChange );
	invalidateDraw();
}

void Node::onScaleChange() {
	sendCommonEvent( Event::OnScaleChange );
	invalidateDraw();
}

void Node::onAlphaChange() {
	sendCommonEvent( Event::OnAlphaChange );
	invalidateDraw();
}

Color Node::getColor( const Color& Col ) {
	return Color( Col.r, Col.g, Col.b, static_cast<Uint8>( (Float)Col.a * ( mAlpha / 255.f ) ) );
}

const OriginPoint& Node::getRotationOriginPoint() const {
	return mRotationOriginPoint;
}

void Node::setRotationOriginPoint( const OriginPoint& center ) {
	mRotationOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
	Transformable::setRotationOrigin( getRotationOriginPoint().x, getRotationOriginPoint().y );
}

void Node::setRotationOriginPointX( const std::string& xEq ) {
	mRotationOriginPoint.setXEq( xEq );
	updateOriginPoint();
	Transformable::setRotationOrigin( getRotationOriginPoint().x, getRotationOriginPoint().y );
}

void Node::setRotationOriginPointY( const std::string& yEq ) {
	mRotationOriginPoint.setYEq( yEq );
	updateOriginPoint();
	Transformable::setRotationOrigin( getRotationOriginPoint().x, getRotationOriginPoint().y );
}

Vector2f Node::getRotationCenter() const {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			return mCenter;
		case OriginPoint::OriginTopLeft:
			return mScreenPos;
		case OriginPoint::OriginCustom:
		default:
			return mScreenPos + mRotationOriginPoint;
	}
}

void Node::setRotation( float angle ) {
	Transformable::setRotation( angle );

	updateOriginPoint();
	Transformable::setRotationOrigin( getRotationOriginPoint().x, getRotationOriginPoint().y );

	if ( getRotation() != 0.f ) {
		mNodeFlags |= NODE_FLAG_ROTATED;
	} else {
		if ( mNodeFlags & NODE_FLAG_ROTATED )
			mNodeFlags &= ~NODE_FLAG_ROTATED;
	}

	setDirty();

	onAngleChange();
}

void Node::setRotation( const Float& angle, const OriginPoint& center ) {
	mRotationOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
	setRotation( angle );
}

void Node::setScale( const Vector2f& scale ) {
	Transformable::setScale( scale.x, scale.y );

	updateOriginPoint();
	Transformable::setScaleOrigin( getScaleOriginPoint().x, getScaleOriginPoint().y );

	if ( getScale() != 1.f ) {
		mNodeFlags |= NODE_FLAG_SCALED;
	} else {
		if ( mNodeFlags & NODE_FLAG_SCALED )
			mNodeFlags &= ~NODE_FLAG_SCALED;
	}

	setDirty();

	onScaleChange();
}

const OriginPoint& Node::getScaleOriginPoint() const {
	return mScaleOriginPoint;
}

void Node::setScaleOriginPoint( const OriginPoint& center ) {
	mScaleOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
	Transformable::setScaleOrigin( getScaleCenter().x, getScaleCenter().y );
}

void Node::setScaleOriginPointX( const std::string& xEq ) {
	mScaleOriginPoint.setXEq( xEq );
	updateOriginPoint();
	Transformable::setScaleOrigin( getScaleCenter().x, getScaleCenter().y );
}

void Node::setScaleOriginPointY( const std::string& yEq ) {
	mScaleOriginPoint.setYEq( yEq );
	updateOriginPoint();
	Transformable::setScaleOrigin( getScaleCenter().x, getScaleCenter().y );
}

Vector2f Node::getScaleCenter() const {
	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			return mCenter;
		case OriginPoint::OriginTopLeft:
			return mScreenPos;
		case OriginPoint::OriginCustom:
		default:
			return mScreenPos + mScaleOriginPoint;
	}
}

void Node::setScale( const Vector2f& scale, const OriginPoint& center ) {
	mScaleOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
	Transformable::setScaleOrigin( getScaleOriginPoint().x, getScaleOriginPoint().y );
	setScale( Vector2f( scale.x, scale.y ) );
}

void Node::setScale( const Float& scale, const OriginPoint& center ) {
	setScale( Vector2f( scale, scale ), center );
}

const Float& Node::getAlpha() const {
	return mAlpha;
}

void Node::setAlpha( const Float& alpha ) {
	if ( mAlpha != alpha ) {
		mAlpha = alpha;
		invalidateDraw();
		onAlphaChange();
	}
}

void Node::setChildsAlpha( const Float& alpha ) {
	Node* child = mChild;
	while ( NULL != child ) {
		child->setAlpha( alpha );
		child->setChildsAlpha( alpha );
		child = child->getNextNode();
	}
}

ActionManager* Node::getActionManager() const {
	return mSceneNode->getActionManager();
}

Node* Node::runAction( Action* action ) {
	if ( NULL != action ) {
		action->setTarget( this );

		action->start();

		getActionManager()->addAction( action );
	}

	return this;
}

bool Node::removeAction( Action* action ) {
	return getActionManager()->removeAction( action );
}

bool Node::removeActions( const std::vector<Action*>& actions ) {
	return getActionManager()->removeActions( actions );
}

bool Node::removeActionsByTag( const String::HashType& tag ) {
	return getActionManager()->removeActionsByTagFromTarget( this, tag );
}

std::vector<Action*> Node::getActions() {
	return getActionManager()->getActionsFromTarget( this );
}

std::vector<Action*> Node::getActionsByTag( const Uint32& tag ) {
	return getActionManager()->getActionsByTagFromTarget( this, tag );
}

void Node::clearActions() {
	getActionManager()->removeAllActionsFromTarget( this );
}

void Node::runOnMainThread( Actions::Runnable::RunnableFunc runnable, const Time& delay,
							const Uint32& uniqueIdentifier ) {
	Action* action = Actions::Runnable::New( std::move( runnable ), delay );
	action->setTag( uniqueIdentifier );
	runAction( action );
}

void Node::setTimeout( Actions::Runnable::RunnableFunc runnable, const Time& delay,
					   const Uint32& uniqueIdentifier ) {
	Action* action = Actions::Runnable::New( std::move( runnable ), delay );
	action->setTag( uniqueIdentifier );
	runAction( action );
}

void Node::debounce( Actions::Runnable::RunnableFunc runnable, const Time& delay,
					 const Uint32& uniqueIdentifier ) {
	removeActionsByTag( uniqueIdentifier );
	setTimeout( std::move( runnable ), std::move( delay ), uniqueIdentifier );
}

void Node::setInterval( Actions::Runnable::RunnableFunc runnable, const Time& interval,
						const Uint32& uniqueIdentifier ) {
	Action* action = Actions::Runnable::New( std::move( runnable ), interval, true );
	action->setTag( uniqueIdentifier );
	runAction( action );
}

Transform Node::getLocalTransform() const {
	return getTransform();
}

Transform Node::getGlobalTransform() const {
	return NULL != mParentNode ? mParentNode->getGlobalTransform() * getTransform()
							   : getTransform();
}

Transform Node::getNodeToWorldTransform() const {
	return getGlobalTransform();
}

Transform Node::getWorldToNodeTransform() const {
	return getNodeToWorldTransform().getInverse();
}

Vector2f Node::convertToNodeSpace( const Vector2f& worldPoint ) const {
	return getWorldToNodeTransform().transformPoint( worldPoint.x, worldPoint.y );
}

Vector2f Node::convertToWorldSpace( const Vector2f& nodePoint ) const {
	return getNodeToWorldTransform().transformPoint( nodePoint.x, nodePoint.y );
}

void Node::setScale( float factorX, float factorY ) {
	setScale( Vector2f( factorX, factorY ) );
}

void Node::setScaleOrigin( float x, float y ) {
	setScaleOriginPoint( OriginPoint( x, y ) );
}

void Node::setRotationOrigin( float x, float y ) {
	setRotationOriginPoint( OriginPoint( x, y ) );
}

Uint32 Node::onFocus() {
	mNodeFlags |= NODE_FLAG_HAS_FOCUS;

	sendCommonEvent( Event::OnFocus );

	invalidateDraw();

	return 1;
}

Uint32 Node::onFocusLoss() {
	mNodeFlags &= ~NODE_FLAG_HAS_FOCUS;

	sendCommonEvent( Event::OnFocusLoss );

	invalidateDraw();

	return 1;
}

bool Node::hasFocus() const {
	return 0 != ( mNodeFlags & NODE_FLAG_HAS_FOCUS );
}

bool Node::hasFocusWithin() const {
	return hasFocus() || inParentTreeOf( getEventDispatcher()->getFocusNode() );
}

void Node::setFocus() {}

Node* Node::getFirstWidget() const {
	Node* child = mChild;
	while ( NULL != child ) {
		if ( child->isWidget() ) {
			return child;
		}
		child = child->getNextNode();
	}
	return NULL;
}

Node* Node::getParentWidget() const {
	Node* parentNode = mParentNode;

	while ( NULL != parentNode ) {
		if ( parentNode->isWidget() ) {
			return parentNode;
		}

		parentNode = parentNode->getParent();
	}

	return NULL;
}

void Node::sendParentSizeChange( const Vector2f& sizeChange ) {
	if ( reportSizeChangeToChilds() ) {
		Node* child = mChild;

		while ( NULL != child ) {
			child->onParentSizeChange( sizeChange );
			child = child->getNextNode();
		}
	}
}

bool Node::reportSizeChangeToChilds() const {
	return 0 != ( mNodeFlags & NODE_FLAG_REPORT_SIZE_CHANGE_TO_CHILDS );
}

void Node::enableReportSizeChangeToChilds() {
	writeNodeFlag( NODE_FLAG_REPORT_SIZE_CHANGE_TO_CHILDS, 1 );
}

void Node::disableReportSizeChangeToChilds() {
	writeNodeFlag( NODE_FLAG_REPORT_SIZE_CHANGE_TO_CHILDS, 0 );
}

Node* Node::centerHorizontal() {
	Node* node = getParent();
	if ( NULL != node ) {
		setPosition( eefloor( ( node->getSize().getWidth() - getSize().getWidth() ) * 0.5f ),
					 getPosition().y );
	}
	return this;
}

Node* Node::centerVertical() {
	Node* node = getParent();
	if ( NULL != node ) {
		setPosition( getPosition().x,
					 eefloor( node->getSize().getHeight() - getSize().getHeight() ) * 0.5f );
	}
	return this;
}

Node* Node::center() {
	Node* node = getParent();
	if ( NULL != node ) {
		setPosition( eefloor( ( node->getSize().getWidth() - getSize().getWidth() ) * 0.5f ),
					 eefloor( node->getSize().getHeight() - getSize().getHeight() ) * 0.5f );
	}
	return this;
}

Node* Node::clipEnable() {
	writeNodeFlag( NODE_FLAG_CLIP_ENABLE, 1 );
	return this;
}

Node* Node::clipDisable() {
	writeNodeFlag( NODE_FLAG_CLIP_ENABLE, 0 );
	return this;
}

void Node::clipSmartEnable( const Int32& x, const Int32& y, const Uint32& Width,
							const Uint32& Height ) {
	if ( isMeOrParentTreeScaledOrRotatedOrFrameBuffer() ) {
		GLi->getClippingMask()->clipPlaneEnable( x, y, Width, Height );
	} else {
		GLi->getClippingMask()->clipEnable( x, y, Width, Height );
	}
}

void Node::clipSmartDisable() {
	if ( isMeOrParentTreeScaledOrRotatedOrFrameBuffer() ) {
		GLi->getClippingMask()->clipPlaneDisable();
	} else {
		GLi->getClippingMask()->clipDisable();
	}
}

Node* Node::getDrawInvalidator() {
	Node* node = mParentNode;
	while ( node != NULL ) {
		if ( node->isDrawInvalidator() )
			return node;
		node = node->getParent();
	}
	return isDrawInvalidator() ? this : NULL;
}

bool Node::isDrawInvalidator() const {
	return false;
}

void Node::invalidate( Node* ) {
	if ( mVisible && mAlpha != 0.f ) {
		writeNodeFlag( NODE_FLAG_VIEW_DIRTY, 1 );
	}
}

bool Node::invalidated() const {
	return 0 != ( mNodeFlags & NODE_FLAG_VIEW_DIRTY );
}

}} // namespace EE::Scene
