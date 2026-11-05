#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>

namespace EE { namespace UI {

UISplitter* UISplitter::New() {
	return eeNew( UISplitter, () );
}

UISplitter::UISplitter() :
	UILayout( "splitter" ),
	mOrientation( UIOrientation::Horizontal ),
	mAlwaysShowSplitter( true ),
	mHideSplitterOnEdge( false ),
	mSplitPartition( StyleSheetLength( "50%" ) ),
	mFirstWidget( NULL ),
	mLastWidget( NULL ) {
	mFlags |= UI_OWNS_CHILDREN_POSITION;
	mSplitter = UIWidget::NewWithTag( "splitter::separator" );
	mSplitter->setDragEnabled( true );
	mEventConnections += mSplitter->connect( Event::OnDragStart, [this]( const Event* ) {
		mSplitter->pushState( UIState::StateSelected );
	} );
	mEventConnections += mSplitter->connect( Event::OnDragStop, [this]( const Event* ) {
		mSplitter->popState( UIState::StateSelected );
	} );
	mSplitter->setParent( this );
	mSplitter->setMinWidth( 4 );
	mSplitter->setMinHeight( 4 );
	mEventConnections += mSplitter->connect( Event::OnSizeChange, [this]( const Event* ) {
		setLayoutDirty( LayoutInvalidation::ContainerLayout );
	} );
	mEventConnections += mSplitter->connect( Event::MouseEnter, [this]( const Event* ) {
		getUISceneNode()->setCursor( mOrientation == UIOrientation::Horizontal ? Cursor::SizeWE
																			   : Cursor::SizeNS );
	} );
	mEventConnections += mSplitter->connect( Event::MouseLeave, [this]( const Event* ) {
		getUISceneNode()->setCursor( Cursor::Arrow );
	} );
	mEventConnections += mSplitter->connect( Event::OnPositionChange, [this]( const Event* ) {
		if ( mSplitter->isDragging() && !mDirtyLayout )
			updateFromDrag();
	} );

	updateSplitterDragFlags();

	applyDefaultTheme();
}

UISplitter::~UISplitter() {}

Uint32 UISplitter::getType() const {
	return UI_TYPE_SPLITTER;
}

bool UISplitter::isType( const Uint32& type ) const {
	return UISplitter::getType() == type ? true : UILayout::isType( type );
}

const UIOrientation& UISplitter::getOrientation() const {
	return mOrientation;
}

void UISplitter::setOrientation( const UIOrientation& orientation ) {
	if ( orientation != mOrientation ) {
		mOrientation = orientation;
		updateSplitterDragFlags();
		setLayoutDirty( LayoutInvalidation::ContainerLayout );
	}
}

const bool& UISplitter::alwaysShowSplitter() const {
	return mAlwaysShowSplitter;
}

void UISplitter::setAlwaysShowSplitter( bool alwaysShowSplitter ) {
	if ( alwaysShowSplitter != mAlwaysShowSplitter ) {
		mAlwaysShowSplitter = alwaysShowSplitter;
		setLayoutDirty( LayoutInvalidation::ContainerLayout );
	}
}

const bool& UISplitter::hideSplitterOnEdge() const {
	return mHideSplitterOnEdge;
}

void UISplitter::setHideSplitterOnEdge( bool hideSplitterOnEdge ) {
	if ( hideSplitterOnEdge != mHideSplitterOnEdge ) {
		mHideSplitterOnEdge = hideSplitterOnEdge;
		setLayoutDirty( LayoutInvalidation::ContainerLayout );
	}
}

bool UISplitter::isResizable() const {
	return mResizable;
}

void UISplitter::setResizable( bool resizable ) {
	if ( resizable == mResizable )
		return;

	mResizable = resizable;

	if ( !mResizable ) {
		cancelSeparatorDrag();
		// A disabled separator receives no MouseLeave.
		if ( mSplitter->isMouseOver() )
			getUISceneNode()->setCursor( Cursor::Arrow );
	}

	updateSeparatorState();
}

UISplitter::PartitionOrigin UISplitter::getPartitionOrigin() const {
	return mPartitionOrigin;
}

void UISplitter::setPartitionOrigin( PartitionOrigin origin ) {
	if ( origin != mPartitionOrigin ) {
		mPartitionOrigin = origin;
		setLayoutDirty( LayoutInvalidation::ContainerLayout );
	}
}

bool UISplitter::isSeparatorDragging() const {
	return mSplitter->isDragging();
}

void UISplitter::cancelSeparatorDrag() {
	if ( mSplitter->isDragging() ) {
		mSplitter->setDragging( false, false );
		getEventDispatcher()->setNodeDragging( nullptr );
	}
}

const StyleSheetLength& UISplitter::getSplitPartition() const {
	return mSplitPartition;
}

void UISplitter::setSplitPartition( const StyleSheetLength& divisionSplit ) {
	if ( divisionSplit != mSplitPartition ) {
		mSplitPartition = divisionSplit;
		setLayoutDirty( LayoutInvalidation::ContainerLayout );
	}
}

void UISplitter::swap( bool swapSplitPartition ) {
	if ( isFull() ) {
		UIWidget* tmp = mFirstWidget;
		mFirstWidget = mLastWidget;
		mLastWidget = tmp;
		if ( swapSplitPartition ) {
			if ( mSplitPartition.getUnit() == StyleSheetLength::Percentage ) {
				mSplitPartition.setValue( 100.f - mSplitPartition.getValue(),
										  StyleSheetLength::Percentage );
			} else {
				Float totalSpace = getUsableSplitSpace();
				mSplitPartition = partitionFromPixels(
					totalSpace - convertLength( mSplitPartition, totalSpace ), totalSpace );
			}
		}
		setLayoutDirty( LayoutInvalidation::ContainerLayout );
	}
}

bool UISplitter::isEmpty() {
	return !mFirstWidget && !mLastWidget;
}

bool UISplitter::isFull() {
	return mFirstWidget && mLastWidget;
}

UIWidget* UISplitter::getFirstWidget() const {
	return mFirstWidget;
}

UIWidget* UISplitter::getLastWidget() const {
	return mLastWidget;
}

bool UISplitter::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::SplitterPartition:
			setSplitPartition( StyleSheetLength( attribute.value() ) );
			break;
		case PropertyId::SplitterAlwaysShow:
			setAlwaysShowSplitter( attribute.asBool() );
			break;
		case PropertyId::SplitterHideOnEdge:
			setHideSplitterOnEdge( attribute.asBool() );
			break;
		case PropertyId::SplitterResizable:
			setResizable( attribute.asBool() );
			break;
		case PropertyId::SplitterPartitionOrigin:
			setPartitionOrigin( partitionOriginFromString( attribute.value() ) );
			break;
		case PropertyId::Orientation:
			setOrientation( String::iequals( attribute.getValue(), "horizontal" )
								? UIOrientation::Horizontal
								: UIOrientation::Vertical );
			break;
		default:
			return UILayout::applyProperty( attribute );
	}

