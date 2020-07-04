#include <eepp/ui/abstract/uiabstracttableview.hpp>

namespace EE { namespace UI { namespace Abstract {

static const int minimum_column_width = 2;

UIAbstractTableView::UIAbstractTableView() {
	set_should_hide_unnecessary_scrollbars( true );
}

UIAbstractTableView::~UIAbstractTableView() {}

void UIAbstractTableView::select_all() {
	selection().clear();
	for ( int item_index = 0; item_index < item_count(); ++item_index ) {
		auto index = model()->index( item_index );
		selection().add( index );
	}
}

void UIAbstractTableView::update_column_sizes() {
	if ( !model() )
		return;

	Model& model = *this->model();
	int column_count = model.columnCount();
	int row_count = model.rowCount();
	int key_column = model.keyColumn();

	for ( int column = 0; column < column_count; ++column ) {
		if ( is_column_hidden( column ) )
			continue;
		int header_width = header_font().width( model.column_name( column ) );
		if ( column == key_column && model.isColumnSortable( column ) )
			header_width += font().width( " \xE2\xAC\x86" ); // UPWARDS BLACK ARROW
		int column_width = header_width;
		for ( int row = 0; row < row_count; ++row ) {
			auto cell_data = model.data( model.index( row, column ) );
			int cell_width = 0;
			if ( cell_data.is_icon() ) {
				cell_width = item_height();
			} else if ( cell_data.is_bitmap() ) {
				cell_width = cell_data.as_bitmap().width();
			} else if ( cell_data.is_valid() ) {
				cell_width = font().width( cell_data.to_string() );
			}
			column_width = max( column_width, cell_width );
		}
		auto& column_data = this->column_data( column );
		column_data.width = max( column_data.width, column_width );
		column_data.has_initialized_width = true;
	}
}

void UIAbstractTableView::update_content_size() {
	if ( !model() )
		return set_content_size( {} );

	int content_width = 0;
	int column_count = model()->column_count();
	for ( int i = 0; i < column_count; ++i ) {
		if ( !is_column_hidden( i ) )
			content_width += column_width( i ) + horizontal_padding() * 2;
	}
	int content_height = item_count() * item_height();

	set_content_size( {content_width, content_height} );
	set_size_occupied_by_fixed_elements( {0, header_height()} );
}

Rect UIAbstractTableView::header_rect( int column_index ) const {
	if ( !model() )
		return {};
	if ( is_column_hidden( column_index ) )
		return {};
	int x_offset = 0;
	for ( int i = 0; i < column_index; ++i ) {
		if ( is_column_hidden( i ) )
			continue;
		x_offset += column_width( i ) + horizontal_padding() * 2;
	}
	return {x_offset, 0, column_width( column_index ) + horizontal_padding() * 2, header_height()};
}

void UIAbstractTableView::set_hovered_header_index( int index ) {
	if ( m_hovered_column_header_index == index )
		return;
	m_hovered_column_header_index = index;
	update_headers();
}

void UIAbstractTableView::paint_headers( Painter& painter ) {
	if ( !headers_visible() )
		return;
	int exposed_width = max( content_size().width(), width() );
	painter.fill_rect( {0, 0, exposed_width, header_height()}, palette().button() );
	painter.draw_line( {0, 0}, {exposed_width - 1, 0}, palette().threed_highlight() );
	painter.draw_line( {0, header_height() - 1}, {exposed_width - 1, header_height() - 1},
					   palette().threed_shadow1() );
	int x_offset = 0;
	int column_count = model()->column_count();
	for ( int column_index = 0; column_index < column_count; ++column_index ) {
		if ( is_column_hidden( column_index ) )
			continue;
		int column_width = this->column_width( column_index );
		bool is_key_column = model()->key_column() == column_index;
		Rect cell_rect( x_offset, 0, column_width + horizontal_padding() * 2, header_height() );
		bool pressed =
			column_index == m_pressed_column_header_index && m_pressed_column_header_is_pressed;
		bool hovered = column_index == m_hovered_column_header_index &&
					   model()->is_column_sortable( column_index );
		Gfx::StylePainter::paint_button( painter, cell_rect, palette(), Gfx::ButtonStyle::Normal,
										 pressed, hovered );
		String text;
		if ( is_key_column ) {
			StringBuilder builder;
			builder.append( model()->column_name( column_index ) );
			auto sort_order = model()->sort_order();
			if ( sort_order == SortOrder::Ascending )
				builder.append( " \xE2\xAC\x86" ); // UPWARDS BLACK ARROW
			else if ( sort_order == SortOrder::Descending )
				builder.append( " \xE2\xAC\x87" ); // DOWNWARDS BLACK ARROW
			text = builder.to_string();
		} else {
			text = model()->column_name( column_index );
		}
		auto text_rect = cell_rect.translated( horizontal_padding(), 0 );
		if ( pressed )
			text_rect.move_by( 1, 1 );
		painter.draw_text( text_rect, text, header_font(), Gfx::TextAlignment::CenterLeft,
						   palette().button_text() );
		x_offset += column_width + horizontal_padding() * 2;
	}
}

bool UIAbstractTableView::is_column_hidden( int column ) const {
	return !column_data( column ).visibility;
}

void UIAbstractTableView::set_column_hidden( int column, bool hidden ) {
	auto& column_data = this->column_data( column );
	if ( column_data.visibility == !hidden )
		return;
	column_data.visibility = !hidden;
	if ( column_data.visibility_action ) {
		column_data.visibility_action->set_checked( !hidden );
	}
	update_content_size();
	update();
}

void UIAbstractTableView::set_cell_painting_delegate(
	int column, UITableCellDrawDelegate* delegate ) {
	column_data( column ).cell_painting_delegate = delegate;
}

void UIAbstractTableView::update_headers() {
	Rect rect{0, 0, frame_inner_rect().width(), header_height()};
	rect.move_by( frame_thickness(), frame_thickness() );
	update( rect );
}

UIAbstractTableView::ColumnData& UIAbstractTableView::column_data( int column ) const {
	if ( static_cast<size_t>( column ) >= m_column_data.size() )
		m_column_data.resize( column + 1 );
	return m_column_data.at( column );
}

Rect UIAbstractTableView::column_resize_grabbable_rect( int column ) const {
	if ( !model() )
		return {};
	auto header_rect = this->header_rect( column );
	return {header_rect.right() - 1, header_rect.top(), 4, header_rect.height()};
}

int UIAbstractTableView::column_width( int column_index ) const {
	if ( !model() )
		return 0;
	return column_data( column_index ).width;
}

void UIAbstractTableView::mousemove_event( MouseEvent& event ) {
	if ( !model() )
		return UIAbstractView::mousemove_event( event );

	auto adjusted_position = this->adjusted_position( event.position() );
	Vector2i horizontally_adjusted_position( adjusted_position.x(), event.position().y() );

	if ( m_in_column_resize ) {
		auto delta = adjusted_position - m_column_resize_origin;
		int new_width = m_column_resize_original_width + delta.x();
		if ( new_width <= minimum_column_width )
			new_width = minimum_column_width;
		ASSERT( m_resizing_column >= 0 && m_resizing_column < model()->column_count() );
		auto& column_data = this->column_data( m_resizing_column );
		if ( column_data.width != new_width ) {
			column_data.width = new_width;
			update_content_size();
			update();
		}
		return;
	}

	if ( m_pressed_column_header_index != -1 ) {
		auto header_rect = this->header_rect( m_pressed_column_header_index );
		if ( header_rect.contains( horizontally_adjusted_position ) ) {
			set_hovered_header_index( m_pressed_column_header_index );
			if ( !m_pressed_column_header_is_pressed )
				update_headers();
			m_pressed_column_header_is_pressed = true;
		} else {
			set_hovered_header_index( -1 );
			if ( m_pressed_column_header_is_pressed )
				update_headers();
			m_pressed_column_header_is_pressed = false;
		}
		return;
	}

	if ( event.buttons() == 0 ) {
		int column_count = model()->column_count();
		bool found_hovered_header = false;
		for ( int i = 0; i < column_count; ++i ) {
			if ( column_resize_grabbable_rect( i ).contains( horizontally_adjusted_position ) ) {
				window()->set_override_cursor( StandardCursor::ResizeHorizontal );
				set_hovered_header_index( -1 );
				return;
			}
			if ( header_rect( i ).contains( horizontally_adjusted_position ) ) {
				set_hovered_header_index( i );
				found_hovered_header = true;
			}
		}
		if ( !found_hovered_header )
			set_hovered_header_index( -1 );
	}
	window()->set_override_cursor( StandardCursor::None );

	UIAbstractView::mousemove_event( event );
}

Uint32 UIAbstractTableView::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	auto adjusted_position = this->adjusted_position( event.position() );
	Vector2i horizontally_adjusted_position( adjusted_position.x(), event.position().y() );
	if ( event.button() == MouseButton::Left ) {
		if ( m_in_column_resize ) {
			if ( !column_resize_grabbable_rect( m_resizing_column )
					  .contains( horizontally_adjusted_position ) )
				window()->set_override_cursor( StandardCursor::None );
			m_in_column_resize = false;
			return;
		}
		if ( m_pressed_column_header_index != -1 ) {
			auto header_rect = this->header_rect( m_pressed_column_header_index );
			if ( header_rect.contains( horizontally_adjusted_position ) ) {
				auto new_sort_order = SortOrder::Ascending;
				if ( model()->key_column() == m_pressed_column_header_index )
					new_sort_order = model()->sort_order() == SortOrder::Ascending
										 ? SortOrder::Descending
										 : SortOrder::Ascending;
				model()->set_key_column_and_sort_order( m_pressed_column_header_index,
														new_sort_order );
			}
			m_pressed_column_header_index = -1;
			m_pressed_column_header_is_pressed = false;
			update_headers();
			return;
		}
	}

