#include <deque>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitreeview.hpp>

namespace EE { namespace UI {

UITreeView* UITreeView::New() {
	return eeNew( UITreeView, () );
}

UITreeView::UITreeView() :
	UIAbstractTableView( "treeview" ),
	mIndentWidth( PixelDensity::dpToPx( 12 ) ),
	mExpanderIconSize( PixelDensity::dpToPxI( 12 ) ) {
	clipEnable();
	mExpandIcon = getUISceneNode()->findIcon( "tree-expanded" );
	mContractIcon = getUISceneNode()->findIcon( "tree-contracted" );
}

Uint32 UITreeView::getType() const {
	return UI_TYPE_TREEVIEW;
}

bool UITreeView::isType( const Uint32& type ) const {
	return UITreeView::getType() == type ? true : UIAbstractTableView::isType( type );
}

UITreeView::MetadataForIndex& UITreeView::getIndexMetadata( const ModelIndex& index ) const {
	eeASSERT( index.isValid() );
	auto it = mViewMetadata.find( index.internalData() );
	if ( it != mViewMetadata.end() )
		return it->second;
	auto newMetadata = MetadataForIndex();
	mViewMetadata.insert( { index.internalData(), std::move( newMetadata ) } );
	return mViewMetadata[index.internalData()];
}

void UITreeView::traverseTree( TreeViewCallback callback ) const {
	if ( !getModel() )
		return;
	auto& model = *getModel();
	Lock l( const_cast<Model*>( getModel() )->resourceLock() );
	int indentLevel = 0;
	Float yOffset = getHeaderHeight();
	int rowIndex = -1;
	std::function<IterationDecision( const ModelIndex& )> traverseIndex =
		[&]( const ModelIndex& index ) {
			if ( index.isValid() ) {
				auto& metadata = getIndexMetadata( index );
				rowIndex++;
				IterationDecision decision = callback( rowIndex, index, indentLevel, yOffset );
				if ( decision == IterationDecision::Break || decision == IterationDecision::Stop )
					return decision;
				yOffset += getRowHeight();
				if ( !metadata.open ) {
					return IterationDecision::Continue;
				}
			}
			if ( indentLevel >= 0 && !index.isValid() )
				return IterationDecision::Continue;
			++indentLevel;
			int rowCount = model.rowCount( index );
			for ( int i = 0; i < rowCount; ++i ) {
				IterationDecision decision =
					traverseIndex( model.index( i, model.treeColumn(), index ) );
				if ( decision == IterationDecision::Break || decision == IterationDecision::Stop )
					return decision;
			}
			--indentLevel;
			return IterationDecision::Continue;
		};
	int rootCount = model.rowCount();
	for ( int rootIndex = 0; rootIndex < rootCount; ++rootIndex ) {
		IterationDecision decision =
			traverseIndex( model.index( rootIndex, model.treeColumn(), ModelIndex() ) );
		if ( decision == IterationDecision::Break || decision == IterationDecision::Stop )
			break;
	}
}

void UITreeView::createOrUpdateColumns() {
	if ( !getModel() )
		return;
	updateContentSize();
	UIAbstractTableView::createOrUpdateColumns();
}

size_t UITreeView::getItemCount() const {
	size_t count = 0;
	traverseTree( [&]( const int&, const ModelIndex&, const size_t&, const Float& ) {
		count++;
		return IterationDecision::Continue;
	} );
	return count;
}

void UITreeView::onColumnSizeChange( const size_t& colIndex, bool fromUserInteraction ) {
	UIAbstractTableView::onColumnSizeChange( colIndex, fromUserInteraction );
	updateContentSize();
}

void UITreeView::updateContentSize() {
	Sizef oldSize( mContentSize );
	mContentSize = UIAbstractTableView::getContentSize();
	if ( oldSize != mContentSize )
		onContentSizeChange();
}

void UITreeView::bindNavigationClick( UIWidget* widget ) {
	mWidgetsClickCbId[widget] = widget->addEventListener(
		mSingleClickNavigation ? Event::MouseClick : Event::MouseDoubleClick,
		[&]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				Lock l( const_cast<Model*>( getModel() )->resourceLock() );
				if ( getModel()->rowCount( idx ) ) {
					auto& data = getIndexMetadata( idx );
					data.open = !data.open;
					createOrUpdateColumns();
					onOpenTreeModelIndex( idx, data.open );
				} else {
					onOpenModelIndex( idx, event );
				}
			}
		} );

