#ifndef EE_GRAPHICSCSHAPEMANAGER_H
#define EE_GRAPHICSCSHAPEMANAGER_H

#include "base.hpp"
#include "cshape.hpp"

namespace EE { namespace Graphics {

class EE_API cShapeManager : public cSingleton<cShapeManager> {
	friend class cSingleton<cShapeManager>;
	public:
		cShapeManager();

		~cShapeManager();

		void Alloc( const Uint32& Size );

		Uint32 GetIndex( const Uint32& Hash );

		Uint32 GetIndex( const std::string& Name );

		cShape * GetAt( const Uint32& Index );

		cShape * Get( const Uint32& Hash );

		cShape * Get( const std::string& Name );

		bool Remove( const Uint32& Hash );

		bool RemoveAt( const Uint32& Index );

		bool Remove( const std::string& Name );

		cShape * Add( cShape * Shape );

		cShape * Add( const Uint32& TexId, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name = "" );

		void Clear();

		Uint32 Count();
	protected:
		std::vector<cShape*> mShapes;

		Uint32 mCount;
};

}}

#endif
