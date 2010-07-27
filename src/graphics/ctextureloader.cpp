#include "ctextureloader.hpp"
#include "ctexturefactory.hpp"

namespace EE { namespace Graphics {

cTextureLoader::cTextureLoader( const std::string& Filepath,
	const bool& Mipmap,
	const eeRGB& ColorKey,
	const EE_CLAMP_MODE& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy
) : cObjectLoader( cObjectLoader::TextureLoader ),
	mLoadType(TEX_LT_PATH),
	mPixels(NULL),
	mTexId(0),
	mImgWidth(0),
	mImgHeight(0),
	mFilepath(Filepath),
	mWidth(0),
	mHeight(0),
	mMipmap(Mipmap),
	mChannels(0),
	mColorKey(ColorKey),
	mClampMode(ClampMode),
	mCompressTexture(CompressTexture),
	mLocalCopy(KeepLocalCopy),
	mPack(NULL),
	mImagePtr(NULL),
	mSize(0),
	mTexLoaded(false)
{
}

cTextureLoader::cTextureLoader( const unsigned char * ImagePtr,
	const eeUint& Size,
	const bool& Mipmap,
	const eeRGB& ColorKey,
	const EE_CLAMP_MODE& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy
) : cObjectLoader( cObjectLoader::TextureLoader ),
	mLoadType(TEX_LT_MEM),
	mPixels(NULL),
	mTexId(0),
	mImgWidth(0),
	mImgHeight(0),
	mFilepath(""),
	mWidth(0),
	mHeight(0),
	mMipmap(Mipmap),
	mChannels(0),
	mColorKey(ColorKey),
	mClampMode(ClampMode),
	mCompressTexture(CompressTexture),
	mLocalCopy(KeepLocalCopy),
	mPack(NULL),
	mImagePtr(ImagePtr),
	mSize(Size),
	mTexLoaded(false)
{
}

cTextureLoader::cTextureLoader( cPack * Pack,
	const std::string& FilePackPath,
	const bool& Mipmap ,
	const eeRGB& ColorKey,
	const EE_CLAMP_MODE& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy
) : cObjectLoader( cObjectLoader::TextureLoader ),
	mLoadType(TEX_LT_PACK),
	mPixels(NULL),
	mTexId(0),
	mImgWidth(0),
	mImgHeight(0),
	mFilepath(FilePackPath),
	mWidth(0),
	mHeight(0),
	mMipmap(Mipmap),
	mChannels(0),
	mColorKey(ColorKey),
	mClampMode(ClampMode),
	mCompressTexture(CompressTexture),
	mLocalCopy(KeepLocalCopy),
	mPack(Pack),
	mImagePtr(NULL),
	mSize(0),
	mTexLoaded(false)
{
}

cTextureLoader::cTextureLoader( const unsigned char * Pixels,
	const eeUint& Width,
	const eeUint& Height,
	const eeUint& Channels,
	const bool& Mipmap,
	const eeRGB& ColorKey,
	const EE_CLAMP_MODE& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy,
	const std::string& FileName
) : cObjectLoader( cObjectLoader::TextureLoader ),
	mLoadType(TEX_LT_PIXELS),
	mPixels( const_cast<unsigned char *> ( Pixels ) ),
	mTexId(0),
	mImgWidth(Width),
	mImgHeight(Height),
	mFilepath(FileName),
	mWidth(0),
	mHeight(0),
	mMipmap(Mipmap),
	mChannels(Channels),
	mColorKey(ColorKey),
	mClampMode(ClampMode),
	mCompressTexture(CompressTexture),
	mLocalCopy(KeepLocalCopy),
	mPack(NULL),
	mImagePtr(NULL),
	mSize(0),
	mTexLoaded(false)
{
}

cTextureLoader::~cTextureLoader() {
	if ( TEX_LT_PIXELS != mLoadType )
		eeSAFE_FREE( mPixels );
}

void cTextureLoader::Start() {
	cObjectLoader::Start();
	
	if ( TEX_LT_PATH == mLoadType )
		LoadFromPath();
	else if ( TEX_LT_MEM == mLoadType )
		LoadFromMemory();
	else if ( TEX_LT_PACK == mLoadType )
		LoadFromPack();

	mTexLoaded = true;

	if ( !mThreaded )
		LoadFromPixels();
}

void cTextureLoader::LoadFromPath() {
	if ( FileExists( mFilepath ) ) {
		mPixels = SOIL_load_image( mFilepath.c_str(), &mImgWidth, &mImgHeight, &mChannels, SOIL_LOAD_AUTO );

		if ( NULL == mPixels )
			cLog::instance()->Write( SOIL_last_result() );
	}
}

void cTextureLoader::LoadFromPack() {
	std::vector<Uint8> TmpData;

	if ( NULL != mPack && mPack->IsOpen() && mPack->ExtractFileToMemory( mFilepath, TmpData ) ) {
		mImagePtr 	= reinterpret_cast<const Uint8*> (&TmpData[0]);
		mSize 		= TmpData.size();

		LoadFromMemory();
	}
}

void cTextureLoader::LoadFromMemory() {
	mPixels = SOIL_load_image_from_memory( mImagePtr, mSize, &mImgWidth, &mImgHeight, &mChannels, SOIL_LOAD_AUTO );

	if ( NULL == mPixels )
		cLog::instance()->Write( SOIL_last_result() );
}

void cTextureLoader::LoadFromPixels() {
	if ( !mLoaded && mTexLoaded ) {
		Uint32 tTexId = 0;

		if ( NULL != mPixels ) {
			int width = mImgWidth;
			int height = mImgHeight;

			Uint32 flags = mMipmap ? SOIL_FLAG_MIPMAPS : 0;

			flags = ( mClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
			flags = ( mCompressTexture ) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;

			GLint PreviousTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

			tTexId = SOIL_create_OGL_texture( mPixels, &width, &height, mChannels, SOIL_CREATE_NEW_ID, flags );

			glBindTexture(GL_TEXTURE_2D, PreviousTexture);

			if ( tTexId )
				mTexId = cTextureFactory::instance()->PushTexture( mFilepath, tTexId, mImgWidth, mImgHeight, width, height, mMipmap, mChannels, mColorKey, mClampMode, mCompressTexture, mLocalCopy );

			if ( TEX_LT_PIXELS != mLoadType )
				eeSAFE_FREE( mPixels );

			mPixels = NULL;

		} else
			cLog::instance()->Write( SOIL_last_result() );

		SetLoaded();
	}
}

void cTextureLoader::Update() {
	LoadFromPixels();
}

const Uint32& cTextureLoader::TexId() const {
	return mTexId;
}

}}

