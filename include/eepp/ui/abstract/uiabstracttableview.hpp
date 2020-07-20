#ifndef EE_UI_UIABSTRACTTABLEVIEW_HPP
#define EE_UI_UIABSTRACTTABLEVIEW_HPP

#include <eepp/math/rect.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/uitableheadercolumn.hpp>

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

	void columnResizeToContent( const size_t& colIndex );

	/** In pixels. */
	void setColumnWidth( const size_t& colIndex, const Float& width );

  protected:
	friend class EE::UI::UITableHeaderColumn;

	virtual ~UIAbstractTableView();

	UIAbstractTableView( const std::string& tag );

	Float mRowHeight{0};

	struct ColumnData {
		Float width{0};
		bool visible{true};
		UIPushButton* widget{nullptr};
	};

	ColumnData& columnData( const size_t& column ) const;

	mutable std::vector<ColumnData> mColumn;

	virtual size_t getItemCount() const;

	virtual void onModelUpdate( unsigned flags );

	virtual void createOrUpdateColumns();

	virtual void onSizeChange();

	virtual void onColumnSizeChange( const size_t& colIndex );

	virtual void onColumnResizeToContent( const size_t& colIndex );

	virtual void updateColumnsWidth();

	virtual void updateScroll();

	void updateHeaderSize();

	int visibleColumn();

	UILinearLayout* mHeader;
	Float mDragBorderDistance{8};
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTTABLEVIEW_HPP
