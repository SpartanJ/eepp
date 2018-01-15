#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uiactionmanager.hpp>
#include <eepp/ui/uiaction.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/window/engine.hpp>

#include <eepp/ui/actions/fade.hpp>
#include <eepp/ui/actions/scale.hpp>
#include <eepp/ui/actions/rotate.hpp>
#include <eepp/ui/actions/move.hpp>

namespace EE { namespace UI {

UINode * UINode::New() {
	return eeNew( UINode, () );
}

UINode::UINode() :
	mIdHash( 0 ),
	mPos( 0, 0 ),
	mRealPos( 0, 0 ),
	mSize( 0, 0 ),
	mRealSize( 0, 0 ),
	mFlags( UI_CONTROL_DEFAULT_FLAGS ),
	mData( 0 ),
	mParentCtrl( NULL ),
	mParentWindowCtrl( NULL ),
	mChild( NULL ),
	mChildLast( NULL ),
	mNext( NULL ),
	mPrev( NULL ),
	mSkinState( NULL ),
	mBackground( NULL ),
	mBorder( NULL ),
	mNodeFlags( NODE_FLAG_POSITION_DIRTY | NODE_FLAG_TRANFORM_DIRTY | NODE_FLAG_TRANFORM_INVERT_DIRTY ),
	mBlend( BlendAlpha ),
	mNumCallBacks( 0 ),
	mVisible( true ),
	mEnabled( true ),
	mDragButton( EE_BUTTON_LMASK ),
	mAlpha(255.f),
	mActionManager(NULL)
{
	if ( NULL == mParentCtrl && NULL != UIManager::instance()->getMainControl() ) {
		mParentCtrl = UIManager::instance()->getMainControl();
		mParentWindowCtrl = getParentWindow();
	}

	if ( NULL != mParentCtrl )
		mParentCtrl->childAdd( this );
}

UINode::~UINode() {
	removeSkin();
	eeSAFE_DELETE( mBackground );
	eeSAFE_DELETE( mBorder );
	eeSAFE_DELETE( mActionManager );

	childDeleteAll();

	if ( NULL != mParentCtrl )
		mParentCtrl->childRemove( this );

	if ( UIManager::instance()->getFocusControl() == this && UIManager::instance()->getMainControl() != this ) {
		UIManager::instance()->setFocusControl( UIManager::instance()->getMainControl() );
	}

	if ( UIManager::instance()->getOverControl() == this && UIManager::instance()->getMainControl() != this ) {
		UIManager::instance()->setOverControl( UIManager::instance()->getMainControl() );
	}
}

void UINode::screenToNode( Vector2i& Pos ) const {
	UINode * ParentLoop = mParentCtrl;

	Pos.x -= mRealPos.x;
	Pos.y -= mRealPos.y;

	while ( NULL != ParentLoop ) {
		const Vector2i& ParentPos = ParentLoop->getRealPosition();

		Pos.x -= ParentPos.x;
		Pos.y -= ParentPos.y;

		ParentLoop = ParentLoop->getParent();
	}
}

void UINode::nodeToScreen( Vector2i& Pos ) const {
	UINode * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		const Vector2i& ParentPos = ParentLoop->getRealPosition();

		Pos.x += ParentPos.x;
		Pos.y += ParentPos.y;

		ParentLoop = ParentLoop->getParent();
	}
}

Uint32 UINode::getType() const {
	return UI_TYPE_NODE;
}

bool UINode::isType( const Uint32& type ) const {
	return UINode::getType() == type;
}

void UINode::messagePost( const UIMessage * Msg ) {
	UINode * Ctrl = this;

	while( NULL != Ctrl ) {
		if ( Ctrl->onMessage( Msg ) )
			break;

		Ctrl = Ctrl->getParent();
	}
}

Uint32 UINode::onMessage( const UIMessage * Msg ) {
	return 0;
}

void UINode::setInternalPosition( const Vector2i& Pos ) {
	mPos = Pos;
	mRealPos = Vector2i( Pos.x * PixelDensity::getPixelDensity(), Pos.y * PixelDensity::getPixelDensity() );
	Transformable::setPosition( mRealPos.x, mRealPos.y );
	setDirty();
}

UINode * UINode::setPosition( const Vector2f& Pos ) {
	return setPosition( Vector2i( Pos.x, Pos.y ) );
}

UINode * UINode::setPosition( const Vector2i& Pos ) {
	if ( Pos != mPos ) {
		setInternalPosition( Pos );
		onPositionChange();
	}
	return this;
}

UINode * UINode::setPosition( const Int32& x, const Int32& y ) {
	setPosition( Vector2i( x, y ) );
	return this;
}

void UINode::setPixelsPosition( const Vector2i& Pos ) {
	if ( mRealPos != Pos ) {
		mPos = Vector2i( PixelDensity::pxToDpI( Pos.x ), PixelDensity::pxToDpI( Pos.y ) );
		mRealPos = Pos;
		setDirty();
		onPositionChange();
	}
}

void UINode::setPixelsPosition( const Int32& x, const Int32& y ) {
	setPixelsPosition( Vector2i( x, y ) );
}

const Vector2i& UINode::getPosition() const {
	return mPos;
}

const Vector2i &UINode::getRealPosition() const {
	return mRealPos;
}

void UINode::setInternalSize( const Sizei& size ) {
	mSize = size;
	mRealSize = Sizei( size.x * PixelDensity::getPixelDensity(), size.y * PixelDensity::getPixelDensity() );
	updateCenter();
	sendCommonEvent( UIEvent::OnSizeChange );
	invalidateDraw();
}

void UINode::setInternalPixelsSize( const Sizei& size ) {
	mSize = PixelDensity::pxToDpI( size );
	mRealSize = size;
	updateCenter();
	sendCommonEvent( UIEvent::OnSizeChange );
	invalidateDraw();
}

UINode * UINode::setSize( const Sizei& Size ) {
	if ( Size != mSize ) {
		Vector2i sizeChange( Size.x - mSize.x, Size.y - mSize.y );

		setInternalSize( Size );

		onSizeChange();

		if ( mFlags & UI_REPORT_SIZE_CHANGE_TO_CHILDS ) {
			sendParentSizeChange( sizeChange );
		}
	}

	return this;
}

UINode * UINode::setSize( const Int32& Width, const Int32& Height ) {
	setSize( Sizei( Width, Height ) );
	return this;
}

void UINode::setPixelsSize( const Sizei & size ) {
	if ( size != mRealSize ) {
		Vector2i sizeChange( size.x - mRealSize.x, size.y - mRealSize.y );

		setInternalPixelsSize( size );

		onSizeChange();

		if ( mFlags & UI_REPORT_SIZE_CHANGE_TO_CHILDS ) {
			sendParentSizeChange( PixelDensity::pxToDpI( sizeChange ) );
		}
	}
}

void UINode::setPixelsSize( const Int32& x, const Int32& y ) {
	setPixelsSize( Sizei( x, y ) );
}

void UINode::setInternalWidth( const Int32& width ) {
	setInternalSize( Sizei( width, mSize.getHeight() ) );
}

void UINode::setInternalHeight( const Int32& height ) {
	setInternalSize( Sizei( mSize.getWidth(), height ) );
}

void UINode::setInternalPixelsWidth( const Int32& width ) {
	setInternalPixelsSize( Sizei( width, mRealSize.y ) );
}

void UINode::setInternalPixelsHeight( const Int32& height ) {
	setInternalPixelsSize( Sizei( mRealSize.x, height ) );
}

Rect UINode::getRect() const {
	return Rect( mPos, mSize );
}

const Sizei& UINode::getSize() {
	return mSize;
}

const Sizei& UINode::getRealSize() {
	return mRealSize;
}

UINode * UINode::setVisible( const bool& visible ) {
	if ( mVisible != visible ) {
		mVisible = visible;
		onVisibilityChange();
	}
	return this;
}

bool UINode::isVisible() const {
	return mVisible;
}

bool UINode::isHided() const {
	return !mVisible;
}

UINode * UINode::setEnabled( const bool& enabled ) {
	if ( mEnabled != enabled ) {
		mEnabled = enabled;
		onEnabledChange();
	}
	return this;
}

bool UINode::isEnabled() const {
	return mEnabled;
}

bool UINode::isDisabled() const {
	return !mEnabled;
}

UINode * UINode::getParent() const {
	return mParentCtrl;
}

UINode * UINode::setParent( UINode * parent ) {
	if ( parent == mParentCtrl )
		return this;

	if ( NULL != mParentCtrl )
		mParentCtrl->childRemove( this );

	mParentCtrl = parent;

	if ( NULL != mParentCtrl )
		mParentCtrl->childAdd( this );

	setDirty();

	onParentChange();

	if ( mParentWindowCtrl != getParentWindow() )
		onParentWindowChange();

	return this;
}

bool UINode::isParentOf( UINode * Ctrl ) {
	eeASSERT( NULL != Ctrl );

	UINode * tParent = Ctrl->getParent();

	while ( NULL != tParent ) {
		if ( this == tParent )
			return true;

		tParent = tParent->getParent();
	}

	return false;
}

void UINode::centerHorizontal() {
	UINode * Ctrl = getParent();

	if ( NULL != Ctrl )
		setPosition( ( Ctrl->getSize().getWidth() - mSize.getWidth() ) / 2, mPos.y );
}

void UINode::centerVertical(){
	UINode * Ctrl = getParent();

	if ( NULL != Ctrl )
		setPosition( mPos.x, ( Ctrl->getSize().getHeight() - mSize.getHeight() ) / 2 );
}

void UINode::center() {
	centerHorizontal();
	centerVertical();
}

void UINode::close() {
	mNodeFlags |= NODE_FLAG_CLOSE;

	UIManager::instance()->addToCloseQueue( this );
}

void UINode::drawHighlightFocus() {
	if ( UIManager::instance()->getHighlightFocus() && UIManager::instance()->getFocusControl() == this ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( UIManager::instance()->getHighlightFocusColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getRectf() );
	}
}

void UINode::drawOverNode() {
	if ( UIManager::instance()->getHighlightOver() && UIManager::instance()->getOverControl() == this ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( UIManager::instance()->getHighlightOverColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getRectf() );
	}
}

void UINode::drawDebugData() {
	if ( UIManager::instance()->getDrawDebugData() ) {
		if ( isWidget() ) {
			UIWidget * me = static_cast<UIWidget*>( this );

			if ( UIManager::instance()->getOverControl() == this ) {
				String text( String::strFormated( "X: %d Y: %d\nW: %d H: %d", mPos.x, mPos.y, mSize.x, mSize.y ) );

				if ( !mId.empty() ) {
					text = "ID: " + mId + "\n" + text;
				}

				me->setTooltipText( text );
			} else {
				me->setTooltipText( "" );
			}
		}
	}
}

void UINode::drawBox() {
	if ( UIManager::instance()->getDrawBoxes() ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( Color::fromPointer( this ) );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getRectf() );
	}
}

