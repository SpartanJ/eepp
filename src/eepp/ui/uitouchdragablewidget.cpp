#include <eepp/ui/uitouchdragablewidget.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITouchDragableWidget * UITouchDragableWidget::New() {
	return eeNew( UITouchDragableWidget, () );
}

UITouchDragableWidget::UITouchDragableWidget() :
	UIWidget(),
	mTouchDragDeceleration( 5.f, 5.f )
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
	return 0 != ( mControlFlags & UI_CTRL_FLAG_TOUCH_DRAGGING );
}

UITouchDragableWidget * UITouchDragableWidget::setTouchDragging( const bool& dragging ) {
	writeCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, true == dragging );
	return this;
}

Vector2f UITouchDragableWidget::getTouchDragDeceleration() const {
	return mTouchDragDeceleration;
}

UITouchDragableWidget * UITouchDragableWidget::setTouchDragDeceleration( const Vector2f& touchDragDeceleration ) {
	mTouchDragDeceleration = touchDragDeceleration;
	return this;
}

void UITouchDragableWidget::update() {
	if ( mEnabled && mVisible ) {
		if ( mFlags & UI_TOUCH_DRAG_ENABLED ) {
			UIManager * manager = UIManager::instance();
			Uint32 Press	= manager->getPressTrigger();
			Uint32 LPress	= manager->getLastPressTrigger();

			if ( ( mControlFlags & UI_CTRL_FLAG_TOUCH_DRAGGING ) ) {
				// Mouse Not Down
				if ( !( Press & EE_BUTTON_LMASK ) ) {
					writeCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, 0 );
					manager->setControlDragging( false );
					return;
				}

				Float ms = getElapsed().asSeconds();
				Vector2f elapsed( ms, ms );
				Vector2f Pos( manager->getMousePos().x, manager->getMousePos().y );

				if ( mTouchDragPoint != Pos ) {
					Vector2f diff = -( mTouchDragPoint - Pos );

					onTouchDragValueChange( diff );

					mTouchDragAcceleration += elapsed * diff;

					mTouchDragPoint = Pos;

					manager->setControlDragging( true );

					//eePRINTL( "elapsed.x: %.4f diff: %.4f mTouchDragAcceleration: %.4f eq: %.4f", elapsed.y, diff.y, mTouchDragAcceleration.y, elapsed.y * diff.y );
				} else {
					mTouchDragAcceleration -= elapsed * mTouchDragDeceleration;

					//eePRINTL( "elapsed.x: %.2f mTouchDragAcceleration: %.4f eq: %.4f", elapsed.y, mTouchDragAcceleration.y, elapsed.y * mTouchDragDeceleration.y );
				}
			} else {
				// Mouse Down
				if ( isTouchOverAllowedChilds() ) {
					if ( !( LPress & EE_BUTTON_LMASK ) && ( Press & EE_BUTTON_LMASK ) ) {
						writeCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, 1 );

						mTouchDragPoint			= Vector2f( manager->getMousePos().x, manager->getMousePos().y );
						mTouchDragAcceleration	= Vector2f(0,0);
						//eePRINTL( "reset acceleration." );
					}
				}

				// Deaccelerate
				if ( mTouchDragAcceleration.x != 0 || mTouchDragAcceleration.y != 0 ) {
					Float ms = getElapsed().asSeconds();

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

					//eePRINTL( "mTouchDragAcceleration: %.4f deaccelerated: %.4f", mTouchDragAcceleration.y, mTouchDragDeceleration.y * ms );
				}
			}
		}
	}

	UIWidget::update();
}

void UITouchDragableWidget::onTouchDragValueChange( Vector2f diff )
{}

bool UITouchDragableWidget::isTouchOverAllowedChilds() {
	return isMouseOverMeOrChilds();
}

void UITouchDragableWidget::loadFromXmlNode(const pugi::xml_node & node) {
	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "touchdrag" == name ) {
			setTouchDragEnabled( ait->as_bool() );
		} else if ( "touchdragdeceleration" == name ) {
			setTouchDragDeceleration( Vector2f( ait->as_float(), ait->as_float() ) );
		}
	}
}

}}
