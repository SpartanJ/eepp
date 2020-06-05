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
	mSplitOnlyWhenNeeded( true ),
	mAlwaysShowSplitter( true ),
	mDivisionSplit( 0.5f ),
	mFirstWidget( NULL ),
	mLastWidget( NULL ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
	mSplitter = UIWidget::NewWithTag( "splitter::separator" );
	mSplitter->setDragEnabled( true );
	mSplitter->addEventListener( Event::OnDragStart, [&]( const Event* ) {
		mSplitter->pushState( UIState::StateSelected );
	} );
	mSplitter->addEventListener(
		Event::OnDragStop, [&]( const Event* ) { mSplitter->popState( UIState::StateSelected ); } );
	mSplitter->setParent( this );
	mSplitter->setMinWidth( 4 );
	mSplitter->setMinHeight( 4 );
	mSplitter->addEventListener( Event::OnSizeChange, [&]( const Event* ) { setLayoutDirty(); } );
	mSplitter->addEventListener( Event::MouseOver, [&]( const Event* ) {
		getUISceneNode()->setCursor( mOrientation == UIOrientation::Horizontal ? Cursor::SizeWE
																			   : Cursor::SizeNS );
	} );
	mSplitter->addEventListener(
		Event::MouseLeave, [&]( const Event* ) { getUISceneNode()->setCursor( Cursor::Arrow ); } );
	mSplitter->addEventListener( Event::OnPositionChange, [&]( const Event* ) {
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

const Float& UISplitter::getDivisionSplit() const {
	return mDivisionSplit;
}

void UISplitter::setDivisionSplit( const Float& divisionSplit ) {
	if ( eeclamp( divisionSplit, 0.f, 1.f ) != mDivisionSplit ) {
		mDivisionSplit = eeclamp( divisionSplit, 0.f, 1.f );
		setLayoutDirty();
	}
}

void UISplitter::swap() {
	if ( isFull() ) {
		UIWidget* tmp = mFirstWidget;
		mFirstWidget = mLastWidget;
		mLastWidget = tmp;
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
	Float totalSpace = mOrientation == UIOrientation::Horizontal
						   ? mSize.getWidth() - mRealPadding.Left - mRealPadding.Right
						   : mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom;
	if ( mSplitter->isVisible() ) {
		totalSpace -= UIOrientation::Horizontal == mOrientation
						  ? mSplitter->getPixelsSize().getWidth()
						  : mSplitter->getPixelsSize().getHeight();
	}

	if ( UIOrientation::Horizontal == mOrientation ) {
		Float fMinSize = mFirstWidget ? mFirstWidget->getCurrentMinSize().getWidth() : 0.f;

		if ( mSplitter->getPosition().x < mPadding.Left + fMinSize ) {
			mSplitter->setPosition( mPadding.Left + fMinSize, mSplitter->getPosition().y );
		}

		Float lMinSize = mLastWidget ? mLastWidget->getCurrentMinSize().getWidth() : 0.f;

		if ( mSplitter->getPosition().x + mSplitter->getSize().getWidth() >
			 mDpSize.getWidth() - mPadding.Right - lMinSize ) {
			mSplitter->setPosition( mDpSize.getWidth() - mPadding.Right - lMinSize -
										mSplitter->getSize().getWidth(),
									mSplitter->getPosition().y );
		}

		mSplitter->setPixelsSize( mSplitter->getPixelsSize().getWidth(),
								  mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
	} else {
		Float fMinSize = mFirstWidget ? mFirstWidget->getCurrentMinSize().getHeight() : 0.f;

		if ( mSplitter->getPosition().y < mPadding.Top + fMinSize ) {
			mSplitter->setPosition( mSplitter->getPosition().x, mPadding.Top + fMinSize );
		}

		Float lMinSize = mLastWidget ? mLastWidget->getCurrentMinSize().getHeight() : 0.f;

		if ( mSplitter->getPosition().y + mSplitter->getSize().getHeight() >
			 mDpSize.getHeight() - mPadding.Bottom - lMinSize ) {
			mSplitter->setPosition( mSplitter->getPosition().x,
									mDpSize.getHeight() - mPadding.Bottom - lMinSize -
										mSplitter->getSize().getHeight() );
		}

		mSplitter->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
								  mSplitter->getPixelsSize().getHeight() );
	}

	if ( mFirstWidget ) {
		mFirstWidget->setPixelsPosition( mRealPadding.Left, mRealPadding.Top );
		if ( UIOrientation::Horizontal == mOrientation ) {
			mFirstWidget->setPixelsSize( mSplitter->getPixelsPosition().x - mRealPadding.Left,
										 mSize.getHeight() - mRealPadding.Top -
											 mRealPadding.Bottom );
		} else {
			mFirstWidget->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
										 mSplitter->getPixelsPosition().y - mRealPadding.Top );
		}
	}

	if ( mLastWidget ) {
		if ( UIOrientation::Horizontal == mOrientation ) {
			mLastWidget->setPixelsPosition( mSplitter->getPixelsPosition().x +
												mSplitter->getPixelsSize().getWidth(),
											mRealPadding.Top );
			mLastWidget->setPixelsSize(
				mSize.getWidth() - mRealPadding.Right - mSplitter->getPixelsPosition().x -
					mSplitter->getPixelsSize().getWidth(),
				mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
		} else {
			mLastWidget->setPixelsPosition( mRealPadding.Left,
											mSplitter->getPixelsPosition().y +
												mSplitter->getPixelsSize().getHeight() );
			mLastWidget->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
										mSize.getHeight() - mRealPadding.Bottom -
											mSplitter->getPixelsPosition().y -
											mSplitter->getPixelsSize().getHeight() );
		}
	}

	if ( UIOrientation::Horizontal == mOrientation ) {
		mDivisionSplit =
			( mSplitter->getPixelsPosition().x + mSplitter->getPixelsSize().getWidth() ) /
			( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right );
	} else {
		mDivisionSplit =
			( mSplitter->getPixelsPosition().y + mSplitter->getPixelsSize().getHeight() ) /
			( mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
	}

	mDirtyLayout = false;
}

void UISplitter::updateLayout() {
	mDirtyLayout = true;

	if ( !getParent()->isLayout() && !getParent()->asType<UINode>()->ownsChildPosition() ) {
		bool sizeChanged = false;
		Sizef size( getSize() );

		if ( getLayoutWidthPolicy() == SizePolicy::MatchParent && 0 == getLayoutWeight() ) {
			Float w = getParent()->getSize().getWidth() - mLayoutMargin.Left - mLayoutMargin.Right;

			if ( getParent()->isType( UI_TYPE_LAYOUT ) ) {
				UILayout* pLay = static_cast<UILayout*>( getParent() );
				w = w - pLay->getPadding().Left - pLay->getPadding().Right;
			}

			if ( (int)w != (int)getSize().getWidth() ) {
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

			if ( (int)h != (int)getSize().getHeight() ) {
				sizeChanged = true;
				size.setHeight( h );
			}
		}

		if ( sizeChanged ) {
			setInternalSize( size );
		}
	}

	if ( mSplitOnlyWhenNeeded && !mLastWidget ) {
		mSplitter->setVisible( false )->setEnabled( false );

		if ( mFirstWidget ) {
			mFirstWidget->setPixelsPosition( mRealPadding.Left, mRealPadding.Top );
			mFirstWidget->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
										 mSize.getHeight() - mRealPadding.Top -
											 mRealPadding.Bottom );
		}

		mDirtyLayout = false;
		return;
	}

	mSplitter->setVisible( !mAlwaysShowSplitter && !mLastWidget ? false : true );
	mSplitter->setEnabled( mSplitter->isVisible() );
	Float totalSpace = mOrientation == UIOrientation::Horizontal
						   ? mSize.getWidth() - mRealPadding.Left - mRealPadding.Right
						   : mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom;
	if ( mSplitter->isVisible() ) {
		totalSpace -= UIOrientation::Horizontal == mOrientation
						  ? mSplitter->getPixelsSize().getWidth()
						  : mSplitter->getPixelsSize().getHeight();
	}

	Float firstSplit = ( totalSpace * mDivisionSplit );
	Float secondSplit = totalSpace - firstSplit;

	if ( mFirstWidget ) {
		mFirstWidget->setPixelsPosition( mRealPadding.Left, mRealPadding.Top );

		if ( UIOrientation::Horizontal == mOrientation ) {
			Float fMinSize = mFirstWidget ? mFirstWidget->getCurrentMinSize().getWidth() : 0.f;

			firstSplit = eemax( firstSplit, fMinSize );
			secondSplit = totalSpace - firstSplit;

			mFirstWidget->setPixelsSize( firstSplit, mSize.getHeight() - mRealPadding.Top -
														 mRealPadding.Bottom );

			mSplitter->setPixelsPosition( mFirstWidget->getPixelsPosition().x +
											  mFirstWidget->getPixelsSize().getWidth(),
										  mRealPadding.Top );
			mSplitter->setPixelsSize( mSplitter->getPixelsSize().getWidth(),
									  mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
		} else {

			Float fMinSize = mFirstWidget ? mFirstWidget->getCurrentMinSize().getHeight() : 0.f;

			firstSplit = eemax( firstSplit, fMinSize );
			secondSplit = totalSpace - firstSplit;

			mFirstWidget->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
										 firstSplit );

			mSplitter->setPixelsPosition( mRealPadding.Left,
										  mFirstWidget->getPixelsPosition().y +
											  mFirstWidget->getPixelsSize().getHeight() );
			mSplitter->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
									  mSplitter->getPixelsSize().getHeight() );
		}
	}

	if ( mLastWidget ) {
		if ( UIOrientation::Horizontal == mOrientation ) {
			mLastWidget->setPixelsPosition( mSplitter->getPixelsPosition().x +
												mSplitter->getPixelsSize().getWidth(),
											mRealPadding.Top );
			mLastWidget->setPixelsSize( secondSplit, mSize.getHeight() - mRealPadding.Top -
														 mRealPadding.Bottom );
		} else {
			mLastWidget->setPixelsPosition( mRealPadding.Left,
											mSplitter->getPixelsPosition().y +
												mSplitter->getPixelsSize().getHeight() );
			mLastWidget->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
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