void UINode::drawSkin() {
	if ( NULL != mSkinState ) {
		if ( mFlags & UI_SKIN_KEEP_SIZE_ON_DRAW ) {
			Sizei rSize = PixelDensity::dpToPxI( mSkinState->getSkin()->getSize( mSkinState->getState() ) );
			Sizei diff = ( mRealSize - rSize ) / 2;

			mSkinState->draw( mScreenPosf.x + diff.x, mScreenPosf.y + diff.y, (Float)rSize.getWidth(), (Float)rSize.getHeight(), (Uint32)mAlpha );
		} else {
			mSkinState->draw( mScreenPosf.x, mScreenPosf.y, (Float)mRealSize.getWidth(), (Float)mRealSize.getHeight(), (Uint32)mAlpha );
		}
	}
}

void UINode::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		drawBackground();

		drawBorder();

		drawSkin();

		drawHighlightFocus();

		drawOverNode();

		drawDebugData();

		drawBox();
	}
}

void UINode::update( const Time& time ) {
	if ( NULL != mActionManager ) {
		mActionManager->update( time );

		if ( mActionManager->isEmpty() )
			eeSAFE_DELETE( mActionManager );
	}

	if ( isDragEnabled() && isDragging() ) {
		if ( !( UIManager::instance()->getPressTrigger() & mDragButton ) ) {
			setDragging( false );
			UIManager::instance()->setControlDragging( false );
			return;
		}

		Vector2i Pos( UIManager::instance()->getMousePos() );

		if ( mDragPoint != Pos && ( abs( mDragPoint.x - Pos.x ) > PixelDensity::getPixelDensity() || abs( mDragPoint.y - Pos.y ) > PixelDensity::getPixelDensity() ) ) {
			if ( onDrag( Pos ) ) {
				Sizei dragDiff;

				dragDiff.x = (Int32)( (Float)( mDragPoint.x - Pos.x ) / PixelDensity::getPixelDensity() );
				dragDiff.y = (Int32)( (Float)( mDragPoint.y - Pos.y ) / PixelDensity::getPixelDensity() );

				setInternalPosition( mPos - dragDiff );

				mDragPoint = Pos;

				onPositionChange();

				UIManager::instance()->setControlDragging( true );
			}
		}
	}

	UINode * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->update( time );
		ChildLoop = ChildLoop->mNext;
	}

	if ( mNodeFlags & NODE_FLAG_MOUSEOVER_ME_OR_CHILD )
		writeCtrlFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 0 );
}

void UINode::sendMouseEvent( const Uint32& Event, const Vector2i& Pos, const Uint32& Flags ) {
	UIEventMouse MouseEvent( this, Event, Pos, Flags );
	sendEvent( &MouseEvent );
}

void UINode::sendCommonEvent( const Uint32& Event ) {
	UIEvent CommonEvent( this, Event );
	sendEvent( &CommonEvent );
}

Uint32 UINode::onKeyDown( const UIEventKey& Event ) {
	sendEvent( &Event );
	return 0;
}

Uint32 UINode::onKeyUp( const UIEventKey& Event ) {
	sendEvent( &Event );
	return 0;
}

Uint32 UINode::onMouseMove( const Vector2i& Pos, const Uint32 Flags ) {
	sendMouseEvent( UIEvent::MouseMove, Pos, Flags );
	return 1;
}

Uint32 UINode::onMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( !( UIManager::instance()->getLastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && isDragEnabled() && !isDragging() ) {
		setDragging( true );
		mDragPoint = Pos;
	}

	sendMouseEvent( UIEvent::MouseDown, Pos, Flags );

	setSkinState( UISkinState::StateMouseDown );

	return 1;
}

Uint32 UINode::onMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isDragEnabled() && isDragging() && ( Flags & mDragButton ) ) {
		setDragging( false );
	}

	sendMouseEvent( UIEvent::MouseUp, Pos, Flags );

	setPrevSkinState();

	return 1;
}

Uint32 UINode::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	sendMouseEvent( UIEvent::MouseClick, Pos, Flags );
	return 1;
}

