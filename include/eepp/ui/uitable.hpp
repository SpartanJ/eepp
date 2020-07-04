#ifndef EE_UICUIGENERICGRID_HPP
#define EE_UICUIGENERICGRID_HPP

#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitouchdraggablewidget.hpp>

namespace EE { namespace UI {

class EE_API UITable : public UITouchDraggableWidget {
  public:
	static UITable* New();

	UITable();

	~UITable();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	void add( UITableCell* Cell );

	void remove( UITableCell* Cell );

	void remove( std::vector<Uint32> ItemsIndex );

	void remove( Uint32 ItemIndex );

	UITable* setColumnWidth( const Uint32& ColumnIndex, const Uint32& columnWidth );

	const Uint32& getColumnWidth( const Uint32& ColumnIndex ) const;

	Uint32 getCount() const;

	UITable* setColumnsCount( const Uint32& columnsCount );

	const Uint32& getColumnsCount() const;

	UITable* setRowHeight( const Uint32& height );

	const Uint32& getRowHeight() const;

	UITableCell* getCell( const Uint32& CellIndex ) const;

	void setVerticalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getVerticalScrollMode();

	void setHorizontalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getHorizontalScrollMode();

	Uint32 getCellPosition( const Uint32& ColumnIndex );

	UIScrollBar* getVerticalScrollBar() const;

	UIScrollBar* getHorizontalScrollBar() const;

	Uint32 getItemIndex( UITableCell* Item );

	UITableCell* getItemSelected();

	Uint32 getItemSelectedIndex() const;

	Uint32 onMessage( const NodeMessage* Msg );

	UIItemContainer<UITable>* getContainer() const;

	bool getSmoothScroll() const;

	UITable* setSmoothScroll( bool smoothScroll );

	Rectf getContainerPadding() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

  protected:
	friend class UIItemContainer<UITable>;
	friend class UITableCell;

	Rectf mContainerPadding;
	UIItemContainer<UITable>* mContainer;
	UIScrollBar* mVScrollBar;
	UIScrollBar* mHScrollBar;
	ScrollBarMode mVScrollMode;
	ScrollBarMode mHScrollMode;
	std::vector<UITableCell*> mItems;
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
