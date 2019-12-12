#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/triangledrawable.hpp>
#include <eepp/graphics/circledrawable.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/window/window.hpp>
#include <eepp/window/engine.hpp>
#include <pugixml/pugixml.hpp>
#include <algorithm>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>

using namespace EE::Window;

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

	reloadStyle( false );

	updateAnchorsDistances();
}

UIWidget::UIWidget() :
	UIWidget( "widget" )
{
	reloadStyle( false );
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

UIWidget * UIWidget::setLayoutMarginLeft(const Float& marginLeft) {
	if ( mLayoutMargin.Left != marginLeft ) {
		mLayoutMargin.Left = marginLeft;
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::setLayoutMarginRight(const Float& marginRight) {
	if ( mLayoutMargin.Right != marginRight ) {
		mLayoutMargin.Right = marginRight;
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::setLayoutMarginTop(const Float& marginTop) {
	if ( mLayoutMargin.Top != marginTop ) {
		mLayoutMargin.Top = marginTop;
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::setLayoutMarginBottom(const Float& marginBottom) {
	if ( mLayoutMargin.Bottom != marginBottom ) {
		mLayoutMargin.Bottom = marginBottom;
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

		UIThemeManager * themeManager = getUISceneNode()->getUIThemeManager();

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
	return setThemeSkin( NULL != mTheme ? mTheme : getUISceneNode()->getUIThemeManager()->getDefaultTheme(), skinName );
}

UINode * UIWidget::setThemeSkin( UITheme * Theme, const std::string& skinName ) {
	return UINode::setThemeSkin( Theme, skinName );
}

Node * UIWidget::setSize( const Float& Width, const Float& Height ) {
	return UINode::setSize( Width, Height );
}

Node * UIWidget::setId( const std::string& id ) {
	Node::setId( id );

	reloadStyle( true );

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

	Sizef newSize( getSize() );

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

	if ( newSize != getSize() )
		setSize( newSize );
}

void UIWidget::alignAgainstLayout() {
	Vector2f pos = mDpPos;

	switch ( fontHAlignGet( mLayoutGravity ) ) {
		case UI_HALIGN_CENTER:
			pos.x = ( getParent()->getSize().getWidth() - getSize().getWidth() ) / 2;
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
			pos.y = ( getParent()->getSize().getHeight() - getSize().getHeight() ) / 2;
			break;
		case UI_VALIGN_BOTTOM:
			pos.y = getParent()->getSize().getHeight() - mLayoutMargin.Bottom;
			break;
		case UI_VALIGN_TOP:
			pos.y = mLayoutMargin.Top;
			break;
	}

	setPosition( pos );
}

void UIWidget::reportStyleStateChange() {
	if ( NULL != mStyle && !mStyle->isChangingState() )
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

UIWidget * UIWidget::setPaddingLeft(const Float& paddingLeft) {
	if ( paddingLeft != mPadding.Left ) {
		mPadding.Left = paddingLeft;
		mRealPadding.Left = PixelDensity::dpToPx( mPadding.Left );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::setPaddingRight(const Float& paddingRight) {
	if ( paddingRight != mPadding.Right ) {
		mPadding.Right = paddingRight;
		mRealPadding.Right = PixelDensity::dpToPx( mPadding.Right );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::setPaddingTop(const Float& paddingTop) {
	if ( paddingTop != mPadding.Top ) {
		mPadding.Top = paddingTop;
		mRealPadding.Top = PixelDensity::dpToPx( mPadding.Top );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget * UIWidget::setPaddingBottom(const Float& paddingBottom) {
	if ( paddingBottom != mPadding.Bottom ) {
		mPadding.Bottom = paddingBottom;
		mRealPadding.Bottom = PixelDensity::dpToPx( mPadding.Bottom );
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

		reloadStyle( true );
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

		reloadStyle( true );
	}
}

void UIWidget::removeClass( const std::string& cls ) {
	if ( containsClass( cls ) ) {
		mClasses.erase( std::find( mClasses.begin(), mClasses.end(), cls ) );

		reloadStyle( true );
	}
}

void UIWidget::removeClasses( const std::vector<std::string>& classes ) {
	if ( !classes.empty() ) {
		for ( auto cit = classes.begin(); cit != classes.end(); ++cit ) {
			const std::string& cls = *cit;

			if ( !cls.empty() ) {
				auto found = std::find( mClasses.begin(), mClasses.end(), cls );

				if ( found != mClasses.end() ) {
					mClasses.erase(found);
				}
			}
		}

		reloadStyle( true );
	}
}

bool UIWidget::containsClass( const std::string& cls ) const {
	return std::find( mClasses.begin(), mClasses.end(), cls ) != mClasses.end();
}

void UIWidget::setElementTag( const std::string& tag ) {
	if ( mTag != tag ) {
		mTag = tag;

		reloadStyle( true );
	}
}

const std::string& UIWidget::getElementTag() const {
	return mTag;
}

void UIWidget::pushState( const Uint32& State, bool emitEvent ) {
	if ( !( mState & ( 1 << State ) ) ) {
		mState |= 1 << State;

		if ( NULL != mSkinState )
			mSkinState->pushState( State );

		if ( NULL != mStyle ) {
			updatePseudoClasses();
			mStyle->pushState( State );
		}

		if ( emitEvent ) {
			onStateChange();
		} else {
			invalidateDraw();
		}
	}
}

void UIWidget::popState( const Uint32& State, bool emitEvent ) {
	if ( mState & ( 1 << State ) ) {
		mState &= ~( 1 << State );

		if ( NULL != mSkinState )
			mSkinState->popState( State );

		if ( NULL != mStyle ) {
			updatePseudoClasses();
			mStyle->popState( State );
		}

		if ( emitEvent ) {
			onStateChange();
		} else {
			invalidateDraw();
		}
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
					ChildLoop->asType<UIWidget>()->reloadStyle( reloadChilds );

				ChildLoop = ChildLoop->getNextNode();
			}
		}
	}
}

void UIWidget::onPaddingChange() {
	sendCommonEvent( Event::OnPaddingChange );
	invalidateDraw();
}

void UIWidget::onThemeLoaded() {
	reportStyleStateChange();
}

void UIWidget::onParentChange() {
	reloadStyle( true );
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

std::vector<UIWidget*> UIWidget::findAllByClass( const std::string& className ) {
	std::vector<UIWidget*> widgets;

	if ( containsClass( className ) ) {
		widgets.push_back( this );
	}

	Node * child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() ) {
			std::vector<UIWidget*> foundWidgets = child->asType<UIWidget>()->findAllByClass( className );

			if ( !foundWidgets.empty() )
				widgets.insert( widgets.end(), foundWidgets.begin(), foundWidgets.end() );
		}

		child = child->getNextNode();
	}

	return widgets;
}

std::vector<UIWidget*> UIWidget::findAllByTag( const std::string& tag ) {
	std::vector<UIWidget*> widgets;

	if ( getElementTag() == tag ) {
		widgets.push_back( this );
	}

	Node * child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() ) {
			std::vector<UIWidget*> foundWidgets = child->asType<UIWidget>()->findAllByTag( tag );

			if ( !foundWidgets.empty() )
				widgets.insert( widgets.end(), foundWidgets.begin(), foundWidgets.end() );
		}

		child = child->getNextNode();
	}

	return widgets;
}

UIWidget* UIWidget::findByClass( const std::string& className ) {
	if ( containsClass( className ) ) {
		return this;
	} else {
		Node * child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				UIWidget * foundWidget = child->asType<UIWidget>()->findByClass( className );

				if ( NULL != foundWidget )
					return foundWidget;
			}

			child = child->getNextNode();
		}
	}

	return NULL;
}

UIWidget* UIWidget::findByTag( const std::string& tag ) {
	if ( getElementTag() == tag ) {
		return this;
	} else {
		Node * child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				UIWidget * foundWidget = child->asType<UIWidget>()->findByTag( tag );

				if ( NULL != foundWidget )
					return foundWidget;
			}

			child = child->getNextNode();
		}
	}

	return NULL;
}

UIWidget* UIWidget::querySelector( const CSS::StyleSheetSelector& selector ) {
	if ( selector.select( this ) ) {
		return this;
	} else {
		Node * child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				UIWidget * foundWidget = child->asType<UIWidget>()->querySelector( selector );

				if ( NULL != foundWidget )
					return foundWidget;
			}

			child = child->getNextNode();
		}
	}

	return NULL;
}

std::vector<UIWidget*> UIWidget::querySelectorAll( const CSS::StyleSheetSelector& selector ) {
	std::vector<UIWidget*> widgets;

	if ( selector.select( this ) ) {
		widgets.push_back( this );
	}

	Node * child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() ) {
			std::vector<UIWidget*> foundWidgets = child->asType<UIWidget>()->querySelectorAll( selector );

			if ( !foundWidgets.empty() )
				widgets.insert( widgets.end(), foundWidgets.begin(), foundWidgets.end() );
		}

		child = child->getNextNode();
	}

	return widgets;
}

bool UIWidget::checkPropertyDefinition( const StyleSheetProperty& property ) {
	if ( property.getPropertyDefinition() == NULL ) {
		eePRINTL( "applyProperty: Property %s not defined!", property.getName().c_str() );
		return false;
	}
	return true;
}

UIWidget* UIWidget::querySelector( const std::string& selector ) {
	return querySelector( CSS::StyleSheetSelector( selector ) );
}

std::vector<UIWidget*> UIWidget::querySelectorAll( const std::string& selector ) {
	return querySelectorAll( CSS::StyleSheetSelector( selector ) );
}

std::string UIWidget::getPropertyString( const std::string& property ) {
	return getPropertyString( StyleSheetSpecification::instance()->getProperty( property ) );
}

std::string UIWidget::getPropertyString( const PropertyDefinition* propertyDef ) {
	if ( NULL == propertyDef ) return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::X:
			return String::fromFloat( getPosition().x, "dp" );
		case PropertyId::Y:
			return String::fromFloat( getPosition().y, "dp" );
		case PropertyId::Width:
			return String::fromFloat( getSize().getWidth(), "dp" );
		case PropertyId::Height:
			return String::fromFloat( getSize().getHeight(), "dp" );
		case PropertyId::MarginLeft:
			return String::format( "%ddp", getLayoutMargin().Left );
		case PropertyId::MarginTop:
			return String::format( "%ddp", getLayoutMargin().Top );
		case PropertyId::MarginRight:
			return String::format( "%ddp", getLayoutMargin().Right );
		case PropertyId::MarginBottom:
			return String::format( "%ddp", getLayoutMargin().Bottom );
		case PropertyId::PaddingLeft:
			return String::fromFloat( getPadding().Left, "dp" );
		case PropertyId::PaddingTop:
			return String::fromFloat( getPadding().Top, "dp" );
		case PropertyId::PaddingRight:
			return String::fromFloat( getPadding().Right, "dp" );
		case PropertyId::PaddingBottom:
			return String::fromFloat( getPadding().Bottom, "dp" );
		case PropertyId::BackgroundColor:
			return getBackgroundColor().toHexString();
		case PropertyId::ForegroundColor:
			return getForegroundColor().toHexString();
		case PropertyId::ForegroundRadius:
			return String::toStr( getForegroundRadius() );
		case PropertyId::BorderColor:
			return getBorderColor().toHexString();
		case PropertyId::BorderRadius:
			return String::toStr( getBorderRadius() );
		case PropertyId::BorderWidth:
			return String::fromFloat( getBorderWidth() );
		case PropertyId::SkinColor:
			return getSkinColor().toHexString();
		case PropertyId::Rotation:
			return String::fromFloat( getRotation() );
		case PropertyId::Scale:
			return String::fromFloat( getScale().x ) + ", " + String::fromFloat( getScale().y );
		case PropertyId::Opacity:
			return String::fromFloat( getAlpha() );
		case PropertyId::Cursor:
			return "arrow";
		case PropertyId::Visible:
			return isVisible() ? "true" : "false";
		case PropertyId::Enabled:
			return isEnabled() ? "true" : "false";
		case PropertyId::Theme:
			return NULL != mTheme ? mTheme->getName() : "";
		case PropertyId::Skin:
			return mSkinName;
		case PropertyId::Flags:
			return getFlagsString();
		case PropertyId::BackgroundSize:
			return getBackground()->getLayer(0)->getSizeEq();
		case PropertyId::ForegroundSize:
			return getForeground()->getLayer(0)->getSizeEq();
		case PropertyId::LayoutWeight:
			return String::fromFloat( getLayoutWeight() );
		case PropertyId::LayoutGravity:
			return getLayoutGravityString();
		case PropertyId::LayoutWidth:
			return getLayoutWidthRulesString();
		case PropertyId::LayoutHeight:
			return getLayoutHeightRulesString();
		case PropertyId::Clip:
			return isClipped() ? "true" : "false";
		case PropertyId::BackgroundPositionX:
			return getBackground()->getLayer(0)->getPositionX();
		case PropertyId::BackgroundPositionY:
			return getBackground()->getLayer(0)->getPositionY();
		case PropertyId::ForegroundPositionX:
			return getForeground()->getLayer(0)->getPositionX();
		case PropertyId::ForegroundPositionY:
			return getForeground()->getLayer(0)->getPositionY();
		case PropertyId::ScaleOriginPoint:
			return getScaleOriginPoint().toString();
		case PropertyId::BlendMode:
			return "";
		default:
			break;
	}

	return "";
}

void UIWidget::setStyleSheetInlineProperty( const std::string& name, const std::string& value, const Uint32& specificity ) {
	if ( mStyle != NULL )
		mStyle->setStyleSheetProperty( CSS::StyleSheetProperty( name, value, specificity ) );
}

bool UIWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) ) return false;
	bool attributeSet = true;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Id:
			setId( attribute.value() );
			break;
		case PropertyId::Class:
			addClasses( String::split( attribute.getValue(), ' ' ) );
			break;
		case PropertyId::X:
			setLayoutWidthRules( FIXED );
			setInternalPosition( Vector2f( attribute.asDpDimension(), mDpPos.y ) );
			notifyLayoutAttrChange();
			break;
		case PropertyId::Y:
			setLayoutWidthRules( FIXED );
			setInternalPosition( Vector2f( mDpPos.x, attribute.asDpDimensionI() ) );
			notifyLayoutAttrChange();
			break;
		case PropertyId::Width:
			setLayoutWidthRules( FIXED );
			setInternalWidth( attribute.asDpDimensionI() );
			notifyLayoutAttrChange();
			break;
		case PropertyId::Height:
			setLayoutHeightRules( FIXED );
			setInternalHeight( attribute.asDpDimensionI() );
			notifyLayoutAttrChange();
			break;
		case PropertyId::BackgroundColor:
			setBackgroundColor( attribute.asColor() );
			break;
		case PropertyId::BackgroundImage:
			drawablePropertySet( "background", attribute.getValue(), [&] ( Drawable * drawable, bool ownIt, int index ) {
				setBackgroundDrawable( drawable, ownIt, index );
			} );
			break;
		case PropertyId::BackgroundRepeat:
			setBackgroundRepeat( attribute.value(), 0 );
			break;
		case PropertyId::BackgroundSize:
			setBackgroundSize( attribute.value(), 0 );
			break;
		case PropertyId::ForegroundColor:
			setForegroundColor( attribute.asColor() );
			break;
		case PropertyId::ForegroundImage:
			drawablePropertySet( "foreground", attribute.getValue(), [&] ( Drawable * drawable, bool ownIt, int index ) {
				setForegroundDrawable( drawable, ownIt, index );
			} );
			break;
		case PropertyId::ForegroundRadius:
			setForegroundRadius( attribute.asUint() );
			break;
		case PropertyId::ForegroundSize:
			setForegroundSize( attribute.value(), 0 );
			break;
		case PropertyId::BorderColor:
			setBorderColor( attribute.asColor() );
			break;
		case PropertyId::BorderWidth:
			setBorderWidth( PixelDensity::dpToPxI( attribute.asDpDimensionI("1") ) );
			break;
		case PropertyId::BorderRadius:
			setBorderRadius( PixelDensity::dpToPxI( attribute.asDpDimensionUint() ));
			break;
		case PropertyId::Visible:
			setVisible( attribute.asBool() );
			break;
		case PropertyId::Enabled:
			setEnabled( attribute.asBool() );
			break;
		case PropertyId::Theme:
			setThemeByName( attribute.asString() );
			if ( !mSkinName.empty() ) setThemeSkin( mSkinName );
			break;
		case PropertyId::Skin:
			mSkinName = attribute.asString();
			if ( "none" == mSkinName || mSkinName.empty() ) {
				removeSkin();
			} else {
				setThemeSkin( mSkinName );
			}
			break;
		case PropertyId::SkinColor:
			setSkinColor( attribute.asColor() );
			break;
		case PropertyId::Gravity:
		{
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
			break;
		}
		case PropertyId::Flags:
		{
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
			break;
		}
		case PropertyId::MarginLeft:
			setLayoutMarginLeft( attribute.asDpDimensionI() );
			break;
		case PropertyId::MarginRight:
			setLayoutMarginRight( attribute.asDpDimensionI() );
			break;
		case PropertyId::MarginTop:
			setLayoutMarginTop( attribute.asDpDimensionI() );
			break;
		case PropertyId::MarginBottom:
			setLayoutMarginBottom( attribute.asDpDimensionI() );
			break;
		case PropertyId::Tooltip:
			setTooltipText( attribute.asString() );
			break;
		case PropertyId::LayoutWeight:
			setLayoutWeight( attribute.asFloat() );
			break;
		case PropertyId::LayoutGravity:
		{
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
			break;
		}
		case PropertyId::LayoutWidth:
		{
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "match_parent" == val || "match-parent" == val ) {
				setLayoutWidthRules( MATCH_PARENT );
			} else if ( "wrap_content" == val || "wrap-content" == val ) {
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
			break;
		}
		case PropertyId::LayoutHeight:
		{
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
			break;
		}
		case PropertyId::LayoutToBottomOf:
		case PropertyId::LayoutToLeftOf:
		case PropertyId::LayoutToRightOf:
		case PropertyId::LayoutToTopOf:
		{
			LayoutPositionRules rule = NONE;
			PropertyId layoutId = static_cast<PropertyId>( attribute.getPropertyDefinition()->getId() );
			if ( layoutId == PropertyId::LayoutToLeftOf ) rule = LEFT_OF;
			else if ( layoutId == PropertyId::LayoutToRightOf ) rule = RIGHT_OF;
			else if ( layoutId == PropertyId::LayoutToTopOf ) rule = TOP_OF;
			else if ( layoutId == PropertyId::LayoutToBottomOf ) rule = BOTTOM_OF;
			std::string id = attribute.asString();
			Node * control = getParent()->find( id );
			if ( NULL != control && control->isWidget() ) {
				UIWidget * widget = static_cast<UIWidget*>( control );
				setLayoutPositionRule( rule, widget );
			}
			break;
		}
		case PropertyId::Clip:
			if ( attribute.asBool() ) clipEnable();
			else clipDisable();
			break;
		case PropertyId::Rotation:
			setRotation( attribute.asFloat() );
			break;
		case PropertyId::Scale:
			setScale( attribute.asVector2f() );
			break;
		case PropertyId::BlendMode:
			setBlendMode( attribute.asBlendMode() );
			break;
		case PropertyId::PaddingLeft:
			setPaddingLeft( attribute.asDpDimension() );
			break;
		case PropertyId::PaddingRight:
			setPaddingRight( attribute.asDpDimension() );
			break;
		case PropertyId::PaddingTop:
			setPaddingTop( attribute.asDpDimension() );
			break;
		case PropertyId::PaddingBottom:
			setPaddingBottom( attribute.asDpDimension() );
			break;
		case PropertyId::Opacity:
		{
			Float alpha = eemin( attribute.asFloat() * 255.f, 255.f );
			setAlpha( alpha );
			setChildsAlpha( alpha );
			break;
		}
		case PropertyId::Cursor:
			mSceneNode->setCursor( Cursor::fromName( attribute.getValue() ) );
			break;
		case PropertyId::BackgroundPositionX:
			setBackgroundPositionX( attribute.value(), 0 );
			break;
		case PropertyId::BackgroundPositionY:
			setBackgroundPositionY( attribute.value(), 0 );
			break;
		case PropertyId::ForegroundPositionX:
			setForegroundPositionX( attribute.value(), 0 );
			break;
		case PropertyId::ForegroundPositionY:
			setForegroundPositionY( attribute.value(), 0 );
			break;
		case PropertyId::RotationOriginPoint:
			setRotationOriginPoint( attribute.asOriginPoint() );
			break;
		case PropertyId::ScaleOriginPoint:
			setScaleOriginPoint( attribute.asOriginPoint() );
			break;
		default:
			attributeSet = false;
			break;
	}

	return attributeSet;
}

