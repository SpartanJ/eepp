#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/window/engine.hpp>

#include <eepp/scene/actions/fade.hpp>
#include <eepp/scene/actions/scale.hpp>
#include <eepp/scene/actions/rotate.hpp>
#include <eepp/scene/actions/move.hpp>

namespace EE { namespace UI {

UINode * UINode::New() {
	return eeNew( UINode, () );
}

UINode::UINode() :
	Node(),
	mFlags( UI_CONTROL_DEFAULT_FLAGS ),
	mSkinState( NULL ),
	mBackground( NULL ),
	mBorder( NULL ),
	mDragButton( EE_BUTTON_LMASK )
{
	mNodeFlags |= NODE_FLAG_UINODE;
}

UINode::~UINode() {
	removeSkin();

	eeSAFE_DELETE( mBackground );
	eeSAFE_DELETE( mBorder );
}

void UINode::worldToNodeTranslation( Vector2f& Pos ) const {
	Node * ParentLoop = mParentCtrl;

	Pos -= mPosition;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->isUINode() ? static_cast<UINode*>( ParentLoop )->getRealPosition() : ParentLoop->getPosition();

		Pos -= ParentPos;

		ParentLoop = ParentLoop->getParent();
	}
}

void UINode::nodeToWorldTranslation( Vector2f& Pos ) const {
	Node * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->isUINode() ? static_cast<UINode*>( ParentLoop )->getRealPosition() : ParentLoop->getPosition();

		Pos += ParentPos;

		ParentLoop = ParentLoop->getParent();
	}
}

Uint32 UINode::getType() const {
	return UI_TYPE_UINODE;
}

bool UINode::isType( const Uint32& type ) const {
	return UINode::getType() == type || Node::isType( type );
}

void UINode::setInternalPosition( const Vector2f& Pos ) {
	mDpPos = Pos;
	Transformable::setPosition( PixelDensity::dpToPx( Pos ) );
	setDirty();
}

void UINode::setPosition( const Vector2f& Pos ) {
	if ( Pos != mDpPos ) {
		setInternalPosition( Pos );
		onPositionChange();
	}
}

Node * UINode::setPosition( const Float& x, const Float& y ) {
	setPosition( Vector2f( x, y ) );
	return this;
}

void UINode::setPixelsPosition( const Vector2f& Pos ) {
	if ( mPosition != Pos ) {
		mDpPos = PixelDensity::pxToDp( Pos );
		Transformable::setPosition( Pos );
		onPositionChange();
	}
}

void UINode::setPixelsPosition( const Float& x, const Float& y ) {
	setPixelsPosition( Vector2f( x, y ) );
}

const Vector2f& UINode::getPosition() const {
	return mDpPos;
}

const Vector2f &UINode::getRealPosition() const {
	return mPosition;
}

void UINode::setInternalSize( const Sizef& size ) {
	mDpSize = size;
	mSize = PixelDensity::dpToPx( size );
	updateCenter();
	sendCommonEvent( UIEvent::OnSizeChange );
	invalidateDraw();
}

void UINode::setInternalPixelsSize( const Sizef& size ) {
	mDpSize = PixelDensity::pxToDp( size );
	mSize = size;
	updateCenter();
	sendCommonEvent( UIEvent::OnSizeChange );
	invalidateDraw();
}

