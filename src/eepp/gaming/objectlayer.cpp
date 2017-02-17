#include <eepp/gaming/mapobjectlayer.hpp>
#include <eepp/gaming/gameobjectobject.hpp>
#include <eepp/gaming/gameobjectpolygon.hpp>
#include <eepp/gaming/tilemap.hpp>

#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/gl.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

MapObjectLayer::MapObjectLayer( TileMap * map, Uint32 flags, std::string name, Vector2f offset ) :
	MapLayer( map, MAP_LAYER_OBJECT, flags, name, offset )
{
}

MapObjectLayer::~MapObjectLayer() {
	DeallocateLayer();
}

void MapObjectLayer::AllocateLayer() {

}

void MapObjectLayer::DeallocateLayer() {
	for ( ObjList::iterator it = mObjects.begin(); it != mObjects.end(); it++ ) {
		eeSAFE_DELETE( *it );
	}
}

void MapObjectLayer::Draw( const Vector2f &Offset ) {
	GlobalBatchRenderer::instance()->draw();

	ObjList::iterator it;

	GLi->pushMatrix();
	GLi->translatef( mOffset.x, mOffset.y, 0.0f );

	for ( it = mObjects.begin(); it != mObjects.end(); it++ ) {
		(*it)->Draw();
	}

	Texture * Tex = mMap->GetBlankTileTexture();

	if ( mMap->ShowBlocked() && NULL != Tex ) {
		ColorA Col( 255, 0, 0, 200 );

		for ( it = mObjects.begin(); it != mObjects.end(); it++ ) {
			GameObject * Obj = (*it);

			if ( Obj->Blocked() ) {
				Tex->drawEx( Obj->Pos().x, Obj->Pos().y, Obj->Size().getWidth(), Obj->Size().getHeight(), 0, Vector2f::One, Col, Col, Col, Col );
			}
		}
	}

	GlobalBatchRenderer::instance()->draw();

	GLi->popMatrix();
}

void MapObjectLayer::Update() {
	for ( ObjList::iterator it = mObjects.begin(); it != mObjects.end(); it++ ) {
		(*it)->Update();
	}
}

Uint32 MapObjectLayer::GetObjectCount() const {
	return mObjects.size();
}

void MapObjectLayer::AddGameObject( GameObject * obj ) {
	mObjects.push_back( obj );
}

void MapObjectLayer::RemoveGameObject( GameObject * obj ) {
	mObjects.remove( obj );

	eeSAFE_DELETE( obj );
}

void MapObjectLayer::RemoveGameObject( const Vector2i& pos ) {
	GameObject * tObj = GetObjectOver( pos, SEARCH_OBJECT );

	if ( NULL != tObj ) {
		RemoveGameObject( tObj );
	}
}

GameObject * MapObjectLayer::GetObjectOver( const Vector2i& pos, SEARCH_TYPE type ) {
	GameObject * tObj;
	Vector2f tPos;
	Sizei tSize;

	for ( ObjList::reverse_iterator it = mObjects.rbegin(); it != mObjects.rend(); it++ ) {
		tObj = (*it);

		if ( type & SEARCH_POLY ) {
			if ( tObj->IsType( GAMEOBJECT_TYPE_OBJECT ) ) {
				GameObjectObject * tObjObj = reinterpret_cast<GameObjectObject*> ( tObj );

				if ( tObjObj->PointInside( Vector2f( pos.x, pos.y ) ) )
					return tObj;
			}
		} else if ( type & SEARCH_OBJECT ) {
			if ( !tObj->IsType( GAMEOBJECT_TYPE_OBJECT ) ) {
				tPos = tObj->Pos();
				tSize = tObj->Size();

				Recti objR( tPos.x, tPos.y, tPos.x + tSize.x, tPos.y + tSize.y );

				if ( objR.contains( pos ) )
					return tObj;
			}
		} else {
			if ( tObj->IsType( GAMEOBJECT_TYPE_OBJECT ) ) {
				GameObjectObject * tObjObj = reinterpret_cast<GameObjectObject*> ( tObj );

				if ( tObjObj->PointInside( Vector2f( pos.x, pos.y ) ) )
					return tObj;
			} else {
				tPos = tObj->Pos();
				tSize = tObj->Size();

				Recti objR( tPos.x, tPos.y, tPos.x + tSize.x, tPos.y + tSize.y );

				if ( objR.contains( pos ) )
					return tObj;
			}
		}
	}

	return NULL;
}

MapObjectLayer::ObjList& MapObjectLayer::GetObjectList() {
	return mObjects;
}

}}
