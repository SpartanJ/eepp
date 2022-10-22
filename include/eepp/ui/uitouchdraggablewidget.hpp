#ifndef EE_UITOUCHDRAGGABLEWIDGET_HPP
#define EE_UITOUCHDRAGGABLEWIDGET_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UITouchDraggableWidget : public UIWidget {
  public:
	static UITouchDraggableWidget* New();

	UITouchDraggableWidget();

	~UITouchDraggableWidget();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	bool isTouchDragEnabled() const;

	UITouchDraggableWidget* setTouchDragEnabled( const bool& enable );

	bool isTouchDragging() const;

	UITouchDraggableWidget* setTouchDragging( const bool& dragging );

	Vector2f getTouchDragDeceleration() const;

	UITouchDraggableWidget* setTouchDragDeceleration( const Vector2f& touchDragDeceleration );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	Vector2f mTouchDragPoint;
	Vector2f mTouchDragAcceleration;
	Vector2f mTouchDragDeceleration;

	UITouchDraggableWidget( const std::string& tag );

	virtual void onTouchDragValueChange( Vector2f diff );

	virtual bool isTouchOverAllowedChilds();

	virtual void scheduledUpdate( const Time& time );

	virtual Uint32 onMessage( const NodeMessage* msg );
};

}} // namespace EE::UI

#endif
