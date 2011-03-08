#ifndef EE_GRAPHICSCTEXTURELOADER
#define EE_GRAPHICSCTEXTURELOADER

#include "base.hpp"
#include "../system/cobjectloader.hpp"

namespace EE { namespace Graphics {

#define TEX_LT_PATH 	(1)
#define TEX_LT_MEM 		(2)
#define TEX_LT_PACK 	(3)
#define TEX_LT_PIXELS	(4)

class cTexture;

class EE_API cTextureLoader : public cObjectLoader {
	public:
		cTextureLoader( const std::string& Filepath, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		cTextureLoader( const unsigned char * ImagePtr, const eeUint& Size, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		cTextureLoader( cPack * Pack, const std::string& FilePackPath, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		cTextureLoader( const unsigned char * Pixels, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false, const std::string& FileName = std::string("") );

		~cTextureLoader();

		void			SetColorKey( eeColor Color );

		void 			Update();

		void			Unload();

		const std::string&	Filepath() const;

		const Uint32& 	Id() const;

		cTexture *		GetTexture() const;
	protected:
		Uint32			mLoadType; 	// From memory, from path, from pack
		Uint8 * 		mPixels;	// Texture Info
		Uint32 			mTexId;
		Int32			mImgWidth;
		Int32			mImgHeight;

		std::string 	mFilepath;
		eeUint 			mWidth;
		eeUint 			mHeight;
		bool 			mMipmap;
		Int32 			mChannels;
		EE_CLAMP_MODE 	mClampMode;
		bool 			mCompressTexture;
		bool 			mLocalCopy;
		cPack * 		mPack;

		const Uint8 *	mImagePtr;
		Uint32			mSize;

		eeColor *		mColorKey;

		void 			Start();

		void			Reset();
	private:
		bool			mTexLoaded;
		bool			mIsDDS;
		int				mIsDDSCompressed;

		void 			LoadFromPath();
		void			LoadFromMemory();
		void			LoadFromPack();
		void 			LoadFromPixels();
};

}}

#endif
