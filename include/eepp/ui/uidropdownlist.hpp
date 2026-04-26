#ifndef EE_UICUIDROPDOWNLIST_HPP
#define EE_UICUIDROPDOWNLIST_HPP

#include <eepp/ui/uidropdown.hpp>
#include <eepp/ui/uilistbox.hpp>

namespace EE { namespace UI {

class EE_API UIDropDownList : public UIDropDown {
  public:
	using MenuWidthMode = UIDropDown::MenuWidthMode;
	using StyleConfig = UIDropDown::StyleConfig;

	static UIDropDownList* NewWithTag( const std::string& tag );

	static UIDropDownList* New();

	virtual ~UIDropDownList();

	virtual Uint32 getType() const;
	virtual bool isType( const Uint32& type ) const;

	UIListBox* getListBox() const;

	UIDropDownList* showList();

	virtual UIDropDownList* setMaxNumVisibleItems( const Uint32& maxNumVisibleItems );

	virtual bool applyProperty( const StyleSheetProperty& attribute );
	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;
	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual void loadFromXmlNode( const pugi::xml_node& node );

  protected:
	friend class UIComboBox;

	UIListBox* mListBox;
	Uint32 mListBoxCloseCb{ 0 };

	UIDropDownList( const std::string& tag = "dropdownlist" );

	virtual UIWidget* getPopUpWidget() const;

	virtual void onItemSelected( const Event* Event );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual void onClassChange();

	void destroyListBox();
};

}} // namespace EE::UI

#endif
