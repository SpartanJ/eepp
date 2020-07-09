#ifndef EE_UI_UIABSTRACTTABLEVIEW_HPP
#define EE_UI_UIABSTRACTTABLEVIEW_HPP

#include <eepp/math/rect.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>

using namespace EE::Math;

namespace EE { namespace UI {
class UIPushButton;
class UILinearLayout;
}} // namespace EE::UI

namespace EE { namespace UI { namespace Abstract {

class EE_API UIAbstractTableView : public UIAbstractView {
  public:
	static UIAbstractTableView* New();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	virtual Float getRowHeight() const { return getHeaderHeight(); }

	virtual Float getHeaderHeight() const;

	virtual Sizef getContentSize() const;

	bool areHeadersVisible() const;

	void setHeadersVisible( bool visible );

	bool isColumnHidden( const size_t& column ) const;

	void setColumnHidden( const size_t& column, bool hidden );

	virtual void selectAll();

	const Float& getDragBorderDistance() const;

	void setDragBorderDistance( const Float& dragBorderDistance );

	Vector2f getColumnPosition( const size_t& index );

  protected:
	friend class UITableHeaderColumn;

	virtual ~UIAbstractTableView();

	UIAbstractTableView( const std::string& tag );

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

	void updateHeaderSize();

	UILinearLayout* mHeader;
	Float mDragBorderDistance{8};
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTTABLEVIEW_HPP
