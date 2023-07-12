#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uitouchdraggablewidget.hpp>

namespace EE { namespace UI {

UITouchDraggableWidget* UITouchDraggableWidget::New() {
	return eeNew( UITouchDraggableWidget, () );
}

UITouchDraggableWidget::UITouchDraggableWidget( const std::string& tag ) :
	UIWidget( tag ), mTouchDragDeceleration( 5.f, 5.f ) {
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
	mTouchDragDeceleration = touchDragDeceleration;
	return this;
}

void UITouchDraggableWidget::onTouchDragValueChange( Vector2f ) {}

bool UITouchDraggableWidget::isTouchOverAllowedChilds() {
	return isMouseOverMeOrChilds();
}

void UITouchDraggableWidget::scheduledUpdate( const Time& time ) {
	if ( mEnabled && mVisible && isTouchDragEnabled() && NULL != getEventDispatcher() ) {
		if ( isTouchDragging() ) {
			// Mouse Not Down
			if ( !( getEventDispatcher()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
				setTouchDragging( false );
				getEventDispatcher()->setNodeDragging( NULL );
				return;
			}

			Float ms = time.asSeconds();
			Vector2f elapsed( ms, ms );
			Vector2f Pos( getEventDispatcher()->getMousePosf() );

			if ( mTouchDragPoint != Pos ) {
				Vector2f diff = -( mTouchDragPoint - Pos );

				onTouchDragValueChange( diff );

				mTouchDragAcceleration += elapsed * diff;

				mTouchDragPoint = Pos;

				getEventDispatcher()->setNodeDragging( this );
			} else if ( mTouchDragAcceleration != Vector2f::Zero ) {
				mTouchDragAcceleration -= elapsed * mTouchDragDeceleration;
			}
		} else {
			// Deaccelerate
			if ( mTouchDragAcceleration.x != 0 || mTouchDragAcceleration.y != 0 ) {
				Float ms = getEventDispatcher()->getLastFrameTime().asSeconds();

				if ( 0 != mTouchDragAcceleration.x ) {
					bool wasPositiveX = mTouchDragAcceleration.x >= 0;

					if ( mTouchDragAcceleration.x > 0 )
						mTouchDragAcceleration.x -= mTouchDragDeceleration.x * ms;
					else
						mTouchDragAcceleration.x += mTouchDragDeceleration.x * ms;

					if ( wasPositiveX && mTouchDragAcceleration.x < 0 )
						mTouchDragAcceleration.x = 0;
					else if ( !wasPositiveX && mTouchDragAcceleration.x > 0 )
						mTouchDragAcceleration.x = 0;
				}

				if ( 0 != mTouchDragAcceleration.y ) {
					bool wasPositiveY = mTouchDragAcceleration.y >= 0;

					if ( mTouchDragAcceleration.y > 0 )
						mTouchDragAcceleration.y -= mTouchDragDeceleration.y * ms;
					else
						mTouchDragAcceleration.y += mTouchDragDeceleration.y * ms;

					if ( wasPositiveY && mTouchDragAcceleration.y < 0 )
						mTouchDragAcceleration.y = 0;
					else if ( !wasPositiveY && mTouchDragAcceleration.y > 0 )
						mTouchDragAcceleration.y = 0;
				}

				onTouchDragValueChange( mTouchDragAcceleration );
			}
		}
	}
}

Uint32 UITouchDraggableWidget::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::MouseDown && ( msg->getFlags() & EE_BUTTON_LMASK ) &&
		 !isTouchDragging() && isTouchOverAllowedChilds() &&
		 !getEventDispatcher()->isNodeDragging() && isTouchDragEnabled() ) {
		setTouchDragging( true );
		getEventDispatcher()->setNodeDragging( this );
		mTouchDragPoint = getEventDispatcher()->getMousePosf();
		mTouchDragAcceleration = Vector2f( 0, 0 );
		return 1;
	} else if ( msg->getMsg() == NodeMessage::MouseUp && ( msg->getFlags() & EE_BUTTON_LMASK ) &&
				isTouchDragging() && isTouchOverAllowedChilds() ) {
		setTouchDragging( false );
		getEventDispatcher()->setNodeDragging( nullptr );
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
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITouchDraggableWidget::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::TouchDrag, PropertyId::TouchDragDeceleration };
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
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

}} // namespace EE::UI
