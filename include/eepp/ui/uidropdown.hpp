#ifndef EE_UI_UIDROPDOWN_HPP
#define EE_UI_UIDROPDOWN_HPP

#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class EE_API UIDropDown : public UITextInput {
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

	struct StyleConfig {
		Uint32 MaxNumVisibleItems = 10;
		bool PopUpToRoot = false;
		MenuWidthMode menuWidthRule{ MenuWidthMode::DropDown };
	};

	virtual ~UIDropDown();

	virtual Uint32 getType() const;
	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual UIDropDown* showList();

	bool getPopUpToRoot() const;
	UIDropDown* setPopUpToRoot( bool popUpToRoot );

	Uint32 getMaxNumVisibleItems() const;
	virtual UIDropDown* setMaxNumVisibleItems( const Uint32& maxNumVisibleItems );

	const StyleConfig& getStyleConfig() const;
	UIDropDown* setStyleConfig( const StyleConfig& styleConfig );

	UIDropDown* setMenuWidthMode( MenuWidthMode rule );
	MenuWidthMode getMenuWidthMode() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );
	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;
	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	StyleConfig mStyleConfig;
	UINode* mFriendNode{ nullptr };

	UIDropDown( const std::string& tag );

	virtual UIWidget* getPopUpWidget() const;

	void onPopUpFocusLoss( const Event* Event );

	virtual void onItemSelected( const Event* Event );
	virtual void show();
	virtual void hide();

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );
	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );
	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	virtual void onItemClicked( const Event* Event );
	virtual void onItemKeyDown( const Event* Event );
	virtual void onWidgetClear( const Event* Event );
	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual void onSizeChange();
	virtual void onAutoSize();
	virtual void onThemeLoaded();

	void setFriendNode( UINode* friendNode );

	Float getPopUpWidth( Float contentsWidth ) const;
	void alignPopUp( UIWidget* widget );
};

}} // namespace EE::UI

#endif
