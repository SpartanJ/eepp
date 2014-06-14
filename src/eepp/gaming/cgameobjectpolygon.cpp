#include <eepp/gaming/cgameobjectpolygon.hpp>

#include <eepp/gaming/clayer.hpp>
#include <eepp/graphics/cprimitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cGameObjectPolygon::cGameObjectPolygon( Uint32 DataId, eePolygon2f poly, cLayer * Layer, const Uint32& Flags ) :
	cGameObjectObject( DataId, poly.ToAABB(), Layer, Flags )
{
	mPoly = eePolygon2f( poly );
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
	P.FillMode( DRAW_FILL );
	P.SetColor( ColorA( colFill, colFill, colFill, colFill ) );
	P.DrawPolygon( mPoly );

	P.FillMode( DRAW_LINE );
	P.SetColor( ColorA( 255, 255, 0, 200 ) );
	P.DrawPolygon( mPoly );
}

void cGameObjectPolygon::SetPolygonPoint( Uint32 index, eeVector2f p ) {
	mPoly.SetAt( index, p );
	mRect	= mPoly.ToAABB();
	mPos	= eeVector2f( mRect.Left, mRect.Top );
}

bool cGameObjectPolygon::PointInside( const eeVector2f& p ) {
	if ( cGameObjectObject::PointInside( p ) ) {
		return mPoly.PointInside( p );
	}

	return false;
}

cGameObjectObject * cGameObjectPolygon::Copy() {
	return eeNew( cGameObjectPolygon, ( mDataId, mPoly, mLayer, mFlags ) );
}

}}
