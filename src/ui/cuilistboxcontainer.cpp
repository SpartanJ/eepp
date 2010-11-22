#include "cuilistboxcontainer.hpp"
#include "cuilistbox.hpp"
#include "cuilistboxitem.hpp"

namespace EE { namespace UI {

cUIListBoxContainer::cUIListBoxContainer( cUIControl::CreateParams& Params ) :
	cUIControl( Params )
{
	DisableChildCloseCheck();
}

cUIListBoxContainer::~cUIListBoxContainer()
{
}

void cUIListBoxContainer::Update() {
	cUIListBox * LBParent = reinterpret_cast<cUIListBox*> ( Parent() );

	if ( LBParent->mItems.size() ) {
		for ( Uint32 i = LBParent->mVisibleFirst; i <= LBParent->mVisibleLast; i++ )
			if ( NULL != LBParent->mItems[i] )
				LBParent->mItems[i]->Update();
	}
}

void cUIListBoxContainer::DrawChilds() {
	cUIListBox * LBParent = reinterpret_cast<cUIListBox*> ( Parent() );

	if ( LBParent->mItems.size() ) {
		for ( Uint32 i = LBParent->mVisibleFirst; i <= LBParent->mVisibleLast; i++ )
			if ( NULL != LBParent->mItems[i] )
				LBParent->mItems[i]->InternalDraw();
	}
}

cUIControl * cUIListBoxContainer::OverFind( const eeVector2f& Point ) {
	cUIListBox * LBParent = reinterpret_cast<cUIListBox*> ( Parent() );

	cUIControl * pOver = NULL;

	if ( mVisible && mEnabled && LBParent->mItems.size() ) {
		UpdateQuad();

		if ( PointInsidePolygon2( mPoly, Point ) ) {
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
