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

	int rowHeight() const { return PixelDensity::dpToPx( 16 ); }

	Float getHeaderHeight() const;

	bool headersVisible() const { return mHeadersVisible; }

	void setHeadersVisible( bool visible ) { mHeadersVisible = visible; }

	bool isColumnHidden( const size_t& column ) const;

	void setColumnHidden( const size_t& column, bool hidden );

	virtual void selectAll();

	const Float& getDragBorderDistance() const;

	void setDragBorderDistance( const Float& dragBorderDistance );

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

	virtual size_t itemCount() const;

	virtual void onModelUpdate( unsigned flags );

	virtual void createOrUpdateColumns();

	virtual void onSizeChange();

	UILinearLayout* mHeader;
	Float mDragBorderDistance{8};
	bool mHeadersVisible{true};
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTTABLEVIEW_HPP
