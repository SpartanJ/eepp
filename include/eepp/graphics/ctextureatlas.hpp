#ifndef EE_GRAPHICSCTEXTUREATLAS_H
#define EE_GRAPHICSCTEXTUREATLAS_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/csubtexture.hpp>

namespace EE { namespace Graphics {

/** @brief The texture atlas class represents a large image containing a collection of sub-images, or "atlas" which contains many smaller sub-images.
* The texture atlas in eepp can represent more than one texture or image, but the common use should be a image with sub-images.
* More information about Texture Atlases: http://en.wikipedia.org/wiki/Texture_atlas */
class EE_API cTextureAtlas : public ResourceManager<cSubTexture> {
	public:
		/** Creates a new texture atlas with the given name. */
		cTextureAtlas( const std::string& name = "" );

		~cTextureAtlas();

		/** Adds a SubTexture to the Texture Atlas */
		cSubTexture * Add( cSubTexture * subTexture );

		/** Creates and add to the texture atlas a SubTexture from a Texture. It will use the full Texture as a SubTexture.
		*	@param TexId The texture id
		*	@param Name The texture name ( if any )
		*/
		cSubTexture * Add( const Uint32& TexId, const std::string& Name = "" );

		/** Creates and add to the texture atlas a SubTexture of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the SubTexture.
		*	@param Name The texture name ( if any )
		*/
		cSubTexture * Add( const Uint32& TexId, const eeRecti& SrcRect, const std::string& Name = "" );

		/** Creates and add to the texture atlas a SubTexture of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the SubTexture.
		*	@param DestSize The destination size that the SubTexture will have when rendered.
		*	@param Name The texture name ( if any )
		*/
		cSubTexture * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeSizef& DestSize, const std::string& Name = "" );

		/** Creates and add to the texture atlas a SubTexture of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the SubTexture.
		*	@param DestSize The destination size that the SubTexture will have when rendered.
		*	@param Offset The offset that will be added to the position passed when any Draw call is used.
		*	@param Name The texture name ( if any )
		*/
		cSubTexture * Add( const Uint32& TexId, const eeRecti& SrcRect, const eeSizef& DestSize, const eeVector2i& Offset, const std::string& Name = "" );

		/** @return The texture atlas name. */
		const std::string& Name() const;

		/** Sets the texture atlas name. */
		void Name( const std::string& name );

		/** @return The texture atlas path. */
		const std::string& Path() const;

		/** Sets the texture atlas path. */
		void Path( const std::string& path );

		/** @return The texture atlas Id. The Id is the String::Hash of the texture atlas name. */
		const Uint32& Id() const;
		
		/** @return The number of SubTextures inside the texture atlas. */
		Uint32 Count();

		/** @return The texture that corresponds to the texture atlas.
		* @param texnum The texture index. A texture atlas can use more than one texture, so it can be 0 to GetTexturesLoadedCount(). Usually a texture atlas corresponds to only one texture, so the texture index is 0.
		* @note Some texture atlases could not have any texture, since you can use it as a container of SubTextures from any texture. \n
		* The texture atlases loaded from a file always will be linked to a texture. \n
		* The Global Texture Atlas for example doesn't have any texture linked to it.
		*/
		cTexture * GetTexture( const Uint32& texnum = 0 ) const;

		/** @return The number of textures linked to the texture atlas. */
		Uint32 GetTexturesCount();
	protected:
		friend class cTextureAtlasLoader;

		std::string				mName;
		Uint32					mId;
		std::string				mPath;
		std::vector<cTexture*>	mTextures;

		void SetTextures( std::vector<cTexture*> textures );
};

}}

#endif
