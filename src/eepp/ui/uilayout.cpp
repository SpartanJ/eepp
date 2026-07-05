#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

static UILayout::Metrics sMetrics;
static bool sMetricsEnabled{ false };

UILayout::UILayout( const std::string& tag ) : UIWidget( tag ) {
	mNodeFlags |= NODE_FLAG_LAYOUT;
	unsetFlags( UI_TAB_FOCUSABLE );
}

void UILayout::onChildCountChange( Node* child, const bool& removed ) {
	if ( sMetricsEnabled )
		sMetrics.childCountChanges++;

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
	setLayoutDirty( 0 );
}

void UILayout::setLayoutDirty( LayoutInvalidationFlags reasons ) {
	if ( !mDirtyLayout ) {
		if ( sMetricsEnabled )
			sMetrics.invalidations++;

		mUISceneNode->invalidateLayout( this, reasons );
		mDirtyLayout = true;
	} else if ( reasons ) {
		mUISceneNode->invalidateLayout( this, reasons );
	}
}

bool UILayout::isGravityOwner() const {
	return mGravityOwner;
}

void UILayout::setGravityOwner( bool gravityOwner ) {
	mGravityOwner = gravityOwner;
}

void UILayout::tryUpdateLayout() {
	if ( mUISceneNode->isUpdatingLayouts() ) {
		if ( !isPacking() ) {
			if ( sMetricsEnabled ) {
				sMetrics.synchronousUpdates++;
			}
			updateLayout();
		}
	} else if ( !mDirtyLayout ) {
		setLayoutDirty( LayoutInvalidation::Self );
	}
}

void UILayout::updateLayoutTree() {
	if ( sMetricsEnabled )
		sMetrics.treeUpdates++;

	mCurrentLayoutReasons = mDirtyReasons;
	mUpdatingLayoutTree = true;
	updateLayout();

	for ( auto layout : mLayouts ) {
		layout->updateLayoutTree();
	}

	mUpdatingLayoutTree = false;
	mDirtyReasons = 0;
	onLayoutUpdate();
	mCurrentLayoutReasons = 0;
}

bool UILayout::setMatchParentIfNeededVerticalGrowth() {
	bool sizeChanged = false;
	Sizef size( getPixelsSize() );

	if ( getLayoutWidthPolicy() == SizePolicy::MatchParent && 0 == getLayoutWeight() ) {
		Float w = getMatchParentWidth();

		if ( (int)w != (int)getPixelsSize().getWidth() ) {
			sizeChanged = true;
			size.setWidth( w );
		}
	}

	if ( getLayoutHeightPolicy() == SizePolicy::MatchParent ) {
		Float h = getMatchParentHeight();

		if ( (int)h != (int)getPixelsSize().getHeight() ) {
			sizeChanged = true;
			size.setHeight( h );
		}
	}

	if ( sizeChanged )
		setInternalPixelsSize( size );

	return sizeChanged;
}

void UILayout::updateLayoutWrappingContents() {
	auto oldWidthPolicy = mWidthPolicy;
	auto oldHeightPolicy = mHeightPolicy;

	if ( mWidthPolicy == SizePolicy::MatchParent )
		mWidthPolicy = SizePolicy::WrapContent;
	if ( mHeightPolicy == SizePolicy::MatchParent )
		mHeightPolicy = SizePolicy::WrapContent;

	updateLayout();

	mWidthPolicy = oldWidthPolicy;
	mHeightPolicy = oldHeightPolicy;
}

void UILayout::onAutoSizeChild( UIWidget* child ) {
	if ( sMetricsEnabled )
		sMetrics.autoSizeChildren++;

	if ( child->isLayout() ) {
		child->asType<UILayout>()->updateLayoutWrappingContents();
	} else {
		child->onAutoSize();
	}
}

void UILayout::resetMetrics() {
	sMetrics = {};
	sMetricsEnabled = true;
}

UILayout::Metrics UILayout::getMetrics() {
	return sMetrics;
}

void UILayout::setMetricsEnabled( bool enabled ) {
	sMetricsEnabled = enabled;
}

void UILayout::countRichTextRebuild() {
	if ( sMetricsEnabled )
		sMetrics.richTextRebuilds++;
}

}} // namespace EE::UI
