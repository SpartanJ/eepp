#include <eepp/ui/tablelayouter.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

static inline Float sanitizeFloat( Float val ) {
	return std::isfinite( val ) ? val : 0.f;
}

void TableLayouter::setTableLayout( TableLayout layout ) {
	if ( layout != mTableLayout ) {
		mTableLayout = layout;
	}
}

TableLayout TableLayouter::getTableLayout() const {
	return mTableLayout;
}

void TableLayouter::setCellPadding( Float padding ) {
	mCellpadding = padding;
}

Float TableLayouter::getCellPadding() const {
	return mCellpadding;
}

void TableLayouter::setCellSpacing( Float spacing ) {
	mCellspacing = spacing;
}

Float TableLayouter::getCellSpacing() const {
	return mCellspacing;
}

Float TableLayouter::getMinIntrinsicWidth() {
	computeIntrinsicWidths();
	return mMinIntrinsicWidth;
}

Float TableLayouter::getMaxIntrinsicWidth() {
	computeIntrinsicWidths();
	return mMaxIntrinsicWidth;
}

void TableLayouter::computeIntrinsicWidths() {
	if ( !mIntrinsicWidthsDirty )
		return;

	mRows.clear();
	mHead = nullptr;
	mBody = nullptr;
	mFooter = nullptr;
	mCells.clear();
	mRowCellOffsets.clear();

	auto collectRows = [&]( auto&& self, Node* node ) -> void {
		for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() ) {
			if ( child->getType() == UI_TYPE_HTML_TABLE_ROW ) {
				mRows.push_back( child->asType<UIHTMLTableRow>() );
			} else if ( child->getType() != UI_TYPE_HTML_TABLE ) {
				if ( child->getType() == UI_TYPE_HTML_TABLE_HEAD )
					mHead = child->asType<UIHTMLTableHead>();
				else if ( child->getType() == UI_TYPE_HTML_TABLE_BODY )
					mBody = child->asType<UIHTMLTableBody>();
				else if ( child->getType() == UI_TYPE_HTML_TABLE_FOOTER )
					mFooter = child->asType<UIHTMLTableFooter>();

				self( self, child );
			}
		}
	};
	collectRows( collectRows, mContainer );

	auto getRecursiveSpecifiedWidth = [&]( auto&& self, Node* node ) -> Float {
		if ( !node->isWidget() )
			return 0.f;
		if ( node->isType( UI_TYPE_HTML_WIDGET ) && node->asType<UIHTMLWidget>()->isOutOfFlow() )
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
		mMinIntrinsicWidth = mMaxIntrinsicWidth =
			mContainer->getPixelsContentOffset().Left + mContainer->getPixelsContentOffset().Right;
		mIntrinsicWidthsDirty = false;
		return;
	}

	mRowCellOffsets.push_back( 0 );
	size_t maxCols = 0;
	for ( auto* row : mRows ) {
		size_t colCount = 0;
		for ( Node* child = row->getFirstChild(); child; child = child->getNextNode() ) {
			if ( child->getType() == UI_TYPE_HTML_TABLE_CELL ) {
				auto* cell = child->asType<UIHTMLTableCell>();
				mCells.push_back( cell );
				colCount += cell->getColSpan();
				if ( mCellpadding > 0 && cell->getPadding() == Rectf::Zero ) {
					cell->setPadding( { mCellpadding, mCellpadding, mCellpadding, mCellpadding } );
				}
			}
		}
		mRowCellOffsets.push_back( static_cast<Uint32>( mCells.size() ) );
		maxCols = std::max( maxCols, colCount );
	}

	if ( maxCols == 0 ) {
		mMinIntrinsicWidth = mMaxIntrinsicWidth =
			mContainer->getPixelsContentOffset().Left + mContainer->getPixelsContentOffset().Right;
		mIntrinsicWidthsDirty = false;
		return;
	}

	mColMinWidths.assign( maxCols, 0.f );
	mColMaxWidths.assign( maxCols, 0.f );
	mColSpecifiedWidths.assign( maxCols, 0.f ); // 0 = no explicit width

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
				Uint32 colspan = cell->getColSpan();

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
				Uint32 colspan = cell->getColSpan();

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

				Uint32 colspan = cell->getColSpan();

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

				Uint32 colspan = cell->getColSpan();

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

					// Specified width excess
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

	mMinIntrinsicWidth = totalMin + mContainer->getPixelsContentOffset().Left +
						 mContainer->getPixelsContentOffset().Right +
						 ( maxCols + 1 ) * mCellspacing;
	mMaxIntrinsicWidth = totalMax + mContainer->getPixelsContentOffset().Left +
						 mContainer->getPixelsContentOffset().Right +
						 ( maxCols + 1 ) * mCellspacing;

	mIntrinsicWidthsDirty = false;
}

