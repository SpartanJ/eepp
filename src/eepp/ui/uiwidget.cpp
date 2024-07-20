#include <algorithm>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>
#include <eepp/ui/uiborderdrawable.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using namespace EE::Window;

namespace EE { namespace UI {

UIWidget* UIWidget::New() {
	return eeNew( UIWidget, () );
}

UIWidget* UIWidget::NewWithTag( const std::string& tag ) {
	return eeNew( UIWidget, ( tag ) );
}

UIWidget::UIWidget( const std::string& tag ) :
	UINode(),
	mTag( tag ),
	mTheme( NULL ),
	mStyle( NULL ),
	mTooltip( NULL ),
	mLayoutWeight( 0 ),
	mLayoutGravity( 0 ),
	mWidthPolicy( SizePolicy::WrapContent ),
	mHeightPolicy( SizePolicy::WrapContent ),
	mLayoutPositionPolicy( PositionPolicy::None ),
	mLayoutPositionPolicyWidget( NULL ),
	mAttributesTransactionCount( 0 ) {
	mNodeFlags |= NODE_FLAG_WIDGET;
	mFlags |= UI_TAB_FOCUSABLE | UI_TOOLTIP_ENABLED;

	createStyle();

	if ( NULL != mUISceneNode && !isSceneNodeLoading() && !isLoadingState() ) {
		mUISceneNode->invalidateStyle( this );
		mUISceneNode->invalidateStyleState( this, true );
	}
}

UIWidget::UIWidget() : UIWidget( "widget" ) {}

UIWidget::~UIWidget() {
	onClose();

	if ( mUISceneNode && mUISceneNode->getUIEventDispatcher() &&
		 mUISceneNode->getUIEventDispatcher()->getNodeDragging() == this )
		mUISceneNode->getUIEventDispatcher()->setNodeDragging( nullptr );

	if ( !SceneManager::instance()->isShuttingDown() && NULL != mUISceneNode )
		mUISceneNode->onWidgetDelete( this );
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
	if ( NULL != mParentNode ) {
		mDistToBorder = Rect( mPosition.x, mPosition.y,
							  mParentNode->getPixelsSize().x - ( mPosition.x + mSize.x ),
							  mParentNode->getPixelsSize().y - ( mPosition.y + mSize.y ) );
	}
}

const Rectf& UIWidget::getLayoutMargin() const {
	return mLayoutMargin;
}

const Rectf& UIWidget::getLayoutPixelsMargin() const {
	return mLayoutMarginPx;
}

UIWidget* UIWidget::setLayoutMargin( const Rectf& margin ) {
	if ( mLayoutMargin != margin ) {
		mLayoutMargin = margin;
		mLayoutMarginPx = PixelDensity::dpToPx( mLayoutMargin ).ceil();
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutMarginLeft( const Float& marginLeft ) {
	if ( mLayoutMargin.Left != marginLeft ) {
		mLayoutMargin.Left = marginLeft;
		mLayoutMarginPx.Left = eeceil( PixelDensity::dpToPx( mLayoutMargin.Left ) );
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutMarginRight( const Float& marginRight ) {
	if ( mLayoutMargin.Right != marginRight ) {
		mLayoutMargin.Right = marginRight;
		mLayoutMarginPx.Right = eeceil( PixelDensity::dpToPx( mLayoutMargin.Right ) );
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutMarginTop( const Float& marginTop ) {
	if ( mLayoutMargin.Top != marginTop ) {
		mLayoutMargin.Top = marginTop;
		mLayoutMarginPx.Top = eeceil( PixelDensity::dpToPx( mLayoutMargin.Top ) );
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutMarginBottom( const Float& marginBottom ) {
	if ( mLayoutMargin.Bottom != marginBottom ) {
		mLayoutMargin.Bottom = marginBottom;
		mLayoutMarginPx.Bottom = eeceil( PixelDensity::dpToPx( mLayoutMargin.Bottom ) );
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutPixelsMargin( const Rectf& margin ) {
	if ( mLayoutMargin != margin ) {
		mLayoutMarginPx = margin;
		mLayoutMargin = PixelDensity::pxToDp( mLayoutMarginPx ).ceil();
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutPixelsMarginLeft( const Float& marginLeft ) {
	if ( mLayoutMargin.Left != marginLeft ) {
		mLayoutMarginPx.Left = marginLeft;
		mLayoutMargin.Left = eeceil( PixelDensity::pxToDp( mLayoutMarginPx.Left ) );
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutPixelsMarginRight( const Float& marginRight ) {
	if ( mLayoutMargin.Right != marginRight ) {
		mLayoutMarginPx.Right = marginRight;
		mLayoutMargin.Right = eeceil( PixelDensity::pxToDp( mLayoutMarginPx.Right ) );
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutPixelsMarginTop( const Float& marginTop ) {
	if ( mLayoutMargin.Top != marginTop ) {
		mLayoutMarginPx.Top = marginTop;
		mLayoutMargin.Top = eeceil( PixelDensity::pxToDp( mLayoutMarginPx.Top ) );
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutPixelsMarginBottom( const Float& marginBottom ) {
	if ( mLayoutMargin.Bottom != marginBottom ) {
		mLayoutMarginPx.Bottom = marginBottom;
		mLayoutMargin.Bottom = eeceil( PixelDensity::pxToDp( mLayoutMarginPx.Bottom ) );
		onMarginChange();
		notifyLayoutAttrChange();
	}

	return this;
}

Float UIWidget::getLayoutWeight() const {
	return mLayoutWeight;
}

UIWidget* UIWidget::setLayoutWeight( const Float& weight ) {
	if ( mLayoutWeight != weight ) {
		mLayoutWeight = weight;
		notifyLayoutAttrChange();
	}

	return this;
}

Uint32 UIWidget::getLayoutGravity() const {
	return mLayoutGravity;
}

UIWidget* UIWidget::setLayoutGravity( const Uint32& layoutGravity ) {
	if ( mLayoutGravity != layoutGravity ) {
		mLayoutGravity = layoutGravity;
		notifyLayoutAttrChange();
	}

	return this;
}

const SizePolicy& UIWidget::getLayoutWidthPolicy() const {
	return mWidthPolicy;
}

UIWidget* UIWidget::setLayoutWidthPolicy( const SizePolicy& widthPolicy ) {
	if ( mWidthPolicy != widthPolicy ) {
		mWidthPolicy = widthPolicy;
		if ( mWidthPolicy == SizePolicy::WrapContent )
			onAutoSize();
		notifyLayoutAttrChange();
	}

	return this;
}

const SizePolicy& UIWidget::getLayoutHeightPolicy() const {
	return mHeightPolicy;
}

UIWidget* UIWidget::setLayoutHeightPolicy( const SizePolicy& heightPolicy ) {
	if ( mHeightPolicy != heightPolicy ) {
		mHeightPolicy = heightPolicy;
		if ( mHeightPolicy == SizePolicy::WrapContent )
			onAutoSize();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutSizePolicy( const SizePolicy& widthPolicy,
										 const SizePolicy& heightPolicy ) {
	if ( mWidthPolicy != widthPolicy || mHeightPolicy != heightPolicy ) {
		mWidthPolicy = widthPolicy;
		mHeightPolicy = heightPolicy;
		if ( mWidthPolicy == SizePolicy::WrapContent || mHeightPolicy == SizePolicy::WrapContent ) {
			onAutoSize();
		}
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setLayoutPositionPolicy( const PositionPolicy& layoutPositionPolicy,
											 UIWidget* of ) {
	if ( mLayoutPositionPolicy != layoutPositionPolicy || mLayoutPositionPolicyWidget != of ) {
		mLayoutPositionPolicy = layoutPositionPolicy;
		mLayoutPositionPolicyWidget = of;
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::getLayoutPositionPolicyWidget() const {
	return mLayoutPositionPolicyWidget;
}

PositionPolicy UIWidget::getLayoutPositionPolicy() const {
	return mLayoutPositionPolicy;
}

UITooltip* UIWidget::createTooltip() {
	if ( NULL != mTooltip )
		return mTooltip;

	mTooltip = UITooltip::New();
	mTooltip->setVisible( false )->setEnabled( false );
	mTooltip->setTooltipOf( this );
	if ( !mTooltipText.empty() )
		mTooltip->setText( mTooltipText );
	return mTooltip;
}

void UIWidget::onChildCountChange( Node* child, const bool& removed ) {
	UINode::onChildCountChange( child, removed );

	if ( removed && child->isWidget() ) {
		UIWidget* widget = child->asType<UIWidget>();
		if ( widget->getUIStyle() && widget->getUIStyle()->isStructurallyVolatile() ) {
			mStyle->removeStructurallyVolatileChild( widget->asType<UIWidget>() );
		}
	}

	if ( !isSceneNodeLoading() && mUISceneNode != NULL && mStyle != NULL ) {
		// Childs that are structurally volatile can change states when a new
		// element is added to the parent. We pre-store them and invalidate its
		// state if that's the case.
		auto& svc = mStyle->getStructurallyVolatileChilds();
		for ( auto& child : svc ) {
			mUISceneNode->invalidateStyleState( child );
		}
	}
}

Uint32 UIWidget::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_TAB && isTabStop() ) {
		Node::onKeyDown( event );
		return 1;
	}
	return Node::onKeyDown( event );
}

Vector2f UIWidget::getTooltipPosition() {
	if ( NULL == getEventDispatcher() )
		return Vector2f::Zero;
	return mTooltip->getTooltipPosition( getEventDispatcher()->getMousePosf() );
}

void UIWidget::createStyle() {
	if ( NULL == mStyle && NULL != mUISceneNode ) {
		mStyle = UIStyle::New( this );
		mStyle->setState( mState );
	}
}

Uint32 UIWidget::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	EventDispatcher* eventDispatcher = getEventDispatcher();

	if ( NULL != eventDispatcher && eventDispatcher->getMouseOverNode() == this ) {
		if ( mVisible && NULL != mTooltip && !mTooltip->getText().empty() ) {
			UIThemeManager* themeManager = getUISceneNode()->getUIThemeManager();

			if ( themeManager->getTooltipFollowMouse() ) {
				mTooltip->setPixelsPosition( getTooltipPosition() );
			} else if ( !mTooltip->dontAutoHideOnMouseMove() ) {
				mTooltip->hide();
			}
		}
	}

	return UINode::onMouseMove( position, flags );
}

Uint32 UIWidget::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	EventDispatcher* eventDispatcher = getEventDispatcher();

	if ( NULL != eventDispatcher && eventDispatcher->getMouseOverNode() == this ) {
		if ( NULL != mSceneNode && mSceneNode->getDrawDebugData() ) {
			updateDebugData();
		}

		if ( mVisible && isTooltipEnabled() && !mTooltipText.empty() ) {
			UIThemeManager* themeManager = getUISceneNode()->getUIThemeManager();

			if ( NULL == themeManager )
				return UINode::onMouseOver( position, flags );

			if ( Time::Zero == themeManager->getTooltipTimeToShow() ) {
				createTooltip();
				if ( !mTooltip->isVisible() || themeManager->getTooltipFollowMouse() )
					mTooltip->setPosition( getTooltipPosition() );
				mTooltip->show();
			} else {
				runAction( Actions::Runnable::New(
					[this] {
						if ( isTooltipEnabled() &&
							 getEventDispatcher()->getMouseOverNode() == this &&
							 ( mTooltip == NULL || !mTooltip->isVisible() ) &&
							 ( !mTooltipText.empty() ||
							   ( mTooltip && !mTooltip->getText().empty() ) ) ) {
							createTooltip();
							mTooltip->setPixelsPosition( getTooltipPosition() );
							mTooltip->show();
						}
					},
					themeManager->getTooltipTimeToShow() ) );
			}

			if ( mTooltip && themeManager->getTooltipFollowMouse() ) {
				mTooltip->setPixelsPosition( getTooltipPosition() );
			}
		}
	}

	return UINode::onMouseOver( position, flags );
}

Uint32 UIWidget::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	EventDispatcher* eventDispatcher = getEventDispatcher();

	if ( NULL != eventDispatcher && eventDispatcher->getMouseOverNode() != this && mVisible &&
		 NULL != mTooltip && !mTooltip->dontAutoHideOnMouseMove() ) {
		mTooltip->hide();
	}

	return UINode::onMouseLeave( Pos, Flags );
}

UIWidget* UIWidget::setTooltipText( const String& text ) {
	mTooltipText = text;
	if ( mTooltip )
		mTooltip->setText( text );
	return this;
}

String UIWidget::getTooltipText() {
	return mTooltipText;
}

void UIWidget::tooltipRemove() {
	mTooltip = NULL;
}

UINode* UIWidget::setFlags( const Uint32& flags ) {
	if ( flags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) {
		updateAnchorsDistances();
	}

	if ( !( mFlags & UI_AUTO_SIZE ) && ( flags & UI_AUTO_SIZE ) ) {
		onAutoSize();
	}

	return UINode::setFlags( flags );
}

UINode* UIWidget::unsetFlags( const Uint32& flags ) {
	if ( flags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) {
		updateAnchorsDistances();
	}

	return UINode::unsetFlags( flags );
}

UIWidget* UIWidget::setAnchors( const Uint32& flags ) {
	mFlags &= ~( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mFlags |= flags;
	updateAnchorsDistances();
	return this;
}

void UIWidget::setTheme( UITheme* Theme ) {
	mTheme = Theme;
	invalidateDraw();
}

UINode* UIWidget::setThemeSkin( const std::string& skinName ) {
	return setThemeSkin( NULL != mTheme ? mTheme
										: getUISceneNode()->getUIThemeManager()->getDefaultTheme(),
						 skinName );
}

UINode* UIWidget::setThemeSkin( UITheme* Theme, const std::string& skinName ) {
	return UINode::setThemeSkin( Theme, skinName );
}

Node* UIWidget::setId( const std::string& id ) {
	Node::setId( id );

	if ( !isSceneNodeLoading() && !isLoadingState() ) {
		getUISceneNode()->invalidateStyle( this );
		getUISceneNode()->invalidateStyleState( this );
	}

	return this;
}

bool UIWidget::acceptsDropOfWidget( const UIWidget* ) {
	return false;
}

UIWidget* UIWidget::acceptsDropOfWidgetInTree( const UIWidget* widget ) {
	if ( acceptsDropOfWidget( widget ) ) {
		return this;
	} else {
		Node* parent = getParent();
		while ( parent ) {
			if ( parent->isType( UI_TYPE_WIDGET ) &&
				 parent->asType<UIWidget>()->acceptsDropOfWidget( widget ) ) {
				return parent->asType<UIWidget>();
			}
			parent = parent->getParent();
		}
	}
	return nullptr;
}

UITooltip* UIWidget::getTooltip() {
	return mTooltip;
}

void UIWidget::onParentSizeChange( const Vector2f& sizeChange ) {
	updateAnchors( sizeChange );
	UINode::onParentSizeChange( sizeChange );
}

void UIWidget::onPositionChange() {
	updateAnchorsDistances();
	UINode::onPositionChange();
}

void UIWidget::onVisibilityChange() {
	updateAnchorsDistances();
	notifyLayoutAttrChange();
	notifyLayoutAttrChangeParent();
	UINode::onVisibilityChange();
}

void UIWidget::onSizeChange() {
	UINode::onSizeChange();

	if ( mBorder != NULL )
		mBorder->invalidate();

	if ( mBackground != NULL )
		mBackground->invalidate();

	if ( mForeground != NULL )
		mForeground->invalidate();

	notifyLayoutAttrChange();
}

void UIWidget::onAutoSize() {}

void UIWidget::onWidgetCreated() {}

void UIWidget::notifyLayoutAttrChange() {
	if ( 0 == mAttributesTransactionCount ) {
		NodeMessage msg( this, NodeMessage::LayoutAttributeChange );
		messagePost( &msg );
	} else {
		mFlags |= UI_ATTRIBUTE_CHANGED;
	}
}

void UIWidget::notifyLayoutAttrChangeParent() {
	if ( 0 == mAttributesTransactionCount && NULL != mParentNode ) {
		NodeMessage msg( this, NodeMessage::LayoutAttributeChange );
		mParentNode->messagePost( &msg );
	}
}

void UIWidget::updateAnchors( const Vector2f& sizeChange ) {
	if ( !( mFlags & ( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) )
		return;

	Sizef newSize( getSize() );

	if ( !( mFlags & UI_ANCHOR_LEFT ) ) {
		setInternalPosition( Vector2f( mDpPos.x += sizeChange.x, mDpPos.y ) );
	}

	if ( mFlags & UI_ANCHOR_RIGHT ) {
		if ( NULL != mParentNode ) {
			newSize.x = mParentNode->getSize().getWidth() - mDpPos.x -
						PixelDensity::pxToDpI( mDistToBorder.Right );

			if ( newSize.x < mMinSize.getWidth() )
				newSize.x = mMinSize.getWidth();
		}
	}

	if ( !( mFlags & UI_ANCHOR_TOP ) ) {
		setInternalPosition( Vector2f( mDpPos.x, mDpPos.y += sizeChange.y ) );
	}

	if ( mFlags & UI_ANCHOR_BOTTOM ) {
		if ( NULL != mParentNode ) {
			newSize.y =
				mParentNode->getSize().y - mDpPos.y - PixelDensity::pxToDpI( mDistToBorder.Bottom );

			if ( newSize.y < mMinSize.getHeight() )
				newSize.y = mMinSize.getHeight();
		}
	}

	if ( newSize != getSize() )
		setSize( newSize );
}

void UIWidget::alignAgainstLayout() {
	Vector2f pos = mDpPos;

	switch ( Font::getHorizontalAlign( mLayoutGravity ) ) {
		case UI_HALIGN_CENTER:
			pos.x = ( getParent()->getSize().getWidth() - getSize().getWidth() ) / 2;
			break;
		case UI_HALIGN_RIGHT:
			pos.x = getParent()->getSize().getWidth() - getSize().getWidth() - mLayoutMargin.Right;
			break;
		case UI_HALIGN_LEFT:
			pos.x = mLayoutMargin.Left;
			break;
	}

	switch ( Font::getVerticalAlign( mLayoutGravity ) ) {
		case UI_VALIGN_CENTER:
			pos.y = ( getParent()->getSize().getHeight() - getSize().getHeight() ) / 2;
			break;
		case UI_VALIGN_BOTTOM:
			pos.y =
				getParent()->getSize().getHeight() - getSize().getHeight() - mLayoutMargin.Bottom;
			break;
		case UI_VALIGN_TOP:
			pos.y = mLayoutMargin.Top;
			break;
	}

	setPosition( pos );
}

void UIWidget::reportStyleStateChange( bool disableAnimations, bool forceReApplyStyles ) {
	if ( NULL != mStyle && !mStyle->isChangingState() ) {
		bool hasAnimDisabled = mStyle->getDisableAnimations();
		if ( disableAnimations )
			mStyle->setDisableAnimations( disableAnimations );
		if ( forceReApplyStyles )
			mStyle->setForceReapplyProperties( true );
		mStyle->onStateChange();
		if ( disableAnimations )
			mStyle->setDisableAnimations( hasAnimDisabled );
	}
}

bool UIWidget::isSceneNodeLoading() const {
	return NULL != mUISceneNode ? mUISceneNode->isLoading() : false;
}

const Rectf& UIWidget::getPadding() const {
	return mPadding;
}

const Rectf& UIWidget::getPixelsPadding() const {
	return mPaddingPx;
}

UIWidget* UIWidget::setPadding( const Rectf& padding ) {
	if ( padding != mPadding ) {
		mPadding = padding;
		mPaddingPx = PixelDensity::dpToPx( mPadding ).ceil();
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingLeft( const Float& paddingLeft ) {
	if ( paddingLeft != mPadding.Left ) {
		mPadding.Left = paddingLeft;
		mPaddingPx.Left = eeceil( PixelDensity::dpToPx( mPadding.Left ) );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingRight( const Float& paddingRight ) {
	if ( paddingRight != mPadding.Right ) {
		mPadding.Right = paddingRight;
		mPaddingPx.Right = eeceil( PixelDensity::dpToPx( mPadding.Right ) );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingTop( const Float& paddingTop ) {
	if ( paddingTop != mPadding.Top ) {
		mPadding.Top = paddingTop;
		mPaddingPx.Top = eeceil( PixelDensity::dpToPx( mPadding.Top ) );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingBottom( const Float& paddingBottom ) {
	if ( paddingBottom != mPadding.Bottom ) {
		mPadding.Bottom = paddingBottom;
		mPaddingPx.Bottom = eeceil( PixelDensity::dpToPx( mPadding.Bottom ) );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingPixels( const Rectf& padding ) {
	if ( padding != mPadding ) {
		mPaddingPx = padding;
		mPadding = PixelDensity::pxToDp( mPadding ).ceil();
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingPixelsLeft( const Float& paddingLeft ) {
	if ( paddingLeft != mPadding.Left ) {
		mPaddingPx.Left = paddingLeft;
		mPadding.Left = eeceil( PixelDensity::pxToDp( mPadding.Left ) );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingPixelsRight( const Float& paddingRight ) {
	if ( paddingRight != mPadding.Right ) {
		mPaddingPx.Right = paddingRight;
		mPadding.Right = eeceil( PixelDensity::pxToDp( mPadding.Right ) );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingPixelsTop( const Float& paddingTop ) {
	if ( paddingTop != mPadding.Top ) {
		mPaddingPx.Top = paddingTop;
		mPadding.Top = eeceil( PixelDensity::pxToDp( mPadding.Top ) );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

UIWidget* UIWidget::setPaddingPixelsBottom( const Float& paddingBottom ) {
	if ( paddingBottom != mPadding.Bottom ) {
		mPaddingPx.Bottom = paddingBottom;
		mPadding.Bottom = eeceil( PixelDensity::pxToDp( mPadding.Bottom ) );
		onAutoSize();
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

const std::string& UIWidget::getStyleSheetId() const {
	return mId;
}

const std::string& UIWidget::getStyleSheetTag() const {
	return mTag;
}

const std::vector<std::string>& UIWidget::getStyleSheetClasses() const {
	return mClasses;
}

UIWidget* UIWidget::getStyleSheetParentElement() const {
	return NULL != mParentNode && mParentNode->isWidget() ? mParentNode->asType<UIWidget>() : NULL;
}

UIWidget* UIWidget::getStyleSheetPreviousSiblingElement() const {
	return NULL != mPrev && mPrev->isWidget() ? mPrev->asType<UIWidget>() : NULL;
}

UIWidget* UIWidget::getStyleSheetNextSiblingElement() const {
	return NULL != mNext && mNext->isWidget() ? mNext->asType<UIWidget>() : NULL;
}

const std::vector<std::string>& UIWidget::getStyleSheetPseudoClasses() const {
	return mPseudoClasses;
}

void UIWidget::updatePseudoClasses() {
	mPseudoClasses.clear();

	if ( mState & UIState::StateFlagHover )
		mPseudoClasses.push_back( "hover" );

	if ( mState & UIState::StateFlagFocus )
		mPseudoClasses.push_back( "focus" );

	if ( mState & UIState::StateFlagFocusWithin )
		mPseudoClasses.push_back( "focus-within" );

	if ( mState & UIState::StateFlagSelected )
		mPseudoClasses.push_back( "selected" );

	if ( mState & UIState::StateFlagPressed )
		mPseudoClasses.push_back( "pressed" );

	if ( mState & UIState::StateFlagDisabled )
		mPseudoClasses.push_back( "disabled" );

	invalidateDraw();
}

UIWidget* UIWidget::resetClass() {
	if ( !mClasses.empty() ) {
		mClasses.clear();
		if ( !isSceneNodeLoading() && !isLoadingState() ) {
			getUISceneNode()->invalidateStyle( this );
			getUISceneNode()->invalidateStyleState( this );
		}

		onClassChange();
	}
	return this;
}

UIWidget* UIWidget::setClass( const std::string& cls ) {
	if ( mClasses.size() != 1 || mClasses[0] != cls ) {
		mClasses.clear();
		mClasses.push_back( cls );

		if ( !isSceneNodeLoading() && !isLoadingState() ) {
			getUISceneNode()->invalidateStyle( this );
			getUISceneNode()->invalidateStyleState( this );
		}

		onClassChange();
	}
	return this;
}

UIWidget* UIWidget::setClasses( const std::vector<std::string>& classes ) {
	if ( mClasses != classes ) {
		mClasses = classes;

		if ( !isSceneNodeLoading() && !isLoadingState() ) {
			getUISceneNode()->invalidateStyle( this );
			getUISceneNode()->invalidateStyleState( this );
		}

		onClassChange();
	}
	return this;
}

UIWidget* UIWidget::addClass( const std::string& cls ) {
	if ( !cls.empty() && !hasClass( cls ) ) {
		mClasses.push_back( cls );

		if ( !isSceneNodeLoading() && !isLoadingState() ) {
			getUISceneNode()->invalidateStyle( this );
			getUISceneNode()->invalidateStyleState( this );
		}

		onClassChange();
	}
	return this;
}

UIWidget* UIWidget::addClasses( const std::vector<std::string>& classes ) {
	if ( !classes.empty() ) {
		for ( auto cit = classes.begin(); cit != classes.end(); ++cit ) {
			const std::string& cls = *cit;

			if ( !cls.empty() && !hasClass( cls ) ) {
				mClasses.push_back( cls );
			}
		}

		if ( !isSceneNodeLoading() && !isLoadingState() ) {
			getUISceneNode()->invalidateStyle( this );
			getUISceneNode()->invalidateStyleState( this );
		}

		onClassChange();
	}
	return this;
}

UIWidget* UIWidget::removeClass( const std::string& cls ) {
	if ( hasClass( cls ) ) {
		mClasses.erase( std::find( mClasses.begin(), mClasses.end(), cls ) );

		if ( !isSceneNodeLoading() && !isLoadingState() ) {
			getUISceneNode()->invalidateStyle( this );
			getUISceneNode()->invalidateStyleState( this );
		}

		onClassChange();
	}
	return this;
}

UIWidget* UIWidget::removeClasses( const std::vector<std::string>& classes ) {
	if ( !classes.empty() ) {
		for ( auto cit = classes.begin(); cit != classes.end(); ++cit ) {
			const std::string& cls = *cit;

			if ( !cls.empty() ) {
				auto found = std::find( mClasses.begin(), mClasses.end(), cls );

				if ( found != mClasses.end() ) {
					mClasses.erase( found );
				}
			}
		}

		if ( !isSceneNodeLoading() && !isLoadingState() ) {
			getUISceneNode()->invalidateStyle( this );
			getUISceneNode()->invalidateStyleState( this );
		}

		onClassChange();
	}
	return this;
}

bool UIWidget::hasClass( const std::string& cls ) const {
	return std::find( mClasses.begin(), mClasses.end(), cls ) != mClasses.end();
}

void UIWidget::toggleClass( const std::string& cls ) {
	if ( hasClass( cls ) ) {
		removeClass( cls );
	} else {
		addClass( cls );
	}
}

bool UIWidget::hasPseudoClass( const std::string& pseudoCls ) const {
	return std::find( mPseudoClasses.begin(), mPseudoClasses.end(), pseudoCls ) !=
		   mPseudoClasses.end();
}

bool UIWidget::isTooltipEnabled() const {
	return ( mFlags & UI_TOOLTIP_ENABLED ) != 0;
}

void UIWidget::setTooltipEnabled( bool enabled ) {
	writeFlag( UI_TOOLTIP_ENABLED, enabled ? 1 : 0 );
	if ( mTooltip && mTooltip->isVisible() )
		mTooltip->setVisible( false );
}

void UIWidget::setElementTag( const std::string& tag ) {
	if ( mTag != tag ) {
		mTag = tag;
		// Some rules are going to be invalidated if the tag is changed
		mMinWidthEq = "";
		mMinHeightEq = "";
		mMinSize = Sizef::Zero;

		if ( !isSceneNodeLoading() && !isLoadingState() ) {
			getUISceneNode()->invalidateStyle( this );
			getUISceneNode()->invalidateStyleState( this );
		}

		onTagChange();
	}
}

const std::vector<std::string> UIWidget::getClasses() const {
	return mClasses;
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
			if ( !( State == UIState::StateHover && !isMouseOverMeOrChilds() ) ) {
				updatePseudoClasses();
				mStyle->pushState( State );
			}
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
			if ( !( State == UIState::StateHover && isMouseOverMeOrChilds() ) ) {
				updatePseudoClasses();
				mStyle->popState( State );
			}
		}

		if ( emitEvent ) {
			onStateChange();
		} else {
			invalidateDraw();
		}
	}
}

Uint32 UIWidget::onFocus( NodeFocusReason reason ) {
	pushState( UIState::StateFocusWithin );

	Node* parent = mParentNode;
	while ( parent ) {
		if ( parent->isUINode() )
			parent->asType<UINode>()->pushState( UIState::StateFocusWithin );
		parent = parent->getParent();
	}

	return UINode::onFocus( reason );
}

Uint32 UIWidget::onFocusLoss() {
	popState( UIState::StateFocusWithin );

	Node* parent = mParentNode;
	while ( parent ) {
		if ( parent->isUINode() )
			parent->asType<UINode>()->popState( UIState::StateFocusWithin );
		parent = parent->getParent();
	}

	return UINode::onFocusLoss();
}

UIStyle* UIWidget::getUIStyle() const {
	return mStyle;
}

void UIWidget::reloadStyle( const bool& reloadChilds, const bool& disableAnimations,
							const bool& reportStateChange, const bool& forceReApplyProperties ) {
	createStyle();

	if ( NULL != mStyle ) {
		mStyle->load();

		if ( NULL != getFirstChild() && reloadChilds ) {
			Node* child = getFirstChild();

			while ( NULL != child ) {
				if ( child->isWidget() )
					child->asType<UIWidget>()->reloadStyle( reloadChilds, disableAnimations,
															reportStateChange,
															forceReApplyProperties );

				child = child->getNextNode();
			}
		}

		if ( reportStateChange )
			reportStyleStateChange( disableAnimations, forceReApplyProperties );
	}
}

void UIWidget::onPaddingChange() {
	sendCommonEvent( Event::OnPaddingChange );
	invalidateDraw();
}

void UIWidget::onMarginChange() {
	sendCommonEvent( Event::OnMarginChange );
	invalidateDraw();
}

void UIWidget::onThemeLoaded() {}

void UIWidget::onParentChange() {
	if ( !isSceneNodeLoading() && !isLoadingState() ) {
		getUISceneNode()->invalidateStyle( this, true );
		getUISceneNode()->invalidateStyleState( this, true, true );
	}
}

void UIWidget::onClassChange() {
	sendCommonEvent( Event::OnClassChange );
}

void UIWidget::onTagChange() {
	sendCommonEvent( Event::OnTagChange );
}

void UIWidget::beginAttributesTransaction() {
	mAttributesTransactionCount++;
}

void UIWidget::endAttributesTransaction() {
	mAttributesTransactionCount--;

	if ( 0 == mAttributesTransactionCount ) {
		if ( mFlags & UI_ATTRIBUTE_CHANGED ) {
			notifyLayoutAttrChange();

			mFlags &= ~UI_ATTRIBUTE_CHANGED;
		}
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

	if ( !isClosing() && hasClass( className ) ) {
		widgets.push_back( this );
	}

	Node* child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() ) {
			std::vector<UIWidget*> foundWidgets =
				child->asType<UIWidget>()->findAllByClass( className );

			if ( !foundWidgets.empty() )
				widgets.insert( widgets.end(), foundWidgets.begin(), foundWidgets.end() );
		}

		child = child->getNextNode();
	}

	return widgets;
}

std::vector<UIWidget*> UIWidget::findAllByTag( const std::string& tag ) {
	std::vector<UIWidget*> widgets;

	if ( !isClosing() && getElementTag() == tag ) {
		widgets.push_back( this );
	}

	Node* child = mChild;

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
	if ( !isClosing() && hasClass( className ) ) {
		return this;
	} else {
		Node* child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				UIWidget* foundWidget = child->asType<UIWidget>()->findByClass( className );

				if ( NULL != foundWidget )
					return foundWidget;
			}

			child = child->getNextNode();
		}
	}

	return NULL;
}

UIWidget* UIWidget::findByTag( const std::string& tag ) {
	if ( !isClosing() && getElementTag() == tag ) {
		return this;
	} else {
		Node* child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				UIWidget* foundWidget = child->asType<UIWidget>()->findByTag( tag );

				if ( NULL != foundWidget )
					return foundWidget;
			}

			child = child->getNextNode();
		}
	}

	return NULL;
}

UIWidget* UIWidget::querySelector( const CSS::StyleSheetSelector& selector ) {
	if ( !isClosing() && selector.select( this ) ) {
		return this;
	} else {
		Node* child = mChild;

		while ( NULL != child ) {
			if ( child->isWidget() ) {
				UIWidget* foundWidget = child->asType<UIWidget>()->querySelector( selector );

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

	if ( !isClosing() && selector.select( this ) ) {
		widgets.push_back( this );
	}

	Node* child = mChild;

	while ( NULL != child ) {
		if ( child->isWidget() ) {
			std::vector<UIWidget*> foundWidgets =
				child->asType<UIWidget>()->querySelectorAll( selector );

			if ( !foundWidgets.empty() )
				widgets.insert( widgets.end(), foundWidgets.begin(), foundWidgets.end() );
		}

		child = child->getNextNode();
	}

	return widgets;
}

bool UIWidget::checkPropertyDefinition( const StyleSheetProperty& property ) {
	if ( property.getPropertyDefinition() == NULL ) {
		Log::warning( "applyProperty: Property \"%s\" not defined!", property.getName().c_str() );
		return false;
	}
	return true;
}

void UIWidget::reportStyleStateChangeRecursive( bool disableAnimations, bool forceReApplyStyles ) {
	Node* childLoop = getFirstChild();
	while ( childLoop != NULL ) {
		if ( childLoop->isWidget() )
			childLoop->asType<UIWidget>()->reportStyleStateChangeRecursive( disableAnimations,
																			forceReApplyStyles );
		childLoop = childLoop->getNextNode();
	}
	reportStyleStateChange( disableAnimations, forceReApplyStyles );
}

UIWidget* UIWidget::querySelector( const std::string& selector ) {
	return querySelector( CSS::StyleSheetSelector( selector ) );
}

std::vector<UIWidget*> UIWidget::querySelectorAll( const std::string& selector ) {
	return querySelectorAll( CSS::StyleSheetSelector( selector ) );
}

std::vector<PropertyId> UIWidget::getPropertiesImplemented() const {
	return { PropertyId::X,
			 PropertyId::Y,
			 PropertyId::Width,
			 PropertyId::Height,
			 PropertyId::MarginLeft,
			 PropertyId::MarginTop,
			 PropertyId::MarginRight,
			 PropertyId::MarginBottom,
			 PropertyId::PaddingLeft,
			 PropertyId::PaddingTop,
			 PropertyId::PaddingRight,
			 PropertyId::PaddingBottom,
			 PropertyId::BackgroundColor,
			 PropertyId::BackgroundTint,
			 PropertyId::ForegroundColor,
			 PropertyId::ForegroundTint,
			 PropertyId::ForegroundRadius,
			 PropertyId::BorderType,
			 PropertyId::SkinColor,
			 PropertyId::Rotation,
			 PropertyId::Scale,
			 PropertyId::Opacity,
			 PropertyId::Cursor,
			 PropertyId::Visible,
			 PropertyId::Enabled,
			 PropertyId::Theme,
			 PropertyId::Skin,
			 PropertyId::Flags,
			 PropertyId::BackgroundSize,
			 PropertyId::ForegroundSize,
			 PropertyId::LayoutWeight,
			 PropertyId::LayoutGravity,
			 PropertyId::LayoutWidth,
			 PropertyId::LayoutHeight,
			 PropertyId::Clip,
			 PropertyId::BackgroundPositionX,
			 PropertyId::BackgroundPositionY,
			 PropertyId::ForegroundPositionX,
			 PropertyId::ForegroundPositionY,
			 PropertyId::RotationOriginPointX,
			 PropertyId::RotationOriginPointY,
			 PropertyId::ScaleOriginPointX,
			 PropertyId::ScaleOriginPointY,
			 PropertyId::BlendMode,
			 PropertyId::MinWidth,
			 PropertyId::MaxWidth,
			 PropertyId::MinHeight,
			 PropertyId::MaxHeight,
			 PropertyId::BorderLeftColor,
			 PropertyId::BorderTopColor,
			 PropertyId::BorderRightColor,
			 PropertyId::BorderBottomColor,
			 PropertyId::BorderLeftWidth,
			 PropertyId::BorderTopWidth,
			 PropertyId::BorderRightWidth,
			 PropertyId::BorderBottomWidth,
			 PropertyId::BorderTopLeftRadius,
			 PropertyId::BorderTopRightRadius,
			 PropertyId::BorderBottomLeftRadius,
			 PropertyId::BorderBottomRightRadius,
			 PropertyId::BorderSmooth,
			 PropertyId::BackgroundSmooth,
			 PropertyId::Focusable,
			 PropertyId::ForegroundSmooth };
}

std::string UIWidget::getPropertyString( const std::string& property ) const {
	return getPropertyString( StyleSheetSpecification::instance()->getProperty( property ) );
}

std::string UIWidget::getPropertyString( const PropertyDefinition* propertyDef,
										 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

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
			return String::fromFloat( getLayoutMargin().Left, "dp" );
		case PropertyId::MarginTop:
			return String::fromFloat( getLayoutMargin().Top, "dp" );
		case PropertyId::MarginRight:
			return String::fromFloat( getLayoutMargin().Right, "dp" );
		case PropertyId::MarginBottom:
			return String::fromFloat( getLayoutMargin().Bottom, "dp" );
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
		case PropertyId::BackgroundTint:
			return getBackgroundTint( propertyIndex ).toHexString();
		case PropertyId::ForegroundColor:
			return getForegroundColor().toHexString();
		case PropertyId::ForegroundTint:
			return getForegroundTint( propertyIndex ).toHexString();
		case PropertyId::ForegroundRadius:
			return String::toString( getForegroundRadius() );
		case PropertyId::BorderType:
			return Borders::fromBorderType( setBorderEnabled( true )->getBorderType() );
		case PropertyId::SkinColor:
			return getSkinColor().toHexString();
		case PropertyId::Rotation:
			return String::fromFloat( getRotation() );
		case PropertyId::Scale:
			return String::fromFloat( getScale().x ) + " " + String::fromFloat( getScale().y );
		case PropertyId::Opacity:
			return String::fromFloat( getAlpha() / 255.f );
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
			return getBackground()->getLayer( propertyIndex )->getSizeEq();
		case PropertyId::ForegroundSize:
			return getForeground()->getLayer( propertyIndex )->getSizeEq();
		case PropertyId::LayoutWeight:
			return String::fromFloat( getLayoutWeight() );
		case PropertyId::LayoutGravity:
			return getLayoutGravityString();
		case PropertyId::LayoutWidth:
			return getLayoutWidthPolicyString();
		case PropertyId::LayoutHeight:
			return getLayoutHeightPolicyString();
		case PropertyId::Clip:
			return UIClip::toString( mClip.getClipType() );
		case PropertyId::BackgroundPositionX:
			return getBackground()->getLayer( propertyIndex )->getPositionX();
		case PropertyId::BackgroundPositionY:
			return getBackground()->getLayer( propertyIndex )->getPositionY();
		case PropertyId::ForegroundPositionX:
			return getForeground()->getLayer( propertyIndex )->getPositionX();
		case PropertyId::ForegroundPositionY:
			return getForeground()->getLayer( propertyIndex )->getPositionY();
		case PropertyId::RotationOriginPointX:
			return String::fromFloat( getRotationOriginPoint().x, "px" );
		case PropertyId::RotationOriginPointY:
			return String::fromFloat( getRotationOriginPoint().y, "px" );
		case PropertyId::ScaleOriginPointX:
			return String::fromFloat( getScaleOriginPoint().x, "px" );
		case PropertyId::ScaleOriginPointY:
			return String::fromFloat( getScaleOriginPoint().y, "px" );
		case PropertyId::BlendMode:
			return "";
		case PropertyId::MinWidth:
			return mMinWidthEq;
		case PropertyId::MaxWidth:
			return mMaxWidthEq;
		case PropertyId::MinHeight:
			return mMinHeightEq;
		case PropertyId::MaxHeight:
			return mMaxHeightEq;
		case PropertyId::BorderLeftColor:
			return setBorderEnabled( true )->getColorLeft().toHexString();
		case PropertyId::BorderRightColor:
			return setBorderEnabled( true )->getColorRight().toHexString();
		case PropertyId::BorderTopColor:
			return setBorderEnabled( true )->getColorTop().toHexString();
		case PropertyId::BorderBottomColor:
			return setBorderEnabled( true )->getColorBottom().toHexString();
		case PropertyId::BorderLeftWidth:
			return String::fromFloat( setBorderEnabled( true )->getBorders().left.width, "px" );
		case PropertyId::BorderRightWidth:
			return String::fromFloat( setBorderEnabled( true )->getBorders().right.width, "px" );
		case PropertyId::BorderTopWidth:
			return String::fromFloat( setBorderEnabled( true )->getBorders().top.width, "px" );
		case PropertyId::BorderBottomWidth:
			return String::fromFloat( setBorderEnabled( true )->getBorders().bottom.width, "px" );
		case PropertyId::BorderTopLeftRadius:
			return String::format(
				"%s %s",
				String::fromFloat( setBorderEnabled( true )->getBorders().radius.topLeft.x, "px" ),
				String::fromFloat( getBorder()->getBorders().radius.topLeft.y, "px" ) );
		case PropertyId::BorderTopRightRadius:
			return String::format(
				"%s %s",
				String::fromFloat( setBorderEnabled( true )->getBorders().radius.topRight.x, "px" ),
				String::fromFloat( getBorder()->getBorders().radius.topRight.y, "px" ) );
		case PropertyId::BorderBottomLeftRadius:
			return String::format(
				"%s %s",
				String::fromFloat( setBorderEnabled( true )->getBorders().radius.bottomLeft.x,
								   "px" ),
				String::fromFloat( getBorder()->getBorders().radius.bottomLeft.y, "px" ) );
		case PropertyId::BorderBottomRightRadius:
			return String::format(
				"%s %s",
				String::fromFloat( setBorderEnabled( true )->getBorders().radius.bottomRight.x,
								   "px" ),
				String::fromFloat( getBorder()->getBorders().radius.bottomRight.y, "px" ) );
		case PropertyId::BorderSmooth:
			return mBorder ? ( mBorder->isSmooth() ? "true" : "false" ) : "false";
		case PropertyId::BackgroundSmooth:
			return mBackground
					   ? ( mBackground->getBackgroundDrawable().isSmooth() ? "true" : "false" )
					   : "false";
		case PropertyId::ForegroundSmooth:
			return mBackground
					   ? ( mForeground->getBackgroundDrawable().isSmooth() ? "true" : "false" )
					   : "false";
		case PropertyId::Focusable:
			return isTabFocusable() ? "true" : "false";
		default:
			break;
	}

	return "";
}

void UIWidget::setStyleSheetInlineProperty( const std::string& name, const std::string& value,
											const Uint32& specificity ) {
	if ( mStyle != NULL )
		mStyle->setStyleSheetProperty(
			CSS::StyleSheetProperty( name, value, specificity, false, 0 ) );
}

bool UIWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;
	bool attributeSet = true;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Id:
			setId( attribute.value() );
			break;
		case PropertyId::Class:
			addClasses( String::split( attribute.getValue(), ' ' ) );
			break;
		case PropertyId::X:
			setLayoutWidthPolicy( SizePolicy::Fixed );
			setInternalPosition(
				Vector2f( eefloor( lengthFromValueAsDp( attribute ) ), mDpPos.y ) );
			notifyLayoutAttrChange();
			break;
		case PropertyId::Y:
			setLayoutWidthPolicy( SizePolicy::Fixed );
			setInternalPosition(
				Vector2f( mDpPos.x, eefloor( lengthFromValueAsDp( attribute ) ) ) );
			notifyLayoutAttrChange();
			break;
		case PropertyId::Width:
			setLayoutWidthPolicy( SizePolicy::Fixed );
			setSize( eefloor( lengthFromValueAsDp( attribute ) ), getSize().getHeight() );
			notifyLayoutAttrChange();
			break;
		case PropertyId::Height:
			setLayoutHeightPolicy( SizePolicy::Fixed );
			setSize( getSize().getWidth(), eefloor( lengthFromValueAsDp( attribute ) ) );
			notifyLayoutAttrChange();
			break;
		case PropertyId::BackgroundColor:
			setBackgroundColor( attribute.asColor() );
			break;
		case PropertyId::BackgroundTint:
			setBackgroundTint( attribute.asColor(), attribute.getIndex() );
			break;
		case PropertyId::BackgroundImage:
			setBackgroundDrawable( attribute.getValue(), attribute.getIndex() );
			break;
		case PropertyId::BackgroundRepeat:
			setBackgroundRepeat( attribute.value(), attribute.getIndex() );
			break;
		case PropertyId::BackgroundSize:
			setBackgroundSize( attribute.value(), attribute.getIndex() );
			break;
		case PropertyId::ForegroundColor:
			setForegroundColor( attribute.asColor() );
			break;
		case PropertyId::ForegroundTint:
			setForegroundTint( attribute.asColor(), attribute.getIndex() );
			break;
		case PropertyId::ForegroundImage:
			setForegroundDrawable( attribute.getValue(), attribute.getIndex() );
			break;
		case PropertyId::ForegroundRadius:
			setForegroundRadius( lengthFromValue( attribute ) );
			break;
		case PropertyId::ForegroundSize:
			setForegroundSize( attribute.value(), attribute.getIndex() );
			break;
		case PropertyId::BorderType:
			setBorderEnabled( true )->setBorderType(
				Borders::toBorderType( attribute.getValue() ) );
			break;
		case PropertyId::Visible:
			setVisible( attribute.asBool() );
			break;
		case PropertyId::Enabled:
			setEnabled( attribute.asBool() );
			break;
		case PropertyId::Theme:
			setThemeByName( attribute.value() );
			if ( !mSkinName.empty() )
				setThemeSkin( mSkinName );
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
		case PropertyId::Gravity: {
			std::string gravity = attribute.asString();
			String::toLowerInPlace( gravity );
			std::vector<std::string> strings = String::split( gravity, '|' );

			if ( strings.empty() ) {
				strings = String::split( gravity, ' ' );
			}

			if ( !strings.empty() ) {
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
		case PropertyId::Flags: {
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
						setClipType( ClipType::ContentBox );
					} else if ( "multiselect" == cur ) {
						setFlags( UI_MULTI_SELECT );
					} else if ( "auto_padding" == cur || "autopadding" == cur ) {
						setFlags( UI_AUTO_PADDING );
						notifyLayoutAttrChange();
					} else if ( "reportsizechangetochilds" == cur ||
								"report_size_change_to_childs" == cur ) {
						enableReportSizeChangeToChilds();
					}
				}
			}
			break;
		}
		case PropertyId::MarginLeft:
			setLayoutMarginLeft( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::MarginRight:
			setLayoutMarginRight( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::MarginTop:
			setLayoutMarginTop( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::MarginBottom:
			setLayoutMarginBottom( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::Tooltip: {
			String text = getTranslatorString( attribute.value() );
			setTooltipText( text );
			if ( NULL != mTooltip )
				mTooltip->setStringBuffer( text );
			break;
		}
		case PropertyId::LayoutWeight:
			setLayoutWeight( attribute.asFloat() );
			break;
		case PropertyId::LayoutGravity: {
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
		case PropertyId::LayoutWidth: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "match_parent" == val || "match-parent" == val || "mp" == val ) {
				setLayoutWidthPolicy( SizePolicy::MatchParent );
			} else if ( "wrap_content" == val || "wrap-content" == val || "wc" == val ) {
				setLayoutWidthPolicy( SizePolicy::WrapContent );
			} else if ( "fixed" == val ) {
				setLayoutWidthPolicy( SizePolicy::Fixed );
				unsetFlags( UI_AUTO_SIZE );
			} else {
				unsetFlags( UI_AUTO_SIZE );
				setLayoutWidthPolicy( SizePolicy::Fixed );
				Float newVal = eefloor( lengthFromValueAsDp( attribute ) );
				if ( !( newVal == 0 && getLayoutWeight() != 0 &&
						getParent()->isType( UI_TYPE_LINEAR_LAYOUT ) ) ) {
					setInternalWidth( newVal );
					onSizeChange();
				}
			}
			break;
		}
		case PropertyId::LayoutHeight: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "match_parent" == val || "match-parent" == val || "mp" == val ) {
				setLayoutHeightPolicy( SizePolicy::MatchParent );
			} else if ( "wrap_content" == val || "wrap-content" == val || "wc" == val ) {
				setLayoutHeightPolicy( SizePolicy::WrapContent );
			} else if ( "fixed" == val ) {
				setLayoutHeightPolicy( SizePolicy::Fixed );
				unsetFlags( UI_AUTO_SIZE );
			} else {
				unsetFlags( UI_AUTO_SIZE );
				setLayoutHeightPolicy( SizePolicy::Fixed );
				Float newVal = eefloor( lengthFromValueAsDp( attribute ) );
				if ( !( newVal == 0 && getLayoutWeight() != 0 &&
						getParent()->isType( UI_TYPE_LINEAR_LAYOUT ) ) ) {
					setInternalHeight( newVal );
					onSizeChange();
				}
			}
			break;
		}
		case PropertyId::LayoutToBottomOf:
		case PropertyId::LayoutToLeftOf:
		case PropertyId::LayoutToRightOf:
		case PropertyId::LayoutToTopOf: {
			PositionPolicy rule = PositionPolicy::None;
			PropertyId layoutId =
				static_cast<PropertyId>( attribute.getPropertyDefinition()->getId() );
			if ( layoutId == PropertyId::LayoutToLeftOf )
				rule = PositionPolicy::LeftOf;
			else if ( layoutId == PropertyId::LayoutToRightOf )
				rule = PositionPolicy::RightOf;
			else if ( layoutId == PropertyId::LayoutToTopOf )
				rule = PositionPolicy::TopOf;
			else if ( layoutId == PropertyId::LayoutToBottomOf )
				rule = PositionPolicy::BottomOf;
			const std::string& id = attribute.value();
			Node* node = getParent()->find( id );
			if ( NULL != node && node->isWidget() ) {
				UIWidget* widget = static_cast<UIWidget*>( node );
				setLayoutPositionPolicy( rule, widget );
			}
			break;
		}
		case PropertyId::Clip:
			setClipType( UIClip::fromString( attribute.asString() ) );
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
			setPaddingLeft( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::PaddingRight:
			setPaddingRight( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::PaddingTop:
			setPaddingTop( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::PaddingBottom:
			setPaddingBottom( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::Opacity: {
			Float alpha = eemin( attribute.asFloat() * 255.f, 255.f );
			setAlpha( alpha );
			setChildsAlpha( alpha );
			break;
		}
		case PropertyId::Cursor:
			mSceneNode->setCursor( Cursor::fromName( attribute.getValue() ) );
			break;
		case PropertyId::BackgroundPositionX:
			setBackgroundPositionX( attribute.value(), attribute.getIndex() );
			break;
		case PropertyId::BackgroundPositionY:
			setBackgroundPositionY( attribute.value(), attribute.getIndex() );
			break;
		case PropertyId::ForegroundPositionX:
			setForegroundPositionX( attribute.value(), attribute.getIndex() );
			break;
		case PropertyId::ForegroundPositionY:
			setForegroundPositionY( attribute.value(), attribute.getIndex() );
			break;
		case PropertyId::RotationOriginPointX:
			setRotationOriginPointX( attribute.value() );
			break;
		case PropertyId::RotationOriginPointY:
			setRotationOriginPointY( attribute.value() );
			break;
		case PropertyId::ScaleOriginPointX:
			setScaleOriginPointX( attribute.value() );
			break;
		case PropertyId::ScaleOriginPointY:
			setScaleOriginPointY( attribute.value() );
			break;
		case PropertyId::MinWidth:
			setMinWidthEq( attribute.getValue() );
			break;
		case PropertyId::MaxWidth:
			setMaxWidthEq( attribute.getValue() );
			break;
		case PropertyId::MinHeight:
			setMinHeightEq( attribute.getValue() );
			break;
		case PropertyId::MaxHeight:
			setMaxHeightEq( attribute.getValue() );
			break;
		case PropertyId::BorderLeftColor:
			setBorderEnabled( true )->setColorLeft( attribute.asColor() );
			invalidateDraw();
			break;
		case PropertyId::BorderRightColor:
			setBorderEnabled( true )->setColorRight( attribute.asColor() );
			break;
		case PropertyId::BorderTopColor:
			setBorderEnabled( true )->setColorTop( attribute.asColor() );
			break;
		case PropertyId::BorderBottomColor:
			setBorderEnabled( true )->setColorBottom( attribute.asColor() );
			break;
		case PropertyId::BorderLeftWidth:
			setBorderEnabled( true )->setLeftWidth( attribute.value() );
			break;
		case PropertyId::BorderRightWidth:
			setBorderEnabled( true )->setRightWidth( attribute.value() );
			break;
		case PropertyId::BorderTopWidth:
			setBorderEnabled( true )->setTopWidth( attribute.value() );
			break;
		case PropertyId::BorderBottomWidth:
			setBorderEnabled( true )->setBottomWidth( attribute.value() );
			break;
		case PropertyId::BorderTopLeftRadius:
			setTopLeftRadius( attribute.value() );
			break;
		case PropertyId::BorderBottomLeftRadius:
			setBottomLeftRadius( attribute.value() );
			break;
		case PropertyId::BorderTopRightRadius:
			setTopRightRadius( attribute.value() );
			break;
		case PropertyId::BorderBottomRightRadius:
			setBottomRightRadius( attribute.value() );
			break;
		case PropertyId::BorderSmooth:
			setBorderEnabled( true )->setSmooth( attribute.asBool() );
			break;
		case PropertyId::BackgroundSmooth:
			setBackgroundFillEnabled( true )->setSmooth( attribute.asBool() );
			break;
		case PropertyId::ForegroundSmooth:
			setForegroundFillEnabled( true )->setSmooth( attribute.asBool() );
			break;
		case PropertyId::Focusable:
			if ( attribute.asBool() ) {
				setFlags( UI_TAB_FOCUSABLE );
			} else {
				unsetFlags( UI_TAB_FOCUSABLE );
			}
			break;
		default:
			attributeSet = false;
			break;
	}

	return attributeSet;
}

void UIWidget::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	for ( pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end();
		  ++ait ) {
		// Create a property without triming its value
		StyleSheetProperty prop( ait->name(), ait->value(), false,
								 StyleSheetSelectorRule::SpecificityInline );

		if ( prop.getShorthandDefinition() != NULL ) {
			auto properties = prop.getShorthandDefinition()->parse( ait->value() );

			for ( auto& property : properties ) {
				if ( NULL != mStyle )
					mStyle->setStyleSheetProperty( property );
				applyProperty( property );
			}
		} else {
			if ( NULL != mStyle )
				mStyle->setStyleSheetProperty( prop );
			applyProperty( prop );
		}
	}

	endAttributesTransaction();
}

std::string UIWidget::getLayoutWidthPolicyString() const {
	SizePolicy rules = getLayoutWidthPolicy();

	if ( rules == SizePolicy::MatchParent )
		return "match_parent";
	else if ( rules == SizePolicy::WrapContent )
		return "wrap_content";
	return String::fromFloat( getSize().getHeight(), "dp" );
}

std::string UIWidget::getLayoutHeightPolicyString() const {
	SizePolicy rules = getLayoutHeightPolicy();

	if ( rules == SizePolicy::MatchParent )
		return "match_parent";
	else if ( rules == SizePolicy::WrapContent )
		return "wrap_content";
	return String::fromFloat( getSize().getHeight(), "dp" );
}

static std::string getGravityStringFromUint( const Uint32& gravity ) {
	std::vector<std::string> gravec;

	if ( Font::getHorizontalAlign( gravity ) == UI_HALIGN_RIGHT ) {
		gravec.push_back( "right" );
	} else if ( Font::getHorizontalAlign( gravity ) == UI_HALIGN_CENTER ) {
		gravec.push_back( "center_horizontal" );
	} else {
		gravec.push_back( "left" );
	}

	if ( Font::getVerticalAlign( gravity ) == UI_VALIGN_BOTTOM ) {
		gravec.push_back( "bottom" );
	} else if ( Font::getVerticalAlign( gravity ) == UI_VALIGN_CENTER ) {
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

	if ( mFlags & UI_AUTO_SIZE )
		flagvec.push_back( "autosize" );
	if ( mFlags & UI_MULTI_SELECT )
		flagvec.push_back( "multiselect" );
	if ( mFlags & UI_AUTO_PADDING )
		flagvec.push_back( "autopadding" );
	if ( reportSizeChangeToChilds() )
		flagvec.push_back( "reportsizechangetochilds" );
	if ( isClipped() )
		flagvec.push_back( "clip" );

	return String::join( flagvec, '|' );
}

void UIWidget::enableCSSAnimations() {
	if ( NULL != mStyle )
		mStyle->setDisableAnimations( false );
}

void UIWidget::disableCSSAnimations() {
	if ( NULL != mStyle )
		mStyle->setDisableAnimations( true );
}

void UIWidget::reloadFontFamily() {
	if ( NULL != mStyle )
		mStyle->reloadFontFamily();
	Node* child = getFirstChild();
	while ( NULL != child ) {
		if ( child->isWidget() )
			child->asType<UIWidget>()->reloadFontFamily();
		child = child->getNextNode();
	}
}

bool UIWidget::isTabStop() const {
	return ( mFlags & UI_TAB_STOP ) != 0;
}

void UIWidget::setTabStop() {
	mFlags |= UI_TAB_STOP;
}

void UIWidget::unsetTabStop() {
	mFlags &= ~UI_TAB_STOP;
}

bool UIWidget::isTabFocusable() const {
	return ( mFlags & UI_TAB_FOCUSABLE ) != 0;
}

void UIWidget::setTabFocusable() {
	mFlags |= UI_TAB_FOCUSABLE;
}

void UIWidget::unsetTabFocusable() {
	mFlags &= ~UI_TAB_FOCUSABLE;
}

UIWidget* UIWidget::getPrevWidget() const {
	UIWidget* found = NULL;
	UIWidget* possible = NULL;
	Node* child = mChildLast;
	while ( NULL != child ) {
		if ( child->isVisible() && child->isEnabled() && child->isWidget() ) {
			possible = child->asType<UIWidget>();
			if ( possible->isTabFocusable() ) {
				return possible;
			}
			found = possible->getNextWidget();
			if ( found ) {
				return found;
			}
		}
		child = child->getPrevNode();
	}
	return NULL;
}

UIWidget* UIWidget::getNextWidget() const {
	UIWidget* found = NULL;
	UIWidget* possible = NULL;
	Node* child = mChild;
	while ( NULL != child ) {
		if ( child->isVisible() && child->isEnabled() && child->isWidget() ) {
			possible = child->asType<UIWidget>();
			if ( possible->isTabFocusable() ) {
				return possible;
			}
			found = possible->getNextWidget();
			if ( found ) {
				return found;
			}
		}
		child = child->getNextNode();
	}
	return NULL;
}

String UIWidget::getTranslatorString( const std::string& str ) {
	return getUISceneNode() != nullptr ? getUISceneNode()->getTranslatorString( str ) : String();
}

String UIWidget::getTranslatorString( const std::string& str, const String& defaultValue ) {
	return getUISceneNode() != nullptr ? getUISceneNode()->getTranslatorString( str, defaultValue )
									   : defaultValue;
}

String UIWidget::i18n( const std::string& str ) {
	return getTranslatorString( str );
}

String UIWidget::i18n( const std::string& str, const String& defaultValue ) {
	return getTranslatorString( str, defaultValue );
}

UIWidget* UIWidget::getPrevTabWidget() const {
	UIWidget* widget = getPrevWidget();
	if ( widget ) {
		return widget;
	}
	UIWidget* found = NULL;
	UIWidget* possible = NULL;
	const Node* start = this;
	Node* container = getWindowContainer();
	while ( start ) {
		if ( start->isVisible() && start->isEnabled() ) {
			Node* next = start->getPrevNode();
			while ( next ) {
				if ( next->isVisible() && next->isEnabled() && next->isWidget() ) {
					possible = next->asType<UIWidget>();
					if ( possible->isTabFocusable() ) {
						return possible;
					}
					found = possible->getPrevWidget();
					if ( found ) {
						return found;
					}
				}
				next = next->getPrevNode();
			}
			if ( start->getParent() == container && container->getFirstWidget() ) {
				return container->asType<UIWidget>()->getPrevTabWidget();
			}
		}
		start = start->getParent();
	}
	return NULL;
}

UIWidget* UIWidget::getNextTabWidget() const {
	UIWidget* widget = getNextWidget();
	if ( widget ) {
		return widget;
	}
	UIWidget* found = NULL;
	UIWidget* possible = NULL;
	const Node* start = this;
	Node* container = getWindowContainer();
	while ( start ) {
		if ( start->isVisible() && start->isEnabled() ) {
			Node* next = start->getNextNode();
			while ( next ) {
				if ( next->isVisible() && next->isEnabled() && next->isWidget() ) {
					possible = next->asType<UIWidget>();
					if ( possible->isTabFocusable() ) {
						return possible;
					}
					found = possible->getNextWidget();
					if ( found ) {
						return found;
					}
				}
				next = next->getNextNode();
			}
			if ( start->getParent() == container && container->getFirstWidget() ) {
				return container->asType<UIWidget>()->getNextTabWidget();
			}
		}
		start = start->getParent();
	}
	return NULL;
}

void UIWidget::onFocusPrevWidget() {
	if ( !isTabStop() ) {
		Node* node = getPrevTabWidget();
		if ( NULL != node ) {
			node->setFocus( NodeFocusReason::Tab );
			sendCommonEvent( Event::OnTabNavigate );
		}
	} else {
		sendCommonEvent( Event::OnTabNavigate );
	}
}

void UIWidget::onFocusNextWidget() {
	if ( !isTabStop() ) {
		Node* node = getNextTabWidget();
		if ( NULL != node ) {
			node->setFocus( NodeFocusReason::Tab );
			sendCommonEvent( Event::OnTabNavigate );
		}
	} else {
		sendCommonEvent( Event::OnTabNavigate );
	}
}

}} // namespace EE::UI
