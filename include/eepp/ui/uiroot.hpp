#ifndef EE_UI_UIROOT_HPP
#define EE_UI_UIROOT_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIRoot : public UIWidget {
  public:
	static UIRoot* New();

	std::string getPropertyString( const PropertyDefinition* propertyDef,
								   const Uint32& propertyIndex ) const;

	bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	Node* overFind( const Vector2f& point );

	void setChildHitTestTraversalPixelsSize( const Sizef& size );

	void clearChildHitTestTraversalPixelsSize();

	bool hasChildHitTestTraversalPixelsSize() const;

	const Sizef& getChildHitTestTraversalPixelsSize() const;

  protected:
	UIRoot();

	bool childHitTestTraversalContains( const Vector2f& point );

	Color mDroppableHoveringColor{ Color::Transparent };
	Sizef mChildHitTestTraversalPixelsSize;
	bool mHasChildHitTestTraversalPixelsSize{ false };
};

}} // namespace EE::UI

#endif
