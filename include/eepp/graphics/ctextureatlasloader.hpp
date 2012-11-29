#ifndef EE_GRAPHICSCTEXTUREGROUPLOADER_HPP
#define EE_GRAPHICSCTEXTUREGROUPLOADER_HPP

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

		cTextureAtlasLoader( const std::string& TextureGroupPath, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		cTextureAtlasLoader( const Uint8* Data, const Uint32& DataSize, const std::string& TextureGroupName, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		cTextureAtlasLoader( cPack * Pack, const std::string& FilePackPath, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		~cTextureAtlasLoader();

		void					Update();

		void					Load( const std::string& TextureGroupPath = "" );

		void					LoadFromStream( cIOStream& IOS );

		void					LoadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureGroupName );

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
		std::string				mTextureGroupPath;
		bool					mThreaded;
		bool					mLoaded;
		cPack *					mPack;
		bool					mSkipResourceLoad;
		bool					mIsLoading;
		cTextureAtlas *			mTextureAtlas;
		GLLoadCallback			mLoadCallback;
		std::vector<cTexture*>	mTexuresLoaded;

		typedef struct sTempTexGroupS {
			sTextureHdr 			Texture;
			std::vector<sSubTextureHdr>	SubTextures;
		} sTempTexGroup;

		sTextureGroupHdr mTexGrHdr;
		std::vector<sTempTexGroup> mTempGroups;

		void CreateSubTextures();
};

}}

#endif
