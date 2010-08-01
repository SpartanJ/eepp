#include "cshapegroup.hpp"

namespace EE { namespace Graphics {

cShapeGroup::cShapeGroup( const std::string& name, const Uint32& Allocate ) :
	mCount(0)
{
	Name( name );
	this->Allocate( Allocate );
}

cShapeGroup::~cShapeGroup() {
	Destroy();
}

void cShapeGroup::Destroy() {
	for ( Uint32 i = 0; i < mShapes.size(); i++ )
		RemoveAt(i);
}

const std::string& cShapeGroup::Name() const {
	return mName;
}

void cShapeGroup::Name( const std::string& name ) {
	mName = name;
	mId = MakeHash( mName );
}

const Uint32& cShapeGroup::Id() const {
	return mId;
}

void cShapeGroup::Allocate( const Uint32& Size ) {
	if ( Size > mShapes.size() ) {
		mShapes.resize( Size, NULL );

		for ( eeUint i = 0; i < mShapes.size(); i++ )
			mVectorFreeSlots.push( i );
	}
}

Uint32 cShapeGroup::FindFreeSlot() {
	if ( mVectorFreeSlots.size() ) {
		Uint32 Pos = mVectorFreeSlots.front();

		mVectorFreeSlots.pop();

		return Pos;
	}

	mShapes.push_back( NULL );

	return mShapes.size() - 1;
}

Uint32 cShapeGroup::GetIndex( const Uint32& Hash ) {
	if ( 0 != Hash ) {
		for ( Uint32 i = 0; i < mShapes.size(); i++ ) {
			if ( NULL != mShapes[i] && mShapes[i]->Id() == Hash )
				return i;
		}
	}

	return SHAPE_NONE;
}

Uint32 cShapeGroup::GetIndex( const std::string& Name ) {
	return GetIndex( MakeHash( Name ) );
}

cShape * cShapeGroup::GetAt( const Uint32& Index ) {
	// assert here

	return mShapes[ Index ];
}

cShape * cShapeGroup::GetById( const Uint32& Hash ) {
	if ( 0 != Hash ) {
		for ( Uint32 i = mShapes.size() - 1; i >= 0; i-- ) {
			if ( NULL != mShapes[i] && mShapes[i]->Id() == Hash )
				return mShapes[i];
		}
	}

	return NULL;
}

cShape * cShapeGroup::GetByName( const std::string& Name ) {
	return GetById( MakeHash( Name ) );
}

bool cShapeGroup::RemoveById( const Uint32& Hash ) {
	Uint32 Pos = GetIndex( Hash );

	if ( Pos != SHAPE_NONE )
		return RemoveAt( Pos );

	return false;
}

bool cShapeGroup::RemoveAt( const Uint32& Index ) {
	if ( Index < mShapes.size() ) {
		eeSAFE_DELETE( mShapes[ Index ] );

		mVectorFreeSlots.push( Index );

		mCount--;

		return true;
	}

	return false;
}

bool cShapeGroup::RemoveByName( const std::string& Name ) {
	return RemoveById( MakeHash( Name ) );
}

cShape * cShapeGroup::Add( cShape * Shape ) {
	if ( NULL != Shape ) {
		mShapes[ FindFreeSlot() ] = Shape;

		mCount++;
	}

	return Shape;
}

cShape * cShapeGroup::Add( const Uint32& TexId, const std::string& Name ) {
	return Add( new cShape( TexId, Name ) );
}

cShape * cShapeGroup::Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name ) {
	return Add( new cShape( TexId, SrcRect, Name ) );
}

cShape * cShapeGroup::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name ) {
	return Add( new cShape( TexId, SrcRect, DestWidth, DestHeight, Name ) );
}

cShape * cShapeGroup::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name ) {
	return Add( new cShape( TexId, SrcRect, DestWidth, DestHeight, OffsetX, OffsetY, Name ) );
}

void cShapeGroup::Clear() {
	for ( Uint32 i = 0; i < mShapes.size(); i++ )
		RemoveAt(i);
}

const Int32& cShapeGroup::Count() const {
	return mCount;
}

}}
