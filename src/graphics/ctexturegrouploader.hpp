#ifndef EE_GRAPHICSCTEXTUREGROUPLOADER_HPP
#define EE_GRAPHICSCTEXTUREGROUPLOADER_HPP

#include "base.hpp"
#include "packerhelper.hpp"
#include "ctextureloader.hpp"
#include "ctexturefactory.hpp"
#include "../system/cresourceloader.hpp"

namespace EE { namespace Graphics {

using namespace Private;

class EE_API cTextureGroupLoader {
	public:
		cTextureGroupLoader();

		cTextureGroupLoader( const std::string& TextureGroupPath, const bool& Threaded = false );

		cTextureGroupLoader( const Uint8* Data, const Uint32& DataSize, const std::string& TextureGroupName, const bool& Threaded = false );

		cTextureGroupLoader( cPack * Pack, const std::string& FilePackPath, const bool& Threaded = false );

		~cTextureGroupLoader();

		void 				Update();

		void				Load( const std::string& TextureGroupPath = "" );

		void				LoadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureGroupName );

		void				LoadFromPack( cPack * Pack, const std::string& FilePackPath );

		bool				Threaded() const;

		void				Threaded( const bool& threaded );

		const bool&			IsLoaded() const;
		
		const bool&			IsLoading() const;

		/** Will check if the texture atlas is updated ( all the image of the path are inside the texture atlas, and are the same version, otherwise it will recreate or update the texture atlas. */
		bool				UpdateTextureAtlas( std::string TextureAtlasPath, std::string ImagesPath );
	protected:
		cResourceLoader		mRL;
		std::string			mTextureGroupPath;
		bool				mThreaded;
		bool				mLoaded;
		std::string			mAppPath;
		cPack *				mPack;
		bool				mSkipResourceLoad;
		bool				mIsLoading;

		typedef struct sTempTexGroupS {
			sTextureHdr 			Texture;
			std::vector<sShapeHdr>	Shapes;
		} sTempTexGroup;

		sTextureGroupHdr mTexGrHdr;
		std::vector<sTempTexGroup> mTempGroups;

		void CreateShapes();
};

}}

#endif
