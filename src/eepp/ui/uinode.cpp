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
	mFlags( UI_CONTROL_DEFAULT_FLAGS ),
	mState( UIState::StateFlagNormal ),
	mSkinState( NULL ),
	mBackground( NULL ),
	mForeground( NULL ),
	mBorder( NULL ),
	mDragButton( EE_BUTTON_LMASK ),
	mSkinColor( Color::White ) {
	mNodeFlags |= NODE_FLAG_UINODE | NODE_FLAG_OVER_FIND_ALLOWED;

	if ( NULL != SceneManager::instance()->getUISceneNode() )
		setParent( (Node*)SceneManager::instance()->getUISceneNode()->getRoot() );
}

UINode::~UINode() {
	removeSkin();

	eeSAFE_DELETE( mBackground );
	eeSAFE_DELETE( mForeground );
	eeSAFE_DELETE( mBorder );

	if ( isDragging() )
		getEventDispatcher()->setNodeDragging( NULL );
}

void UINode::worldToNodeTranslation( Vector2f& Pos ) const {
	Node* ParentLoop = mParentCtrl;

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
	Node* ParentLoop = mParentCtrl;

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

const Vector2f& UINode::getPixelsPosition() const {
	return mPosition;
}

void UINode::setInternalSize( const Sizef& size ) {
	Sizef s( size );

	if ( s.x < mMinSize.x )
		s.x = mMinSize.x;

	if ( s.y < mMinSize.y )
		s.y = mMinSize.y;

	mDpSize = size;
	mSize = PixelDensity::dpToPx( s );
	mNodeFlags |= NODE_FLAG_POLYGON_DIRTY;
	updateCenter();
	sendCommonEvent( Event::OnSizeChange );
	invalidateDraw();
}

void UINode::setInternalPixelsSize( const Sizef& size ) {
	Sizef s( size );
	Sizef pMinSize( PixelDensity::dpToPx( mMinSize ) );

	if ( s.x < pMinSize.x )
		s.x = pMinSize.x;

	if ( s.y < pMinSize.y )
		s.y = pMinSize.y;

	mDpSize = PixelDensity::pxToDp( s ).ceil();
	mSize = s;
	mNodeFlags |= NODE_FLAG_POLYGON_DIRTY;
	updateCenter();
	sendCommonEvent( Event::OnSizeChange );
	invalidateDraw();
}

Node* UINode::setSize( const Sizef& Size ) {
	Sizef s( Size );

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

const Sizef& UINode::getMinSize() const {
	return mMinSize;
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

const Sizef& UINode::getSize() const {
	return mDpSize;
}

void UINode::drawHighlightFocus() {
	if ( NULL != getEventDispatcher() && mSceneNode->getHighlightFocus() &&
		 getEventDispatcher()->getFocusControl() == this ) {
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
		 getEventDispatcher()->getOverControl() == this ) {
		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( mSceneNode->getHighlightOverColor() );
		P.setLineWidth( PixelDensity::dpToPxI( 1 ) );
		P.drawRectangle( getScreenBounds() );
	}
}

void UINode::updateDebugData() {
	if ( NULL != mSceneNode && mSceneNode->getDrawDebugData() && isWidget() &&
		 NULL != getEventDispatcher() && getEventDispatcher()->getOverControl() == this ) {
		UIWidget* widget = asType<UIWidget>();

		String text = "Tag: " + String::fromUtf8( widget->getStyleSheetTag() ) + "\n";

		if ( !mId.empty() ) {
			text += "ID: " + mId + "\n";
		}

		if ( !widget->getStyleSheetClasses().empty() ) {
			text += "Classes: " + String::join( widget->getStyleSheetClasses(), ' ' ) + "\n";
		}

		text += String::format( "X: %.2f Y: %.2f\nW: %.2f H: %.2f", mDpPos.x, mDpPos.y, mDpSize.x,
								mDpSize.y );

		if ( widget->getPadding() != Rectf( 0, 0, 0, 0 ) ) {
			Rectf p( widget->getPadding() );
			text += String::format( "\npadding: %.2f %.2f %.2f %.2f", p.Top, p.Right, p.Bottom,
									p.Left );
		}

		if ( widget->getLayoutMargin() != Rect( 0, 0, 0, 0 ) ) {
			Rect m( widget->getLayoutMargin() );
			text += String::format( "\nmargin: %d %d %d %d", m.Top, m.Right, m.Bottom, m.Left );
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

void UINode::draw() {
	if ( 0.f != mAlpha ) {
		drawBackground();

		drawSkin();
	}
}

Uint32 UINode::onMouseDown( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != getEventDispatcher() && !getEventDispatcher()->isNodeDragging() &&
		 !( getEventDispatcher()->getLastPressTrigger() & mDragButton ) &&
		 ( Flags & mDragButton ) && isDragEnabled() && !isDragging() ) {
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
		EventDispatcher* eventDispatcher = getEventDispatcher();

		if ( !( flags /*press trigger*/ & mDragButton ) ) {
			setDragging( false );
			eventDispatcher->setNodeDragging( NULL );
			return 1;
		}

		Vector2f Pos( eefloor( position.x ), eefloor( position.y ) );

		if ( mDragPoint != Pos && ( std::abs( mDragPoint.x - Pos.x ) > 1.f ||
									std::abs( mDragPoint.y - Pos.y ) > 1.f ) ) {
			if ( onDrag( Pos, flags ) ) {
				Sizef dragDiff;

				dragDiff.x = ( Float )( mDragPoint.x - Pos.x );
				dragDiff.y = ( Float )( mDragPoint.y - Pos.y );

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

UINode* UINode::setForegroundColor( const Color& color ) {
	setForegroundFillEnabled( true )->setBackgroundColor( color );
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

UIBorderDrawable* UINode::setBorderEnabled( bool enabled ) {
	writeFlag( UI_BORDER, enabled ? 1 : 0 );

	if ( enabled && NULL == mBorder ) {
		getBorder();

		if ( NULL == mBackground ) {
			getBackground();
		}
	}

	invalidateDraw();

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
		mBackground->draw( Vector2f( mScreenPosi.x, mScreenPosi.y ), mSize.floor(), mAlpha );
	}
}

void UINode::drawForeground() {
	if ( ( mFlags & UI_FILL_FOREGROUND ) && NULL != mForeground ) {
		mForeground->draw( Vector2f( mScreenPosi.x, mScreenPosi.y ), mSize.floor(),
						   (Uint32)mAlpha );
	}
}

void UINode::drawBorder() {
	if ( ( mFlags & UI_BORDER ) && NULL != mBorder ) {
		Uint8 alpha = mBorder->getAlpha();
		mBorder->setAlpha( eemin<Uint32>( mAlpha * alpha / 255.f, 255 ) );
		mBorder->draw( mScreenPosi.asFloat(), mSize.floor() );
		mBorder->setAlpha( alpha );
	}
}

void UINode::internalDraw() {
	if ( mVisible ) {
		if ( mNodeFlags & NODE_FLAG_POSITION_DIRTY )
			updateScreenPos();

		if ( mNodeFlags & NODE_FLAG_POLYGON_DIRTY )
			updateWorldPolygon();

		matrixSet();

		clipStart();

		if ( mWorldBounds.intersect( mSceneNode->getWorldBounds() ) ) {
			draw();

			drawChilds();

			if ( 0.f != mAlpha )
				drawForeground();
		} else if ( !isClipped() ) {
			drawChilds();
		}

		clipEnd();

		drawBorder();

		drawHighlightFocus();

		drawOverNode();

		updateDebugData();

		drawBox();

		matrixUnset();
	}
}

UINodeDrawable* UINode::getBackground() {
	if ( NULL == mBackground ) {
		mBackground = UINodeDrawable::New( this );
	}

	return mBackground;
}

UINodeDrawable* UINode::getForeground() {
	if ( NULL == mForeground ) {
		mForeground = UINodeDrawable::New( this );
	}

	return mForeground;
}

UIBorderDrawable* UINode::getBorder() {
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
	setThemeSkin( Theme, "control" );
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
						   bool SkipFlags ) {
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
	invalidateDraw();
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
	const Node* Ctrl = this;

	while ( Ctrl != NULL ) {
		if ( Ctrl->isType( UI_TYPE_WINDOW ) ) {
			return static_cast<const UIWindow*>( Ctrl )->getContainer();
		} else if ( mSceneNode == Ctrl ) {
			if ( mSceneNode->isUISceneNode() ) {
				return static_cast<UISceneNode*>( mSceneNode )->getRoot();
			} else {
				return mSceneNode;
			}
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

Uint32 UINode::onDrag( const Vector2f&, const Uint32& ) {
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
	}
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

void UINode::setFocus() {
	if ( NULL != getEventDispatcher() )
		getEventDispatcher()->setFocusControl( this );
}

Float UINode::getPropertyRelativeTargetContainerLength(
	const CSS::PropertyRelativeTarget& relativeTarget, const Float& defaultValue,
	const Uint32& propertyIndex ) {
	Float containerLength = defaultValue;
	switch ( relativeTarget ) {
		case PropertyRelativeTarget::ContainingBlockWidth:
			containerLength = getParent()->getPixelsSize().getWidth();
			break;
		case PropertyRelativeTarget::ContainingBlockHeight:
			containerLength = getParent()->getPixelsSize().getHeight();
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
							   const Float& defaultValue, const Float& defaultContainerValue,
							   const Uint32& propertyIndex ) {
	Float containerLength =
		getPropertyRelativeTargetContainerLength( relativeTarget, defaultValue, propertyIndex );
	return convertLength( CSS::StyleSheetLength( value, defaultValue ), containerLength );
}

Float UINode::lengthFromValue( const CSS::StyleSheetProperty& property, const Float& defaultValue,
							   const Float& defaultContainerValue ) {
	return lengthFromValue( property.getValue(),
							property.getPropertyDefinition()->getRelativeTarget(), defaultValue,
							defaultContainerValue, property.getIndex() );
}

Float UINode::lengthFromValueAsDp( const std::string& value,
								   const PropertyRelativeTarget& relativeTarget,
								   const Float& defaultValue, const Float& defaultContainerValue,
								   const Uint32& propertyIndex ) {
	Float containerLength =
		getPropertyRelativeTargetContainerLength( relativeTarget, defaultValue, propertyIndex );
	return convertLengthAsDp( CSS::StyleSheetLength::fromString( value, defaultValue ),
							  containerLength );
}

Float UINode::lengthFromValueAsDp( const CSS::StyleSheetProperty& property,
								   const Float& defaultValue, const Float& defaultContainerValue ) {
	return lengthFromValueAsDp( property.getValue(),
								property.getPropertyDefinition()->getRelativeTarget(), defaultValue,
								defaultContainerValue, property.getIndex() );
}

Uint32 UINode::onFocus() {
	pushState( UIState::StateFocus );

	return Node::onFocus();
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

Float UINode::convertLength( const CSS::StyleSheetLength& length, const Float& containerLength ) {
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
			std::string fontSizeStr( asType<UIWidget>()->getPropertyString(
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
						std::string fontSizeStr( node->asType<UIWidget>()->getPropertyString(
							CSS::StyleSheetSpecification::instance()->getProperty(
								(Uint32)PropertyId::FontSize ) ) );
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
								 const Float& containerLength ) {
	return PixelDensity::pxToDp( convertLength( length, containerLength ) );
}

UISceneNode* UINode::getUISceneNode() {
	return mUISceneNode;
}

}} // namespace EE::UI
