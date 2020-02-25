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
	mParentCtrl( NULL ),
	mSceneNode( NULL ),
	mNodeDrawInvalidator( NULL ),
	mChild( NULL ),
	mChildLast( NULL ),
	mNext( NULL ),
	mPrev( NULL ),
	mNodeFlags( NODE_FLAG_POSITION_DIRTY | NODE_FLAG_POLYGON_DIRTY ),
	mBlend( BlendAlpha ),
	mNumCallBacks( 0 ),
	mVisible( true ),
	mEnabled( true ),
	mAlpha( 255.f ) {}

Node::~Node() {
	if ( !SceneManager::instance()->isShootingDown() && NULL != mSceneNode ) {
		if ( mSceneNode != this && NULL != mSceneNode->getActionManager() )
			mSceneNode->getActionManager()->removeAllActionsFromTarget( this );

		if ( mNodeFlags & NODE_FLAG_SCHEDULED_UPDATE )
			mSceneNode->unsubscribeScheduledUpdate( this );

		if ( isMouseOverMeOrChilds() )
			mSceneNode->removeMouseOverNode( this );
	}

	childDeleteAll();

	if ( NULL != mParentCtrl )
		mParentCtrl->childRemove( this );

	EventDispatcher* eventDispatcher = NULL != mSceneNode ? mSceneNode->getEventDispatcher() : NULL;

	if ( NULL != eventDispatcher ) {
		if ( eventDispatcher->getFocusControl() == this && mSceneNode != this ) {
			eventDispatcher->setFocusControl( mSceneNode );
		}

		if ( eventDispatcher->getOverControl() == this && mSceneNode != this ) {
			eventDispatcher->setOverControl( mSceneNode );
		}
	}
}

void Node::worldToNodeTranslation( Vector2f& Pos ) const {
	Node* ParentLoop = mParentCtrl;

	Pos -= mPosition;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->getPosition();

		Pos -= ParentPos;

		ParentLoop = ParentLoop->getParent();
	}
}