bool UINode::isMouseOver() {
	return 0 != ( mNodeFlags & NODE_FLAG_MOUSEOVER );
}

bool UINode::isMouseOverMeOrChilds() {
	return 0 != ( mNodeFlags & NODE_FLAG_MOUSEOVER_ME_OR_CHILD );
}

Uint32 UINode::onMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	sendMouseEvent( UIEvent::MouseDoubleClick, Pos, Flags );
	return 1;
}

Uint32 UINode::onMouseEnter( const Vector2i& Pos, const Uint32 Flags ) {
	writeCtrlFlag( NODE_FLAG_MOUSEOVER, 1 );

	sendMouseEvent( UIEvent::MouseEnter, Pos, Flags );

	setSkinState( UISkinState::StateMouseEnter );

	return 1;
}

Uint32 UINode::onMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	writeCtrlFlag( NODE_FLAG_MOUSEOVER, 0 );

	sendMouseEvent( UIEvent::MouseExit, Pos, Flags );

	setSkinState( UISkinState::StateMouseExit );

	return 1;
}

Uint32 UINode::onFocus() {
	mNodeFlags |= NODE_FLAG_HAS_FOCUS;

	sendCommonEvent( UIEvent::OnFocus );

	setSkinState( UISkinState::StateFocus );

	return 1;
}

Uint32 UINode::onFocusLoss() {
	mNodeFlags &= ~NODE_FLAG_HAS_FOCUS;

	sendCommonEvent( UIEvent::OnFocusLoss );

	invalidateDraw();

	return 1;
}

void UINode::onWidgetFocusLoss() {
	sendCommonEvent( UIEvent::OnWidgetFocusLoss );
	invalidateDraw();
}

bool UINode::hasFocus() const {
	return 0 != ( mNodeFlags & NODE_FLAG_HAS_FOCUS );
}

Uint32 UINode::onValueChange() {
	sendCommonEvent( UIEvent::OnValueChange );
	invalidateDraw();

	return 1;
}

void UINode::onClose() {
	sendCommonEvent( UIEvent::OnClose );
	invalidateDraw();
}

Uint32 UINode::getHorizontalAlign() const {
	return mFlags & UI_HALIGN_MASK;
}

UINode * UINode::setHorizontalAlign( Uint32 halign ) {
	mFlags &= ~UI_HALIGN_MASK;
	mFlags |= halign & UI_HALIGN_MASK;

	onAlignChange();
	return this;
}

Uint32 UINode::getVerticalAlign() const {
	return mFlags & UI_VALIGN_MASK;
}

UINode * UINode::setVerticalAlign( Uint32 valign ) {
	mFlags &= ~UI_VALIGN_MASK;
	mFlags |= valign & UI_VALIGN_MASK;

	onAlignChange();
	return this;
}

UINode * UINode::setGravity( Uint32 hvalign ) {
	mFlags &= ~( UI_VALIGN_MASK | UI_HALIGN_MASK );
	mFlags |= ( hvalign & ( UI_VALIGN_MASK | UI_HALIGN_MASK ) ) ;

	onAlignChange();
	return this;
}

UIBackground * UINode::setBackgroundFillEnabled( bool enabled ) {
	writeFlag( UI_FILL_BACKGROUND, enabled ? 1 : 0 );

	if ( enabled && NULL == mBackground ) {
		mBackground = UIBackground::New( this );
	}

	invalidateDraw();

	return mBackground;
}

UIBorder * UINode::setBorderEnabled( bool enabled ) {
	writeFlag( UI_BORDER, enabled ? 1 : 0 );

	if ( enabled && NULL == mBorder ) {
		mBorder = UIBorder::New( this );

		if ( NULL == mBackground ) {
			mBackground = UIBackground::New( this );
		}
	}

	return mBorder;
}

UINode * UINode::getNextNode() const {
	return mNext;
}

UINode * UINode::getPrevNode() const {
	return mPrev;
}

UINode * UINode::getNextNodeLoop() const {
	if ( NULL == mNext )
		return getParent()->getFirstChild();
	else
		return mNext;
}

UINode * UINode::setData(const UintPtr& data ) {
	mData = data;
	return this;
}

const UintPtr& UINode::getData() const {
	return mData;
}

const Uint32& UINode::getFlags() const {
	return mFlags;
}

UINode * UINode::setFlags( const Uint32& flags ) {
	if ( NULL == mBackground && ( flags & UI_FILL_BACKGROUND ) )
		mBackground = UIBackground::New( this );

	if ( NULL == mBorder && ( flags & UI_BORDER ) )
		mBorder = UIBorder::New( this );

	if ( fontHAlignGet( flags ) || fontVAlignGet( flags ) ) {
		onAlignChange();
	}

	mFlags |= flags;

	return this;
}

UINode * UINode::unsetFlags(const Uint32 & flags) {
	if ( mFlags & flags )
		mFlags &= ~flags;

	if ( fontHAlignGet( flags ) || fontVAlignGet( flags ) ) {
		onAlignChange();
	}

	return this;
}

UINode *UINode::resetFlags( Uint32 newFlags ) {
	mFlags = newFlags;
	return this;
}

UINode * UINode::setBlendMode( const BlendMode& blend ) {
	mBlend = blend;
	invalidateDraw();
	return this;
}

BlendMode UINode::getBlendMode() {
	return mBlend;
}

void UINode::toFront() {
	if ( NULL != mParentCtrl && mParentCtrl->mChildLast != this ) {
		mParentCtrl->childRemove( this );
		mParentCtrl->childAdd( this );
	}
}

void UINode::toBack() {
	if ( NULL != mParentCtrl ) {
		mParentCtrl->childAddAt( this, 0 );
	}
}

void UINode::toPosition( const Uint32& Pos ) {
	if ( NULL != mParentCtrl ) {
		mParentCtrl->childAddAt( this, Pos );
	}
}

void UINode::onVisibilityChange() {
	sendCommonEvent( UIEvent::OnVisibleChange );
	invalidateDraw();
}

void UINode::onEnabledChange() {
	if ( !isEnabled() && NULL != UIManager::instance()->getFocusControl() ) {
		if ( isChild( UIManager::instance()->getFocusControl() ) ) {
			UIManager::instance()->setFocusControl( NULL );
		}
	}

	sendCommonEvent( UIEvent::OnEnabledChange );
	invalidateDraw();
}

void UINode::onPositionChange() {
	sendCommonEvent( UIEvent::OnPosChange );
	invalidateDraw();
}

void UINode::onSizeChange() {
	updateOriginPoint();

	invalidateDraw();
}

Rectf UINode::getRectf() {
	return Rectf( mScreenPosf, Sizef( (Float)mRealSize.getWidth(), (Float)mRealSize.getHeight() ) );
}

void UINode::drawBackground() {
	if ( mFlags & UI_FILL_BACKGROUND ) {
		mBackground->draw( getRectf(), mAlpha );
	}
}

void UINode::drawBorder() {
	if ( mFlags & UI_BORDER ) {
		mBorder->draw( getRectf(), mAlpha, mBackground->getCorners(), ( mFlags & UI_CLIP_ENABLE ) != 0 );
	}
}

const Uint32& UINode::getNodeFlags() const {
	return mNodeFlags;
}

void UINode::setNodeFlags( const Uint32& Flags ) {
	mNodeFlags = Flags;
}

