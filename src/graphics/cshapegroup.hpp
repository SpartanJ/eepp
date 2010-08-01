#ifndef EE_GRAPHICSCSHAPEMANAGER_H
#define EE_GRAPHICSCSHAPEMANAGER_H

#include "base.hpp"
#include "cshape.hpp"

#define SHAPE_NONE 0xFFFFFFFF

namespace EE { namespace Graphics {

class EE_API cShapeGroup {
	public:
		cShapeGroup( const std::string& name = "", const Uint32& Allocate = 0 );

		~cShapeGroup();

		void Allocate( const Uint32& Size );

		void Destroy();

		Uint32 GetIndex( const Uint32& Hash );

		Uint32 GetIndex( const std::string& Name );

		cShape * GetAt( const Uint32& Index );

		cShape * GetById( const Uint32& Hash );

		cShape * GetByName( const std::string& Name );

		bool RemoveById( const Uint32& Hash );

		bool RemoveAt( const Uint32& Index );

		bool RemoveByName( const std::string& Name );

		cShape * Add( cShape * Shape );

		cShape * Add( const Uint32& TexId, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name = "" );

		void Clear();

		const Int32& Count() const;

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;
	protected:
		std::string 			mName;
		Uint32 					mId;
		std::vector<cShape*> 	mShapes;
		std::queue<Uint32>		mVectorFreeSlots;
		Int32					mCount;

		Uint32 FindFreeSlot();
};

}}

#endif
