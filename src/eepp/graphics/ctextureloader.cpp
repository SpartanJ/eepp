#include <eepp/graphics/ctextureloader.hpp>
#include <eepp/graphics/ctexture.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/system/ciostreamfile.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/helper/jpeg-compressor/jpgd.h>

#define TEX_LT_PATH 	(1)
#define TEX_LT_MEM 		(2)
#define TEX_LT_PACK 	(3)
#define TEX_LT_PIXELS	(4)
#define TEX_LT_STREAM	(5)

namespace EE { namespace Graphics {

namespace IOCb
{
	// stb_image callbacks that operate on a sf::InputStream
	int read(void* user, char* data, int size)
	{
		cIOStream * stream = static_cast<cIOStream*>(user);
		return static_cast<int>(stream->Read(data, size));
	}

	void skip(void* user, unsigned int size)
	{
		cIOStream * stream = static_cast<cIOStream*>(user);
		stream->Seek(stream->Tell() + size);
	}

	int eof(void* user)
	{
		cIOStream* stream = static_cast<cIOStream*>(user);
		return stream->Tell() >= stream->GetSize();
	}
}

namespace jpeg
{
	class jpeg_decoder_stream_steam : public jpgd::jpeg_decoder_stream {
		public:
			cIOStream * mStream;

			jpeg_decoder_stream_steam( cIOStream * stream ) :
				mStream( stream )
			{}

			virtual ~jpeg_decoder_stream_steam() {}

			virtual int read(jpgd::uint8 *pBuf, int max_bytes_to_read, bool *pEOF_flag) {
				return mStream->Read( (char*)pBuf, max_bytes_to_read );
			}
	};
}

cTextureLoader::cTextureLoader( cIOStream& Stream,
	const bool& Mipmap,
	const EE_CLAMP_MODE& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy
) : cObjectLoader( cObjectLoader::TextureLoader ),
	mLoadType(TEX_LT_STREAM),
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
	mStream(&Stream),
	mImagePtr(NULL),
	mSize(0),
	mColorKey(NULL),
	mTexLoaded(false),
	mIsDDS(false),
	mIsDDSCompressed(0)
{
}
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
	mStream(NULL),
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
	mStream(NULL),
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
	mStream(NULL),
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
	mStream(NULL),
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

	mTE.Reset();

	if ( TEX_LT_PATH == mLoadType )
		LoadFromPath();
	else if ( TEX_LT_MEM == mLoadType )
		LoadFromMemory();
	else if ( TEX_LT_PACK == mLoadType )
		LoadFromPack();
	else if ( TEX_LT_STREAM == mLoadType )
		LoadFromStream();

	mTexLoaded = true;

