#include "cuimap.hpp"

namespace EE { namespace Gaming { namespace MapEditor {

cUIMap::cUIMap( const cUIComplexControl::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mMap( NULL )
{
	mMap = eeNew( cMap, () );
}

cUIMap::~cUIMap() {
	eeSAFE_DELETE( mMap );
}

cMap * cUIMap::Map() const {
	return mMap;
}

void cUIMap::Draw() {
	if ( NULL != mMap ) {
		mMap->Position( mScreenPos );
		mMap->Draw();
	}
}

void cUIMap::Update() {
	if ( NULL != mMap ) {
		mMap->Update();
	}
}

void cUIMap::OnSizeChange() {
	if ( NULL != mMap ) {
		mMap->Position( mScreenPos );
		mMap->ViewSize( mSize );
	}

	cUIComplexControl::OnSizeChange();
}

}}}
