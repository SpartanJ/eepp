#ifndef EE_UI_UITABLEVIEW_HPP
#define EE_UI_UITABLEVIEW_HPP

#include <eepp/ui/abstract/uiabstracttableview.hpp>

using namespace EE::UI::Abstract;

namespace EE { namespace UI {

class EE_API UITableView : public UIAbstractTableView {
  public:
	static UITableView* New();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	virtual void drawChildren();

	virtual Node* overFind( const Vector2f& point );

	Float getMaxColumnContentWidth( const size_t& colIndex, bool bestGuess = false );

	virtual ModelIndex findRowWithText(
		const std::string& text, const bool& caseSensitive = false,
		FindRowWithTextMatchKind matchKind = FindRowWithTextMatchKind::StartsWith ) const;

	bool getFocusOnSelection() const;

	/** When the selection changes while the table has the keyboard focus, the selected row's
	 * cell takes it, as in UITreeView (on by default, TableFlagFocusOnSelection). Cells are
	 * recycled: without it the focused cell can show another row, and keys it handles (Enter
	 * clicks it, which selects its row) act on that row instead of the selection. */
	void setFocusOnSelection( bool focusOnSelection );

	virtual void setTableFlags( Uint32 flags );

	bool getRubberBandSelection() const;

	/** Dragging from the empty area of the table draws a rectangle that selects the rows it
	 * touches; Ctrl or Shift add them to the selection. Works only with multiple selection and
	 * row selection. Off by default (TableFlagRubberBandSelection). The rectangle is a
	 * "tableview::rubberband" child widget, styled by the theme. */
	void setRubberBandSelection( bool enabled );

	/** True when the last left press became a rubber band (until the next press): its release
	 * is not a click on the empty area. */
	bool rubberBandDragged() const { return mRubberBandDragged; }

  protected:
	Sizef mContentSize;
	bool mFocusOnSelection{ true };
	bool mFocusSelectionDirty{ false };
	bool mRubberBandSelection{ false };
	bool mRubberBandPressed{ false };
	bool mRubberBandDragged{ false };
	/** Where the band started, in content coordinates (scrolling included). */
	Vector2f mRubberBandStart;
	/** The selection kept when the band adds to it (Ctrl or Shift held at the press). */
	std::vector<ModelIndex> mRubberBandBase;
	UIWidget* mRubberBand{ nullptr };
	/** Rows the band covers (-1: none; -2: not applied yet): the selection is only rebuilt when
	 * they change. */
	Int64 mRubberBandFirst{ -2 };
	Int64 mRubberBandLast{ -2 };

	UITableView();

	UITableView( const std::string& tag );

	virtual void createOrUpdateColumns( bool resetColumnData );

	void updateContentSize();

	void onColumnSizeChange( const size_t&, bool );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual void onModelSelectionChange();

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual void scheduledUpdate( const Time& time );

	void updateRubberBand( const Vector2f& localPosition );

	void endRubberBand();
};

}} // namespace EE::UI

#endif // EE_UI_UITABLEVIEW_HPP
