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
	mSplitPartition( StyleSheetLength( "50%" ) ),
	mFirstWidget( NULL ),
	mLastWidget( NULL ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
	mSplitter = UIWidget::NewWithTag( "splitter::separator" );
	mSplitter->setDragEnabled( true );
	mSplitter->on( Event::OnDragStart,
				   [this]( const Event* ) { mSplitter->pushState( UIState::StateSelected ); } );
	mSplitter->on( Event::OnDragStop,
				   [this]( const Event* ) { mSplitter->popState( UIState::StateSelected ); } );
	mSplitter->setParent( this );
	mSplitter->setMinWidth( 4 );
	mSplitter->setMinHeight( 4 );
	mSplitter->on( Event::OnSizeChange, [this]( const Event* ) { setLayoutDirty(); } );
	mSplitter->on( Event::MouseEnter, [this]( const Event* ) {
		getUISceneNode()->setCursor( mOrientation == UIOrientation::Horizontal ? Cursor::SizeWE
																			   : Cursor::SizeNS );
	} );
	mSplitter->on( Event::MouseLeave,
				   [this]( const Event* ) { getUISceneNode()->setCursor( Cursor::Arrow ); } );
	mSplitter->on( Event::OnPositionChange, [this]( const Event* ) {
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
		setLayoutDirty();
	}
}

const bool& UISplitter::alwaysShowSplitter() const {
	return mAlwaysShowSplitter;
}

void UISplitter::setAlwaysShowSplitter( bool alwaysShowSplitter ) {
	if ( alwaysShowSplitter != mAlwaysShowSplitter ) {
		mAlwaysShowSplitter = alwaysShowSplitter;
		setLayoutDirty();
	}
}

const StyleSheetLength& UISplitter::getSplitPartition() const {
	return mSplitPartition;
}

void UISplitter::setSplitPartition( const StyleSheetLength& divisionSplit ) {
	if ( divisionSplit != mSplitPartition ) {
		mSplitPartition = divisionSplit;
		setLayoutDirty();
	}
}

void UISplitter::swap( bool swapSplitPartition ) {
	if ( isFull() ) {
		UIWidget* tmp = mFirstWidget;
		mFirstWidget = mLastWidget;
		mLastWidget = tmp;
		if ( swapSplitPartition )
			mSplitPartition.setValue( 100.f - mSplitPartition.getValue(),
									  StyleSheetLength::Percentage );
		setLayoutDirty();
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
		case PropertyId::SplitterAlwaysShow:
			setAlwaysShowSplitter( attribute.asBool() );
		case PropertyId::Orientation: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "horizontal" == val )
				setOrientation( UIOrientation::Horizontal );
			else if ( "vertical" == val )
				setOrientation( UIOrientation::Vertical );
			break;
		}
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
		case PropertyId::Orientation:
			return getOrientation() == UIOrientation::Horizontal ? "horizontal" : "vertical";
		default:
			return UILayout::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UISplitter::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::SplitterPartition, PropertyId::SplitterAlwaysShow,
				   PropertyId::Orientation };
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
	mSplitter->setVisible( !mAlwaysShowSplitter && !mLastWidget ? false : true );
	mSplitter->setEnabled( mSplitter->isVisible() );

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

	if ( UIOrientation::Horizontal == mOrientation ) {
		mSplitPartition = StyleSheetLength(
			eeclamp<Float>( mSplitter->getPixelsPosition().x /
								( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right -
								  mSplitter->getPixelsSize().getWidth() ) *
								100,
							0, 100 ),
			StyleSheetLength::Percentage );
	} else {
		mSplitPartition = StyleSheetLength(
			eeclamp<Float>( mSplitter->getPixelsPosition().y /
								( mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom -
								  mSplitter->getPixelsSize().getHeight() ) *
								100,
							0, 100 ),
			StyleSheetLength::Percentage );
	}

	mDirtyLayout = false;
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

	mSplitter->setVisible( !mAlwaysShowSplitter && !mLastWidget ? false : true );
	mSplitter->setEnabled( mSplitter->isVisible() );
	Float totalSpace = mOrientation == UIOrientation::Horizontal
						   ? mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right
						   : mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom;
	if ( mSplitter->isVisible() ) {
		totalSpace -= UIOrientation::Horizontal == mOrientation
						  ? mSplitter->getPixelsSize().getWidth()
						  : mSplitter->getPixelsSize().getHeight();
	}

	Float firstSplit = convertLength( mSplitPartition, totalSpace );
	Float secondSplit = totalSpace - firstSplit;

	if ( mFirstWidget ) {
		mFirstWidget->setPixelsPosition( mPaddingPx.Left, mPaddingPx.Top );

		if ( UIOrientation::Horizontal == mOrientation ) {
			Float fMinSize = mFirstWidget ? mFirstWidget->getMinSize().getWidth() : 0.f;

			firstSplit = eemax( firstSplit, fMinSize );
			secondSplit = totalSpace - firstSplit;

			mFirstWidget->setPixelsSize( firstSplit,
										 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );

			mSplitter->setPixelsPosition( mFirstWidget->getPixelsPosition().x +
											  mFirstWidget->getPixelsSize().getWidth(),
										  mPaddingPx.Top );
			mSplitter->setPixelsSize( mSplitter->getPixelsSize().getWidth(),
									  mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		} else {

			Float fMinSize = mFirstWidget ? mFirstWidget->getMinSize().getHeight() : 0.f;

			firstSplit = eemax( firstSplit, fMinSize );
			secondSplit = totalSpace - firstSplit;

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
			mLastWidget->setPixelsPosition( mSplitter->getPixelsPosition().x +
												mSplitter->getPixelsSize().getWidth(),
											mPaddingPx.Top );
			mLastWidget->setPixelsSize( secondSplit,
										mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		} else {
			mLastWidget->setPixelsPosition( mPaddingPx.Left,
											mSplitter->getPixelsPosition().y +
												mSplitter->getPixelsSize().getHeight() );
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
