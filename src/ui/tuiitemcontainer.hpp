#ifndef EE_UITUIITEMCONTAINER_HPP
#define EE_UITUIITEMCONTAINER_HPP

#include "cuicontrol.hpp"

namespace EE { namespace UI {

template<class TContainer>
class tUIItemContainer : public cUIControl {
	public:
		tUIItemContainer( cUIControl::CreateParams& Params );

		~tUIItemContainer();

		void Update();

		void DrawChilds();
	protected:
		cUIControl * OverFind( const eeVector2f& Point );
};

template<class TContainer>
tUIItemContainer<TContainer>::tUIItemContainer( cUIControl::CreateParams& Params ) :
	cUIControl( Params )
{
	DisableChildCloseCheck();
}

template<class TContainer>
tUIItemContainer<TContainer>::~tUIItemContainer()
{
}

template<class TContainer>
void tUIItemContainer<TContainer>::Update() {
	TContainer * LBParent = reinterpret_cast<TContainer*> ( Parent() );

	if ( LBParent->mItems.size() ) {
		for ( Uint32 i = LBParent->mVisibleFirst; i <= LBParent->mVisibleLast; i++ ) {
			if ( NULL != LBParent->mItems[i] )
				LBParent->mItems[i]->Update();
		}
	}
}

template<class TContainer>
void tUIItemContainer<TContainer>::DrawChilds() {
	TContainer * LBParent = reinterpret_cast<TContainer*> ( Parent() );

	if ( LBParent->mItems.size() ) {
		for ( Uint32 i = LBParent->mVisibleFirst; i <= LBParent->mVisibleLast; i++ )
			if ( NULL != LBParent->mItems[i] )
				LBParent->mItems[i]->InternalDraw();
	}
}

template<class TContainer>
cUIControl * tUIItemContainer<TContainer>::OverFind( const eeVector2f& Point ) {
	TContainer * LBParent = reinterpret_cast<TContainer*> ( Parent() );

	cUIControl * pOver = NULL;

	if ( mVisible && mEnabled && LBParent->mItems.size() ) {
		UpdateQuad();

		if ( PointInsidePolygon2( mPoly, Point ) ) {
			WriteCtrlFlag( UI_CTRL_FLAG_MOUSEOVER_ME_OR_CHILD_POS, 1 );

			for ( Uint32 i = LBParent->mVisibleFirst; i <= LBParent->mVisibleLast; i++ ) {
				if ( NULL != LBParent->mItems[i] ) {
					cUIControl * ChildOver = LBParent->mItems[i]->OverFind( Point );

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
