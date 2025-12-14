#ifndef EE_UICUIDROPDOWNLIST_HPP
#define EE_UICUIDROPDOWNLIST_HPP

#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class EE_API UIDropDownList : public UITextInput {
  public:
	enum class MenuWidthMode {
		DropDown,
		Contents,
		ContentsCentered,
		ExpandIfNeeded,
		ExpandIfNeededCentered
	};

	static MenuWidthMode menuWidthModeFromString( std::string_view str );

	static std::string menuWidthModeToString( MenuWidthMode rule );

	class StyleConfig {
	  public:
		Uint32 MaxNumVisibleItems = 10;
		bool PopUpToRoot = false;
		MenuWidthMode menuWidthRule{ MenuWidthMode::DropDown };
	};

	static UIDropDownList* NewWithTag( const std::string& tag );

	static UIDropDownList* New();

	UIDropDownList( const std::string& tag = "dropdownlist" );

	virtual ~UIDropDownList();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	UIListBox* getListBox() const;

	UIDropDownList* showList();

	bool getPopUpToRoot() const;

	UIDropDownList* setPopUpToRoot( bool popUpToRoot );

	Uint32 getMaxNumVisibleItems() const;

	UIDropDownList* setMaxNumVisibleItems( const Uint32& maxNumVisibleItems );

	const StyleConfig& getStyleConfig() const;

	UIDropDownList* setStyleConfig( const StyleConfig& styleConfig );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	UIDropDownList* setMenuWidthMode( MenuWidthMode rule );

	MenuWidthMode getMenuWidthMode() const;

  protected:
	friend class UIComboBox;

	StyleConfig mStyleConfig;
	UIListBox* mListBox;
	UINode* mFriendNode;
	Uint32 mListBoxCloseCb{ 0 };

	void onListBoxFocusLoss( const Event* Event );

	virtual void onItemSelected( const Event* Event );

	virtual void show();

	virtual void hide();

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	virtual void onItemClicked( const Event* Event );

	virtual void onItemKeyDown( const Event* Event );

	virtual void onWidgetClear( const Event* Event );

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual void onClassChange();

	virtual void onSizeChange();

	virtual void onAutoSize();

	virtual void onThemeLoaded();

	void setFriendNode( UINode* friendNode );

	void destroyListBox();
};

}} // namespace EE::UI

#endif
