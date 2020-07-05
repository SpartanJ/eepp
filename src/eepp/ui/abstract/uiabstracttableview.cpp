#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI { namespace Abstract {

class UITableHeaderColumn : public UIPushButton {
  public:
	UITableHeaderColumn( UIAbstractTableView* view, const size_t& colIndex ) :
		UIPushButton( "table::header::column" ), mView( view ), mColIndex( colIndex ) {
		setDragEnabled( true );
	}

  protected:
	UIAbstractTableView* mView;
	size_t mColIndex;

	Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags ) {
		if ( isDragEnabled() && isDragging() && NULL != getEventDispatcher() ) {
			EventDispatcher* eventDispatcher = getEventDispatcher();
			if ( !( flags /*press trigger*/ & mDragButton ) ) {
				setDragging( false );
				eventDispatcher->setNodeDragging( NULL );
				return 1;
			}
			Vector2f pos( eefloor( position.x ), eefloor( position.y ) );
			if ( mDragPoint != pos && std::abs( mDragPoint.x - pos.x ) > 1.f ) {
				Sizef dragDiff( ( Float )( mDragPoint.x - pos.x ), 0 );
				if ( onDrag( pos, flags, dragDiff ) ) {
					mDragPoint = pos;
					eventDispatcher->setNodeDragging( this );
				}
			}
		}
		return 1;
	}

	Uint32 onMouseDown( const Vector2i& position, const Uint32& flags ) {
		Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
		if ( NULL != getEventDispatcher() && !getEventDispatcher()->isNodeDragging() &&
			 !( getEventDispatcher()->getLastPressTrigger() & mDragButton ) &&
			 ( flags & mDragButton ) && isDragEnabled() && !isDragging() &&
			 localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
			startDragging( position.asFloat() );
		}
		pushState( UIState::StatePressed );
		return Node::onMouseDown( position, flags );
	}

	Uint32 onDrag( const Vector2f& position, const Uint32&, const Sizef& dragDiff ) {
		Vector2f localPos( convertToNodeSpace( position ) );
		if ( isDragging() || localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
			setPixelsSize( mSize.x - dragDiff.x, mSize.getHeight() );
			mView->columnData( mColIndex ).width = mSize.getWidth();
			return 1;
		}
		return 0;
	}

	Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags ) {
		if ( !isDragging() )
			getUISceneNode()->setCursor( Cursor::Arrow );
		return UIPushButton::onMouseLeave( position, flags );
	}

	Uint32 onMouseMove( const Vector2i& position, const Uint32& flags ) {
		Vector2f localPos( convertToNodeSpace( position.asFloat() ) );
		if ( isDragging() || localPos.x >= mSize.getWidth() - mView->getDragBorderDistance() ) {
			getUISceneNode()->setCursor( Cursor::SizeWE );
		} else if ( !isDragging() ) {
			getUISceneNode()->setCursor( Cursor::Arrow );
		}
		return UIPushButton::onMouseMove( position, flags );
	};

	Uint32 onDragStop( const Vector2i& pos, const Uint32& flags ) {
		getUISceneNode()->setCursor( Cursor::Arrow );
		return UIPushButton::onDragStop( pos, flags );
	}
};

UIAbstractTableView* UIAbstractTableView::New() {
	return eeNew( UIAbstractTableView, ( "abstractview" ) );
}

UIAbstractTableView::UIAbstractTableView( const std::string& tag ) :
	UIAbstractView( tag ), mDragBorderDistance( PixelDensity::dpToPx( 4 ) ) {
	mHeader = UILinearLayout::NewWithTag( "table::header", UIOrientation::Horizontal );
	mHeader->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mHeader->setParent( this );
	mHeader->setVisible( true );
	mHeader->setEnabled( true );
}

UIAbstractTableView::~UIAbstractTableView() {}

void UIAbstractTableView::selectAll() {
	getSelection().clear();
	for ( size_t itemIndex = 0; itemIndex < itemCount(); ++itemIndex ) {
		auto index = getModel()->index( itemIndex );
		getSelection().add( index );
	}
}

size_t UIAbstractTableView::itemCount() const {
	if ( !getModel() )
		return 0;
	return getModel()->rowCount();
}

void UIAbstractTableView::onModelUpdate( unsigned flags ) {
	UIAbstractView::onModelUpdate( flags );
	createOrUpdateColumns();
}

void UIAbstractTableView::createOrUpdateColumns() {
	Model* model = getModel();
	if ( !model )
		return;

	mHeader->setPixelsSize( mSize.getWidth(), getHeaderHeight() );

	size_t count = model->columnCount();

	for ( size_t i = 0; i < count; i++ ) {
		ColumnData& col = columnData( i );
		if ( !col.widget ) {
			/*UIPushButton::NewWithTag( "table::header::column" )*/;
			col.widget = eeNew( UITableHeaderColumn, ( this, i ) );
			col.widget->setParent( mHeader );
			col.widget->setEnabled( true );
			col.widget->setVisible( true );
		}
		col.visible = !isColumnHidden( i );
		col.widget->setVisible( col.visible );
		if ( !col.visible )
			continue;
		col.widget->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
		col.widget->setText( model->columnName( i ) );
		col.widget->reloadStyle( true, true, true );
		col.width = eeceil( eemax( col.width, col.widget->getPixelsSize().getWidth() ) );
		col.widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		col.widget->setPixelsSize( col.width, getHeaderHeight() );
	}

	if ( count < mColumn.size() ) {
		for ( size_t i = count; i < mColumn.size(); i++ ) {
			ColumnData& col = columnData( i );
			col.width = 0;
			col.visible = false;
			if ( col.widget ) {
				col.widget->close();
				col.widget = nullptr;
			}
		}
	}
}

Float UIAbstractTableView::getHeaderHeight() const {
	return mHeadersVisible ? eeceil( columnData( 0 ).widget
										 ? columnData( 0 ).widget->getPixelsSize().getHeight()
										 : 16 )
						   : 0;
}

void UIAbstractTableView::onSizeChange() {
	UIWidget::onSizeChange();
	mHeader->setPixelsSize( mSize.getWidth(), getHeaderHeight() );
}

const Float& UIAbstractTableView::getDragBorderDistance() const {
	return mDragBorderDistance;
}

void UIAbstractTableView::setDragBorderDistance( const Float& dragBorderDistance ) {
	mDragBorderDistance = dragBorderDistance;
}

UIAbstractTableView::ColumnData& UIAbstractTableView::columnData( const size_t& column ) const {
	if ( column >= mColumn.size() )
		mColumn.resize( column + 1 );
	return mColumn[column];
}

bool UIAbstractTableView::isColumnHidden( const size_t& column ) const {
	return !columnData( column ).visible;
}

void UIAbstractTableView::setColumnHidden( const size_t& column, bool hidden ) {
	if ( columnData( column ).visible != !hidden ) {
		columnData( column ).visible = !hidden;
		createOrUpdateColumns();
	}
}

}}} // namespace EE::UI::Abstract
