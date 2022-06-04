#ifndef EE_UI_UIROOT_HPP
#define EE_UI_UIROOT_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIRoot : public UIWidget {
  public:
	static UIRoot* New();

  protected:
	UIRoot() : UIWidget( ":root" ) {}

	Color mDroppableHoveringColor{ Color::Transparent };

	std::string getPropertyString( const PropertyDefinition* propertyDef,
								   const Uint32& propertyIndex );

	bool applyProperty( const StyleSheetProperty& attribute );
};

}} // namespace EE::UI

#endif
