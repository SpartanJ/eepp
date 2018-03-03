#include <eepp/maps/mapobjectlayer.hpp>
#include <eepp/maps/gameobjectobject.hpp>
#include <eepp/maps/gameobjectpolygon.hpp>
#include <eepp/maps/tilemap.hpp>

#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
using namespace EE::Graphics;

namespace EE { namespace Maps {

MapObjectLayer::MapObjectLayer( TileMap * map, Uint32 flags, std::string name, Vector2f offset ) :
	MapLayer( map, MAP_LAYER_OBJECT, flags, name, offset )
{
}

MapObjectLayer::~MapObjectLayer() {
	deallocateLayer();
}

void MapObjectLayer::allocateLayer() {

}

void MapObjectLayer::deallocateLayer() {
	for ( ObjList::iterator it = mObjects.begin(); it != mObjects.end(); ++it ) {
		eeSAFE_DELETE( *it );
	}
}

void MapObjectLayer::draw( const Vector2f &Offset ) {
	GlobalBatchRenderer::instance()->draw();

	ObjList::iterator it;

	GLi->pushMatrix();
	GLi->translatef( mOffset.x, mOffset.y, 0.0f );

	for ( it = mObjects.begin(); it != mObjects.end(); ++it ) {
		(*it)->draw();
	}

	Texture * Tex = mMap->getBlankTileTexture();

	if ( mMap->getShowBlocked() && NULL != Tex ) {
		Color Col( 255, 0, 0, 200 );

		for ( it = mObjects.begin(); it != mObjects.end(); ++it ) {
			GameObject * Obj = (*it);

			if ( Obj->isBlocked() ) {
				Tex->drawEx( Obj->getPosition().x, Obj->getPosition().y, Obj->getSize().getWidth(), Obj->getSize().getHeight(), 0, Vector2f::One, Col, Col, Col, Col );
			}
		}
	}

	GlobalBatchRenderer::instance()->draw();

	GLi->popMatrix();
}

void MapObjectLayer::update( const Time& dt ) {
	for ( ObjList::iterator it = mObjects.begin(); it != mObjects.end(); ++it ) {
		(*it)->update( dt );
	}
}

Uint32 MapObjectLayer::getObjectCount() const {
	return mObjects.size();
}

void MapObjectLayer::addGameObject( GameObject * obj ) {
	mObjects.push_back( obj );
}

void MapObjectLayer::removeGameObject( GameObject * obj ) {
	mObjects.remove( obj );

	eeSAFE_DELETE( obj );
}

void MapObjectLayer::removeGameObject( const Vector2i& pos ) {
	GameObject * tObj = getObjectOver( pos, SEARCH_OBJECT );

	if ( NULL != tObj ) {
		removeGameObject( tObj );
	}
}

GameObject * MapObjectLayer::getObjectOver( const Vector2i& pos, SEARCH_TYPE type ) {
	GameObject * tObj;
	Vector2f tPos;
	Sizei tSize;

	for ( ObjList::reverse_iterator it = mObjects.rbegin(); it != mObjects.rend(); ++it ) {
		tObj = (*it);

		if ( type & SEARCH_POLY ) {
			if ( tObj->isType( GAMEOBJECT_TYPE_OBJECT ) ) {
				GameObjectObject * tObjObj = reinterpret_cast<GameObjectObject*> ( tObj );

				if ( tObjObj->pointInside( Vector2f( pos.x, pos.y ) ) )
					return tObj;
			}
		} else if ( type & SEARCH_OBJECT ) {
			if ( !tObj->isType( GAMEOBJECT_TYPE_OBJECT ) ) {
				tPos = tObj->getPosition();
				tSize = tObj->getSize();

				Rect objR( tPos.x, tPos.y, tPos.x + tSize.x, tPos.y + tSize.y );

				if ( objR.contains( pos ) )
					return tObj;
			}
		} else {
			if ( tObj->isType( GAMEOBJECT_TYPE_OBJECT ) ) {
				GameObjectObject * tObjObj = reinterpret_cast<GameObjectObject*> ( tObj );

				if ( tObjObj->pointInside( Vector2f( pos.x, pos.y ) ) )
					return tObj;
			} else {
				tPos = tObj->getPosition();
				tSize = tObj->getSize();

				Rect objR( tPos.x, tPos.y, tPos.x + tSize.x, tPos.y + tSize.y );

				if ( objR.contains( pos ) )
					return tObj;
			}
		}
	}

	return NULL;
}

MapObjectLayer::ObjList& MapObjectLayer::getObjectList() {
	return mObjects;
}

}}
