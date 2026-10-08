#include <cmath>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitouchdraggablewidget.hpp>

namespace EE { namespace UI {

UITouchDraggableWidget* UITouchDraggableWidget::New() {
	return eeNew( UITouchDraggableWidget, () );
}

UITouchDraggableWidget::UITouchDraggableWidget( const std::string& tag ) :
	UIWidget( tag ), mTouchDragDeceleration( 5.f, 5.f ) {
	mScrollController.setDragDeceleration( mTouchDragDeceleration );
	if ( getUISceneNode() )
		mSmoothScrollEnabled = getUISceneNode()->isSmoothScrollEnabled();
	subscribeScheduledUpdate();
}

UITouchDraggableWidget::UITouchDraggableWidget() : UITouchDraggableWidget( "touchdraggable" ) {}

UITouchDraggableWidget::~UITouchDraggableWidget() {
	if ( NULL != getEventDispatcher() && isTouchDragging() )
		getEventDispatcher()->setNodeDragging( NULL );
}

Uint32 UITouchDraggableWidget::getType() const {
	return UI_TYPE_TOUCH_DRAGGABLE_WIDGET;
}

bool UITouchDraggableWidget::isType( const Uint32& type ) const {
	return UITouchDraggableWidget::getType() == type ? true : UIWidget::isType( type );
}

bool UITouchDraggableWidget::isTouchDragEnabled() const {
	return 0 != ( mFlags & UI_TOUCH_DRAG_ENABLED );
}

UITouchDraggableWidget* UITouchDraggableWidget::setTouchDragEnabled( const bool& enable ) {
	if ( !enable && isTouchDragging() ) {
		setTouchDragging( false );
		if ( nullptr != getEventDispatcher() && getEventDispatcher()->getNodeDragging() == this )
			getEventDispatcher()->setNodeDragging( nullptr );
		stopScrollController();
	}
	writeFlag( UI_TOUCH_DRAG_ENABLED, true == enable );
	return this;
}

bool UITouchDraggableWidget::isTouchDragging() const {
	return 0 != ( mNodeFlags & NODE_FLAG_TOUCH_DRAGGING );
}

UITouchDraggableWidget* UITouchDraggableWidget::setTouchDragging( const bool& dragging ) {
	writeNodeFlag( NODE_FLAG_TOUCH_DRAGGING, true == dragging );
	return this;
}

Vector2f UITouchDraggableWidget::getTouchDragDeceleration() const {
	return mTouchDragDeceleration;
}

UITouchDraggableWidget*
UITouchDraggableWidget::setTouchDragDeceleration( const Vector2f& touchDragDeceleration ) {
	mTouchDragDeceleration = { eemax( 0.01f, touchDragDeceleration.x ),
							   eemax( 0.01f, touchDragDeceleration.y ) };
	mScrollController.setDragDeceleration( mTouchDragDeceleration );
	return this;
}

bool UITouchDraggableWidget::isSmoothScrollEnabled() const {
	return mSmoothScrollEnabled;
}

UITouchDraggableWidget* UITouchDraggableWidget::setSmoothScrollEnabled( bool enabled ) {
	if ( mSmoothScrollEnabled != enabled ) {
		mSmoothScrollEnabled = enabled;
		stopScrollController();
	}
	return this;
}

void UITouchDraggableWidget::onTouchDragValueChange( Vector2f ) {}

bool UITouchDraggableWidget::isTouchOverAllowedChildren() {
	return isMouseOverMeOrChildren();
}

bool UITouchDraggableWidget::supportsScrollController() const {
	return false;
}

Vector2f UITouchDraggableWidget::getScrollControllerPosition() const {
	return Vector2f::Zero;
}

Vector2f UITouchDraggableWidget::getScrollControllerMaxPosition() const {
	return Vector2f::Zero;
}

void UITouchDraggableWidget::setScrollControllerPosition( const Vector2f& ) {}

bool UITouchDraggableWidget::scrollBy( const Vector2f& delta, Float durationScale ) {
	if ( !supportsScrollController() )
		return false;

	const bool moved = mScrollController.scrollBy( delta, getScrollControllerPosition(),
												   getScrollControllerMaxPosition(),
												   mSmoothScrollEnabled, durationScale );
	if ( moved && !mSmoothScrollEnabled ) {
		mApplyingScrollController = true;
		setScrollControllerPosition( mScrollController.getPosition() );
		mApplyingScrollController = false;
	}
	return moved;
}

Float UITouchDraggableWidget::getWheelScrollFactor( Float offset ) {
	return eeclamp( offset, -1.f, 1.f );
}

void UITouchDraggableWidget::stopScrollController() {
	if ( supportsScrollController() ) {
		mScrollController.setPosition( getScrollControllerPosition(),
									   getScrollControllerMaxPosition() );
	} else {
		mScrollController.stop();
		mTouchDragVelocity = Vector2f::Zero;
	}
}

void UITouchDraggableWidget::scheduledUpdate( const Time& time ) {
	if ( !mEnabled || !mVisible || NULL == getEventDispatcher() )
		return;

	if ( isTouchDragEnabled() ) {
		if ( isTouchDragging() ) {
			// Mouse Not Down
			if ( !( getEventDispatcher()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
				setTouchDragging( false );
				getEventDispatcher()->setNodeDragging( NULL );
				if ( supportsScrollController() )
					mScrollController.endDrag();
			}

			if ( isTouchDragging() ) {
				const Float seconds = eemax<Float>( 0.f, time.asSeconds() );
				Vector2f pos( getEventDispatcher()->getMousePosf() );

				if ( mTouchDragPoint != pos ) {
					Vector2f diff( pos - mTouchDragPoint );

					if ( supportsScrollController() ) {
						mScrollController.dragBy( -diff, time, getScrollControllerMaxPosition() );
						mApplyingScrollController = true;
						setScrollControllerPosition( mScrollController.getPosition() );
						mApplyingScrollController = false;
					} else {
						onTouchDragValueChange( diff );
						if ( seconds > 0.f ) {
							const Vector2f velocity( diff / seconds );
							const Float blend = 1.f - std::exp( -20.f * seconds );
							mTouchDragVelocity += ( velocity - mTouchDragVelocity ) * blend;
						}
					}

					mTouchDragPoint = pos;

					getEventDispatcher()->setNodeDragging( this );
				} else if ( supportsScrollController() ) {
					mScrollController.dragBy( Vector2f::Zero, time,
											  getScrollControllerMaxPosition() );
				} else if ( mTouchDragVelocity != Vector2f::Zero ) {
					mTouchDragVelocity *= std::exp( -20.f * seconds );
				}
			}
		}
	}

	if ( !isTouchDragging() ) {
		if ( supportsScrollController() &&
			 mScrollController.update( time, getScrollControllerMaxPosition() ) ) {
			mApplyingScrollController = true;
			setScrollControllerPosition( mScrollController.getPosition() );
			mApplyingScrollController = false;
		} else if ( !supportsScrollController() && mTouchDragVelocity != Vector2f::Zero ) {
			const Float seconds = eemax<Float>( 0.f, time.asSeconds() );
			const Vector2f decay( std::exp( -mTouchDragDeceleration.x * seconds ),
								  std::exp( -mTouchDragDeceleration.y * seconds ) );
			onTouchDragValueChange(
				{ mTouchDragVelocity.x * ( 1.f - decay.x ) / mTouchDragDeceleration.x,
				  mTouchDragVelocity.y * ( 1.f - decay.y ) / mTouchDragDeceleration.y } );
			mTouchDragVelocity.x *= decay.x;
			mTouchDragVelocity.y *= decay.y;
			if ( std::abs( mTouchDragVelocity.x ) <= 1.f )
				mTouchDragVelocity.x = 0.f;
			if ( std::abs( mTouchDragVelocity.y ) <= 1.f )
				mTouchDragVelocity.y = 0.f;
		}
	}
}

Uint32 UITouchDraggableWidget::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::MouseDown && ( msg->getFlags() & EE_BUTTON_LMASK ) &&
		 !isTouchDragging() && isTouchOverAllowedChildren() &&
		 !getEventDispatcher()->isNodeDragging() && isTouchDragEnabled() ) {
		setTouchDragging( true );
		getEventDispatcher()->setNodeDragging( this );
		mTouchDragPoint = getEventDispatcher()->getMousePosf();
		mTouchDragVelocity = Vector2f::Zero;
		if ( supportsScrollController() )
			mScrollController.beginDrag( getScrollControllerPosition(),
										 getScrollControllerMaxPosition() );
		return 1;
	} else if ( msg->getMsg() == NodeMessage::MouseUp && ( msg->getFlags() & EE_BUTTON_LMASK ) &&
				isTouchDragging() && isTouchOverAllowedChildren() ) {
		setTouchDragging( false );
		getEventDispatcher()->setNodeDragging( nullptr );
		if ( supportsScrollController() )
			mScrollController.endDrag();
		return 1;
	}
	return 0;
}

std::string UITouchDraggableWidget::getPropertyString( const PropertyDefinition* propertyDef,
													   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::TouchDrag:
			return isTouchDragEnabled() ? "true" : "false";
		case PropertyId::TouchDragDeceleration:
			return String::fromFloat( getTouchDragDeceleration().x ) + " " +
				   String::fromFloat( getTouchDragDeceleration().y );
		case PropertyId::ScrollBehavior:
			return isSmoothScrollEnabled() ? "smooth" : "instant";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITouchDraggableWidget::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::TouchDrag, PropertyId::TouchDragDeceleration,
				   PropertyId::ScrollBehavior };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UITouchDraggableWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::TouchDrag:
			setTouchDragEnabled( attribute.asBool() );
			break;
		case PropertyId::TouchDragDeceleration:
			setTouchDragDeceleration( attribute.asVector2f() );
			break;
		case PropertyId::ScrollBehavior:
			setSmoothScrollEnabled( String::iequals( attribute.getValue(), "smooth" ) );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

}} // namespace EE::UI