	if ( !mThreaded )
		LoadFromPixels();
}

void cTextureLoader::LoadFromPath() {
	if ( FileSystem::FileExists( mFilepath ) ) {
		if ( GLi->IsExtension( EEGL_EXT_texture_compression_s3tc ) )
			mIsDDS = 0 != stbi_dds_test_filename( mFilepath.c_str() );

		if ( mIsDDS ) {
			cIOStreamFile fs( mFilepath , std::ios::in | std::ios::binary );

			mSize = FileSystem::FileSize( mFilepath );

			mPixels = (Uint8*) eeMalloc( mSize );

			fs.Read( reinterpret_cast<char*> ( mPixels ), mSize );

			stbi_dds_info_from_memory( mPixels, mSize, &mImgWidth, &mImgHeight, &mChannels, &mIsDDSCompressed );
		} else {
			mPixels = stbi_load( mFilepath.c_str(), &mImgWidth, &mImgHeight, &mChannels, STBI_default );
		}

		if ( NULL == mPixels ) {
			cLog::instance()->Write( "Filed to load: " + mFilepath + " Reason: " + std::string( stbi_failure_reason() ) );

			if ( !mIsDDS ) {
				mPixels = jpgd::decompress_jpeg_image_from_file( mFilepath.c_str(), &mImgWidth, &mImgHeight, &mChannels, 3 );

				if ( NULL != mPixels ) {
					cLog::instance()->Write( "Loaded: " + mFilepath + " using jpeg-compressor." );
				} else {

				}
			}
		}
	} else if ( cPackManager::instance()->FallbackToPacks() ) {
		mPack = cPackManager::instance()->Exists( mFilepath );

		if ( NULL != mPack ) {
			mLoadType = TEX_LT_PACK;

			LoadFromPack();
		}
	}
}

void cTextureLoader::LoadFromPack() {
	SafeDataPointer PData;

	if ( NULL != mPack && mPack->IsOpen() && mPack->ExtractFileToMemory( mFilepath, PData ) ) {
		mImagePtr	= PData.Data;
		mSize		= PData.DataSize;

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
		mPixels = stbi_load_from_memory( mImagePtr, mSize, &mImgWidth, &mImgHeight, &mChannels, STBI_default );
	}

	if ( NULL == mPixels ) {
		cLog::instance()->Write( stbi_failure_reason() );

		if ( !mIsDDS ) {
			mPixels = jpgd::decompress_jpeg_image_from_memory( mImagePtr, mSize, &mImgWidth, &mImgHeight, &mChannels, 3 );

			if ( NULL != mPixels ) {
				cLog::instance()->Write( "Loaded: image using jpeg-compressor." );
			}
		}
	}
}

void cTextureLoader::LoadFromStream() {
	if ( mStream->IsOpen() ) {
		mSize = mStream->GetSize();

		stbi_io_callbacks callbacks;
		callbacks.read = &IOCb::read;
		callbacks.skip = &IOCb::skip;
		callbacks.eof  = &IOCb::eof;

		if ( GLi->IsExtension( EEGL_EXT_texture_compression_s3tc ) )
			mIsDDS = 0 != stbi_dds_test_callbacks( &callbacks, mStream );

		if ( mIsDDS ) {
			mSize	= mStream->GetSize();
			mPixels	= (Uint8*) eeMalloc( mSize );

			mStream->Seek( 0 );
			mStream->Read( reinterpret_cast<char*> ( mPixels ), mSize );

			mStream->Seek( 0 );
			stbi_dds_info_from_callbacks( &callbacks, mStream, &mImgWidth, &mImgHeight, &mChannels, &mIsDDSCompressed );
			mStream->Seek( 0 );
		} else {
			mStream->Seek( 0 );
			mPixels = stbi_load_from_callbacks( &callbacks, mStream, &mImgWidth, &mImgHeight, &mChannels, STBI_default );
			mStream->Seek( 0 );
		}

		if ( NULL == mPixels ) {
			cLog::instance()->Write( stbi_failure_reason() );

			if ( !mIsDDS ) {
				jpeg::jpeg_decoder_stream_steam stream( mStream );

				mPixels = jpgd::decompress_jpeg_image_from_stream( &stream, &mImgWidth, &mImgHeight, &mChannels, 3 );
				mStream->Seek( 0 );
			}
		}
	}
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

				cLog::instance()->Write( "Texture " + mFilepath  + " loaded in " + String::ToStr( mTE.Elapsed() ) + " ms." );

				if ( NULL != mColorKey && 0 != mTexId )
					cTextureFactory::instance()->GetTexture( mTexId )->CreateMaskFromColor( eeColorA( mColorKey->R(), mColorKey->G(), mColorKey->B(), 255 ), 0 );
			} else {
				cLog::instance()->Write( "Failed to create texture. Reason: " + std::string( SOIL_last_result() ) );
			}

			if ( TEX_LT_PIXELS != mLoadType ) {
				if ( mIsDDS ) {
					eeFree( mPixels );
				} else {
					if ( mPixels )
						free( mPixels );
				}
			}

			mPixels = NULL;

		} else {
			if ( NULL != stbi_failure_reason() ) {
				cLog::instance()->Write( stbi_failure_reason() );
			} else {
				std::string failText( "Texture " + mFilepath + " failed to load" );

				failText += ( NULL != mPack ) ? ( " from Pack " + mPack->GetPackPath() + "." ) : ".";

				cLog::instance()->Write( failText );
			}
		}

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

const std::string& cTextureLoader::Filepath() const {
	return mFilepath;
}

cTexture * cTextureLoader::GetTexture() const {
	if ( 0 != mTexId )
		return cTextureFactory::instance()->GetTexture( mTexId );

	return NULL;
}

void cTextureLoader::Unload() {
	if ( mLoaded ) {
		cTextureFactory::instance()->Remove( mTexId );

		Reset();
	}
}

void cTextureLoader::Reset() {
	cObjectLoader::Reset();

	mPixels				= NULL;
	mTexId				= 0;
	mImgWidth			= 0;
	mImgHeight			= 0;
	mWidth				= 0;
	mHeight				= 0;
	mChannels			= 0;
	mSize				= 0;
	mTexLoaded			= false;
	mIsDDS				= false;
	mIsDDSCompressed	= 0;
}

}}

