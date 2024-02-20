#include <eepp/scene/actions/move.hpp>
#include <eepp/ui/uiviewpager.hpp>

namespace EE { namespace UI {

UIViewPager* UIViewPager::New() {
	return eeNew( UIViewPager, () );
}

UIViewPager* UIViewPager::NewHorizontal() {
	return New();
}

UIViewPager* UIViewPager::NewVertical() {
	return New()->setOrientation( UIOrientation::Vertical );
}

UIViewPager::UIViewPager() :
	UIWidget( "viewpager" ),
	mOrientation( UIOrientation::Horizontal ),
	mDragging( false ),
	mLocked( false ),
	mDragResistance( PixelDensity::dpToPx( 8 ) ),
	mInitialDisplacement( 0 ),
	mDisplacement( 0 ),
	mChangePagePercent( 0.33f ),
	mMaxEdgeResistance( 0.0f ),
	mPageTransitionDuration( Seconds( 0.5f ) ),
	mCurrentPage( 0 ),
	mTotalPages( 0 ),
	mTimingFunction( Ease::Interpolation::SineIn ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;
	mContainer = UIWidget::New();
	mContainer->setParent( this );
	mContainer->setFlags( UI_OWNS_CHILDS_POSITION );
	setClipType( ClipType::ContentBox );
}

UIViewPager::~UIViewPager() {
	if ( NULL != getEventDispatcher() && mDragging )
		getEventDispatcher()->setNodeDragging( NULL );
}

Uint32 UIViewPager::getType() const {
	return UI_TYPE_VIEWPAGER;
}

bool UIViewPager::isType( const Uint32& type ) const {
	return UIViewPager::getType() == type ? true : UIWidget::isType( type );
}

void UIViewPager::onChildCountChange( Node* child, const bool& removed ) {
	if ( !removed && child != mContainer ) {
		child->setParent( mContainer );
	}

	if ( child != mContainer ) {
		mTotalPages = mContainer->getChildCount();
		updateChilds();
	}

	if ( !removed && child != mContainer ) {
		child->on( Event::OnPositionChange, [this]( const Event* ) { updateChilds(); } );
	}

	UIWidget::onChildCountChange( child, removed );
}

const UIOrientation& UIViewPager::getOrientation() const {
	return mOrientation;
}

UIViewPager* UIViewPager::setOrientation( const UIOrientation& orientation ) {
	mOrientation = orientation;
	return this;
}

Float UIViewPager::getDragResistance() const {
	return mDragResistance;
}

void UIViewPager::setDragResistance( const Float& dragResistance ) {
	mDragResistance = dragResistance;
}

const Float& UIViewPager::getChangePagePercent() const {
	return mChangePagePercent;
}

void UIViewPager::setChangePagePercent( const Float& changePagePercent ) {
	mChangePagePercent = eemin( 0.f, eemax( 1.f, changePagePercent ) );
}

const Time& UIViewPager::getPageTransitionDuration() const {
	return mPageTransitionDuration;
}

void UIViewPager::setPageTransitionDuration( const Time& pageChangeAnimationTime ) {
	mPageTransitionDuration = pageChangeAnimationTime;
}

const Float& UIViewPager::getMaxEdgeResistance() const {
	return mMaxEdgeResistance;
}

void UIViewPager::setMaxEdgeResistance( const Float& maxEdgeResistance ) {
	mMaxEdgeResistance = eemin( 0.f, eemax( 1.f, maxEdgeResistance ) );
}

const Ease::Interpolation& UIViewPager::getTimingFunction() const {
	return mTimingFunction;
}

void UIViewPager::setTimingFunction( const Ease::Interpolation& timingFunction ) {
	mTimingFunction = timingFunction;
}

void UIViewPager::onSizeChange() {
	updateChilds();
	UIWidget::onSizeChange();
}

Uint32 UIViewPager::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseDown: {
			if ( ( Msg->getFlags() & EE_BUTTON_LMASK ) ) {
				onMouseDownEvent();
			}
			break;
		}
		case NodeMessage::MouseMove: {
			if ( ( Msg->getFlags() & EE_BUTTON_LMASK ) ) {
				onMouseMoveEvent();
			}
			break;
		}
		default:
			break;
	}

