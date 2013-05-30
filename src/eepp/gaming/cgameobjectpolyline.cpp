#include <eepp/gaming/cgameobjectpolyline.hpp>

#include <eepp/gaming/clayer.hpp>
#include <eepp/graphics/cprimitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cGameObjectPolyline::cGameObjectPolyline( Uint32 DataId, eePolygon2f poly, cLayer * Layer, const Uint32& Flags ) :
	cGameObjectPolygon( DataId, poly, Layer, Flags )
{
}

cGameObjectPolyline::~cGameObjectPolyline() {
}

Uint32 cGameObjectPolyline::Type() const {
	return GAMEOBJECT_TYPE_POLYGON;
}

bool cGameObjectPolyline::IsType( const Uint32& type ) {
	return ( cGameObjectPolyline::Type() == type ) ? true : cGameObjectPolygon::IsType( type );
}

void cGameObjectPolyline::Draw() {
	cPrimitives P;

	if ( mSelected ) {
		P.FillMode( DRAW_FILL );
		P.SetColor( eeColorA( 150, 150, 150, 150 ) );
		P.DrawPolygon( mPoly );
	}

	P.FillMode( DRAW_LINE );
	P.SetColor( eeColorA( 255, 255, 0, 200 ) );
	P.DrawPolygon( mPoly );
}

cGameObjectObject * cGameObjectPolyline::Copy() {
	return eeNew( cGameObjectPolyline, ( mDataId, mPoly, mLayer, mFlags ) );
}

}}
