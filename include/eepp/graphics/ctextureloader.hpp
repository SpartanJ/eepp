#ifndef EE_GRAPHICSCTEXTURELOADER
#define EE_GRAPHICSCTEXTURELOADER

#include <eepp/graphics/base.hpp>
#include <eepp/system/cobjectloader.hpp>

namespace EE { namespace Graphics {

class cTexture;

class EE_API cTextureLoader : public cObjectLoader {
	public:
		cTextureLoader( cIOStream& Stream, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		cTextureLoader( const std::string& Filepath, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		cTextureLoader( const unsigned char * ImagePtr, const eeUint& Size, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		cTextureLoader( cPack * Pack, const std::string& FilePackPath, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		cTextureLoader( const unsigned char * Pixels, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false, const std::string& FileName = std::string("") );

		virtual ~cTextureLoader();

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
		cIOStream *		mStream;

		const Uint8 *	mImagePtr;
		Uint32			mSize;

		eeColor *		mColorKey;

		void 			Start();

		void			Reset();
	private:
		bool			mTexLoaded;
		bool			mIsDDS;
		int				mIsDDSCompressed;

		cTimeElapsed	mTE;

		void 			LoadFromPath();
		void			LoadFromMemory();
		void			LoadFromPack();
		void 			LoadFromPixels();
		void			LoadFromStream();
};

}}

#endif
