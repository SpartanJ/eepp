#include <eepp/graphics/font.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/actions/fade.hpp>
#include <eepp/scene/actions/move.hpp>
#include <eepp/scene/actions/rotate.hpp>
#include <eepp/scene/actions/scale.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiborderdrawable.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uiskinstate.hpp>
#include <eepp/ui/uistate.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace UI {

UINode* UINode::New() {
	return eeNew( UINode, () );
}

UINode::UINode() :
	Node(),
	mFlags( UI_NODE_DEFAULT_FLAGS ),
	mState( UIState::StateFlagNormal ),
	mSkinState( NULL ),
	mBackground( NULL ),
	mForeground( NULL ),
	mBorder( NULL ),
	mDragButton( EE_BUTTON_LMASK ),
	mSkinColor( Color::White ),
	mUISceneNode( SceneManager::instance()->getUISceneNode() ) {
	mNodeFlags |= NODE_FLAG_UINODE | NODE_FLAG_OVER_FIND_ALLOWED;

	if ( NULL != mUISceneNode )
		setParent( (Node*)mUISceneNode->getRoot() );
}

UINode::~UINode() {
	removeSkin();

	eeSAFE_DELETE( mBackground );
	eeSAFE_DELETE( mForeground );
	eeSAFE_DELETE( mBorder );

	if ( isDragging() && getEventDispatcher() )
		getEventDispatcher()->setNodeDragging( NULL );
}

void UINode::worldToNodeTranslation( Vector2f& Pos ) const {
	Node* ParentLoop = mParentNode;

	Pos -= mPosition;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->isUINode()
										? ParentLoop->asType<UINode>()->getPixelsPosition()
										: ParentLoop->getPosition();

		Pos -= ParentPos;

		ParentLoop = ParentLoop->getParent();
	}
}