void UINode::drawChilds() {
	if ( isReverseDraw() ) {
		UINode * ChildLoop = mChildLast;

		while ( NULL != ChildLoop ) {
			if ( ChildLoop->mVisible ) {
				ChildLoop->internalDraw();
			}

			ChildLoop = ChildLoop->mPrev;
		}
	} else {
		UINode * ChildLoop = mChild;

		while ( NULL != ChildLoop ) {
			if ( ChildLoop->mVisible ) {
				ChildLoop->internalDraw();
			}

			ChildLoop = ChildLoop->mNext;
		}
	}
}

void UINode::internalDraw() {
	if ( mVisible ) {
		if ( mNodeFlags & NODE_FLAG_POSITION_DIRTY )
			updateScreenPos();

		matrixSet();

		clipMe();

		draw();

		drawChilds();

		clipDisable();

		matrixUnset();
	}
}

void UINode::clipMe() {
	if ( mVisible && ( mFlags & UI_CLIP_ENABLE ) ) {
		if ( mFlags & UI_BORDER )
			UIManager::instance()->clipSmartEnable( this, mScreenPos.x, mScreenPos.y, mRealSize.getWidth(), mRealSize.getHeight() + 1 );
		else
			UIManager::instance()->clipSmartEnable( this, mScreenPos.x, mScreenPos.y, mRealSize.getWidth(), mRealSize.getHeight() );
	}
}

void UINode::clipDisable() {
	if ( mVisible && ( mFlags & UI_CLIP_ENABLE ) ) {
		UIManager::instance()->clipSmartDisable( this );
	}
}

void UINode::matrixSet() {
	if ( getScale() != 1.f || getRotation() != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->pushMatrix();

		Vector2f scaleCenter = getScaleCenter();
		GLi->translatef( scaleCenter.x , scaleCenter.y, 0.f );
		GLi->scalef( getScale().x, getScale().y, 1.0f );
		GLi->translatef( -scaleCenter.x, -scaleCenter.y, 0.f );

		Vector2f rotationCenter = getRotationCenter();
		GLi->translatef( rotationCenter.x , rotationCenter.y, 0.f );
		GLi->rotatef( getRotation(), 0.0f, 0.0f, 1.0f );
		GLi->translatef( -rotationCenter.x, -rotationCenter.y, 0.f );

		//GLi->loadMatrixf( getGlobalTransform().getMatrix() );
	}
}

void UINode::matrixUnset() {
	if ( getScale() != 1.f || getRotation() != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->popMatrix();
	}
}

void UINode::childDeleteAll() {
	while( NULL != mChild ) {
		eeDelete( mChild );
	}
}

void UINode::childAdd( UINode * ChildCtrl ) {
	if ( NULL == mChild ) {
		mChild 		= ChildCtrl;
		mChildLast 	= ChildCtrl;
	} else {
		mChildLast->mNext 		= ChildCtrl;
		ChildCtrl->mPrev		= mChildLast;
		ChildCtrl->mNext		= NULL;
		mChildLast 				= ChildCtrl;
	}

	onChildCountChange();
}

void UINode::childAddAt( UINode * ChildCtrl, Uint32 Pos ) {
	eeASSERT( NULL != ChildCtrl );

	UINode * ChildLoop = mChild;
	
	ChildCtrl->setParent( this );

	childRemove( ChildCtrl );

	ChildCtrl->mParentCtrl = this;
	ChildCtrl->mParentWindowCtrl = ChildCtrl->getParentWindow();
	
	if ( ChildLoop == NULL ) {
		mChild 				= ChildCtrl;
		mChildLast			= ChildCtrl;
		ChildCtrl->mNext 	= NULL;
		ChildCtrl->mPrev 	= NULL;
	} else {
		if( Pos == 0 ) {
			if ( NULL != mChild ) {
				mChild->mPrev		= ChildCtrl;
			}

			ChildCtrl->mNext 	= mChild;
			ChildCtrl->mPrev	= NULL;
			mChild 				= ChildCtrl;
		} else {
			Uint32 i = 0;

			while ( NULL != ChildLoop->mNext && i < Pos ) {
				ChildLoop = ChildLoop->mNext;
				i++;
			}

			UINode * ChildTmp = ChildLoop->mNext;
			ChildLoop->mNext 	= ChildCtrl;
			ChildCtrl->mPrev 	= ChildLoop;
			ChildCtrl->mNext 	= ChildTmp;

			if ( NULL != ChildTmp ) {
				ChildTmp->mPrev = ChildCtrl;
			} else {
				mChildLast		= ChildCtrl;
			}
		}
	}

	onChildCountChange();
}

void UINode::childRemove( UINode * ChildCtrl ) {
	if ( ChildCtrl == mChild ) {
		mChild 			= mChild->mNext;

		if ( NULL != mChild ) {
			mChild->mPrev 	= NULL;

			if ( ChildCtrl == mChildLast )
				mChildLast		= mChild;
		} else {
			mChildLast		= NULL;
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

void UINode::childsCloseAll() {
	UINode * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->close();
		ChildLoop = ChildLoop->mNext;
	}
}

std::string UINode::getId() const {
	return mId;
}

UINode * UINode::setId(const std::string & id) {
	mId = id;
	mIdHash = String::hash( id );
	return this;
}

Uint32 UINode::getIdHash() const {
	return mIdHash;
}

UINode * UINode::findIdHash( const Uint32& idHash ) {
	if ( mIdHash == idHash ) {
		return this;
	} else {
		UINode * child = mChild;

		while ( NULL != child ) {
			UINode * foundCtrl = child->findIdHash( idHash );

			if ( NULL != foundCtrl )
				return foundCtrl;

			child = child->mNext;
		}
	}

	return NULL;
}

UINode * UINode::find( const std::string& id ) {
	return findIdHash( String::hash( id ) );
}

bool UINode::isChild( UINode * ChildCtrl ) const {
	UINode * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildCtrl == ChildLoop )
			return true;

		ChildLoop = ChildLoop->mNext;
	}

	return false;
}

bool UINode::inParentTreeOf( UINode * Child ) const {
	UINode * ParentLoop = Child->mParentCtrl;

	while ( NULL != ParentLoop ) {
		if ( ParentLoop == this )
			return true;

		ParentLoop = ParentLoop->mParentCtrl;
	}

	return false;
}

Uint32 UINode::childCount() const {
	UINode * ChildLoop = mChild;
	Uint32 Count = 0;

	while( NULL != ChildLoop ) {
		ChildLoop = ChildLoop->mNext;
		Count++;
	}

	return Count;
}

UINode * UINode::childAt( Uint32 Index ) const {
	UINode * ChildLoop = mChild;

	while( NULL != ChildLoop && Index ) {
		ChildLoop = ChildLoop->mNext;
		Index--;
	}

	return ChildLoop;
}

UINode * UINode::childPrev( UINode * Ctrl, bool Loop ) const {
	if ( Loop && Ctrl == mChild && NULL != mChild->mNext )
		return mChildLast;

	return Ctrl->mPrev;
}

UINode * UINode::childNext( UINode * Ctrl, bool Loop ) const {
	if ( NULL == Ctrl->mNext && Loop )
		return mChild;

	return Ctrl->mNext;
}

UINode * UINode::getFirstChild() const {
	return mChild;
}

UINode * UINode::getLastChild() const {
	return mChildLast;
}

