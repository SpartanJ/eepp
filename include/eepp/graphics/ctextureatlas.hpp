#ifndef EE_GRAPHICSCTEXTUREATLAS_H
#define EE_GRAPHICSCTEXTUREATLAS_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/csubtexture.hpp>

#define SUBTEXTURE_NONE 0xFFFFFFFF

namespace EE { namespace Graphics {

class EE_API cTextureAtlas : public tResourceManager<cSubTexture> {
	public:
		cTextureAtlas( const std::string& name = "" );

		~cTextureAtlas();

		cSubTexture * Add( cSubTexture * subTexture );

		cSubTexture * Add( const Uint32& TexId, const std::string& Name = "" );

		cSubTexture * Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name = "" );

		cSubTexture * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const std::string& Name = "" );

		cSubTexture * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& OffsetX, const eeFloat& OffsetY, const std::string& Name = "" );

		const std::string& Name() const;

		void Name( const std::string& name );

		const std::string& Path() const;

		void Path( const std::string& path );

		const Uint32& Id() const;
		
		Uint32 Count();
	protected:
		std::string 	mName;
		Uint32 			mId;
		std::string		mPath;
};

}}

#endif
