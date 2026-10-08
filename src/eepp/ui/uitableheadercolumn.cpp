#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitableheadercolumn.hpp>
#include <eepp/window/input.hpp>

namespace EE { namespace UI {

UITableHeaderColumn::UITableHeaderColumn( const std::string& parentTag, UIAbstractTableView* view,
										  const size_t& colIndex ) :
	UIPushButton( parentTag + "::header::column" ), mView( view ), mColIndex( colIndex ) {
	setDragEnabled( true );
	mInnerWidgetOrientation = InnerWidgetOrientation::IconTextBoxWidget;
	auto cb = [this]( const Event* ) { updateLayout(); };
	mImage = UIImage::NewWithTag( mTag + "::arrow" );
	mImage->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER )
		->setParent( this );
	mImage->setEnabled( false );
	mImage->on( Event::OnPaddingChange, cb );
	mImage->on( Event::OnMarginChange, cb );
	mImage->on( Event::OnSizeChange, cb );
	mImage->on( Event::OnVisibleChange, cb );
}

Uint32 UITableHeaderColumn::onCalculateDrag( const Vector2f& position, const Uint32& flags ) {
	if ( isDragEnabled() && isDragging() && NULL != getEventDispatcher() ) {
		EventDispatcher* eventDispatcher = getEventDispatcher();
		if ( !( flags /*press trigger*/ & mDragButton ) ) {
			setDragging( false );
			eventDispatcher->setNodeDragging( NULL );
			return 1;
		}
		Vector2f pos( eefloor( position.x ), eefloor( position.y ) );
		if ( mDragPoint != pos && std::abs( mDragPoint.x - pos.x ) > 1.f ) {
			Sizef dragDiff( (Float)( mDragPoint.x - pos.x ), 0 );
			if ( onDrag( pos, flags, dragDiff ) ) {
				mDragPoint = pos;
				eventDispatcher->setNodeDragging( this );
			}
		}
	}
	return 1;
}

Sizef UITableHeaderColumn::updateLayout() {
	Sizef res = UIPushButton::updateLayout();
	updateSortIconPosition();
	return res;
}

void UITableHeaderColumn::updateSortIconPosition() {
	mImage->setPixelsPosition( getPixelsSize().getWidth() - mImage->getPixelsSize().getWidth() -
								   mImage->getLayoutPixelsMargin().Right,
							   0 );
	mImage->centerVertical();
}

Uint32 UITableHeaderColumn::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
	if ( NULL != getEventDispatcher() && !getEventDispatcher()->isNodeDragging() &&
		 !( getEventDispatcher()->getLastPressTrigger() & mDragButton ) &&
		 ( flags & mDragButton ) && isDragEnabled() && !isDragging() ) {
		if ( localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
			mDragMode = DragMode::Resize;
			setFocus();
			startDragging( position.asFloat() );
		} else if ( mView->isColumnReorderingEnabled() ) {
			mDragMode = DragMode::ReorderPending;
			mReorderGrabX = localPos.x;
			mReorderPressPos = position.asFloat();
		}
	}
	pushState( UIState::StatePressed );
	return Node::onMouseDown( position, flags );
}

Uint32 UITableHeaderColumn::onMouseClick( const Vector2i& position, const Uint32& flags ) {
	Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
	if ( ( flags & EE_BUTTON_LMASK ) && !isDragging() &&
		 !getEventDispatcher()->justFinishDragging() &&
		 localPos.x < mSize.getWidth() - mView->getDragBorderDistance() ) {
		mView->onSortColumn( mColIndex );
		return 1;
	}
	return UIPushButton::onMouseClick( position, flags );
}

