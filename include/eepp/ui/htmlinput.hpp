#ifndef EE_UI_HTMLINPUT_HPP
#define EE_UI_HTMLINPUT_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API HTMLInput : public UIWidget {
  public:
	static HTMLInput* New();

	HTMLInput();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	const std::string& getInputType() const;

	void setInputType( const std::string& type );

	UIWidget* getChildWidget() const;

  protected:
	std::string mInputType{ "text" };
	UIWidget* mChildWidget{ nullptr };
	std::map<PropertyId, StyleSheetProperty> mProperties;

	void createChildWidget();

	virtual void onSizeChange();
};

}} // namespace EE::UI

#endif
