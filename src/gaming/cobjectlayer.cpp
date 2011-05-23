#include "cobjectlayer.hpp"
#include "cmap.hpp"

#include "../graphics/cglobalbatchrenderer.hpp"
#include "../graphics/renderer/cgl.hpp"
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cObjectLayer::cObjectLayer( cMap * map, Uint32 flags, std::string name, eeVector2f offset ) :
	cLayer( map, MAP_LAYER_OBJECT, flags, name, offset )
{
}

cObjectLayer::~cObjectLayer() {
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

	GLi->LoadIdentity();
	GLi->PushMatrix();
	GLi->Translatef( mOffset.x + Offset.x, mOffset.y + Offset.y, 0.0f );

	for ( ObjList::iterator it = mObjects.begin(); it != mObjects.end(); it++ ) {
		(*it)->Draw();
	}

	cGlobalBatchRenderer::instance()->Draw();

	GLi->PopMatrix();
}

void cObjectLayer::Update() {
	for ( ObjList::iterator it = mObjects.begin(); it != mObjects.end(); it++ ) {
		(*it)->Update();
	}
}

void cObjectLayer::AddGameObject( cGameObject * obj ) {
	mObjects.push_back( obj );
}

void cObjectLayer::RemoveGameObject( cGameObject * obj ) {
	mObjects.remove( obj );
}

}}
