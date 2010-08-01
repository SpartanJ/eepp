#ifndef EE_GRAPHICSCSHAPEMANAGER_H
#define EE_GRAPHICSCSHAPEMANAGER_H

#include "base.hpp"
#include "cshape.hpp"

#define SHAPE_NONE 0xFFFFFFFF

namespace EE { namespace Graphics {

class EE_API cShapeGroup : public tResourceManager<cShape> {
	public:
		cShapeGroup( const std::string& name = "" );

		~cShapeGroup();

		cShape * Add( cShape * Shape );

		cShape * Add( const Uint32& TexId, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name = "" );

		cShape * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name = "" );

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;
	protected:
		std::string 	mName;
		Uint32 			mId;
};

}}

#endif
