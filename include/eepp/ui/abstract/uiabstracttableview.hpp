#ifndef EE_UI_UIABSTRACTTABLEVIEW_HPP
#define EE_UI_UIABSTRACTTABLEVIEW_HPP

#include <eepp/math/rect.hpp>
#include <eepp/thirdparty/nlohmann/json_fwd.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitableheadercolumn.hpp>
#include <eepp/ui/uitablerow.hpp>
#include <unordered_map>

using namespace EE::Math;

namespace EE { namespace UI {
class UIPushButton;
class UILinearLayout;
class UIDropDownModelList;
class UIPopUpMenu;
}} // namespace EE::UI

namespace EE { namespace UI { namespace Abstract {

class EE_API UIAbstractTableView : public UIAbstractView {
  public:
	enum class ColumnWidthMode : Uint8 { Pixels, Percentage };

	enum TableFlags : Uint32 {
		TableFlagNone = 0,
		TableFlagHeaders = ( 1 << 0 ),
		TableFlagAutoExpand = ( 1 << 1 ),
		TableFlagAutoColumns = ( 1 << 2 ),
		TableFlagFitColumns = ( 1 << 3 ),
		TableFlagSingleClick = ( 1 << 4 ),
		TableFlagRowSearch = ( 1 << 5 ),
		TableFlagRowHeader = ( 1 << 6 ),
		TableFlagExpandersAsIcons = ( 1 << 7 ),
		TableFlagFocusOnSelection = ( 1 << 8 ),
		TableFlagDisableClipping = ( 1 << 9 ),
	};

	static const Uint32 UITABLE_DEFAULT_FLAGS =
		TableFlagHeaders | TableFlagRowSearch | TableFlagFocusOnSelection;

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	virtual Float getRowHeight() const;

	virtual Float getHeaderHeight() const;

	virtual Sizef getContentSize() const;

	bool areHeadersVisible() const;

	void setHeadersVisible( bool visible );

	bool isColumnHidden( const size_t& column ) const;

	/** Returns the header widget for a model column, or nullptr before it is created. */
	UITableHeaderColumn* getHeaderColumn( const size_t& column ) const;

	void setColumnHidden( const size_t& column, bool hidden );

	void setColumnsHidden( const std::vector<size_t>& columns, bool hidden );

	/** Enables dragging column headers to reorder them. Disabled by default. */
	void setColumnReorderingEnabled( bool enabled );

	bool isColumnReorderingEnabled() const;

	/** Model column IDs in left-to-right display order, including hidden columns. */
	const std::vector<size_t>& getColumnOrder() const;

	/** Sets a complete permutation of the current model's column IDs. Returns false if invalid. */
	bool setColumnOrder( std::vector<size_t> order );

	/** Moves a model column to a position in the complete display order. */
	bool moveColumn( size_t column, size_t position );

	virtual void selectAll();

	virtual std::vector<ModelIndex> getSelectionRange( const ModelIndex& start,
													   const ModelIndex& end ) const;

	const Float& getDragBorderDistance() const;

	void setDragBorderDistance( const Float& dragBorderDistance );

	Vector2f getColumnPosition( const size_t& index );

	int visibleColumnCount() const;

	/** In pixels. */
	void setRowHeight( const Float& rowHeight );

	/** In pixels. */
	void setColumnWidth( const size_t& colIndex, const Float& width );

	void setColumnMaxWidth( const size_t& colIndex, const Float& width );

	/** In pixels. */
	void setColumnsWidth( const Float& width );

	void setColumnsMaxWidth( const Float& width );

	const Float& getColumnWidth( const size_t& colIndex ) const;

	Float getColumnWidthPercentage( const size_t& colIndex ) const;

	std::vector<Float> getColumnsWidthPercentage() const;

	ColumnWidthMode getColumnWidthMode() const;

	void setColumnWidthMode( ColumnWidthMode mode, bool convertCurrentWidths = true );

	bool isColumnWidthModeMenuEnabled() const;

	void setColumnWidthModeMenuEnabled( bool enabled );

