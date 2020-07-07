#ifndef EE_UITUIITEMCONTAINER_HPP
#define EE_UITUIITEMCONTAINER_HPP

#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

template <class TContainer> class UIItemContainer : public UIWidget {
  public:
	UIItemContainer();

	~UIItemContainer();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	void update( const Time& time );

	void drawChilds();

	virtual void onChildCountChange( Node* child, const bool& removed );

	Node* overFind( const Vector2f& Point );
};

template <class TContainer> Uint32 UIItemContainer<TContainer>::getType() const {
	return UI_TYPE_ITEMCONTAINER;
}

template <class TContainer> bool UIItemContainer<TContainer>::isType( const Uint32& type ) const {
	return UIItemContainer<TContainer>::getType() == type ? true : UIWidget::isType( type );
}

template <class TContainer> UIItemContainer<TContainer>::UIItemContainer() : UIWidget() {}

template <class TContainer> UIItemContainer<TContainer>::~UIItemContainer() {}

template <class TContainer>
void UIItemContainer<TContainer>::onChildCountChange( Node* child, const bool& removed ) {
	UINode::onChildCountChange( child, removed );
}

template <class TContainer> void UIItemContainer<TContainer>::update( const Time& time ) {
	TContainer* tParent = reinterpret_cast<TContainer*>( getParent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->update( time );
		}
	}
}

template <class TContainer> void UIItemContainer<TContainer>::drawChilds() {
	TContainer* tParent = reinterpret_cast<TContainer*>( getParent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ )
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->nodeDraw();
	}
}

template <class TContainer> Node* UIItemContainer<TContainer>::overFind( const Vector2f& Point ) {
	TContainer* tParent = reinterpret_cast<TContainer*>( getParent() );

	Node* pOver = NULL;

	if ( mEnabled && mVisible && tParent->mItems.size() ) {
		updateWorldPolygon();

		if ( mWorldBounds.contains( Point ) && mPoly.pointInside( Point ) ) {
			writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );
			mSceneNode->addMouseOverNode( this );

			for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
				if ( NULL != tParent->mItems[i] ) {
					Node* ChildOver = tParent->mItems[i]->overFind( Point );

					if ( NULL != ChildOver ) {
						pOver = ChildOver;

						break;
					}
				}
			}

			if ( NULL == pOver )
				pOver = const_cast<Node*>( reinterpret_cast<const Node*>( this ) );
		}
	}

	return pOver;
}

}} // namespace EE::UI

#endif
