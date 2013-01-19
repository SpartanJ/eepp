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

/** @brief The Texture Atlas Loader loads any previously created Texture Atlas. */
class EE_API cTextureAtlasLoader {
	public:
		typedef cb::Callback1<void, cTextureAtlasLoader *> GLLoadCallback;

		/** Creates an empty loader. The texture atlas can be loaded callin any Load* function. */
		cTextureAtlasLoader();

		/** Loads a texture atlas from its path ( the texture atlas binary is expected, not the texture, the ".eta" file ).
		*	If the loader is not threaded, it will load the atlas immediately.
		*	@param TextureAtlasPath The texture atlas path.
		*	@param Threaded Indicates if the loading is done in another thread.
		*	@param LoadCallback The load notification callback.
		*/
		cTextureAtlasLoader( const std::string& TextureAtlasPath, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		/** Loads a texture atlas from memory.
		*	If the loader is not threaded, it will load the atlas immediately.
		*	@param Data The texture atlas buffer pointer
		*	@param DataSize The texture atlas buffer pointer size
		*	@param TextureAtlasName Since the texture atlas is loaded from memory, the name can't be obtained from the file name. So it needs to be indicated manually.
		*	@param Threaded Indicates if the loading is done in another thread.
		*	@param LoadCallback The load notification callback.
		*/
		cTextureAtlasLoader( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		/** Loads a texture atlas from a pack file.
		*	If the loader is not threaded, it will load the atlas immediately.
		*	@param Pack The pointer of the pack instance to be used to load the file.
		*	@param FilePathPath The path of the file inside the pack.
		*	@param Threaded Indicates if the loading is done in another thread.
		*	@param LoadCallback The load notification callback.
		*/
		cTextureAtlasLoader( cPack * Pack, const std::string& FilePackPath, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		/** Loads a texture atlas from a io stream.
		*	If the loader is not threaded, it will load the atlas immediately.
		*	@param IOS The io stream to use for the loading.
		*	@param Threaded Indicates if the loading is done in another thread.
		*	@param LoadCallback The load notification callback.
		*/
		cTextureAtlasLoader( cIOStream& IOS, const bool& Threaded = false, GLLoadCallback LoadCallback = GLLoadCallback() );

		~cTextureAtlasLoader();

		/** In the case that the loader is threaded, to know if the texture atlas was loaded, the main thread must call Update to update the state of the load. And finish the job. */
		void					Update();

		/** Loads a texture atlas from its path ( the texture atlas binary is expected, not the texture, the ".eta" file ).
		*	If the loader is not threaded, it will load the atlas immediately.
		*	@param TextureAtlasPath The texture atlas path.
		*/
		void					Load( const std::string& TextureAtlasPath = "" );

		/** Loads a texture atlas from a io stream.
		*	If the loader is not threaded, it will load the atlas immediately.
		*	@param IOS The io stream to use for the loading.
		*/
		void					LoadFromStream( cIOStream& IOS );

		/** Loads a texture atlas from memory.
		*	If the loader is not threaded, it will load the atlas immediately.
		*	@param Data The texture atlas buffer pointer
		*	@param DataSize The texture atlas buffer pointer size
		*	@param TextureAtlasName Since the texture atlas is loaded from memory, the name can't be obtained from the file name. So it needs to be indicated manually.
		*/
		void					LoadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName );

		/** Loads a texture atlas from a pack file.
		*	If the loader is not threaded, it will load the atlas immediately.
		*	@param Pack The pointer of the pack instance to be used to load the file.
		*	@param FilePathPath The path of the file inside the pack.
		*/
		void					LoadFromPack( cPack * Pack, const std::string& FilePackPath );

		/** @return If the loader is threaded ( asynchronous ). */
		bool					Threaded() const;

		/** If threaded is true sets the loader as asynchronous. This must be called before the loading is done. */
		void					Threaded( const bool& threaded );

		/** @return True if the texture atlas is loaded. */
		const bool&				IsLoaded() const;
		
		/** @return True if the texture atlas is loading. */
		const bool&				IsLoading() const;

		/** @brief The function will check if the texture atlas is updated.
			Checks if all the images inside the images path are inside the texture atlas, and if they have the same date and size, otherwise it will recreate or update the texture atlas.
		*/
		bool					UpdateTextureAtlas( std::string TextureAtlasPath, std::string ImagesPath );

		/** Rewrites the texture atlas file. Usefull if the SubTextures where modified and need to be updated inside the texture atlas. */
		bool					UpdateTextureAtlas();

		/** @return The texture that corresponds to the texture atlas.
		* @param texnum The texture index. A texture atlas can use more than one texture, so it can be 0 to GetTexturesLoadedCount(). Usually a texture atlas corresponds to only one texture, so the texture index is 0.
		*/
		cTexture *				GetTexture( const Uint32& texnum = 0 ) const;

		/** @return The number of textures linked to the texture atlas. */
		Uint32					GetTexturesLoadedCount();

		/** @return The texture atlas instance pointer ( NULL if the atlas isn't loaded yet ). */
		cTextureAtlas *			GetTextureAtlas() const;

		/** Sets a load notification callback. */
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
