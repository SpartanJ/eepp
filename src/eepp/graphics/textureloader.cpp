#include <eepp/graphics/textureloader.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/scopedtexture.hpp>
#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/window/engine.hpp>
#include <SOIL2/src/SOIL2/stb_image.h>
#include <SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/graphics/stbi_iocb.hpp>
using namespace EE::Window;
using namespace EE::Graphics::Private;

#define TEX_LT_PATH 	(1)
#define TEX_LT_MEM 		(2)
#define TEX_LT_PACK 	(3)
#define TEX_LT_PIXELS	(4)
#define TEX_LT_STREAM	(5)

namespace EE { namespace Graphics {

TextureLoader::TextureLoader( IOStream& Stream,
	const bool& Mipmap,
	const Texture::ClampMode& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy
) : mLoadType(TEX_LT_STREAM),
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
	mLoaded(false),
	mTexLoaded(false),
	mDirectUpload(false),
	mImgType(STBI_unknown),
	mIsCompressed(0)
{
}

TextureLoader::TextureLoader( const std::string& Filepath,
	const bool& Mipmap,
	const Texture::ClampMode& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy
) : mLoadType(TEX_LT_PATH),
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
	mLoaded(false),
	mTexLoaded(false),
	mDirectUpload(false),
	mImgType(STBI_unknown),
	mIsCompressed(0)
{
}

TextureLoader::TextureLoader( const unsigned char * ImagePtr,
	const unsigned int& Size,
	const bool& Mipmap,
	const Texture::ClampMode& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy
) : mLoadType(TEX_LT_MEM),
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
	mLoaded(false),
	mTexLoaded(false),
	mDirectUpload(false),
	mImgType(STBI_unknown),
	mIsCompressed(0)
{
}

TextureLoader::TextureLoader( Pack * Pack,
	const std::string& FilePackPath,
	const bool& Mipmap ,
	const Texture::ClampMode& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy
) : mLoadType(TEX_LT_PACK),
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
	mLoaded(false),
	mTexLoaded(false),
	mDirectUpload(false),
	mImgType(STBI_unknown),
	mIsCompressed(0)
{
}

TextureLoader::TextureLoader( const unsigned char * Pixels,
	const unsigned int& Width,
	const unsigned int& Height,
	const unsigned int& Channels,
	const bool& Mipmap,
	const Texture::ClampMode& ClampMode,
	const bool& CompressTexture,
	const bool& KeepLocalCopy,
	const std::string& FileName
) : mLoadType(TEX_LT_PIXELS),
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
	mLoaded(false),
	mTexLoaded(false),
	mDirectUpload(false),
	mImgType(STBI_unknown),
	mIsCompressed(0)
{
}

TextureLoader::~TextureLoader() {
	eeSAFE_DELETE( mColorKey );

	if ( TEX_LT_PIXELS != mLoadType )
		eeSAFE_FREE( mPixels );
}

void TextureLoader::load() {
	mTE.restart();

	if ( TEX_LT_PATH == mLoadType )
		loadFromFile();
	else if ( TEX_LT_MEM == mLoadType )
		loadFromMemory();
	else if ( TEX_LT_PACK == mLoadType )
		loadFromPack();
	else if ( TEX_LT_STREAM == mLoadType )
		loadFromStream();

	mTexLoaded = true;

	loadFromPixels();
}

void TextureLoader::loadFile() {
	IOStreamFile fs( mFilepath );

	mSize		= FileSystem::fileSize( mFilepath );
	mPixels		= (Uint8*) eeMalloc( mSize );
	fs.read( reinterpret_cast<char*> ( mPixels ), mSize );
}

void TextureLoader::loadFromFile() {
	if ( FileSystem::fileExists( mFilepath ) ) {
		mImgType = stbi_test( mFilepath.c_str() );

		if ( STBI_dds == mImgType && GLi->isExtension( EEGL_EXT_texture_compression_s3tc ) ) {
			loadFile();
			mDirectUpload = true;
			stbi__dds_info_from_memory( mPixels, mSize, &mImgWidth, &mImgHeight, &mChannels, &mIsCompressed );
		} else if ( STBI_pvr == mImgType &&
					stbi__pvr_info_from_path( mFilepath.c_str(), &mImgWidth, &mImgHeight, &mChannels, &mIsCompressed ) &&
					( !mIsCompressed || GLi->isExtension( EEGL_IMG_texture_compression_pvrtc ) ) )
		{
			// If the PVR is valid, and the pvrtc extension is present or it's not compressed ( so it doesn't need the extension )
			// means that it can be uploaded directly to the GPU.
			loadFile();
			mDirectUpload = true;
		} else if ( STBI_pkm == mImgType && GLi->isExtension( EEGL_OES_compressed_ETC1_RGB8_texture ) ) {
			loadFile();
			mIsCompressed =  mDirectUpload = true;
			stbi__pkm_info_from_memory( mPixels, mSize, &mImgWidth, &mImgHeight, &mChannels );
		} else {
			if ( mCompressTexture ) {
				mSize		= FileSystem::fileSize( mFilepath );
			}

			Image image( mFilepath, ( NULL != mColorKey ) ? STBI_rgb_alpha : STBI_default, mFormatConfiguration );
			image.avoidFreeImage( true );
			mPixels = image.getPixels(); mImgWidth = image.getWidth(); mImgHeight = image.getHeight(); mChannels = image.getChannels();
		}
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		mPack = PackManager::instance()->exists( mFilepath );

		if ( NULL != mPack ) {
			mLoadType = TEX_LT_PACK;

			loadFromPack();
		}
	}
}

void TextureLoader::loadFromPack() {
	ScopedBuffer buffer;

	if ( NULL != mPack && mPack->isOpen() && mPack->extractFileToMemory( mFilepath, buffer ) ) {
		mImagePtr	= buffer.get();
		mSize		= buffer.length();

		loadFromMemory();
	}
}

void TextureLoader::loadFromMemory() {
	mImgType = stbi_test_from_memory( mImagePtr, mSize );

	if ( STBI_dds == mImgType && GLi->isExtension( EEGL_EXT_texture_compression_s3tc ) ) {
		mPixels = (Uint8*) eeMalloc( mSize );
		memcpy( mPixels, mImagePtr, mSize );
		stbi__dds_info_from_memory( mPixels, mSize, &mImgWidth, &mImgHeight, &mChannels, &mIsCompressed );
		mDirectUpload = true;
	} else if ( STBI_pvr == mImgType &&
				stbi__pvr_info_from_memory( mImagePtr, mSize, &mImgWidth, &mImgHeight, &mChannels, &mIsCompressed ) &&
				( !mIsCompressed || GLi->isExtension( EEGL_IMG_texture_compression_pvrtc ) ) )
	{
		mPixels = (Uint8*) eeMalloc( mSize );
		memcpy( mPixels, mImagePtr, mSize );
		mDirectUpload = true;
	} else if ( STBI_pkm == mImgType && GLi->isExtension( EEGL_OES_compressed_ETC1_RGB8_texture ) ) {
		mPixels = (Uint8*) eeMalloc( mSize );
		memcpy( mPixels, mImagePtr, mSize );
		stbi__pkm_info_from_memory( mPixels, mSize, &mImgWidth, &mImgHeight, &mChannels );
		mIsCompressed = mDirectUpload = true;
	} else {
		Image image( mImagePtr, mSize, ( NULL != mColorKey ) ? STBI_rgb_alpha : STBI_default, mFormatConfiguration );
		image.avoidFreeImage( true );
		mPixels = image.getPixels(); mImgWidth = image.getWidth(); mImgHeight = image.getHeight(); mChannels = image.getChannels();
	}
}

void TextureLoader::loadFromStream() {
	if ( mStream->isOpen() ) {
		mSize = mStream->getSize();

		stbi_io_callbacks callbacks;
		callbacks.read = &IOCb::read;
		callbacks.skip = &IOCb::skip;
		callbacks.eof  = &IOCb::eof;

		mImgType = stbi_test_from_callbacks( &callbacks, mStream );

		if ( STBI_dds == mImgType && GLi->isExtension( EEGL_EXT_texture_compression_s3tc ) ) {
			mSize	= mStream->getSize();
			mPixels	= (Uint8*) eeMalloc( mSize );
			mStream->seek( 0 );
			mStream->read( reinterpret_cast<char*> ( mPixels ), mSize );
			mStream->seek( 0 );
			stbi__dds_info_from_callbacks( &callbacks, mStream, &mImgWidth, &mImgHeight, &mChannels, &mIsCompressed );
			mStream->seek( 0 );
			mDirectUpload = true;
		} else if ( STBI_pvr == mImgType &&
					stbi__pvr_info_from_callbacks( &callbacks, mStream, &mImgWidth, &mImgHeight, &mChannels, &mIsCompressed ) &&
					( !mIsCompressed || GLi->isExtension( EEGL_IMG_texture_compression_pvrtc ) ) )
		{
			mSize	= mStream->getSize();
			mPixels	= (Uint8*) eeMalloc( mSize );
			mStream->seek( 0 );
			mStream->read( reinterpret_cast<char*> ( mPixels ), mSize );
			mStream->seek( 0 );
			mDirectUpload = true;
		} else if ( STBI_pkm == mImgType && GLi->isExtension( EEGL_OES_compressed_ETC1_RGB8_texture ) ) {
			mSize	= mStream->getSize();
			mPixels	= (Uint8*) eeMalloc( mSize );
			mStream->seek( 0 );
			mStream->read( reinterpret_cast<char*> ( mPixels ), mSize );
			mStream->seek( 0 );
			stbi__pkm_info_from_callbacks( &callbacks, mStream, &mImgWidth, &mImgHeight, &mChannels );
			mStream->seek( 0 );
			mIsCompressed = mDirectUpload = true;
		} else {
			mStream->seek( 0 );

			Image image( *mStream, ( NULL != mColorKey ) ? STBI_rgb_alpha : STBI_default, mFormatConfiguration );
			image.avoidFreeImage( true );
			mPixels = image.getPixels(); mImgWidth = image.getWidth(); mImgHeight = image.getHeight(); mChannels = image.getChannels();

			mStream->seek( 0 );
		}
	}
}

void TextureLoader::loadFromPixels() {
	if ( !mLoaded && mTexLoaded ) {
		Uint32 tTexId = 0;

		if ( NULL != mPixels ) {
			int width	= mImgWidth;
			int height	= mImgHeight;

			Uint32 flags = mMipmap ? SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS : 0;

			flags = ( mClampMode == Texture::ClampMode::ClampRepeat) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
			flags = ( mCompressTexture ) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;

			bool threadedLoad = Thread::getCurrentThreadId() != Engine::instance()->getMainThreadId();

			if ( threadedLoad && Engine::instance()->isSharedGLContextEnabled() ) {
				Engine::instance()->getCurrentWindow()->setGLContextThread();
			}

			{
				ScopedTexture scopedTexture;

				if ( mDirectUpload ) {
					if ( STBI_dds == mImgType ) {
						tTexId = SOIL_direct_load_DDS_from_memory( mPixels, mSize, SOIL_CREATE_NEW_ID, flags, 0 );
					} else if ( STBI_pvr == mImgType ) {
						tTexId = SOIL_direct_load_PVR_from_memory( mPixels, mSize, SOIL_CREATE_NEW_ID, flags, 0 );
					} else if ( STBI_pkm == mImgType ) {
						tTexId = SOIL_direct_load_ETC1_from_memory( mPixels, mSize, SOIL_CREATE_NEW_ID, flags );
					}
				} else {
					if ( NULL != mColorKey ) {
						mChannels = STBI_rgb_alpha;

						Image * tImg = Image::New( mPixels, mImgWidth, mImgHeight, mChannels );

						tImg->createMaskFromColor( Color( mColorKey->r, mColorKey->g, mColorKey->b, 255 ), 0 );

						tImg->avoidFreeImage( true  );

						eeSAFE_DELETE( tImg );
					}

					tTexId = SOIL_create_OGL_texture( mPixels, &width, &height, mChannels, SOIL_CREATE_NEW_ID, flags );
				}
			}

			if ( threadedLoad && Engine::instance()->isSharedGLContextEnabled() ) {
				Engine::instance()->getCurrentWindow()->unsetGLContextThread();
			}

			if ( tTexId ) {
				mWidth	= width;
				mHeight	= height;

				if ( ( ( STBI_dds == mImgType && mIsCompressed ) || mCompressTexture ) && mSize > 128 ) {
					mSize -= 128;	// Remove the DDS header size
				} else if ( STBI_pvr == mImgType && mIsCompressed && mSize > 52 ) {
					mSize -= 52;	// Remove the PVR header size
				} else if ( STBI_pkm == mImgType && mIsCompressed && mSize > 16 ) {
					mSize -= 16;	// Remove the PKM header size
				} else {
					mSize	= mWidth * mHeight * mChannels;

					if( mMipmap ) {
						int w = mWidth;
						int h = mHeight;

						while( w > 2 && h > 2 ) {
							w>>=1;
							h>>=1;
							mSize += ( w * h * mChannels );
						}
					}
				}

				mTexId = TextureFactory::instance()->pushTexture( mFilepath, tTexId, width, height, mImgWidth, mImgHeight, mMipmap, mChannels, mClampMode, mCompressTexture || mIsCompressed, mLocalCopy, mSize );

				if ( mFilepath.empty() ) {
					eePRINTL( "Texture ID %d loaded in %4.3f ms.", mTexId, mTE.getElapsed().asMilliseconds() );
				} else {
					eePRINTL( "Texture %s loaded in %4.3f ms.", mFilepath.c_str(), mTE.getElapsed().asMilliseconds() );
				}
			} else {
				eePRINTL( "Failed to create texture. Reason: %s", SOIL_last_result() );
			}

			if ( TEX_LT_PIXELS != mLoadType ) {
				if ( mDirectUpload ) {
					eeFree( mPixels );
				} else if ( NULL != mPixels ) {
					free( mPixels );
				}
			}

			mPixels = NULL;
		} else {
			if ( NULL != stbi_failure_reason() ) {
				if ( TEX_LT_PATH != mLoadType )
					eePRINTL( stbi_failure_reason() );
			} else {
				std::string failText( "Texture " + mFilepath + " failed to load" );

				failText += ( NULL != mPack ) ? ( " from Pack " + mPack->getPackPath() + "." ) : ".";

				eePRINTL( failText.c_str() );
			}
		}

		mLoaded = true;
	}
}

const Uint32& TextureLoader::getId() const {
	return mTexId;
}

void TextureLoader::setColorKey( RGB Color ) {
	eeSAFE_DELETE( mColorKey );
	mColorKey = eeNew( RGB, ( Color.r, Color.g, Color.b ) );
}

const std::string& TextureLoader::getFilepath() const {
	return mFilepath;
}

Texture * TextureLoader::getTexture() const {
	if ( 0 != mTexId )
		return TextureFactory::instance()->getTexture( mTexId );

	return NULL;
}

Image::FormatConfiguration TextureLoader::getFormatConfiguration() const {
	return mFormatConfiguration;
}

void TextureLoader::setFormatConfiguration(const Image::FormatConfiguration & formatConfiguration ) {
	mFormatConfiguration = formatConfiguration;
}

void TextureLoader::unload() {
	if ( mLoaded ) {
		TextureFactory::instance()->remove( mTexId );

		reset();
	}
}

void TextureLoader::reset() {
	mPixels				= NULL;
	mTexId				= 0;
	mImgWidth			= 0;
	mImgHeight			= 0;
	mWidth				= 0;
	mHeight				= 0;
	mChannels			= 0;
	mSize				= 0;
	mLoaded				= false;
	mTexLoaded			= false;
	mDirectUpload		= false;
	mImgType			= STBI_unknown;
	mIsCompressed		= 0;
}

}}

