#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitreeview.hpp>

namespace EE { namespace UI {

struct UITreeView::MetadataForIndex {
	bool open{false};
};

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
		return *it->second;
	auto newMetadata = std::make_unique<MetadataForIndex>();
	auto* ref = newMetadata.get();
	mViewMetadata.insert( {index.data(), std::move( newMetadata )} );
	return *ref;
}

UIWidget* UITreeView::getIndexWidget( const int& column, void* data ) {
	auto it = mWidgets[column].find( data );
	if ( it != mWidgets[column].end() )
		return it->second;
	return nullptr;
}

template <typename Callback> void UITreeView::traverseTree( Callback callback ) const {
	eeASSERT( getModel() );
	auto& model = *getModel();
	int indentLevel = 1;
	Float yOffset = getHeaderHeight();
	std::function<IterationDecision( const ModelIndex& )> traverseIndex =
		[&]( const ModelIndex& index ) {
			if ( index.isValid() ) {
				auto& metadata = getIndexMetadata( index );
				IterationDecision decision = callback( index, indentLevel, yOffset );
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
	traverseTree( [&]( const ModelIndex&, const size_t&, const Float& ) {
		count++;
		return IterationDecision::Continue;
	} );
	return count;
}

void UITreeView::onColumnSizeChange( const size_t& ) {
	updateContentSize();
}

UIWidget* UITreeView::updateRow( const ModelIndex& index, const Float& yOffset ) {
	UIWidget* rowWidget = nullptr;
	auto it = mRows.find( index.data() );
	if ( it == mRows.end() ) {
		rowWidget = UIWidget::NewWithTag( "table::row" );
		rowWidget->clipEnable();
		rowWidget->setParent( this );
		rowWidget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		rowWidget->reloadStyle( true, true, true );
		mRows.insert( {index.data(), rowWidget} );
	} else {
		rowWidget = it->second;
	}
	rowWidget->setPixelsSize( getContentSize().getWidth(), getRowHeight() );
	rowWidget->setPixelsPosition( {-mScrollOffset.x, yOffset - mScrollOffset.y} );
	return rowWidget;
}

UIWidget* UITreeView::createCell( UIWidget* rowWidget, const ModelIndex& index,
								  const size_t& col ) {
	UIPushButton* widget = UIPushButton::NewWithTag( "table::cell" );
	widget->setParent( rowWidget );
	widget->unsetFlags( UI_AUTO_SIZE );
	widget->clipEnable();
	widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	widget->asType<UIPushButton>()->setTextAlign( UI_HALIGN_LEFT );
	if ( col == getModel()->treeColumn() ) {
		widget->addEventListener( Event::MouseDoubleClick, [&, index]( const Event* event ) {
			auto mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				auto& data = getIndexMetadata( index );
				data.open = !data.open;
				createOrUpdateColumns();
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

UIWidget* UITreeView::updateCell( const ModelIndex& index, const size_t& col,
								  const size_t& indentLevel, const Float& yOffset ) {
	auto* widget = getIndexWidget( col, index.data() );
	if ( !widget ) {
		UIWidget* rowWidget = updateRow(
			getModel()->index( index.row(), getModel()->treeColumn(), index.parent() ), yOffset );
		widget = createCell( rowWidget, index, col );
		mWidgets[col].insert( {index.data(), widget} );
	}
	widget->setPixelsSize( columnData( col ).width, getRowHeight() );
	widget->setPixelsPosition( {getColumnPosition( col ).x, 0} );

	if ( col == getModel()->treeColumn() )
		widget->setPaddingLeft( getIndentWidth() * indentLevel );

	ModelIndex idx( getModel()->index( index.row(), col, index.parent() ) );

	Variant variant( getModel()->data( idx, Model::Role::Display ) );

	if ( widget->isType( UI_TYPE_PUSHBUTTON ) ) {
		UIPushButton* pushButton = widget->asType<UIPushButton>();
		if ( variant.isValid() ) {
			if ( variant.is( Variant::Type::String ) )
				pushButton->setText( variant.asString() );
			else if ( variant.is( Variant::Type::cstr ) )
				pushButton->setText( variant.asCStr() );
		}
		if ( col == getModel()->treeColumn() && getModel()->rowCount( index ) > 0 )
			pushButton->setIcon( getIndexMetadata( index ).open ? mExpandIcon : mContractIcon );
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
	traverseTree( [&]( const ModelIndex& index, const size_t& indentLevel, const Float& yOffset ) {
		if ( yOffset - mScrollOffset.y > mSize.getHeight() )
			return IterationDecision::Stop;
		if ( yOffset - mScrollOffset.y + getRowHeight() < 0 )
			return IterationDecision::Continue;
		for ( size_t colIndex = 0; colIndex < getModel()->columnCount(); colIndex++ )
			updateCell( index, colIndex, indentLevel, yOffset );
		updateRow( getModel()->index( index.row(), getModel()->treeColumn(), index.parent() ),
				   yOffset )
			->nodeDraw();
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
			traverseTree(
				[&, point]( const ModelIndex& index, const size_t&, const Float& yOffset ) {
					if ( yOffset - mScrollOffset.y > mSize.getHeight() )
						return IterationDecision::Stop;
					if ( yOffset - mScrollOffset.y + getRowHeight() < 0 )
						return IterationDecision::Continue;
					pOver = updateRow( getModel()->index( index.row(), getModel()->treeColumn(),
														  index.parent() ),
									   yOffset )
								->overFind( point );
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
	traverseTree(
		[&, colIndex]( const ModelIndex& index, const size_t& indentLevel, const Float& yOffset ) {
			UIWidget* widget = updateCell( index, colIndex, indentLevel, yOffset );
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

}} // namespace EE::UI