	widget->addEventListener( Event::MouseClick, [&]( const Event* event ) {
		auto mouseEvent = static_cast<const MouseEvent*>( event );
		auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
		if ( mouseEvent->getFlags() & EE_BUTTON_RMASK ) {
			onOpenMenuModelIndex( idx, event );
		}
	} );
}

UIWidget* UITreeView::setupCell( UITableCell* widget, UIWidget* rowWidget,
								 const ModelIndex& index ) {
	widget->setParent( rowWidget );
	widget->unsetFlags( UI_AUTO_SIZE );
	widget->clipEnable();
	widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	widget->setTextAlign( UI_HALIGN_LEFT );
	widget->setCurIndex( index );
	if ( index.column() == (Int64)getModel()->treeColumn() ) {
		bindNavigationClick( widget );
		widget->addEventListener( Event::MouseClick, [&]( const Event* event ) {
			if ( mSingleClickNavigation )
				return;
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			UIWidget* icon = mouseEvent->getNode()->asType<UIPushButton>()->getExtraInnerWidget();
			if ( icon ) {
				Vector2f pos( icon->convertToNodeSpace( mouseEvent->getPosition().asFloat() ) );
				if ( pos >= Vector2f::Zero && pos <= icon->getPixelsSize() ) {
					Lock l( const_cast<Model*>( getModel() )->resourceLock() );
					auto idx =
						mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
					if ( getModel()->rowCount( idx ) ) {
						auto& data = getIndexMetadata( idx );
						data.open = !data.open;
						createOrUpdateColumns();
						onOpenTreeModelIndex( idx, data.open );
					}
				}
			}
		} );
	}
	return widget;
}

UIWidget* UITreeView::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	UITableCell* widget = index.column() == (Int64)getModel()->treeColumn() ? UITreeViewCell::New()
																			: UITableCell::New();
	return setupCell( widget, rowWidget, index );
}

UIWidget* UITreeView::updateCell( const int& rowIndex, const ModelIndex& index,
								  const size_t& indentLevel, const Float& yOffset ) {
	if ( rowIndex >= (int)mWidgets.size() )
		mWidgets.resize( rowIndex + 1 );
	auto* widget = mWidgets[rowIndex][index.column()];
	if ( !widget ) {
		UIWidget* rowWidget = updateRow( rowIndex, index, yOffset );
		widget = createCell( rowWidget, index );
		mWidgets[rowIndex][index.column()] = widget;
		widget->reloadStyle( true, true, true );
	}
	widget->setPixelsSize( columnData( index.column() ).width, getRowHeight() );
	widget->setPixelsPosition( { getColumnPosition( index.column() ).x, 0 } );

	if ( widget->isType( UI_TYPE_TABLECELL ) ) {
		UITableCell* cell = widget->asType<UITableCell>();
		cell->setCurIndex( index );

		Variant txt( getModel()->data( index, ModelRole::Display ) );
		if ( txt.isValid() ) {
			if ( txt.is( Variant::Type::String ) )
				cell->setText( txt.asString() );
			else if ( txt.is( Variant::Type::cstr ) )
				cell->setText( txt.asCStr() );
		}

		bool hasChilds = false;

		if ( widget->isType( UI_TYPE_TREEVIEW_CELL ) ) {
			UITreeViewCell* cell = widget->asType<UITreeViewCell>();
			UIImage* image = widget->asType<UITreeViewCell>()->getImage();

			Float minIndent =
				!mExpandersAsIcons
					? eemax( mExpandIcon->getSize( mExpanderIconSize )->getPixelsSize().getWidth(),
							 mContractIcon->getSize( mExpanderIconSize )
								 ->getPixelsSize()
								 .getWidth() ) +
						  PixelDensity::dpToPx( image->getLayoutMargin().Right )
					: 0;

			if ( index.column() == (Int64)getModel()->treeColumn() )
				cell->setIndentation( minIndent + getIndentWidth() * indentLevel );

			hasChilds = getModel()->rowCount( index ) > 0;

			if ( hasChilds ) {
				UIIcon* icon = getIndexMetadata( index ).open ? mExpandIcon : mContractIcon;
				Drawable* drawable = icon->getSize( mExpanderIconSize );

				image->setVisible( true );
				image->setPixelsSize( drawable->getPixelsSize() );
				image->setDrawable( drawable );
				if ( !mExpandersAsIcons ) {
					cell->setIndentation( cell->getIndentation() -
										  image->getPixelsSize().getWidth() -
										  PixelDensity::dpToPx( image->getLayoutMargin().Right ) );
				}
			} else {
				image->setVisible( false );
			}
		}

		if ( hasChilds && mExpandersAsIcons ) {
			cell->getIcon()->setVisible( false );
			return widget;
		}

		bool isVisible = false;
		Variant icon( getModel()->data( index, ModelRole::Icon ) );
		if ( icon.is( Variant::Type::Drawable ) && icon.asDrawable() ) {
			isVisible = true;
			cell->setIcon( icon.asDrawable() );
		} else if ( icon.is( Variant::Type::Icon ) && icon.asIcon() ) {
			isVisible = true;
			cell->setIcon( icon.asIcon()->getSize( mIconSize ) );
		}
		cell->getIcon()->setVisible( isVisible );

		cell->updateCell( getModel() );
	}

	return widget;
}

