#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uistate.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/fade.hpp>
#include <eepp/scene/actions/scale.hpp>
#include <eepp/scene/actions/rotate.hpp>
#include <eepp/scene/actions/move.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uiskinstate.hpp>

namespace EE { namespace UI {

UINode * UINode::New() {
	return eeNew( UINode, () );
}

UINode::UINode() :
	Node(),
	mFlags( UI_CONTROL_DEFAULT_FLAGS ),
	mState(UIState::StateFlagNormal),
	mSkinState( NULL ),
	mBackgroundState( NULL ),
	mForegroundState( NULL ),
	mBorder( NULL ),
	mDragButton( EE_BUTTON_LMASK ),
	mSkinColor( Color::White )
{
	mNodeFlags |= NODE_FLAG_UINODE | NODE_FLAG_OVER_FIND_ALLOWED;

	if ( NULL != SceneManager::instance()->getUISceneNode() )
		setParent( (Node*)SceneManager::instance()->getUISceneNode() );
}

UINode::~UINode() {
	removeSkin();

	if ( NULL != mBackgroundState && NULL != mBackgroundState->getSkin() )
		eeDelete( mBackgroundState->getSkin() );

	if ( NULL != mForegroundState && NULL != mForegroundState->getSkin() )
		eeDelete( mForegroundState->getSkin() );

	eeSAFE_DELETE( mBackgroundState );
	eeSAFE_DELETE( mForegroundState );
	eeSAFE_DELETE( mBorder );

	if ( isDragging() )
		getEventDispatcher()->setNodeDragging( NULL );
}

void UINode::worldToNodeTranslation( Vector2f& Pos ) const {
	Node * ParentLoop = mParentCtrl;

	Pos -= mPosition;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->isUINode() ? static_cast<UINode*>( ParentLoop )->getPixelsPosition() : ParentLoop->getPosition();

		Pos -= ParentPos;

		ParentLoop = ParentLoop->getParent();
	}
}

void UINode::nodeToWorldTranslation( Vector2f& Pos ) const {
	Node * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->isUINode() ? static_cast<UINode*>( ParentLoop )->getPixelsPosition() : ParentLoop->getPosition();

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
		setDirty();
		onPositionChange();
	}
}

void UINode::setPixelsPosition( const Float& x, const Float& y ) {
	setPixelsPosition( Vector2f( x, y ) );
}

const Vector2f& UINode::getPosition() const {
	return mDpPos;
}

const Vector2f &UINode::getPixelsPosition() const {
	return mPosition;
}

void UINode::setInternalSize( const Sizef& size ) {
	mDpSize = size;
	mSize = PixelDensity::dpToPx( size );
	updateCenter();
	sendCommonEvent( Event::OnSizeChange );
	invalidateDraw();
}