void UIWidget::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		StyleSheetProperty prop( ait->name(), ait->value() );

		if ( prop.getShorthandDefinition() != NULL ) {
			auto properties = ShorthandDefinition::parseShorthand( prop.getShorthandDefinition(), ait->value() );

			for ( auto& property : properties )
				applyProperty( property );
		} else {
			applyProperty( prop );
		}
	}

	endAttributesTransaction();
}

std::string UIWidget::getLayoutWidthRulesString() const {
	LayoutSizeRules rules = getLayoutWidthRules();

	if ( rules == LayoutSizeRules::MATCH_PARENT ) return "match_parent";
	else if ( rules == LayoutSizeRules::WRAP_CONTENT ) return "wrap_content";
	return String::toStr( getSize().getHeight() ) + "dp";
}

std::string UIWidget::getLayoutHeightRulesString() const {
	LayoutSizeRules rules = getLayoutHeightRules();

	if ( rules == LayoutSizeRules::MATCH_PARENT ) return "match_parent";
	else if ( rules == LayoutSizeRules::WRAP_CONTENT ) return "wrap_content";
	return String::toStr( getSize().getHeight() ) + "dp";
}

static std::string getGravityStringFromUint( const Uint32& gravity ) {
	std::vector<std::string> gravec;

	if ( HAlignGet( gravity ) == UI_HALIGN_RIGHT ) {
		gravec.push_back( "right" );
	} else if ( HAlignGet( gravity ) == UI_HALIGN_CENTER ) {
		gravec.push_back( "center_horizontal" );
	} else {
		gravec.push_back( "left" );
	}

	if ( VAlignGet( gravity ) == UI_VALIGN_BOTTOM ) {
		gravec.push_back( "bottom" );
	} else if ( VAlignGet( gravity ) == UI_VALIGN_CENTER ) {
		gravec.push_back( "center_vertical" );
	} else {
		gravec.push_back( "top" );
	}

	return String::join( gravec, '|' );
}

