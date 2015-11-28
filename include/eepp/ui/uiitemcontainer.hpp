#ifndef EE_UITUIITEMCONTAINER_HPP
#define EE_UITUIITEMCONTAINER_HPP

#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

template<class TContainer>
class UIItemContainer : public UIControl {
	public:
		UIItemContainer( UIControl::CreateParams& Params );

		~UIItemContainer();

		void Update();

		void DrawChilds();
	protected:
		UIControl * OverFind( const Vector2f& Point );
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
void UIItemContainer<TContainer>::Update() {
	TContainer * tParent = reinterpret_cast<TContainer*> ( Parent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->Update();
		}
	}
}

template<class TContainer>
void UIItemContainer<TContainer>::DrawChilds() {
	TContainer * tParent = reinterpret_cast<TContainer*> ( Parent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ )
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->InternalDraw();
	}
}

template<class TContainer>
UIControl * UIItemContainer<TContainer>::OverFind( const Vector2f& Point ) {
	TContainer * tParent = reinterpret_cast<TContainer*> ( Parent() );

	UIControl * pOver = NULL;

	if ( mEnabled && mVisible && tParent->mItems.size() ) {
		UpdateQuad();

		if ( mPoly.PointInside( Point ) ) {
			WriteCtrlFlag( UI_CTRL_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );

			for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
				if ( NULL != tParent->mItems[i] ) {
					UIControl * ChildOver = tParent->mItems[i]->OverFind( Point );

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