	return UIWidget::onMessage( Msg );
}

void UIViewPager::updateChilds() {
	Float containerLength = mOrientation == UIOrientation::Horizontal
								? getSize().getWidth() * mTotalPages
								: getSize().getHeight() * mTotalPages;
	mContainer->setSize( mOrientation == UIOrientation::Horizontal
							 ? Sizef( containerLength, getSize().getHeight() )
							 : Sizef( getSize().getWidth(), containerLength ) );
	Node* childLoop = mContainer->getFirstChild();
	Float val = 0;
	while ( NULL != childLoop ) {
		if ( childLoop->isWidget() ) {
			childLoop->asType<UIWidget>()->setLayoutSizePolicy( SizePolicy::Fixed,
																SizePolicy::Fixed );
		}
		childLoop->setSize( getSize() );
		childLoop->setPosition( mOrientation == UIOrientation::Horizontal ? Vector2f( val, 0.f )
																		  : Vector2f( 0.f, val ) );
		val += mOrientation == UIOrientation::Horizontal ? getSize().getWidth()
														 : getSize().getHeight();
		childLoop = childLoop->getNextNode();
	}
}

void UIViewPager::limitDisplacement() {
	Float length = getLength();

	if ( mCurrentPage == 0 ) {
		if ( -mDisplacement > 0 && -mDisplacement / length > mMaxEdgeResistance ) {
			setDisplacement( length * -mMaxEdgeResistance );
		}
	}

	if ( mCurrentPage == mTotalPages - 1 ) {
		Float maxDisplacement = ( ( mTotalPages - 1 ) * length ) + length * mMaxEdgeResistance;

		if ( mDisplacement > maxDisplacement ) {
			setDisplacement( mDisplacement = maxDisplacement );
		}
	}
}

void UIViewPager::setDisplacement( const Float& val ) {
	mDisplacement = val;
	Float normalizedDisplacement = mDisplacement - mCurrentPage * getLength();

	if ( eeabs( normalizedDisplacement ) > mDragResistance ) {
		if ( mOrientation == UIOrientation::Horizontal ) {
			mContainer->setPixelsPosition(
				Vector2f( -mDisplacement, mContainer->getPixelsPosition().y ) );
		} else {
			mContainer->setPixelsPosition(
				Vector2f( mContainer->getPixelsPosition().x, -mDisplacement ) );
		}
	}
}

Float UIViewPager::getLength() const {
	return mOrientation == UIOrientation::Horizontal ? getPixelsSize().getWidth()
													 : getPixelsSize().getHeight();
}

void UIViewPager::moveToPage( const Int32& pageNum, bool animate ) {
	if ( pageNum >= 0 && pageNum < mTotalPages ) {
		mCurrentPage = pageNum;
		if ( animate ) {
			Float initPos = -mDisplacement;
			Float endPos = -getLength() * mCurrentPage;
			Float normalizedDisplacement = mDisplacement - mCurrentPage * getLength();
			Float normalizedProgress =
				getLength() != 0 ? eeabs( normalizedDisplacement ) / getLength() : 1;
			Action* action = Actions::MoveCoordinate::New(
				initPos, endPos, mPageTransitionDuration * normalizedProgress, mTimingFunction,
				mOrientation == UIOrientation::Horizontal ? Actions::MoveCoordinate::CoordinateX
														  : Actions::MoveCoordinate::CoordinateY,
				Actions::MoveCoordinate::CoordinateType::PixelPosition );
			action->on( Action::OnDone, [this]( Action*, const Action::ActionType& ) {
				sendCommonEvent( Event::OnPageChanged );
			} );
			mContainer->runAction( action );
		} else {
			Float val = -getLength() * mCurrentPage;
			mContainer->setPosition(
				mOrientation == UIOrientation::Horizontal ? Sizef( val, 0.f ) : Sizef( 0.f, val ) );
			sendCommonEvent( Event::OnPageChanged );
		}
	}
}

Uint32 UIViewPager::onCalculateDrag( const Vector2f&, const Uint32& flags ) {
	if ( !( flags & EE_BUTTON_LMASK ) ) {
		onMouseUpEvent();
	}
	return 1;
}

