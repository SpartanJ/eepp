#include <algorithm>
#include <eepp/ui/uihtmltable.hpp>

namespace EE { namespace UI {

UIHTMLTable* UIHTMLTable::New() {
	return eeNew( UIHTMLTable, () );
}

UIHTMLTable::UIHTMLTable() : UILayout( "table" ) {
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTable::getType() const {
	return UI_TYPE_HTML_TABLE;
}

bool UIHTMLTable::isType( const Uint32& type ) const {
	return UIHTMLTable::getType() == type || UILayout::isType( type );
}

void UIHTMLTable::updateLayout() {
	if ( mPacking || !mVisible )
		return;
	mPacking = true;

	// TODO: Optimize this horrendous implementation (fix the heap-allocation crazyness)
	UIHTMLTableHead* head = nullptr;
	UIHTMLTableBody* body = nullptr;
	UIHTMLTableFooter* footer = nullptr;

	std::vector<UIHTMLTableRow*> rows;
	std::function<void( Node* )> collectRows = [&]( Node* node ) {
		Node* child = node->getFirstChild();
		while ( child ) {
			if ( child->getType() == UI_TYPE_HTML_TABLE_ROW ) {
				rows.push_back( child->asType<UIHTMLTableRow>() );
			} else if ( child->getType() != UI_TYPE_HTML_TABLE ) {
				if ( child->getType() == UI_TYPE_HTML_TABLE_HEAD )
					head = child->asType<UIHTMLTableHead>();
				else if ( child->getType() == UI_TYPE_HTML_TABLE_BODY )
					body = child->asType<UIHTMLTableBody>();
				else if ( child->getType() == UI_TYPE_HTML_TABLE_FOOTER )
					footer = child->asType<UIHTMLTableFooter>();

				collectRows( child );
			}
			child = child->getNextNode();
		}
	};
	collectRows( this );

	if ( rows.empty() ) {
		mPacking = false;
		return;
	}

	std::vector<std::vector<UIHTMLTableCell*>> grid;
	size_t maxCols = 0;
	for ( auto* row : rows ) {
		std::vector<UIHTMLTableCell*> cells;
		Node* child = row->getFirstChild();
		while ( child ) {
			if ( child->getType() == UI_TYPE_HTML_TABLE_CELL )
				cells.push_back( child->asType<UIHTMLTableCell>() );
			child = child->getNextNode();
		}
		grid.push_back( cells );
		maxCols = std::max( maxCols, cells.size() );
	}

	if ( maxCols == 0 ) {
		mPacking = false;
		return;
	}

	std::vector<Float> colWidths( maxCols, 0.f );

	// Get natural width for each column (without wrapping)
	for ( const auto& rowCells : grid ) {
		for ( size_t i = 0; i < rowCells.size(); ++i ) {
			UIHTMLTableCell* cell = rowCells[i];
			cell->setLayoutWidthPolicy( SizePolicy::WrapContent );
			cell->updateLayout();
			colWidths[i] = std::max( colWidths[i], cell->getPixelsSize().getWidth() );
		}
	}

	Float availableWidth = getPixelsSize().getWidth() - mPaddingPx.Left - mPaddingPx.Right;
	Float totalUnwrappedWidth = 0;
	for ( Float w : colWidths )
		totalUnwrappedWidth += w;

	if ( totalUnwrappedWidth > availableWidth && totalUnwrappedWidth > 0 ) {
		Float scale = availableWidth / totalUnwrappedWidth;
		for ( size_t i = 0; i < maxCols; ++i )
			colWidths[i] *= scale;
	} else if ( totalUnwrappedWidth < availableWidth && maxCols > 0 && totalUnwrappedWidth > 0 ) {
		Float scale = availableWidth / totalUnwrappedWidth;
		for ( size_t i = 0; i < maxCols; ++i )
			colWidths[i] *= scale;
	}

	Float headHeight = 0;
	Float bodyHeight = 0;
	Float footerHeight = 0;

	// Apply layout and calculate heights
	size_t rowCount = grid.size();
	for ( size_t r = 0; r < rowCount; ++r ) {
		Float rowHeight = 0;
		size_t columnCount = grid[r].size();
		for ( size_t c = 0; c < columnCount; ++c ) {
			UIHTMLTableCell* cell = grid[r][c];
			cell->setLayoutWidthPolicy( SizePolicy::Fixed );
			cell->setPixelsSize( colWidths[c], cell->getPixelsSize().getHeight() );
			cell->updateLayout();
			rowHeight = std::max( rowHeight, cell->getPixelsSize().getHeight() );
		}

		// Position cells inside the row and equalize height
		Float currentX = 0;
		for ( size_t c = 0; c < columnCount; ++c ) {
			UIHTMLTableCell* cell = grid[r][c];
			cell->setPixelsPosition( currentX, 0 );
			cell->setPixelsSize( cell->getPixelsSize().getWidth(), rowHeight );
			currentX += colWidths[c];
		}

		// Set row height and width
		UIHTMLTableRow* row = rows[r];
		row->setPixelsSize( availableWidth, rowHeight );

		if ( r == 0 ) {
			headHeight = rowHeight;
		} else if ( r == rowCount - 1 && columnCount &&
					grid[r][0]->getParent()->isType( UI_TYPE_HTML_TABLE_FOOTER ) ) {
			footerHeight = rowHeight;
		} else {
			bodyHeight += rowHeight;
		}
	}

	// Position rows vertically
	// We also need to ensure that the containers (thead, tbody, etc.) are positioned at 0,0
	// and have the correct size, so the absolute positioning of the rows works as expected.
	if ( head ) {
		head->setPixelsPosition( 0, 0 );
		head->setPixelsSize( { getPixelsSize().x, headHeight } );
	}

	if ( body ) {
		body->setPixelsPosition( 0, headHeight );
		body->setPixelsSize( { getPixelsSize().x, bodyHeight } );
	}

	if ( footer ) {
		footer->setPixelsPosition( 0, headHeight + bodyHeight );
		footer->setPixelsSize( { getPixelsSize().x, footerHeight } );
	}

	Float currentY = mPaddingPx.Top - headHeight;
	for ( auto* row : rows ) {
		row->setPixelsPosition( mPaddingPx.Left, currentY );
		currentY += row->getPixelsSize().getHeight();
	}
	if ( head && !rows.empty() )
		rows[0]->setPixelsPosition( mPaddingPx.Left, 0 );

	if ( footer && !rows.empty() )
		rows[rowCount - 1]->setPixelsPosition( mPaddingPx.Left, 0 );

	if ( mWidthPolicy == SizePolicy::MatchParent )
		setInternalPixelsWidth( getMatchParentWidth() );

	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( headHeight + bodyHeight + footerHeight + mPaddingPx.Bottom );
	} else if ( mHeightPolicy == SizePolicy::MatchParent ) {
		setInternalPixelsHeight( getMatchParentHeight() );
	}

	mPacking = false;
	mDirtyLayout = false;
}

Uint32 UIHTMLTable::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

UIHTMLTableRow* UIHTMLTableRow::New() {
	return eeNew( UIHTMLTableRow, () );
}

UIHTMLTableRow::UIHTMLTableRow() : UIWidget( "tr" ) {
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableRow::getType() const {
	return UI_TYPE_HTML_TABLE_ROW;
}

bool UIHTMLTableRow::isType( const Uint32& type ) const {
	return UIHTMLTableRow::getType() == type || UIWidget::isType( type );
}

UIHTMLTableCell* UIHTMLTableCell::New( const std::string& tag ) {
	return eeNew( UIHTMLTableCell, ( tag ) );
}

UIHTMLTableCell::UIHTMLTableCell( const std::string& tag ) : UIRichText( tag ) {
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableCell::getType() const {
	return UI_TYPE_HTML_TABLE_CELL;
}

bool UIHTMLTableCell::isType( const Uint32& type ) const {
	return UIHTMLTableCell::getType() == type || UIRichText::isType( type );
}

UIHTMLTableHead* UIHTMLTableHead::New() {
	return eeNew( UIHTMLTableHead, () );
}

UIHTMLTableHead::UIHTMLTableHead() : UIWidget( "thead" ) {
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableHead::getType() const {
	return UI_TYPE_HTML_TABLE_HEAD;
}

bool UIHTMLTableHead::isType( const Uint32& type ) const {
	return UIHTMLTableHead::getType() == type || UIWidget::isType( type );
}

UIHTMLTableBody* UIHTMLTableBody::New() {
	return eeNew( UIHTMLTableBody, () );
}

UIHTMLTableBody::UIHTMLTableBody() : UIWidget( "tbody" ) {
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableBody::getType() const {
	return UI_TYPE_HTML_TABLE_BODY;
}

bool UIHTMLTableBody::isType( const Uint32& type ) const {
	return UIHTMLTableBody::getType() == type || UIWidget::isType( type );
}

UIHTMLTableFooter* UIHTMLTableFooter::New() {
	return eeNew( UIHTMLTableFooter, () );
}

UIHTMLTableFooter::UIHTMLTableFooter() : UIWidget( "tfoot" ) {
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

Uint32 UIHTMLTableFooter::getType() const {
	return UI_TYPE_HTML_TABLE_FOOTER;
}

bool UIHTMLTableFooter::isType( const Uint32& type ) const {
	return UIHTMLTableFooter::getType() == type || UIWidget::isType( type );
}

}} // namespace EE::UI
