#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

UILayout::UILayout( const std::string& tag ) : UIWidget( tag ) {
	mNodeFlags |= NODE_FLAG_LAYOUT;
	unsetFlags( UI_TAB_FOCUSABLE );
}

void UILayout::onChildCountChange( Node* child, const bool& removed ) {
	UIWidget::onChildCountChange( child, removed );

	if ( child->isLayout() ) {
		UILayout* layout = child->asType<UILayout>();
		if ( removed ) {
			mLayouts.erase( layout );
		} else {
			mLayouts.insert( layout );
		}
	}

	tryUpdateLayout();
}

void UILayout::onSizeChange() {
	UIWidget::onSizeChange();
	tryUpdateLayout();
}

void UILayout::onPaddingChange() {
	UIWidget::onPaddingChange();
	tryUpdateLayout();
}

void UILayout::onParentSizeChange( const Vector2f& sizeChange ) {
	UIWidget::onParentSizeChange( sizeChange );
	if ( !getParent()->isLayout() ) {
		mPacking = false;
		tryUpdateLayout();
	}
}

void UILayout::onLayoutUpdate() {
	sendCommonEvent( Event::OnLayoutUpdate );
}

Uint32 UILayout::getType() const {
	return UI_TYPE_LAYOUT;
}

bool UILayout::isType( const Uint32& type ) const {
	return UILayout::getType() == type ? true : UIWidget::isType( type );
}

const Sizef& UILayout::getSize() const {
	if ( mDirtyLayout )
		const_cast<UILayout*>( this )->updateLayout();
	return UIWidget::getSize();
}

void UILayout::updateLayout() {}

void UILayout::setLayoutDirty() {
	if ( !mDirtyLayout ) {
		mUISceneNode->invalidateLayout( this );
		mDirtyLayout = true;
	}
}

Sizef UILayout::getSizeFromLayoutPolicy() {
	Sizef size( getPixelsSize() );

	if ( getLayoutWidthPolicy() == SizePolicy::MatchParent ) {
		Float w = getMatchParentWidth();

		if ( (int)w != (int)getPixelsSize().getWidth() )
			size.setWidth( w );
	}

	if ( getLayoutHeightPolicy() == SizePolicy::MatchParent ) {
		Float h = getMatchParentHeight();

		if ( (int)h != (int)getPixelsSize().getHeight() )
			size.setHeight( h );
	}

	return size;
}

Float UILayout::getMatchParentWidth() const {
	Rectf padding = Rectf();

	if ( getParent()->isWidget() )
		padding = static_cast<UIWidget*>( getParent() )->getPixelsPadding();

	Float width = getParent()->getPixelsSize().getWidth() - mLayoutMarginPx.Left -
				  mLayoutMarginPx.Right - padding.Left - padding.Right;

	if ( !mMaxWidthEq.empty() ) {
		Float maxWidth( getMaxSizePx().getWidth() - mLayoutMarginPx.Left - mLayoutMarginPx.Right -
						padding.Left - padding.Right );
		if ( maxWidth > 0 && maxWidth < width )
			width = maxWidth;
	}

	return eemax( 0.f, width );
}

Float UILayout::getMatchParentHeight() const {
	Rectf padding = Rectf();

	if ( getParent()->isWidget() )
		padding = static_cast<UIWidget*>( getParent() )->getPadding();

	Float height = getParent()->getPixelsSize().getHeight() - mLayoutMarginPx.Top -
				   mLayoutMarginPx.Bottom - padding.Top - padding.Bottom;

	if ( !mMaxHeightEq.empty() ) {
		Float maxHeight( getMaxSizePx().getHeight() - mLayoutMarginPx.Left - mLayoutMarginPx.Right -
						 padding.Left - padding.Right );
		if ( maxHeight > 0 && maxHeight < height )
			height = maxHeight;
	}

	return eemax( 0.f, height );
}

bool UILayout::isGravityOwner() const {
	return mGravityOwner;
}

void UILayout::setGravityOwner( bool gravityOwner ) {
	mGravityOwner = gravityOwner;
}

void UILayout::tryUpdateLayout() {
	if ( mUISceneNode->isUpdatingLayouts() ) {
		updateLayout();
	} else if ( !mDirtyLayout ) {
		setLayoutDirty();
	}
}

void UILayout::updateLayoutTree() {
	updateLayout();

	for ( auto layout : mLayouts ) {
		layout->updateLayoutTree();
	}

	onLayoutUpdate();
}

}} // namespace EE::UI