const Float& UITreeView::getIndentWidth() const {
	return mIndentWidth;
}

void UITreeView::setIndentWidth( const Float& indentWidth ) {
	if ( mIndentWidth != indentWidth ) {
		mIndentWidth = indentWidth;
		createOrUpdateColumns();
	}
}

Sizef UITreeView::getContentSize() const {
	return mContentSize;
}

void UITreeView::drawChilds() {
	int realIndex = 0;

	traverseTree( [&]( const int&, const ModelIndex& index, const size_t& indentLevel,
					   const Float& yOffset ) {
		if ( yOffset - mScrollOffset.y > mSize.getHeight() )
			return IterationDecision::Stop;
		if ( yOffset - mScrollOffset.y + getRowHeight() < 0 )
			return IterationDecision::Continue;
		for ( size_t colIndex = 0; colIndex < getModel()->columnCount(); colIndex++ ) {
			if ( columnData( colIndex ).visible ) {
				if ( (Int64)colIndex != index.column() ) {
					updateCell( realIndex,
								getModel()->index( index.row(), colIndex, index.parent() ),
								indentLevel, yOffset );
				} else {
					auto* cell = updateCell( realIndex, index, indentLevel, yOffset );

					if ( mFocusSelectionDirty && index == getSelection().first() ) {
						cell->setFocus();
						mFocusSelectionDirty = false;
					}
				}
			}
		}
		updateRow( realIndex, index, yOffset )->nodeDraw();
		realIndex++;
		return IterationDecision::Continue;
	} );

	if ( mHeader && mHeader->isVisible() )
		mHeader->nodeDraw();
	if ( mHScroll->isVisible() )
		mHScroll->nodeDraw();
	if ( mVScroll->isVisible() )
		mVScroll->nodeDraw();
}

Node* UITreeView::overFind( const Vector2f& point ) {
	mUISceneNode->setIsLoading( true );

	Node* pOver = NULL;
	if ( mEnabled && mVisible ) {
		updateWorldPolygon();
		if ( mWorldBounds.contains( point ) && mPoly.pointInside( point ) ) {
			writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );
			mSceneNode->addMouseOverNode( this );
			if ( mHScroll->isVisible() && ( pOver = mHScroll->overFind( point ) ) )
				return pOver;
			if ( mVScroll->isVisible() && ( pOver = mVScroll->overFind( point ) ) )
				return pOver;
			if ( mHeader && ( pOver = mHeader->overFind( point ) ) )
				return pOver;
			int realIndex = 0;
			traverseTree(
				[&, point]( int, const ModelIndex& index, const size_t&, const Float& yOffset ) {
					if ( yOffset - mScrollOffset.y > mSize.getHeight() )
						return IterationDecision::Stop;
					if ( yOffset - mScrollOffset.y + getRowHeight() < 0 )
						return IterationDecision::Continue;
					pOver = updateRow( realIndex, index, yOffset )->overFind( point );
					realIndex++;
					if ( pOver )
						return IterationDecision::Stop;
					return IterationDecision::Continue;
				} );
			if ( !pOver )
				pOver = this;
		}
	}

	mUISceneNode->setIsLoading( false );

	return pOver;
}

bool UITreeView::isExpanded( const ModelIndex& index ) const {
	return getIndexMetadata( index ).open;
}

