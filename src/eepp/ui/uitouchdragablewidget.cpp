#include <eepp/ui/uitouchdragablewidget.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITouchDragableWidget * UITouchDragableWidget::New() {
	return eeNew( UITouchDragableWidget, () );
}

UITouchDragableWidget::UITouchDragableWidget( const std::string& tag ) :
	UIWidget( tag ),
	mTouchDragDeceleration( 5.f, 5.f )
{}

UITouchDragableWidget::UITouchDragableWidget() :
	UITouchDragableWidget( "touchdragablewidget" )
{}

Uint32 UITouchDragableWidget::getType() const {
	return UI_TYPE_TOUCH_DRAGABLE_WIDGET;
}

bool UITouchDragableWidget::isType( const Uint32& type ) const {
	return UITouchDragableWidget::getType() == type ? true : UIWidget::isType( type );
}

bool UITouchDragableWidget::isTouchDragEnabled() const {
	return 0 != ( mFlags & UI_TOUCH_DRAG_ENABLED );
}

UITouchDragableWidget * UITouchDragableWidget::setTouchDragEnabled( const bool& enable ) {
	writeFlag( UI_TOUCH_DRAG_ENABLED, true == enable );
	return this;
}

bool UITouchDragableWidget::isTouchDragging() const {
	return 0 != ( mNodeFlags & NODE_FLAG_TOUCH_DRAGGING );
}

UITouchDragableWidget * UITouchDragableWidget::setTouchDragging( const bool& dragging ) {
	writeCtrlFlag( NODE_FLAG_TOUCH_DRAGGING, true == dragging );
	return this;
}

Vector2f UITouchDragableWidget::getTouchDragDeceleration() const {
	return mTouchDragDeceleration;
}

UITouchDragableWidget * UITouchDragableWidget::setTouchDragDeceleration( const Vector2f& touchDragDeceleration ) {
	mTouchDragDeceleration = touchDragDeceleration;
	return this;
}

void UITouchDragableWidget::update( const Time& time ) {
	if ( mEnabled && mVisible && NULL != getEventDispatcher() ) {
		if ( isTouchDragEnabled() ) {
			EventDispatcher * eventDispatcher = getEventDispatcher();
			Uint32 Press	= eventDispatcher->getPressTrigger();

			if ( isTouchDragging() ) {
				// Mouse Not Down
				if ( !( Press & EE_BUTTON_LMASK ) ) {
					setTouchDragging( false );
					eventDispatcher->setControlDragging( false );
					return;
				}

				Float ms = time.asSeconds();
				Vector2f elapsed( ms, ms );
				Vector2f Pos( eventDispatcher->getMousePosf() );

				if ( mTouchDragPoint != Pos ) {
					Vector2f diff = -( mTouchDragPoint - Pos );

					onTouchDragValueChange( diff );

					mTouchDragAcceleration += elapsed * diff;

					mTouchDragPoint = Pos;

					eventDispatcher->setControlDragging( true );
				} else {
					mTouchDragAcceleration -= elapsed * mTouchDragDeceleration;
				}
			} else {
				// Mouse Down
				if ( Press & EE_BUTTON_LMASK ) {
					if ( isTouchOverAllowedChilds() && !eventDispatcher->isControlDragging() ) {
						setTouchDragging( true );
						eventDispatcher->setControlDragging( true );

						mTouchDragPoint			= Vector2f( eventDispatcher->getMousePos().x, eventDispatcher->getMousePos().y );
						mTouchDragAcceleration	= Vector2f(0,0);
					}
				}

				// Deaccelerate
				if ( mTouchDragAcceleration.x != 0 || mTouchDragAcceleration.y != 0 ) {
					Float ms = time.asSeconds();

					if ( 0 != mTouchDragAcceleration.x ) {
						bool wasPositiveX = mTouchDragAcceleration.x >= 0;

						if ( mTouchDragAcceleration.x > 0 )
							mTouchDragAcceleration.x -= mTouchDragDeceleration.x * ms;
						else
							mTouchDragAcceleration.x += mTouchDragDeceleration.x * ms;


						if ( wasPositiveX && mTouchDragAcceleration.x < 0 ) mTouchDragAcceleration.x = 0;
						else if ( !wasPositiveX && mTouchDragAcceleration.x > 0 ) mTouchDragAcceleration.x = 0;
					}

					if ( 0 != mTouchDragAcceleration.y ) {
						bool wasPositiveY = mTouchDragAcceleration.y >= 0;

						if ( mTouchDragAcceleration.y > 0 )
							mTouchDragAcceleration.y -= mTouchDragDeceleration.y * ms;
						else
							mTouchDragAcceleration.y += mTouchDragDeceleration.y * ms;


						if ( wasPositiveY && mTouchDragAcceleration.y < 0 ) mTouchDragAcceleration.y = 0;
						else if ( !wasPositiveY && mTouchDragAcceleration.y > 0 ) mTouchDragAcceleration.y = 0;
					}

					onTouchDragValueChange( mTouchDragAcceleration );
				}
			}
		}
	}

	UIWidget::update( time );
}

void UITouchDragableWidget::onTouchDragValueChange( Vector2f )
{}

bool UITouchDragableWidget::isTouchOverAllowedChilds() {
	return isMouseOverMeOrChilds();
}

bool UITouchDragableWidget::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "touchdrag" == name ) {
		setTouchDragEnabled( attribute.asBool() );
	} else if ( "touchdragdeceleration" == name ) {
		setTouchDragDeceleration( Vector2f( attribute.asFloat(), attribute.asFloat() ) );
	} else {
		return UIWidget::setAttribute( attribute, state );
	}

	return true;
}

}}
