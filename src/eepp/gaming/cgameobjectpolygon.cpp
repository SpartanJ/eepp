#include <eepp/gaming/cgameobjectpolygon.hpp>

#include <eepp/gaming/clayer.hpp>
#include <eepp/graphics/cprimitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cGameObjectPolygon::cGameObjectPolygon( Uint32 DataId, eePolygon2f poly, cLayer * Layer, const Uint32& Flags ) :
	cGameObjectObject( DataId, poly.ToAABB(), Layer, Flags ),
	mPoly( poly )
{
}

cGameObjectPolygon::~cGameObjectPolygon() {
}

Uint32 cGameObjectPolygon::Type() const {
	return GAMEOBJECT_TYPE_POLYGON;
}

bool cGameObjectPolygon::IsType( const Uint32& type ) {
	return ( cGameObjectPolygon::Type() == type ) ? true : cGameObjectObject::IsType( type );
}

eeSize cGameObjectPolygon::Size() {
	return eeSize( mRect.Size().x, mRect.Size().y );
}

void cGameObjectPolygon::Draw() {
	Int32 selAdd	= mSelected ? 50 : 0;
	Int32 colFill	= 100 + selAdd;

	cPrimitives P;
	P.FillMode( EE_DRAW_FILL );
	P.SetColor( eeColorA( colFill, colFill, colFill, colFill ) );
	P.DrawPolygon( mPoly );

	P.FillMode( EE_DRAW_LINE );
	P.SetColor( eeColorA( 255, 255, 0, 200 ) );
	P.DrawPolygon( mPoly );
}

eeVector2f cGameObjectPolygon::Pos() const {
	return mPos;
}

void cGameObjectPolygon::Pos( eeVector2f pos ) {
	mPoly.Move( pos - mPos );

	cGameObjectObject::Pos( pos );
}

eePolygon2f cGameObjectPolygon::GetPolygon() {
	return mPoly;
}

bool cGameObjectPolygon::PointInside( const eeVector2f& p ) {
	if ( cGameObjectObject::PointInside( p ) ) {
		return mPoly.PointInside( p );
	}

	return false;
}

}}