UINode * UINode::overFind( const Vector2f& Point ) {
	UINode * pOver = NULL;

	if ( mEnabled && mVisible ) {
		if ( mNodeFlags & NODE_FLAG_TRANFORM_DIRTY )
			updateWorldPolygon();

		if ( mPoly.pointInside( Point ) ) {
			writeCtrlFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );

			UINode * ChildLoop = mChildLast;

			while ( NULL != ChildLoop ) {
				UINode * ChildOver = ChildLoop->overFind( Point );

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

void UINode::onParentWindowChange() {
	mParentWindowCtrl = getParentWindow();

	UINode * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->onParentWindowChange();
		ChildLoop = ChildLoop->mNext;
	}
}

UINode * UINode::childGetAt( Vector2i CtrlPos, unsigned int RecursiveLevel ) {
	UINode * Ctrl = NULL;

	for( UINode * pLoop = mChild; NULL != pLoop && NULL == Ctrl; pLoop = pLoop->mNext )
	{
		if ( !pLoop->isVisible() )
			continue;

		if ( pLoop->getRect().contains( CtrlPos ) ) {
			if ( RecursiveLevel )
				Ctrl = childGetAt( CtrlPos - pLoop->getPosition(), RecursiveLevel - 1 );

			if ( NULL == Ctrl )
				Ctrl = pLoop;
		}
	}

	return Ctrl;
}

Uint32 UINode::isWidget() {
	return mNodeFlags & NODE_FLAG_WIDGET;
}

Uint32 UINode::isWindow() {
	return mNodeFlags & NODE_FLAG_WINDOW;
}

Uint32 UINode::isClipped() {
	return mFlags & UI_CLIP_ENABLE;
}

Uint32 UINode::isRotated() {
	return mNodeFlags & NODE_FLAG_ROTATED;
}

Uint32 UINode::isScaled() {
	return mNodeFlags & NODE_FLAG_SCALED;
}

Uint32 UINode::isFrameBuffer() {
	return mNodeFlags & NODE_FLAG_FRAME_BUFFER;
}

bool UINode::isMeOrParentTreeRotated() {
	UINode * Ctrl = this;

	while( NULL != Ctrl ) {
		if ( Ctrl->isRotated() )
			return true;

		Ctrl = Ctrl->getParent();
	}

	return false;
}

bool UINode::isMeOrParentTreeScaled() {
	UINode * Ctrl = this;

	while( NULL != Ctrl ) {
		if ( Ctrl->isScaled() )
			return true;

		Ctrl = Ctrl->getParent();
	}

	return false;
}

bool UINode::isMeOrParentTreeScaledOrRotated() {
	UINode * Ctrl = this;

	while( NULL != Ctrl ) {
		if ( Ctrl->isScaled() || Ctrl->isRotated() )
			return true;

		Ctrl = Ctrl->getParent();
	}

	return false;
}

bool UINode::isMeOrParentTreeScaledOrRotatedOrFrameBuffer() {
	UINode * Ctrl = this;

	while( NULL != Ctrl ) {
		if ( Ctrl->isScaled() || Ctrl->isRotated() || Ctrl->isFrameBuffer() )
			return true;

		Ctrl = Ctrl->getParent();
	}

	return false;
}

Polygon2f& UINode::getPolygon() {
	if ( mNodeFlags & NODE_FLAG_TRANFORM_DIRTY )
		updateWorldPolygon();

	return mPoly;
}

Vector2f UINode::getPolygonCenter() {
	if ( mNodeFlags & NODE_FLAG_TRANFORM_DIRTY )
		updateWorldPolygon();

	return mPoly.getBounds().getCenter();
}

void UINode::updateWorldPolygon() {
	if ( !( mNodeFlags & NODE_FLAG_TRANFORM_DIRTY ) &&
		 !( mNodeFlags & NODE_FLAG_TRANFORM_DIRTY ) ) {
		return;
	}

	if ( mNodeFlags & NODE_FLAG_POSITION_DIRTY ) {
		updateScreenPos();
	}

	mPoly		= Polygon2f( Rectf( mScreenPosf.x, mScreenPosf.y, mScreenPosf.x + mRealSize.getWidth(), mScreenPosf.y + mRealSize.getHeight() ) );

	mPoly.rotate( getRotation(), getRotationCenter() );
	mPoly.scale( getScale(), getScaleCenter() );

	UINode * tParent = getParent();

	while ( tParent ) {
		mPoly.rotate( tParent->getRotation(), tParent->getRotationCenter() );
		mPoly.scale( tParent->getScale(), tParent->getScaleCenter() );

		tParent = tParent->getParent();
	};

	mNodeFlags &= ~NODE_FLAG_TRANFORM_DIRTY | NODE_FLAG_TRANFORM_INVERT_DIRTY;
}

void UINode::updateCenter() {
	mCenter = Vector2f( mScreenPosf.x + (Float)mRealSize.getWidth() * 0.5f, mScreenPosf.y + (Float)mRealSize.getHeight() * 0.5f );
}

Uint32 UINode::addEventListener( const Uint32& EventType, const UIEventCallback& Callback ) {
	mNumCallBacks++;

	mEvents[ EventType ][ mNumCallBacks ] = Callback;

	return mNumCallBacks;
}

void UINode::removeEventListener( const Uint32& CallbackId ) {
	UIEventsMap::iterator it;

	for ( it = mEvents.begin(); it != mEvents.end(); ++it ) {
		std::map<Uint32, UIEventCallback> event = it->second;

		if ( event.erase( CallbackId ) )
			break;
	}
}

void UINode::sendEvent( const UIEvent * Event ) {
	if ( 0 != mEvents.count( Event->getEventType() ) ) {
		std::map<Uint32, UIEventCallback>			event = mEvents[ Event->getEventType() ];
		std::map<Uint32, UIEventCallback>::iterator it;

		if ( event.begin() != event.end() ) {
			for ( it = event.begin(); it != event.end(); ++it )
				it->second( Event );
		}
	}
}

UIBackground * UINode::getBackground() {
	if ( NULL == mBackground ) {
		mBackground = UIBackground::New( this );
	}

	return mBackground;
}

UIBorder * UINode::getBorder() {
	if ( NULL == mBorder ) {
		mBorder = UIBorder::New( this );
	}

	return mBorder;
}

void UINode::setThemeByName( const std::string& Theme ) {
	setTheme( UIThemeManager::instance()->getByName( Theme ) );
}

void UINode::setTheme( UITheme * Theme ) {
	setThemeSkin( Theme, "control" );
}

UINode * UINode::setThemeSkin( const std::string& skinName ) {
	return setThemeSkin( UIThemeManager::instance()->getDefaultTheme(), skinName );
}

UINode * UINode::setThemeSkin( UITheme * Theme, const std::string& skinName ) {
	if ( NULL != Theme ) {
		UISkin * tSkin = Theme->getSkin( skinName );

		if ( NULL != tSkin ) {
			Uint32 InitialState = UISkinState::StateNormal;

			if ( NULL != mSkinState ) {
				InitialState = mSkinState->getState();
			}

			removeSkin();

			mSkinState = UISkinState::New( tSkin );
			mSkinState->setState( InitialState );

			onThemeLoaded();
		}
	}

	return this;
}

UINode * UINode::setSkin( const UISkin& Skin ) {
	removeSkin();

	writeCtrlFlag( NODE_FLAG_SKIN_OWNER, 1 );

	UISkin * SkinCopy = const_cast<UISkin*>( &Skin )->clone();

	mSkinState = UISkinState::New( SkinCopy );

	onThemeLoaded();

	return this;
}

UINode * UINode::setSkin( UISkin * skin ) {
	if ( NULL != skin ) {
		if ( NULL != mSkinState && mSkinState->getSkin() == skin )
			return this;

		Uint32 InitialState = UISkinState::StateNormal;

		if ( NULL != mSkinState ) {
			InitialState = mSkinState->getState();
		}

		removeSkin();

		mSkinState = UISkinState::New( skin );
		mSkinState->setState( InitialState );

		onThemeLoaded();
	}

	return this;
}

void UINode::removeSkin() {
	if ( NULL != mSkinState && ( mNodeFlags & NODE_FLAG_SKIN_OWNER ) ) {
		UISkin * tSkin = mSkinState->getSkin();

		eeSAFE_DELETE( tSkin );
	}

	eeSAFE_DELETE( mSkinState );
}

void UINode::onStateChange() {
	invalidateDraw();
}

void UINode::onParentChange() {
	invalidateDraw();
}

void UINode::onAlignChange() {
	invalidateDraw();
}

void UINode::setSkinState( const Uint32& State ) {
	if ( NULL != mSkinState ) {
		mSkinState->setState( State );

		onStateChange();
	}
}

void UINode::setPrevSkinState() {
	if ( NULL != mSkinState ) {
		mSkinState->setPrevState();

		onStateChange();
	}
}

void UINode::setThemeToChilds( UITheme * Theme ) {
	UINode * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->setThemeToChilds( Theme );
		ChildLoop->setTheme( Theme );	// First set the theme to childs to let the father override the childs forced themes

		ChildLoop = ChildLoop->mNext;
	}
}

void UINode::updateScreenPos() {
	if ( !(mNodeFlags & NODE_FLAG_POSITION_DIRTY) )
		return;

	Vector2i Pos( mRealPos );

	nodeToScreen( Pos );

	mScreenPos = Pos;
	mScreenPosf = Vector2f( Pos.x, Pos.y );

	updateCenter();

	Transformable::setPosition( mScreenPosf.x, mScreenPosf.y );

	mNodeFlags &= ~NODE_FLAG_POSITION_DIRTY;
}

UISkin * UINode::getSkin() {
	if ( NULL != mSkinState )
		return mSkinState->getSkin();

	return NULL;
}

void UINode::writeCtrlFlag( const Uint32& Flag, const Uint32& Val ) {
	BitOp::setBitFlagValue( &mNodeFlags, Flag, Val );
}

void UINode::writeFlag( const Uint32& Flag, const Uint32& Val ) {
	if ( Val )
		mFlags |= Flag;
	else {
		if ( mFlags & Flag )
			mFlags &= ~Flag;
	}
}

void UINode::applyDefaultTheme() {
	UIThemeManager::instance()->applyDefaultTheme( this );
}

Rect UINode::getScreenRect() {
	return Rect( mScreenPos, mRealSize );
}

Rect UINode::makePadding( bool PadLeft, bool PadRight, bool PadTop, bool PadBottom, bool SkipFlags ) {
	Rect tPadding( 0, 0, 0, 0 );

	if ( mFlags & UI_AUTO_PADDING || SkipFlags ) {
		if ( NULL != mSkinState && NULL != mSkinState->getSkin() ) {
			Rect rPadding = mSkinState->getSkin()->getBorderSize( UISkinState::StateNormal );

			if ( PadLeft ) {
				tPadding.Left = rPadding.Left;
			}

			if ( PadRight ) {
				tPadding.Right = rPadding.Right;
			}

			if ( PadTop ) {
				tPadding.Top = rPadding.Top;
			}

			if ( PadBottom ) {
				tPadding.Bottom = rPadding.Bottom;
			}
		}
	}

	return tPadding;
}

void UINode::setFocus() {
	UIManager::instance()->setFocusControl( this );
}

void UINode::sendParentSizeChange( const Vector2i& SizeChange ) {
	if ( mFlags & UI_REPORT_SIZE_CHANGE_TO_CHILDS )	{
		UINode * ChildLoop = mChild;

		while( NULL != ChildLoop ) {
			ChildLoop->onParentSizeChange( SizeChange );
			ChildLoop = ChildLoop->mNext;
		}
	}
}

void UINode::onParentSizeChange( const Vector2i& SizeChange ) {
	sendCommonEvent( UIEvent::OnParentSizeChange );
	invalidateDraw();
}

Sizei UINode::getSkinSize( UISkin * Skin, const Uint32& State ) {
	if ( NULL != Skin ) {
		return Skin->getSize( State );
	}

	return Sizei::Zero;
}

Sizei UINode::getSkinSize() {
	if ( NULL != getSkin() ) {
		return getSkin()->getSize();
	}

	return Sizei::Zero;
}

UINode * UINode::getNextWidget() {
	UINode * Found		= NULL;
	UINode * ChildLoop	= mChild;

	while( NULL != ChildLoop ) {
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

		ChildLoop = ChildLoop->mNext;
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
			if ( NULL != ChildLoop->mNext ) {
				if ( ChildLoop->mNext->isVisible() && ChildLoop->mNext->isEnabled() && ChildLoop->mNext->isWidget() ) {
					return ChildLoop->mNext;
				} else {
					return ChildLoop->mNext->getNextWidget();
				}
			}

			ChildLoop = ChildLoop->mParentCtrl;
		}
	}

	return UIManager::instance()->getMainControl();
}

