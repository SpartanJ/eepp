#ifndef EE_GRAPHICSCTEXTUREATLASLOADER_HPP
#define EE_GRAPHICSCTEXTUREATLASLOADER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/packerhelper.hpp>
#include <eepp/graphics/ctextureloader.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/system/cresourceloader.hpp>
#include <eepp/system/ciostream.hpp>

namespace EE { namespace Graphics {

using namespace Private;

class cTextureAtlas;

class EE_API cTextureAtlasLoader {
	public:
		typedef cb::Callback1<void, cTextureAtlasLoader *> GLLoadCallback;

		cTextureAtlasLoader();

		cTextureAtlasLoader( const std::string& TextureAtlasPath, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		cTextureAtlasLoader( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		cTextureAtlasLoader( cPack * Pack, const std::string& FilePackPath, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		cTextureAtlasLoader( cIOStream& IOS, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		~cTextureAtlasLoader();

		void					Update();

		void					Load( const std::string& TextureAtlasPath = "" );

		void					LoadFromStream( cIOStream& IOS );

		void					LoadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName );

		void					LoadFromPack( cPack * Pack, const std::string& FilePackPath );

		bool					Threaded() const;

		void					Threaded( const bool& threaded );

		const bool&				IsLoaded() const;
		
		const bool&				IsLoading() const;

		/** Will check if the texture atlas is updated ( all the image of the path are inside the texture atlas, and are the same version, otherwise it will recreate or update the texture atlas. */
		bool					UpdateTextureAtlas( std::string TextureAtlasPath, std::string ImagesPath );

		bool					UpdateTextureAtlas();

		cTexture *				GetTexture( const Uint32& texnum = 0 ) const;

		Uint32					GetTexturesLoadedCount();

		cTextureAtlas *			GetTextureAtlas() const;

		void					SetLoadCallback( GLLoadCallback LoadCallback );
	protected:
		cResourceLoader			mRL;
		std::string				mTextureAtlasPath;
		bool					mThreaded;
		bool					mLoaded;
		cPack *					mPack;
		bool					mSkipResourceLoad;
		bool					mIsLoading;
		cTextureAtlas *			mTextureAtlas;
		GLLoadCallback			mLoadCallback;
		std::vector<cTexture*>	mTexuresLoaded;

		typedef struct sTempTexAtlasS {
			sTextureHdr 			Texture;
			std::vector<sSubTextureHdr>	SubTextures;
		} sTempTexAtlas;

		sTextureAtlasHdr mTexGrHdr;
		std::vector<sTempTexAtlas> mTempAtlass;

		void CreateSubTextures();
};

}}

#endif
