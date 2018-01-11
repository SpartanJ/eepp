#ifndef EE_GRAPHICSCTEXTUREATLASMANAGER_HPP
#define EE_GRAPHICSCTEXTUREATLASMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textureatlas.hpp>

#include <eepp/system/singleton.hpp>
#include <eepp/system/pack.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The Texture Atlas Manager is a singleton class that manages all the instances of Texture Atlases instanciated.
	Releases the Texture Atlases instances automatically. So the user doesn't need to release any Texture Atlas instance. */
class EE_API TextureAtlasManager : public ResourceManager<TextureAtlas> {
	SINGLETON_DECLARE_HEADERS(TextureAtlasManager)

	public:
		virtual ~TextureAtlasManager();

		/** Loads a texture atlas from its path ( the texture atlas binary is expected, not the texture, the ".eta" file ). */
		TextureAtlas * loadFromFile( const std::string& TextureAtlasPath );

		/** Loads a texture atlas from a io stream. */
		TextureAtlas * loadFromStream( IOStream& IOS );

		/** Loads a texture atlas from memory. */
		TextureAtlas * loadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName );

		/** Loads a texture atlas from a pack file. */
		TextureAtlas * loadFromPack( Pack * Pack, const std::string& FilePackPath );

		/** It will search for a SubTexture Name in the texture atlases loaded.
		*	@return The first SubTexture found with the given name in any atlas. */
		SubTexture * getSubTextureByName( const std::string& Name );

		/** It will search for a SubTexture Id in the texture atlases loaded.
		*	@return The first SubTexture found with the given id in any atlas. */
		SubTexture * getSubTextureById( const Uint32& Id );

		/** Search for a pattern name
		* For example search for name "car" with extensions "png", i will try to find car00.png car01.png car02.png, and so on, it will continue if find something, otherwise it will stop ( it will always search at least for car00.png and car01.png )
		* @param name First part of the sub texture name
		* @param extension Extension of the sub texture name ( if have one, otherwise is empty )
		* @param SearchInTextureAtlas If you want only to search in a especific atlas ( NULL if you want to search in all atlases )
		* @note Texture atlases saves the SubTextures names without extension by default.
		*/
		std::vector<SubTexture*> getSubTexturesByPattern( const std::string& name, const std::string& extension = "", TextureAtlas * SearchInTextureAtlas = NULL );

		/** Search for a pattern id.
		*	This will look for the SubTexture with the id passed, and it will try to find any pattern by the SubTexture name.
		*	@see GetSubTexturesByPattern
		*/
		std::vector<SubTexture*> getSubTexturesByPatternId( const Uint32& SubTextureId, const std::string& extension = "", TextureAtlas * SearchInTextureAtlas = NULL );

		/** Prints all the resources name to the screen. */
		void printResources();

		/** Sets if the warnings for not finding a resource must be printed in screen. */
		void setPrintWarnings( const bool& warn );

		/** @return If warnings are being printed. */
		const bool& getPrintWarnings() const;
	protected:
		bool	mWarnings;

		TextureAtlasManager();
};

}}

#endif