std::string UIWidget::getLayoutGravityString() const {
	return getGravityStringFromUint( getLayoutGravity() );
}

std::string UIWidget::getGravityString() const {
	return getGravityStringFromUint( getHorizontalAlign() | getVerticalAlign() );
}

std::string UIWidget::getFlagsString() const {
	std::vector<std::string> flagvec;

	if ( mFlags & UI_AUTO_SIZE ) flagvec.push_back( "autosize" );
	if ( mFlags & UI_MULTI_SELECT ) flagvec.push_back( "multi" );
	if ( mFlags & UI_AUTO_PADDING ) flagvec.push_back( "autopadding" );
	if ( reportSizeChangeToChilds() ) flagvec.push_back( "reportsizechangetochilds" );
	if ( isClipped() ) flagvec.push_back( "clip" );

	return String::join( flagvec, '|' );
}

bool UIWidget::drawablePropertySet(const std::string& propertyName, const std::string& value, std::function<void (Drawable*, bool, int)> funcSet) {
	FunctionString functionType = FunctionString::parse( value );
	Drawable * res = NULL;
	int index = 0;
	bool attributeSet = true;

	if ( !functionType.isEmpty() ) {
		if ( functionType.getName() == "linear-gradient" && functionType.getParameters().size() >= 2 ) {
			RectangleDrawable * drawable = RectangleDrawable::New();
			RectColors rectColors;

			const std::vector<std::string>& params( functionType.getParameters() );

			if ( Color::isColorString( params.at(0) ) && params.size() >= 2 ) {
				rectColors.TopLeft = rectColors.TopRight = Color::fromString( params.at(0) );
				rectColors.BottomLeft = rectColors.BottomRight = Color::fromString( params.at(1) );
			} else if ( params.size() >= 3 ) {
				std::string direction = params.at(0);
				String::toLowerInPlace( direction );

				if ( direction == "to bottom" ) {
					rectColors.TopLeft = rectColors.TopRight = Color::fromString( params.at(1) );
					rectColors.BottomLeft = rectColors.BottomRight = Color::fromString( params.at(2) );
				} else if ( direction == "to left" ) {
					rectColors.TopLeft = rectColors.BottomLeft = Color::fromString( params.at(2) );
					rectColors.TopRight = rectColors.BottomRight = Color::fromString( params.at(1) );
				} else if ( direction == "to right" ) {
					rectColors.TopLeft = rectColors.BottomLeft = Color::fromString( params.at(1) );
					rectColors.TopRight = rectColors.BottomRight = Color::fromString( params.at(2) );
				} else if ( direction == "to top" ) {
					rectColors.TopLeft = rectColors.TopRight = Color::fromString( params.at(2) );
					rectColors.BottomLeft = rectColors.BottomRight = Color::fromString( params.at(1) );
				} else {
					rectColors.TopLeft = rectColors.TopRight = Color::fromString( params.at(1) );
					rectColors.BottomLeft = rectColors.BottomRight = Color::fromString( params.at(2) );
				}
			} else {
				return applyProperty( StyleSheetProperty( propertyName + "-color", params.at(0) ) );
			}

			drawable->setRectColors( rectColors );

			funcSet( drawable, true, index );
		} else if ( functionType.getName() == "circle" && functionType.getParameters().size() >= 1 ) {
			CircleDrawable * drawable = CircleDrawable::New();

			const std::vector<std::string>& params( functionType.getParameters() );

			CSS::StyleSheetLength length( params[0] );
			drawable->setRadius( convertLength( length, getPixelsSize().getWidth() / 2.f ) );

			if ( params.size() >= 2 ) {
				drawable->setColor( Color::fromString( params[1] ) );
			}

			if ( params.size() >= 3 ) {
				std::string fillMode( String::toLower( params[2] ) );
				if ( fillMode == "line" || fillMode == "solid" || fillMode == "fill" ) {
					drawable->setFillMode( fillMode == "line" ? DRAW_LINE : DRAW_FILL );
				}
			}

			drawable->setOffset( drawable->getSize() / 2.f );

			funcSet( drawable, true, index );
		} else if ( functionType.getName() == "rectangle" && functionType.getParameters().size() >= 1 ) {
			RectangleDrawable * drawable = RectangleDrawable::New();
			RectColors rectColors;
			std::vector<Color> colors;

			const std::vector<std::string>& params( functionType.getParameters() );

			for ( size_t i = 0; i < params.size(); i++ ) {
				std::string param( String::toLower( params[i] ) );

				if ( param == "solid" || param == "fill" ) {
					drawable->setFillMode( DRAW_FILL );
				} else if ( String::startsWith( param, "line" ) ) {
					drawable->setFillMode( DRAW_LINE );

					std::vector<std::string> parts( String::split( param, ' ') );

					if ( parts.size() >= 2 ) {
						CSS::StyleSheetLength length( parts[1] );
						drawable->setLineWidth( convertLength( length, getPixelsSize().getWidth() ) );
					}
				} else if ( param.find( "º" ) != std::string::npos ) {
					String::replaceAll( param, "º", "" );
					Float floatVal;
					if ( String::fromString( floatVal, param ) ) {
						drawable->setRotation( floatVal );
					}
				} else if ( Color::isColorString( param ) ) {
					colors.push_back( Color::fromString( param ) );
				} else {
					int intVal = 0;

					if ( String::fromString( intVal, param ) ) {
						drawable->setCorners( intVal );
					}
				}
			}

			if ( colors.size() > 0 ) {
				while( colors.size() < 4 ) {
					colors.push_back( colors[ colors.size() - 1 ] );
				};

				rectColors.TopLeft = colors[0];
				rectColors.BottomLeft = colors[1];
				rectColors.BottomRight = colors[2];
				rectColors.TopRight = colors[3];
				drawable->setRectColors( rectColors );

				funcSet( drawable, true, index );
			} else {
				eeSAFE_DELETE( drawable );
			}
		} else if ( functionType.getName() == "triangle" && functionType.getParameters().size() >= 2 ) {
			TriangleDrawable* drawable = TriangleDrawable::New();
			std::vector<Color> colors;
			std::vector<Vector2f> vertices;

			const std::vector<std::string>& params( functionType.getParameters() );

			for ( size_t i = 0; i < params.size(); i++ ) {
				std::string param( String::toLower( params[i] ) );

				if ( Color::isColorString( param ) ) {
					colors.push_back( Color::fromString( param ) );
				} else {
					std::vector<std::string> vertex( String::split( param, ',' ) );

					if ( vertex.size() == 3 ) {
						for ( size_t v = 0; v < vertex.size(); v++ ) {
							vertex[v] = String::trim( vertex[v] );
							std::vector<std::string> coords( String::split( vertex[v], ' ' ) );

							if ( coords.size() == 2 ) {
								CSS::StyleSheetLength posX( coords[0] );
								CSS::StyleSheetLength posY( coords[1] );
								vertices.push_back(
									Vector2f( convertLength( posX, getPixelsSize().getWidth() ),
											  convertLength( posY, getPixelsSize().getHeight() ) ) );
							}
						}
					}
				}
			}

			if ( vertices.size() == 3 && !colors.empty() ) {
				Triangle2f triangle;

				for ( size_t i = 0; i < 3; i++ ) {
					triangle.V[i] = vertices[i];
				}

				if ( colors.size() == 3 ) {
					drawable->setTriangleColors( colors[0], colors[1], colors[2] );
				} else {
					drawable->setColor( colors[0] );
				}

				drawable->setTriangle( triangle );

				funcSet( drawable, true, index );
			} else {
				eeSAFE_DELETE( drawable );
			}
		} else if ( functionType.getName() == "url" && functionType.getParameters().size() >= 1 ) {
			if ( NULL != ( res = DrawableSearcher::searchByName( functionType.getParameters().at(0) ) ) ) {
				funcSet( res, res->getDrawableType() == Drawable::SPRITE, index );
			}
		}
	} else if ( NULL != ( res = DrawableSearcher::searchByName( value ) ) ) {
		funcSet( res, res->getDrawableType() == Drawable::SPRITE, index );
	} else {
		attributeSet = false;
	}

	return attributeSet;
}

}}