	/** Lets a view append application-specific items to a column header's context menu. */
	void setOnHeaderContextMenuCb( std::function<void( UIPopUpMenu*, size_t )> callback ) {
		mOnHeaderContextMenuCb = std::move( callback );
	}

	void setColumnWidthPercentage( const size_t& colIndex, Float percentage );

	void setColumnsWidthPercentage( const std::vector<Float>& percentages );

	/** Serializes the column width mode and widths. Pixel widths are stored as dp. */
	nlohmann::json serializeColumnWidths() const;

	/** Restores column widths serialized by serializeColumnWidths(). Legacy percentage arrays are
	 * also accepted. Percentage values may be partial or contain surplus entries; pixel widths must
	 * match the model. Returns false if the data is invalid or incompatible with the model. */
	bool unserializeColumnWidths( const nlohmann::json& widths );

	virtual Float getMaxColumnContentWidth( const size_t& colIndex, bool bestGuess = false );

	bool getAutoExpandOnSingleColumn() const;

	void setAutoExpandOnSingleColumn( bool autoExpandOnSingleColumn );

	void columnResizeToContent( const size_t& colIndex );

	Float getContentSpaceWidth() const;

	void moveSelection( int steps );

	virtual void setSelection( const ModelIndex& index, bool scrollToSelection = true,
							   bool openModelIndexTree = false );

	const size_t& getIconSize() const;

	void setIconSize( const size_t& iconSize );

	const size_t& getSortIconSize() const;

	void setSortIconSize( const size_t& sortIconSize );

	void setColumnsVisible( const std::vector<size_t>& columns );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	bool getRowSearchByName() const;

	void setRowSearchByName( bool rowSearchByName );

	bool getAutoColumnsWidth() const;

	void setAutoColumnsWidth( bool autoColumnsWidth );

	/** Sorts @p colIndex through the model and reflects it in the header indicator, as if the user
	 *  had clicked that column header. */
	virtual void sortByColumn( const size_t& colIndex, const SortOrder& sortOrder );

	const size_t& getMainColumn() const;

	/** The main column is the column that should be prioritized to occupy as much space as
	 * possible. */
	void setMainColumn( const size_t& mainColumn );

	bool getSingleClickNavigation() const;

	void setSingleClickNavigation( bool singleClickNavigation );

	bool getFitAllColumnsToWidget() const;

	/** Tries to make all columns visible in the widget content. */
	void setFitAllColumnsToWidget( bool fitAllColumnsToWidget );

	Uint32 getTableFlags() const;

	virtual void setTableFlags( Uint32 flags );

	void recalculateColumnsWidth();

	UITableCell* getCellFromIndex( const ModelIndex& index ) const;

	virtual void onOpenModelIndex( const ModelIndex& index, const Event* triggerEvent = nullptr );

	virtual void onOpenMenuModelIndex( const ModelIndex& index,
									   const Event* triggerEvent = nullptr );

	bool isRowHeaderVisible() const;

	void setRowHeaderVisible( bool rowHeaderVisible );

	Float getRowHeaderWidth() const;

	void setRowHeaderWidth( Float rowHeaderWidth );

	bool hasOnUpdateCellCb() const;

	void setOnUpdateCellCb( const std::function<void( UITableCell*, Model* )>& onUpdateCellCb );

	bool hasSetupCellCb() const;

	void setSetupCellCb( const std::function<void( UITableCell* )>& onSetupCellCb );

	void resetSearchText();

  protected:
	friend class EE::UI::UITableHeaderColumn;
	friend class EE::UI::UIDropDownModelList;

	struct ColumnData {
		Float minWidth{ 0 };
		Float minHeight{ 0 };
		Float maxWidth{ 0 };
		Float width{ 0 };
		Float percentage{ 0 };
		bool visible{ true };
		bool manuallySet{ false };
		UIPushButton* widget{ nullptr };

		void setWidth( Float w, bool manuallySet = false ) {
			width = w;
			this->manuallySet = manuallySet;
		}
	};