void UITreeView::setAllExpanded( const ModelIndex& index, bool expanded ) {
	Model& model = *getModel();
	size_t count = model.rowCount( index );
	for ( size_t i = 0; i < count; i++ ) {
		auto curIndex = model.index( i, model.treeColumn(), index );
		getIndexMetadata( curIndex ).open = expanded;
		if ( model.rowCount( curIndex ) > 0 )
			setAllExpanded( curIndex, expanded );
	}

	createOrUpdateColumns();
}

void UITreeView::expandAll( const ModelIndex& index ) {
	if ( !getModel() )
		return;
	setAllExpanded( index, true );
	createOrUpdateColumns();
}

void UITreeView::collapseAll( const ModelIndex& index ) {
	if ( !getModel() )
		return;
	setAllExpanded( index, false );
	createOrUpdateColumns();
}

UIIcon* UITreeView::getExpandIcon() const {
	return mExpandIcon;
}

void UITreeView::setExpandedIcon( UIIcon* expandIcon ) {
	if ( mExpandIcon != expandIcon ) {
		mExpandIcon = expandIcon;
		createOrUpdateColumns();
	}
}

void UITreeView::setExpandedIcon( const std::string& expandIcon ) {
	setExpandedIcon( mUISceneNode->findIcon( expandIcon ) );
}

UIIcon* UITreeView::getContractIcon() const {
	return mContractIcon;
}

void UITreeView::setContractedIcon( UIIcon* contractIcon ) {
	if ( mContractIcon != contractIcon ) {
		mContractIcon = contractIcon;
		createOrUpdateColumns();
	}
}

void UITreeView::setContractedIcon( const std::string& contractIcon ) {
	setContractedIcon( mUISceneNode->findIcon( contractIcon ) );
}

bool UITreeView::getExpandersAsIcons() const {
	return mExpandersAsIcons;
}

void UITreeView::setExpandersAsIcons( bool expandersAsIcons ) {
	mExpandersAsIcons = expandersAsIcons;
}

Float UITreeView::getMaxColumnContentWidth( const size_t& colIndex, bool ) {
	Float lWidth = 0;
	getUISceneNode()->setIsLoading( true );
	traverseTree( [&, colIndex]( const int&, const ModelIndex& index, const size_t& indentLevel,
								 const Float& yOffset ) {
		UIWidget* widget = updateCell(
			0, getModel()->index( index.row(), colIndex, index.parent() ), indentLevel, yOffset );
		if ( widget->isType( UI_TYPE_PUSHBUTTON ) ) {
			Float w = widget->asType<UIPushButton>()->getContentSize().getWidth();
			if ( w > lWidth )
				lWidth = w;
		}
		return IterationDecision::Continue;
	} );
	getUISceneNode()->setIsLoading( false );
	return lWidth;
}

const size_t& UITreeView::getExpanderIconSize() const {
	return mExpanderIconSize;
}

void UITreeView::setExpanderIconSize( const size_t& expanderSize ) {
	mExpanderIconSize = expanderSize;
}

