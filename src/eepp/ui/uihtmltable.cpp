#include "eepp/ui/uistyle.hpp"
#include <algorithm>
#include <eepp/ui/uihtmltable.hpp>

namespace EE { namespace UI {

UIHTMLTable* UIHTMLTable::New() {
	return eeNew( UIHTMLTable, () );
}

UIHTMLTable::UIHTMLTable() : UILayout( "table" ) {
	mFlags |= UI_OWNS_CHILDREN_POSITION;
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
	setMatchParentIfNeededVerticalGrowth();

	const StyleSheetProperty* prop = nullptr;
	if ( getLayoutWidthPolicy() == SizePolicy::Fixed && mStyle &&
		 ( prop = mStyle->getProperty( PropertyId::Width ) ) ) {
		setInternalPixelsSize( { lengthFromValue( *prop, mSize.x ), mSize.getHeight() } );
	}

	UIHTMLTableHead* head = nullptr;
	UIHTMLTableBody* body = nullptr;
	UIHTMLTableFooter* footer = nullptr;

	mRows.clear();
	auto collectRows = [&]( auto self, Node* node ) -> void {
		for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() ) {
			if ( child->getType() == UI_TYPE_HTML_TABLE_ROW ) {
				mRows.push_back( child->asType<UIHTMLTableRow>() );
			} else if ( child->getType() != UI_TYPE_HTML_TABLE ) {
				if ( child->getType() == UI_TYPE_HTML_TABLE_HEAD )
					head = child->asType<UIHTMLTableHead>();
				else if ( child->getType() == UI_TYPE_HTML_TABLE_BODY )
					body = child->asType<UIHTMLTableBody>();
				else if ( child->getType() == UI_TYPE_HTML_TABLE_FOOTER )
					footer = child->asType<UIHTMLTableFooter>();

				self( self, child );
			}
		}
	};
	collectRows( collectRows, this );

	if ( mRows.empty() ) {
		mPacking = false;
		return;
	}

	mCells.clear();
	mRowCellOffsets.clear();
	mRowCellOffsets.push_back( 0 );
	size_t maxCols = 0;
	for ( auto* row : mRows ) {
		size_t colCount = 0;
		for ( Node* child = row->getFirstChild(); child; child = child->getNextNode() ) {
			if ( child->getType() == UI_TYPE_HTML_TABLE_CELL ) {
				auto* cell = child->asType<UIHTMLTableCell>();
				mCells.push_back( cell );
				colCount += cell->getColspan();
			}
		}
		mRowCellOffsets.push_back( (Uint32)mCells.size() );
		maxCols = std::max( maxCols, colCount );
	}

	if ( maxCols == 0 ) {
		mPacking = false;
		return;
	}

	mColWidths.assign( maxCols, 0.f );

	if ( maxCols == 1 ) {
		mColWidths[0] = mSize.getWidth();
	}

	// Get natural width for each column (without wrapping)
	for ( size_t r = 0; r < mRows.size(); ++r ) {
		Uint32 start = mRowCellOffsets[r];
		Uint32 end = mRowCellOffsets[r + 1];
		Uint32 colIndex = 0;
		for ( Uint32 i = 0; i < end - start; ++i ) {
			UIHTMLTableCell* cell = mCells[start + i];
			cell->setLayoutWidthPolicy( SizePolicy::WrapContent );
			cell->mSize.x = 0;
			cell->updateLayout();
			Uint32 cellColspan = cell->getColspan();
			if ( cellColspan == 1 ) {
				mColWidths[colIndex] =
					std::max( mColWidths[colIndex], cell->getPixelsSize().getWidth() );
			} else {
				Float widthPerCol = cell->getPixelsSize().getWidth() / cellColspan;
				for ( Uint32 j = 0; j < cellColspan; ++j ) {
					if ( colIndex + j < maxCols ) {
						mColWidths[colIndex + j] =
							std::max( mColWidths[colIndex + j], widthPerCol );
					}
				}
			}
			colIndex += cellColspan;
		}
	}

	Float availableWidth = getPixelsSize().getWidth() - mPaddingPx.Left - mPaddingPx.Right;
	Float totalUnwrappedWidth = 0;
	for ( Float w : mColWidths )
		totalUnwrappedWidth += w;

