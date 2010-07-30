#include "cshapemanager.hpp"

namespace EE { namespace Graphics {

cShapeManager::cShapeManager() :
	mCount(0)
{
	Alloc( 256 );
}

cShapeManager::~cShapeManager() {
}

void cShapeManager::Alloc( const Uint32& Size ) {
	mShapes.resize( Size );
}

Uint32 cShapeManager::GetIndex( const Uint32& Hash ) {
	if ( 0 != Hash ) {
		for ( Uint32 i = 0; i < mShapes.size(); i++ ) {
			if ( NULL != mShapes[i] && mShapes[i]->Id() == Hash )
				return i;
		}
	}

	return (Uint32)-1;
}

Uint32 cShapeManager::GetIndex( const std::string& Name ) {
	return GetIndex( MakeHash( Name ) );
}

cShape * cShapeManager::GetAt( const Uint32& Index ) {
	// assert here

	return mShapes[ Index ];
}

cShape * cShapeManager::Get( const Uint32& Hash ) {
	if ( 0 != Hash ) {
		for ( Uint32 i = mShapes.size() - 1; i >= 0; i-- ) {
			if ( NULL != mShapes[i] && mShapes[i]->Id() == Hash )
				return mShapes[i];
		}
	}

	return NULL;
}

cShape * cShapeManager::Get( const std::string& Name ) {
	return Get( MakeHash( Name ) );
}

bool cShapeManager::Remove( const Uint32& Hash ) {
	Uint32 Pos = GetIndex( Hash );

	if ( Pos != (Uint32)-1 ) {
		eeSAFE_DELETE( mShapes[ Pos ] );

		return true;
	}

	return false;
}

bool cShapeManager::RemoveAt( const Uint32& Index ) {
	if ( Index < mShapes.size() ) {
		eeSAFE_DELETE( mShapes[ Index ] );

		return true;
	}

	return false;
}

bool cShapeManager::Remove( const std::string& Name ) {
	return Remove( MakeHash( Name ) );
}

cShape * cShapeManager::Add( cShape * Shape ) {
	if ( NULL != Shape ) {
		if ( mCount < mShapes.size() ) {
			mShapes[ mCount ] = Shape;
			mCount++;
		} else {
			mShapes.push_back( Shape );
			mCount++;
		}
	}

	return Shape;
}

cShape * cShapeManager::Add( const Uint32& TexId, const std::string& Name ) {
	return Add( new cShape( TexId, Name ) );
}

cShape * cShapeManager::Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name ) {
	return Add( new cShape( TexId, SrcRect, Name ) );
}

cShape * cShapeManager::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name ) {
	return Add( new cShape( TexId, SrcRect, DestWidth, DestHeight, Name ) );
}

cShape * cShapeManager::Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name ) {
	return Add( new cShape( TexId, SrcRect, DestWidth, DestHeight, OffsetX, OffsetY, Name ) );
}

void cShapeManager::Clear() {
	for ( Uint32 i = 0; i < mShapes.size(); i++ ) {
		if ( mShapes[i] != NULL )
			eeSAFE_DELETE( mShapes[i] );
	}
}

Uint32 cShapeManager::Count() {
	return mCount;
}

}}
