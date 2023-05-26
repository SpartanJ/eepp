#ifndef EE_UI_UISTACKLAYOUT_HPP
#define EE_UI_UISTACKLAYOUT_HPP

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UIStackLayout : public UILayout {
  public:
	static UIStackLayout* New();

	static UIStackLayout* NewWithTag( const std::string& tag = "stacklayout" );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void updateLayout();

  protected:
	UIStackLayout();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	explicit UIStackLayout( const std::string& tag );

	void applySizePolicyOnChilds();

};

}} // namespace EE::UI

#endif // EE_UI_UISTACKLAYOUT_HPP