	return true;
}

std::string UISplitter::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::SplitterPartition:
			return getSplitPartition().toString();
		case PropertyId::SplitterAlwaysShow:
			return alwaysShowSplitter() ? "true" : "false";
		case PropertyId::SplitterHideOnEdge:
			return hideSplitterOnEdge() ? "true" : "false";
		case PropertyId::SplitterResizable:
			return isResizable() ? "true" : "false";
		case PropertyId::SplitterPartitionOrigin:
			return partitionOriginToString( getPartitionOrigin() );
		case PropertyId::Orientation:
			return getOrientation() == UIOrientation::Horizontal ? "horizontal" : "vertical";
		default:
			return UILayout::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UISplitter::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::SplitterPartition,		PropertyId::SplitterAlwaysShow,
				   PropertyId::SplitterHideOnEdge,		PropertyId::SplitterResizable,
				   PropertyId::SplitterPartitionOrigin, PropertyId::Orientation };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UISplitter::onChildCountChange( Node* child, const bool& removed ) {
	if ( child != mSplitter ) {
		if ( !removed ) {
			if ( !child->isWidget() ) {
				child->close();
				return;
			}
			UIWidget* childWidget = child->asType<UIWidget>();

			if ( getChildCount() > 3 ) {
				child->close();
				return;
			} else {
				if ( NULL == mFirstWidget ) {
					mFirstWidget = childWidget;
				} else {
					mLastWidget = childWidget;
				}
				childWidget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
				mSplitter->toFront();
			}
		} else {
			if ( mLastWidget == child ) {
				mLastWidget = NULL;
			} else if ( mFirstWidget == child ) {
				if ( mLastWidget ) {
					mFirstWidget = mLastWidget;
					mLastWidget = NULL;
				} else {
					mFirstWidget = NULL;
				}
			}
		}
	}

	UILayout::onChildCountChange( child, removed );
}

