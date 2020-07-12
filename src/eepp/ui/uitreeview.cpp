#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitreeview.hpp>

namespace EE { namespace UI {

UITreeView* UITreeView::New() {
	return eeNew( UITreeView, () );
}

UITreeView::UITreeView() : UIAbstractTableView( "treeview" ), mIndentWidth( 16 ) {
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
	auto it = mViewMetadata.find( index.data() );
	if ( it != mViewMetadata.end() )
		return it->second;
	auto newMetadata = MetadataForIndex();
	auto* ref = &newMetadata;
	mViewMetadata.insert( {index.data(), std::move( newMetadata )} );
	return *ref;
}

template <typename Callback> void UITreeView::traverseTree( Callback callback ) const {
	eeASSERT( getModel() );
	auto& model = *getModel();
	int indentLevel = 1;
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
			if ( indentLevel > 0 && !index.isValid() )
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
	UIAbstractTableView::createOrUpdateColumns();
	updateContentSize();
}

size_t UITreeView::getItemCount() const {
	size_t count = 0;
	traverseTree( [&]( const int&, const ModelIndex&, const size_t&, const Float& ) {
		count++;
		return IterationDecision::Continue;
	} );
	return count;
}

void UITreeView::onColumnSizeChange( const size_t& ) {
	updateContentSize();
}

class UITableRow : public UIWidget {
  public:
	UITableRow( const std::string& tag ) : UIWidget( tag ) {}

	ModelIndex getCurIndex() const { return mCurIndex; }

	void setCurIndex( const ModelIndex& curIndex ) { mCurIndex = curIndex; }

  protected:
	virtual Uint32 onMessage( const NodeMessage* msg ) {
		if ( msg->getMsg() == NodeMessage::MouseDown && ( msg->getFlags() & EE_BUTTON_LMASK ) &&
			 ( !getEventDispatcher()->getMouseDownNode() ||
			   getEventDispatcher()->getMouseDownNode() == this ||
			   isParentOf( getEventDispatcher()->getMouseDownNode() ) ) &&
			 getEventDispatcher()->getNodeDragging() == nullptr ) {
			sendMouseEvent( Event::MouseDown, getEventDispatcher()->getMousePos(),
							msg->getFlags() );
		}
		return 0;
	}

	ModelIndex mCurIndex;
};

UIWidget* UITreeView::updateRow( const int& rowIndex, const ModelIndex& index,
								 const Float& yOffset ) {
	if ( rowIndex >= (int)mRows.size() )
		mRows.resize( rowIndex + 1, nullptr );
	UITableRow* rowWidget = nullptr;
	if ( mRows[rowIndex] == nullptr ) {
		rowWidget = eeNew( UITableRow, ( "table::row" ) );
		rowWidget->clipEnable();
		rowWidget->setParent( this );
		rowWidget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		rowWidget->reloadStyle( true, true, true );
		rowWidget->addEventListener( Event::MouseDown, [&]( const Event* event ) {
			if ( ( !getEventDispatcher()->getMouseDownNode() ||
				   getEventDispatcher()->getMouseDownNode() == this ||
				   isParentOf( getEventDispatcher()->getMouseDownNode() ) ) &&
				 getEventDispatcher()->getNodeDragging() == nullptr ) {
				getSelection().set( event->getNode()->asType<UITableRow>()->getCurIndex() );
			}
		} );
		mRows[rowIndex] = rowWidget;
	} else {
		rowWidget = mRows[rowIndex];
	}
	rowWidget->setCurIndex( index );
	rowWidget->setPixelsSize( getContentSize().getWidth(), getRowHeight() );
	rowWidget->setPixelsPosition( {-mScrollOffset.x, yOffset - mScrollOffset.y} );
	if ( getSelection().first() == index ) {
		rowWidget->pushState( UIState::StateSelected );
	} else {
		rowWidget->popState( UIState::StateSelected );
	}
	return rowWidget;
}

UIWidget* UITreeView::createCell( UIWidget* rowWidget, const ModelIndex&, const size_t& col ) {
	UIPushButton* widget = UIPushButton::NewWithTag( "table::cell" );
	widget->setParent( rowWidget );
	widget->unsetFlags( UI_AUTO_SIZE );
	widget->clipEnable();
	widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	widget->asType<UIPushButton>()->setTextAlign( UI_HALIGN_LEFT );
	if ( col == getModel()->treeColumn() ) {
		widget->addEventListener( Event::MouseDoubleClick, [&]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			auto idx = mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				if ( getModel()->rowCount( idx ) ) {
					auto& data = getIndexMetadata( idx );
					data.open = !data.open;
					createOrUpdateColumns();
				} else {
					onOpenModelIndex( idx );
				}
			}
		} );
		widget->addEventListener( Event::MouseClick, [&]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			UIImage* icon = mouseEvent->getNode()->asType<UIPushButton>()->getIcon();
			if ( icon ) {
				Vector2f pos( icon->convertToNodeSpace( mouseEvent->getPosition().asFloat() ) );
				if ( pos >= Vector2f::Zero && pos <= icon->getPixelsSize() ) {
					auto idx =
						mouseEvent->getNode()->getParent()->asType<UITableRow>()->getCurIndex();
					auto& data = getIndexMetadata( idx );
					if ( getModel()->rowCount( idx ) ) {
						data.open = !data.open;
						createOrUpdateColumns();
					}
				}
			}
		} );
	}
	return widget;
}

