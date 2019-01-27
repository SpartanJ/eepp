#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <algorithm>

namespace EE { namespace UI {

UIWidget * UIWidget::New() {
	return eeNew( UIWidget, () );
}

UIWidget * UIWidget::NewWithTag( const std::string& tag ) {
	return eeNew( UIWidget, ( tag ) );
}

UIWidget::UIWidget( const std::string & tag ) :
	UINode(),
	mTag( tag ),
	mTheme( NULL ),
	mStyle( UIStyle::New( this ) ),
	mTooltip( NULL ),
	mMinControlSize(),
	mLayoutWeight(0),
	mLayoutGravity(0),
	mLayoutWidthRules(WRAP_CONTENT),
	mLayoutHeightRules(WRAP_CONTENT),
	mLayoutPositionRule(NONE),
	mLayoutPositionRuleWidget(NULL),
	mAttributesTransactionCount(0)
{
	mNodeFlags |= NODE_FLAG_WIDGET;

	updateAnchorsDistances();
}

UIWidget::UIWidget() :
	UIWidget( "widget" )
{
}

UIWidget::~UIWidget() {
	eeSAFE_DELETE( mStyle );
	eeSAFE_DELETE( mTooltip );
}

Uint32 UIWidget::getType() const {
	return UI_TYPE_WIDGET;
}

bool UIWidget::isType( const Uint32& type ) const {
	return UIWidget::getType() == type ? true : UINode::isType( type );
}

void UIWidget::updateAnchorsDistances() {
	if ( NULL != mParentCtrl ) {
		mDistToBorder	= Rect( mPosition.x, mPosition.y, mParentCtrl->getPixelsSize().x - ( mPosition.x + mSize.x ), mParentCtrl->getPixelsSize().y - ( mPosition.y + mSize.y ) );
	}
}

Rect UIWidget::getLayoutMargin() const {
	return mLayoutMargin;
}

UIWidget * UIWidget::setLayoutMargin(const Rect & margin) {
	if ( mLayoutMargin != margin ) {
		mLayoutMargin = margin;
		notifyLayoutAttrChange();
	}

	return this;
}

Float UIWidget::getLayoutWeight() const {
	return mLayoutWeight;
}

UIWidget * UIWidget::setLayoutWeight(const Float & weight) {
	if ( mLayoutWeight != weight ) {
		mLayoutWeight = weight;
		notifyLayoutAttrChange();
	}

	return this;
}

Uint32 UIWidget::getLayoutGravity() const {
	return mLayoutGravity;
}

UIWidget * UIWidget::setLayoutGravity(const Uint32 & layoutGravity) {
	if ( mLayoutGravity != layoutGravity ) {
		mLayoutGravity = layoutGravity;
		notifyLayoutAttrChange();
	}

	return this;
}

LayoutSizeRules UIWidget::getLayoutWidthRules() const {
	return mLayoutWidthRules;
}

UIWidget * UIWidget::setLayoutWidthRules(const LayoutSizeRules & layoutWidthRules) {
	if ( mLayoutWidthRules != layoutWidthRules ) {
		mLayoutWidthRules = layoutWidthRules;
		notifyLayoutAttrChange();
	}

	return this;
}

LayoutSizeRules UIWidget::getLayoutHeightRules() const {
	return mLayoutHeightRules;
}

UIWidget * UIWidget::setLayoutHeightRules(const LayoutSizeRules & layoutHeightRules) {
	if ( mLayoutHeightRules != layoutHeightRules ) {
		mLayoutHeightRules = layoutHeightRules;
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::setLayoutSizeRules(const LayoutSizeRules & layoutWidthRules, const LayoutSizeRules & layoutHeightRules) {
	if ( mLayoutWidthRules != layoutWidthRules || mLayoutHeightRules != layoutHeightRules ) {
		mLayoutWidthRules = layoutWidthRules;
		mLayoutHeightRules = layoutHeightRules;
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::setLayoutPositionRule(const LayoutPositionRules & layoutPositionRule, UIWidget * of) {

	if ( mLayoutPositionRule != layoutPositionRule || mLayoutPositionRuleWidget != of ) {
		mLayoutPositionRule = layoutPositionRule;
		mLayoutPositionRuleWidget = of;
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::getLayoutPositionRuleWidget() const {
	return mLayoutPositionRuleWidget;
}

LayoutPositionRules UIWidget::getLayoutPositionRule() const {
	return mLayoutPositionRule;
}

void UIWidget::createTooltip() {
	if ( NULL != mTooltip )
		return;

	mTooltip = UITooltip::New();
	mTooltip->setVisible( false )->setEnabled( false );
	mTooltip->setTooltipOf( this );
}

Uint32 UIWidget::onMouseMove( const Vector2i & Pos, const Uint32& Flags ) {
	if ( mVisible && NULL != mTooltip && !mTooltip->getText().empty() ) {
		EventDispatcher * eventDispatcher = getEventDispatcher();

		if ( NULL == eventDispatcher )
			return 1;

		UIThemeManager * themeManager = UIThemeManager::instance();

		Vector2f Pos = eventDispatcher->getMousePosf();
		Pos.x += themeManager->getCursorSize().x;
		Pos.y += themeManager->getCursorSize().y;

		if ( Pos.x + mTooltip->getPixelsSize().getWidth() > eventDispatcher->getSceneNode()->getPixelsSize().getWidth() ) {
			Pos.x = eventDispatcher->getMousePos().x - mTooltip->getPixelsSize().getWidth();
		}

		if ( Pos.y + mTooltip->getPixelsSize().getHeight() > eventDispatcher->getSceneNode()->getPixelsSize().getHeight() ) {
			Pos.y = eventDispatcher->getMousePos().y - mTooltip->getPixelsSize().getHeight();
		}

		if ( Time::Zero == themeManager->getTooltipTimeToShow() ) {
			if ( !mTooltip->isVisible() || themeManager->getTooltipFollowMouse() )
				mTooltip->setPosition( PixelDensity::pxToDp( Pos ) );

			mTooltip->show();
		} else {
			if ( -1.f != mTooltip->getTooltipTime().asMilliseconds() ) {
				mTooltip->addTooltipTime( mSceneNode->getElapsed() );
			}

			if ( mTooltip->getTooltipTime() >= themeManager->getTooltipTimeToShow() ) {
				if ( mTooltip->getTooltipTime().asMilliseconds() != -1.f ) {
					mTooltip->setPixelsPosition( Pos );

					mTooltip->show();

					mTooltip->setTooltipTime( Milliseconds( -1.f ) );
				}
			}
		}

		if ( themeManager->getTooltipFollowMouse() ) {
			mTooltip->setPixelsPosition( Pos );
		}
	}

	return UINode::onMouseMove( Pos, Flags );
}

Uint32 UIWidget::onMouseLeave( const Vector2i & Pos, const Uint32& Flags ) {
	if ( mVisible && NULL != mTooltip && !mTooltip->getText().empty() ) {
		mTooltip->setTooltipTime( Milliseconds( 0.f ) );

		if ( mTooltip->isVisible() )
			mTooltip->hide();
	}

	return UINode::onMouseLeave( Pos, Flags );
}

UIWidget * UIWidget::setTooltipText( const String& Text ) {
	if ( NULL == mTooltip ) {	// If the tooltip wasn't created it will avoid to create a new one if the string is ""
		if ( Text.size() ) {
			createTooltip();

			mTooltip->setText( Text );
		}
	} else { // but if it's created, i will allow it
		mTooltip->setText( Text );
	}

	return this;
}

String UIWidget::getTooltipText() {
	if ( NULL != mTooltip )
		return mTooltip->getText();

	return String();
}

void UIWidget::tooltipRemove() {
	mTooltip = NULL;
}

Node * UIWidget::setSize( const Sizef& size ) {
	Sizef s( size );

	if ( s.x < mMinControlSize.x )
		s.x = mMinControlSize.x;

	if ( s.y < mMinControlSize.y )
		s.y = mMinControlSize.y;

	return UINode::setSize( s );
}

UINode * UIWidget::setFlags(const Uint32 & flags) {
	if ( flags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) {
		updateAnchorsDistances();
	}

	if ( !( mFlags & UI_AUTO_SIZE ) && ( flags & UI_AUTO_SIZE ) ) {
		onAutoSize();
	}

	return UINode::setFlags( flags );
}

UINode * UIWidget::unsetFlags(const Uint32 & flags) {
	if ( flags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) {
		updateAnchorsDistances();
	}

	return UINode::unsetFlags( flags );
}

UIWidget * UIWidget::setAnchors(const Uint32 & flags) {
	mFlags &= ~(UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM);
	mFlags |= flags;
	updateAnchorsDistances();
	return this;
}

void UIWidget::setTheme( UITheme * Theme ) {
	mTheme = Theme;
	invalidateDraw();
}

UINode * UIWidget::setThemeSkin( const std::string& skinName ) {
	return setThemeSkin( NULL != mTheme ? mTheme : UIThemeManager::instance()->getDefaultTheme(), skinName );
}

UINode * UIWidget::setThemeSkin( UITheme * Theme, const std::string& skinName ) {
	return UINode::setThemeSkin( Theme, skinName );
}

Node * UIWidget::setSize( const Float& Width, const Float& Height ) {
	return UINode::setSize( Width, Height );
}

Node * UIWidget::setId( const std::string& id ) {
	Node::setId( id );

	reloadStyle( false );

	return this;
}

const Sizef& UIWidget::getSize() const {
	return UINode::getSize();
}

UITooltip * UIWidget::getTooltip() {
	return mTooltip;
}

void UIWidget::onParentSizeChange( const Vector2f& SizeChange ) {
	updateAnchors( SizeChange );
	UINode::onParentSizeChange( SizeChange );
}

void UIWidget::onPositionChange() {
	updateAnchorsDistances();
	UINode::onPositionChange();
}

void UIWidget::onVisibilityChange() {
	updateAnchorsDistances();
	notifyLayoutAttrChange();
	UINode::onVisibilityChange();
}

void UIWidget::onSizeChange() {
	UINode::onSizeChange();
	notifyLayoutAttrChange();
}

void UIWidget::onAutoSize() {
}

void UIWidget::onWidgetCreated() {
}

void UIWidget::notifyLayoutAttrChange() {
	if ( 0 == mAttributesTransactionCount ) {
		NodeMessage msg( this, NodeMessage::LayoutAttributeChange );
		messagePost( &msg );
	}
}

void UIWidget::notifyLayoutAttrChangeParent() {
	if ( 0 == mAttributesTransactionCount && NULL != mParentCtrl ) {
		NodeMessage msg( this, NodeMessage::LayoutAttributeChange );
		mParentCtrl->messagePost( &msg );
	}
}

void UIWidget::updateAnchors( const Vector2f& SizeChange ) {
	if ( !( mFlags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) )
		return;

	Sizef newSize( mDpSize );

	if ( !( mFlags & UI_ANCHOR_LEFT ) ) {
		setInternalPosition( Vector2f( mDpPos.x += SizeChange.x, mDpPos.y ) );
	}

	if ( mFlags & UI_ANCHOR_RIGHT ) {
		if ( NULL != mParentCtrl ) {
			newSize.x = mParentCtrl->getSize().getWidth() - mDpPos.x - PixelDensity::pxToDpI( mDistToBorder.Right );

			if ( newSize.x < mMinControlSize.getWidth() )
				newSize.x = mMinControlSize.getWidth();
		}
	}

	if ( !( mFlags & UI_ANCHOR_TOP ) ) {
		setInternalPosition( Vector2f( mDpPos.x, mDpPos.y += SizeChange.y ) );
	}

	if ( mFlags & UI_ANCHOR_BOTTOM ) {
		if ( NULL != mParentCtrl ) {
			newSize.y = mParentCtrl->getSize().y - mDpPos.y - PixelDensity::pxToDpI( mDistToBorder.Bottom );

			if ( newSize.y < mMinControlSize.getHeight() )
				newSize.y = mMinControlSize.getHeight();
		}
	}

	if ( newSize != mDpSize )
		setSize( newSize );
}

void UIWidget::alignAgainstLayout() {
	Vector2f pos = mDpPos;

	switch ( fontHAlignGet( mLayoutGravity ) ) {
		case UI_HALIGN_CENTER:
			pos.x = ( getParent()->getSize().getWidth() - mDpSize.getWidth() ) / 2;
			break;
		case UI_HALIGN_RIGHT:
			pos.x = getParent()->getSize().getWidth() - mLayoutMargin.Right;
			break;
		case UI_HALIGN_LEFT:
			pos.x = mLayoutMargin.Left;
			break;
	}

	switch ( fontVAlignGet( mLayoutGravity ) ) {
		case UI_VALIGN_CENTER:
			pos.y = ( getParent()->getSize().getHeight() - mDpSize.getHeight() ) / 2;
			break;
		case UI_VALIGN_BOTTOM:
			pos.y = getParent()->getSize().getHeight() - mLayoutMargin.Bottom;
			break;
		case UI_VALIGN_TOP:
			pos.y = mLayoutMargin.Top;
			break;
	}

	setInternalPosition( pos );
}

void UIWidget::reportStyleStateChange() {
	if ( NULL != mStyle )
		mStyle->onStateChange();
}

bool UIWidget::isSceneNodeLoading() const {
	return getSceneNode()->isUISceneNode() ? static_cast<UISceneNode*>( getSceneNode() )->isLoading() : false;
}

const Rectf& UIWidget::getPadding() const {
	return mPadding;
}

UIWidget * UIWidget::setPadding(const Rectf& padding) {
	if ( padding != mPadding ) {
		mPadding = padding;
		mRealPadding = PixelDensity::dpToPx( mPadding );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

const std::string &UIWidget::getStyleSheetId() const {
	return mId;
}

const std::string& UIWidget::getStyleSheetTag() const {
	return mTag;
}

const std::vector<std::string> &UIWidget::getStyleSheetClasses() const {
	return mClasses;
}

CSS::StyleSheetElement * UIWidget::getStyleSheetParentElement() const {
	return NULL != mParentCtrl && mParentCtrl->isWidget() ? dynamic_cast<CSS::StyleSheetElement*>( mParentCtrl ) : NULL;
}

CSS::StyleSheetElement * UIWidget::getStyleSheetPreviousSiblingElement() const {
	return NULL != mPrev && mPrev->isWidget() ? dynamic_cast<CSS::StyleSheetElement*>( mPrev ) : NULL;
}

CSS::StyleSheetElement * UIWidget::getStyleSheetNextSiblingElement() const {
	return NULL != mNext && mNext->isWidget() ? dynamic_cast<CSS::StyleSheetElement*>( mNext ) : NULL;
}

const std::vector<std::string> &UIWidget::getStyleSheetPseudoClasses() const {
	return mPseudoClasses;
}

void UIWidget::updatePseudoClasses() {
	mPseudoClasses.clear();

	if ( mState & UIState::StateFlagHover )
		mPseudoClasses.push_back( "hover" );

	if ( mState & UIState::StateFlagFocus )
		mPseudoClasses.push_back( "focus" );

	if ( mState & UIState::StateFlagSelected )
		mPseudoClasses.push_back( "selected" );

	if ( mState & UIState::StateFlagPressed )
		mPseudoClasses.push_back( "pressed" );

	if ( mState & UIState::StateFlagDisabled )
		mPseudoClasses.push_back( "disabled" );

	invalidateDraw();
}

void UIWidget::addClass( const std::string& cls ) {
	if ( !cls.empty() && !containsClass( cls ) ) {
		mClasses.push_back( cls );

		reloadStyle( false );
	}
}

void UIWidget::addClasses( const std::vector<std::string>& classes ) {
	if ( !classes.empty() ) {
		for ( auto cit = classes.begin(); cit != classes.end(); ++cit ) {
			const std::string& cls = *cit;

			if ( !cls.empty() && !containsClass( cls ) ) {
				mClasses.push_back( cls );
			}
		}

		reloadStyle( false );
	}
}

void UIWidget::removeClass( const std::string& cls ) {
	if ( containsClass( cls ) ) {
		mClasses.erase( std::find( mClasses.begin(), mClasses.end(), cls ) );

		reloadStyle( false );
	}
}

bool UIWidget::containsClass( const std::string& cls ) {
	return std::find( mClasses.begin(), mClasses.end(), cls ) != mClasses.end();
}

void UIWidget::setElementTag( const std::string& tag ) {
	if ( mTag != tag ) {
		mTag = tag;

		reloadStyle( false );
	}
}

const std::string& UIWidget::getElementTag() const {
	return mTag;
}

void UIWidget::pushState( const Uint32& State, bool emitEvent ) {
	if ( !( mState & ( 1 << State ) ) ) {
		if ( NULL != mStyle ) {
			updatePseudoClasses();
			mStyle->pushState( State );
		}

		UINode::pushState( State, emitEvent );
	}
}

void UIWidget::popState( const Uint32& State, bool emitEvent ) {
	if ( mState & ( 1 << State ) ) {
		if ( NULL != mStyle ) {
			updatePseudoClasses();
			mStyle->popState( State );
		}

		UINode::popState( State, emitEvent );
	}
}



UIStyle * UIWidget::getUIStyle() const {
	return mStyle;
}

void UIWidget::reloadStyle( const bool& reloadChilds ) {
	if ( NULL == mStyle && getSceneNode()->isUISceneNode() && static_cast<UISceneNode*>( getSceneNode() )->hasStyleSheet() ) {
		mStyle = UIStyle::New( this );
		mStyle->setState( mState );
	}

	if ( NULL != mStyle ) {
		mStyle->load();
		reportStyleStateChange();

		if ( NULL != mChild && reloadChilds ) {
			Node * ChildLoop = mChild;

			while ( NULL != ChildLoop ) {
				if ( ChildLoop->isWidget() )
					static_cast<UIWidget*>( ChildLoop )->reloadStyle();

				ChildLoop = ChildLoop->getNextNode();
			}
		}
	}
}

void UIWidget::onPaddingChange() {
	invalidateDraw();
}

void UIWidget::onThemeLoaded() {
	reportStyleStateChange();
}

void UIWidget::beginAttributesTransaction() {
	mAttributesTransactionCount++;
}

void UIWidget::endAttributesTransaction() {
	mAttributesTransactionCount--;

	if ( 0 == mAttributesTransactionCount ) {
		notifyLayoutAttrChange();
	}
}

const Uint32& UIWidget::getStyleState() const {
	return NULL != mStyle ? mStyle->getCurrentState() : mState;
}

const Uint32& UIWidget::getStylePreviousState() const {
	return NULL != mStyle ? mStyle->getPreviousState() : mState;
}

bool UIWidget::setAttribute( const std::string& name, const std::string& value, const Uint32& state ) {
	return setAttribute( NodeAttribute( name, value ), state );
}

#define SAVE_NORMAL_STATE_ATTR( ATTR_FORMATED ) \
	CSS::StyleSheetProperty oldAttribute = mStyle->getStyleSheetProperty( UIState::StateFlagNormal, attribute.getName() ); \
	if ( oldAttribute.isEmpty() && mStyle->getPreviousState() == UIState::StateFlagNormal ) \
		mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), ATTR_FORMATED ) ); \

bool UIWidget::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	bool attributeSet = true;

	if ( "id" == name ) {
		setId( attribute.value() );
	} else if ( "class" == name ) {
		addClasses( String::split( attribute.getValue(), ' ' ) );
	} else if ( "x" == name ) {
		setLayoutWidthRules( FIXED );

		Float newX = attribute.asDpDimensionI();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );

			SAVE_NORMAL_STATE_ATTR( String::format( "%.2f", mDpSize.getWidth() ) );

			Action * action = Actions::MoveCoordinate::New( getPosition().x, newX, transitionInfo.duration, transitionInfo.timingFunction, Actions::MoveCoordinate::CoordinateX );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setInternalPosition( Vector2f( newX, mDpPos.y ) );
			notifyLayoutAttrChange();
		}
	} else if ( "y" == name ) {
		setLayoutWidthRules( FIXED );

		Float newY = attribute.asDpDimensionI();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );

			SAVE_NORMAL_STATE_ATTR( String::format( "%.2f", mDpSize.getWidth() ) );

			Action * action = Actions::MoveCoordinate::New( getPosition().y, newY, transitionInfo.duration, transitionInfo.timingFunction, Actions::MoveCoordinate::CoordinateY );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setInternalPosition( Vector2f( mDpPos.x, newY ) );
			notifyLayoutAttrChange();
		}
	} else if ( "width" == name ) {
		setLayoutWidthRules( FIXED );

		Float newWidth = attribute.asDpDimensionI();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );

			SAVE_NORMAL_STATE_ATTR( String::format( "%.2f", mDpSize.getWidth() ) );

			Action * action = Actions::ResizeWidth::New( getSize().getWidth(), newWidth, transitionInfo.duration, transitionInfo.timingFunction );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setInternalWidth( newWidth );
			notifyLayoutAttrChange();
		}
	} else if ( "height" == name ) {
		setLayoutHeightRules( FIXED );

		Float newHeight = attribute.asDpDimensionI();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );

