#include <eepp/gaming/gameobjectpolygon.hpp>

#include <eepp/gaming/maplayer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

GameObjectPolygon::GameObjectPolygon( Uint32 DataId, Polygon2f poly, MapLayer * Layer, const Uint32& Flags ) :
	GameObjectObject( DataId, poly.ToAABB(), Layer, Flags )
{
	mPoly = Polygon2f( poly );
}

GameObjectPolygon::~GameObjectPolygon() {
}

Uint32 GameObjectPolygon::Type() const {
	return GAMEOBJECT_TYPE_POLYGON;
}

bool GameObjectPolygon::IsType( const Uint32& type ) {
	return ( GameObjectPolygon::Type() == type ) ? true : GameObjectObject::IsType( type );
}

Sizei GameObjectPolygon::Size() {
	return Sizei( mRect.Size().x, mRect.Size().y );
}

void GameObjectPolygon::Draw() {
	Int32 selAdd	= mSelected ? 50 : 0;
	Int32 colFill	= 100 + selAdd;

	Primitives P;
	P.FillMode( DRAW_FILL );
	P.SetColor( ColorA( colFill, colFill, colFill, colFill ) );
	P.DrawPolygon( mPoly );

	P.FillMode( DRAW_LINE );
	P.SetColor( ColorA( 255, 255, 0, 200 ) );
	P.DrawPolygon( mPoly );
}

void GameObjectPolygon::SetPolygonPoint( Uint32 index, Vector2f p ) {
	mPoly.SetAt( index, p );
	mRect	= mPoly.ToAABB();
	mPos	= Vector2f( mRect.Left, mRect.Top );
}

bool GameObjectPolygon::PointInside( const Vector2f& p ) {
	if ( GameObjectObject::PointInside( p ) ) {
		return mPoly.PointInside( p );
	}

	return false;
}

GameObjectObject * GameObjectPolygon::Copy() {
	return eeNew( GameObjectPolygon, ( mDataId, mPoly, mLayer, mFlags ) );
}

}}