void UITreeView::onScrollChange() {
	mHeader->setPixelsPosition( -mScrollOffset.x, 0 );
	invalidateDraw();
}

void UITreeView::updateContentSize() {
	Sizef oldSize( mContentSize );
	mContentSize = UIAbstractTableView::getContentSize();
	if ( oldSize != mContentSize )
		onContentSizeChange();
}

UIWidget* UITreeView::updateCell( const int& rowIndex, const ModelIndex& index, const size_t& col,
								  const size_t& indentLevel, const Float& yOffset ) {
	if ( rowIndex >= (int)mWidgets.size() )
		mWidgets.resize( rowIndex + 1 );
	auto* widget = mWidgets[rowIndex][col];
	if ( !widget ) {
		UIWidget* rowWidget = updateRow( rowIndex, index, yOffset );
		widget = createCell( rowWidget, index, col );
		mWidgets[rowIndex][col] = widget;
	}
	widget->setPixelsSize( columnData( col ).width, getRowHeight() );
	widget->setPixelsPosition( {getColumnPosition( col ).x, 0} );

	if ( col == getModel()->treeColumn() )
		widget->setPaddingLeft( getIndentWidth() * indentLevel );

	ModelIndex idx( getModel()->index( index.row(), col, index.parent() ) );

	Variant txt( getModel()->data( idx, Model::Role::Display ) );

	if ( widget->isType( UI_TYPE_PUSHBUTTON ) ) {
		UIPushButton* pushButton = widget->asType<UIPushButton>();
		if ( txt.isValid() ) {
			if ( txt.is( Variant::Type::String ) )
				pushButton->setText( txt.asString() );
			else if ( txt.is( Variant::Type::cstr ) )
				pushButton->setText( txt.asCStr() );
		}
		pushButton->setIcon( nullptr );
		if ( col == getModel()->treeColumn() )
			pushButton->setIcon( getIndexMetadata( index ).open ? mExpandIcon : mContractIcon );
		Variant icon( getModel()->data( idx, Model::Role::Icon ) );
		if ( icon.is( Variant::Type::Drawable ) )
			pushButton->setIcon( icon.asDrawable() );
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
	traverseTree( [&]( const int& rowIndex, const ModelIndex& index, const size_t& indentLevel,
					   const Float& yOffset ) {
		if ( yOffset - mScrollOffset.y > mSize.getHeight() )
			return IterationDecision::Stop;
		if ( yOffset - mScrollOffset.y + getRowHeight() < 0 )
			return IterationDecision::Continue;
		for ( size_t colIndex = 0; colIndex < getModel()->columnCount(); colIndex++ )
			updateCell( rowIndex, index, colIndex, indentLevel, yOffset );
		updateRow( rowIndex, index, yOffset )->nodeDraw();
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
			traverseTree( [&, point]( int rowIndex, const ModelIndex& index, const size_t&,
									  const Float& yOffset ) {
				if ( yOffset - mScrollOffset.y > mSize.getHeight() )
					return IterationDecision::Stop;
				if ( yOffset - mScrollOffset.y + getRowHeight() < 0 )
					return IterationDecision::Continue;
				pOver = updateRow( rowIndex, index, yOffset )->overFind( point );
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

Drawable* UITreeView::getExpandIcon() const {
	return mExpandIcon;
}

void UITreeView::setExpandedIcon( Drawable* expandIcon ) {
	if ( mExpandIcon != expandIcon ) {
		mExpandIcon = expandIcon;
		createOrUpdateColumns();
	}
}

Drawable* UITreeView::getContractIcon() const {
	return mContractIcon;
}

void UITreeView::setContractedIcon( Drawable* contractIcon ) {
	if ( mContractIcon != contractIcon ) {
		mContractIcon = contractIcon;
		createOrUpdateColumns();
	}
}

void UITreeView::onColumnResizeToContent( const size_t& colIndex ) {
	UIWidget* lWidget = nullptr;
	Float lWidth = 0;
	getUISceneNode()->setIsLoading( true );
	traverseTree( [&, colIndex]( const int& rowIndex, const ModelIndex& index,
								 const size_t& indentLevel, const Float& yOffset ) {
		UIWidget* widget = updateCell( rowIndex, index, colIndex, indentLevel, yOffset );
		if ( widget->isType( UI_TYPE_PUSHBUTTON ) ) {
			Float w = widget->asType<UIPushButton>()->getContentSize().getWidth();
			if ( w > lWidth ) {
				lWidget = widget;
				lWidth = w;
			}
		}
		return IterationDecision::Continue;
	} );
	getUISceneNode()->setIsLoading( false );
	if ( lWidget ) {
		columnData( colIndex ).width = lWidth;
		createOrUpdateColumns();
	}
}

void UITreeView::onModelSelectionChange() {
	UIAbstractTableView::onModelSelectionChange();
	invalidateDraw();
}

Uint32 UITreeView::onKeyDown( const KeyEvent& event ) {
	if ( event.getMod() != 0 )
		return 0;

	auto curIndex = getSelection().first();

	switch ( event.getKeyCode() ) {
		case KEY_UP: {
			ModelIndex prevIndex;
			ModelIndex foundIndex;
			Float curY;
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
					 curY > mScrollOffset.y + getPixelsSize().getHeight() - mRealPadding.Top -
								mRealPadding.Bottom - getRowHeight() ) {
					curY -= getHeaderHeight() + getRowHeight();
					mVScroll->setValue( eemin<Float>(
						1.f, eemax<Float>( 0.f, curY / getScrollableArea().getHeight() ) ) );
				}
			}
			break;
		}
		case KEY_DOWN: {
			ModelIndex prevIndex;
			ModelIndex foundIndex;
			Float curY;
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
					 curY > mScrollOffset.y + getPixelsSize().getHeight() - mRealPadding.Top -
								mRealPadding.Bottom - getRowHeight() ) {
					curY -=
						eefloor( getVisibleArea().getHeight() / getRowHeight() ) * getRowHeight() -
						getRowHeight();
					mVScroll->setValue(
						eemin<Float>( 1.f, curY / getScrollableArea().getHeight() ) );
				}
				break;
			}
		}
		case KEY_END: {
			scrollToBottom();
			ModelIndex lastIndex;
			traverseTree( [&]( const int&, const ModelIndex& index, const size_t&, const Float& ) {
				lastIndex = index;
				return IterationDecision::Continue;
			} );
			getSelection().set( lastIndex );
			break;
		}
		case KEY_HOME: {
			scrollToTop();
			getSelection().set( getModel()->index( 0, 0 ) );
			break;
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
			break;
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
			break;
		}
		case KEY_RETURN:
		case KEY_SPACE: {
			if ( curIndex.isValid() ) {
				if ( getModel()->rowCount( curIndex ) ) {
					auto& metadata = getIndexMetadata( curIndex );
					metadata.open = !metadata.open;
					createOrUpdateColumns();
				} else {
					onOpenModelIndex( curIndex );
				}
			}
			break;
		}
		default:
			break;
	}
	return 0;
}

void UITreeView::onOpenModelIndex( const ModelIndex& index ) {
	ModelEvent event( getModel(), index, this );
	sendEvent( &event );
}

}} // namespace EE::UI
