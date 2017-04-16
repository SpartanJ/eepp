#ifndef EE_UITOUCHDRAGABLEWIDGET_HPP
#define EE_UITOUCHDRAGABLEWIDGET_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UITouchDragableWidget : public UIWidget {
	public:
		static UITouchDragableWidget * New();
		
		UITouchDragableWidget();

		virtual void update();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		bool isTouchDragEnabled() const;

		UITouchDragableWidget * setTouchDragEnabled( const bool& enable );

		bool isTouchDragging() const;

		UITouchDragableWidget * setTouchDragging( const bool& dragging );

		Vector2f getTouchDragDeceleration() const;

		UITouchDragableWidget * setTouchDragDeceleration( const Vector2f& touchDragDeceleration );
	protected:
		Vector2f mTouchDragPoint;
		Vector2f mTouchDragAcceleration;
		Vector2f mTouchDragDeceleration;

		virtual void onTouchDragValueChange( Vector2f diff );

		virtual bool isTouchOverAllowedChilds();
};

}}

#endif
