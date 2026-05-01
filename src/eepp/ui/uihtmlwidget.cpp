#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uilayoutermanager.hpp>

namespace EE { namespace UI {

UIHTMLWidget* UIHTMLWidget::New() {
	return eeNew( UIHTMLWidget, () );
}

UIHTMLWidget::UIHTMLWidget( const std::string& tag ) : UILayout( tag ) {}

UIHTMLWidget::~UIHTMLWidget() {
	eeSAFE_DELETE( mLayouter );
}

UILayouter* UIHTMLWidget::getLayouter() {
	if ( !mLayouter ) {
		mLayouter = UILayouterManager::create( mDisplay, this );
	}
	return mLayouter;
}

Uint32 UIHTMLWidget::getType() const {
	return UI_TYPE_HTML_WIDGET;
}

bool UIHTMLWidget::isType( const Uint32& type ) const {
	return UIHTMLWidget::getType() == type ? true : UILayout::isType( type );
}

bool UIHTMLWidget::isPacking() const {
	UILayouter* layouter = const_cast<UIHTMLWidget*>( this )->getLayouter();
	if ( layouter )
		return layouter->isPacking();
	return UILayout::isPacking();
}

void UIHTMLWidget::onDisplayChange() {
	eeSAFE_DELETE( mLayouter );
	getLayouter();
	notifyLayoutAttrChange();
}

void UIHTMLWidget::setDisplay( CSSDisplay display ) {
	if ( mDisplay != display ) {
		mDisplay = display;
		if ( mDisplay == CSSDisplay::InlineBlock || mDisplay == CSSDisplay::Inline ) {
			if ( getLayoutWidthPolicy() == SizePolicy::MatchParent )
				setLayoutWidthPolicy( SizePolicy::WrapContent );
		} else if ( mDisplay == CSSDisplay::Block || mDisplay == CSSDisplay::ListItem ) {
			if ( getLayoutWidthPolicy() == SizePolicy::WrapContent &&
				 mPosition != CSSPosition::Absolute && mPosition != CSSPosition::Fixed )
				setLayoutWidthPolicy( SizePolicy::MatchParent );
		}
		onDisplayChange();
	}
}

void UIHTMLWidget::setCSSPosition( CSSPosition position ) {
	if ( mPosition != position ) {
		mPosition = position;
		if ( position == CSSPosition::Absolute || position == CSSPosition::Fixed ) {
			if ( getLayoutWidthPolicy() == SizePolicy::MatchParent )
				setLayoutWidthPolicy( SizePolicy::WrapContent );
		}
		onPositionChange();
	}
}

void UIHTMLWidget::setOffsets( const Rectf& offsets ) {
	if ( mOffsets != offsets ) {
		mOffsets = offsets;
		mTopEq = String::fromFloat( offsets.Top, "dp" );
		mLeftEq = String::fromFloat( offsets.Left, "dp" );
		mRightEq = String::fromFloat( offsets.Right, "dp" );
		mBottomEq = String::fromFloat( offsets.Bottom, "dp" );
		notifyLayoutAttrChange();
	}
}

void UIHTMLWidget::setZIndex( int zIndex ) {
	mZIndex = zIndex;
}

std::vector<PropertyId> UIHTMLWidget::getPropertiesImplemented() const {
	auto props = UILayout::getPropertiesImplemented();
	auto local = { PropertyId::Display, PropertyId::Position, PropertyId::Top,	 PropertyId::Right,
				   PropertyId::Bottom,	PropertyId::Left,	  PropertyId::ZIndex };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

std::string UIHTMLWidget::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& state ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Display:
			return CSSDisplayHelper::toString( mDisplay );
		case PropertyId::Position:
			return CSSPositionHelper::toString( mPosition );
		case PropertyId::Top:
			return mTopEq;
		case PropertyId::Right:
			return mRightEq;
		case PropertyId::Bottom:
			return mBottomEq;
		case PropertyId::Left:
			return mLeftEq;
		case PropertyId::ZIndex:
			return String::toString( mZIndex );
		default:
			return UILayout::getPropertyString( propertyDef );
	}
}

bool UIHTMLWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Display: {
			setDisplay( CSSDisplayHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::Position: {
			setCSSPosition( CSSPositionHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::ZIndex: {
			setZIndex( attribute.asInt() );
			return true;
		}
		case PropertyId::Top: {
			mTopEq = attribute.asString();
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Right: {
			mRightEq = attribute.asString();
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Bottom: {
			mBottomEq = attribute.asString();
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Left: {
			mLeftEq = attribute.asString();
			notifyLayoutAttrChange();
			return true;
		}
		default:
			break;
	}

	return UILayout::applyProperty( attribute );
}
void UIHTMLWidget::updateLayout() {
	if ( getLayouter() )
		getLayouter()->updateLayout();
	else
		UILayout::updateLayout();

	positionOutOfFlowChildren();

	mDirtyLayout = false;
}

UIWidget* UIHTMLWidget::getContainingBlock() {
	if ( mPosition == CSSPosition::Fixed ) {
		Node* parent = getParent();
		UIWidget* lastWidget = parent && parent->isWidget() ? parent->asType<UIWidget>() : nullptr;
		while ( parent ) {
			if ( parent->isWidget() )
				lastWidget = parent->asType<UIWidget>();
			parent = parent->getParent();
		}
		return lastWidget;
	}

	Node* parent = getParent();
	UIWidget* lastWidget = nullptr;
	while ( parent ) {
		if ( parent->isWidget() ) {
			lastWidget = parent->asType<UIWidget>();
			if ( lastWidget->isType( UI_TYPE_HTML_WIDGET ) ) {
				if ( lastWidget->asType<UIHTMLWidget>()->getCSSPosition() != CSSPosition::Static ) {
					return lastWidget;
				}
			}
		}
		parent = parent->getParent();
	}
	return lastWidget;
}

void UIHTMLWidget::positionOutOfFlowChildren() {
	Node* child = mChild;
	while ( child ) {
		if ( child->isWidget() && child->isType( UI_TYPE_HTML_WIDGET ) ) {
			UIHTMLWidget* htmlChild = static_cast<UIHTMLWidget*>( child );
			CSSPosition pos = htmlChild->getCSSPosition();
			if ( pos == CSSPosition::Absolute || pos == CSSPosition::Fixed ) {
				UIWidget* cb = htmlChild->getContainingBlock();
				if ( cb ) {
					Rectf cbContentOffset = cb->getPixelsContentOffset();
					Float cbContentWidth = cb->getPixelsSize().getWidth() - cbContentOffset.Left -
										   cbContentOffset.Right;
					Float cbContentHeight = cb->getPixelsSize().getHeight() - cbContentOffset.Top -
											cbContentOffset.Bottom;

					Rectf margin = htmlChild->getLayoutPixelsMargin();
					Float childWidth = htmlChild->getPixelsSize().getWidth();
					Float childHeight = htmlChild->getPixelsSize().getHeight();

					Float top = 0;
					Float left = 0;

					bool useTop = htmlChild->mTopEq != "auto";
					bool useBottom = htmlChild->mBottomEq != "auto";
					bool useLeft = htmlChild->mLeftEq != "auto";
					bool useRight = htmlChild->mRightEq != "auto";

					if ( useLeft ) {
						left = htmlChild->lengthFromValue(
							htmlChild->mLeftEq, CSS::PropertyRelativeTarget::ContainingBlockWidth,
							0 );
					} else if ( useRight ) {
						Float rightVal = htmlChild->lengthFromValue(
							htmlChild->mRightEq, CSS::PropertyRelativeTarget::ContainingBlockWidth,
							0 );
						left = cbContentWidth - childWidth - margin.Left - margin.Right - rightVal;
					}

					if ( useTop ) {
						top = htmlChild->lengthFromValue(
							htmlChild->mTopEq, CSS::PropertyRelativeTarget::ContainingBlockHeight,
							0 );
					} else if ( useBottom ) {
						Float bottomVal = htmlChild->lengthFromValue(
							htmlChild->mBottomEq,
							CSS::PropertyRelativeTarget::ContainingBlockHeight, 0 );
						top =
							cbContentHeight - childHeight - margin.Top - margin.Bottom - bottomVal;
					}

					top += margin.Top;
					left += margin.Left;

					Vector2f cbPos( cbContentOffset.Left, cbContentOffset.Top );
					cbPos.x += left;
					cbPos.y += top;

					Vector2f worldPos = cb->convertToWorldSpace( cbPos );
					Vector2f localPos = convertToNodeSpace( worldPos );
					htmlChild->setPixelsPosition( localPos );
				}
			}
		}
		child = child->getNextNode();
	}
}

void UIHTMLWidget::invalidateIntrinsicSize() {
	if ( mLayouter )
		mLayouter->invalidateIntrinsicWidths();
	UIWidget::invalidateIntrinsicSize();
}

bool UIHTMLWidget::isOutOfFlow() const {
	return mPosition == CSSPosition::Absolute || mPosition == CSSPosition::Fixed;
}

}} // namespace EE::UI
