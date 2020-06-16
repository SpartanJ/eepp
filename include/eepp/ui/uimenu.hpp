#ifndef EE_UICUIMENU_HPP
#define EE_UICUIMENU_HPP

#include <deque>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenuseparator.hpp>
#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIMenu : public UIWidget {
  public:
	static UIMenu* New();

	static void fixMenuPos( Vector2f& position, UIMenu* Menu, UIMenu* parent = NULL,
							UIMenuSubMenu* SubMenu = NULL );

	UIMenu();

	virtual ~UIMenu();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UIMenuItem* add( const String& Text, Drawable* Icon = NULL );

	UIWidget* add( UIWidget* widget );

	UIMenuSeparator* addSeparator();

	UIMenuCheckBox* addCheckBox( const String& Text, const bool& Active = false );

	UIMenuSubMenu* addSubMenu( const String& Text, Drawable* Icon = NULL, UIMenu* SubMenu = NULL );

	UIWidget* getItem( const Uint32& Index );

	UIWidget* getItem( const String& Text );

	Uint32 getItemIndex( UIWidget* Item );

	Uint32 getCount() const;

	void remove( const Uint32& Index );

	void remove( UIWidget* Ctrl );

	void removeAll();

	void insert( const String& Text, Drawable* Icon, const Uint32& Index );

	void insert( UIWidget* Control, const Uint32& Index );

	virtual void setTheme( UITheme* Theme );

	virtual bool show();

	virtual bool hide();

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	UINode* getOwnerNode() const;

	/** The owner node is the node who triggers the visibility of the menu */
	void setOwnerNode( UINode* ownerNode );

	void setIconMinimumSize( const Sizei& minIconSize );

	const Sizei& getIconMinimumSize() const;

  protected:
	friend class UIMenuItem;
	friend class UIMenuCheckBox;
	friend class UIMenuSubMenu;

	std::deque<UIWidget*> mItems;
	Uint32 mMaxWidth;
	Uint32 mNextPosY;
	Int32 mBiggestIcon;
	UIWidget* mItemSelected;
	Uint32 mItemSelectedIndex;
	bool mClickHide;
	bool mResizing;
	Uint32 mLastTickMove;
	UINode* mOwnerNode;
	Sizei mIconMinSize;

	virtual void onSizeChange();

	void autoPadding();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void setWidgetSize( UIWidget* widget );

	void widgetsResize();

	void widgetsSetPos();

	void resizeMe();

	UIMenuItem* createMenuItem( const String& Text, Drawable* Icon );

	UIMenuCheckBox* createMenuCheckBox( const String& Text, const bool& Active );

	UIMenuSubMenu* createSubMenu( const String& Text, Drawable* Icon, UIMenu* SubMenu );

	void onThemeLoaded();

	virtual void onPaddingChange();

	bool widgetCheckSize( UIWidget* widget, const bool& Resize = true );

	bool isSubMenu( Node* Ctrl );

	void setItemSelected( UIWidget* Item );

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	void prevSel();

	void nextSel();

	void trySelect( UIWidget* Ctrl, bool Up );
};

}} // namespace EE::UI

#endif
