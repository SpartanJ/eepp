#include <eepp/gaming/gameobjectpolygon.hpp>

#include <eepp/gaming/maplayer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

GameObjectPolygon::GameObjectPolygon( Uint32 DataId, Polygon2f poly, MapLayer * Layer, const Uint32& Flags ) :
	GameObjectObject( DataId, poly.toAABB(), Layer, Flags )
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
	return Sizei( mRect.getSize().x, mRect.getSize().y );
}

void GameObjectPolygon::Draw() {
	Int32 selAdd	= mSelected ? 50 : 0;
	Int32 colFill	= 100 + selAdd;

	Primitives P;
	P.fillMode( DRAW_FILL );
	P.setColor( ColorA( colFill, colFill, colFill, colFill ) );
	P.drawPolygon( mPoly );

	P.fillMode( DRAW_LINE );
	P.setColor( ColorA( 255, 255, 0, 200 ) );
	P.drawPolygon( mPoly );
}

void GameObjectPolygon::SetPolygonPoint( Uint32 index, Vector2f p ) {
	mPoly.setAt( index, p );
	mRect	= mPoly.toAABB();
	mPos	= Vector2f( mRect.Left, mRect.Top );
}

bool GameObjectPolygon::PointInside( const Vector2f& p ) {
	if ( GameObjectObject::PointInside( p ) ) {
		return mPoly.pointInside( p );
	}

	return false;
}

GameObjectObject * GameObjectPolygon::Copy() {
	return eeNew( GameObjectPolygon, ( mDataId, mPoly, mLayer, mFlags ) );
}

}}
