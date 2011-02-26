#include "ctextureloader.hpp"
#include "ctexturefactory.hpp"
#include "glhelper.hpp"

namespace EE { namespace Graphics {

cTextureLoader::cTextureLoader( const std::string& Filepath,
	const bool& Mipmap,
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
	mClampMode(ClampMode),
	mCompressTexture(CompressTexture),
	mLocalCopy(KeepLocalCopy),
	mPack(NULL),
	mImagePtr(NULL),
	mSize(0),
	mColorKey(NULL),
	mTexLoaded(false),
	mIsDDS(false),
	mIsDDSCompressed(0)
{
}

cTextureLoader::cTextureLoader( const unsigned char * ImagePtr,
	const eeUint& Size,
	const bool& Mipmap,
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
	mClampMode(ClampMode),
	mCompressTexture(CompressTexture),
	mLocalCopy(KeepLocalCopy),
	mPack(NULL),
	mImagePtr(ImagePtr),
	mSize(Size),
	mColorKey(NULL),
	mTexLoaded(false),
	mIsDDS(false),
	mIsDDSCompressed(0)
{
}

cTextureLoader::cTextureLoader( cPack * Pack,
	const std::string& FilePackPath,
	const bool& Mipmap ,
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
	mClampMode(ClampMode),
	mCompressTexture(CompressTexture),
	mLocalCopy(KeepLocalCopy),
	mPack(Pack),
	mImagePtr(NULL),
	mSize(0),
	mColorKey(NULL),
	mTexLoaded(false),
	mIsDDS(false),
	mIsDDSCompressed(0)
{
}

cTextureLoader::cTextureLoader( const unsigned char * Pixels,
	const eeUint& Width,
	const eeUint& Height,
	const eeUint& Channels,
	const bool& Mipmap,
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
	mClampMode(ClampMode),
	mCompressTexture(CompressTexture),
	mLocalCopy(KeepLocalCopy),
	mPack(NULL),
	mImagePtr(NULL),
	mSize(0),
	mColorKey(NULL),
	mTexLoaded(false),
	mIsDDS(false),
	mIsDDSCompressed(0)
{
}

cTextureLoader::~cTextureLoader() {
	eeSAFE_DELETE( mColorKey );

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
		if ( GLi->IsExtension( EEGL_EXT_texture_compression_s3tc ) )
			mIsDDS = 0 != stbi_dds_test_filename( mFilepath.c_str() );

		if ( mIsDDS ) {
			std::fstream fs ( mFilepath.c_str() , std::ios::in | std::ios::binary );
			mSize = FileSize( mFilepath );
			mPixels = (Uint8*) eeMalloc( mSize );
			fs.read( reinterpret_cast<char*> ( mPixels ), mSize );
			fs.close();

			stbi_dds_info_from_memory( mPixels, mSize, &mImgWidth, &mImgHeight, &mChannels, &mIsDDSCompressed );
		} else {
			mPixels = SOIL_load_image( mFilepath.c_str(), &mImgWidth, &mImgHeight, &mChannels, SOIL_LOAD_AUTO );
		}

		if ( NULL == mPixels )
			cLog::instance()->Write( SOIL_last_result() );
	}
}

void cTextureLoader::LoadFromPack() {
	std::vector<Uint8> TmpData;

	if ( NULL != mPack && mPack->IsOpen() && mPack->ExtractFileToMemory( mFilepath, TmpData ) ) {
		mImagePtr 	= reinterpret_cast<const Uint8*> (&TmpData[0]);
		mSize 		= (Uint32)TmpData.size();

		LoadFromMemory();
	}
}

void cTextureLoader::LoadFromMemory() {
	if ( GLi->IsExtension( EEGL_EXT_texture_compression_s3tc ) )
		mIsDDS = 0 != stbi_dds_test_memory( mImagePtr, mSize );

	if ( mIsDDS ) {
		mPixels = (Uint8*) eeMalloc( mSize );
		memcpy( mPixels, mImagePtr, mSize );
		stbi_dds_info_from_memory( mPixels, mSize, &mImgWidth, &mImgHeight, &mChannels, &mIsDDSCompressed );
	} else {
		mPixels = SOIL_load_image_from_memory( mImagePtr, mSize, &mImgWidth, &mImgHeight, &mChannels, SOIL_LOAD_AUTO );
	}

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

			if ( mIsDDS )
				tTexId = SOIL_direct_load_DDS_from_memory( mPixels, mSize, SOIL_CREATE_NEW_ID, flags, 0 );
			else
				tTexId = SOIL_create_OGL_texture( mPixels, &width, &height, mChannels, SOIL_CREATE_NEW_ID, flags );

			glBindTexture(GL_TEXTURE_2D, PreviousTexture);

			if ( tTexId ) {
				if ( mIsDDS && mIsDDSCompressed && mSize > 128 ) {
					mSize -= 128; // Remove the header size
				} else {
					mWidth	= width;
					mHeight	= height;
					mSize	= mWidth * mHeight * mChannels;
				}

				mTexId = cTextureFactory::instance()->PushTexture( mFilepath, tTexId, mImgWidth, mImgHeight, width, height, mMipmap, mChannels, mClampMode, mCompressTexture || mIsDDSCompressed, mLocalCopy, mSize );

				if ( NULL != mColorKey && 0 != mTexId )
					cTextureFactory::instance()->GetTexture( mTexId )->CreateMaskFromColor( eeColorA( mColorKey->R(), mColorKey->G(), mColorKey->B(), 255 ), 0 );
			}

			if ( TEX_LT_PIXELS != mLoadType ) {
				if ( mIsDDS ) {
					eeFree( mPixels );
				} else {
					SOIL_free_image_data( mPixels );
				}
			}

			mPixels = NULL;

		} else
			cLog::instance()->Write( SOIL_last_result() );

		SetLoaded();
	}
}

void cTextureLoader::Update() {
	LoadFromPixels();
}

const Uint32& cTextureLoader::Id() const {
	return mTexId;
}

void cTextureLoader::SetColorKey( eeColor Color ) {
	eeSAFE_DELETE( mColorKey );
	mColorKey = eeNew( eeColor, ( Color.R(), Color.G(), Color.B() ) );
}

}}

