#ifndef EE_UI_UIHTMLINPUT_HPP
#define EE_UI_UIHTMLINPUT_HPP

#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIHTMLInput : public UIHTMLWidget {
  public:
	static UIHTMLInput* New();

	UIHTMLInput();

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

	String getFormValue() const;

  protected:
	std::string mInputType{ "text" };
	UIWidget* mChildWidget{ nullptr };
	std::map<PropertyId, StyleSheetProperty> mProperties;
	String mValue;
	bool mChecked{ false };

	void createChildWidget();
	void syncCheckedState();

	virtual void onSizeChange();
};

}} // namespace EE::UI

#endif
