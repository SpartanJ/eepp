#include "eepp/ui/uistyle.hpp"
#include <algorithm>
#include <cmath>
#include <eepp/ui/uihtmltable.hpp>

namespace EE { namespace UI {

static inline Float sanitizeFloat( Float val ) {
	return std::isfinite( val ) ? val : 0.f;
}

UIHTMLTable* UIHTMLTable::New() {
	return eeNew( UIHTMLTable, () );
}

UIHTMLTable::UIHTMLTable() : UILayout( "table" ) {
	mFlags |= UI_HTML_ELEMENT | UI_OWNS_CHILDREN_POSITION;
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
}

void UIHTMLTable::setTableLayout( TableLayout layout ) {
	if ( layout != mTableLayout ) {
		mTableLayout = layout;
		invalidateIntrinsicSize();
		tryUpdateLayout();
	}
}

TableLayout UIHTMLTable::getTableLayout() const {
	return mTableLayout;
}

Uint32 UIHTMLTable::getType() const {
	return UI_TYPE_HTML_TABLE;
}

bool UIHTMLTable::isType( const Uint32& type ) const {
	return UIHTMLTable::getType() == type || UILayout::isType( type );
}

bool UIHTMLTable::applyProperty( const StyleSheetProperty& attribute ) {
	if ( attribute.getPropertyDefinition() == nullptr )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Cellspacing:
			mCellspacing = lengthFromValue( attribute );
			invalidateIntrinsicSize();
			tryUpdateLayout();
			return true;
		case PropertyId::Cellpadding:
			mCellpadding = lengthFromValue( attribute );
			invalidateIntrinsicSize();
			tryUpdateLayout();
			return true;
		case PropertyId::TableLayout: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );
			if ( val == "fixed" ) {
				setTableLayout( TableLayout::Fixed );
			} else if ( val == "auto" ) {
				setTableLayout( TableLayout::Auto );
			}
			return true;
		}
		default:
			break;
	}

	return UILayout::applyProperty( attribute );
}