	UIAbstractView::mouseup_event( event );
}

void UIAbstractTableView::mousedown_event( MouseEvent& event ) {
	if ( !model() )
		return UIAbstractView::mousedown_event( event );

	if ( event.button() != MouseButton::Left )
		return UIAbstractView::mousedown_event( event );

	auto adjusted_position = this->adjusted_position( event.position() );
	Vector2i horizontally_adjusted_position( adjusted_position.x(), event.position().y() );

	if ( event.y() < header_height() ) {
		int column_count = model()->column_count();
		for ( int i = 0; i < column_count; ++i ) {
			if ( column_resize_grabbable_rect( i ).contains( horizontally_adjusted_position ) ) {
				m_resizing_column = i;
				m_in_column_resize = true;
				m_column_resize_original_width = column_width( i );
				m_column_resize_origin = adjusted_position;
				return;
			}
			auto header_rect = this->header_rect( i );
			if ( header_rect.contains( horizontally_adjusted_position ) &&
				 model()->is_column_sortable( i ) ) {
				m_pressed_column_header_index = i;
				m_pressed_column_header_is_pressed = true;
				update_headers();
				return;
			}
		}
		return;
	}

	bool is_toggle;
	auto index = index_at_event_position( event.position(), is_toggle );

	if ( index.is_valid() && is_toggle && model()->row_count( index ) ) {
		toggle_index( index );
		return;
	}

	UIAbstractView::mousedown_event( event );
}

ModelIndex UIAbstractTableView::index_at_event_position( const Vector2i& position,
														 bool& is_toggle ) const {
	is_toggle = false;
	if ( !model() )
		return {};

	auto adjusted_position = this->adjusted_position( position );
	for ( int row = 0, row_count = model()->row_count(); row < row_count; ++row ) {
		if ( !row_rect( row ).contains( adjusted_position ) )
			continue;
		for ( int column = 0, column_count = model()->column_count(); column < column_count;
			  ++column ) {
			if ( !content_rect( row, column ).contains( adjusted_position ) )
				continue;
			return model()->index( row, column );
		}
		return model()->index( row, 0 );
	}
	return {};
}

ModelIndex UIAbstractTableView::index_at_event_position( const Vector2i& position ) const {
	bool is_toggle;
	auto index = index_at_event_position( position, is_toggle );
	return is_toggle ? ModelIndex() : index;
}

int UIAbstractTableView::item_count() const {
	if ( !model() )
		return 0;
	return model()->row_count();
}

void UIAbstractTableView::move_selection( int steps ) {
	if ( !model() )
		return;
	auto& model = *this->model();
	ModelIndex new_index;
	if ( !selection().is_empty() ) {
		auto old_index = selection().first();
		new_index = model.index( old_index.row() + steps, old_index.column() );
	} else {
		new_index = model.index( 0, 0 );
	}
	if ( model.is_valid( new_index ) ) {
		selection().set( new_index );
		scroll_into_view( new_index, Orientation::Vertical );
		update();
	}
}

void UIAbstractTableView::scroll_into_view( const ModelIndex& index, Orientation orientation ) {
	auto rect = row_rect( index.row() ).translated( 0, -header_height() );
	ScrollableWidget::scroll_into_view( rect, orientation );
}

void UIAbstractTableView::doubleclick_event( MouseEvent& event ) {
	if ( !model() )
		return;
	if ( event.button() == MouseButton::Left ) {
		if ( event.y() < header_height() )
			return;
		if ( !selection().is_empty() ) {
			if ( is_editable() )
				begin_editing( selection().first() );
			else
				activate_selected();
		}
	}
}

void UIAbstractTableView::context_menu_event( ContextMenuEvent& event ) {
	if ( !model() )
		return;
	if ( event.position().y() < header_height() ) {
		ensure_header_context_menu().popup( event.screen_position() );
		return;
	}

	bool is_toggle;
	auto index = index_at_event_position( event.position(), is_toggle );
	if ( index.is_valid() ) {
		if ( !selection().contains( index ) )
			selection().set( index );
	} else {
		selection().clear();
	}
	if ( on_context_menu_request )
		on_context_menu_request( index, event );
}

void UIAbstractTableView::leave_event( Core::Event& event ) {
	UIAbstractView::leave_event( event );
	window()->set_override_cursor( StandardCursor::None );
	set_hovered_header_index( -1 );
}

Rect UIAbstractTableView::content_rect( int row, int column ) const {
	auto row_rect = this->row_rect( row );
	int x = 0;
	for ( int i = 0; i < column; ++i )
		x += column_width( i ) + horizontal_padding() * 2;

	return {row_rect.x() + x, row_rect.y(), column_width( column ) + horizontal_padding() * 2,
			item_height()};
}

Rect UIAbstractTableView::content_rect( const ModelIndex& index ) const {
	return content_rect( index.row(), index.column() );
}

Rect UIAbstractTableView::row_rect( int item_index ) const {
	return {0, header_height() + ( item_index * item_height() ),
			max( content_size().width(), width() ), item_height()};
}

Vector2i UIAbstractTableView::adjusted_position( const Vector2i& position ) const {
	return position.translated( horizontal_scrollbar().value() - frame_thickness(),
								vertical_scrollbar().value() - frame_thickness() );
}

void UIAbstractTableView::didUpdateModel( unsigned flags ) {
	UIAbstractView::didUpdateModel( flags );
	update_column_sizes();
	update_content_size();
	update();
}

}}} // namespace EE::UI::Abstract
