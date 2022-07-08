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

  protected:
	UIRoot();

	Color mDroppableHoveringColor{ Color::Transparent };
};

}} // namespace EE::UI

#endif
