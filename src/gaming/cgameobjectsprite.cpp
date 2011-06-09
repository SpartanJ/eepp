#include "cgameobjectsprite.hpp"
#include "../graphics/cshapegroupmanager.hpp"

namespace EE { namespace Gaming {

cGameObjectSprite::cGameObjectSprite( const Uint32& Flags, cSprite * Sprite ) :
	cGameObject( Flags ),
	mSprite( Sprite )
{
	if ( NULL != mSprite )
		mSprite->SetRenderType( RenderTypeFromFlags() );
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
	if ( NULL != mSprite )
		return mSprite->Position();

	return eeVector2f();
}

void cGameObjectSprite::Pos( eeVector2f pos ) {
	if ( NULL != mSprite )
		mSprite->Position( pos );
}

eeSize cGameObjectSprite::Size() {
	if ( NULL != mSprite )
		return mSprite->GetShape(0)->RealSize();

	return eeSize();
}

cSprite * cGameObjectSprite::Sprite() const {
	return mSprite;
}

void cGameObjectSprite::Sprite( cSprite * sprite ) {
	eeSAFE_DELETE( mSprite );
	mSprite = sprite;
}

void cGameObjectSprite::FlagSet( const Uint32& Flag ) {
	if ( NULL != mSprite )
		mSprite->SetRenderType( RenderTypeFromFlags() );

	cGameObject::FlagSet( Flag );
}

Uint32 cGameObjectSprite::DataId() {
	return mSprite->GetShape(0)->Id();
}

void cGameObjectSprite::DataId( Uint32 Id ) {
	std::vector<cShape*> tShapeVec = cShapeGroupManager::instance()->GetShapesByPatternId( Id );

	if ( tShapeVec.size() ) {
		cSprite * tSprite = eeNew( cSprite, () );
		tSprite->CreateAnimation();
		tSprite->AddFrames( tShapeVec );
	}
}

}}
