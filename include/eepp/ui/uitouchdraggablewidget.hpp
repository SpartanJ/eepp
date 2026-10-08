#ifndef EE_UITOUCHDRAGGABLEWIDGET_HPP
#define EE_UITOUCHDRAGGABLEWIDGET_HPP

#include <eepp/ui/uiscrollcontroller.hpp>
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

	/** Enables interpolation of discrete wheel input. Disabled by default. */
	bool isSmoothScrollEnabled() const;

	UITouchDraggableWidget* setSmoothScrollEnabled( bool enabled );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	Vector2f mTouchDragPoint;
	Vector2f mTouchDragVelocity;
	Vector2f mTouchDragDeceleration;
	UIScrollController mScrollController;
	bool mSmoothScrollEnabled{ false };
	bool mApplyingScrollController{ false };

	UITouchDraggableWidget( const std::string& tag );

	virtual void onTouchDragValueChange( Vector2f diff );

	virtual bool isTouchOverAllowedChildren();

	/** Returns true for widgets that expose an absolute pixel scroll position to the controller. */
	virtual bool supportsScrollController() const;

	virtual Vector2f getScrollControllerPosition() const;

	virtual Vector2f getScrollControllerMaxPosition() const;

	virtual void setScrollControllerPosition( const Vector2f& position );

	/** Adds a pixel-space wheel displacement using the configured scroll behavior. */
	bool scrollBy( const Vector2f& delta, Float durationScale = 1.f );

	/** Preserves sub-tick precision without allowing one native event to exceed a legacy step. */
	static Float getWheelScrollFactor( Float offset );

	/** Synchronizes externally initiated scrolling and cancels pending motion. */
	void stopScrollController();

	virtual void scheduledUpdate( const Time& time );

	virtual Uint32 onMessage( const NodeMessage* msg );
};

}} // namespace EE::UI

#endif