Uint32 UITreeView::onKeyDown( const KeyEvent& event ) {
	if ( event.getMod() )
		return UIAbstractTableView::onKeyDown( event );
	auto curIndex = getSelection().first();

	switch ( event.getKeyCode() ) {
		case KEY_PAGEUP: {
			int pageSize = eefloor( getVisibleArea().getHeight() / getRowHeight() ) - 1;
			std::deque<std::pair<ModelIndex, Float>> deque;
			Float curY;
			traverseTree(
				[&]( const int&, const ModelIndex& index, const size_t&, const Float& offsetY ) {
					deque.push_back( { index, offsetY } );
					if ( (int)deque.size() > pageSize )
						deque.pop_front();
					if ( index == curIndex )
						return IterationDecision::Break;
					return IterationDecision::Continue;
				} );
			curY = deque.front().second - getHeaderHeight();
			getSelection().set( deque.front().first );
			scrollToPosition(
				{ { mScrollOffset.x, curY },
				  { columnData( deque.front().first.column() ).width, getRowHeight() } } );
			return 1;
		}
		case KEY_PAGEDOWN: {
			int pageSize = eefloor( getVisibleArea().getHeight() / getRowHeight() ) - 1;
			int counted = 0;
			bool foundStart = false;
			bool resultFound = false;
			ModelIndex foundIndex;
			Float curY = 0;
			Float lastOffsetY = 0;
			ModelIndex lastIndex;
			traverseTree(
				[&]( const int&, const ModelIndex& index, const size_t&, const Float& offsetY ) {
					if ( index == curIndex ) {
						foundStart = true;
					} else if ( foundStart ) {
						counted++;
						if ( counted == pageSize ) {
							foundIndex = index;
							curY = offsetY;
							resultFound = true;
							return IterationDecision::Break;
						}
					}
					lastOffsetY = offsetY;
					lastIndex = index;
					return IterationDecision::Continue;
				} );
			if ( !resultFound ) {
				foundIndex = lastIndex;
				curY = lastOffsetY;
			}
			curY += getRowHeight();
			getSelection().set( foundIndex );
			scrollToPosition( { { mScrollOffset.x, curY },
								{ columnData( foundIndex.column() ).width, getRowHeight() } } );
			return 1;
		}
		case KEY_UP: {
			ModelIndex prevIndex;
			ModelIndex foundIndex;
			Float curY = 0;
			traverseTree(
				[&]( const int&, const ModelIndex& index, const size_t&, const Float& offsetY ) {
					if ( index == curIndex ) {
						foundIndex = prevIndex;
						curY = offsetY;
						return IterationDecision::Break;
					}
					prevIndex = index;
					return IterationDecision::Continue;
				} );
			if ( foundIndex.isValid() ) {
				getSelection().set( foundIndex );
				if ( curY < mScrollOffset.y + getHeaderHeight() + getRowHeight() ||
					 curY > mScrollOffset.y + getPixelsSize().getHeight() - mPaddingPx.Top -
								mPaddingPx.Bottom - getRowHeight() ) {
					curY -= getHeaderHeight() + getRowHeight();
					mVScroll->setValue( eemin<Float>(
						1.f, eemax<Float>( 0.f, curY / getScrollableArea().getHeight() ) ) );
				}
			}
			return 1;
		}
		case KEY_DOWN: {
			ModelIndex prevIndex;
			ModelIndex foundIndex;
			Float curY = 0;
			traverseTree(
				[&]( const int&, const ModelIndex& index, const size_t&, const Float& offsetY ) {
					if ( prevIndex == curIndex ) {
						foundIndex = index;
						curY = offsetY;
						return IterationDecision::Break;
					}
					prevIndex = index;
					return IterationDecision::Continue;
				} );
			if ( foundIndex.isValid() ) {
				getSelection().set( foundIndex );
				if ( curY < mScrollOffset.y ||
					 curY > mScrollOffset.y + getPixelsSize().getHeight() - mPaddingPx.Top -
								mPaddingPx.Bottom - getRowHeight() ) {
					curY -=
						eefloor( getVisibleArea().getHeight() / getRowHeight() ) * getRowHeight() -
						getRowHeight();
					mVScroll->setValue(
						eemin<Float>( 1.f, curY / getScrollableArea().getHeight() ) );
				}
			}
			return 1;
		}
		case KEY_END: {
			scrollToBottom();
			ModelIndex lastIndex;
			traverseTree( [&]( const int&, const ModelIndex& index, const size_t&, const Float& ) {
				lastIndex = index;
				return IterationDecision::Continue;
			} );
			getSelection().set( lastIndex );
			return 1;
		}
		case KEY_HOME: {
			scrollToTop();
			getSelection().set( getModel()->index( 0, 0 ) );
			return 1;
		}
		case KEY_RIGHT: {
			if ( curIndex.isValid() && getModel()->rowCount( curIndex ) ) {
				auto& metadata = getIndexMetadata( curIndex );
				if ( !metadata.open ) {
					metadata.open = true;
					createOrUpdateColumns();
					return 0;
				}
				getSelection().set( getModel()->index( 0, getModel()->treeColumn(), curIndex ) );
			}
			return 1;
		}
		case KEY_LEFT: {
			if ( curIndex.isValid() && getModel()->rowCount( curIndex ) ) {
				auto& metadata = getIndexMetadata( curIndex );
				if ( metadata.open ) {
					metadata.open = false;
					createOrUpdateColumns();
					return 0;
				}
			}
			if ( curIndex.isValid() && curIndex.parent().isValid() ) {
				getSelection().set( curIndex.parent() );
				return 0;
			}
			return 1;
		}
		case KEY_RETURN:
		case KEY_KP_ENTER: {
			if ( curIndex.isValid() ) {
				if ( getModel()->rowCount( curIndex ) ) {
					auto& metadata = getIndexMetadata( curIndex );
					metadata.open = !metadata.open;
					createOrUpdateColumns();
				} else {
					onOpenModelIndex( curIndex, &event );
				}
			}
			return 1;
		}
		case KEY_MENU: {
			if ( curIndex.isValid() )
				onOpenMenuModelIndex( curIndex, &event );
			return 1;
		}
		default:
			break;
	}
	return UIAbstractTableView::onKeyDown( event );
}

