#ifndef EE_GRAPHICSCTEXTUREATLASMANAGER_HPP
#define EE_GRAPHICSCTEXTUREATLASMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/csubtexture.hpp>
#include <eepp/graphics/ctextureatlas.hpp>

namespace EE { namespace Graphics {

class EE_API cTextureAtlasManager : public tResourceManager<cTextureAtlas> {
	SINGLETON_DECLARE_HEADERS(cTextureAtlasManager)

	public:
		cTextureAtlasManager();

		virtual ~cTextureAtlasManager();

		cSubTexture * GetSubTextureByName( const std::string& Name );

		cSubTexture * GetSubTextureById( const Uint32& Id );

		/** Search for a pattern name
		* @param name First part of the sub texture name
		* @param extension Extension of the sub texture name ( if have one, otherwise "" )
		* @param SearchInTextureAtlas If you want only to search in a especific atlas ( NULL if you want to search in all atlases )
		* @example Search for name "car" with extensions "png", i will try to find car00.png car01.png car02.png, and so on, it will continue if find something, otherwise it will stop ( it will always search at least for car00.png and car01.png )
		*/
		std::vector<cSubTexture*> GetSubTexturesByPattern( const std::string& name, const std::string& extension = "", cTextureAtlas * SearchInTextureAtlas = NULL );

		std::vector<cSubTexture*> GetSubTexturesByPatternId( const Uint32& SubTextureId, const std::string& extension = "", cTextureAtlas * SearchInTextureAtlas = NULL );

		void PrintResources();

		void PrintWarnings( const bool& warn );

		const bool& PrintWarnings() const;
	protected:
		bool	mWarnings;
};

}}

#endif