void Node::nodeToWorldTranslation( Vector2f& Pos ) const {
	Node* ParentLoop = mParentCtrl;

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
	Node* Ctrl = this;

	while ( NULL != Ctrl ) {
		if ( Ctrl->onMessage( Msg ) )
			break;

		Ctrl = Ctrl->getParent();
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
	updateCenter();
	sendCommonEvent( Event::OnSizeChange );
	invalidateDraw();
}

void Node::scheduledUpdate( const Time& ) {}

Node* Node::setSize( const Sizef& Size ) {
	if ( Size != mSize ) {
		Vector2f sizeChange( Size.x - mSize.x, Size.y - mSize.y );

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
	return mParentCtrl;
}

void Node::updateDrawInvalidator( bool force ) {
	mNodeDrawInvalidator = getDrawInvalidator();

	if ( NULL != mChild && ( mChild->mNodeDrawInvalidator != mNodeDrawInvalidator || force ) ) {
		Node* ChildLoop = mChild;

		while ( NULL != ChildLoop ) {
			ChildLoop->updateDrawInvalidator();

			ChildLoop = ChildLoop->mNext;
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

	if ( parent == mParentCtrl )
		return this;

	if ( NULL != mParentCtrl )
		mParentCtrl->childRemove( this );

	mParentCtrl = parent;

	updateDrawInvalidator();

	if ( NULL != mParentCtrl )
		mParentCtrl->childAdd( this );

	setDirty();

	onParentChange();

	if ( mSceneNode != findSceneNode() )
		onSceneChange();

	return this;
}

bool Node::isParentOf( Node* Ctrl ) const {
	eeASSERT( NULL != Ctrl );

	Node* tParent = Ctrl->getParent();

	while ( NULL != tParent ) {
		if ( this == tParent )
			return true;

		tParent = tParent->getParent();
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
	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->update( time );
		ChildLoop = ChildLoop->mNext;
	}

	writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 0 );
}

void Node::sendMouseEvent( const Uint32& Event, const Vector2i& Pos, const Uint32& Flags ) {
	MouseEvent MouseEvent( this, Event, Pos, Flags );
	sendEvent( &MouseEvent );
}

void Node::sendCommonEvent( const Uint32& event ) {
	Event CommonEvent( this, event );
	sendEvent( &CommonEvent );
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
	if ( NULL != mParentCtrl && mParentCtrl->isMouseOverMeOrChilds() )
		mParentCtrl->onMouseOver( Pos, Flags );

	writeNodeFlag( NODE_FLAG_MOUSEOVER, 1 );

	EventDispatcher* eventDispatcher = NULL != mSceneNode ? mSceneNode->getEventDispatcher() : NULL;

	if ( NULL != eventDispatcher && eventDispatcher->getOverControl() == this )
		sendMouseEvent( Event::MouseOver, Pos, Flags );

	return 1;
}

Uint32 Node::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != mParentCtrl && !mParentCtrl->isMouseOverMeOrChilds() )
		mParentCtrl->onMouseLeave( Pos, Flags );

	writeNodeFlag( NODE_FLAG_MOUSEOVER, 0 );

	EventDispatcher* eventDispatcher = NULL != mSceneNode ? mSceneNode->getEventDispatcher() : NULL;

	if ( NULL != eventDispatcher && eventDispatcher->getOverControl() == this )
		sendMouseEvent( Event::MouseLeave, Pos, Flags );

	return 1;
}

Uint32 Node::onCalculateDrag( const Vector2f&, const Uint32& ) {
	return 1;
}

void Node::onClose() {
	sendCommonEvent( Event::OnClose );
	invalidateDraw();
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

void Node::toFront() {
	if ( NULL != mParentCtrl && mParentCtrl->mChildLast != this ) {
		mParentCtrl->childRemove( this );
		mParentCtrl->childAdd( this );
	}
}

void Node::toBack() {
	if ( NULL != mParentCtrl ) {
		mParentCtrl->childAddAt( this, 0 );
	}
}

void Node::toPosition( const Uint32& Pos ) {
	if ( NULL != mParentCtrl ) {
		mParentCtrl->childAddAt( this, Pos );
	}
}

void Node::onVisibilityChange() {
	sendCommonEvent( Event::OnVisibleChange );
	invalidateDraw();
}

void Node::onEnabledChange() {
	EventDispatcher* eventDispatcher = NULL != mSceneNode ? mSceneNode->getEventDispatcher() : NULL;

	if ( !isEnabled() && NULL != eventDispatcher && NULL != eventDispatcher->getFocusControl() ) {
		if ( isChild( eventDispatcher->getFocusControl() ) ) {
			eventDispatcher->setFocusControl( NULL );
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
				  Sizef( ( Float )(int)mSize.getWidth(), ( Float )(int)mSize.getHeight() ) );
}

Rectf Node::getLocalBounds() const {
	return Rectf( 0, 0, mSize.getWidth(), mSize.getHeight() );
}

const Uint32& Node::getNodeFlags() const {
	return mNodeFlags;
}

void Node::setNodeFlags( const Uint32& Flags ) {
	mNodeFlags = Flags;
}

void Node::drawChilds() {
	if ( isReverseDraw() ) {
		Node* ChildLoop = mChildLast;

		while ( NULL != ChildLoop ) {
			if ( ChildLoop->mVisible ) {
				ChildLoop->internalDraw();
			}

			ChildLoop = ChildLoop->mPrev;
		}
	} else {
		Node* ChildLoop = mChild;

		while ( NULL != ChildLoop ) {
			if ( ChildLoop->mVisible ) {
				ChildLoop->internalDraw();
			}

			ChildLoop = ChildLoop->mNext;
		}
	}
}

void Node::internalDraw() {
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

void Node::childAdd( Node* ChildCtrl ) {
	if ( NULL == mChild ) {
		mChild = ChildCtrl;
		mChildLast = ChildCtrl;
	} else {
		mChildLast->mNext = ChildCtrl;
		ChildCtrl->mPrev = mChildLast;
		ChildCtrl->mNext = NULL;
		mChildLast = ChildCtrl;
	}

	onChildCountChange();
}

void Node::childAddAt( Node* ChildCtrl, Uint32 Pos ) {
	eeASSERT( NULL != ChildCtrl );

	Node* ChildLoop = mChild;

	ChildCtrl->setParent( this );

	childRemove( ChildCtrl );

	ChildCtrl->mParentCtrl = this;
	ChildCtrl->mSceneNode = ChildCtrl->findSceneNode();

	if ( ChildLoop == NULL ) {
		mChild = ChildCtrl;
		mChildLast = ChildCtrl;
		ChildCtrl->mNext = NULL;
		ChildCtrl->mPrev = NULL;
	} else {
		if ( Pos == 0 ) {
			if ( NULL != mChild ) {
				mChild->mPrev = ChildCtrl;
			}

			ChildCtrl->mNext = mChild;
			ChildCtrl->mPrev = NULL;
			mChild = ChildCtrl;
		} else {
			Uint32 i = 0;

			while ( NULL != ChildLoop->mNext && i < Pos ) {
				ChildLoop = ChildLoop->mNext;
				i++;
			}

			Node* ChildTmp = ChildLoop->mNext;
			ChildLoop->mNext = ChildCtrl;
			ChildCtrl->mPrev = ChildLoop;
			ChildCtrl->mNext = ChildTmp;

			if ( NULL != ChildTmp ) {
				ChildTmp->mPrev = ChildCtrl;
			} else {
				mChildLast = ChildCtrl;
			}
		}
	}

	onChildCountChange();
}

void Node::childRemove( Node* ChildCtrl ) {
	if ( ChildCtrl == mChild ) {
		mChild = mChild->mNext;

		if ( NULL != mChild ) {
			mChild->mPrev = NULL;

			if ( ChildCtrl == mChildLast )
				mChildLast = mChild;
		} else {
			mChildLast = NULL;
		}
	} else {
		if ( mChildLast == ChildCtrl )
			mChildLast = mChildLast->mPrev;

		ChildCtrl->mPrev->mNext = ChildCtrl->mNext;

		if ( NULL != ChildCtrl->mNext ) {
			ChildCtrl->mNext->mPrev = ChildCtrl->mPrev;
			ChildCtrl->mNext = NULL;
		}

		ChildCtrl->mPrev = NULL;
	}

	onChildCountChange();
}

void Node::childsCloseAll() {
	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->close();
		ChildLoop = ChildLoop->mNext;
	}
}

std::string Node::getId() const {
	return mId;
}

Node* Node::setId( const std::string& id ) {
	mId = id;
	mIdHash = String::hash( id );
	return this;
}

Uint32 Node::getIdHash() const {
	return mIdHash;
}

Node* Node::findIdHash( const Uint32& idHash ) const {
	if ( mIdHash == idHash ) {
		return const_cast<Node*>( this );
	} else {
		Node* child = mChild;

		while ( NULL != child ) {
			Node* foundCtrl = child->findIdHash( idHash );

			if ( NULL != foundCtrl )
				return foundCtrl;

			child = child->mNext;
		}
	}

	return NULL;
}

Node* Node::find( const std::string& id ) const {
	return findIdHash( String::hash( id ) );
}

bool Node::isChild( Node* ChildCtrl ) const {
	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildCtrl == ChildLoop )
			return true;

		ChildLoop = ChildLoop->mNext;
	}

	return false;
}

bool Node::inParentTreeOf( Node* Child ) const {
	Node* ParentLoop = Child->mParentCtrl;

	while ( NULL != ParentLoop ) {
		if ( ParentLoop == this )
			return true;

		ParentLoop = ParentLoop->mParentCtrl;
	}

	return false;
}

Uint32 Node::getChildCount() const {
	Node* ChildLoop = mChild;
	Uint32 Count = 0;

	while ( NULL != ChildLoop ) {
		ChildLoop = ChildLoop->mNext;
		Count++;
	}

	return Count;
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

Node* Node::getChildAt( Uint32 Index ) const {
	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop && Index ) {
		ChildLoop = ChildLoop->mNext;
		Index--;
	}

	return ChildLoop;
}

Uint32 Node::getNodeIndex() const {
	Uint32 nodeIndex = 0;
	if ( NULL != mParentCtrl ) {
		Node* parentChild = mParentCtrl->mChild;
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
	if ( NULL != mParentCtrl ) {
		Node* parentChild = mParentCtrl->mChild;
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

Node* Node::overFind( const Vector2f& Point ) {
	Node* pOver = NULL;

	if ( ( mNodeFlags & NODE_FLAG_OVER_FIND_ALLOWED ) && mEnabled && mVisible ) {
		updateWorldPolygon();

		if ( mWorldBounds.contains( Point ) && mPoly.pointInside( Point ) ) {
			writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );
			mSceneNode->addMouseOverNode( this );

			Node* ChildLoop = mChildLast;

			while ( NULL != ChildLoop ) {
				Node* ChildOver = ChildLoop->overFind( Point );

				if ( NULL != ChildOver ) {
					pOver = ChildOver;

					break; // Search from top to bottom, so the first over will be the topmost
				}

				ChildLoop = ChildLoop->mPrev;
			}

			if ( NULL == pOver )
				pOver = this;
		}
	}

	return pOver;
}

void Node::onSceneChange() {
	mSceneNode = findSceneNode();

	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->onSceneChange();
		ChildLoop = ChildLoop->mNext;
	}
}

bool Node::isWidget() const {
	return 0 != ( mNodeFlags & NODE_FLAG_WIDGET );
}

bool Node::isWindow() const {
	return 0 != ( mNodeFlags & NODE_FLAG_WINDOW );
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
	const Node* Ctrl = this;

	while ( NULL != Ctrl ) {
		if ( !Ctrl->isVisible() )
			return false;

		Ctrl = Ctrl->getParent();
	}

	return true;
}

bool Node::isMeOrParentTreeRotated() const {
	const Node* Ctrl = this;

	while ( NULL != Ctrl ) {
		if ( Ctrl->isRotated() )
			return true;

		Ctrl = Ctrl->getParent();
	}

	return false;
}

bool Node::isMeOrParentTreeScaled() const {
	const Node* Ctrl = this;

	while ( NULL != Ctrl ) {
		if ( Ctrl->isScaled() )
			return true;

		Ctrl = Ctrl->getParent();
	}

	return false;
}

bool Node::isMeOrParentTreeScaledOrRotated() const {
	const Node* Ctrl = this;

	while ( NULL != Ctrl ) {
		if ( Ctrl->isScaled() || Ctrl->isRotated() )
			return true;

		Ctrl = Ctrl->getParent();
	}

	return false;
}

bool Node::isMeOrParentTreeScaledOrRotatedOrFrameBuffer() const {
	const Node* Ctrl = this;

	while ( NULL != Ctrl ) {
		if ( Ctrl->isScaled() || Ctrl->isRotated() || Ctrl->isFrameBuffer() )
			return true;

		Ctrl = Ctrl->getParent();
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

Uint32 Node::addEventListener( const Uint32& EventType, const EventCallback& Callback ) {
	mNumCallBacks++;

	mEvents[EventType][mNumCallBacks] = Callback;

	return mNumCallBacks;
}

void Node::removeEventListener( const Uint32& CallbackId ) {
	EventsMap::iterator it;

	for ( it = mEvents.begin(); it != mEvents.end(); ++it ) {
		std::map<Uint32, EventCallback>& event = it->second;

		if ( event.erase( CallbackId ) > 0 )
			break;
	}
}

void Node::sendEvent( const Event* Event ) {
	if ( 0 != mEvents.count( Event->getType() ) ) {
		std::map<Uint32, EventCallback> event = mEvents[Event->getType()];
		std::map<Uint32, EventCallback>::iterator it;

		if ( event.begin() != event.end() ) {
			for ( it = event.begin(); it != event.end(); ++it )
				it->second( Event );
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

void Node::onChildCountChange() {
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
	Node* Ctrl = mParentCtrl;

	while ( Ctrl != NULL ) {
		if ( Ctrl->isSceneNode() )
			return static_cast<SceneNode*>( Ctrl );

		Ctrl = Ctrl->getParent();
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
	Node* CurChild = mChild;

	while ( NULL != CurChild ) {
		CurChild->setAlpha( alpha );
		CurChild->setChildsAlpha( alpha );
		CurChild = CurChild->getNextNode();
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

void Node::removeAction( Action* action ) {
	getActionManager()->removeAction( action );
}

void Node::removeActions( const std::vector<Action*>& actions ) {
	getActionManager()->removeActions( actions );
}

void Node::removeActionsByTag( const Uint32& tag ) {
	getActionManager()->removeActionsByTagFromTarget( this, tag );
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

void Node::runOnMainThread( Actions::Runnable::RunnableFunc runnable, const Time& delay ) {
	runAction( Actions::Runnable::New( runnable, delay ) );
}

Transform Node::getLocalTransform() const {
	return getTransform();
}

Transform Node::getGlobalTransform() const {
	return NULL != mParentCtrl ? mParentCtrl->getGlobalTransform() * getTransform()
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

void Node::setFocus() {}

Node* Node::getNextWidget() const {
	Node* Found = NULL;
	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isVisible() && ChildLoop->isEnabled() ) {
			if ( ChildLoop->isWidget() ) {
				return ChildLoop;
			} else {
				Found = ChildLoop->getNextWidget();

				if ( NULL != Found ) {
					return Found;
				}
			}
		}

		ChildLoop = ChildLoop->getNextNode();
	}

	if ( NULL != mNext ) {
		if ( mNext->isVisible() && mNext->isEnabled() && mNext->isWidget() ) {
			return mNext;
		} else {
			return mNext->getNextWidget();
		}
	} else {
		ChildLoop = mParentCtrl;

		while ( NULL != ChildLoop ) {
			if ( NULL != ChildLoop->getNextNode() ) {
				if ( ChildLoop->getNextNode()->isVisible() &&
					 ChildLoop->getNextNode()->isEnabled() &&
					 ChildLoop->getNextNode()->isWidget() ) {
					return ChildLoop->getNextNode();
				} else {
					return ChildLoop->getNextNode()->getNextWidget();
				}
			}

			ChildLoop = ChildLoop->getParent();
		}
	}

	return NULL;
}

Node* Node::getParentWidget() const {
	Node* parentNode = mParentCtrl;

	while ( NULL != parentNode ) {
		if ( parentNode->isWidget() ) {
			return parentNode;
		}

		parentNode = parentNode->getParent();
	}

	return NULL;
}

void Node::sendParentSizeChange( const Vector2f& SizeChange ) {
	if ( reportSizeChangeToChilds() ) {
		Node* ChildLoop = mChild;

		while ( NULL != ChildLoop ) {
			ChildLoop->onParentSizeChange( SizeChange );
			ChildLoop = ChildLoop->getNextNode();
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

void Node::centerHorizontal() {
	Node* Ctrl = getParent();

	if ( NULL != Ctrl ) {
		setPosition( eefloor( ( Ctrl->getSize().getWidth() - getSize().getWidth() ) * 0.5f ),
					 getPosition().y );
	}
}

void Node::centerVertical() {
	Node* Ctrl = getParent();

	if ( NULL != Ctrl ) {
		setPosition( getPosition().x,
					 eefloor( Ctrl->getSize().getHeight() - getSize().getHeight() ) * 0.5f );
	}
}

void Node::center() {
	Node* Ctrl = getParent();

	if ( NULL != Ctrl )
		setPosition( eefloor( ( Ctrl->getSize().getWidth() - getSize().getWidth() ) * 0.5f ),
					 eefloor( Ctrl->getSize().getHeight() - getSize().getHeight() ) * 0.5f );
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
	Node* Ctrl = mParentCtrl;

	while ( Ctrl != NULL ) {
		if ( Ctrl->isDrawInvalidator() )
			return Ctrl;

		Ctrl = Ctrl->getParent();
	}

	return isDrawInvalidator() ? this : NULL;
}

bool Node::isDrawInvalidator() const {
	return false;
}

void Node::invalidate( Node* invalidator ) {
	if ( mVisible && mAlpha != 0.f ) {
		writeNodeFlag( NODE_FLAG_VIEW_DIRTY, 1 );
	}
}

bool Node::invalidated() const {
	return 0 != ( mNodeFlags & NODE_FLAG_VIEW_DIRTY );
}

}} // namespace EE::Scene
