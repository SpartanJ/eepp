#ifndef EE_GRAPHICS_TEXTURELOADER
#define EE_GRAPHICS_TEXTURELOADER

#include <atomic>

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/pack.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The Texture loader loads a texture in synchronous or asynchronous mode. */
class EE_API TextureLoader {
  public:
	typedef std::function<void( Uint32, Texture* )> OnTextureLoaded;

	static Uint32 pushLoadedCallback( const OnTextureLoaded& cb );

	static void popLoadedCallback( const Uint32& cbId );

	/** Load a Texture from stream
	 * @param Stream The io stream instance
	 * @param Mipmap Use mipmaps?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 */
	TextureLoader( IOStream& Stream, const bool& Mipmap = false,
				   const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
				   const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

	/** Load a Texture from a file path
	 * @param filepath The path for the texture
	 * @param Mipmap Use mipmaps?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 */
	TextureLoader( const std::string& filepath, const bool& Mipmap = false,
				   const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
				   const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

	/** Load a texture from memory
	 * @param ImagePtr The image data in memory just as if it were still in a file
	 * @param Size The size of the texture ( Width * Height * BytesPerPixel )
	 * @param Mipmap Use mipmaps?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 */
	TextureLoader( const unsigned char* ImagePtr, const unsigned int& Size,
				   const bool& Mipmap = false,
				   const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
				   const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

	/** Load a texture from a Pack file
	 * @param Pack Pointer to the pack instance
	 * @param FilePackPath The path of the file inside the pack
	 * @param Mipmap Create Mipmap?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 */
	TextureLoader( Pack* Pack, const std::string& FilePackPath, const bool& Mipmap = false,
				   const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
				   const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

	/** Loads a RAW Texture from Memory
	 * @param Pixels The Texture array
	 * @param Width Texture Width
	 * @param Height Texture Height
	 * @param Channels Texture Number of Channels (in bytes)
	 * @param Mipmap Create Mipmap?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 * @param FileName A filename to recognize the texture ( the path in case that was loaded from
	 * outside the texture factory ).
	 */
	TextureLoader( const unsigned char* Pixels, const unsigned int& Width,
				   const unsigned int& Height, const unsigned int& Channels,
				   const bool& Mipmap = false,
				   const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
				   const bool& CompressTexture = false, const bool& KeepLocalCopy = false,
				   const std::string& FileName = std::string( "" ) );

	virtual ~TextureLoader();

	/** A color key can be set to be transparent in the texture. This must be set before the loading
	 * is done. */
	void setColorKey( RGB Color );

	/** @brief Releases the texture loaded ( if was already loaded ), it will destroy the texture
	 * from memory. */
	void unload();

	/** @return The file path to the texture ( if any ) */
	const std::string& getFilepath() const;

	/** @return The texture internal id  */
	Uint32 getId() const;

	/** @return The texture instance ( if it was loaded ). */
	Texture* getTexture() const;

	Image::FormatConfiguration getFormatConfiguration() const;

	void setFormatConfiguration( const Image::FormatConfiguration& formatConfiguration );

	/** Starts loading the texture */
	void load();

  protected:
	Uint32 mLoadType{ 0 };	   // From memory, from path, from pack
	Uint8* mPixels{ nullptr }; // Texture Info
	Texture* mTexture{ nullptr };
	Int32 mImgWidth{ 0 };
	Int32 mImgHeight{ 0 };

	std::string mFilepath;
	unsigned int mWidth{ 0 };
	unsigned int mHeight{ 0 };
	bool mMipmap{ false };
	Int32 mChannels{ 0 };
	Texture::ClampMode mClampMode{ Texture::ClampMode::ClampToEdge };
	bool mCompressTexture{ false };
	bool mLocalCopy{ false };
	Pack* mPack{ nullptr };
	IOStream* mStream{ nullptr };

	const Uint8* mImagePtr{ nullptr };
	Uint32 mSize{ 0 };

	RGB* mColorKey{ nullptr };
	Image::FormatConfiguration mFormatConfiguration;

	void reset();

  private:
	static UnorderedMap<Uint32, OnTextureLoaded> sCbs;
	static std::atomic<Uint32> sNumCbs;

	bool mLoaded{ false };
	bool mTexLoaded{ false };
	bool mDirectUpload{ false };
	Image::Format mImgType{ 0 };
	int mIsCompressed{ 0 };

	Clock mTE;

	void loadFile();
	void loadFromFile();
	void loadFromMemory();
	void loadFromPack();
	void loadFromPixels();
	void loadFromStream();

	void notifyLoaded();
};

}} // namespace EE::Graphics

#endif