Uint32 UITableHeaderColumn::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( flags & mDragButton && mDragMode == DragMode::ReorderPending )
		mDragMode = DragMode::None;
	if ( ( flags & EE_BUTTON_RMASK ) && mView->isColumnWidthModeMenuEnabled() &&
		 mView->getModel() ) {
		auto* menu = UIPopUpMenu::New();
		const Model* model = mView->getModel();
		const size_t columnCount = model->columnCount();
		const auto columnLabel = [this, model]( size_t column ) {
			std::string label = model->columnName( column );
			if ( label.empty() )
				label = String::format( i18n( "uitable_column_number", "Column %zu" ).toUtf8(),
										column + 1 );
			return label;
		};
		bool hasColumnVisibilityItems = false;
		bool hasHiddenColumnItems = false;

		if ( mView->visibleColumnCount() > 1 ) {
			const std::string title =
				String::format( i18n( "uitable_hide_column", "Hide Column '%s'" ).toUtf8(),
								columnLabel( mColIndex ).c_str() );
			menu->add( title )->setId( "hide-column" )->setData( mColIndex );
			hasColumnVisibilityItems = true;
		}

		for ( size_t col = 0; col < columnCount; ++col ) {
			if ( !mView->isColumnHidden( col ) )
				continue;
			if ( hasColumnVisibilityItems && !hasHiddenColumnItems )
				menu->addSeparator();
			const std::string title =
				String::format( i18n( "uitable_show_column", "Show Column '%s'" ).toUtf8(),
								columnLabel( col ).c_str() );
			menu->add( title )->setId( "show-column" )->setData( col );
			hasColumnVisibilityItems = true;
			hasHiddenColumnItems = true;
		}

		if ( hasColumnVisibilityItems )
			menu->addSeparator();

		menu->addRadioButton( i18n( "uitable_fit_columns_to_view", "Fit Columns to View" ),
							  mView->getColumnWidthMode() ==
								  UIAbstractTableView::ColumnWidthMode::Percentage )
			->setId( "percentage" );
		menu->addRadioButton( i18n( "uitable_free_column_widths", "Free Column Widths" ),
							  mView->getColumnWidthMode() ==
								  UIAbstractTableView::ColumnWidthMode::Pixels )
			->setId( "pixels" );
		menu->on( Event::OnItemClicked, [view = mView, columnCount]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;

			const auto* item = event->getNode();
			const std::string id( item->getId() );
			const size_t column = static_cast<size_t>( item->getData() );
			if ( id == "hide-column" ) {
				if ( column < columnCount && view->visibleColumnCount() > 1 ) {
					if ( view->getMainColumn() == column ) {
						for ( size_t next = 0; next < columnCount; ++next ) {
							if ( next != column && !view->isColumnHidden( next ) ) {
								view->setMainColumn( next );
								break;
							}
						}
					}
					view->setColumnHidden( column, true );
				}
			} else if ( id == "show-column" ) {
				if ( column < columnCount )
					view->setColumnHidden( column, false );
			} else if ( id == "percentage" ) {
				view->setColumnWidthMode( UIAbstractTableView::ColumnWidthMode::Percentage );
			} else if ( id == "pixels" ) {
				view->setAutoColumnsWidth( false );
				view->setColumnWidthMode( UIAbstractTableView::ColumnWidthMode::Pixels );
			}
		} );
		if ( mView->mOnHeaderContextMenuCb )
			mView->mOnHeaderContextMenuCb( menu, mColIndex );
		menu->setCloseOnHide( true );
		menu->showAtScreenPosition( position.asFloat() );
	}
	return UIPushButton::onMouseUp( position, flags );
}

Uint32 UITableHeaderColumn::onDrag( const Vector2f& position, const Uint32&,
									const Sizef& dragDiff ) {
	if ( mDragMode == DragMode::Reorder ) {
		Vector2f headerPos( mView->mHeader->convertToNodeSpace( position ) );
		const Float left = headerPos.x - mReorderGrabX;
		mView->reorderColumnAt( mColIndex, left + mSize.getWidth() * 0.5f );
		setPixelsPosition( left, getPixelsPosition().y );
		return 1;
	}
	if ( mDragMode != DragMode::Resize )
		return 0;
	Vector2f localPos( convertToNodeSpace( position ) );
	if ( isDragging() || localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
		const Float width = eemax( mSize.x - dragDiff.x, mView->columnData( mColIndex ).minWidth );
		setPixelsSize( width, mSize.getHeight() );
		if ( mSize.getWidth() != mView->columnData( mColIndex ).width ) {
			mView->columnData( mColIndex ).setWidth( mSize.getWidth(), true );
			mView->updateHeaderSize();
			mView->onColumnSizeChange( mColIndex, true );
		}
		return 1;
	}
	return 0;
}

Uint32 UITableHeaderColumn::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
	if ( mDragMode == DragMode::ReorderPending && getInput() &&
		 ( getInput()->getPressTrigger() & mDragButton ) ) {
		mDragMode = DragMode::Reorder;
		startDragging( mReorderPressPos );
	}
	if ( !isDragging() )
		getUISceneNode()->setCursor( Cursor::Arrow );
	return UIPushButton::onMouseLeave( position, flags );
}

Uint32 UITableHeaderColumn::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	if ( mDragMode == DragMode::ReorderPending && ( flags & mDragButton ) &&
		 std::abs( position.x - mReorderPressPos.x ) >= mView->getDragBorderDistance() ) {
		mDragMode = DragMode::Reorder;
		startDragging( mReorderPressPos );
	}
	Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
	if ( isDragging() || localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
		getUISceneNode()->setCursor( Cursor::SizeWE );
	} else if ( !isDragging() ) {
		getUISceneNode()->setCursor( Cursor::Arrow );
	}
	return UIPushButton::onMouseMove( position, flags );
}

Uint32 UITableHeaderColumn::onMouseDoubleClick( const Vector2i& position, const Uint32& flags ) {
	Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
	if ( localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() )
		mView->onColumnResizeToContent( mColIndex );
	return UIPushButton::onMouseDoubleClick( position, flags );
}

Uint32 UITableHeaderColumn::onDragStop( const Vector2i& pos, const Uint32& flags ) {
	getUISceneNode()->setCursor( Cursor::Arrow );
	if ( mDragMode == DragMode::Resize ) {
		mView->columnData( mColIndex ).setWidth( mSize.getWidth(), true );
		mView->updateHeaderSize();
		mView->onColumnSizeChange( mColIndex, true );
	} else if ( mDragMode == DragMode::Reorder ) {
		mView->mHeader->updateLayout();
	}
	mDragMode = DragMode::None;
	return UIPushButton::onDragStop( pos, flags );
}

UIWidget* UITableHeaderColumn::getExtraInnerWidget() const {
	return mImage;
}

}} // namespace EE::UI