void UIHTMLTable::computeIntrinsicWidths() const {
	if ( !mIntrinsicWidthsDirty )
		return;

	UIHTMLTable* me = const_cast<UIHTMLTable*>( this );

	me->mRows.clear();
	me->mHead = nullptr;
	me->mBody = nullptr;
	me->mFooter = nullptr;
	me->mCells.clear();
	me->mRowCellOffsets.clear();

	auto collectRows = [&]( auto&& self, Node* node ) -> void {
		for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() ) {
			if ( child->getType() == UI_TYPE_HTML_TABLE_ROW ) {
				me->mRows.push_back( child->asType<UIHTMLTableRow>() );
			} else if ( child->getType() != UI_TYPE_HTML_TABLE ) {
				if ( child->getType() == UI_TYPE_HTML_TABLE_HEAD )
					me->mHead = child->asType<UIHTMLTableHead>();
				else if ( child->getType() == UI_TYPE_HTML_TABLE_BODY )
					me->mBody = child->asType<UIHTMLTableBody>();
				else if ( child->getType() == UI_TYPE_HTML_TABLE_FOOTER )
					me->mFooter = child->asType<UIHTMLTableFooter>();

				self( self, child );
			}
		}
	};
	collectRows( collectRows, me );

	auto getRecursiveSpecifiedWidth = [&]( auto&& self, Node* node ) -> Float {
		if ( !node->isWidget() )
			return 0.f;
		UIWidget* widget = node->asType<UIWidget>();
		Float spec = 0.f;
		if ( widget->getLayoutWidthPolicy() == SizePolicy::Fixed )
			spec = sanitizeFloat( widget->getPropertyWidth() );
		for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() )
			spec = std::max( spec, self( self, child ) );
		return spec;
	};

	if ( mRows.empty() ) {
		mMinIntrinsicWidth = mMaxIntrinsicWidth = mPaddingPx.Left + mPaddingPx.Right;
		mIntrinsicWidthsDirty = false;
		return;
	}

	me->mRowCellOffsets.push_back( 0 );
	size_t maxCols = 0;
	for ( auto* row : mRows ) {
		size_t colCount = 0;
		for ( Node* child = row->getFirstChild(); child; child = child->getNextNode() ) {
			if ( child->getType() == UI_TYPE_HTML_TABLE_CELL ) {
				auto* cell = child->asType<UIHTMLTableCell>();
				me->mCells.push_back( cell );
				colCount += cell->getColspan();
				if ( mCellpadding > 0 && cell->getPadding() == Rectf() ) {
					cell->setPadding( { mCellpadding, mCellpadding, mCellpadding, mCellpadding } );
				}
			}
		}
		me->mRowCellOffsets.push_back( static_cast<Uint32>( mCells.size() ) );
		maxCols = std::max( maxCols, colCount );
	}

	if ( maxCols == 0 ) {
		mMinIntrinsicWidth = mMaxIntrinsicWidth = mPaddingPx.Left + mPaddingPx.Right;
		mIntrinsicWidthsDirty = false;
		return;
	}

	me->mColMinWidths.assign( maxCols, 0.f );
	me->mColMaxWidths.assign( maxCols, 0.f );
	me->mColSpecifiedWidths.assign( maxCols, 0.f ); // 0 = no explicit width

	if ( mTableLayout == TableLayout::Fixed ) {
		if ( !mRows.empty() ) {
			Uint32 start = mRowCellOffsets[0];
			Uint32 end = mRowCellOffsets[1];
			Uint32 colIndex = 0;

			// PASS 1: Single colspan first row
			for ( Uint32 i = 0; i < end - start; ++i ) {
				UIHTMLTableCell* cell = mCells[start + i];
				Float cellSpecified = sanitizeFloat(
					std::max( cell->getPropertyWidth(),
							  getRecursiveSpecifiedWidth( getRecursiveSpecifiedWidth, cell ) ) );
				Uint32 colspan = cell->getColspan();

				if ( colspan == 1 && colIndex < maxCols ) {
					if ( cellSpecified > 0.f ) {
						mColSpecifiedWidths[colIndex] =
							std::max( mColSpecifiedWidths[colIndex], cellSpecified );
					}
				}
				colIndex += colspan;
			}

			// PASS 2: Multi-colspan cells first row
			colIndex = 0;
			for ( Uint32 i = 0; i < end - start; ++i ) {
				UIHTMLTableCell* cell = mCells[start + i];
				Float cellSpecified = sanitizeFloat(
					std::max( cell->getPropertyWidth(),
							  getRecursiveSpecifiedWidth( getRecursiveSpecifiedWidth, cell ) ) );
				Uint32 colspan = cell->getColspan();

				if ( colspan > 1 && cellSpecified > 0.f ) {
					Float curSpec = 0.f;
					for ( Uint32 j = 0; j < colspan && colIndex + j < maxCols; ++j )
						curSpec += mColSpecifiedWidths[colIndex + j];
					Float extraSpec = std::max( 0.f, cellSpecified - curSpec );
					if ( extraSpec > 0.f ) {
						Float add = extraSpec / colspan;
						for ( Uint32 j = 0; j < colspan && colIndex + j < maxCols; ++j )
							mColSpecifiedWidths[colIndex + j] =
								std::max( mColSpecifiedWidths[colIndex + j], add );
					}
				}
				colIndex += colspan;
			}
		}
	} else {
		// PASS 1: Collect intrinsic + explicit widths (single colspan first)
		for ( size_t r = 0; r < mRows.size(); ++r ) {
			Uint32 start = mRowCellOffsets[r];
			Uint32 end = mRowCellOffsets[r + 1];
			Uint32 colIndex = 0;

			for ( Uint32 i = 0; i < end - start; ++i ) {
				UIHTMLTableCell* cell = mCells[start + i];
				auto widthPolicy = cell->getLayoutWidthPolicy();
				cell->mWidthPolicy = SizePolicy::WrapContent;
				Float cellMin = sanitizeFloat( cell->getMinIntrinsicWidth() );
				Float cellMax = sanitizeFloat( cell->getMaxIntrinsicWidth() );
				Float cellSpecified = sanitizeFloat(
					std::max( cell->getPropertyWidth(),
							  getRecursiveSpecifiedWidth( getRecursiveSpecifiedWidth, cell ) ) );
				cell->mWidthPolicy = widthPolicy;

				Uint32 colspan = cell->getColspan();

				if ( colspan == 1 && colIndex < maxCols ) {
					mColMinWidths[colIndex] = std::max( mColMinWidths[colIndex], cellMin );
					mColMaxWidths[colIndex] = std::max( mColMaxWidths[colIndex], cellMax );
					if ( cellSpecified > 0.f ) {
						mColSpecifiedWidths[colIndex] =
							std::max( mColSpecifiedWidths[colIndex], cellSpecified );
					}
				}
				colIndex += colspan;
			}
		}

		// PASS 2: Multi-colspan cells - distribute excess only
		for ( size_t r = 0; r < mRows.size(); ++r ) {
			Uint32 start = mRowCellOffsets[r];
			Uint32 end = mRowCellOffsets[r + 1];
			Uint32 colIndex = 0;

			for ( Uint32 i = 0; i < end - start; ++i ) {
				UIHTMLTableCell* cell = mCells[start + i];
				auto widthPolicy = cell->getLayoutWidthPolicy();
				cell->mWidthPolicy = SizePolicy::WrapContent;
				Float cellMin = sanitizeFloat( cell->getMinIntrinsicWidth() );
				Float cellMax = sanitizeFloat( cell->getMaxIntrinsicWidth() );
				Float cellSpecified = sanitizeFloat(
					std::max( cell->getPropertyWidth(),
							  getRecursiveSpecifiedWidth( getRecursiveSpecifiedWidth, cell ) ) );
				cell->mWidthPolicy = widthPolicy;

				Uint32 colspan = cell->getColspan();

				if ( colspan > 1 ) {
					// Min excess
					Float curMin = 0.f;
					for ( Uint32 j = 0; j < colspan && colIndex + j < maxCols; ++j )
						curMin += mColMinWidths[colIndex + j];
					Float extraMin = std::max( 0.f, cellMin - curMin );
					if ( extraMin > 0.f ) {
						Float add = extraMin / colspan;
						for ( Uint32 j = 0; j < colspan && colIndex + j < maxCols; ++j )
							mColMinWidths[colIndex + j] += add;
					}

					// Max excess
					Float curMax = 0.f;
					for ( Uint32 j = 0; j < colspan && colIndex + j < maxCols; ++j )
						curMax += mColMaxWidths[colIndex + j];
					Float extraMax = std::max( 0.f, cellMax - curMax );
					if ( extraMax > 0.f ) {
						Float add = extraMax / colspan;
						for ( Uint32 j = 0; j < colspan && colIndex + j < maxCols; ++j )
							mColMaxWidths[colIndex + j] += add;
					}

					// Specified width excess (simple even distribution for now)
					if ( cellSpecified > 0.f ) {
						Float curSpec = 0.f;
						for ( Uint32 j = 0; j < colspan && colIndex + j < maxCols; ++j )
							curSpec += mColSpecifiedWidths[colIndex + j];
						Float extraSpec = std::max( 0.f, cellSpecified - curSpec );
						if ( extraSpec > 0.f ) {
							Float add = extraSpec / colspan;
							for ( Uint32 j = 0; j < colspan && colIndex + j < maxCols; ++j )
								mColSpecifiedWidths[colIndex + j] =
									std::max( mColSpecifiedWidths[colIndex + j], add );
						}
					}
				}
				colIndex += colspan;
			}
		}
	}

	Float totalMin = 0.f, totalMax = 0.f;
	for ( size_t i = 0; i < maxCols; ++i ) {
		mColMinWidths[i] = sanitizeFloat( std::max( mColMinWidths[i], mColSpecifiedWidths[i] ) );
		mColMaxWidths[i] = sanitizeFloat( std::max( mColMaxWidths[i], mColSpecifiedWidths[i] ) );
		totalMin += mColMinWidths[i];
		totalMax += mColMaxWidths[i];
	}

	mMinIntrinsicWidth =
		totalMin + mPaddingPx.Left + mPaddingPx.Right + ( maxCols + 1 ) * mCellspacing;
	mMaxIntrinsicWidth =
		totalMax + mPaddingPx.Left + mPaddingPx.Right + ( maxCols + 1 ) * mCellspacing;

	mIntrinsicWidthsDirty = false;
}