void UITreeView::onOpenTreeModelIndex( const ModelIndex& index, bool open ) {
	ModelEvent event( getModel(), index, this,
					  open ? ModelEventType::OpenTree : ModelEventType::CloseTree );
	sendEvent( &event );
}

void UITreeView::onSortColumn( const size_t& ) {
	// Do nothing.
	return;
}

ModelIndex UITreeView::findRowWithText( const std::string& text, const bool& caseSensitive,
										const bool& exactMatch ) const {
	const Model* model = getModel();
	Lock l( const_cast<Model*>( getModel() )->resourceLock() );
	if ( !model || model->rowCount() == 0 )
		return {};
	ModelIndex foundIndex = {};
	traverseTree( [&]( const int&, const ModelIndex& index, const size_t&, const Float& ) {
		Variant var = model->data( index );
		if ( var.isValid() &&
			 ( exactMatch ? var.toString() == text
						  : String::startsWith( caseSensitive ? var.toString()
															  : String::toLower( var.toString() ),
												text ) ) ) {
			foundIndex = index;
			return IterationDecision::Stop;
		}
		return IterationDecision::Continue;
	} );
	return foundIndex;
}

ModelIndex UITreeView::selectRowWithPath( std::string path ) {
	const Model* model = getModel();
	if ( !model || model->rowCount() == 0 )
		return {};
	String::replaceAll( path, "\\", "/" );
	auto pathPart = String::split( path, "/" );
	if ( pathPart.empty() )
		return {};

	for ( size_t i = 0; i < pathPart.size(); i++ ) {
		ModelIndex foundIndex = {};
		const auto& part = pathPart[i];

		traverseTree( [&]( const int&, const ModelIndex& index, const size_t&, const Float& ) {
			Variant var = model->data( index );
			if ( var.isValid() && var.toString() == part ) {
				foundIndex = index;
				return IterationDecision::Stop;
			}
			return IterationDecision::Continue;
		} );

		if ( foundIndex == ModelIndex() )
			break;

		size_t rowCount = 0;
		{
			Lock l( const_cast<Model*>( getModel() )->resourceLock() );
			rowCount = getModel()->rowCount( foundIndex );
		}
		if ( rowCount ) {
			auto& data = getIndexMetadata( foundIndex );
			if ( !data.open ) {
				data.open = true;
				createOrUpdateColumns();
				onOpenTreeModelIndex( foundIndex, data.open );
			}
		}

		if ( i == pathPart.size() - 1 ) {
			setSelection( foundIndex );
			return foundIndex;
		}
	}
	return {};
}

void UITreeView::setSelection( const ModelIndex& index, bool scrollToSelection ) {
	if ( !getModel() )
		return;
	auto& model = *this->getModel();
	if ( model.isValid( index ) && scrollToSelection ) {
		getSelection().set( index );

		ModelIndex prevIndex;
		ModelIndex foundIndex;
		Float curY = 0;
		traverseTree(
			[&]( const int&, const ModelIndex& _index, const size_t&, const Float& offsetY ) {
				if ( index == _index ) {
					foundIndex = prevIndex;
					curY = offsetY;
					return IterationDecision::Break;
				}
				prevIndex = index;
				return IterationDecision::Continue;
			} );

		if ( foundIndex.isValid() ) {
			if ( curY < mScrollOffset.y + getHeaderHeight() + getRowHeight() ||
				 curY > mScrollOffset.y + getPixelsSize().getHeight() - mPaddingPx.Top -
							mPaddingPx.Bottom - getRowHeight() ) {
				curY -= getHeaderHeight() + getRowHeight();
				mVScroll->setValue( eemin<Float>(
					1.f, eemax<Float>( 0.f, curY / getScrollableArea().getHeight() ) ) );
			}
		}
	}
}

bool UITreeView::getFocusOnSelection() const {
	return mFocusOnSelection;
}

void UITreeView::setFocusOnSelection( bool focusOnSelection ) {
	mFocusOnSelection = focusOnSelection;
}

void UITreeView::onModelSelectionChange() {
	UIAbstractTableView::onModelSelectionChange();
	if ( mFocusOnSelection )
		mFocusSelectionDirty = true;
}

}} // namespace EE::UI
