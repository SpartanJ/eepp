#ifndef EE_UI_UIABSTRACTTABLEVIEW_HPP
#define EE_UI_UIABSTRACTTABLEVIEW_HPP

#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/math/rect.hpp>

using namespace EE::Math;

namespace EE { namespace UI { namespace Abstract {

class UITableCellDrawDelegate {
  public:
	virtual ~UITableCellDrawDelegate() {}

	virtual void draw( const Rectf&, const Model&, const ModelIndex& ) = 0;
};

class EE_API UIAbstractTableView : public UIAbstractView {
  public:
	int item_height() const { return 16; }

	bool alternating_row_colors() const { return m_alternating_row_colors; }
	void set_alternating_row_colors( bool b ) { m_alternating_row_colors = b; }

	int header_height() const { return m_headers_visible ? 16 : 0; }

	bool headers_visible() const { return m_headers_visible; }
	void set_headers_visible( bool headers_visible ) { m_headers_visible = headers_visible; }

	bool is_column_hidden( int ) const;
	void set_column_hidden( int, bool );

	void set_cell_painting_delegate( int column, UITableCellDrawDelegate* );

	int horizontal_padding() const { return m_horizontal_padding; }

	Vector2i adjusted_position( const Vector2i& ) const;

	virtual Rect content_rect( const ModelIndex& ) const ;
	Rect content_rect( int row, int column ) const;
	Rect row_rect( int item_index ) const;

	//void scroll_into_view( const ModelIndex&, Orientation );

	virtual ModelIndex index_at_event_position( const Vector2i&, bool& is_toggle ) const;
	virtual ModelIndex index_at_event_position( const Vector2i& ) const ;

	virtual void select_all() ;

	void move_selection( int steps );

  protected:
	virtual ~UIAbstractTableView() ;
	UIAbstractTableView();

	virtual void didUpdateModel( unsigned flags ) ;
	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );
	virtual void mousedown_event( MouseEvent& ) ;
	virtual void mousemove_event( MouseEvent& ) ;
	virtual void doubleclick_event( MouseEvent& ) ;
	virtual void leave_event( Core::Event& ) ;

	virtual void toggle_index( const ModelIndex& ) {}

	void paint_headers( Painter& );
	Rect header_rect( int column ) const;

	void update_headers();
	void set_hovered_header_index( int );

	struct ColumnData {
		int width{0};
		bool has_initialized_width{false};
		bool visibility{true};
		std::shared_ptr<Action> visibility_action;
		UITableCellDrawDelegate* cell_painting_delegate;
	};
	ColumnData& column_data( int column ) const;

	mutable std::vector<ColumnData> m_column_data;


	Rect column_resize_grabbable_rect( int ) const;
	int column_width( int ) const;
	void update_content_size();
	virtual void update_column_sizes();
	virtual int item_count() const;

  private:
	bool m_headers_visible{true};
	bool m_in_column_resize{false};
	bool m_alternating_row_colors{true};
	int m_horizontal_padding{5};
	Vector2i m_column_resize_origin;
	int m_column_resize_original_width{0};
	int m_resizing_column{-1};
	int m_pressed_column_header_index{-1};
	bool m_pressed_column_header_is_pressed{false};
	int m_hovered_column_header_index{-1};
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTTABLEVIEW_HPP