void UINode::setInternalPixelsSize( const Sizef& size ) {
	mDpSize = PixelDensity::pxToDp( size );
	mSize = size;
	updateCenter();
	sendCommonEvent( Event::OnSizeChange );
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

const Sizef& UINode::getSize() const {
	return mDpSize;
}

void UINode::drawHighlightFocus() {
	if ( NULL != getEventDispatcher() && mSceneNode->getHighlightFocus() && getEventDispatcher()->getFocusControl() == this ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( mSceneNode->getHighlightFocusColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getScreenBounds() );
	}
}

void UINode::drawOverNode() {
	if ( NULL != getEventDispatcher() && mSceneNode->getHighlightOver() && getEventDispatcher()->getOverControl() == this ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( mSceneNode->getHighlightOverColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getScreenBounds() );
	}
}

void UINode::drawDebugData() {
	if ( NULL != mSceneNode && mSceneNode->getDrawDebugData() ) {
		if ( isWidget() ) {
			UIWidget * me = static_cast<UIWidget*>( this );

			if ( NULL != getEventDispatcher() && getEventDispatcher()->getOverControl() == this ) {
				String text( String::format( "X: %2.4f Y: %2.4f\nW: %2.4f H: %2.4f", mDpPos.x, mDpPos.y, mDpSize.x, mDpSize.y ) );

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
	if ( NULL != mSceneNode && mSceneNode->getDrawBoxes() ) {
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
		mSkinState->setStateColor( mSkinState->getCurrentState(), mSkinColor );

		if ( mFlags & UI_SKIN_KEEP_SIZE_ON_DRAW ) {
			Sizef rSize = PixelDensity::dpToPx( getSkinSize( getSkin(), mSkinState->getCurrentState() ) );
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

		drawForeground();
	}
}

Uint32 UINode::onMouseDown( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != getEventDispatcher() && !getEventDispatcher()->isNodeDragging() && !( getEventDispatcher()->getLastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && isDragEnabled() && !isDragging() ) {
		setDragging( true );

		if ( NULL != getEventDispatcher() )
			getEventDispatcher()->setNodeDragging( this );

		mDragPoint = Vector2f( Pos.x, Pos.y );
	}

	pushState( UIState::StatePressed );

	return Node::onMouseDown( Pos, Flags );
}

Uint32 UINode::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	if ( isDragEnabled() && isDragging() && ( Flags & mDragButton ) ) {
		setDragging( false );

		if ( NULL != getEventDispatcher() )
			getEventDispatcher()->setNodeDragging( NULL );
	}

	popState( UIState::StatePressed );

	return Node::onMouseUp( Pos, Flags );
}

Uint32 UINode::onCalculateDrag( const Vector2f& position, const Uint32& flags ) {
	if ( isDragEnabled() && isDragging() && NULL != getEventDispatcher() ) {
		EventDispatcher * eventDispatcher = getEventDispatcher();

		if ( !( flags/*press trigger*/ & mDragButton ) ) {
			setDragging( false );
			eventDispatcher->setNodeDragging( NULL );
			return 1;
		}

		Vector2f Pos( eefloor( position.x ), eefloor( position.y ) );

		if ( mDragPoint != Pos && ( std::abs( mDragPoint.x - Pos.x ) > 1.f || std::abs( mDragPoint.y - Pos.y ) > 1.f ) ) {
			if ( onDrag( Pos, flags ) ) {
				Sizef dragDiff;

				dragDiff.x = (Float)( mDragPoint.x - Pos.x );
				dragDiff.y = (Float)( mDragPoint.y - Pos.y );

				setPixelsPosition( mPosition - dragDiff );

				mDragPoint = Pos;

				onPositionChange();

				eventDispatcher->setNodeDragging( this );
			}
		}
	}

	return 1;
}

Uint32 UINode::onValueChange() {
	sendCommonEvent( Event::OnValueChange );
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

UISkin * UINode::setBackgroundFillEnabled( bool enabled ) {
	writeFlag( UI_FILL_BACKGROUND, enabled ? 1 : 0 );

	if ( enabled && NULL == mBackgroundState ) {
		getBackground();
	}

	invalidateDraw();

	return NULL != mBackgroundState ? mBackgroundState->getSkin() : NULL;
}

UINode * UINode::setBackgroundDrawable( Drawable * drawable, bool ownIt ) {
	setBackgroundFillEnabled( true )->setStateDrawable( UIState::StateFlagNormal, drawable, ownIt );
	return this;
}

UINode * UINode::setBackgroundColor( const Color& color ) {
	UISkin * background = setBackgroundFillEnabled( true );

	Drawable * stateDrawable = background->getStateDrawable( UIState::StateFlagNormal );

	if ( NULL == stateDrawable )
		setBackgroundDrawable( RectangleDrawable::New(), true );

	if ( NULL != mBackgroundState )
		mBackgroundState->setStateColor( UIState::StateFlagNormal, color );

	return this;
}

Color UINode::getBackgroundColor() const {
	if ( NULL != mBackgroundState )
		return mBackgroundState->getStateColor( UIState::StateFlagNormal );

	return Color::Transparent;
}

UINode * UINode::setBorderRadius( const unsigned int& corners ) {
	setBorderEnabled( true )->setCorners( corners );

	UISkin * background = setBackgroundFillEnabled( true );

	Drawable * stateDrawable = background->getStateDrawable( UIState::StateFlagNormal );

	if ( NULL == stateDrawable ) {
		setBackgroundColor( Color::Transparent );

		stateDrawable = background->getStateDrawable( UIState::StateFlagNormal );
	}

	if ( stateDrawable->getDrawableType() == Drawable::RECTANGLE ) {
		static_cast<RectangleDrawable*>( stateDrawable )->setCorners( corners );
	}

	return this;
}

Uint32 UINode::getBorderRadius() const {
	if ( NULL != mBorder )
		return mBorder->getCorners();

	return 0;
}

UISkin * UINode::setForegroundFillEnabled( bool enabled ) {
	writeFlag( UI_FILL_FOREGROUND, enabled ? 1 : 0 );

	if ( enabled && NULL == mForegroundState ) {
		getForeground();
	}

	invalidateDraw();

	return NULL != mForegroundState ? mForegroundState->getSkin() : NULL;
}

UINode * UINode::setForegroundDrawable( Drawable * drawable, bool ownIt ) {
	setForegroundFillEnabled( true )->setStateDrawable( UIState::StateFlagNormal, drawable, ownIt );
	return this;
}

Color UINode::getForegroundColor() const {
	if ( NULL != mForegroundState )
		return mForegroundState->getStateColor( UIState::StateFlagNormal );

	return Color::Transparent;
}

UINode * UINode::setForegroundColor( const Color& color ) {
	UISkin * foreground = setForegroundFillEnabled( true );

	Drawable * stateDrawable = foreground->getStateDrawable( UIState::StateFlagNormal );

	if ( NULL == stateDrawable )
		setForegroundDrawable( RectangleDrawable::New(), true );

	if ( NULL != mForegroundState )
		mForegroundState->setStateColor( UIState::StateFlagNormal, color );

	return this;
}

UINode * UINode::setForegroundRadius( const unsigned int& corners ) {
	UISkin * foreground = setForegroundFillEnabled( true );

	Drawable * stateDrawable = foreground->getStateDrawable( UIState::StateFlagNormal );

	if ( NULL == stateDrawable ) {
		setForegroundColor( Color::Black );

		stateDrawable = foreground->getStateDrawable( UIState::StateFlagNormal );
	}

	if ( stateDrawable->getDrawableType() == Drawable::RECTANGLE ) {
		RectangleDrawable * rectangleDrawable = static_cast<RectangleDrawable*>( stateDrawable );

		rectangleDrawable->setCorners( corners );
	}

	return this;
}

Uint32 UINode::getForegroundRadius() const {
	if ( NULL != mForegroundState && NULL != mForegroundState->getSkin() ) {
		Drawable * stateDrawable = mForegroundState->getSkin()->getStateDrawable( UIState::StateFlagNormal );

		if ( stateDrawable->getDrawableType() == Drawable::RECTANGLE ) {
			RectangleDrawable * rectangleDrawable = static_cast<RectangleDrawable*>( stateDrawable );

			return rectangleDrawable->getCorners();
		}
	}

	return 0;
}

RectangleDrawable * UINode::setBorderEnabled( bool enabled ) {
	writeFlag( UI_BORDER, enabled ? 1 : 0 );

	if ( enabled && NULL == mBorder ) {
		getBorder();

		if ( NULL == mBackgroundState ) {
			getBackground();
		}
	}

	invalidateDraw();

	return NULL != mBorder ? mBorder : NULL;
}

UINode * UINode::setBorderColor( const Color& color ) {
	setBorderEnabled( true )->setColor( color );
	return this;
}

Color UINode::getBorderColor() {
	return setBorderEnabled( true )->getColor();
}

UINode * UINode::setBorderWidth( const unsigned int& width ) {
	setBorderEnabled( true )->setLineWidth( width );
	return this;
}

Float UINode::getBorderWidth() const {
	return NULL != mBorder ? mBorder->getLineWidth() : 1.f;
}

const Uint32& UINode::getFlags() const {
	return mFlags;
}

UINode * UINode::setFlags( const Uint32& flags ) {
	if ( NULL == mBackgroundState && ( flags & UI_FILL_BACKGROUND ) )
		setBackgroundFillEnabled( true );

	if ( NULL == mForegroundState && ( flags & UI_FILL_FOREGROUND ) )
		setForegroundFillEnabled( true );

	if ( NULL == mBorder && ( flags & UI_BORDER ) )
		setBorderEnabled( true );

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

UINode * UINode::resetFlags( Uint32 newFlags ) {
	mFlags = newFlags;
	return this;
}

void UINode::drawBackground() {
	if ( ( mFlags & UI_FILL_BACKGROUND ) && NULL != mBackgroundState ) {
		mBackgroundState->draw( mScreenPosi.x, mScreenPosi.y, eefloor(mSize.getWidth()), eefloor(mSize.getHeight()), (Uint32)mAlpha );
	}
}

void UINode::drawForeground() {
	if ( ( mFlags & UI_FILL_FOREGROUND ) && NULL != mForegroundState ) {
		mForegroundState->draw( mScreenPosi.x, mScreenPosi.y, eefloor(mSize.getWidth()), eefloor(mSize.getHeight()), (Uint32)mAlpha );
	}
}

void UINode::drawBorder() {
	if ( ( mFlags & UI_BORDER ) && NULL != mBorder ) {
		Uint8 alpha = mBorder->getAlpha();
		mBorder->setAlpha( eemin<Uint32>( mAlpha * alpha / 255.f, 255 ) );
		mBorder->draw( Vector2f( mScreenPosi.x, mScreenPosi.y ), Sizef( eefloor(mSize.getWidth()), eefloor(mSize.getHeight()) ) );
		mBorder->setAlpha( alpha );
	}
}

void UINode::internalDraw() {
	if ( mVisible ) {
		if ( mNodeFlags & NODE_FLAG_POSITION_DIRTY )
			updateScreenPos();

		matrixSet();

		clipStart();

		draw();

		drawChilds();

		clipEnd();

		drawBorder();

		drawHighlightFocus();

		drawOverNode();

		drawDebugData();

		drawBox();

		matrixUnset();
	}
}

UISkin * UINode::getBackground() {
	if ( NULL == mBackgroundState ) {
		mBackgroundState = UISkinState::New( UISkin::New() );
	}

	return mBackgroundState->getSkin();
}

UISkin * UINode::getForeground() {
	if ( NULL == mForegroundState ) {
		mForegroundState = UISkinState::New( UISkin::New() );
	}

	return mForegroundState->getSkin();
}

RectangleDrawable * UINode::getBorder() {
	if ( NULL == mBorder ) {
		mBorder = RectangleDrawable::New();
		mBorder->setColor( Color::Transparent );
		mBorder->setFillMode( PrimitiveFillMode::DRAW_LINE );
		mBorder->setLineWidth( PixelDensity::dpToPx(1) );
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
		setSkin( Theme->getSkin( skinName ) );
	}

	return this;
}

UINode * UINode::setSkin( const UISkin& Skin ) {
	removeSkin();

	writeNodeFlag( NODE_FLAG_SKIN_OWNER, 1 );

	UISkin * SkinCopy = const_cast<UISkin*>( &Skin )->clone();

	mSkinState = UISkinState::New( SkinCopy );

	onThemeLoaded();

	return this;
}

UINode * UINode::setSkin( UISkin * skin ) {
	if ( NULL != skin ) {
		if ( NULL != mSkinState && mSkinState->getSkin() == skin )
			return this;

		Uint32 InitialState = UIState::StateFlagNormal;

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

UINode * UINode::setSkinColor( const Color& color ) {
	mSkinColor = color;
	return this;
}

const Color& UINode::getSkinColor() const {
	return mSkinColor;
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

void UINode::onEnabledChange() {
	if ( !mEnabled ) {
		pushState( UIState::StateDisabled );
	} else {
		popState( UIState::StateDisabled );
	}

	Node::onEnabledChange();
}

void UINode::onAlignChange() {
	invalidateDraw();
}

void UINode::pushState(const Uint32& State , bool emitEvent) {
	if ( !( mState & ( 1 << State ) ) ) {
		mState |= 1 << State;

		if ( NULL != mSkinState )
			mSkinState->pushState( State );

		if ( NULL != mBackgroundState )
			mBackgroundState->pushState( State );

		if ( NULL != mForegroundState )
			mForegroundState->pushState( State );

		if ( emitEvent ) {
			onStateChange();
		} else {
			invalidateDraw();
		}
	}
}

void UINode::popState(const Uint32& State , bool emitEvent) {
	if ( mState & ( 1 << State ) ) {
		mState &= ~( 1 << State );

		if ( NULL != mSkinState )
			mSkinState->popState( State );

		if ( NULL != mBackgroundState )
			mBackgroundState->popState( State );

		if ( NULL != mForegroundState )
			mForegroundState->popState( State );

		if ( emitEvent ) {
			onStateChange();
		} else {
			invalidateDraw();
		}
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

UISkin * UINode::getSkin() const {
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
			Rectf rPadding = mSkinState->getSkin()->getBorderSize();

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

	return PixelDensity::pxToDp( tPadding );
}

Sizef UINode::getSkinSize( UISkin * Skin, const Uint32& State ) const {
	if ( NULL != Skin ) {
		return Skin->getSize( State );
	}

	return Sizef::Zero;
}

Sizef UINode::getSkinSize() const {
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

void UINode::worldToNode( Vector2i& pos ) const {
	Vector2f toPos( convertToNodeSpace( Vector2f( pos.x, pos.y ) ) );
	pos = Vector2i( toPos.x  / PixelDensity::getPixelDensity(), toPos.y / PixelDensity::getPixelDensity() );
}

void UINode::nodeToWorld( Vector2i& pos ) const {
	Vector2f toPos( convertToWorldSpace( Vector2f( pos.x * PixelDensity::getPixelDensity(), pos.y * PixelDensity::getPixelDensity() ) ) );
	pos = Vector2i( toPos.x, toPos.y );
}

void UINode::worldToNode( Vector2f& pos ) const {
	Vector2f toPos( convertToNodeSpace( pos ) );
	pos = Vector2f( toPos.x  / PixelDensity::getPixelDensity(), toPos.y / PixelDensity::getPixelDensity() );
}

void UINode::nodeToWorld( Vector2f& pos ) const {
	Vector2f toPos( convertToWorldSpace( Vector2f( pos.x * PixelDensity::getPixelDensity(), pos.y * PixelDensity::getPixelDensity() ) ) );
	pos = Vector2f( toPos.x, toPos.y );
}

Node * UINode::getWindowContainer() const {
	const Node * Ctrl = this;

	while ( Ctrl != NULL ) {
		if ( Ctrl->isType( UI_TYPE_WINDOW ) ) {
			return static_cast<const UIWindow*>( Ctrl )->getContainer();
		} else if ( mSceneNode == Ctrl ) {
			return mSceneNode;
		}

		Ctrl = Ctrl->getParent();
	}

	return mSceneNode;
}

const Vector2f& UINode::getDragPoint() const {
	return mDragPoint;
}

void UINode::setDragPoint( const Vector2f& Point ) {
	mDragPoint = Point;
}

Uint32 UINode::onDrag( const Vector2f& , const Uint32& ) {
	return 1;
}

Uint32 UINode::onDragStart( const Vector2i& ) {
	sendCommonEvent( Event::OnDragStart );
	return 1;
}

Uint32 UINode::onDragStop( const Vector2i& ) {
	sendCommonEvent( Event::OnDragStop );
	return 1;
}

Uint32 UINode::onMouseOver(const Vector2i& position, const Uint32& flags) {
	Node::onMouseOver( position, flags );

	pushState( UIState::StateHover );

	return 1;
}

Uint32 UINode::onMouseLeave(const Vector2i& position, const Uint32& flags) {
	Node::onMouseLeave( position, flags );

	popState( UIState::StateHover );
	popState( UIState::StatePressed );

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
	if ( NULL == getEventDispatcher() )
		return;

	writeNodeFlag( NODE_FLAG_DRAGGING, dragging );

	if ( dragging ) {
		NodeMessage tMsg( this, NodeMessage::DragStart, 0 );
		messagePost( &tMsg );

		onDragStart( getEventDispatcher()->getMousePos() );
	} else {
		NodeMessage tMsg( this, NodeMessage::DragStop, 0 );
		messagePost( &tMsg );

		onDragStop( getEventDispatcher()->getMousePos() );
	}
}

void UINode::setDragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& UINode::getDragButton() const {
	return mDragButton;
}

void UINode::onWidgetFocusLoss() {
	sendCommonEvent( Event::OnWidgetFocusLoss );
	invalidateDraw();
}

void UINode::setFocus() {
	if ( NULL != getEventDispatcher() )
		getEventDispatcher()->setFocusControl( this );
}

Uint32 UINode::onFocus() {
	pushState( UIState::StateFocus );

	return Node::onFocus();
}

Uint32 UINode::onFocusLoss() {
	popState( UIState::StateFocus );

	return Node::onFocusLoss();
}

}}