	if ( totalUnwrappedWidth > 0 ) {
		Float scale = availableWidth / totalUnwrappedWidth;
		for ( size_t i = 0; i < maxCols; ++i )
			mColWidths[i] *= scale;
	}

	Float headHeight = 0;
	Float bodyHeight = 0;
	Float footerHeight = 0;

	// Apply layout and calculate heights
	size_t rowCount = mRows.size();
	for ( size_t r = 0; r < rowCount; ++r ) {
		Float rowHeight = 0;
		Uint32 start = mRowCellOffsets[r];
		Uint32 end = mRowCellOffsets[r + 1];
		Uint32 columnCount = end - start;
		Uint32 colIndex = 0;
		for ( Uint32 c = 0; c < columnCount; ++c ) {
			UIHTMLTableCell* cell = mCells[start + c];
			cell->setLayoutWidthPolicy( SizePolicy::Fixed );
			Uint32 cellColspan = cell->getColspan();
			Float cellWidth = 0;
			for ( Uint32 j = 0; j < cellColspan && ( colIndex + j ) < maxCols; ++j ) {
				cellWidth += mColWidths[colIndex + j];
			}
			cell->setPixelsSize( cellWidth, cell->getPixelsSize().getHeight() );
			cell->updateLayout();
			rowHeight = std::max( rowHeight, cell->getPixelsSize().getHeight() );
			colIndex += cellColspan;
		}

		// Position cells inside the row and equalize height
		Float currentX = 0;
		colIndex = 0;
		for ( Uint32 c = 0; c < columnCount; ++c ) {
			UIHTMLTableCell* cell = mCells[start + c];
			cell->setPixelsPosition( currentX, 0 );
			Uint32 cellColspan = cell->getColspan();
			Float cellWidth = 0;
			for ( Uint32 j = 0; j < cellColspan && ( colIndex + j ) < maxCols; ++j ) {
				cellWidth += mColWidths[colIndex + j];
			}
			cell->setPixelsSize( cellWidth, rowHeight );
			currentX += cellWidth;
			colIndex += cellColspan;
		}

		// Set row height and width
		UIHTMLTableRow* row = mRows[r];
		row->setPixelsSize( availableWidth, rowHeight );

		if ( r == 0 && mCells[start]->getParent()->isType( UI_TYPE_HTML_TABLE_HEAD ) ) {
			headHeight = rowHeight;
		} else if ( r == rowCount - 1 && columnCount &&
					mCells[start]->getParent()->isType( UI_TYPE_HTML_TABLE_FOOTER ) ) {
			footerHeight = rowHeight;
		} else {
			bodyHeight += rowHeight;
		}
	}

	// Position rows vertically
	// We also need to ensure that the containers (thead, tbody, etc.) are positioned correctly and
	// have the correct size, so the absolute positioning of the rows works as expected.
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
	for ( size_t r = 0; r < rowCount; ++r ) {
		UIHTMLTableRow* row = mRows[r];
		row->setPixelsPosition( mPaddingPx.Left, currentY );
		currentY += row->getPixelsSize().getHeight();
	}

	// Reset positions if they are inside specialized containers
	if ( head && !mRows.empty() )
		mRows[0]->setPixelsPosition( mPaddingPx.Left, 0 );

	if ( footer && !mRows.empty() )
		mRows[rowCount - 1]->setPixelsPosition( mPaddingPx.Left, 0 );

	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( mPaddingPx.Top + headHeight + bodyHeight + footerHeight +
								 mPaddingPx.Bottom );
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

bool UIHTMLTableCell::applyProperty( const StyleSheetProperty& attribute ) {
	if ( attribute.getPropertyDefinition() == nullptr )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Colspan: {
			mColspan = attribute.asUint( 1 );
			if ( mColspan == 0 )
				mColspan = 1;
			notifyLayoutAttrChangeParent();
			return true;
		}
		default:
			break;
	}
	return UIRichText::applyProperty( attribute );
}

Uint32 UIHTMLTableCell::getColspan() const {
	return mColspan;
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
