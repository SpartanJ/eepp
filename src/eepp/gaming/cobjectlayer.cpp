#include <eepp/gaming/cobjectlayer.hpp>
#include <eepp/gaming/cgameobjectobject.hpp>
#include <eepp/gaming/cgameobjectpolygon.hpp>
#include <eepp/gaming/cmap.hpp>

#include <eepp/graphics/ctexture.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cObjectLayer::cObjectLayer( cMap * map, Uint32 flags, std::string name, eeVector2f offset ) :
	cLayer( map, MAP_LAYER_OBJECT, flags, name, offset )
{
}

cObjectLayer::~cObjectLayer() {
	DeallocateLayer();
}

void cObjectLayer::AllocateLayer() {

}

void cObjectLayer::DeallocateLayer() {
	for ( ObjList::iterator it = mObjects.begin(); it != mObjects.end(); it++ ) {
		eeSAFE_DELETE( *it );
	}
}

void cObjectLayer::Draw( const eeVector2f &Offset ) {
	cGlobalBatchRenderer::instance()->Draw();

	ObjList::iterator it;

	GLi->PushMatrix();
	GLi->Translatef( mOffset.x, mOffset.y, 0.0f );

	for ( it = mObjects.begin(); it != mObjects.end(); it++ ) {
		(*it)->Draw();
	}

	cTexture * Tex = mMap->GetBlankTileTexture();

	if ( mMap->ShowBlocked() && NULL != Tex ) {
		eeColorA Col( 255, 0, 0, 200 );

		for ( it = mObjects.begin(); it != mObjects.end(); it++ ) {
			cGameObject * Obj = (*it);

			if ( Obj->Blocked() ) {
				Tex->DrawEx( Obj->Pos().x, Obj->Pos().y, Obj->Size().Width(), Obj->Size().Height(), 0, 1, Col, Col, Col, Col );
			}
		}
	}

	cGlobalBatchRenderer::instance()->Draw();

	GLi->PopMatrix();
}

void cObjectLayer::Update() {
	for ( ObjList::iterator it = mObjects.begin(); it != mObjects.end(); it++ ) {
		(*it)->Update();
	}
}

Uint32 cObjectLayer::GetObjectCount() const {
	return mObjects.size();
}

void cObjectLayer::AddGameObject( cGameObject * obj ) {
	mObjects.push_back( obj );
}

void cObjectLayer::RemoveGameObject( cGameObject * obj ) {
	mObjects.remove( obj );

	eeSAFE_DELETE( obj );
}

void cObjectLayer::RemoveGameObject( const eeVector2i& pos ) {
	cGameObject * tObj = GetObjectOver( pos, SEARCH_OBJECT );

	if ( NULL != tObj ) {
		RemoveGameObject( tObj );
	}
}

cGameObject * cObjectLayer::GetObjectOver( const eeVector2i& pos, SEARCH_TYPE type ) {
	cGameObject * tObj;
	eeVector2f tPos;
	eeSize tSize;

	for ( ObjList::reverse_iterator it = mObjects.rbegin(); it != mObjects.rend(); it++ ) {
		tObj = (*it);

		if ( type & SEARCH_POLY ) {
			if ( tObj->IsType( GAMEOBJECT_TYPE_OBJECT ) ) {
				cGameObjectObject * tObjObj = reinterpret_cast<cGameObjectObject*> ( tObj );

				if ( tObjObj->PointInside( eeVector2f( pos.x, pos.y ) ) )
					return tObj;
			}
		} else if ( type & SEARCH_OBJECT ) {
			if ( !tObj->IsType( GAMEOBJECT_TYPE_OBJECT ) ) {
				tPos = tObj->Pos();
				tSize = tObj->Size();

				eeRecti objR( tPos.x, tPos.y, tPos.x + tSize.x, tPos.y + tSize.y );

				if ( objR.Contains( pos ) )
					return tObj;
			}
		} else {
			if ( tObj->IsType( GAMEOBJECT_TYPE_OBJECT ) ) {
				cGameObjectObject * tObjObj = reinterpret_cast<cGameObjectObject*> ( tObj );

				if ( tObjObj->PointInside( eeVector2f( pos.x, pos.y ) ) )
					return tObj;
			} else {
				tPos = tObj->Pos();
				tSize = tObj->Size();

				eeRecti objR( tPos.x, tPos.y, tPos.x + tSize.x, tPos.y + tSize.y );

				if ( objR.Contains( pos ) )
					return tObj;
			}
		}
	}

	return NULL;
}

cObjectLayer::ObjList& cObjectLayer::GetObjectList() {
	return mObjects;
}

}}
