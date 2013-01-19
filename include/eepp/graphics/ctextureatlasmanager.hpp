#ifndef EE_GRAPHICSCTEXTUREATLASMANAGER_HPP
#define EE_GRAPHICSCTEXTUREATLASMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/csubtexture.hpp>
#include <eepp/graphics/ctextureatlas.hpp>

namespace EE { namespace Graphics {

/** @brief The Texture Atlas Manager is a singleton class that manages all the instances of Texture Atlases instanciated.
	Releases the Texture Atlases instances automatically. So the user doesn't need to release any Texture Atlas instance. */
class EE_API cTextureAtlasManager : public tResourceManager<cTextureAtlas> {
	SINGLETON_DECLARE_HEADERS(cTextureAtlasManager)

	public:
		virtual ~cTextureAtlasManager();

		/** Loads a texture atlas from its path ( the texture atlas binary is expected, not the texture, the ".eta" file ). */
		cTextureAtlas * Load( const std::string& TextureAtlasPath );

		/** Loads a texture atlas from a io stream. */
		cTextureAtlas * LoadFromStream( cIOStream& IOS );

		/** Loads a texture atlas from memory. */
		cTextureAtlas * LoadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName );

		/** Loads a texture atlas from a pack file. */
		cTextureAtlas * LoadFromPack( cPack * Pack, const std::string& FilePackPath );

		/** It will search for a SubTexture Name in the texture atlases loaded.
		*	@return The first SubTexture found with the given name in any atlas. */
		cSubTexture * GetSubTextureByName( const std::string& Name );

		/** It will search for a SubTexture Id in the texture atlases loaded.
		*	@return The first SubTexture found with the given id in any atlas. */
		cSubTexture * GetSubTextureById( const Uint32& Id );

		/** Search for a pattern name
		* @param name First part of the sub texture name
		* @param extension Extension of the sub texture name ( if have one, otherwise is empty )
		* @param SearchInTextureAtlas If you want only to search in a especific atlas ( NULL if you want to search in all atlases )
		* @example Search for name "car" with extensions "png", i will try to find car00.png car01.png car02.png, and so on, it will continue if find something, otherwise it will stop ( it will always search at least for car00.png and car01.png )
		* @note Texture atlases saves the SubTextures names without extension by default.
		*/
		std::vector<cSubTexture*> GetSubTexturesByPattern( const std::string& name, const std::string& extension = "", cTextureAtlas * SearchInTextureAtlas = NULL );

		/** Search for a pattern id.
		*	This will look for the SubTexture with the id passed, and it will try to find any pattern by the SubTexture name.
		*	@see GetSubTexturesByPattern
		*/
		std::vector<cSubTexture*> GetSubTexturesByPatternId( const Uint32& SubTextureId, const std::string& extension = "", cTextureAtlas * SearchInTextureAtlas = NULL );

		/** Prints all the resources name to the screen. */
		void PrintResources();

		/** Sets if the warnings for not finding a resource must be printed in screen. */
		void PrintWarnings( const bool& warn );

		/** @return If warnings are being printed. */
		const bool& PrintWarnings() const;
	protected:
		bool	mWarnings;

		cTextureAtlasManager();
};

}}

#endif
