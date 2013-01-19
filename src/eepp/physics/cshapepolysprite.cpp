#include <eepp/physics/cshapepolysprite.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

#include <eepp/graphics/csprite.hpp>

CP_NAMESPACE_BEGIN

cShapePolySprite * cShapePolySprite::New( cBody * body, int numVerts, cVect *verts, cVect offset, cSprite * Sprite, bool AutoDeleteSprite ) {
	return cpNew( cShapePolySprite, ( body, numVerts, verts, offset, Sprite, AutoDeleteSprite ) );
}

cShapePolySprite * cShapePolySprite::New( cBody * body, cpFloat width, cpFloat height, cSprite * Sprite, bool AutoDeleteSprite ) {
	return cpNew( cShapePolySprite, ( body, width, height, Sprite, AutoDeleteSprite ) );
}

cShapePolySprite::cShapePolySprite( cBody * body, int numVerts, cVect *verts, cVect offset, cSprite * Sprite, bool AutoDeleteSprite ) :
	cShapePoly( body, numVerts, verts, offset ),
	mSprite( Sprite ),
	mSpriteAutoDelete( AutoDeleteSprite )
{
	OffsetSet( Centroid( numVerts, verts ) );
}

cShapePolySprite::cShapePolySprite( cBody * body, cpFloat width, cpFloat height, cSprite * Sprite, bool AutoDeleteSprite ) :
	cShapePoly( body, width, height ),
	mSprite( Sprite ),
	mSpriteAutoDelete( AutoDeleteSprite )
{
	mSprite->Size( eeSizef( width, height ) );
	OffsetSet( cVectNew( width / 2, height / 2 ) );
}

cShapePolySprite::~cShapePolySprite() {
	if ( mSpriteAutoDelete )
		eeSAFE_DELETE( mSprite );
}

void cShapePolySprite::Draw( cSpace * space ) {
	cVect Pos = Body()->Pos();

	mSprite->Offset( mOffset );
	mSprite->Position( Pos.x, Pos.y );
	mSprite->Angle( Body()->AngleDeg() );
	mSprite->Draw();
}

void cShapePolySprite::OffsetSet( cVect center ) {
	cVect myCenter = cVectNew( ( mSprite->Size().x / 2 ), ( mSprite->Size().y / 2 ) );

	mOffset = eeVector2i(  (Int32)( -myCenter.x + ( center.x - myCenter.x ) ) , (Int32)( -myCenter.y + ( center.y - myCenter.y ) ) );
}

cSprite * cShapePolySprite::GetSprite() const {
	return mSprite;
}

CP_NAMESPACE_END

#endif
