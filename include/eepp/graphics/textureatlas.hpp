#ifndef EE_GRAPHICSCTEXTUREATLAS_H
#define EE_GRAPHICSCTEXTUREATLAS_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/system/resourcemanager.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The texture atlas class represents a large image containing a collection of sub-images, or "atlas" which contains many smaller sub-images.
* The texture atlas in eepp can represent more than one texture or image, but the common use should be a image with sub-images.
* More information about Texture Atlases: http://en.wikipedia.org/wiki/Texture_atlas */
class EE_API TextureAtlas : public ResourceManager<TextureRegion> {
	public:
		static TextureAtlas * New( const std::string& name = "" );

		/** Creates a new texture atlas with the given name. */
		TextureAtlas( const std::string& name = "" );

		~TextureAtlas();

		/** Adds a TextureRegion to the Texture Atlas */
		TextureRegion * add( TextureRegion * textureRegion );

		/** Creates and add to the texture atlas a TextureRegion from a Texture. It will use the full Texture as a TextureRegion.
		*	@param TexId The texture id
		*	@param Name The texture name ( if any )
		*/
		TextureRegion * add( const Uint32& TexId, const std::string& Name = "" );

		/** Creates and add to the texture atlas a TextureRegion of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the TextureRegion.
		*	@param Name The texture name ( if any )
		*/
		TextureRegion * add( const Uint32& TexId, const Rect& SrcRect, const std::string& Name = "" );

		/** Creates and add to the texture atlas a TextureRegion of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the TextureRegion.
		*	@param DestSize The destination size that the TextureRegion will have when rendered.
		*	@param Name The texture name ( if any )
		*/
		TextureRegion * add( const Uint32& TexId, const Rect& SrcRect, const Sizef& DestSize, const std::string& Name = "" );

		/** Creates and add to the texture atlas a TextureRegion of the indicated part of the texture.
		*	@param TexId The texture id
		*	@param SrcRect The texture part that will be used as the TextureRegion.
		*	@param DestSize The destination size that the TextureRegion will have when rendered.
		*	@param Offset The offset that will be added to the position passed when any Draw call is used.
		*	@param Name The texture name ( if any )
		*/
		TextureRegion * add( const Uint32& TexId, const Rect& SrcRect, const Sizef& DestSize, const Vector2i& Offset, const std::string& Name = "" );

		/** @return The texture atlas name. */
		const std::string& getName() const;

		/** Sets the texture atlas name. */
		void setName( const std::string& name );

		/** @return The texture atlas path. */
		const std::string& getPath() const;

		/** Sets the texture atlas path. */
		void setPath( const std::string& path );

		/** @return The texture atlas Id. The Id is the String::hash of the texture atlas name. */
		const Uint32& getId() const;
		
		/** @return The number of TextureRegions inside the texture atlas. */
		Uint32 getCount();

		/** @return The texture that corresponds to the texture atlas.
		* @param texnum The texture index. A texture atlas can use more than one texture, so it can be 0 to GetTexturesLoadedCount(). Usually a texture atlas corresponds to only one texture, so the texture index is 0.
		* @note Some texture atlases could not have any texture, since you can use it as a container of TextureRegions from any texture. \n
		* The texture atlases loaded from a file always will be linked to a texture. \n
		* The Global Texture Atlas for example doesn't have any texture linked to it.
		*/
		Texture * getTexture( const Uint32& texnum = 0 ) const;

		/** @return The number of textures linked to the texture atlas. */
		Uint32 getTexturesCount();
	protected:
		friend class TextureAtlasLoader;

		std::string				mName;
		Uint32					mId;
		std::string				mPath;
		std::vector<Texture*>	mTextures;

		void setTextures( std::vector<Texture*> textures );
};

}}

#endif
