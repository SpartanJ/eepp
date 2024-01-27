#ifndef EE_UICUILISTBOX_HPP
#define EE_UICUILISTBOX_HPP

#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitouchdraggablewidget.hpp>

namespace EE { namespace UI {

class EE_API UIListBox : public UITouchDraggableWidget {
  public:
	static UIListBox* New();

	UIListBox();

	static UIListBox* NewWithTag( const std::string& tag );

	explicit UIListBox( const std::string& tag );

	virtual ~UIListBox();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	void clear();

	void addListBoxItems( std::vector<String> texts );

	Uint32 addListBoxItem( const String& Text );

	Uint32 addListBoxItem( UIListBoxItem* Item );

	Uint32 removeListBoxItem( const String& Text );

	Uint32 removeListBoxItem( UIListBoxItem* Item );

	Uint32 removeListBoxItem( Uint32 ItemIndex );

	void removeListBoxItems( std::vector<Uint32> ItemsIndex );

	virtual void setTheme( UITheme* Theme );

	bool isMultiSelect() const;

	UIScrollBar* getVerticalScrollBar() const;

	UIScrollBar* getHorizontalScrollBar() const;

	UIListBoxItem* getItem( const Uint32& Index ) const;

	Uint32 getItemIndex( UIListBoxItem* Item );

	Uint32 getItemIndex( const String& Text );

	UIListBoxItem* getItemSelected();

	String getItemSelectedText() const;

	const std::vector<String>& getItemsText() const;

	Uint32 getItemSelectedIndex() const;

	bool hasSelection() const;

	std::vector<Uint32> getItemsSelectedIndex() const;

	std::vector<UIListBoxItem*> getItemsSelected();

	Rectf getContainerPadding() const;

	void setSmoothScroll( const bool& soft );

	const bool& isSmoothScroll() const;

	void setRowHeight( const Uint32& height );

	const Uint32& getRowHeight() const;

	Uint32 getCount() const;

	bool isEmpty() const;

	void setSelected( Uint32 Index );

	void setSelected( const String& Text );

	void selectPrev();

	void selectNext();

	void setVerticalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getVerticalScrollMode() const;

	void setHorizontalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getHorizontalScrollMode() const;

	void loadFromXmlNode( const pugi::xml_node& node );

	void loadItemsFromXmlNode( const pugi::xml_node& node );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	Uint32 getMaxTextWidth() const;

	void setItemText( const Uint32& index, const String& newText );

  protected:
	friend class UIListBoxItem;
	friend class UIItemContainer<UIListBox>;
	friend class UIDropDownList;

	Uint32 mRowHeight;
	ScrollBarMode mVScrollMode;
	ScrollBarMode mHScrollMode;
	Rectf mContainerPadding;
	UIItemContainer<UIListBox>* mContainer;
	UIScrollBar* mVScrollBar;
	UIScrollBar* mHScrollBar;
	Uint32 mLastPos;
	Uint32 mMaxTextWidth;
	Int32 mHScrollInit;
	Int32 mItemsNotVisible;
	UIListBoxItem* mDummyItem;
	Uint32 mVisibleFirst;
	Uint32 mVisibleLast;

	bool mSmoothScroll;

	std::vector<Uint32> mSelected;
	std::vector<UIListBoxItem*> mItems;
	std::vector<String> mTexts;

	void updateScroll( bool fromScrollChange = false );

	void updateScrollBarState();

	void onScrollValueChange( const Event* Event );

	void onHScrollValueChange( const Event* Event );

	virtual void onSizeChange();

	virtual void onPaddingChange();

	void setRowHeight();

	void updateListBoxItemsSize();

	Uint32 getListBoxItemIndex( const String& Name );

	Uint32 getListBoxItemIndex( UIListBoxItem* Item );

	void itemClicked( UIListBoxItem* Item );

	void resetItemsStates();

	virtual Uint32 onSelected();

	void containerResize();

	void itemUpdateSize( UIListBoxItem* Item );

	void autoPadding();

	void findMaxWidth();

	UIListBoxItem* createListBoxItem( const String& Name );

	void createItemIndex( const Uint32& i );

	virtual void onAlphaChange();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	void itemKeyEvent( const KeyEvent& Event );

	void setHScrollStep();

	void updateScrollBar();

	void updatePageStep();

	virtual void onTouchDragValueChange( Vector2f diff );

	virtual bool isTouchOverAllowedChilds();
};

}} // namespace EE::UI

#endif