void UINode::onThemeLoaded() {
	invalidateDraw();
}

void UINode::onChildCountChange() {
	invalidateDraw();
}

void UINode::worldToNode( Vector2i& pos ) {
	Vector2f PosTest( convertToNodeSpace( Vector2f( pos.x, pos.y ) ) );

	Vector2f Pos( pos.x, pos.y );

	std::list<UINode*> parents;

	UINode * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		parents.push_front( ParentLoop );
		ParentLoop = ParentLoop->getParent();
	}

	parents.push_back( const_cast<UINode*>( reinterpret_cast<const UINode*>( this ) ) );

	Vector2f scale(1,1);

	for ( std::list<UINode*>::iterator it = parents.begin(); it != parents.end(); it++ ) {
		UINode * tParent = (*it);
		Vector2f pPos( tParent->mRealPos.x * scale.x			, tParent->mRealPos.y * scale.y			);
		Vector2f Center;

		if ( NULL != tParent && 1.f != tParent->getScale() ) {
			Center = tParent->getScaleOriginPoint() * scale;
			scale *= tParent->getScale();

			pPos.scale( scale, pPos + Center );
		}

		Pos -= pPos;

		if ( NULL != tParent && 0.f != tParent->getRotation() ) {
			Center = tParent->getRotationOriginPoint() * scale;
			Pos.rotate( -tParent->getRotation(), Center );
		}
	}

	Pos = Vector2f( Pos.x / scale.x, Pos.y / scale.y );
	pos = Vector2i( Pos.x / PixelDensity::getPixelDensity(), Pos.y / PixelDensity::getPixelDensity() );
}

void UINode::nodeToWorld( Vector2i& pos ) {
	Vector2f Pos( (Float)pos.x * PixelDensity::getPixelDensity(), (Float)pos.y * PixelDensity::getPixelDensity() );

	std::list<UINode*> parents;

	UINode * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		parents.push_back( ParentLoop );
		ParentLoop = ParentLoop->getParent();
	}

	parents.push_front( const_cast<UINode*>( reinterpret_cast<const UINode*>( this ) ) );

	for ( std::list<UINode*>::iterator it = parents.begin(); it != parents.end(); it++ ) {
		UINode * tParent = (*it);
		Vector2f pPos( tParent->mRealPos.x					, tParent->mRealPos.y					);

		Pos += pPos;

		Vector2f CenterAngle( pPos.x + tParent->mRotationOriginPoint.x, pPos.y + tParent->mRotationOriginPoint.y );
		Vector2f CenterScale( pPos.x + tParent->mScaleOriginPoint.x, pPos.y + tParent->mScaleOriginPoint.y );

		Pos.rotate( tParent->getRotation(), CenterAngle );
		Pos.scale( tParent->getScale(), CenterScale );
	}

	pos = Vector2i( eeceil( Pos.x ), eeceil( Pos.y ) );
}

