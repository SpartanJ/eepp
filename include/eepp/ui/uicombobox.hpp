#ifndef EE_UICUICOMBOBOX_HPP
#define EE_UICUICOMBOBOX_HPP

#include <eepp/ui/uidropdownlist.hpp>

namespace EE { namespace UI {

class EE_API UIComboBox : public UIWidget {
  public:
	static UIComboBox* New();

	UIComboBox();

	virtual ~UIComboBox();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	UIListBox* getListBox();

	UIDropDownList* getDropDownList() const { return mDropDownList; }

	UINode* getButton() const { return mButton; }

	InputTextBuffer* getInputTextBuffer();

	const String& getText();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	void loadFromXmlNode( const pugi::xml_node& node );

  protected:
	UIDropDownList* mDropDownList;
	UINode* mButton;

	Uint32 onMessage( const NodeMessage* Msg );

	void updateControls();

	virtual void onSizeChange();

	virtual void onPositionChange();

	virtual void onPaddingChange();

	virtual void onAutoSize();
};

}} // namespace EE::UI

#endif