Float UIHTMLTable::getMinIntrinsicWidth() const {
	computeIntrinsicWidths();
	return mMinIntrinsicWidth;
}

Float UIHTMLTable::getMaxIntrinsicWidth() const {
	computeIntrinsicWidths();
	return mMaxIntrinsicWidth;
}

void UIHTMLTable::updateLayout() {
	if ( mPacking || !mVisible )
		return;
	mPacking = true;
	setMatchParentIfNeededVerticalGrowth();

	const StyleSheetProperty* prop = nullptr;
	if ( getLayoutWidthPolicy() == SizePolicy::Fixed && mStyle &&
		 ( prop = mStyle->getProperty( PropertyId::Width ) ) ) {
		setInternalPixelsSize( { lengthFromValue( *prop ), mSize.getHeight() } );
	}

	computeIntrinsicWidths();

	if ( mRows.empty() ) {
		mPacking = false;
		return;
	}

	size_t maxCols = mColMinWidths.size();
	mColWidths.assign( maxCols, 0.f );
	Float paddingH = mPaddingPx.Left + mPaddingPx.Right;
	Float containerWidth = getPixelsSize().getWidth();
	Float availableWidth = sanitizeFloat(
		std::max( 0.f, containerWidth - paddingH - ( maxCols + 1 ) * mCellspacing ) );

	if ( availableWidth <= 0.f || maxCols == 0 ) {
		mPacking = false;
		return;
	}
	Float totalMin = 0.f;
	Float totalMax = 0.f; // Make sure this is uncommented
	for ( size_t i = 0; i < maxCols; ++i ) {
		totalMin += sanitizeFloat( mColMinWidths[i] );
		totalMax += sanitizeFloat( mColMaxWidths[i] ); // Accumulate max widths
	}

	Float tableUsedWidth = availableWidth; // always try to fill the container

	// Assign column widths
	if ( mTableLayout == TableLayout::Fixed ) {
		Float sumOfSpecifiedWidths = 0.f;
		size_t unspecifiedCount = 0;
		for ( size_t i = 0; i < maxCols; ++i ) {
			if ( mColSpecifiedWidths[i] > 0.f ) {
				sumOfSpecifiedWidths += mColSpecifiedWidths[i];
				mColWidths[i] = mColSpecifiedWidths[i];
			} else {
				unspecifiedCount++;
			}
		}

		Float remainingSpace = std::max( 0.f, availableWidth - sumOfSpecifiedWidths );

		if ( unspecifiedCount > 0 ) {
			Float share = remainingSpace / static_cast<Float>( unspecifiedCount );
			for ( size_t i = 0; i < maxCols; ++i ) {
				if ( mColSpecifiedWidths[i] <= 0.f ) {
					mColWidths[i] = share;
				}
			}
		} else if ( remainingSpace > 0.f && sumOfSpecifiedWidths > 0.f ) {
			for ( size_t i = 0; i < maxCols; ++i ) {
				Float scale = mColSpecifiedWidths[i] / sumOfSpecifiedWidths;
				mColWidths[i] += remainingSpace * scale;
			}
		}
	} else if ( tableUsedWidth <= totalMin + 0.001f ) {
		// 1. Too narrow → scale down proportionally to min widths
		Float scale = totalMin > 0.001f ? ( tableUsedWidth / totalMin ) : 0.f;
		for ( size_t i = 0; i < maxCols; ++i )
			mColWidths[i] = mColMinWidths[i] * scale;

	} else if ( tableUsedWidth <= totalMax + 0.001f ) {
		// 2. Partial flex → space is between min and max. Distribute extra by flexibility (text
		// wrapping)
		Float extraSpace = tableUsedWidth - totalMin;
		Float totalFlex = 0.f;

		for ( size_t i = 0; i < maxCols; ++i ) {
			Float flex = mColMaxWidths[i] - mColMinWidths[i];
			if ( mColSpecifiedWidths[i] > 0.f )
				flex = 0.f; // explicit widths stay rigid here
			totalFlex += flex;
		}

		if ( totalFlex > 0.001f ) {
			for ( size_t i = 0; i < maxCols; ++i ) {
				Float flex = mColMaxWidths[i] - mColMinWidths[i];
				if ( mColSpecifiedWidths[i] > 0.f )
					flex = 0.f;
				Float added = extraSpace * ( flex / totalFlex );
				mColWidths[i] = mColMinWidths[i] + added;
			}
		} else {
			// Fallback if no flex exists
			Float scale = totalMin > 0.001f ? ( tableUsedWidth / totalMin ) : 0.f;
			for ( size_t i = 0; i < maxCols; ++i )
				mColWidths[i] = mColMinWidths[i] * scale;
		}

	} else {
		// 3. Abundant space → table is wider than all max widths combined.
		// Give everyone their max width, then distribute the leftover space.
		Float leftOver = tableUsedWidth - totalMax;

		Float totalMaxUnspecified = 0.f;
		size_t unspecifiedCount = 0;

		for ( size_t i = 0; i < maxCols; ++i ) {
			if ( mColSpecifiedWidths[i] <= 0.f ) {
				totalMaxUnspecified += mColMaxWidths[i];
				unspecifiedCount++;
			}
		}

		if ( unspecifiedCount > 0 ) {
			// Distribute leftover space proportionally to max-widths for a balanced look
			if ( totalMaxUnspecified > 0.001f ) {
				for ( size_t i = 0; i < maxCols; ++i ) {
					if ( mColSpecifiedWidths[i] <= 0.f ) {
						Float scale = mColMaxWidths[i] / totalMaxUnspecified;
						mColWidths[i] = mColMaxWidths[i] + ( leftOver * scale );
					} else {
						mColWidths[i] = mColMaxWidths[i]; // Rigid explicit column stays rigid
					}
				}
			} else {
				// Fallback to strict even split if max widths are 0
				Float share = leftOver / static_cast<Float>( unspecifiedCount );
				for ( size_t i = 0; i < maxCols; ++i ) {
					if ( mColSpecifiedWidths[i] <= 0.f ) {
						mColWidths[i] = mColMaxWidths[i] + share;
					} else {
						mColWidths[i] = mColMaxWidths[i];
					}
				}
			}
		} else {
			// Absolute fallback: All columns explicitly specified, but space remains. Scale up.
			Float scale = totalMax > 0.001f ? ( tableUsedWidth / totalMax ) : 0.f;
			for ( size_t i = 0; i < maxCols; ++i )
				mColWidths[i] = mColMaxWidths[i] * scale;
		}
	}

	// Safety fallback (should never trigger now)
	Float sum = 0.f;
	for ( float w : mColWidths )
		sum += w;
	if ( sum < 1.f && maxCols > 0 ) {
		Float w = tableUsedWidth / static_cast<Float>( maxCols );
		for ( size_t i = 0; i < maxCols; ++i )
			mColWidths[i] = w;
	}

	for ( float& w : mColWidths )
		w = sanitizeFloat( w );

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
			cell->beginAttributesTransaction();
			cell->setLayoutWidthPolicy( SizePolicy::Fixed );
			cell->setLayoutHeightPolicy( SizePolicy::WrapContent );
			Uint32 cellColspan = cell->getColspan();
			Float cellWidth = 0;
			for ( Uint32 j = 0; j < cellColspan && ( colIndex + j ) < maxCols; ++j ) {
				cellWidth += mColWidths[colIndex + j];
			}
			if ( cellColspan > 1 )
				cellWidth += ( cellColspan - 1 ) * mCellspacing;
			cell->setPixelsSize( cellWidth, cell->getPixelsSize().getHeight() );
			cell->updateLayout();
			cell->setLayoutHeightPolicy( SizePolicy::Fixed );
			cell->endAttributesTransaction();
			rowHeight = std::max( rowHeight, cell->getPixelsSize().getHeight() );
			colIndex += cellColspan;
		}

		// Position cells inside the row and equalize height
		Float currentX = mCellspacing;
		colIndex = 0;
		for ( Uint32 c = 0; c < columnCount; ++c ) {
			UIHTMLTableCell* cell = mCells[start + c];
			cell->beginAttributesTransaction();
			cell->setPixelsPosition( currentX, 0 );
			Uint32 cellColspan = cell->getColspan();
			Float cellWidth = 0;
			for ( Uint32 j = 0; j < cellColspan && ( colIndex + j ) < maxCols; ++j ) {
				cellWidth += mColWidths[colIndex + j];
			}
			if ( cellColspan > 1 )
				cellWidth += ( cellColspan - 1 ) * mCellspacing;
			cell->setPixelsSize( cellWidth, rowHeight );
			cell->endAttributesTransaction();
			currentX += cellWidth + mCellspacing;
			colIndex += cellColspan;
		}

		// Set row height and width
		UIHTMLTableRow* row = mRows[r];
		row->setPixelsSize( containerWidth - paddingH, rowHeight );

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
	if ( mHead ) {
		mHead->setPixelsPosition( 0, 0 );
		mHead->setPixelsSize( { getPixelsSize().x, headHeight } );
	}

	if ( mBody ) {
		mBody->setPixelsPosition( 0, headHeight );
		mBody->setPixelsSize( { getPixelsSize().x, bodyHeight } );
	}

	if ( mFooter ) {
		mFooter->setPixelsPosition( 0, headHeight + bodyHeight );
		mFooter->setPixelsSize( { getPixelsSize().x, footerHeight } );
	}

	Float currentY = mPaddingPx.Top + mCellspacing - headHeight;
	for ( size_t r = 0; r < rowCount; ++r ) {
		UIHTMLTableRow* row = mRows[r];
		row->setPixelsPosition( mPaddingPx.Left, currentY );
		currentY += row->getPixelsSize().getHeight() + mCellspacing;
	}

	// Reset positions if they are inside specialized containers
	if ( mHead && !mRows.empty() )
		mRows[0]->setPixelsPosition( mPaddingPx.Left, 0 );

	if ( mFooter && !mRows.empty() )
		mRows[rowCount - 1]->setPixelsPosition( mPaddingPx.Left, 0 );

	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( mPaddingPx.Top + headHeight + bodyHeight + footerHeight +
								 ( rowCount + 1 ) * mCellspacing + mPaddingPx.Bottom );
	}

	mPacking = false;
	mDirtyLayout = false;
}

Uint32 UIHTMLTable::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			if ( Msg->getSender() != this && !mPacking ) {
				mIntrinsicWidthsDirty = true;
				notifyLayoutAttrChangeParent();
			}
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

void UIHTMLTableCell::onSizeChange() {
	UIRichText::onSizeChange();
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