UINode * UINode::getWindowContainer() {
	UINode * Ctrl = this;

	while ( Ctrl != NULL ) {
		if ( Ctrl->isType( UI_TYPE_WINDOW ) ) {
			if ( UIManager::instance()->getMainControl() == Ctrl ) {
				return Ctrl;
			} else {
				return static_cast<UIWindow*>( Ctrl )->getContainer();
			}
		}

		Ctrl = Ctrl->getParent();
	}

	return NULL;
}

UIWindow * UINode::getParentWindow() {
	UINode * Ctrl = mParentCtrl;

	while ( Ctrl != NULL ) {
		if ( Ctrl->isType( UI_TYPE_WINDOW ) )
			return static_cast<UIWindow*>( Ctrl );

		Ctrl = Ctrl->getParent();
	}

	return NULL;
}

bool UINode::isReverseDraw() const {
	return 0 != ( mNodeFlags & NODE_FLAG_REVERSE_DRAW );
}

void UINode::setReverseDraw( bool reverseDraw ) {
	writeCtrlFlag( NODE_FLAG_REVERSE_DRAW, reverseDraw ? 1 : 0 );
	invalidateDraw();
}

void UINode::invalidateDraw() {
	if ( NULL != mParentWindowCtrl ) {
		mParentWindowCtrl->invalidate();
	} else if ( NULL == mParentCtrl && isWindow() ) {
		static_cast<UIWindow*>( this )->invalidate();
	}
}

void UINode::setClipEnabled() {
	writeFlag( UI_CLIP_ENABLE, 1 );
}

void UINode::setClipDisabled() {
	writeFlag( UI_CLIP_ENABLE, 0 );
}

UIWindow * UINode::getOwnerWindow() {
	return mParentWindowCtrl;
}

const Vector2i& UINode::getDragPoint() const {
	return mDragPoint;
}

void UINode::setDragPoint( const Vector2i& Point ) {
	mDragPoint = Point;
}

Uint32 UINode::onDrag( const Vector2i& Pos ) {
	return 1;
}

Uint32 UINode::onDragStart( const Vector2i& Pos ) {
	sendCommonEvent( UIEvent::OnDragStart );
	return 1;
}

Uint32 UINode::onDragStop( const Vector2i& Pos ) {
	sendCommonEvent( UIEvent::OnDragStop );
	return 1;
}

bool UINode::isDragEnabled() const {
	return 0 != ( mFlags & UI_DRAG_ENABLE );
}

void UINode::setDragEnabled( const bool& enable ) {
	writeFlag( UI_DRAG_ENABLE, true == enable );
}

bool UINode::isDragging() const {
	return 0 != ( mNodeFlags & NODE_FLAG_DRAGGING );
}

void UINode::setDragging( const bool& dragging ) {
	writeCtrlFlag( NODE_FLAG_DRAGGING, dragging );

	if ( dragging ) {
		UIMessage tMsg( this, UIMessage::DragStart, 0 );
		messagePost( &tMsg );

		onDragStart( UIManager::instance()->getMousePos() );
	} else {
		UIMessage tMsg( this, UIMessage::DragStop, 0 );
		messagePost( &tMsg );

		onDragStop( UIManager::instance()->getMousePos() );
	}
}

void UINode::setDragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& UINode::getDragButton() const {
	return mDragButton;
}

void UINode::updateOriginPoint() {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			mRotationOriginPoint.x = mRealSize.x * 0.5f;
			mRotationOriginPoint.y = mRealSize.y * 0.5f;
			break;
		case OriginPoint::OriginTopLeft:
			mRotationOriginPoint.x = mRotationOriginPoint.y = 0;
			break;
		default: {}
	}

	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			mScaleOriginPoint.x = mRealSize.x * 0.5f;
			mScaleOriginPoint.y = mRealSize.y * 0.5f;
			break;
		case OriginPoint::OriginTopLeft:
			mScaleOriginPoint.x = mScaleOriginPoint.y = 0;
			break;
		default: {}
	}

	setDirty();
}

void UINode::setDirty() {
	if ( ( mNodeFlags & NODE_FLAG_POSITION_DIRTY ) &&
		 ( mNodeFlags & NODE_FLAG_TRANFORM_DIRTY ) &&
		 ( mNodeFlags & NODE_FLAG_TRANFORM_INVERT_DIRTY ) )
	{
		return;
	}

	mNodeFlags |= NODE_FLAG_POSITION_DIRTY | NODE_FLAG_TRANFORM_DIRTY | NODE_FLAG_TRANFORM_INVERT_DIRTY;

	setChildsDirty();
}

void UINode::setChildsDirty() {
	UINode * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->setDirty();

		ChildLoop = ChildLoop->mNext;
	}
}

void UINode::onAngleChange() {
	sendCommonEvent( UIEvent::OnAngleChange );
	invalidateDraw();
}

void UINode::onScaleChange() {
	sendCommonEvent( UIEvent::OnScaleChange );
	invalidateDraw();
}

void UINode::onAlphaChange() {
	sendCommonEvent( UIEvent::OnAlphaChange );
	invalidateDraw();
}

Color UINode::getColor( const Color& Col ) {
	return Color( Col.r, Col.g, Col.b, static_cast<Uint8>( (Float)Col.a * ( mAlpha / 255.f ) ) );
}

bool UINode::isFadingOut() {
	return 0 != ( mNodeFlags & NODE_FLAG_DISABLE_DELAYED );
}

bool UINode::isAnimating() {
	return NULL != mActionManager && !mActionManager->isEmpty();
}

static void UINode_onFadeDone( UIAction * action, const UIAction::ActionType& actionType ) {
	UINode * node = action->getTarget();

	if ( NULL != node ) {
		if ( ( node->getNodeFlags() & NODE_FLAG_CLOSE_DELAYED )  )
			node->close();

		if ( ( node->getNodeFlags() & NODE_FLAG_DISABLE_DELAYED ) ) {
			node->setNodeFlags( node->getNodeFlags() & ~NODE_FLAG_DISABLE_DELAYED );

			node->setVisible( false );
		}
	}
}

Interpolation1d * UINode::startAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& AlphaChilds, const Ease::Interpolation& Type, Interpolation1d::OnPathEndCallback PathEndCallback ) {
	Action::Fade * action = Action::Fade::New( From, To, TotalTime, Type );

	action->getInterpolation()->setPathEndCallback( PathEndCallback );

	action->addEventListener( UIAction::ActionType::OnDone, cb::Make2( &UINode_onFadeDone ) );

	runAction( action );

	return action->getInterpolation();
}

Interpolation2d * UINode::startScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation2d::OnPathEndCallback PathEndCallback ) {
	Action::Scale * action = Action::Scale::New( From, To, TotalTime, Type );

	action->getInterpolation()->setPathEndCallback( PathEndCallback );

	runAction( action );

	return action->getInterpolation();
}

Interpolation2d * UINode::startScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation2d::OnPathEndCallback PathEndCallback ) {
	return startScaleAnim( Vector2f( From, From ), Vector2f( To, To ), TotalTime, Type, PathEndCallback );
}

