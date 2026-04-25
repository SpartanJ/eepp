#ifndef EE_UI_TABLELAYOUTER_HPP
#define EE_UI_TABLELAYOUTER_HPP

#include <eepp/ui/uilayouter.hpp>
#include <eepp/core/small_vector.hpp>

namespace EE { namespace UI {

class UIHTMLTableRow;
class UIHTMLTableCell;
class UIHTMLTableHead;
class UIHTMLTableBody;
class UIHTMLTableFooter;

enum class TableLayout { Auto, Fixed };

class EE_API TableLayouter : public UILayouter {
  public:
	TableLayouter( UIWidget* container ) : UILayouter( container ) {}
	void updateLayout() override;
	void computeIntrinsicWidths() override;

	void setTableLayout( TableLayout layout );
	TableLayout getTableLayout() const;
	void setCellpadding( Float padding );
	Float getCellpadding() const;
	void setCellspacing( Float spacing );
	Float getCellspacing() const;

	Float getMinIntrinsicWidth() override;
	Float getMaxIntrinsicWidth() override;

  protected:
	SmallVector<UIHTMLTableRow*> mRows;
	SmallVector<Float> mColWidths;
	SmallVector<UIHTMLTableCell*> mCells;
	SmallVector<Uint32> mRowCellOffsets;
	SmallVector<Float> mColMinWidths;
	SmallVector<Float> mColMaxWidths;
	SmallVector<Float> mColSpecifiedWidths;
	TableLayout mTableLayout{ TableLayout::Auto };
	UIHTMLTableHead* mHead{ nullptr };
	UIHTMLTableBody* mBody{ nullptr };
	UIHTMLTableFooter* mFooter{ nullptr };
	Float mCellpadding{ 0 };
	Float mCellspacing{ 0 };
};

}} // namespace EE::UI

#endif