			SAVE_NORMAL_STATE_ATTR( String::format( "%.2f", mDpSize.getHeight() ) );

			Action * action = Actions::ResizeHeight::New( getSize().getHeight(), newHeight, transitionInfo.duration, transitionInfo.timingFunction );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setInternalHeight( newHeight );
			notifyLayoutAttrChange();
		}
	} else if ( "background" == name ) {
		Drawable * res = NULL;

		if ( String::startsWith( attribute.getValue(), "#" ) ) {
			setAttribute( NodeAttribute( "backgroundcolor", attribute.getValue() ) );
		} else if ( NULL != ( res = DrawableSearcher::searchByName( attribute.getValue() ) ) ) {
			setBackgroundDrawable( state, res, res->getDrawableType() == Drawable::SPRITE );
		}
	} else if ( "backgroundcolor" == name ) {
		Color color = attribute.asColor();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Color start( getBackgroundColor( getStylePreviousState() ) );

			Action * action = Actions::Tint::New( start, color, true, transitionInfo.duration, transitionInfo.timingFunction, Actions::Tint::Background );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setBackgroundColor( state, color );
		}
	} else if ( "backgroundimage" == name ) {
		NodeAttribute::FunctionType functionType = NodeAttribute::FunctionType::parse( attribute.getValue() );

		if ( !functionType.isEmpty() ) {
			if ( functionType.getName() == "linear-gradient" && functionType.getParameters().size() >= 2 ) {
				RectangleDrawable * drawable = RectangleDrawable::New();
				RectColors rectColors;

				if ( Color::isColorString( functionType.getParameters().at(0) ) ) {
					rectColors.TopLeft = rectColors.TopRight = Color::fromString( functionType.getParameters().at(0) );
					rectColors.BottomLeft = rectColors.BottomRight = Color::fromString( functionType.getParameters().at(1) );
				} else if ( functionType.getParameters().size() >= 3 ) {
					std::string direction = functionType.getParameters().at(0);
					String::toLowerInPlace( direction );

					if ( direction == "to bottom" ) {
						rectColors.TopLeft = rectColors.TopRight = Color::fromString( functionType.getParameters().at(1) );
						rectColors.BottomLeft = rectColors.BottomRight = Color::fromString( functionType.getParameters().at(2) );
					} else if ( direction == "to left" ) {
						rectColors.TopLeft = rectColors.BottomLeft = Color::fromString( functionType.getParameters().at(2) );
						rectColors.TopRight = rectColors.BottomRight = Color::fromString( functionType.getParameters().at(1) );
					} else if ( direction == "to right" ) {
						rectColors.TopLeft = rectColors.BottomLeft = Color::fromString( functionType.getParameters().at(1) );
						rectColors.TopRight = rectColors.BottomRight = Color::fromString( functionType.getParameters().at(2) );
					} else if ( direction == "to top" ) {
						rectColors.TopLeft = rectColors.TopRight = Color::fromString( functionType.getParameters().at(2) );
						rectColors.BottomLeft = rectColors.BottomRight = Color::fromString( functionType.getParameters().at(1) );
					} else {
						rectColors.TopLeft = rectColors.TopRight = Color::fromString( functionType.getParameters().at(1) );
						rectColors.BottomLeft = rectColors.BottomRight = Color::fromString( functionType.getParameters().at(2) );
					}
				} else {
					return setAttribute( "backgroundcolor", functionType.getParameters().at(0) );
				}

				drawable->setRectColors( rectColors );

				setBackgroundDrawable( state, drawable, true );
			}
		}
	} else if ( "foreground" == name ) {
		Drawable * res = NULL;

		if ( String::startsWith( attribute.getValue(), "#" ) ) {
			setAttribute( NodeAttribute( "foregroundcolor", attribute.getValue() ) );
		} else if ( NULL != ( res = DrawableSearcher::searchByName( attribute.getValue() ) ) ) {
			setForegroundDrawable( state, res, res->getDrawableType() == Drawable::SPRITE );
		}
	} else if ( "foregroundcolor" == name ) {
		Color color = attribute.asColor();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Color start( getForegroundColor( getStylePreviousState() ) );

			Action * action = Actions::Tint::New( start, color, true, transitionInfo.duration, transitionInfo.timingFunction, Actions::Tint::Foreground );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setForegroundColor( state, color );
		}
	} else if ( "foregroundradius" == name ) {
		setForegroundRadius( state, attribute.asUint() );
	} else if ( "bordercolor" == name ) {
		Color color = attribute.asColor();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Color start( getBorderColor() );

			SAVE_NORMAL_STATE_ATTR( start.toHexString() )

			Action * action = Actions::Tint::New( start, color, false, transitionInfo.duration, transitionInfo.timingFunction, Actions::Tint::Border );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setBorderColor( color );
		}
	} else if ( "borderwidth" == name ) {
		setBorderWidth( attribute.asDpDimensionI("1") );
	} else if ( "borderradius" == name ) {
		Uint32 borderRadius = attribute.asUint();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Uint32 start( getBorderRadius( getStylePreviousState() ) );

			SAVE_NORMAL_STATE_ATTR( String::format( "%d", start ) );

			Action * action = Actions::ResizeBorderRadius::New( start, borderRadius, transitionInfo.duration, transitionInfo.timingFunction );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setBorderRadius( state, attribute.asUint() );
		}
	} else if ( "visible" == name ) {
		setVisible( attribute.asBool() );
	} else if ( "enabled" == name ) {
		setEnabled( attribute.asBool() );
	} else if ( "theme" == name ) {
		setThemeByName( attribute.asString() );

		if ( !mSkinName.empty() )
			setThemeSkin( mSkinName );
	} else if ( "skin" == name ) {
		mSkinName = attribute.asString();
		setThemeSkin( mSkinName );
	} else if ( "skincolor" == name ) {
		Color color = attribute.asColor();

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Color start( getSkinColor( getStylePreviousState() ) );

			SAVE_NORMAL_STATE_ATTR( start.toHexString() )

			Action * action = Actions::Tint::New( start, color, true, transitionInfo.duration, transitionInfo.timingFunction, Actions::Tint::Skin );

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setSkinColor( state, color );
		}
	} else if ( "gravity" == name ) {
		std::string gravity = attribute.asString();
		String::toLowerInPlace( gravity );
		std::vector<std::string> strings = String::split( gravity, '|' );

		if ( strings.size() ) {
			for ( std::size_t i = 0; i < strings.size(); i++ ) {
				std::string cur = strings[i];
				String::toLowerInPlace( cur );

				if ( "left" == cur )
					setHorizontalAlign( UI_HALIGN_LEFT );
				else if ( "right" == cur )
					setHorizontalAlign( UI_HALIGN_RIGHT );
				else if ( "center_horizontal" == cur )
					setHorizontalAlign( UI_HALIGN_CENTER );
				else if ( "top" == cur )
					setVerticalAlign( UI_VALIGN_TOP );
				else if ( "bottom" == cur )
					setVerticalAlign( UI_VALIGN_BOTTOM );
				else if ( "center_vertical" == cur ) {
					setVerticalAlign( UI_VALIGN_CENTER );
				} else if ( "center" == cur ) {
					setHorizontalAlign( UI_HALIGN_CENTER );
					setVerticalAlign( UI_VALIGN_CENTER );
				}
			}

			notifyLayoutAttrChange();
		}
	} else if ( "flags" == name ) {
		std::string flags = attribute.asString();
		String::toLowerInPlace( flags );
		std::vector<std::string> strings = String::split( flags, '|' );

		if ( strings.size() ) {
			for ( std::size_t i = 0; i < strings.size(); i++ ) {
				std::string cur = strings[i];
				String::toLowerInPlace( cur );

				if ( "auto_size" == cur || "autosize" == cur ) {
					setFlags( UI_AUTO_SIZE );
					notifyLayoutAttrChange();
				} else if ( "clip" == cur ) {
					clipEnable();
				} else if ( "multi" == cur ) {
					setFlags(  UI_MULTI_SELECT );
				} else if ( "auto_padding" == cur || "autopadding" == cur ) {
					setFlags( UI_AUTO_PADDING );
					notifyLayoutAttrChange();
				} else if ( "reportsizechangetochilds" == cur || "report_size_change_to_childs" == cur ) {
					enableReportSizeChangeToChilds();
				}
			}
		}
	} else if ( String::startsWith( name, "layout_margin" ) ) {
		Rect margin;
		Uint32 marginFlag = 0;

		if ( "layout_margin" == name ) {
			margin = attribute.asRect();
			marginFlag = Actions::MarginMove::All;
		} else if ( "layout_marginleft" == name ) {
			margin = Rect( attribute.asDpDimensionI(), mLayoutMargin.Top, mLayoutMargin.Right, mLayoutMargin.Bottom );
			marginFlag = Actions::MarginMove::Left;
		} else if ( "layout_marginright" == name ) {
			margin = Rect( mLayoutMargin.Left, mLayoutMargin.Top, attribute.asDpDimensionI(), mLayoutMargin.Bottom );
			marginFlag = Actions::MarginMove::Right;
		} else if ( "layout_margintop" == name ) {
			margin = Rect( mLayoutMargin.Left, attribute.asDpDimensionI(), mLayoutMargin.Right, mLayoutMargin.Bottom );
			marginFlag = Actions::MarginMove::Top;
		} else if ( "layout_marginbottom" == name ) {
			margin = Rect( mLayoutMargin.Left, mLayoutMargin.Top, mLayoutMargin.Right, attribute.asDpDimensionI() );
			marginFlag = Actions::MarginMove::Bottom;
		}

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Action * action = Actions::MarginMove::New( mLayoutMargin, margin, transitionInfo.duration, transitionInfo.timingFunction, marginFlag );

			CSS::StyleSheetProperty oldAttribute = mStyle->getStyleSheetProperty( UIState::StateFlagNormal, attribute.getName() );
			if ( oldAttribute.isEmpty() && mStyle->getPreviousState() == UIState::StateFlagNormal ) {
				if ( "layout_marginleft" == name || "layout_margin" == name )
					mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), String::format( "%d", mLayoutMargin.Left ) ) );

				if ( "layout_marginright" == name || "layout_margin" == name )
					mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), String::format( "%d", mLayoutMargin.Right ) ) );

				if ( "layout_margintop" == name || "layout_margin" == name )
					mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), String::format( "%d", mLayoutMargin.Top ) ) );

				if ( "layout_marginbottom" == name || "layout_margin" == name )
					mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), String::format( "%d", mLayoutMargin.Bottom ) ) );
			}

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setLayoutMargin( margin );
		}
	} else if ( "tooltip" == name ) {
		setTooltipText( attribute.asString() );
	} else if ( "layout_weight" == name ) {
		setLayoutWeight( attribute.asFloat() );
	} else if ( "layout_gravity" == name ) {
		std::string gravityStr = attribute.asString();
		String::toLowerInPlace( gravityStr );
		std::vector<std::string> strings = String::split( gravityStr, '|' );
		Uint32 gravity = 0;

		if ( strings.size() ) {
			for ( std::size_t i = 0; i < strings.size(); i++ ) {
				std::string cur = strings[i];
				String::toLowerInPlace( cur );

				if ( "left" == cur )
					gravity |= UI_HALIGN_LEFT;
				else if ( "right" == cur )
					gravity |= UI_HALIGN_RIGHT;
				else if ( "center_horizontal" == cur )
					gravity |= UI_HALIGN_CENTER;
				else if ( "top" == cur )
					gravity |= UI_VALIGN_TOP;
				else if ( "bottom" == cur )
					gravity |= UI_VALIGN_BOTTOM;
				else if ( "center_vertical" == cur ) {
					gravity |= UI_VALIGN_CENTER;
				} else if ( "center" == cur ) {
					gravity |= UI_VALIGN_CENTER | UI_HALIGN_CENTER;
				}
			}

			setLayoutGravity( gravity );
		}
	} else if ( "layout_width" == name ) {
		std::string val = attribute.asString();
		String::toLowerInPlace( val );

		if ( "match_parent" == val ) {
			setLayoutWidthRules( MATCH_PARENT );
		} else if ( "wrap_content" == val ) {
			setLayoutWidthRules( WRAP_CONTENT );
		} else if ( "fixed" == val ) {
			setLayoutWidthRules( FIXED );
			unsetFlags( UI_AUTO_SIZE );
		} else {
			unsetFlags( UI_AUTO_SIZE );
			setLayoutWidthRules( FIXED );
			setInternalWidth( PixelDensity::toDpFromStringI( val ) );
			onSizeChange();
		}
	} else if ( "layout_height" == name ) {
		std::string val = attribute.asString();
		String::toLowerInPlace( val );

		if ( "match_parent" == val ) {
			setLayoutHeightRules( MATCH_PARENT );
		} else if ( "wrap_content" == val ) {
			setLayoutHeightRules( WRAP_CONTENT );
		} else if ( "fixed" == val ) {
			setLayoutHeightRules( FIXED );
			unsetFlags( UI_AUTO_SIZE );
		} else {
			unsetFlags( UI_AUTO_SIZE );
			setLayoutHeightRules( FIXED );
			setInternalHeight( PixelDensity::toDpFromStringI( val ) );
			onSizeChange();
		}
	} else if ( String::startsWith( name, "layout_to_" ) || String::startsWith( name, "layoutto" ) ) {
		LayoutPositionRules rule = NONE;
		if ( "layout_to_left_of" == name || "layouttoleftof" == name ) rule = LEFT_OF;
		else if ( "layout_to_right_of" == name || "layouttorightof" == name ) rule = RIGHT_OF;
		else if ( "layout_to_top_of" == name || "layouttotopof" == name ) rule = TOP_OF;
		else if ( "layout_to_bottom_of" == name || "layouttobottomof" == name ) rule = BOTTOM_OF;

		std::string id = attribute.asString();

		Node * control = getParent()->find( id );

		if ( NULL != control && control->isWidget() ) {
			UIWidget * widget = static_cast<UIWidget*>( control );

			setLayoutPositionRule( rule, widget );
		}
	} else if ( "clip" == name ) {
		if ( attribute.asBool() )
			clipEnable();
		else
			clipDisable();
	} else if ( "rotation" == name ) {
		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Float newRotation( mStyle->getNodeAttribute( state, attribute.getName() ).asFloat() );
			Action * action = Actions::Rotate::New( mRotation, newRotation, transitionInfo.duration, transitionInfo.timingFunction );

			SAVE_NORMAL_STATE_ATTR( String::format( "%2.f", mRotation ) )

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setRotation( attribute.asFloat() );
		}
	} else if ( "scale" == name ) {
		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Vector2f newScale( mStyle->getNodeAttribute( state, attribute.getName() ).asVector2f() );
			Action * action = Actions::Scale::New( mScale, newScale, transitionInfo.duration, transitionInfo.timingFunction );

			SAVE_NORMAL_STATE_ATTR( String::format( "%2.f, %2.f", mScale.x, mScale.y ) )

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setScale( attribute.asVector2f() );
		}
	} else if ( "rotationoriginpoint" == name ) {
		setRotationOriginPoint( attribute.asOriginPoint() );
	} else if ( "scaleoriginpoint" == name ) {
		setScaleOriginPoint( attribute.asOriginPoint() );
	} else if ( "blendmode" == name ) {
		setBlendMode( attribute.asBlendMode() );
	} else if ( String::startsWith( name, "padding" ) ) {
		Rectf padding;
		Uint32 paddingFlag = 0;

		if ( "padding" == name ) {
			padding = ( attribute.asRectf() );
			paddingFlag = Actions::PaddingTransition::All;
		} else if ( "paddingleft" == name ) {
			padding = Rectf( attribute.asDpDimension(), mPadding.Top, mPadding.Right, mPadding.Bottom );
			paddingFlag = Actions::PaddingTransition::Left;
		} else if ( "paddingright" == name ) {
			padding = Rectf( mPadding.Left, mPadding.Top, attribute.asDpDimension(), mPadding.Bottom );
			paddingFlag = Actions::PaddingTransition::Right;
		} else if ( "paddingtop" == name ) {
			padding = Rectf( mPadding.Left, attribute.asDpDimension(), mPadding.Right, mPadding.Bottom );
			paddingFlag = Actions::PaddingTransition::Top;
		} else if ( "paddingbottom" == name ) {
			padding = Rectf( mPadding.Left, mPadding.Top, mPadding.Right, attribute.asDpDimension() );
			paddingFlag = Actions::PaddingTransition::Bottom;
		}

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Action * action = Actions::PaddingTransition::New( mPadding, padding, transitionInfo.duration, transitionInfo.timingFunction, paddingFlag );

			CSS::StyleSheetProperty oldAttribute = mStyle->getStyleSheetProperty( UIState::StateFlagNormal, attribute.getName() );
			if ( oldAttribute.isEmpty() && mStyle->getPreviousState() == UIState::StateFlagNormal ) {
				if ( "paddingleft" == name || "padding" == name )
					mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), String::format( "%2.f", mPadding.Left ) ) );

				if ( "paddingright" == name || "padding" == name )
					mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), String::format( "%2.f", mPadding.Right ) ) );

				if ( "paddingtop" == name || "padding" == name )
					mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), String::format( "%2.f", mPadding.Top ) ) );

				if ( "paddingbottom" == name || "padding" == name )
					mStyle->addStyleSheetProperty( UIState::StateFlagNormal, CSS::StyleSheetProperty( attribute.getName(), String::format( "%2.f", mPadding.Bottom ) ) );
			}

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setPadding( padding );
		}
	} else if ( "opacity" == name ) {
		Float alpha = eemin( attribute.asFloat() * 255.f, 255.f );

		if ( !isSceneNodeLoading() && NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Action * action = Actions::Fade::New( mAlpha, alpha, transitionInfo.duration, transitionInfo.timingFunction );

			SAVE_NORMAL_STATE_ATTR( String::format( "%2.f", mAlpha ) )

			if ( Time::Zero != transitionInfo.delay )
				action = Actions::Sequence::New( Actions::Delay::New( transitionInfo.delay ), action );

			runAction( action );
		} else {
			setAlpha( alpha );
			setChildsAlpha( alpha );
		}
	} else if ( "cursor" == name ) {
		SAVE_NORMAL_STATE_ATTR( "arrow" );
		mSceneNode->setCursor( Cursor::fromName( attribute.getValue() ) );
	} else {
		attributeSet = false;
	}

	return attributeSet;
}

void UIWidget::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		setAttribute( ait->name(), ait->value() );
	}

	endAttributesTransaction();
}

}}
