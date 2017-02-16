#ifndef EE_UITUIITEMCONTAINER_HPP
#define EE_UITUIITEMCONTAINER_HPP

#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

template<class TContainer>
class UIItemContainer : public UIControl {
	public:
		UIItemContainer( UIControl::CreateParams& Params );

		~UIItemContainer();

		void update();

		void drawChilds();
	protected:
		UIControl * overFind( const Vector2f& Point );
};

template<class TContainer>
UIItemContainer<TContainer>::UIItemContainer( UIControl::CreateParams& Params ) :
	UIControl( Params )
{
}

template<class TContainer>
UIItemContainer<TContainer>::~UIItemContainer()
{
}

template<class TContainer>
void UIItemContainer<TContainer>::update() {
	TContainer * tParent = reinterpret_cast<TContainer*> ( parent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->update();
		}
	}
}

template<class TContainer>
void UIItemContainer<TContainer>::drawChilds() {
	TContainer * tParent = reinterpret_cast<TContainer*> ( parent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ )
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->internalDraw();
	}
}

template<class TContainer>
UIControl * UIItemContainer<TContainer>::overFind( const Vector2f& Point ) {
	TContainer * tParent = reinterpret_cast<TContainer*> ( parent() );

	UIControl * pOver = NULL;

	if ( mEnabled && mVisible && tParent->mItems.size() ) {
		updateQuad();

		if ( mPoly.pointInside( Point ) ) {
			writeCtrlFlag( UI_CTRL_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );

			for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
				if ( NULL != tParent->mItems[i] ) {
					UIControl * ChildOver = tParent->mItems[i]->overFind( Point );

					if ( NULL != ChildOver ) {
						pOver = ChildOver;

						break;
					}
				}
			}

			if ( NULL == pOver )
				pOver = const_cast<UIControl *>( reinterpret_cast<const UIControl *>( this ) );
		}
	}

	return pOver;
}

}}

#endif
