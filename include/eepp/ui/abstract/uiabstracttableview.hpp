#ifndef EE_UI_UIABSTRACTTABLEVIEW_HPP
#define EE_UI_UIABSTRACTTABLEVIEW_HPP

#include <eepp/math/rect.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitableheadercolumn.hpp>
#include <eepp/ui/uitablerow.hpp>

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

	virtual Float getMaxColumnContentWidth( const size_t& colIndex );

	bool getAutoExpandOnSingleColumn() const;

	void setAutoExpandOnSingleColumn( bool autoExpandOnSingleColumn );

	void columnResizeToContent( const size_t& colIndex );

	Float getContentSpaceWidth() const;

	void moveSelection( int steps );

	const size_t& getIconSize() const;

	void setIconSize( const size_t& iconSize );

	const size_t& getSortIconSize() const;

	void setSortIconSize( const size_t& sortIconSize );

	void setColumnsVisible( const std::vector<size_t> columns );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	bool getRowSearchByName() const;

	void setRowSearchByName( bool rowSearchByName );

  protected:
	friend class EE::UI::UITableHeaderColumn;

	struct ColumnData {
		Float minWidth{0};
		Float width{0};
		bool visible{true};
		UIPushButton* widget{nullptr};
	};

	Float mRowHeight{0};
	mutable std::vector<UITableRow*> mRows;
	mutable std::vector<ColumnData> mColumn;
	mutable std::vector<std::map<int, UIWidget*>> mWidgets;
	UILinearLayout* mHeader;
	Float mDragBorderDistance{8};
	size_t mIconSize{12};
	size_t mSortIconSize{16};
	bool mAutoExpandOnSingleColumn{false};
	bool mRowSearchByName{true};
	Action* mSearchTextAction{nullptr};
	std::string mSearchText;

	virtual ~UIAbstractTableView();

	UIAbstractTableView( const std::string& tag );

	ColumnData& columnData( const size_t& column ) const;

	virtual size_t getItemCount() const;

	virtual void onModelUpdate( unsigned flags );

	virtual void createOrUpdateColumns();

	virtual void onSizeChange();

	virtual void onColumnSizeChange( const size_t& colIndex );

	virtual void onColumnResizeToContent( const size_t& colIndex );

	virtual void updateColumnsWidth();

	virtual UITableRow* createRow();

	virtual UITableRow* updateRow( const int& rowIndex, const ModelIndex& index,
								   const Float& yOffset );

	virtual UIWidget* updateCell( const int& rowIndex, const ModelIndex& index,
								  const size_t& indentLevel, const Float& yOffset );

	virtual UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index );

	virtual void onScrollChange();

	virtual void onOpenModelIndex( const ModelIndex& index );

	virtual void onSortColumn( const size_t& colIndex );

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual ModelIndex findRowWithText( const std::string& text );

	void updateHeaderSize();

	int visibleColumn();
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTTABLEVIEW_HPP
