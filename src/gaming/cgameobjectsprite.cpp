#include "cgameobjectsprite.hpp"

namespace EE { namespace Gaming {

cGameObjectSprite::cGameObjectSprite( const Uint32& Flags, cSprite * Sprite ) :
	cGameObject( Flags ),
	mSprite( Sprite )
{
}

cGameObjectSprite::~cGameObjectSprite() {
	eeSAFE_DELETE( mSprite );
}

Uint32 cGameObjectSprite::Type() const {
	return GAMEOBJECT_TYPE_SPRITE;
}

void cGameObjectSprite::Draw() {
	if ( NULL != mSprite ) {
		mSprite->Draw();
	}
}

void cGameObjectSprite::Update() {
}

eeVector2f cGameObjectSprite::Pos() const {
	eeASSERT( NULL != mSprite )

	return mSprite->Position();
}

void cGameObjectSprite::Pos( eeVector2f pos ) {
	eeASSERT( NULL != mSprite );

	mSprite->Position( pos );
}

cSprite * cGameObjectSprite::Sprite() const {
	return mSprite;
}

void cGameObjectSprite::Sprite( cSprite * sprite ) {
	mSprite = sprite;
}

}}