Node * UINode::setSize( const Sizef & Size ) {
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

Node * UINode::setSize(const Float & Width, const Float & Height) {
	return setSize( Vector2f( Width, Height ) );
}

UINode * UINode::setPixelsSize( const Sizef& size ) {
	if ( size != mSize ) {
		Vector2f sizeChange( size.x - mSize.x, size.y - mSize.y );

		setInternalPixelsSize( size );

		onSizeChange();

		if ( reportSizeChangeToChilds() ) {
			sendParentSizeChange( PixelDensity::pxToDp( sizeChange ) );
		}
	}

	return this;
}

UINode * UINode::setPixelsSize(const Float & x, const Float & y ) {
	return setPixelsSize( Sizef( x, y ) );
}

void UINode::setInternalPixelsWidth( const Float& width ) {
	setInternalPixelsSize( Sizef( width, mSize.y ) );
}

void UINode::setInternalPixelsHeight( const Float& height ) {
	setInternalPixelsSize( Sizef( mSize.x, height ) );
}

Rect UINode::getRect() const {
	return Rect( Vector2i( mDpPos.x, mDpPos.y ), Sizei( mDpSize.x, mDpSize.y ) );
}

const Sizef& UINode::getSize() {
	return mDpSize;
}

const Sizef& UINode::getRealSize() {
	return mSize;
}

void UINode::drawHighlightFocus() {
	if ( UIManager::instance()->getHighlightFocus() && UIManager::instance()->getFocusControl() == this ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( UIManager::instance()->getHighlightFocusColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getScreenBounds() );
	}
}

void UINode::drawOverNode() {
	if ( UIManager::instance()->getHighlightOver() && UIManager::instance()->getOverControl() == this ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( UIManager::instance()->getHighlightOverColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getScreenBounds() );
	}
}

void UINode::drawDebugData() {
	if ( UIManager::instance()->getDrawDebugData() ) {
		if ( isWidget() ) {
			UIWidget * me = static_cast<UIWidget*>( this );

			if ( UIManager::instance()->getOverControl() == this ) {
				String text( String::strFormated( "X: %2.4f Y: %2.4f\nW: %2.4f H: %2.4f", mDpPos.x, mDpPos.y, mDpSize.x, mDpSize.y ) );

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
		P.drawRectangle( getScreenBounds() );
	}
}

void UINode::drawSkin() {
	if ( NULL != mSkinState ) {
		if ( mFlags & UI_SKIN_KEEP_SIZE_ON_DRAW ) {
			Sizef rSize = PixelDensity::dpToPx( mSkinState->getSkin()->getSize( mSkinState->getState() ) );
			Sizef diff = ( mSize - rSize ) * 0.5f;

			mSkinState->draw( mScreenPosi.x + eefloor(diff.x), mScreenPosi.y + eefloor(diff.y), eefloor(rSize.getWidth()), eefloor(rSize.getHeight()), (Uint32)mAlpha );
		} else {
			mSkinState->draw( mScreenPosi.x, mScreenPosi.y, eefloor(mSize.getWidth()), eefloor(mSize.getHeight()), (Uint32)mAlpha );
		}
	}
}

void UINode::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		drawBackground();

		drawSkin();
	}
}

void UINode::update( const Time& time ) {
	if ( isDragEnabled() && isDragging() ) {
		if ( !( UIManager::instance()->getPressTrigger() & mDragButton ) ) {
			setDragging( false );
			UIManager::instance()->setControlDragging( false );
			return;
		}

		Vector2f Pos( UIManager::instance()->getMousePosf() );

		if ( mDragPoint != Pos && ( abs( mDragPoint.x - Pos.x ) > 1 || abs( mDragPoint.y - Pos.y ) > 1 ) ) {
			if ( onDrag( Pos ) ) {
				Sizef dragDiff;

				dragDiff.x = (Float)( mDragPoint.x - Pos.x );
				dragDiff.y = (Float)( mDragPoint.y - Pos.y );

				setPixelsPosition( mPosition - dragDiff );

				mDragPoint = Pos;

				onPositionChange();

				UIManager::instance()->setControlDragging( true );
			}
		}
	}

	Node::update( time );
}

Uint32 UINode::onMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( !( UIManager::instance()->getLastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && isDragEnabled() && !isDragging() ) {
		setDragging( true );
		mDragPoint = Vector2f( Pos.x, Pos.y );
	}

	setSkinState( UISkinState::StateMouseDown );

	return Node::onMouseDown( Pos, Flags );
}

Uint32 UINode::onMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isDragEnabled() && isDragging() && ( Flags & mDragButton ) ) {
		setDragging( false );
	}

	setPrevSkinState();

	return Node::onMouseUp( Pos, Flags );
}

Uint32 UINode::onValueChange() {
	sendCommonEvent( UIEvent::OnValueChange );
	invalidateDraw();
	return 1;
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

void UINode::drawBackground() {
	if ( mFlags & UI_FILL_BACKGROUND ) {
		mBackground->draw( getScreenBounds(), mAlpha );
	}
}

void UINode::drawBorder() {
	if ( mFlags & UI_BORDER ) {
		mBorder->draw( getScreenBounds(), mAlpha, mBackground->getCorners() );
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

		drawBorder();

		drawHighlightFocus();

		drawOverNode();

		drawDebugData();

		drawBox();

		matrixUnset();
	}
}

void UINode::clipMe() {
	if ( mVisible && ( mFlags & UI_CLIP_ENABLE ) ) {
		UIManager::instance()->clipSmartEnable( this, mScreenPos.x, mScreenPos.y, mSize.getWidth(), mSize.getHeight() );
	}
}

void UINode::clipDisable() {
	if ( mVisible && ( mFlags & UI_CLIP_ENABLE ) ) {
		UIManager::instance()->clipSmartDisable( this );
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
	Node * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isUINode() ) {
			UINode * node = static_cast<UINode*>( ChildLoop );
			node->setThemeToChilds( Theme );
			node->setTheme( Theme );	// First set the theme to childs to let the father override the childs forced themes
		}

		ChildLoop = ChildLoop->getNextNode();
	}
}