	Float mRowHeight{ 0 };
	Float mHeaderHeight{ 16 };
	mutable std::vector<UITableRow*> mRows;
	mutable std::vector<ColumnData> mColumn;
	// Visual order only. ColumnData and ModelIndex remain keyed by model column ID.
	std::vector<size_t> mColumnOrder;
	mutable std::vector<UnorderedMap<int, UIWidget*>> mWidgets;
	UILinearLayout* mHeader{ nullptr };
	UILinearLayout* mRowHeader{ nullptr };
	Float mDragBorderDistance{ 8 };
	size_t mIconSize{ 12 };
	size_t mSortIconSize{ 16 };
	bool mAutoExpandOnSingleColumn{ false };
	bool mAutoColumnsWidth{ false };
	bool mRowSearchByName{ true };
	bool mSingleClickNavigation{ false };
	bool mFitAllColumnsToWidget{ false };
	Action* mSearchTextAction{ nullptr };
	std::string mSearchText;
	size_t mMainColumn{ 0 };
	std::unordered_map<UIWidget*, std::vector<Uint32>> mWidgetsClickCbId;
	std::function<void( UITableCell*, Model* )> mOnUpdateCellCb;
	std::function<void( UITableCell* )> mSetupCellCb;
	std::function<void( UIPopUpMenu*, size_t )> mOnHeaderContextMenuCb;
	Float mRowHeaderWidth{ 0 };
	Uint32 mTableFlags{ UITABLE_DEFAULT_FLAGS };
	ColumnWidthMode mColumnWidthMode{ ColumnWidthMode::Pixels };
	bool mColumnWidthModeMenuEnabled{ false };
	bool mColumnReorderingEnabled{ false };
	bool mUpdatingColumnsForScrollbars{ false };
	bool mAutoExpandedColumnUsesVerticalScroll{ false };
	std::string mPendingSerializedColumnWidths;
	// Last sort state drawn in the header, so the indicator is only touched when it changes.
	int mSortIndicatorColumn{ -1 };
	SortOrder mSortIndicatorOrder{ SortOrder::None };

	virtual ~UIAbstractTableView();

	UIAbstractTableView( const std::string& tag );

	ColumnData& columnData( const size_t& column ) const;

	void applyColumnOrder();

	void reorderColumnAt( size_t column, Float centerX );

	virtual size_t getItemCount() const;

	virtual void onModelUpdate( unsigned flags );

	virtual void createOrUpdateColumns( bool resetColumnData = false );

	virtual void onSizeChange();

	virtual void onColumnSizeChange( const size_t& colIndex, bool fromUserInteraction = false );

	virtual void onColumnResizeToContent( const size_t& colIndex );

	virtual void updateColumnsWidth();

	void restorePendingColumnWidths();

	virtual Uint32 onFocus( NodeFocusReason reason );

	virtual Uint32 onFocusLoss();

	virtual UITableRow* createRow();

	virtual UITableRow* updateRow( const int& rowIndex, const ModelIndex& index,
								   const Float& yOffset );

	virtual UIWidget* updateCell( const Vector2<Int64>& posIndex, const ModelIndex& index,
								  const size_t& indentLevel, const Float& yOffset );

	virtual void updateTableCellData( UITableCell* cell, const ModelIndex& index );

	virtual UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index );

	virtual UIWidget* setupCell( UITableCell* widget, UIWidget* rowWidget,
								 const ModelIndex& index );

	virtual void onScrollChange();

	virtual void onContentSizeChange();

	virtual void onRowCreated( UITableRow* row );

	virtual void onSortColumn( const size_t& colIndex );

	/** Draws the sort indicator on @p colIndex, clearing any indicator left on another column.
	 *  Pass SortOrder::None to clear the indicator entirely. */
	void applySortIndicator( const size_t& colIndex, const SortOrder& sortOrder );

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual void bindNavigationClick( UIWidget* widget );

	bool tryBeginEditing( KeyBindings::Shortcut shortcut );

	void updateHeaderSize();

	int visibleColumn();

	void resetColumnData();

	void updatePercentageColumnWidths();

	int adjacentVisibleColumn( size_t column ) const;

	void buildRowHeader();

	void updateRowHeader( int realRowIndex, const ModelIndex& index, Float yOffset );
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTTABLEVIEW_HPP
