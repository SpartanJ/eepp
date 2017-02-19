#include <eepp/gaming/gameobjectpolyline.hpp>

#include <eepp/gaming/maplayer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

GameObjectPolyline::GameObjectPolyline( Uint32 DataId, Polygon2f poly, MapLayer * Layer, const Uint32& Flags ) :
	GameObjectPolygon( DataId, poly, Layer, Flags )
{
}

GameObjectPolyline::~GameObjectPolyline() {
}

Uint32 GameObjectPolyline::getType() const {
	return GAMEOBJECT_TYPE_POLYGON;
}

bool GameObjectPolyline::isType( const Uint32& type ) {
	return ( GameObjectPolyline::getType() == type ) ? true : GameObjectPolygon::isType( type );
}

void GameObjectPolyline::draw() {
	Primitives P;

	if ( mSelected ) {
		P.setFillMode( DRAW_FILL );
		P.setColor( ColorA( 150, 150, 150, 150 ) );
		P.drawPolygon( mPoly );
	}

	P.setFillMode( DRAW_LINE );
	P.setColor( ColorA( 255, 255, 0, 200 ) );
	P.drawPolygon( mPoly );
}

GameObjectObject * GameObjectPolyline::clone() {
	return eeNew( GameObjectPolyline, ( mDataId, mPoly, mLayer, mFlags ) );
}

}}
