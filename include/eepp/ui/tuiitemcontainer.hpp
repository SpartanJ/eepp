#ifndef EE_UITUIITEMCONTAINER_HPP
#define EE_UITUIITEMCONTAINER_HPP

#include <eepp/ui/cuicontrol.hpp>

namespace EE { namespace UI {

template<class TContainer>
class tUIItemContainer : public cUIControl {
	public:
		tUIItemContainer( cUIControl::CreateParams& Params );

		~tUIItemContainer();

		void Update();

		void DrawChilds();
	protected:
		cUIControl * OverFind( const Vector2f& Point );
};

template<class TContainer>
tUIItemContainer<TContainer>::tUIItemContainer( cUIControl::CreateParams& Params ) :
	cUIControl( Params )
{
}

template<class TContainer>
tUIItemContainer<TContainer>::~tUIItemContainer()
{
}

template<class TContainer>
void tUIItemContainer<TContainer>::Update() {
	TContainer * tParent = reinterpret_cast<TContainer*> ( Parent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->Update();
		}
	}
}

template<class TContainer>
void tUIItemContainer<TContainer>::DrawChilds() {
	TContainer * tParent = reinterpret_cast<TContainer*> ( Parent() );

	if ( tParent->mItems.size() ) {
		for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ )
			if ( NULL != tParent->mItems[i] )
				tParent->mItems[i]->InternalDraw();
	}
}

template<class TContainer>
cUIControl * tUIItemContainer<TContainer>::OverFind( const Vector2f& Point ) {
	TContainer * tParent = reinterpret_cast<TContainer*> ( Parent() );

	cUIControl * pOver = NULL;

	if ( mEnabled && mVisible && tParent->mItems.size() ) {
		UpdateQuad();

		if ( mPoly.PointInside( Point ) ) {
			WriteCtrlFlag( UI_CTRL_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );

			for ( Uint32 i = tParent->mVisibleFirst; i <= tParent->mVisibleLast; i++ ) {
				if ( NULL != tParent->mItems[i] ) {
					cUIControl * ChildOver = tParent->mItems[i]->OverFind( Point );

					if ( NULL != ChildOver ) {
						pOver = ChildOver;

						break;
					}
				}
			}

			if ( NULL == pOver )
				pOver = const_cast<cUIControl *>( reinterpret_cast<const cUIControl *>( this ) );
		}
	}

	return pOver;
}

}}

#endif