void UIViewPager::onMouseDownEvent() {
	if ( !mDragging && !mLocked && !getEventDispatcher()->isNodeDragging() ) {
		mDragging = true;
		mMouseDownPos = getEventDispatcher()->getMousePos().asFloat();
		mContainer->clearActions();
		mInitialDisplacement = mOrientation == UIOrientation::Horizontal
								   ? -mContainer->getPixelsPosition().x
								   : -mContainer->getPixelsPosition().y;
		mDisplacement = 0;
		getEventDispatcher()->setNodeDragging( this );
	}
}

void UIViewPager::onMouseMoveEvent() {
	if ( mDragging ) {
		mDisplacement = mInitialDisplacement +
						( mOrientation == UIOrientation::Horizontal
							  ? mMouseDownPos.x - getEventDispatcher()->getMousePos().asFloat().x
							  : mMouseDownPos.y - getEventDispatcher()->getMousePos().asFloat().y );

		limitDisplacement();

		setDisplacement( mDisplacement );
	}
}

void UIViewPager::onMouseUpEvent() {
	if ( mDragging ) {
		if ( eeabs( mDisplacement ) > mDragResistance ) {
			Float normalizedDisplacement = mDisplacement - mCurrentPage * getLength();
			Float length = getLength();

			if ( eeabs( normalizedDisplacement ) / length >= mChangePagePercent ) {
				if ( normalizedDisplacement < 0 ) {
					moveToPage( mCurrentPage - 1 );
				} else {
					moveToPage( mCurrentPage + 1 );
				}
			} else {
				moveToPage( mCurrentPage );
			}
		}

		mDragging = false;
		getEventDispatcher()->setNodeDragging( NULL );
	}
}

std::string UIViewPager::getPropertyString( const PropertyDefinition* propertyDef,
											const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Orientation:
			return getOrientation() == UIOrientation::Horizontal ? "horizontal" : "vertical";
		case PropertyId::DragResistance:
			return String::fromFloat( mDragResistance );
		case PropertyId::ChangePagePercent:
			return String::fromFloat( mChangePagePercent );
		case PropertyId::MaxEdgeResistance:
			return String::fromFloat( mMaxEdgeResistance );
		case PropertyId::PageTransitionDuration:
			return mPageTransitionDuration.toString();
		case PropertyId::TimingFunction:
			return Ease::toString( mTimingFunction );
		case PropertyId::PageLocked:
			return isLocked() ? "true" : "false";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIViewPager::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Orientation,
				   PropertyId::DragResistance,
				   PropertyId::ChangePagePercent,
				   PropertyId::MaxEdgeResistance,
				   PropertyId::PageTransitionDuration,
				   PropertyId::TimingFunction,
				   PropertyId::PageLocked };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIViewPager::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Orientation: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "horizontal" == val )
				setOrientation( UIOrientation::Horizontal );
			else if ( "vertical" == val )
				setOrientation( UIOrientation::Vertical );
			break;
		}
		case PropertyId::DragResistance:
			setDragResistance( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::ChangePagePercent:
			setChangePagePercent( attribute.asFloat() );
			break;
		case PropertyId::MaxEdgeResistance:
			setMaxEdgeResistance( attribute.asFloat() );
			break;
		case PropertyId::TimingFunction:
			setTimingFunction( Ease::fromName( attribute.getValue(), Ease::SineIn ) );
			break;
		case PropertyId::PageLocked:
			setLocked( attribute.asBool() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

const Int32& UIViewPager::getTotalPages() const {
	return mTotalPages;
}

const Int32& UIViewPager::getCurrentPage() const {
	return mCurrentPage;
}

void UIViewPager::setCurrentPage( const Int32& currentPage, bool animate ) {
	if ( !mLocked && mCurrentPage != currentPage ) {
		moveToPage( currentPage, animate );
	}
}

const bool& UIViewPager::isLocked() const {
	return mLocked;
}

void UIViewPager::setLocked( bool locked ) {
	if ( locked != mLocked ) {
		mLocked = locked;
	}
}

}} // namespace EE::UI
