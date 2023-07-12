#ifndef UI_UILINEARLAYOUT_HPP
#define UI_UILINEARLAYOUT_HPP

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UILinearLayout : public UILayout {
  public:
	static UILinearLayout* NewWithTag( const std::string& tag, const UIOrientation& orientation );

	static UILinearLayout* New();

	static UILinearLayout* NewVertical();

	static UILinearLayout* NewHorizontal();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UIOrientation getOrientation() const;

	UILinearLayout* setOrientation( const UIOrientation& getOrientation );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void updateLayout();

  protected:
	UIOrientation mOrientation;

	UILinearLayout();

	UILinearLayout( const std::string& tag, const UIOrientation& orientation );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void packVertical();

	void packHorizontal();

	Sizei getTotalUsedSize();

	void applyWidthPolicyOnChilds();

	void applyHeightPolicyOnChilds();
};

}} // namespace EE::UI

#endif
