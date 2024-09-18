#include <deque>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <stack>

namespace EE { namespace UI {

UITreeView* UITreeView::New() {
	return eeNew( UITreeView, () );
}

UITreeView::UITreeView() :
	UIAbstractTableView( "treeview" ),
	mIndentWidth( PixelDensity::dpToPx( 6 ) ),
	mExpanderIconSize( PixelDensity::dpToPxI( 12 ) ) {
	setClipType( ClipType::ContentBox );
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
	Lock l( const_cast<Model*>( getModel() )->resourceMutex() );
	auto& model = *getModel();
	int indentLevel = 0;
	Float yOffset = getHeaderHeight();
	int rowIndex = -1;
	Float rowHeight = getRowHeight();
	std::function<IterationDecision( const ModelIndex& )> traverseIndex =
		[&]( const ModelIndex& index ) {
			if ( index.isValid() ) {
				const auto& metadata = getIndexMetadata( index );
				rowIndex++;
				IterationDecision decision = callback( rowIndex, index, indentLevel, yOffset );
				if ( decision == IterationDecision::Break || decision == IterationDecision::Stop )
					return decision;
				yOffset += rowHeight;
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

void UITreeView::createOrUpdateColumns( bool resetColumnData ) {
	updateContentSize();
	if ( !getModel() )
		return;
	UIAbstractTableView::createOrUpdateColumns( resetColumnData );
}

size_t UITreeView::getItemCount() const {
	size_t count = 0;
	traverseTree( [&count]( const int&, const ModelIndex&, const size_t&, const Float& ) {
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
	auto openTree = [this]( const ModelIndex& idx, const Event* event ) {
		ConditionalLock l( getModel() != nullptr,
						   getModel() ? &getModel()->resourceMutex() : nullptr );
		if ( getModel()->rowCount( idx ) ) {
			auto& data = getIndexMetadata( idx );
			data.open = !data.open;
			createOrUpdateColumns( false );
			onOpenTreeModelIndex( idx, data.open );
		} else {
			onOpenModelIndex( idx, event );
		}
	};

	mWidgetsClickCbId[widget].push_back(
		widget->addEventListener( Event::MouseDoubleClick, [this, openTree]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			auto cellIdx = mouseEvent->getNode()->asType<UITableCell>()->getCurIndex();
			auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
			if ( isEditable() && ( mEditTriggers & EditTrigger::DoubleClicked ) && getModel() &&
				 getModel()->isEditable( cellIdx ) ) {
				beginEditing( cellIdx, mouseEvent->getNode()->asType<UIWidget>() );
			} else if ( ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) && !mSingleClickNavigation ) {
				openTree( idx, event );
			}
		} ) );

	mWidgetsClickCbId[widget].push_back(
		widget->addEventListener( Event::MouseClick, [this, openTree]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
			if ( mouseEvent->getFlags() & EE_BUTTON_RMASK ) {
				onOpenMenuModelIndex( idx, event );
			} else if ( ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) && mSingleClickNavigation ) {
				openTree( idx, event );
			}
		} ) );
}

bool UITreeView::tryOpenModelIndex( const ModelIndex& index, bool forceUpdate ) {
	size_t rowCount = 0;
	{
		ConditionalLock l( getModel() != nullptr,
						   getModel() ? &getModel()->resourceMutex() : nullptr );
		rowCount = getModel()->rowCount( index );
	}
	if ( rowCount ) {
		auto& data = getIndexMetadata( index );
		if ( !data.open ) {
			data.open = true;
			if ( forceUpdate )
				createOrUpdateColumns( false );
			onOpenTreeModelIndex( index, data.open );
		}
		return true;
	}
	return false;
}

UIWidget* UITreeView::setupCell( UITableCell* widget, UIWidget* rowWidget,
								 const ModelIndex& index ) {
	widget->setParent( rowWidget );
	widget->unsetFlags( UI_AUTO_SIZE );
	widget->setClipType( mDisableCellClipping ? ClipType::None : ClipType::ContentBox );
	widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	widget->setTextAlign( UI_HALIGN_LEFT );
	widget->setCurIndex( index );
	if ( index.column() == (Int64)getModel()->treeColumn() ) {
		bindNavigationClick( widget );
		widget->onClick( [this]( const MouseEvent* mouseEvent ) {
			if ( mSingleClickNavigation )
				return;
			UIWidget* icon = mouseEvent->getNode()->asType<UIPushButton>()->getExtraInnerWidget();
			if ( nullptr == icon )
				return;
			Vector2f pos( icon->convertToNodeSpace( mouseEvent->getPosition().asFloat() ) );
			if ( pos >= Vector2f::Zero && pos <= icon->getPixelsSize() ) {
				ConditionalLock l( getModel() != nullptr,
								   getModel() ? &getModel()->resourceMutex() : nullptr );
				auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
				if ( getModel()->rowCount( idx ) ) {
					auto& data = getIndexMetadata( idx );
					data.open = !data.open;
					createOrUpdateColumns( false );
					onOpenTreeModelIndex( idx, data.open );
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

UIWidget* UITreeView::updateCell( const Vector2<Int64>& posIndex, const ModelIndex& index,
								  const size_t& indentLevel, const Float& yOffset ) {
	if ( posIndex.y >= (int)mWidgets.size() )
		mWidgets.resize( posIndex.y + 1 );
	auto* widget = mWidgets[posIndex.y][index.column()];
	if ( !widget ) {
		UIWidget* rowWidget = updateRow( posIndex.y, index, yOffset );
		widget = createCell( rowWidget, index );
		mWidgets[posIndex.y][index.column()] = widget;
		widget->reloadStyle( true, true, true );
	}
	const auto& colData = columnData( index.column() );
	if ( !colData.visible ) {
		widget->setVisible( false, false );
		return widget;
	} else {
		widget->setVisible( true, false );
	}
	widget->setPixelsSize( colData.width, getRowHeight() );
	widget->setPixelsPosition( { getColumnPosition( index.column() ).x, 0 } );
	if ( widget->isType( UI_TYPE_TABLECELL ) ) {
		UITableCell* cell = widget->asType<UITableCell>();
		cell->setCurIndex( index );

		if ( getModel()->classModelRoleEnabled() ) {
			bool needsReloadStyle = false;
			Variant cls( getModel()->data( index, ModelRole::Class ) );
			cell->setLoadingState( true );
			if ( cls.isValid() ) {
				std::string clsStr( cls.toString() );
				needsReloadStyle = cell->getClasses().empty() || cell->getClasses().size() != 1 ||
								   clsStr != cell->getClasses()[0];
				cell->setClass( clsStr );
			} else {
				needsReloadStyle = !cell->getClasses().empty();
				cell->resetClass();
			}
			cell->setLoadingState( false );
			if ( needsReloadStyle )
				cell->reportStyleStateChangeRecursive();
		}

		Variant txt( getModel()->data( index, ModelRole::Display ) );
		if ( txt.isValid() ) {
			if ( txt.is( Variant::Type::String ) )
				cell->setText( txt.asString() );
			else if ( txt.is( Variant::Type::StringPtr ) )
				cell->setText( txt.asStringPtr() );
			else
				cell->setText( txt.toString() );
		}

		bool hasChilds = false;

		if ( widget->isType( UI_TYPE_TREEVIEW_CELL ) &&
			 index.column() == (Int64)getModel()->treeColumn() ) {
			UITreeViewCell* tcell = widget->asType<UITreeViewCell>();
			UIImage* image = widget->asType<UITreeViewCell>()->getImage();

			Float minIndent = 0;
			if ( !mExpandersAsIcons && mExpandIcon && mContractIcon ) {
				minIndent =
					eemax(
						mExpandIcon->getSize( mExpanderIconSize )->getPixelsSize().getWidth(),
						mContractIcon->getSize( mExpanderIconSize )->getPixelsSize().getWidth() ) +
					image->getLayoutPixelsMargin().Right;
			}

			Float indentation = minIndent + getIndentWidth() * indentLevel;

			hasChilds = getModel()->rowCount( index ) > 0;

			if ( hasChilds ) {
				UIIcon* icon = getIndexMetadata( index ).open ? mExpandIcon : mContractIcon;
				Drawable* drawable = icon ? icon->getSize( mExpanderIconSize ) : nullptr;

				if ( drawable == nullptr ) {
					image->setVisible( false );
				} else {
					image->setVisible( true );
					image->setPixelsSize( drawable ? drawable->getPixelsSize() : Sizef( 0, 0 ) );
					image->setDrawable( drawable );
					if ( !mExpandersAsIcons )
						indentation = indentation - image->getPixelsSize().getWidth();
				}
			} else {
				image->setVisible( false );
			}

			tcell->setIndentation( indentation );
		}

		if ( hasChilds && mExpandersAsIcons && cell->hasIcon() ) {
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
		if ( cell->hasIcon() )
			cell->getIcon()->setVisible( isVisible );

		cell->updateCell( getModel() );
	}

	if ( isCellSelection() ) {
		if ( getSelection().contains( index ) ) {
			widget->pushState( UIState::StateSelected );
		} else {
			widget->popState( UIState::StateSelected );
		}
	}

	return widget;
}

const Float& UITreeView::getIndentWidth() const {
	return mIndentWidth;
}

void UITreeView::setIndentWidth( const Float& indentWidth ) {
	if ( mIndentWidth != indentWidth ) {
		mIndentWidth = indentWidth;
		createOrUpdateColumns( false );
	}
}

Sizef UITreeView::getContentSize() const {
	return mContentSize;
}

void UITreeView::drawChilds() {
	int realRowIndex = 0;
	int realColIndex = 0;
	Float rowHeight = getRowHeight();

	traverseTree( [this, &realRowIndex, &realColIndex,
				   rowHeight]( const int&, const ModelIndex& index, const size_t& indentLevel,
							   const Float& yOffset ) {
		if ( yOffset - mScrollOffset.y > mSize.getHeight() )
			return IterationDecision::Stop;
		if ( yOffset - mScrollOffset.y + rowHeight < 0 )
			return IterationDecision::Continue;
		Float xOffset = 0;
		UITableRow* rowNode = updateRow( realRowIndex, index, yOffset );
		rowNode->setChildsVisibility( false, false );
		realColIndex = 0;
		for ( size_t colIndex = 0; colIndex < getModel()->columnCount(); colIndex++ ) {
			auto& colData = columnData( colIndex );
			if ( !colData.visible || ( xOffset + colData.width ) - mScrollOffset.x < 0 ) {
				if ( colData.visible )
					xOffset += colData.width;
				continue;
			}
			if ( xOffset - mScrollOffset.x > mSize.getWidth() )
				break;
			xOffset += colData.width;
			if ( (Int64)colIndex != index.column() ) {
				updateCell( { realColIndex, realRowIndex },
							getModel()->index( index.row(), colIndex, index.parent() ), indentLevel,
							yOffset );
			} else {
				auto* cell =
					updateCell( { realColIndex, realRowIndex }, index, indentLevel, yOffset );

				if ( mFocusSelectionDirty && index == getSelection().first() ) {
					cell->setFocus();
					mFocusSelectionDirty = false;
				}
			}
			realColIndex++;
		}
		rowNode->nodeDraw();
		realRowIndex++;
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
	ScopedOp op( [this] { mUISceneNode->setIsLoading( true ); },
				 [this] { mUISceneNode->setIsLoading( false ); } );
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
			Float rowHeight = getRowHeight();
			traverseTree( [this, &pOver, &realIndex, point, rowHeight](
							  int, const ModelIndex& index, const size_t&, const Float& yOffset ) {
				if ( yOffset - mScrollOffset.y > mSize.getHeight() )
					return IterationDecision::Stop;
				if ( yOffset - mScrollOffset.y + rowHeight < 0 )
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
	return pOver;
}

bool UITreeView::isExpanded( const ModelIndex& index ) const {
	return getIndexMetadata( index ).open;
}

void UITreeView::setExpanded( const std::vector<ModelIndex>& indexes, bool expanded ) {
	if ( !getModel() )
		return;
	Model& model = *getModel();
	for ( const auto& index : indexes ) {
		if ( !index.isValid() )
			continue;
		size_t count = model.rowCount( index );
		if ( count )
			getIndexMetadata( index ).open = expanded;
	}
	createOrUpdateColumns( false );
}

void UITreeView::setExpanded( const ModelIndex& index, bool expanded ) {
	setExpanded( std::vector{ index }, expanded );
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
}

void UITreeView::expandAll( const ModelIndex& index ) {
	if ( !getModel() )
		return;
	setAllExpanded( index, true );
	createOrUpdateColumns( false );
}

void UITreeView::collapseAll( const ModelIndex& index ) {
	if ( !getModel() )
		return;
	setAllExpanded( index, false );
	createOrUpdateColumns( false );
}

UIIcon* UITreeView::getExpandIcon() const {
	return mExpandIcon;
}

void UITreeView::setExpandedIcon( UIIcon* expandIcon ) {
	if ( mExpandIcon != expandIcon ) {
		mExpandIcon = expandIcon;
		createOrUpdateColumns( false );
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
		createOrUpdateColumns( false );
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
	ScopedOp op( [this] { mUISceneNode->setIsLoading( true ); },
				 [this] { mUISceneNode->setIsLoading( false ); } );
	traverseTree( [this, &lWidth, colIndex]( const int&, const ModelIndex& index,
											 const size_t& indentLevel, const Float& yOffset ) {
		UIWidget* widget = updateCell( { (Int64)0, (Int64)0 },
									   getModel()->index( index.row(), colIndex, index.parent() ),
									   indentLevel, yOffset );
		if ( widget->isType( UI_TYPE_PUSHBUTTON ) ) {
			Float w = widget->asType<UIPushButton>()->getContentSize().getWidth();
			if ( w > lWidth )
				lWidth = w;
		}
		return IterationDecision::Continue;
	} );
	return lWidth;
}

const size_t& UITreeView::getExpanderIconSize() const {
	return mExpanderIconSize;
}

void UITreeView::setExpanderIconSize( const size_t& expanderSize ) {
	mExpanderIconSize = expanderSize;
}

Uint32 UITreeView::onKeyDown( const KeyEvent& event ) {
	if ( event.getMod() & KEYMOD_CTRL_SHIFT_ALT_META )
		return UIAbstractTableView::onKeyDown( event );
	auto curIndex = getSelection().first();

	if ( nullptr == getModel() || getModel()->rowCount() == 0 )
		return UIAbstractTableView::onKeyDown( event );

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
					createOrUpdateColumns( false );
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
					createOrUpdateColumns( false );
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
					createOrUpdateColumns( false );
				} else {
					onOpenModelIndex( curIndex, &event );
				}
			}
			return 1;
		}
		case KEY_MENU:
		case KEY_APPLICATION: {
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

bool UITreeView::getDisableCellClipping() const {
	return mDisableCellClipping;
}

void UITreeView::setDisableCellClipping( bool disableCellClipping ) {
	mDisableCellClipping = disableCellClipping;
}

void UITreeView::onSortColumn( const size_t& ) {
	// Do nothing.
	return;
}

ModelIndex UITreeView::findRowWithText( const std::string& text, const bool& caseSensitive,
										const bool& exactMatch ) const {
	const Model* model = getModel();
	ConditionalLock l( getModel() != nullptr,
					   getModel() ? &const_cast<Model*>( getModel() )->resourceMutex() : nullptr );
	if ( !model || model->rowCount() == 0 )
		return {};
	ModelIndex foundIndex = {};
	traverseTree( [&]( const int&, const ModelIndex& index, const size_t&, const Float& ) {
		Variant var = model->data( index );
		if ( var.isValid() &&
			 ( exactMatch ? var.toString() == text
						  : String::startsWith(
								caseSensitive ? var.toString() : String::toLower( var.toString() ),
								caseSensitive ? text : String::toLower( text ) ) ) ) {
			foundIndex = index;
			return IterationDecision::Stop;
		}
		return IterationDecision::Continue;
	} );
	return foundIndex;
}

ModelIndex UITreeView::selectRowWithPath( const std::vector<std::string>& pathTree ) {
	const Model* model = getModel();
	if ( !model || model->rowCount() == 0 )
		return {};
	ModelIndex parentIndex = {};
	for ( size_t i = 0; i < pathTree.size(); i++ ) {
		ModelIndex foundIndex = {};
		const auto& part = pathTree[i];

		traverseTree(
			[&model, &foundIndex, &part, &parentIndex,
			 i]( const int&, const ModelIndex& index, const size_t& indentLevel, const Float& ) {
				Variant var = model->data( index );
				if ( i == indentLevel && var.isValid() && var.toString() == part ) {
					if ( !parentIndex.isValid() || parentIndex == index.parent() ) {
						foundIndex = index;
						return IterationDecision::Stop;
					}
				}
				return IterationDecision::Continue;
			} );

		if ( foundIndex == ModelIndex() )
			break;

		tryOpenModelIndex( foundIndex );

		parentIndex = foundIndex;

		if ( i == pathTree.size() - 1 ) {
			setSelection( foundIndex );
			return foundIndex;
		}
	}
	return {};
}

ModelIndex UITreeView::selectRowWithPath( std::string path ) {
	String::replaceAll( path, "\\", "/" );
	auto pathTree = String::split( path, "/" );
	if ( pathTree.empty() )
		return {};
	return selectRowWithPath( pathTree );
}

void UITreeView::setSelection( const ModelIndex& index, bool scrollToSelection,
							   bool openModelIndexTree ) {
	if ( !getModel() )
		return;
	auto& model = *this->getModel();
	if ( model.isValid( index ) ) {
		if ( openModelIndexTree )
			openModelIndexParentTree( index );

		getSelection().set( index );

		if ( !scrollToSelection )
			return;

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
				runOnMainThread( [this, curY] {
					scrollToPosition( { { mScrollOffset.x, curY },
										{ getPixelsSize().getWidth(), getRowHeight() } } );
				} );
			}
		}
	}
}

void UITreeView::openModelIndexParentTree( const ModelIndex& index ) {
	if ( !index.hasParent() )
		return;

	std::stack<ModelIndex> indexes;
	ModelIndex parent = index;
	while ( parent.hasParent() ) {
		indexes.push( parent );
		parent = parent.parent();
	}

	while ( !indexes.empty() ) {
		if ( !tryOpenModelIndex( indexes.top() ) )
			return;
		indexes.pop();
	}

	createOrUpdateColumns( false );
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

bool UITreeViewCell::isType( const Uint32& type ) const {
	return UITreeViewCell::getType() == type ? true : UITableCell::isType( type );
}

Rectf UITreeViewCell::calculatePadding() const {
	Sizef size;
	Rectf autoPadding;
	if ( mFlags & UI_AUTO_PADDING ) {
		autoPadding = makePadding( true, true, true, true );
		if ( autoPadding != Rectf() )
			autoPadding = PixelDensity::dpToPx( autoPadding );
	}
	if ( mPaddingPx.Top > autoPadding.Top )
		autoPadding.Top = mPaddingPx.Top;
	if ( mPaddingPx.Bottom > autoPadding.Bottom )
		autoPadding.Bottom = mPaddingPx.Bottom;
	if ( mPaddingPx.Left > autoPadding.Left )
		autoPadding.Left = mPaddingPx.Left;
	if ( mPaddingPx.Right > autoPadding.Right )
		autoPadding.Right = mPaddingPx.Right;
	autoPadding.Left += mIndent;
	return autoPadding;
}

void UITreeViewCell::setIndentation( const Float& indent ) {
	if ( mIndent != indent ) {
		mIndent = indent;
		updateLayout();
	}
}

UITreeViewCell::UITreeViewCell( const std::function<UITextView*( UIPushButton* )>& newTextViewCb ) :
	UITableCell( "treeview::cell", newTextViewCb ) {
	mTextBox->setElementTag( mTag + "::text" );
	mInnerWidgetOrientation = InnerWidgetOrientation::WidgetIconTextBox;
	auto cb = [this]( const Event* ) { updateLayout(); };
	mImage = UIImage::NewWithTag( mTag + "::expander" );
	mImage->setScaleType( UIScaleType::FitInside )
		->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
		->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER )
		->setParent( const_cast<UITreeViewCell*>( this ) )
		->setVisible( false )
		->setEnabled( false );
	mImage->addEventListener( Event::OnPaddingChange, cb );
	mImage->addEventListener( Event::OnMarginChange, cb );
	mImage->addEventListener( Event::OnSizeChange, cb );
	mImage->addEventListener( Event::OnVisibleChange, cb );
}

UIWidget* UITreeViewCell::getExtraInnerWidget() const {
	return mImage;
}

}} // namespace EE::UI
