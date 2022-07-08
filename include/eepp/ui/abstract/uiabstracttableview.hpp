#ifndef EE_UI_UIABSTRACTTABLEVIEW_HPP
#define EE_UI_UIABSTRACTTABLEVIEW_HPP

#include <eepp/math/rect.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitableheadercolumn.hpp>
#include <eepp/ui/uitablerow.hpp>
#include <unordered_map>

using namespace EE::Math;

namespace EE { namespace UI {
class UIPushButton;
class UILinearLayout;
}} // namespace EE::UI

namespace EE { namespace UI { namespace Abstract {

class EE_API UIAbstractTableView : public UIAbstractView {
  public:
	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	virtual Float getRowHeight() const;

	virtual Float getHeaderHeight() const;

	virtual Sizef getContentSize() const;

	bool areHeadersVisible() const;

	void setHeadersVisible( bool visible );

	bool isColumnHidden( const size_t& column ) const;

	void setColumnHidden( const size_t& column, bool hidden );

	void setColumnsHidden( const std::vector<size_t> columns, bool hidden );

	virtual void selectAll();

	const Float& getDragBorderDistance() const;

	void setDragBorderDistance( const Float& dragBorderDistance );

	Vector2f getColumnPosition( const size_t& index );

	int visibleColumnCount() const;

	/** In pixels. */
	void setRowHeight( const Float& rowHeight );

	/** In pixels. */
	void setColumnWidth( const size_t& colIndex, const Float& width );

	const Float& getColumnWidth( const size_t& colIndex ) const;

	virtual Float getMaxColumnContentWidth( const size_t& colIndex, bool bestGuess = false );

	bool getAutoExpandOnSingleColumn() const;

	void setAutoExpandOnSingleColumn( bool autoExpandOnSingleColumn );

	void columnResizeToContent( const size_t& colIndex );

	Float getContentSpaceWidth() const;

	void moveSelection( int steps );

	virtual void setSelection( const ModelIndex& index, bool scrollToSelection = true );

	const size_t& getIconSize() const;

	void setIconSize( const size_t& iconSize );

	const size_t& getSortIconSize() const;

	void setSortIconSize( const size_t& sortIconSize );

	void setColumnsVisible( const std::vector<size_t> columns );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	bool getRowSearchByName() const;

	void setRowSearchByName( bool rowSearchByName );

	bool getAutoColumnsWidth() const;

	void setAutoColumnsWidth( bool autoColumnsWidth );

	const size_t& getMainColumn() const;

	/** The main column is the column that should be prioritized to occupy as much space as
	 * possible. */
	void setMainColumn( const size_t& mainColumn );

	bool getSingleClickNavigation() const;

	void setSingleClickNavigation( bool singleClickNavigation );

  protected:
	friend class EE::UI::UITableHeaderColumn;

	struct ColumnData {
		Float minWidth{ 0 };
		Float minHeight{ 0 };
		Float width{ 0 };
		bool visible{ true };
		UIPushButton* widget{ nullptr };
	};

	Float mRowHeight{ 0 };
	Float mHeaderHeight{ 16 };
	mutable std::vector<UITableRow*> mRows;
	mutable std::vector<ColumnData> mColumn;
	mutable std::vector<std::map<int, UIWidget*>> mWidgets;
	UILinearLayout* mHeader;
	Float mDragBorderDistance{ 8 };
	size_t mIconSize{ 12 };
	size_t mSortIconSize{ 16 };
	bool mAutoExpandOnSingleColumn{ false };
	bool mAutoColumnsWidth{ false };
	bool mRowSearchByName{ true };
	bool mSingleClickNavigation{ false };
	Action* mSearchTextAction{ nullptr };
	std::string mSearchText;
	size_t mMainColumn{ 0 };
	std::unordered_map<UIWidget*, Uint32> mWidgetsClickCbId;

	virtual ~UIAbstractTableView();

	UIAbstractTableView( const std::string& tag );

	ColumnData& columnData( const size_t& column ) const;

	virtual size_t getItemCount() const;

	virtual void onModelUpdate( unsigned flags );

	virtual void createOrUpdateColumns();

	virtual void onSizeChange();

	virtual void onColumnSizeChange( const size_t& colIndex, bool fromUserInteraction = false );

	virtual void onColumnResizeToContent( const size_t& colIndex );

	virtual void updateColumnsWidth();

	virtual UITableRow* createRow();

	virtual UITableRow* updateRow( const int& rowIndex, const ModelIndex& index,
								   const Float& yOffset );

	virtual UIWidget* updateCell( const int& rowIndex, const ModelIndex& index,
								  const size_t& indentLevel, const Float& yOffset );

	virtual UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index );

	virtual void onScrollChange();

	virtual void onOpenModelIndex( const ModelIndex& index, const Event* triggerEvent = nullptr );

	virtual void onOpenMenuModelIndex( const ModelIndex& index,
									   const Event* triggerEvent = nullptr );

	virtual void onSortColumn( const size_t& colIndex );

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual void bindNavigationClick( UIWidget* widget );

	void updateHeaderSize();

	int visibleColumn();
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTTABLEVIEW_HPP