void UINode::nodeToWorldTranslation( Vector2f& Pos ) const {
	Node* ParentLoop = mParentNode;

	while ( NULL != ParentLoop ) {
		const Vector2f& ParentPos = ParentLoop->isUINode()
										? ParentLoop->asType<UINode>()->getPixelsPosition()
										: ParentLoop->getPosition();

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

Node* UINode::setPosition( const Float& x, const Float& y ) {
	setPosition( Vector2f( x, y ) );
	return this;
}

UINode* UINode::setPixelsPosition( const Vector2f& Pos ) {
	if ( mPosition != Pos ) {
		mDpPos = PixelDensity::pxToDp( Pos );
		Transformable::setPosition( Pos );
		setDirty();
		onPositionChange();
	}
	return this;
}

UINode* UINode::setPixelsPosition( const Float& x, const Float& y ) {
	setPixelsPosition( Vector2f( x, y ) );
	return this;
}

const Vector2f& UINode::getPosition() const {
	return mDpPos;
}

const Vector2f& UINode::getPixelsPosition() const {
	return mPosition;
}

void UINode::setInternalSize( const Sizef& size ) {
	Sizef s( size );

	if ( s.x < mMinSize.x )
		s.x = mMinSize.x;

	if ( s.y < mMinSize.y )
		s.y = mMinSize.y;

	if ( s != mDpSize ) {
		mDpSize = size;
		mSize = PixelDensity::dpToPx( s );
		mNodeFlags |= NODE_FLAG_POLYGON_DIRTY;
		updateCenter();
		sendCommonEvent( Event::OnSizeChange );
		invalidateDraw();
	}
}

void UINode::setInternalPixelsSize( const Sizef& size ) {
	Sizef s( size );
	Sizef pMinSize( PixelDensity::dpToPx( mMinSize ) );

	if ( s.x < pMinSize.x )
		s.x = pMinSize.x;

	if ( s.y < pMinSize.y )
		s.y = pMinSize.y;

	if ( s != mSize ) {
		mDpSize = PixelDensity::pxToDp( s ).ceil();
		mSize = s;
		mNodeFlags |= NODE_FLAG_POLYGON_DIRTY;
		updateCenter();
		sendCommonEvent( Event::OnSizeChange );
		invalidateDraw();
	}
}

Node* UINode::setSize( const Sizef& size ) {
	Sizef s( fitMinMaxSizeDp( size ) );

	if ( s.x < mMinSize.x )
		s.x = mMinSize.x;

	if ( s.y < mMinSize.y )
		s.y = mMinSize.y;

	if ( s != mDpSize ) {
		Vector2f sizeChange( s.x - mDpSize.x, s.y - mDpSize.y );

		setInternalSize( s );

		onSizeChange();

		if ( reportSizeChangeToChilds() ) {
			sendParentSizeChange( sizeChange );
		}
	}

	return this;
}

Node* UINode::setSize( const Float& Width, const Float& Height ) {
	return setSize( Vector2f( Width, Height ) );
}

UINode* UINode::setPixelsSize( const Sizef& size ) {
	Sizef s( fitMinMaxSizePx( size ) );
	Sizef pMinSize( PixelDensity::dpToPx( mMinSize ) );

	if ( s.x < pMinSize.x )
		s.x = pMinSize.x;

	if ( s.y < pMinSize.y )
		s.y = pMinSize.y;

	if ( s != mSize ) {
		Vector2f sizeChange( s.x - mSize.x, s.y - mSize.y );

		setInternalPixelsSize( s );

		onSizeChange();

		if ( reportSizeChangeToChilds() ) {
			sendParentSizeChange( PixelDensity::pxToDp( sizeChange ) );
		}
	}

	return this;
}

UINode* UINode::setPixelsSize( const Float& x, const Float& y ) {
	return setPixelsSize( Sizef( x, y ) );
}

void UINode::setInternalPixelsWidth( const Float& width ) {
	setInternalPixelsSize( Sizef( width, mSize.y ) );
}

void UINode::setInternalPixelsHeight( const Float& height ) {
	setInternalPixelsSize( Sizef( mSize.x, height ) );
}

void UINode::setMinSize( const Sizef& size ) {
	if ( size != mMinSize ) {
		mMinSize = size;
		setSize( getSize() );
	}
}

void UINode::setMinWidth( const Float& width ) {
	if ( width != mMinSize.x ) {
		mMinSize.x = width;
		setSize( getSize() );
	}
}

void UINode::setMinHeight( const Float& height ) {
	if ( height != mMinSize.y ) {
		mMinSize.y = height;
		setSize( getSize() );
	}
}

const Sizef& UINode::getCurMinSize() const {
	return mMinSize;
}

const std::string& UINode::getMinWidthEq() const {
	return mMinWidthEq;
}

void UINode::setMinSizeEq( const std::string& minWidthEq, const std::string& minHeightEq ) {
	if ( mMinWidthEq != minWidthEq || mMinHeightEq != minHeightEq ) {
		mMinWidthEq = minWidthEq;
		mMinHeightEq = minHeightEq;

		if ( !mMinWidthEq.empty() ) {
			mMinSize.x = lengthFromValueAsDp( mMinWidthEq,
											  CSS::PropertyRelativeTarget::ContainingBlockWidth );
		}

		if ( !mMinHeightEq.empty() ) {
			mMinSize.y = lengthFromValueAsDp( mMinHeightEq,
											  CSS::PropertyRelativeTarget::ContainingBlockHeight );
		}

		setSize( mDpSize );
	}
}

void UINode::setMinWidthEq( const std::string& minWidthEq ) {
	if ( mMinWidthEq != minWidthEq ) {
		mMinWidthEq = minWidthEq;

		if ( !mMinWidthEq.empty() ) {
			mMinSize.x = lengthFromValueAsDp( mMinWidthEq,
											  CSS::PropertyRelativeTarget::ContainingBlockWidth );
		}

		setSize( mDpSize );
	}
}

const std::string& UINode::getMinHeightEq() const {
	return mMinHeightEq;
}

void UINode::setMinHeightEq( const std::string& minHeightEq ) {
	if ( mMinHeightEq != minHeightEq ) {
		mMinHeightEq = minHeightEq;

		if ( !mMinHeightEq.empty() ) {
			mMinSize.y = lengthFromValueAsDp( mMinHeightEq,
											  CSS::PropertyRelativeTarget::ContainingBlockHeight );
		}

		setSize( mDpSize );
	}
}

const std::string& UINode::getMaxWidthEq() const {
	return mMaxWidthEq;
}

void UINode::setMaxSizeEq( const std::string& maxWidthEq, const std::string& maxHeightEq ) {
	if ( mMaxWidthEq != maxWidthEq || mMaxHeightEq != maxHeightEq ) {
		mMaxWidthEq = maxWidthEq;
		mMaxHeightEq = maxHeightEq;
		setSize( mDpSize );
	}
}

void UINode::setMaxWidthEq( const std::string& maxWidthEq ) {
	if ( mMaxWidthEq != maxWidthEq ) {
		mMaxWidthEq = maxWidthEq;
		setSize( mDpSize );
	}
}

const std::string& UINode::getMaxHeightEq() const {
	return mMaxHeightEq;
}

void UINode::setMaxHeightEq( const std::string& maxHeightEq ) {
	if ( mMaxHeightEq != maxHeightEq ) {
		mMaxHeightEq = maxHeightEq;
		setSize( mDpSize );
	}
}

Sizef UINode::getMaxSize() const {
	Sizef s;

	if ( !mMaxWidthEq.empty() ) {
		Float length =
			lengthFromValueAsDp( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		s.x = eemax( s.x, length );
	}

	if ( !mMaxHeightEq.empty() ) {
		Float length =
			lengthFromValueAsDp( mMaxHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
		s.y = eemax( s.y, length );
	}

	return s;
}

Sizef UINode::getMaxSizePx() const {
	Sizef s;

	if ( !mMaxWidthEq.empty() ) {
		Float length =
			lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		s.x = eemax( s.x, length );
	}

	if ( !mMaxHeightEq.empty() ) {
		Float length =
			lengthFromValue( mMaxHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
		s.y = eemax( s.y, length );
	}

	return s;
}

Sizef UINode::getMinSize() const {
	Sizef s;

	if ( !mMinWidthEq.empty() ) {
		Float length =
			lengthFromValueAsDp( mMinWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		s.x = eemax( s.x, length );
	}

	if ( !mMinHeightEq.empty() ) {
		Float length =
			lengthFromValueAsDp( mMinHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
		s.y = eemax( s.y, length );
	}

	return s;
}

Sizef UINode::getMinSizePx() const {
	Sizef s;

	if ( !mMinWidthEq.empty() ) {
		Float length =
			lengthFromValue( mMinWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		s.x = eemax( s.x, length );
	}

	if ( !mMinHeightEq.empty() ) {
		Float length =
			lengthFromValue( mMinHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
		s.y = eemax( s.y, length );
	}

	return s;
}

Sizef UINode::fitMinMaxSizePx( const Sizef& size ) const {
	Sizef s( size );

	if ( mMinSize.x != 0.f && s.x < PixelDensity::pxToDp( mMinSize.x ) )
		s.x = PixelDensity::pxToDp( mMinSize.x );

	if ( mMinSize.y != 0.f && s.y < PixelDensity::pxToDp( mMinSize.y ) )
		s.y = PixelDensity::pxToDp( mMinSize.y );

	if ( !mMinWidthEq.empty() ) {
		Float length =
			lengthFromValue( mMinWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		s.x = eemax( s.x, length );
	}

	if ( !mMinHeightEq.empty() ) {
		Float length =
			lengthFromValue( mMinHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
		s.y = eemax( s.y, length );
	}

	if ( !mMaxWidthEq.empty() ) {
		Float length =
			lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		s.x = eemin( s.x, length );
	}

	if ( !mMaxHeightEq.empty() ) {
		Float length =
			lengthFromValue( mMaxHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
		s.y = eemin( s.y, length );
	}

	return s;
}

bool UINode::isScrollable() const {
	return 0 != ( mFlags & UI_SCROLLABLE );
}

Sizef UINode::fitMinMaxSizeDp( const Sizef& size ) const {
	Sizef s( size );

	if ( s.x < mMinSize.x )
		s.x = mMinSize.x;

	if ( s.y < mMinSize.y )
		s.y = mMinSize.y;

	if ( !mMinWidthEq.empty() ) {
		Float length =
			lengthFromValueAsDp( mMinWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		s.x = eemax( s.x, length );
	}

	if ( !mMinHeightEq.empty() ) {
		Float length =
			lengthFromValueAsDp( mMinHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
		s.y = eemax( s.y, length );
	}

	if ( !mMaxWidthEq.empty() ) {
		Float length =
			lengthFromValueAsDp( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		s.x = eemin( s.x, length );
	}

	if ( !mMaxHeightEq.empty() ) {
		Float length =
			lengthFromValueAsDp( mMaxHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
		s.y = eemin( s.y, length );
	}

	return s;
}

void UINode::updateOriginPoint() {
	Node::updateOriginPoint();

	if ( mRotationOriginPoint.OriginType == OriginPoint::OriginEquation ) {
		if ( !mRotationOriginPoint.getXEq().empty() ) {
			mRotationOriginPoint.x = lengthFromValue( mRotationOriginPoint.getXEq(),
													  PropertyRelativeTarget::LocalBlockWidth );
		}

		if ( !mRotationOriginPoint.getYEq().empty() ) {
			mRotationOriginPoint.y = lengthFromValue( mRotationOriginPoint.getYEq(),
													  PropertyRelativeTarget::LocalBlockHeight );
		}

		Transformable::setRotationOrigin( getRotationOriginPoint().x, getRotationOriginPoint().y );
	}

	if ( mScaleOriginPoint.OriginType == OriginPoint::OriginEquation ) {
		if ( !mScaleOriginPoint.getXEq().empty() ) {
			mScaleOriginPoint.x = lengthFromValue( mScaleOriginPoint.getXEq(),
												   PropertyRelativeTarget::LocalBlockWidth );
		}

		if ( !mScaleOriginPoint.getYEq().empty() ) {
			mScaleOriginPoint.y = lengthFromValue( mScaleOriginPoint.getYEq(),
												   PropertyRelativeTarget::LocalBlockHeight );
		}

		Transformable::setScaleOrigin( getScaleOriginPoint().x, getScaleOriginPoint().y );
	}
}

Rect UINode::getRect() const {
	return Rect( Vector2i( mDpPos.x, mDpPos.y ), Sizei( mDpSize.x, mDpSize.y ) );
}

Rectf UINode::getRectBox() const {
	return Rectf( mPosition, mSize );
}

const Sizef& UINode::getSize() const {
	return mDpSize;
}

void UINode::drawHighlightFocus() {
	if ( ( mFlags & UI_HIGHLIGHT ) ||
		 ( NULL != getEventDispatcher() && mSceneNode->getHighlightFocus() &&
		   getEventDispatcher()->getFocusNode() == this ) ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( mSceneNode->getHighlightFocusColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getScreenBounds() );
	}
}

void UINode::drawOverNode() {
	if ( NULL != getEventDispatcher() && mSceneNode->getHighlightOver() &&
		 getEventDispatcher()->getMouseOverNode() == this ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( mSceneNode->getHighlightOverColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getScreenBounds() );
	}
}

void UINode::drawDroppableHovering() {
	const PropertyDefinition* def =
		StyleSheetSpecification::instance()->getProperty( "droppable-hovering-color" );
	Color color = Color::fromString( def->getDefaultValue() );
	if ( isWidget() ) {
		UIWidget* widget = asType<UIWidget>();
		std::string colorString = widget->getPropertyString( def );
		if ( !colorString.empty() ) {
			color = Color::fromString( colorString );
		} else {
			colorString = mUISceneNode->getRoot()->getPropertyString( def );
			if ( !colorString.empty() )
				color = Color::fromString( colorString );
		}
	}

	Primitives P;
	P.setFillMode( DRAW_FILL );
	P.setBlendMode( getBlendMode() );
	P.setColor( color );
	P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
	P.drawRectangle( getScreenBounds() );
}

void UINode::updateDebugData() {
	if ( NULL != mSceneNode && mSceneNode->getDrawDebugData() && isWidget() &&
		 NULL != getEventDispatcher() && getEventDispatcher()->getMouseOverNode() == this ) {
		UIWidget* widget = asType<UIWidget>();

		String text = "Tag: " + String::fromUtf8( widget->getStyleSheetTag() ) + "\n";

		if ( !mId.empty() ) {
			text += "ID: " + mId + "\n";
		}

		if ( !widget->getStyleSheetClasses().empty() ) {
			text += "Classes: " + String::join( widget->getStyleSheetClasses(), ' ' ) + "\n";
		}

		if ( !widget->getStyleSheetPseudoClasses().empty() ) {
			text += "Pseudo Classes: " + String::join( widget->getStyleSheetPseudoClasses(), ' ' ) +
					"\n";
		}

		text += String::format(
			"X: %.2fpx (%.2fdp) Y: %.2fpx (%.2fdp)\nW: %.2fpx (%.2fdp) H: %.2fpx (%.2fdp)",
			mPosition.x, mDpPos.x, mPosition.y, mDpPos.y, mSize.x, mDpSize.x, mSize.y, mDpSize.y );

		if ( widget->getPadding() != Rectf( 0, 0, 0, 0 ) ) {
			Rectf p( widget->getPadding() );
			text += String::format( "\npadding: %.2f %.2f %.2f %.2f", p.Top, p.Right, p.Bottom,
									p.Left );
		}

		if ( widget->getLayoutMargin() != Rectf( 0, 0, 0, 0 ) ) {
			Rectf m( widget->getLayoutMargin() );
			text +=
				String::format( "\nmargin: %.2f %.2f %.2f %.2f", m.Top, m.Right, m.Bottom, m.Left );
		}

		widget->setTooltipText( text );
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
			Sizef rSize =
				PixelDensity::dpToPx( getSkinSize( getSkin(), mSkinState->getCurrentState() ) );
			Sizef diff = ( mSize - rSize ) * 0.5f;

			mSkinState->draw( mScreenPosi.x + eefloor( diff.x ), mScreenPosi.y + eefloor( diff.y ),
							  eefloor( rSize.getWidth() ), eefloor( rSize.getHeight() ),
							  (Uint32)mAlpha );
		} else {
			mSkinState->draw( mScreenPosi.x, mScreenPosi.y, eefloor( mSize.getWidth() ),
							  eefloor( mSize.getHeight() ), (Uint32)mAlpha );
		}
	}
}

void UINode::draw() {}

Uint32 UINode::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( NULL != getEventDispatcher() && !getEventDispatcher()->isNodeDragging() &&
		 !( getEventDispatcher()->getLastPressTrigger() & mDragButton ) &&
		 ( flags & mDragButton ) && isDragEnabled() && !isDragging() ) {
		startDragging( position.asFloat() );
	}

	pushState( UIState::StatePressed );

	return Node::onMouseDown( position, flags );
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
		EventDispatcher* eventDispatcher = getEventDispatcher();

		if ( !( flags /*press trigger*/ & mDragButton ) ) {
			setDragging( false );
			eventDispatcher->setNodeDragging( NULL );
			return 1;
		}

		Vector2f pos( eefloor( position.x ), eefloor( position.y ) );
		if ( mDragPoint != pos && ( std::abs( mDragPoint.x - pos.x ) > 1.f ||
									std::abs( mDragPoint.y - pos.y ) > 1.f ) ) {
			Sizef dragDiff;
			dragDiff.x = ( mFlags & UI_DRAG_HORIZONTAL ) ? (Float)( mDragPoint.x - pos.x ) : 0;
			dragDiff.y = ( mFlags & UI_DRAG_VERTICAL ) ? (Float)( mDragPoint.y - pos.y ) : 0;

			if ( onDrag( pos, flags, dragDiff ) ) {
				setPixelsPosition( mPosition - dragDiff );
				mDragPoint = pos;
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

UINode* UINode::setHorizontalAlign( Uint32 halign ) {
	mFlags &= ~UI_HALIGN_MASK;
	mFlags |= halign & UI_HALIGN_MASK;

	onAlignChange();
	return this;
}

Uint32 UINode::getVerticalAlign() const {
	return mFlags & UI_VALIGN_MASK;
}

UINode* UINode::setVerticalAlign( Uint32 valign ) {
	mFlags &= ~UI_VALIGN_MASK;
	mFlags |= valign & UI_VALIGN_MASK;

	onAlignChange();
	return this;
}

UINode* UINode::setGravity( Uint32 hvalign ) {
	mFlags &= ~( UI_VALIGN_MASK | UI_HALIGN_MASK );
	mFlags |= ( hvalign & ( UI_VALIGN_MASK | UI_HALIGN_MASK ) );

	onAlignChange();
	return this;
}

UINodeDrawable* UINode::setBackgroundFillEnabled( bool enabled ) {
	writeFlag( UI_FILL_BACKGROUND, enabled ? 1 : 0 );

	if ( enabled && NULL == mBackground ) {
		getBackground();
	}

	invalidateDraw();

	return mBackground;
}

UINode* UINode::setBackgroundDrawable( Drawable* drawable, bool ownIt, int index ) {
	setBackgroundFillEnabled( true )->setDrawable( index, drawable, ownIt );
	return this;
}

UINode* UINode::setBackgroundDrawable( const std::string& drawable, int index ) {
	setBackgroundFillEnabled( true )->setDrawable( index, drawable );
	return this;
}

UINode* UINode::setBackgroundColor( const Color& color ) {
	setBackgroundFillEnabled( true )->setBackgroundColor( color );
	return this;
}

UINode* UINode::setBackgroundTint( const Color& color, int index ) {
	setBackgroundFillEnabled( true )->setDrawableColor( index, color );
	return this;
}

UINode* UINode::setBackgroundPositionX( const std::string& positionX, int index ) {
	setBackgroundFillEnabled( true )->setDrawablePositionX( index, positionX );
	return this;
}

UINode* UINode::setBackgroundPositionY( const std::string& positionY, int index ) {
	setBackgroundFillEnabled( true )->setDrawablePositionY( index, positionY );
	return this;
}

UINode* UINode::setBackgroundRepeat( const std::string& repeatRule, int index ) {
	setBackgroundFillEnabled( true )->setDrawableRepeat( index, repeatRule );
	return this;
}

UINode* UINode::setBackgroundSize( const std::string& size, int index ) {
	setBackgroundFillEnabled( true )->setDrawableSize( index, size );
	return this;
}

Color UINode::getBackgroundColor() const {
	return NULL != mBackground ? mBackground->getBackgroundColor() : Color::Transparent;
}

Color UINode::getBackgroundTint( int index ) const {
	return mBackground ? mBackground->getLayer( index )->getColor() : Color::White;
}

UINode* UINode::setBorderRadius( const unsigned int& corners ) {
	setBorderEnabled( true )->setRadius( corners );
	setBackgroundFillEnabled( true )->setBorderRadius( corners );
	return this;
}

UINode* UINode::setTopLeftRadius( const std::string& radius ) {
	setBorderEnabled( true )->setTopLeftRadius( radius );
	setBackgroundFillEnabled( true )->getBackgroundDrawable().setTopLeftRadius( radius );
	return this;
}

UINode* UINode::setTopRightRadius( const std::string& radius ) {
	setBorderEnabled( true )->setTopRightRadius( radius );
	setBackgroundFillEnabled( true )->getBackgroundDrawable().setTopRightRadius( radius );
	return this;
}

UINode* UINode::setBottomLeftRadius( const std::string& radius ) {
	setBorderEnabled( true )->setBottomLeftRadius( radius );
	setBackgroundFillEnabled( true )->getBackgroundDrawable().setBottomLeftRadius( radius );
	return this;
}

UINode* UINode::setBottomRightRadius( const std::string& radius ) {
	setBorderEnabled( true )->setBottomRightRadius( radius );
	setBackgroundFillEnabled( true )->getBackgroundDrawable().setBottomRightRadius( radius );
	return this;
}

Uint32 UINode::getBorderRadius() const {
	return NULL != mBorder ? mBorder->getRadius() : 0;
}

UINodeDrawable* UINode::setForegroundFillEnabled( bool enabled ) {
	writeFlag( UI_FILL_FOREGROUND, enabled ? 1 : 0 );

	if ( enabled && NULL == mForeground ) {
		getForeground();
	}

	invalidateDraw();

	return mForeground;
}

UINode* UINode::setForegroundDrawable( Drawable* drawable, bool ownIt, int index ) {
	setForegroundFillEnabled( true )->setDrawable( index, drawable, ownIt );
	return this;
}

UINode* UINode::setForegroundDrawable( const std::string& drawable, int index ) {
	setForegroundFillEnabled( true )->setDrawable( index, drawable );
	return this;
}

Color UINode::getForegroundColor() const {
	return NULL != mForeground ? mForeground->getBackgroundColor() : Color::Transparent;
}

Color UINode::getForegroundTint( int index ) const {
	return mForeground ? mForeground->getLayer( index )->getColor() : Color::White;
}

UINode* UINode::setForegroundColor( const Color& color ) {
	setForegroundFillEnabled( true )->setBackgroundColor( color );
	return this;
}

UINode* UINode::setForegroundTint( const Color& color, int index ) {
	setForegroundFillEnabled( true )->setDrawableColor( index, color );
	return this;
}

UINode* UINode::setForegroundPositionX( const std::string& positionX, int index ) {
	setForegroundFillEnabled( true )->setDrawablePositionX( index, positionX );
	return this;
}

UINode* UINode::setForegroundPositionY( const std::string& positionY, int index ) {
	setForegroundFillEnabled( true )->setDrawablePositionY( index, positionY );
	return this;
}

UINode* UINode::setForegroundRepeat( const std::string& repeatRule, int index ) {
	setForegroundFillEnabled( true )->setDrawableRepeat( index, repeatRule );
	return this;
}

UINode* UINode::setForegroundSize( const std::string& size, int index ) {
	setForegroundFillEnabled( true )->setDrawableSize( index, size );
	return this;
}

UINode* UINode::setForegroundRadius( const unsigned int& corners ) {
	setForegroundFillEnabled( true )->setBorderRadius( corners );
	return this;
}

Uint32 UINode::getForegroundRadius() const {
	return NULL != mForeground ? mForeground->getBorderRadius() : 0;
}

UIBorderDrawable* UINode::setBorderEnabled( bool enabled ) const {
	const_cast<UINode*>( this )->writeFlag( UI_BORDER, enabled ? 1 : 0 );

	if ( enabled && NULL == mBorder ) {
		getBorder();

		if ( NULL == mBackground ) {
			getBackground();
		}
	}

	const_cast<UINode*>( this )->invalidateDraw();

	return NULL != mBorder ? mBorder : NULL;
}

UINode* UINode::setBorderColor( const Color& color ) {
	setBorderEnabled( true )->setColor( color );
	return this;
}

Color UINode::getBorderColor() {
	return setBorderEnabled( true )->getColor();
}

UINode* UINode::setBorderWidth( const unsigned int& width ) {
	setBorderEnabled( true )->setLineWidth( width );
	return this;
}

Float UINode::getBorderWidth() const {
	return NULL != mBorder ? mBorder->getLineWidth() : 1.f;
}

const Uint32& UINode::getFlags() const {
	return mFlags;
}

UINode* UINode::setFlags( const Uint32& flags ) {
	if ( NULL == mBackground && ( flags & UI_FILL_BACKGROUND ) )
		setBackgroundFillEnabled( true );

	if ( NULL == mForeground && ( flags & UI_FILL_FOREGROUND ) )
		setForegroundFillEnabled( true );

	if ( NULL == mBorder && ( flags & UI_BORDER ) )
		setBorderEnabled( true );

	mFlags |= flags;

	if ( Font::getHorizontalAlign( flags ) || Font::getVerticalAlign( flags ) ) {
		onAlignChange();
	}

	return this;
}

UINode* UINode::unsetFlags( const Uint32& flags ) {
	if ( mFlags & flags )
		mFlags &= ~flags;

	if ( Font::getHorizontalAlign( flags ) || Font::getVerticalAlign( flags ) ) {
		onAlignChange();
	}

	return this;
}

UINode* UINode::resetFlags( Uint32 newFlags ) {
	mFlags = newFlags;
	return this;
}

void UINode::drawBackground() {
	if ( ( mFlags & UI_FILL_BACKGROUND ) && NULL != mBackground ) {
		mBackground->draw( mScreenPosi.asFloat(), mSize.floor(), mAlpha );
	}
}

void UINode::drawForeground() {
	if ( ( mFlags & UI_FILL_FOREGROUND ) && NULL != mForeground ) {
		mForeground->draw( mScreenPosi.asFloat(), mSize.floor(), (Uint32)mAlpha );
	}
}

void UINode::drawBorder() {
	if ( ( mFlags & UI_BORDER ) && NULL != mBorder ) {
		mBorder->setAlpha( mAlpha );
		mBorder->draw( mScreenPosi.asFloat(), mSize.floor() );
	}
}

void UINode::smartClipStart( const ClipType& reqClipType, bool needsClipPlanes ) {
	if ( mClip.getClipType() != reqClipType )
		return;
	switch ( mClip.getClipType() ) {
		case ClipType::PaddingBox: {
			const Rectf& pd = getPixelsPadding();
			clipSmartEnable( mScreenPos.x + pd.Left, mScreenPos.y + pd.Top,
							 mSize.getWidth() - pd.Left - pd.Right,
							 mSize.getHeight() - pd.Top - pd.Bottom, needsClipPlanes );
			break;
		}
		case ClipType::ContentBox: {
			clipSmartEnable( mScreenPos.x, mScreenPos.y, mSize.getWidth(), mSize.getHeight(),
							 needsClipPlanes );
			break;
		}
		case ClipType::BorderBox: {
			Rectf borderDiff;
			if ( mBorder )
				borderDiff = mBorder->getBorderBoxDiff();
			clipSmartEnable( mScreenPos.x + borderDiff.Left, mScreenPos.y + borderDiff.Top,
							 mSize.getWidth() + borderDiff.Right,
							 mSize.getHeight() + borderDiff.Bottom, needsClipPlanes );
			break;
		}
		case ClipType::None: {
			break;
		}
	}
}

void UINode::smartClipEnd( const ClipType& reqClipType, bool needsClipPlanes ) {
	if ( mVisible && isClipped() && mClip.getClipType() == reqClipType ) {
		clipEnd( needsClipPlanes );
	}
}

void UINode::smartClipStart( const ClipType& reqClipType ) {
	smartClipStart( reqClipType, isMeOrParentTreeScaledOrRotatedOrFrameBuffer() );
}

void UINode::smartClipEnd( const ClipType& reqClipType ) {
	smartClipEnd( reqClipType, isMeOrParentTreeScaledOrRotatedOrFrameBuffer() );
}

void UINode::nodeDraw() {
	if ( mVisible ) {
		if ( mNodeFlags & NODE_FLAG_POSITION_DIRTY )
			updateScreenPos();

		if ( mNodeFlags & NODE_FLAG_POLYGON_DIRTY )
			updateWorldPolygon();

		matrixSet();

		bool needsClipPlanes =
			mClip.getClipType() != ClipType::None && isMeOrParentTreeScaledOrRotatedOrFrameBuffer();

		smartClipStart( ClipType::BorderBox, needsClipPlanes );

		bool intersected = mWorldBounds.intersect( mSceneNode->getWorldBounds() );

		if ( intersected ) {
			smartClipStart( ClipType::ContentBox, needsClipPlanes );

			if ( 0.f != mAlpha ) {
				drawBackground();

				drawSkin();
			}

			smartClipStart( ClipType::PaddingBox, needsClipPlanes );

			draw();

			drawChilds();

			smartClipEnd( ClipType::PaddingBox, needsClipPlanes );

			if ( 0.f != mAlpha )
				drawForeground();

			smartClipEnd( ClipType::ContentBox, needsClipPlanes );
		} else if ( !isClipped() ) {
			drawChilds();
		}

		if ( intersected )
			drawBorder();

		if ( mNodeFlags & NODE_FLAG_DROPPABLE_HOVERING )
			drawDroppableHovering();

		if ( intersected ) {
			drawHighlightFocus();

			drawOverNode();

			updateDebugData();

			drawBox();
		}

		smartClipEnd( ClipType::BorderBox, needsClipPlanes );

		matrixUnset();
	}
}

void UINode::clearForeground() {
	eeSAFE_DELETE( mForeground );
}

void UINode::clearBackground() {
	eeSAFE_DELETE( mBackground );
}

const ClipType& UINode::getClipType() const {
	return mClip.getClipType();
}

UINode* UINode::setClipType( const ClipType& clipType ) {
	if ( mClip.getClipType() != clipType ) {
		mClip.setClipType( clipType );
		if ( mClip.getClipType() != ClipType::None ) {
			clipEnable();
		} else {
			clipDisable();
		}
	}
	return this;
}

bool UINode::hasBorder() const {
	return ( mFlags & UI_BORDER ) != 0;
}

const Rectf& UINode::getPixelsPadding() const {
	static const Rectf Zero{};
	return Zero;
}

UINodeDrawable* UINode::getBackground() const {
	if ( NULL == mBackground ) {
		mBackground = UINodeDrawable::New( const_cast<UINode*>( this ) );
	}

	return mBackground;
}

bool UINode::hasBackground() const {
	return mBackground != nullptr;
}

UINodeDrawable* UINode::getForeground() const {
	if ( NULL == mForeground ) {
		mForeground = UINodeDrawable::New( const_cast<UINode*>( this ) );
	}

	return mForeground;
}

bool UINode::hasForeground() const {
	return mForeground != nullptr;
}

UIBorderDrawable* UINode::getBorder() const {
	if ( NULL == mBorder ) {
		mBorder = UIBorderDrawable::New( this );
		mBorder->setColor( Color::Transparent );
		mBorder->setLineWidth( PixelDensity::dpToPx( 1 ) );
	}

	return mBorder;
}

void UINode::setThemeByName( const std::string& Theme ) {
	setTheme( getUISceneNode()->getUIThemeManager()->getByName( Theme ) );
}

void UINode::setTheme( UITheme* Theme ) {
	setThemeSkin( Theme, "widget" );
}

UINode* UINode::setThemeSkin( const std::string& skinName ) {
	return setThemeSkin( getUISceneNode()->getUIThemeManager()->getDefaultTheme(), skinName );
}

UINode* UINode::setThemeSkin( UITheme* Theme, const std::string& skinName ) {
	if ( NULL != Theme ) {
		setSkin( Theme->getSkin( skinName ) );
	}

	return this;
}

UINode* UINode::setSkin( const UISkin& Skin ) {
	removeSkin();

	writeNodeFlag( NODE_FLAG_SKIN_OWNER, 1 );

	UISkin* SkinCopy = const_cast<UISkin*>( &Skin )->clone();

	mSkinState = UISkinState::New( SkinCopy );

	onThemeLoaded();

	return this;
}

UINode* UINode::setSkin( UISkin* skin ) {
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
	} else {
		removeSkin();
	}

	return this;
}

UINode* UINode::setSkinColor( const Color& color ) {
	if ( color != mSkinColor ) {
		mSkinColor = color;
		invalidateDraw();
	}
	return this;
}

const Color& UINode::getSkinColor() const {
	return mSkinColor;
}

void UINode::removeSkin() {
	if ( NULL != mSkinState && ( mNodeFlags & NODE_FLAG_SKIN_OWNER ) ) {
		UISkin* tSkin = mSkinState->getSkin();

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

void UINode::pushState( const Uint32& State, bool emitEvent ) {
	if ( !( mState & ( 1 << State ) ) ) {
		mState |= 1 << State;

		if ( NULL != mSkinState )
			mSkinState->pushState( State );

		if ( emitEvent ) {
			onStateChange();
		} else {
			invalidateDraw();
		}
	}
}

void UINode::popState( const Uint32& State, bool emitEvent ) {
	if ( mState & ( 1 << State ) ) {
		mState &= ~( 1 << State );

		if ( NULL != mSkinState )
			mSkinState->popState( State );

		if ( emitEvent ) {
			onStateChange();
		} else {
			invalidateDraw();
		}
	}
}

void UINode::setThemeToChilds( UITheme* Theme ) {
	Node* ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->isUINode() ) {
			UINode* node = static_cast<UINode*>( ChildLoop );
			node->setThemeToChilds( Theme );
			node->setTheme( Theme ); // First set the theme to childs to let the father override the
									 // childs forced themes
		}

		ChildLoop = ChildLoop->getNextNode();
	}
}

UISkin* UINode::getSkin() const {
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
	getUISceneNode()->getUIThemeManager()->applyDefaultTheme( this );
}

Rectf UINode::makePadding( bool PadLeft, bool PadRight, bool PadTop, bool PadBottom,
						   bool SkipFlags ) const {
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

Sizef UINode::getSkinSize( UISkin* Skin, const Uint32& State ) const {
	if ( NULL != Skin ) {
		return Skin->getSize( State );
	}

	return Sizef::Zero;
}

Sizef UINode::getSkinSize( const Uint32& state ) const {
	if ( NULL != getSkin() ) {
		return getSkin()->getSize( state );
	}

	return Sizef::Zero;
}

void UINode::onThemeLoaded() {
	invalidateDraw();
}

void UINode::onChildCountChange( Node* child, const bool& removed ) {
	Node::onChildCountChange( child, removed );
}

void UINode::worldToNode( Vector2i& pos ) const {
	Vector2f toPos( convertToNodeSpace( Vector2f( pos.x, pos.y ) ) );
	pos = Vector2i( toPos.x / PixelDensity::getPixelDensity(),
					toPos.y / PixelDensity::getPixelDensity() );
}

void UINode::nodeToWorld( Vector2i& pos ) const {
	Vector2f toPos( convertToWorldSpace( Vector2f( pos.x * PixelDensity::getPixelDensity(),
												   pos.y * PixelDensity::getPixelDensity() ) ) );
	pos = Vector2i( toPos.x, toPos.y );
}

void UINode::worldToNode( Vector2f& pos ) const {
	Vector2f toPos( convertToNodeSpace( pos ) );
	pos = Vector2f( toPos.x / PixelDensity::getPixelDensity(),
					toPos.y / PixelDensity::getPixelDensity() );
}

void UINode::nodeToWorld( Vector2f& pos ) const {
	Vector2f toPos( convertToWorldSpace( Vector2f( pos.x * PixelDensity::getPixelDensity(),
												   pos.y * PixelDensity::getPixelDensity() ) ) );
	pos = Vector2f( toPos.x, toPos.y );
}

Node* UINode::getWindowContainer() const {
	const Node* node = this;

	while ( node != NULL ) {
		if ( node->isType( UI_TYPE_WINDOW ) ) {
			return static_cast<const UIWindow*>( node )->getContainer();
		} else if ( mSceneNode == node ) {
			if ( mSceneNode->isUISceneNode() ) {
				return static_cast<UISceneNode*>( mSceneNode )->getRoot();
			} else {
				return mSceneNode;
			}
		}

		node = node->getParent();
	}

	return mSceneNode;
}

bool UINode::isTabFocusable() const {
	return 0 != ( mFlags & UI_TAB_FOCUSABLE );
}

const Vector2f& UINode::getDragPoint() const {
	return mDragPoint;
}

void UINode::setDragPoint( const Vector2f& Point ) {
	mDragPoint = Point;
}

Uint32 UINode::onDrag( const Vector2f&, const Uint32&, const Sizef& ) {
	return 1;
}

Uint32 UINode::onDragStart( const Vector2i& pos, const Uint32& flags ) {
	sendMouseEvent( Event::OnDragStart, pos, flags );
	return 1;
}

Uint32 UINode::onDragStop( const Vector2i& pos, const Uint32& flags ) {
	sendMouseEvent( Event::OnDragStop, pos, flags );
	return 1;
}

Uint32 UINode::onDrop( UINode* widget ) {
	DropEvent event( this, widget, Event::OnNodeDropped );
	sendEvent( &event );
	return 1;
}

Uint32 UINode::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	Node::onMouseOver( position, flags );

	pushState( UIState::StateHover );

	return 1;
}

Uint32 UINode::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
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
		NodeMessage tMsg( this, NodeMessage::DragStart, getEventDispatcher()->getPressTrigger() );
		messagePost( &tMsg );

		onDragStart( getEventDispatcher()->getMousePos(), getEventDispatcher()->getPressTrigger() );
	} else {
		NodeMessage tMsg( this, NodeMessage::DragStop, getEventDispatcher()->getPressTrigger() );
		messagePost( &tMsg );

		onDragStop( getEventDispatcher()->getMousePos(), getEventDispatcher()->getPressTrigger() );

		bool enabled = isEnabled();
		mEnabled = false;
		Node* found = getUISceneNode()->overFind( getEventDispatcher()->getMousePosf() );
		if ( found && found->isUINode() ) {
			NodeDropMessage msg( found, NodeMessage::Drop, this );
			found->messagePost( &msg );
			found->asType<UINode>()->onDrop( this );
		}
		mEnabled = enabled;
	}
}

void UINode::startDragging( const Vector2f& position ) {
	setDragging( true );

	if ( NULL != getEventDispatcher() )
		getEventDispatcher()->setNodeDragging( this );

	mDragPoint = position;
}

bool UINode::ownsChildPosition() const {
	return 0 != ( mFlags & UI_OWNS_CHILDS_POSITION );
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

Node* UINode::setFocus( NodeFocusReason reason ) {
	if ( NULL != getEventDispatcher() )
		getEventDispatcher()->setFocusNode( this, reason );
	return this;
}

Float UINode::getPropertyRelativeTargetContainerLength(
	const CSS::PropertyRelativeTarget& relativeTarget, const Float& defaultValue,
	const Uint32& propertyIndex ) const {
	Float containerLength = defaultValue;
	switch ( relativeTarget ) {
		case PropertyRelativeTarget::ContainingBlockWidth:
			containerLength = getParent() ? getParent()->getPixelsSize().getWidth() : 0;
			break;
		case PropertyRelativeTarget::ContainingBlockHeight:
			containerLength = getParent() ? getParent()->getPixelsSize().getHeight() : 0;
			break;
		case PropertyRelativeTarget::LocalBlockWidth:
			containerLength = getPixelsSize().getWidth();
			break;
		case PropertyRelativeTarget::LocalBlockHeight:
			containerLength = getPixelsSize().getHeight();
			break;
		case PropertyRelativeTarget::BackgroundWidth:
			containerLength =
				getPixelsSize().getWidth() -
				getBackground()->getLayer( propertyIndex )->getDrawableSize().getWidth();
			break;
		case PropertyRelativeTarget::BackgroundHeight:
			containerLength =
				getPixelsSize().getHeight() -
				getBackground()->getLayer( propertyIndex )->getDrawableSize().getHeight();
			break;
		case PropertyRelativeTarget::ForegroundWidth:
			containerLength =
				getPixelsSize().getWidth() -
				getForeground()->getLayer( propertyIndex )->getDrawableSize().getWidth();
			break;
		case PropertyRelativeTarget::ForegroundHeight:
			containerLength =
				getPixelsSize().getHeight() -
				getForeground()->getLayer( propertyIndex )->getDrawableSize().getHeight();
			break;
		case PropertyRelativeTarget::LocalBlockRadiusWidth:
			containerLength = getPixelsSize().getWidth() * 0.5f;
			break;
		case PropertyRelativeTarget::LocalBlockRadiusHeight:
			containerLength = getPixelsSize().getHeight() * 0.5f;
			break;
		case PropertyRelativeTarget::None:
		default:
			break;
	}
	return containerLength;
}

Float UINode::lengthFromValue( const std::string& value,
							   const PropertyRelativeTarget& relativeTarget,
							   const Float& defaultValue, const Uint32& propertyIndex ) const {
	Float containerLength =
		getPropertyRelativeTargetContainerLength( relativeTarget, defaultValue, propertyIndex );
	return convertLength( CSS::StyleSheetLength( value, defaultValue ), containerLength );
}

Float UINode::lengthFromValue( const CSS::StyleSheetProperty& property,
							   const Float& defaultValue ) {
	return lengthFromValue( property.getValue(),
							property.getPropertyDefinition()->getRelativeTarget(), defaultValue,
							property.getIndex() );
}

Float UINode::lengthFromValueAsDp( const std::string& value,
								   const PropertyRelativeTarget& relativeTarget,
								   const Float& defaultValue, const Uint32& propertyIndex ) const {
	Float containerLength =
		getPropertyRelativeTargetContainerLength( relativeTarget, defaultValue, propertyIndex );
	return convertLengthAsDp( CSS::StyleSheetLength::fromString( value, defaultValue ),
							  containerLength );
}

Float UINode::lengthFromValueAsDp( const CSS::StyleSheetProperty& property,
								   const Float& defaultValue ) const {
	return lengthFromValueAsDp( property.getValue(),
								property.getPropertyDefinition()->getRelativeTarget(), defaultValue,
								property.getIndex() );
}

Uint32 UINode::onFocus( NodeFocusReason reason ) {
	pushState( UIState::StateFocus );

	return Node::onFocus( reason );
}

Uint32 UINode::onFocusLoss() {
	popState( UIState::StateFocus );

	return Node::onFocusLoss();
}

void UINode::onSceneChange() {
	Node::onSceneChange();
	if ( NULL != mSceneNode && mSceneNode->isUISceneNode() ) {
		mUISceneNode = static_cast<UISceneNode*>( mSceneNode );
	}
}

Float UINode::convertLength( const CSS::StyleSheetLength& length,
							 const Float& containerLength ) const {
	Float elFontSize = 12;
	Float rootFontSize = 12;

	if ( length.getUnit() == CSS::StyleSheetLength::Unit::Rem ) {
		if ( getUISceneNode() != NULL ) {
			std::string fontSizeStr( getUISceneNode()->getRoot()->getPropertyString(
				CSS::StyleSheetSpecification::instance()->getProperty(
					(Uint32)PropertyId::FontSize ) ) );
			if ( !fontSizeStr.empty() ) {
				Float num;
				if ( String::fromString( num, fontSizeStr ) ) {
					rootFontSize = num;
				}
			} else if ( NULL != getUISceneNode() &&
						NULL != getUISceneNode()->getUIThemeManager() ) {
				UIThemeManager* themeManager = getUISceneNode()->getUIThemeManager();
				if ( NULL != themeManager->getDefaultTheme() ) {
					rootFontSize = getUISceneNode()
									   ->getUIThemeManager()
									   ->getDefaultTheme()
									   ->getDefaultFontSize();
				} else {
					rootFontSize = themeManager->getDefaultFontSize();
				}
			}
		}
	} else if ( length.getUnit() == CSS::StyleSheetLength::Unit::Em ) {
		if ( isWidget() ) {
			std::string fontSizeStr( asConstType<UIWidget>()->getPropertyString(
				CSS::StyleSheetSpecification::instance()->getProperty(
					(Uint32)PropertyId::FontSize ) ) );
			if ( !fontSizeStr.empty() ) {
				Float num;
				if ( String::fromString( num, fontSizeStr ) ) {
					elFontSize = num;
				}
			} else {
				Node* node = getParent();

				while ( NULL != node ) {
					if ( node->isWidget() ) {
						fontSizeStr = node->asType<UIWidget>()->getPropertyString(
							CSS::StyleSheetSpecification::instance()->getProperty(
								(Uint32)PropertyId::FontSize ) );
						if ( !fontSizeStr.empty() ) {
							Float num;
							if ( String::fromString( num, fontSizeStr ) ) {
								elFontSize = num;
								break;
							}
						}
					}

					node = node->getParent();
				}

				if ( node == NULL ) {
					if ( NULL != getUISceneNode() &&
						 NULL != getUISceneNode()->getUIThemeManager() ) {
						UIThemeManager* themeManager = getUISceneNode()->getUIThemeManager();
						if ( NULL != themeManager->getDefaultTheme() ) {
							elFontSize = getUISceneNode()
											 ->getUIThemeManager()
											 ->getDefaultTheme()
											 ->getDefaultFontSize();
						} else {
							elFontSize = themeManager->getDefaultFontSize();
						}
					}
				}
			}
		}
	}

	return length.asPixels( containerLength, getSceneNode()->getPixelsSize(),
							getSceneNode()->getDPI(), elFontSize, rootFontSize );
}

Float UINode::convertLengthAsDp( const CSS::StyleSheetLength& length,
								 const Float& containerLength ) const {
	return PixelDensity::pxToDp( convertLength( length, containerLength ) );
}

UISceneNode* UINode::getUISceneNode() const {
	return mUISceneNode;
}

Rectf UINode::getLocalDpBounds() const {
	return Rectf( 0, 0, mDpSize.getWidth(), mDpSize.getHeight() );
}

}} // namespace EE::UI
