#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

UILayout* UILayout::New() {
	return eeNew( UILayout, () );
}

UILayout::UILayout() : UIWidget( "layout" ), mDirtyLayout( false ) {
	mNodeFlags |= NODE_FLAG_LAYOUT;
}

UILayout::UILayout( const std::string& tag ) : UIWidget( tag ), mDirtyLayout( false ) {
	mNodeFlags |= NODE_FLAG_LAYOUT;
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

	packConditional();
}

void UILayout::onSizeChange() {
	UIWidget::onSizeChange();
	packConditional();
}

void UILayout::onPaddingChange() {
	UIWidget::onPaddingChange();
	packConditional();
}

void UILayout::onParentSizeChange( const Vector2f& ) {
	UIWidget::onParentChange();
	packConditional();
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

void UILayout::pack() {}

void UILayout::setLayoutDirty() {
	if ( !mDirtyLayout ) {
		mUISceneNode->invalidateLayout( this );
		mDirtyLayout = true;
	}
}

void UILayout::packConditional() {
	if ( mUISceneNode->isUpdatingLayouts() ) {
		pack();
	} else if ( !mDirtyLayout ) {
		setLayoutDirty();
	}
}

void UILayout::packLayoutTree() {
	pack();

	for ( auto layout : mLayouts ) {
		layout->packLayoutTree();
	}

	onLayoutUpdate();
}

}} // namespace EE::UI