Interpolation2d * UINode::startTranslation( const Vector2i& From, const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation2d::OnPathEndCallback PathEndCallback ) {
	Action::Move * action = Action::Move::New( Vector2f( From.x, From.y ), Vector2f( To.x, To.y ), TotalTime, Type );

	action->getInterpolation()->setPathEndCallback( PathEndCallback );

	runAction( action );

	return action->getInterpolation();
}

Interpolation1d * UINode::startRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation1d::OnPathEndCallback PathEndCallback ) {
	Action::Rotate * action = Action::Rotate::New( From, To, TotalTime, Type );

	action->getInterpolation()->setPathEndCallback( PathEndCallback );

	runAction( action );

	return action->getInterpolation();
}

Interpolation1d * UINode::startAlphaAnim(const Float & To, const Time & TotalTime, const bool & alphaChilds, const Ease::Interpolation & type, Interpolation1d::OnPathEndCallback PathEndCallback) {
	return startAlphaAnim( mAlpha, To, TotalTime, alphaChilds, type, PathEndCallback );
}

Interpolation2d * UINode::startScaleAnim(const Vector2f & To, const Time & TotalTime, const Ease::Interpolation & type, Interpolation2d::OnPathEndCallback PathEndCallback) {
	return startScaleAnim( getScale(), To, TotalTime, type, PathEndCallback );
}

Interpolation2d * UINode::startScaleAnim(const Float & To, const Time & TotalTime, const Ease::Interpolation & type, Interpolation2d::OnPathEndCallback PathEndCallback) {
	return startScaleAnim( getScale(), Vector2f(To,To), TotalTime, type, PathEndCallback );
}

Interpolation2d * UINode::startTranslation(const Vector2i & To, const Time & TotalTime, const Ease::Interpolation & type, Interpolation2d::OnPathEndCallback PathEndCallback) {
	return startTranslation( mPos, To, TotalTime, type, PathEndCallback );
}

Interpolation1d * UINode::startRotation(const Float & To, const Time & TotalTime, const Ease::Interpolation & type, Interpolation1d::OnPathEndCallback PathEndCallback) {
	return startRotation( getRotation(), To, TotalTime, type, PathEndCallback );
}

Interpolation1d * UINode::createFadeIn( const Time& time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	return startAlphaAnim( mAlpha, 255.f, time, AlphaChilds, Type );
}

Interpolation1d * UINode::createFadeOut( const Time& time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	return startAlphaAnim( 255.f, mAlpha, time, AlphaChilds, Type );
}

Interpolation1d * UINode::closeFadeOut( const Time& time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	mNodeFlags |= NODE_FLAG_CLOSE_DELAYED;

	return startAlphaAnim( mAlpha, 0.f, time, AlphaChilds, Type );
}

Interpolation1d * UINode::disableFadeOut( const Time& time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	setEnabled( false );

	mNodeFlags |= NODE_FLAG_DISABLE_DELAYED;

	return startAlphaAnim( mAlpha, 0.f, time, AlphaChilds, Type );
}

const OriginPoint& UINode::getRotationOriginPoint() const {
	return mRotationOriginPoint;
}

void UINode::setRotationOriginPoint( const OriginPoint & center ) {
	mRotationOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
	Transformable::setRotationOrigin( getRotationCenter().x, getRotationCenter().y );
}

Vector2f UINode::getRotationCenter() {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter: return mCenter;
		case OriginPoint::OriginTopLeft: return mScreenPosf;
		case OriginPoint::OriginCustom: default: return mScreenPosf + mRotationOriginPoint;
	}
}

void UINode::setRotation( float angle ) {
	Transformable::setRotation( angle );

	updateOriginPoint();
	Transformable::setRotationOrigin( getRotationCenter().x, getRotationCenter().y );

	if ( getRotation() != 0.f ) {
		mNodeFlags |= NODE_FLAG_ROTATED;
	} else {
		if ( mNodeFlags & NODE_FLAG_ROTATED )
			mNodeFlags &= ~NODE_FLAG_ROTATED;
	}

	setDirty();

	onAngleChange();
}

void UINode::setRotation( const Float& angle , const OriginPoint & center ) {
	mRotationOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
	setRotation( angle );
}

void UINode::setScale( const Vector2f & scale ) {
	Transformable::setScale( scale.x, scale.y );

	updateOriginPoint();
	Transformable::setScaleOrigin( getScaleCenter().x, getScaleCenter().y );

	if ( getScale() != 1.f ) {
		mNodeFlags |= NODE_FLAG_SCALED;
	} else {
		if ( mNodeFlags & NODE_FLAG_SCALED )
			mNodeFlags &= ~NODE_FLAG_SCALED;
	}

	setDirty();

	onScaleChange();
}

const OriginPoint& UINode::getScaleOriginPoint() const {
	return mScaleOriginPoint;
}

void UINode::setScaleOriginPoint( const OriginPoint & center ) {
	mScaleOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
	Transformable::setScaleOrigin( getScaleCenter().x, getScaleCenter().y );
}

Vector2f UINode::getScaleCenter() {
	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter: return mCenter;
		case OriginPoint::OriginTopLeft: return mScreenPosf;
		case OriginPoint::OriginCustom: default: return mScreenPosf + mScaleOriginPoint;
	}
}

void UINode::setScale( const Vector2f& scale, const OriginPoint& center ) {
	mScaleOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
	Transformable::setScale( scale.x, scale.y );
	Transformable::setScaleOrigin( getScaleCenter().x, getScaleCenter().y );
}

void UINode::setScale( const Float& scale, const OriginPoint& center ) {
	setScale( Vector2f( scale, scale ), center );
}

const Float& UINode::getAlpha() const {
	return mAlpha;
}

void UINode::setAlpha( const Float& alpha ) {
	mAlpha = alpha;
	onAlphaChange();
}

void UINode::setChildsAlpha( const Float &alpha ) {
	UINode * CurChild = mChild;

	while ( NULL != CurChild ) {
		CurChild->setAlpha( alpha );
		CurChild->setChildsAlpha( alpha );
		CurChild = CurChild->getNextNode();
	}
}

UIActionManager * UINode::getActionManager() {
	if ( NULL == mActionManager )
		mActionManager = eeNew( UIActionManager, () );

	return mActionManager;
}

void UINode::runAction( UIAction * action ) {
	if ( NULL != action ) {
		action->setTarget( this );

		action->start();

		getActionManager()->addAction( action );
	}
}

Transform UINode::getLocalTransform() {
	return getTransform();
}

Transform UINode::getGlobalTransform() {
	return NULL != mParentCtrl ? mParentCtrl->getGlobalTransform() * getTransform() : getTransform();
}

Transform UINode::getNodeToWorldTransform() {
	return getGlobalTransform();
}

Transform UINode::getWorldToNodeTransform() {
	return getNodeToWorldTransform().getInverse();
}

Vector2f UINode::convertToNodeSpace(const Vector2f& worldPoint) {
	return getWorldToNodeTransform().transformPoint(worldPoint.x, worldPoint.y);
}

Vector2f UINode::convertToWorldSpace(const Vector2f& nodePoint) {
	return getNodeToWorldTransform().transformPoint(nodePoint.x, nodePoint.y);
}

void UINode::setPosition(float x, float y) {
	setPosition( Vector2f( x, y ) );
}

void UINode::setScale(float factorX, float factorY) {
	setScale( Vector2f( factorX, factorY ) );
}

void UINode::setScaleOrigin(float x, float y) {
	setScaleOriginPoint( OriginPoint( x, y ) );
}

void UINode::setRotationOrigin(float x, float y) {
	setRotationOriginPoint( OriginPoint( x, y ) );
}


}}
