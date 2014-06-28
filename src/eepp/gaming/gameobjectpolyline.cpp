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

Uint32 GameObjectPolyline::Type() const {
	return GAMEOBJECT_TYPE_POLYGON;
}

bool GameObjectPolyline::IsType( const Uint32& type ) {
	return ( GameObjectPolyline::Type() == type ) ? true : GameObjectPolygon::IsType( type );
}

void GameObjectPolyline::Draw() {
	Primitives P;

	if ( mSelected ) {
		P.FillMode( DRAW_FILL );
		P.SetColor( ColorA( 150, 150, 150, 150 ) );
		P.DrawPolygon( mPoly );
	}

	P.FillMode( DRAW_LINE );
	P.SetColor( ColorA( 255, 255, 0, 200 ) );
	P.DrawPolygon( mPoly );
}

GameObjectObject * GameObjectPolyline::Copy() {
	return eeNew( GameObjectPolyline, ( mDataId, mPoly, mLayer, mFlags ) );
}

}}