void TableLayouter::updateLayout() {
	if ( !mContainer->isVisible() )
		return;

	if ( mPacking )
		return;
	mPacking = true;

	setMatchParentIfNeededVerticalGrowth();

	const StyleSheetProperty* prop = nullptr;
	UIWidget* widget = mContainer->asType<UIWidget>();
	if ( widget->getLayoutWidthPolicy() == SizePolicy::Fixed && widget->getUIStyle() &&
		 ( prop = widget->getUIStyle()->getProperty( PropertyId::Width ) ) ) {
		widget->asType<UINode>()->setInternalPixelsSize(
			{ widget->lengthFromValue( *prop ), widget->getPixelsSize().getHeight() } );
	}

	if ( widget->getLayoutHeightPolicy() == SizePolicy::Fixed && widget->getUIStyle() &&
		 ( prop = widget->getUIStyle()->getProperty( PropertyId::Height ) ) ) {
		widget->asType<UINode>()->setInternalPixelsSize(
			{ widget->getPixelsSize().getWidth(), widget->lengthFromValue( *prop ) } );
	}

	computeIntrinsicWidths();

	bool useContentWidth = false;
	if ( widget->getLayoutWidthPolicy() == SizePolicy::Fixed && widget->getUIStyle() ) {
		const StyleSheetProperty* wprop = widget->getUIStyle()->getProperty( PropertyId::Width );
		if ( wprop && StyleSheetLength::isPercentage( wprop->value() ) && widget->getParent() &&
			 widget->getParent()->isWidget() &&
			 widget->getParent()->asType<UIWidget>()->getLayoutWidthPolicy() ==
				 SizePolicy::WrapContent ) {
			useContentWidth = true;
		}
	}

	bool useContentHeight = false;
	if ( widget->getLayoutHeightPolicy() == SizePolicy::Fixed && widget->getUIStyle() ) {
		const StyleSheetProperty* hprop = widget->getUIStyle()->getProperty( PropertyId::Height );
		if ( hprop && StyleSheetLength::isPercentage( hprop->value() ) && widget->getParent() &&
			 widget->getParent()->isWidget() &&
			 widget->getParent()->asType<UIWidget>()->getLayoutHeightPolicy() ==
				 SizePolicy::WrapContent ) {
			useContentHeight = true;
		}
	}

	if ( mRows.empty() ) {
		mPacking = false;
		return;
	}

	size_t maxCols = mColMinWidths.size();
	mColWidths.assign( maxCols, 0.f );
	Float paddingH =
		mContainer->getPixelsContentOffset().Left + mContainer->getPixelsContentOffset().Right;
	Float containerWidth = mContainer->getPixelsSize().getWidth();
	if ( useContentWidth )
		containerWidth = mMaxIntrinsicWidth;
	Float availableWidth = sanitizeFloat(
		std::max( 0.f, containerWidth - paddingH - ( maxCols + 1 ) * mCellspacing ) );

	if ( availableWidth <= 0.f || maxCols == 0 ) {
		mPacking = false;
		return;
	}

	Float totalMin = 0.f;
	Float totalMax = 0.f;
	for ( size_t i = 0; i < maxCols; ++i ) {
		totalMin += sanitizeFloat( mColMinWidths[i] );
		totalMax += sanitizeFloat( mColMaxWidths[i] );
	}

	Float tableUsedWidth = availableWidth;

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
		Float scale = totalMin > 0.001f ? ( tableUsedWidth / totalMin ) : 0.f;
		for ( size_t i = 0; i < maxCols; ++i )
			mColWidths[i] = mColMinWidths[i] * scale;
	} else if ( tableUsedWidth <= totalMax + 0.001f ) {
		Float extraSpace = tableUsedWidth - totalMin;
		Float totalFlex = 0.f;

		for ( size_t i = 0; i < maxCols; ++i ) {
			Float flex = mColMaxWidths[i] - mColMinWidths[i];
			if ( mColSpecifiedWidths[i] > 0.f )
				flex = 0.f;
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
			Float scale = totalMin > 0.001f ? ( tableUsedWidth / totalMin ) : 0.f;
			for ( size_t i = 0; i < maxCols; ++i )
				mColWidths[i] = mColMinWidths[i] * scale;
		}
	} else {
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
			if ( totalMaxUnspecified > 0.001f ) {
				for ( size_t i = 0; i < maxCols; ++i ) {
					if ( mColSpecifiedWidths[i] <= 0.f ) {
						Float scale = mColMaxWidths[i] / totalMaxUnspecified;
						mColWidths[i] = mColMaxWidths[i] + ( leftOver * scale );
					} else {
						mColWidths[i] = mColMaxWidths[i];
					}
				}
			} else {
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
			Float scale = totalMax > 0.001f ? ( tableUsedWidth / totalMax ) : 0.f;
			for ( size_t i = 0; i < maxCols; ++i )
				mColWidths[i] = mColMaxWidths[i] * scale;
		}
	}

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
			Uint32 cellColspan = cell->getColSpan();
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

		Float currentX = mCellspacing;
		colIndex = 0;
		for ( Uint32 c = 0; c < columnCount; ++c ) {
			UIHTMLTableCell* cell = mCells[start + c];
			cell->beginAttributesTransaction();
			cell->setPixelsPosition( currentX, 0 );
			Uint32 cellColspan = cell->getColSpan();
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

	if ( mHead ) {
		mHead->setPixelsPosition( 0, 0 );
		mHead->setPixelsSize( { mContainer->getPixelsSize().x, headHeight } );
	}

	if ( mBody ) {
		mBody->setPixelsPosition( 0, headHeight );
		mBody->setPixelsSize( { mContainer->getPixelsSize().x, bodyHeight } );
	}

	if ( mFooter ) {
		mFooter->setPixelsPosition( 0, headHeight + bodyHeight );
		mFooter->setPixelsSize( { mContainer->getPixelsSize().x, footerHeight } );
	}

	Float currentY = mContainer->getPixelsContentOffset().Top + mCellspacing - headHeight;
	for ( size_t r = 0; r < rowCount; ++r ) {
		UIHTMLTableRow* row = mRows[r];
		row->setPixelsPosition( mContainer->getPixelsContentOffset().Left, currentY );
		currentY += row->getPixelsSize().getHeight() + mCellspacing;
	}

	if ( mHead && !mRows.empty() )
		mRows[0]->setPixelsPosition( mContainer->getPixelsContentOffset().Left, 0 );

	if ( mFooter && !mRows.empty() )
		mRows[rowCount - 1]->setPixelsPosition( mContainer->getPixelsContentOffset().Left, 0 );

	if ( mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent || useContentHeight ) {
		mContainer->asType<UINode>()->setInternalPixelsHeight(
			mContainer->getPixelsContentOffset().Top + headHeight + bodyHeight + footerHeight +
			( rowCount + 1 ) * mCellspacing + mContainer->getPixelsContentOffset().Bottom );
	}

	if ( useContentWidth )
		widget->asType<UINode>()->setInternalPixelsWidth( mMaxIntrinsicWidth );

	mPacking = false;
}

}} // namespace EE::UI
