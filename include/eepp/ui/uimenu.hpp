#ifndef EE_UICUIMENU_HPP
#define EE_UICUIMENU_HPP

#include <deque>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenuradiobutton.hpp>
#include <eepp/ui/uimenuseparator.hpp>
#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIMenu : public UIWidget {
  public:
	static UIMenu* New();

	static void findBestMenuPos( Vector2f& position, UIMenu* menu, UIMenu* parent = NULL,
								 UIMenuSubMenu* subMenu = NULL );

	UIMenu();

	virtual ~UIMenu();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UIMenuItem* add( const String& text, Drawable* icon = NULL, const String& shortcutText = "" );

	UIWidget* add( UIWidget* widget );

	UIMenuSeparator* addSeparator();

	UIMenuCheckBox* addCheckBox( const String& text, const bool& active = false,
								 const String& shortcutText = "" );

	UIMenuRadioButton* addRadioButton( const String& text, const bool& active = false );

	UIMenuSubMenu* addSubMenu( const String& text, Drawable* icon = NULL, UIMenu* subMenu = NULL );

	UIWidget* getItem( const Uint32& index );

	UIMenuItem* getItem( const String& text );

	UIMenuItem* getItemId( const std::string& id );

	Uint32 getItemIndex( UIWidget* item );

	Uint32 getCount() const;

	UIWidget* getItemSelected() const;

	void remove( const Uint32& index );

	void remove( UIWidget* widget );

	void removeAll();

	void insert( const String& text, Drawable* icon, const Uint32& index );

	void insert( UIWidget* widget, const Uint32& index );

	virtual void setTheme( UITheme* theme );

	virtual bool show();

	virtual bool hide();

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	UINode* getOwnerNode() const;

	/** The owner node is the node who triggers the visibility of the menu */
	void setOwnerNode( UIWidget* ownerNode );

	void setIconMinimumSize( const Sizei& minIconSize );

	const Sizei& getIconMinimumSize() const;

	void backpropagateHide();

	const Clock& getInactiveTime() const;

  protected:
	friend class UIMenuItem;
	friend class UIMenuCheckBox;
	friend class UIMenuRadioButton;
	friend class UIMenuSubMenu;

	std::deque<UIWidget*> mItems;
	Uint32 mMaxWidth;
	Uint32 mNextPosY;
	Int32 mBiggestIcon;
	UIWidget* mItemSelected;
	Uint32 mItemSelectedIndex;
	bool mResizing;
	UIWidget* mOwnerNode;
	Sizei mIconMinSize;
	UIMenu* mCurrentSubMenu{ nullptr };
	Clock mInactiveTime;

	virtual void onSizeChange();

	void autoPadding();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void setWidgetSize( UIWidget* widget );

	void widgetsResize();

	void widgetsSetPos();

	void resizeMe();

	UIMenuItem* createMenuItem( const String& text, Drawable* icon,
								const String& shortcutText = "" );

	UIMenuCheckBox* createMenuCheckBox( const String& text, const bool& active,
										const String& shortcutText = "" );

	UIMenuRadioButton* createMenuRadioButton( const String& text, const bool& active );

	UIMenuSubMenu* createSubMenu( const String& text, Drawable* icon, UIMenu* subMenu );

	void onThemeLoaded();

	virtual void onPaddingChange();

	bool widgetCheckSize( UIWidget* widget, const bool& resize = true );

	bool isSubMenu( Node* node );

	void setItemSelected( UIWidget* Item );

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	void prevSel();

	void nextSel();

	void trySelect( UIWidget* node, bool up );

	void safeHide();

	virtual void onVisibilityChange();

	virtual void scheduledUpdate( const Time& time );

	bool isChildOrSubMenu( Node* node );

	void unselectSelected();
};

}} // namespace EE::UI

#endif
