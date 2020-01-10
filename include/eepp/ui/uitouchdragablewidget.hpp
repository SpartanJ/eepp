#ifndef EE_UITOUCHDRAGABLEWIDGET_HPP
#define EE_UITOUCHDRAGABLEWIDGET_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UITouchDragableWidget : public UIWidget {
  public:
	static UITouchDragableWidget* New();

	UITouchDragableWidget();

	~UITouchDragableWidget();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	bool isTouchDragEnabled() const;

	UITouchDragableWidget* setTouchDragEnabled( const bool& enable );

	bool isTouchDragging() const;

	UITouchDragableWidget* setTouchDragging( const bool& dragging );

	Vector2f getTouchDragDeceleration() const;

	UITouchDragableWidget* setTouchDragDeceleration( const Vector2f& touchDragDeceleration );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef );

  protected:
	Vector2f mTouchDragPoint;
	Vector2f mTouchDragAcceleration;
	Vector2f mTouchDragDeceleration;

	UITouchDragableWidget( const std::string& tag );

	virtual void onTouchDragValueChange( Vector2f diff );

	virtual bool isTouchOverAllowedChilds();

	virtual void scheduledUpdate( const Time& time );
};

}} // namespace EE::UI

#endif
