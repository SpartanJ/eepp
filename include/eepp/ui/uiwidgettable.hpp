#ifndef EE_UICUIGENERICGRID_HPP
#define EE_UICUIGENERICGRID_HPP

#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitouchdraggablewidget.hpp>
#include <eepp/ui/uiwidgettablerow.hpp>

namespace EE { namespace UI {

class EE_API UIWidgetTable : public UITouchDraggableWidget {
  public:
	static UIWidgetTable* New();

	~UIWidgetTable();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	void add( UIWidgetTableRow* row );

	void remove( UIWidgetTableRow* row );

	void remove( std::vector<Uint32> itemsIndex );

	void remove( Uint32 itemIndex );

	UIWidgetTable* setColumnWidth( const Uint32& columnIndex, const Uint32& columnWidth );

	const Uint32& getColumnWidth( const Uint32& columnIndex ) const;

	Uint32 getCount() const;

	UIWidgetTable* setColumnsCount( const Uint32& columnsCount );

	const Uint32& getColumnsCount() const;

	UIWidgetTable* setRowHeight( const Uint32& height );

	const Uint32& getRowHeight() const;

	UIWidgetTableRow* getRow( const Uint32& rowIndex ) const;

	void setVerticalScrollMode( const ScrollBarMode& mode );

	const ScrollBarMode& getVerticalScrollMode() const;

	void setHorizontalScrollMode( const ScrollBarMode& mode );

	const ScrollBarMode& getHorizontalScrollMode() const;

	Uint32 getColumnPosition( const Uint32& columnIndex );

	UIScrollBar* getVerticalScrollBar() const;

	UIScrollBar* getHorizontalScrollBar() const;

	Uint32 getItemIndex( UIWidgetTableRow* item );

	UIWidgetTableRow* getItemSelected();

	Uint32 getItemSelectedIndex() const;

	Uint32 onMessage( const NodeMessage* Msg );

	UIItemContainer<UIWidgetTable>* getContainer() const;

	bool getSmoothScroll() const;

	UIWidgetTable* setSmoothScroll( bool smoothScroll );

	Rectf getContainerPadding() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	friend class UIItemContainer<UIWidgetTable>;
	friend class UIWidgetTableRow;

	Rectf mContainerPadding;
	UIItemContainer<UIWidgetTable>* mContainer;
	UIScrollBar* mVScrollBar;
	UIScrollBar* mHScrollBar;
	ScrollBarMode mVScrollMode;
	ScrollBarMode mHScrollMode;
	std::vector<UIWidgetTableRow*> mItems;
	Uint32 mColumnsCount;
	Uint32 mRowHeight;
	std::vector<Uint32> mColumnsWidth;
	std::vector<Uint32> mColumnsPos;
	Uint32 mTotalWidth;
	Uint32 mTotalHeight;
	Uint32 mLastPos;
	Uint32 mVisibleFirst;
	Uint32 mVisibleLast;
	Int32 mHScrollInit;
	Int32 mItemsNotVisible;
	Int32 mSelected;
	bool mSmoothScroll;
	bool mCollWidthAssigned;

	UIWidgetTable();

	void updateCells();

	void updateColumnsPos();

	void autoPadding();

	virtual void onSizeChange();

	virtual void onAlphaChange();

	virtual void onPaddingChange();

	void containerResize();

	void onScrollValueChange( const Event* Event );

	void setDefaultColumnsWidth();

	void updateScroll( bool FromScrollChange = false );

	void updateSize();

	virtual Uint32 onSelected();

	void updateVScroll();

	void updateHScroll();

	void setHScrollStep();

	void updateScrollBar();

	virtual void onTouchDragValueChange( Vector2f diff );

	virtual bool isTouchOverAllowedChilds();

	void updatePageStep();
};

}} // namespace EE::UI

#endif
