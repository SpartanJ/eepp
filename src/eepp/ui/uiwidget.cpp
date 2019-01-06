#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <pugixml/pugixml.hpp>
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
	mStyle( NULL ),
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

	if ( getSceneNode()->isUISceneNode() && static_cast<UISceneNode*>( getSceneNode() )->hasStyleSheet() )
		mStyle = UIStyle::New( this );
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
		mDistToBorder	= Rect( mPosition.x, mPosition.y, mParentCtrl->getRealSize().x - ( mPosition.x + mSize.x ), mParentCtrl->getRealSize().y - ( mPosition.y + mSize.y ) );
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

Uint32 UIWidget::onMouseMove( const Vector2i & Pos, const Uint32 Flags ) {
	if ( mVisible && NULL != mTooltip && !mTooltip->getText().empty() ) {
		EventDispatcher * eventDispatcher = getEventDispatcher();

		if ( NULL == eventDispatcher )
			return 1;

		UIThemeManager * themeManager = UIThemeManager::instance();

		Vector2f Pos = eventDispatcher->getMousePosf();
		Pos.x += themeManager->getCursorSize().x;
		Pos.y += themeManager->getCursorSize().y;

		if ( Pos.x + mTooltip->getRealSize().getWidth() > eventDispatcher->getSceneNode()->getRealSize().getWidth() ) {
			Pos.x = eventDispatcher->getMousePos().x - mTooltip->getRealSize().getWidth();
		}

		if ( Pos.y + mTooltip->getRealSize().getHeight() > eventDispatcher->getSceneNode()->getRealSize().getHeight() ) {
			Pos.y = eventDispatcher->getMousePos().y - mTooltip->getRealSize().getHeight();
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

Uint32 UIWidget::onMouseExit( const Vector2i & Pos, const Uint32 Flags ) {
	if ( mVisible && NULL != mTooltip && !mTooltip->getText().empty() ) {
		mTooltip->setTooltipTime( Milliseconds( 0.f ) );

		if ( mTooltip->isVisible() )
			mTooltip->hide();
	}

	return UINode::onMouseExit( Pos, Flags );
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

	if ( flags & UI_AUTO_SIZE ) {
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

const Sizef& UIWidget::getSize() {
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

void UIWidget::addClass( const std::string& cls ) {
	if ( !containsClass( cls ) ) {
		mClasses.push_back( cls );

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
	if ( NULL != mStyle )
		mStyle->pushState( State );

	UINode::pushState( State, emitEvent );
}

void UIWidget::popState( const Uint32& State, bool emitEvent ) {
	if ( NULL != mStyle )
		mStyle->popState( State );

	UINode::popState( State, emitEvent );
}

const UIStyle * UIWidget::getUIStyle() const {
	return mStyle;
}

void UIWidget::reloadStyle( const bool& reloadChilds ) {
	if ( NULL == mStyle && getSceneNode()->isUISceneNode() && static_cast<UISceneNode*>( getSceneNode() )->hasStyleSheet() ) {
		mStyle = UIStyle::New( this );
		mStyle->setState( mState );
	}

	if ( NULL != mStyle ) {
		mStyle->load();
		mStyle->onStateChange();

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

bool UIWidget::setAttribute( const std::string& name, const std::string& value, const Uint32& state ) {
	return setAttribute( NodeAttribute( name, value ), state );
}

bool UIWidget::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	std::string name = attribute.getName();

	bool attributeSet = true;

	if ( "id" == name ) {
		setId( attribute.value() );
	} else if ( "x" == name ) {
		setInternalPosition( Vector2f( attribute.asDpDimension(), mDpPos.y ) );
	} else if ( "y" == name ) {
		setInternalPosition( Vector2f( mDpPos.x, attribute.asDpDimension() ) );
	} else if ( "width" == name ) {
		setInternalWidth( attribute.asDpDimensionI() );
		notifyLayoutAttrChange();
	} else if ( "height" == name ) {
		setInternalHeight( attribute.asDpDimensionI() );
		notifyLayoutAttrChange();
	} else if ( "background" == name ) {
		Drawable * res = NULL;

		const std::string attributeName( attribute.asString() );

		if ( String::startsWith( attributeName, "#" ) ) {
			setBackgroundColor( state, attribute.asColor() );
		} else if ( NULL != ( res = DrawableSearcher::searchByName( attributeName ) ) ) {
			setBackgroundDrawable( state, res, res->getDrawableType() == Drawable::SPRITE );
		}
	} else if ( "backgroundcolor" == name ) {
		setBackgroundColor( state, attribute.asColor() );
	} else if ( "foreground" == name ) {
		Drawable * res = NULL;

		const std::string attributeName( attribute.asString() );

		if ( String::startsWith( attributeName, "#" ) ) {
			setForegroundColor( state, attribute.asColor() );
		} else if ( NULL != ( res = DrawableSearcher::searchByName( attributeName ) ) ) {
			setForegroundDrawable( state, res, res->getDrawableType() == Drawable::SPRITE );
		}
	} else if ( "foregroundcolor" == name ) {
		setForegroundColor( state, attribute.asColor() );
	} else if ( "foregroundcorners" == name ) {
		setForegroundCorners( state, attribute.asUint() );
	} else if ( "bordercolor" == name ) {
		setBorderColor( state, attribute.asColor() );
	} else if ( "borderwidth" == name ) {
		setBorderWidth( state, attribute.asDpDimensionI("1") );
	} else if ( "bordercorners" == name || "backgroundcorners" == name ) {
		setBackgroundCorners( state, attribute.asUint() );
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
				} else if ( "word_wrap" == cur || "wordwrap" == cur ) {
					setFlags( UI_WORD_WRAP );
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
	} else if ( "layout_margin" == name ) {
		setLayoutMargin( attribute.asRect() );
	} else if ( "layout_marginleft" == name ) {
		setLayoutMargin( Rect( attribute.asDpDimensionI(), mLayoutMargin.Top, mLayoutMargin.Right, mLayoutMargin.Bottom ) );
	} else if ( "layout_marginright" == name ) {
		setLayoutMargin( Rect( mLayoutMargin.Left, mLayoutMargin.Top, attribute.asDpDimensionI(), mLayoutMargin.Bottom ) );
	} else if ( "layout_margintop" == name ) {
		setLayoutMargin( Rect( mLayoutMargin.Left, attribute.asDpDimensionI(), mLayoutMargin.Right, mLayoutMargin.Bottom ) );
	} else if ( "layout_marginbottom" == name ) {
		setLayoutMargin( Rect( mLayoutMargin.Left, mLayoutMargin.Top, mLayoutMargin.Right, attribute.asDpDimensionI() ) );
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
		setRotation( attribute.asFloat() );
	} else if ( "scale" == name ) {
		/*if ( NULL != mStyle && mStyle->hasTransition( state, attribute.getName() ) ) {
			UIStyle::TransitionInfo transitionInfo( mStyle->getTransition( state, attribute.getName() ) );
			Action * action = NULL;

			if ( Time::Zero == transitionInfo.delay ) {
				action = Actions::Scale::New( mScale, mStyle->getAttribute( state, { attribute.getName() } ).asVector2f(), transitionInfo.duration, transitionInfo.timingFunction );
			}

			if ( NULL != action )
				runAction( action );
		} else {*/
			setScale( attribute.asVector2f() );
		//}
	} else if ( "rotationoriginpoint" == name ) {
		setRotationOriginPoint( attribute.asOriginPoint() );
	} else if ( "scaleoriginpoint" == name ) {
		setScaleOriginPoint( attribute.asOriginPoint() );
	} else if ( "blendmode" == name ) {
		setBlendMode( attribute.asBlendMode() );
	} else if ( "padding" == name ) {
		setPadding( attribute.asRectf() );
	} else if ( "paddingleft" == name ) {
		setPadding( Rectf( attribute.asDpDimension(), mPadding.Top, mPadding.Right, mPadding.Bottom ) );
	} else if ( "paddingright" == name ) {
		setPadding( Rectf( mPadding.Left, mPadding.Top, attribute.asDpDimension(), mPadding.Bottom ) );
	} else if ( "paddingtop" == name ) {
		setPadding( Rectf( mPadding.Left, attribute.asDpDimension(), mPadding.Right, mPadding.Bottom ) );
	} else if ( "paddingbottom" == name ) {
		setPadding( Rectf( mPadding.Left, mPadding.Top, mPadding.Right, attribute.asDpDimension() ) );
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
