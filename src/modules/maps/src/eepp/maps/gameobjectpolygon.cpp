#include <eepp/maps/gameobjectpolygon.hpp>

#include <eepp/graphics/primitives.hpp>
#include <eepp/maps/maplayer.hpp>
using namespace EE::Graphics;

namespace EE { namespace Maps {

GameObjectPolygon::GameObjectPolygon( Uint32 DataId, Polygon2f poly, MapLayer* Layer,
									  const Uint32& Flags ) :
	GameObjectObject( DataId, poly.getBounds(), Layer, Flags ) {
	mPoly = Polygon2f( poly );
}

GameObjectPolygon::~GameObjectPolygon() {}

Uint32 GameObjectPolygon::getType() const {
	return GAMEOBJECT_TYPE_POLYGON;
}

bool GameObjectPolygon::isType( const Uint32& type ) {
	return ( GameObjectPolygon::getType() == type ) ? true : GameObjectObject::isType( type );
}

Sizei GameObjectPolygon::getSize() {
	return Sizei( mRect.getSize().x, mRect.getSize().y );
}

void GameObjectPolygon::draw() {
	Int32 selAdd = mSelected ? 50 : 0;
	Int32 colFill = 100 + selAdd;

	Primitives P;
	P.setFillMode( DRAW_FILL );
	P.setColor( Color( colFill, colFill, colFill, colFill ) );
	P.drawPolygon( mPoly );

	P.setFillMode( DRAW_LINE );
	P.setColor( Color( 255, 255, 0, 200 ) );
	P.drawPolygon( mPoly );
}

void GameObjectPolygon::setPolygonPoint( Uint32 index, Vector2f p ) {
	mPoly.setAt( index, p );
	mRect = mPoly.getBounds();
	mPos = Vector2f( mRect.Left, mRect.Top );
}

bool GameObjectPolygon::pointInside( const Vector2f& p ) {
	if ( GameObjectObject::pointInside( p ) ) {
		return mPoly.pointInside( p );
	}

	return false;
}

GameObjectObject* GameObjectPolygon::clone() {
	return eeNew( GameObjectPolygon, ( mDataId, mPoly, mLayer, mFlags ) );
}

}} // namespace EE::Maps