UISkin * UINode::getSkin() {
	if ( NULL != mSkinState )
		return mSkinState->getSkin();

	return NULL;
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

Rectf UINode::makePadding( bool PadLeft, bool PadRight, bool PadTop, bool PadBottom, bool SkipFlags ) {
	Rectf tPadding( 0, 0, 0, 0 );

	if ( mFlags & UI_AUTO_PADDING || SkipFlags ) {
		if ( NULL != mSkinState && NULL != mSkinState->getSkin() ) {
			Rectf rPadding = mSkinState->getSkin()->getBorderSize( UISkinState::StateNormal );

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

Sizef UINode::getSkinSize( UISkin * Skin, const Uint32& State ) {
	if ( NULL != Skin ) {
		return Skin->getSize( State );
	}

	return Sizef::Zero;
}

Sizef UINode::getSkinSize() {
	if ( NULL != getSkin() ) {
		return getSkin()->getSize();
	}

	return Sizef::Zero;
}

void UINode::onThemeLoaded() {
	invalidateDraw();
}

void UINode::onChildCountChange() {
	invalidateDraw();
}

void UINode::worldToNode( Vector2i& pos ) {
	Vector2f toPos( convertToNodeSpace( Vector2f( pos.x, pos.y ) ) );
	pos = Vector2i( toPos.x  / PixelDensity::getPixelDensity(), toPos.y / PixelDensity::getPixelDensity() );
}

void UINode::nodeToWorld( Vector2i& pos ) {
	Vector2f toPos( convertToWorldSpace( Vector2f( pos.x * PixelDensity::getPixelDensity(), pos.y * PixelDensity::getPixelDensity() ) ) );
	pos = Vector2i( toPos.x, toPos.y );
}

void UINode::worldToNode( Vector2f& pos ) {
	Vector2f toPos( convertToNodeSpace( pos ) );
	pos = Vector2f( toPos.x  / PixelDensity::getPixelDensity(), toPos.y / PixelDensity::getPixelDensity() );
}

void UINode::nodeToWorld( Vector2f& pos ) {
	Vector2f toPos( convertToWorldSpace( Vector2f( pos.x * PixelDensity::getPixelDensity(), pos.y * PixelDensity::getPixelDensity() ) ) );
	pos = Vector2f( toPos.x, toPos.y );
}

Node * UINode::getWindowContainer() {
	Node * Ctrl = this;

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

void UINode::setClipEnabled() {
	writeFlag( UI_CLIP_ENABLE, 1 );
}

void UINode::setClipDisabled() {
	writeFlag( UI_CLIP_ENABLE, 0 );
}

const Vector2f& UINode::getDragPoint() const {
	return mDragPoint;
}

void UINode::setDragPoint( const Vector2f& Point ) {
	mDragPoint = Point;
}

Uint32 UINode::onDrag( const Vector2f& Pos ) {
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

Uint32 UINode::onMouseEnter(const Vector2i & position, const Uint32 flags) {
	setSkinState( UISkinState::StateMouseEnter );

	return Node::onMouseEnter( position, flags );
}

Uint32 UINode::onMouseExit(const Vector2i & position, const Uint32 flags) {
	setSkinState( UISkinState::StateMouseExit );

	return Node::onMouseExit( position, flags );
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

bool UINode::isFadingOut() {
	return 0 != ( mNodeFlags & NODE_FLAG_DISABLE_DELAYED );
}

bool UINode::isAnimating() {
	return NULL != mActionManager && !mActionManager->isEmpty();
}

static void UINode_onFadeDone( Action * action, const Action::ActionType& actionType ) {
	Node * node = action->getTarget();

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
	Actions::Fade * action = Actions::Fade::New( From, To, TotalTime, Type );

	action->getInterpolation()->setPathEndCallback( PathEndCallback );

	action->addEventListener( Action::ActionType::OnDone, cb::Make2( &UINode_onFadeDone ) );

	runAction( action );

	return action->getInterpolation();
}

Interpolation2d * UINode::startScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation2d::OnPathEndCallback PathEndCallback ) {
	Actions::Scale * action = Actions::Scale::New( From, To, TotalTime, Type );

	action->getInterpolation()->setPathEndCallback( PathEndCallback );

	runAction( action );

	return action->getInterpolation();
}

Interpolation2d * UINode::startScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation2d::OnPathEndCallback PathEndCallback ) {
	return startScaleAnim( Vector2f( From, From ), Vector2f( To, To ), TotalTime, Type, PathEndCallback );
}

Interpolation2d * UINode::startTranslation( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation2d::OnPathEndCallback PathEndCallback ) {
	Actions::Move * action = Actions::Move::New( From, To, TotalTime, Type );

	action->getInterpolation()->setPathEndCallback( PathEndCallback );

	runAction( action );

	return action->getInterpolation();
}

Interpolation1d * UINode::startRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation1d::OnPathEndCallback PathEndCallback ) {
	Actions::Rotate * action = Actions::Rotate::New( From, To, TotalTime, Type );

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

Interpolation2d * UINode::startTranslation(const Vector2f& To, const Time & TotalTime, const Ease::Interpolation & type, Interpolation2d::OnPathEndCallback PathEndCallback) {
	return startTranslation( mDpPos, To, TotalTime, type, PathEndCallback );
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

void UINode::onWidgetFocusLoss() {
	sendCommonEvent( UIEvent::OnWidgetFocusLoss );
	invalidateDraw();
}

void UINode::setFocus() {
	UIManager::instance()->setFocusControl( this );
}

Uint32 UINode::onFocus() {
	setSkinState( UISkinState::StateFocus );

	return Node::onFocus();
}

}}
