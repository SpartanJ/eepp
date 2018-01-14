#ifndef EE_UITUIITEMCONTAINER_HPP
#define EE_UITUIITEMCONTAINER_HPP

#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI {

template<class TContainer>
class UIItemContainer : public UINode {
	public:
		UIItemContainer();

		~UIItemContainer();

		void update();

		void drawChilds();
	protected:
		UINode * overFind( const Vector2f& Point );
};

template<class TContainer>
UIItemContainer<TContainer>::UIItemContainer() :
	UINode()
{
}

template<class TContainer>
UIItemContainer<TContainer>::~UIItemContainer()
{
}

template<class TContainer>
void UIItemContainer<TContainer>::update() {
	TContainer * tParent = reinterpret_cast<TContainer*> ( getParent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->update();
		}
	}
}

template<class TContainer>
void UIItemContainer<TContainer>::drawChilds() {
	TContainer * tParent = reinterpret_cast<TContainer*> ( getParent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ )
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->internalDraw();
	}
}

template<class TContainer>
UINode * UIItemContainer<TContainer>::overFind( const Vector2f& Point ) {
	TContainer * tParent = reinterpret_cast<TContainer*> ( getParent() );

	UINode * pOver = NULL;

	if ( mEnabled && mVisible && tParent->mItems.size() ) {
		updateQuad();

		if ( mPoly.pointInside( Point ) ) {
			writeCtrlFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );

			for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
				if ( NULL != tParent->mItems[i] ) {
					UINode * ChildOver = tParent->mItems[i]->overFind( Point );

					if ( NULL != ChildOver ) {
						pOver = ChildOver;

						break;
					}
				}
			}

			if ( NULL == pOver )
				pOver = const_cast<UINode *>( reinterpret_cast<const UINode *>( this ) );
		}
	}

	return pOver;
}

}}

#endif