void UISplitter::updateFromDrag() {
	mDirtyLayout = true;
	updateSeparatorState();

	if ( UIOrientation::Horizontal == mOrientation ) {
		Float fMinSize = mFirstWidget ? mFirstWidget->getMinSize().getWidth() : 0.f;

		if ( mSplitter->getPosition().x < mPadding.Left + fMinSize ) {
			mSplitter->setPosition( mPadding.Left + fMinSize, mSplitter->getPosition().y );
		}

		Float lMinSize = mLastWidget ? mLastWidget->getMinSize().getWidth() : 0.f;

		if ( mSplitter->getPosition().x + mSplitter->getSize().getWidth() >
			 mDpSize.getWidth() - mPadding.Right - lMinSize ) {
			mSplitter->setPosition( mDpSize.getWidth() - mPadding.Right - lMinSize -
										mSplitter->getSize().getWidth(),
									mSplitter->getPosition().y );
		}

		// Minimums wider than the splitter must not push the separator past its start edge.
		if ( mSplitter->getPosition().x < mPadding.Left )
			mSplitter->setPosition( mPadding.Left, mSplitter->getPosition().y );

		mSplitter->setPixelsSize( mSplitter->getPixelsSize().getWidth(),
								  mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
	} else {
		Float fMinSize = mFirstWidget ? mFirstWidget->getMinSize().getHeight() : 0.f;

		if ( mSplitter->getPosition().y < mPadding.Top + fMinSize ) {
			mSplitter->setPosition( mSplitter->getPosition().x, mPadding.Top + fMinSize );
		}

		Float lMinSize = mLastWidget ? mLastWidget->getMinSize().getHeight() : 0.f;

		if ( mSplitter->getPosition().y + mSplitter->getSize().getHeight() >
			 mDpSize.getHeight() - mPadding.Bottom - lMinSize ) {
			mSplitter->setPosition( mSplitter->getPosition().x,
									mDpSize.getHeight() - mPadding.Bottom - lMinSize -
										mSplitter->getSize().getHeight() );
		}

		if ( mSplitter->getPosition().y < mPadding.Top )
			mSplitter->setPosition( mSplitter->getPosition().x, mPadding.Top );

		mSplitter->setPixelsSize( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
								  mSplitter->getPixelsSize().getHeight() );
	}

	if ( mFirstWidget ) {
		mFirstWidget->setPixelsPosition( mPaddingPx.Left, mPaddingPx.Top );
		if ( UIOrientation::Horizontal == mOrientation ) {
			mFirstWidget->setPixelsSize( mSplitter->getPixelsPosition().x - mPaddingPx.Left,
										 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		} else {
			mFirstWidget->setPixelsSize( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
										 mSplitter->getPixelsPosition().y - mPaddingPx.Top );
		}
	}

	if ( mLastWidget ) {
		if ( UIOrientation::Horizontal == mOrientation ) {
			mLastWidget->setPixelsPosition( mSplitter->getPixelsPosition().x +
												mSplitter->getPixelsSize().getWidth(),
											mPaddingPx.Top );
			mLastWidget->setPixelsSize( mSize.getWidth() - mPaddingPx.Right -
											mSplitter->getPixelsPosition().x -
											mSplitter->getPixelsSize().getWidth(),
										mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		} else {
			mLastWidget->setPixelsPosition( mPaddingPx.Left,
											mSplitter->getPixelsPosition().y +
												mSplitter->getPixelsSize().getHeight() );
			mLastWidget->setPixelsSize( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
										mSize.getHeight() - mPaddingPx.Bottom -
											mSplitter->getPixelsPosition().y -
											mSplitter->getPixelsSize().getHeight() );
		}
	}

	Float totalSpace = getUsableSplitSpace();
	Float firstSplit = UIOrientation::Horizontal == mOrientation
						   ? mSplitter->getPixelsPosition().x - mPaddingPx.Left
						   : mSplitter->getPixelsPosition().y - mPaddingPx.Top;
	StyleSheetLength partition = partitionFromPixels(
		mPartitionOrigin == PartitionOrigin::Start ? firstSplit : totalSpace - firstSplit,
		totalSpace );
	bool partitionChanged = partition != mSplitPartition;
	mSplitPartition = partition;

	mDirtyLayout = false;

	// Only user drags notify: an owner can store the new size without reacting to its own changes.
	if ( partitionChanged )
		sendCommonEvent( Event::OnValueChange );
}

void UISplitter::updateLayout() {
	mDirtyLayout = true;

	if ( !getParent()->isLayout() &&
		 ( !getParent()->asType<UINode>()->ownsChildPosition() || isGravityOwner() ) ) {
		bool sizeChanged = false;
		Sizef size( UIWidget::getSize() );

		if ( getLayoutWidthPolicy() == SizePolicy::MatchParent && 0 == getLayoutWeight() ) {
			Float w = getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right;

			if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
				UILayout* pLay = static_cast<UILayout*>( getParent() );
				w = w - pLay->getPadding().Left - pLay->getPadding().Right;
			}

			if ( (int)w != (int)UIWidget::getSize().getWidth() ) {
				sizeChanged = true;
				size.setWidth( w );
			}
		}

		if ( getLayoutHeightPolicy() == SizePolicy::MatchParent ) {
			Float h = getParent()->getSize().getHeight() - mLayoutMargin.Top - mLayoutMargin.Bottom;

			if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
				UILayout* pLay = static_cast<UILayout*>( getParent() );
				h = h - pLay->getPadding().Top - pLay->getPadding().Bottom;
			}

			if ( (int)h != (int)UIWidget::getSize().getHeight() ) {
				sizeChanged = true;
				size.setHeight( h );
			}
		}

		if ( sizeChanged ) {
			setInternalSize( size );
		}
	}

	if ( !mLastWidget ) {
		mSplitter->setVisible( false )->setEnabled( false );

		if ( mFirstWidget ) {
			mFirstWidget->setPixelsPosition( mPaddingPx.Left, mPaddingPx.Top );
			mFirstWidget->setPixelsSize( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
										 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		}

		mDirtyLayout = false;
		return;
	}

	updateSeparatorState();
	Float totalSpace = getUsableSplitSpace();
	Float firstSplit = getFirstPaneSize( totalSpace );
	Float secondSplit = eemax( totalSpace - firstSplit, 0.f );
	// A hidden separator takes no space: the last pane starts where the first one ends.
	Float separatorSize = !mSplitter->isVisible() ? 0.f
						  : UIOrientation::Horizontal == mOrientation
							  ? mSplitter->getPixelsSize().getWidth()
							  : mSplitter->getPixelsSize().getHeight();

	if ( mFirstWidget ) {
		mFirstWidget->setPixelsPosition( mPaddingPx.Left, mPaddingPx.Top );

		if ( UIOrientation::Horizontal == mOrientation ) {
			mFirstWidget->setPixelsSize( firstSplit,
										 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );

			mSplitter->setPixelsPosition( mFirstWidget->getPixelsPosition().x +
											  mFirstWidget->getPixelsSize().getWidth(),
										  mPaddingPx.Top );
			mSplitter->setPixelsSize( mSplitter->getPixelsSize().getWidth(),
									  mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		} else {
			mFirstWidget->setPixelsSize( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
										 firstSplit );

			mSplitter->setPixelsPosition( mPaddingPx.Left,
										  mFirstWidget->getPixelsPosition().y +
											  mFirstWidget->getPixelsSize().getHeight() );
			mSplitter->setPixelsSize( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
									  mSplitter->getPixelsSize().getHeight() );
		}
	}

	if ( mLastWidget ) {
		if ( UIOrientation::Horizontal == mOrientation ) {
			mLastWidget->setPixelsPosition( mSplitter->getPixelsPosition().x + separatorSize,
											mPaddingPx.Top );
			mLastWidget->setPixelsSize( secondSplit,
										mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		} else {
			mLastWidget->setPixelsPosition( mPaddingPx.Left,
											mSplitter->getPixelsPosition().y + separatorSize );
			mLastWidget->setPixelsSize( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
										secondSplit );
		}
	}

	mDirtyLayout = false;
}

void UISplitter::updateSplitterDragFlags() {
	mSplitter->setFlags( getOrientation() == UIOrientation::Horizontal ? UI_DRAG_HORIZONTAL
																	   : UI_DRAG_VERTICAL );
	mSplitter->unsetFlags( getOrientation() == UIOrientation::Horizontal ? UI_DRAG_VERTICAL
																		 : UI_DRAG_HORIZONTAL );
}

bool UISplitter::shouldShowSplitter() const {
	if ( !mLastWidget )
		return false;
	return mAlwaysShowSplitter || !mHideSplitterOnEdge ||
		   !isPartitionOnEdge( getAxisContentSize() );
}

void UISplitter::updateSeparatorState() {
	mSplitter->setVisible( shouldShowSplitter() );
	mSplitter->setEnabled( mSplitter->isVisible() && mResizable );
}

Float UISplitter::getAxisContentSize() const {
	return UIOrientation::Horizontal == mOrientation
			   ? mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right
			   : mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom;
}

Float UISplitter::getUsableSplitSpace() const {
	Float totalSpace = getAxisContentSize();
	if ( mSplitter->isVisible() ) {
		totalSpace -= UIOrientation::Horizontal == mOrientation
						  ? mSplitter->getPixelsSize().getWidth()
						  : mSplitter->getPixelsSize().getHeight();
	}
	return totalSpace;
}

Float UISplitter::getAxisMinSize( const UIWidget* widget ) const {
	if ( !widget )
		return 0.f;
	return UIOrientation::Horizontal == mOrientation ? widget->getMinSizePx().getWidth()
													 : widget->getMinSizePx().getHeight();
}

bool UISplitter::isPartitionOnEdge( Float extent ) const {
	if ( mSplitPartition.getUnit() == StyleSheetLength::Percentage )
		return mSplitPartition.getValue() <= 0.f || mSplitPartition.getValue() >= 100.f;

	Float specified = convertLength( mSplitPartition, extent );
	if ( specified <= 0.f )
		return true;

	// An absolute partition that doesn't fit is only an edge when the pane it would squeeze accepts
	// that: a narrow splitter must still clamp to that pane's minimum and keep its divider.
	const UIWidget* opposite =
		mPartitionOrigin == PartitionOrigin::Start ? mLastWidget : mFirstWidget;
	return specified >= extent && getAxisMinSize( opposite ) <= 0.f;
}

Float UISplitter::getFirstPaneSize( Float totalSpace ) const {
	Float specified = convertLength( mSplitPartition, totalSpace );
	Float firstSplit =
		mPartitionOrigin == PartitionOrigin::Start ? specified : totalSpace - specified;

	// A partition on the edge collapses a pane on purpose, so that pane's minimum doesn't apply.
	if ( !isPartitionOnEdge( getAxisContentSize() ) )
		firstSplit = eemin( firstSplit, totalSpace - getAxisMinSize( mLastWidget ) );

	firstSplit = eemax( firstSplit, getAxisMinSize( mFirstWidget ) );

	return eeclamp( firstSplit, 0.f, eemax( totalSpace, 0.f ) );
}

StyleSheetLength UISplitter::partitionFromPixels( Float pixels, Float totalSpace ) const {
	switch ( mSplitPartition.getUnit() ) {
		case StyleSheetLength::Percentage:
			return StyleSheetLength(
				totalSpace > 0.f ? eeclamp<Float>( pixels / totalSpace * 100, 0, 100 ) : 0.f,
				StyleSheetLength::Percentage );
		case StyleSheetLength::Px:
			return StyleSheetLength( pixels, StyleSheetLength::Px );
		default:
			// Dp, and any unit that can't follow a drag: materialized as a density independent
			// size.
			return StyleSheetLength( PixelDensity::pxToDp( pixels ), StyleSheetLength::Dp );
	}
}

Uint32 UISplitter::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			if ( !mSplitter->isDragging() ) {
				tryUpdateLayout();
			}
			return 1;
		}
	}

	return 0;
}

}} // namespace EE::UI
