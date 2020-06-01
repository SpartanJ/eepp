#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>

namespace EE { namespace UI {

class UISplitterSeparator : public UIWidget {

  public:
	static UISplitterSeparator* NewWithTag( const std::string& tag, UISplitter* parent ) {
		return eeNew( UISplitterSeparator, ( tag, parent ) );
	}

	UISplitterSeparator( const std::string& tagname, UISplitter* parent ) :
		UIWidget( tagname ), mContainer( parent ) {
		setDragEnabled( true );
	}

  protected:
	UISplitter* mContainer;

	Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags ) {
		if ( isDragging() && !( flags /*press trigger*/ & EE_BUTTON_LMASK ) ) {
			setDragging( false );
			getUISceneNode()->getEventDispatcher()->setNodeDragging( NULL );
			return 1;
		}
		Vector2f pos( eefloor( position.x ), eefloor( position.y ) );
		if ( mDragPoint != pos && ( std::abs( mDragPoint.x - pos.x ) > 1.f ||
									std::abs( mDragPoint.y - pos.y ) > 1.f ) ) {
			if ( onDrag( pos, flags ) ) {
				Sizef dragDiff;
				dragDiff.x = ( Float )( mDragPoint.x - pos.x );
				dragDiff.y = ( Float )( mDragPoint.y - pos.y );
				if ( mContainer->getOrientation() == UIOrientation::Horizontal ) {
					setPixelsPosition( mPosition.x - dragDiff.x, mPosition.y );
				} else {
					setPixelsPosition( mPosition.x, mPosition.y - dragDiff.y );
				}
				mDragPoint = pos;
				onPositionChange();
				getUISceneNode()->getEventDispatcher()->setNodeDragging( this );
			}
		}
		return 1;
	}

	Uint32 onDragStart( const Vector2i& pos, const Uint32& flags ) {
		pushState( UIState::StateSelected );
		return UIWidget::onDragStart( pos, flags );
	}

	Uint32 onDragStop( const Vector2i& pos, const Uint32& flags ) {
		popState( UIState::StateSelected );
		return UIWidget::onDragStop( pos, flags );
	}
};

UISplitter* UISplitter::New() {
	return eeNew( UISplitter, () );
}

UISplitter::UISplitter() :
	UILayout( "splitter" ),
	mOrientation( UIOrientation::Horizontal ),
	mSplitOnlyWhenNeeded( false ),
	mAlwaysShowSplitter( true ),
	mDivisionSplit( 1.f ),
	mFirstWidget( NULL ),
	mLastWidget( NULL ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
	mSplitter = UISplitterSeparator::NewWithTag( "splitter::separator", this );
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

void UISplitter::onChildCountChange( Node* child, const bool& removed ) {
	UILayout::onChildCountChange( child, removed );

	if ( child != mSplitter && !removed ) {
		if ( !child->isWidget() ) {
			child->close();
			return;
		}
		UIWidget* childWidget = child->asType<UIWidget>();

		if ( getChildCount() > 3 ) {
			child->close();
		} else {
			if ( NULL == mFirstWidget ) {
				mFirstWidget = childWidget;
			} else {
				mLastWidget = childWidget;
			}
			mSplitter->toFront();
		}
	}
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
		Float firstWidgetMinSize =
			mFirstWidget ? mFirstWidget->getCurrentMinSize().getWidth() : 0.f;
		if ( mSplitter->getPosition().x < mPadding.Left + firstWidgetMinSize ) {
			mSplitter->setPosition( mPadding.Left + firstWidgetMinSize,
									mSplitter->getPosition().y );
		}

		Float lastWidgetMinSize = mLastWidget ? mLastWidget->getCurrentMinSize().getWidth() : 0.f;
		if ( mSplitter->getPosition().x + mSplitter->getSize().getWidth() >
			 mDpSize.getWidth() - mPadding.Right - lastWidgetMinSize ) {
			mSplitter->setPosition( mDpSize.getWidth() - mPadding.Right - lastWidgetMinSize -
										mSplitter->getSize().getWidth(),
									mSplitter->getPosition().y );
		}

		mSplitter->setPixelsSize( mSplitter->getPixelsSize().getWidth(),
								  mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
	} else {
		Float firstWidgetMinSize =
			mFirstWidget ? mFirstWidget->getCurrentMinSize().getHeight() : 0.f;
		if ( mSplitter->getPosition().y < mPadding.Top + firstWidgetMinSize ) {
			mSplitter->setPosition( mSplitter->getPosition().x, mPadding.Top + firstWidgetMinSize );
		}

		Float lastWidgetMinSize = mLastWidget ? mLastWidget->getCurrentMinSize().getHeight() : 0.f;
		if ( mSplitter->getPosition().y >
			 mDpSize.getHeight() - mPadding.Bottom - lastWidgetMinSize ) {
			mSplitter->setPosition( mSplitter->getPosition().x,
									mDpSize.getHeight() - mPadding.Bottom - lastWidgetMinSize -
										+mSplitter->getSize().getHeight() );
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
	if ( mSplitOnlyWhenNeeded && !mLastWidget ) {
		mSplitter->setVisible( false )->setEnabled( false );

		if ( mFirstWidget ) {
			mFirstWidget->setPosition( mRealPadding.Left, mRealPadding.Top );
			mFirstWidget->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
										 mSize.getHeight() - mRealPadding.Top -
											 mRealPadding.Bottom );
		}
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
			mFirstWidget->setPixelsSize( firstSplit, mSize.getHeight() - mRealPadding.Top -
														 mRealPadding.Bottom );
		} else {
			mFirstWidget->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
										 firstSplit );
		}

		if ( UIOrientation::Horizontal == mOrientation ) {
			mSplitter->setPixelsPosition( mFirstWidget->getPixelsPosition().x +
											  mFirstWidget->getPixelsSize().getWidth(),
										  mRealPadding.Top );
			mSplitter->setPixelsSize( mSplitter->getPixelsSize().getWidth(),
									  mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
		} else {
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

Uint32 UISplitter::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			tryUpdateLayout();
			return 1;
		}
	}

	return 0;
}

}} // namespace EE::UI
