#ifndef EE_GRAPHICSCTEXTURELOADER
#define EE_GRAPHICSCTEXTURELOADER

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/system/objectloader.hpp>

#include <eepp/system/pack.hpp>
#include <eepp/system/clock.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The Texture loader loads a texture in synchronous or asynchronous mode.
@see ObjectLoader
*/
class EE_API TextureLoader : public ObjectLoader {
	public:
		/** Load a Texture from stream
		* @param Stream The io stream instance
		* @param Mipmap Use mipmaps?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
		*/
		TextureLoader( IOStream& Stream, const bool& Mipmap = false, const Texture::ClampMode& ClampMode = Texture::ClampMode::CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Load a Texture from a file path
		* @param Filepath The path for the texture
		* @param Mipmap Use mipmaps?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
		*/
		TextureLoader( const std::string& filepath, const bool& Mipmap = false, const Texture::ClampMode& ClampMode = Texture::ClampMode::CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Load a texture from memory
		* @param ImagePtr The image data in memory just as if it were still in a file
		* @param Size The size of the texture ( Width * Height * BytesPerPixel )
		* @param Mipmap Use mipmaps?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
		*/
		TextureLoader( const unsigned char * ImagePtr, const unsigned int& Size, const bool& Mipmap = false, const Texture::ClampMode& ClampMode = Texture::ClampMode::CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Load a texture from a Pack file
		* @param Pack Pointer to the pack instance
		* @param FilePackPath The path of the file inside the pack
		* @param Mipmap Create Mipmap?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
		*/
		TextureLoader( Pack * Pack, const std::string& FilePackPath, const bool& Mipmap = false, const Texture::ClampMode& ClampMode = Texture::ClampMode::CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Loads a RAW Texture from Memory
		* @param Pixels The Texture array
		* @param Width Texture Width
		* @param Height Texture Height
		* @param Channels Texture Number of Channels (in bytes)
		* @param Mipmap Create Mipmap?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
		* @param FileName A filename to recognize the texture ( the path in case that was loaded from outside the texture factory ).
		*/
		TextureLoader( const unsigned char * Pixels, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels, const bool& Mipmap = false, const Texture::ClampMode& ClampMode = Texture::ClampMode::CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false, const std::string& FileName = std::string("") );

		virtual ~TextureLoader();

		/** A color key can be set to be transparent in the texture. This must be set before the loading is done. */
		void			setColorKey( RGB Color );

		/** This must be called for the asynchronous mode to update the texture data to the GPU, the call must be done from the same thread that the GL context was created ( the main thread ).
		** @see ObjectLoader::Update */
		void 			update();

		/** @brief Releases the texture loaded ( if was already loaded ), it will destroy the texture from memory. */
		void			unload();

		/** @return The file path to the texture ( if any ) */
		const std::string&	filepath() const;

		/** @return The texture internal id  */
		const Uint32& 	getId() const;

		/** @return The texture instance ( if it was loaded ). */
		Texture *		getTexture() const;
	protected:
		Uint32			mLoadType; 	// From memory, from path, from pack
		Uint8 * 		mPixels;	// Texture Info
		Uint32 			mTexId;
		Int32			mImgWidth;
		Int32			mImgHeight;

		std::string 	mFilepath;
		unsigned int 			mWidth;
		unsigned int 			mHeight;
		bool 			mMipmap;
		Int32 			mChannels;
		Texture::ClampMode 	mClampMode;
		bool 			mCompressTexture;
		bool 			mLocalCopy;
		Pack * 		mPack;
		IOStream *		mStream;

		const Uint8 *	mImagePtr;
		Uint32			mSize;

		RGB *		mColorKey;

		void 			start();

		void			reset();
	private:
		bool			mTexLoaded;
		bool			mDirectUpload;
		int				mImgType;
		int				mIsCompressed;

		Clock			mTE;

		void			loadFile();
		void 			loadFromFile();
		void			loadFromMemory();
		void			loadFromPack();
		void 			loadFromPixels();
		void			loadFromStream();
};

}}

#endif
