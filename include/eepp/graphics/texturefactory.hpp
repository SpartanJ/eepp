#ifndef EECTEXTUREFACTORY_H
#define EECTEXTUREFACTORY_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <list>

#include <eepp/system/mutex.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The Texture Manager Class. Here we do all the textures stuff. (Singleton Class) */
class EE_API TextureFactory : protected Mutex {
	SINGLETON_DECLARE_HEADERS( TextureFactory )

  public:
	/** Creates an empty texture
	 * @param Width Texture Width
	 * @param Height Texture Height
	 * @param Channels Texture Number of Channels (in bytes)
	 * @param DefaultColor The background color for the texture
	 * @param Mipmap Create Mipmap?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 * @param Filename A filename to recognize the texture.
	 * @return Internal Texture Id
	 */
	Uint32 createEmptyTexture(
		const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels = 4,
		const Color& DefaultColor = Color( 0, 0, 0, 255 ), const bool& Mipmap = false,
		const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
		const bool& CompressTexture = false, const bool& KeepLocalCopy = false,
		const std::string& Filename = std::string( "" ) );

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
	 * @return Internal Texture Id
	 */
	Uint32 loadFromPixels( const unsigned char* Pixels, const unsigned int& Width,
						   const unsigned int& Height, const unsigned int& Channels,
						   const bool& Mipmap = false,
						   const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
						   const bool& CompressTexture = false, const bool& KeepLocalCopy = false,
						   const std::string& FileName = std::string( "" ) );

	/** Load a texture from Pack file
	 * @param Pack Pointer to the pack instance
	 * @param FilePackPath The path of the file inside the pack
	 * @param Mipmap Create Mipmap?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 * @param imageformatConfiguration The specific image format configuration to use when decoding
	 * the image.
	 * @return Internal Texture Id
	 */
	Uint32 loadFromPack(
		Pack* Pack, const std::string& FilePackPath, const bool& Mipmap = false,
		const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
		const bool& CompressTexture = false, const bool& KeepLocalCopy = false,
		const Image::FormatConfiguration& imageformatConfiguration = Image::FormatConfiguration() );

	/** Load a texture from memory
	 * @param ImagePtr The image data in RAM just as if it were still in a file
	 * @param Size The size of the texture ( Width * Height * BytesPerPixel )
	 * @param Mipmap Use mipmaps?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 * @param imageformatConfiguration The specific image format configuration to use when decoding
	 * the image.
	 * @return The internal Texture Id
	 */
	Uint32 loadFromMemory(
		const unsigned char* ImagePtr, const unsigned int& Size, const bool& Mipmap = false,
		const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
		const bool& CompressTexture = false, const bool& KeepLocalCopy = false,
		const Image::FormatConfiguration& imageformatConfiguration = Image::FormatConfiguration() );

	/** Load a Texture from stream
	 * @param Stream The IOStream instance
	 * @param Mipmap Use mipmaps?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 * @param imageformatConfiguration The specific image format configuration to use when decoding
	 * the image.
	 * @return The internal Texture Id
	 */
	Uint32 loadFromStream(
		IOStream& Stream, const bool& Mipmap = false,
		const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
		const bool& CompressTexture = false, const bool& KeepLocalCopy = false,
		const Image::FormatConfiguration& imageformatConfiguration = Image::FormatConfiguration() );

	/** Load a Texture from a file path
	 * @param Filepath The path for the texture
	 * @param Mipmap Use mipmaps?
	 * @param ClampMode Defines the CLAMP MODE
	 * @param CompressTexture If use the DXT compression on the texture loading ( if the card can
	 * display them, will convert RGB to DXT1, RGBA to DXT5 )
	 * @param KeepLocalCopy Keep the array data copy. ( useful if want to reload the texture )
	 * @param imageformatConfiguration The specific image format configuration to use when decoding
	 * the image.
	 * @return The internal Texture Id
	 */
	Uint32 loadFromFile(
		const std::string& Filepath, const bool& Mipmap = false,
		const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
		const bool& CompressTexture = false, const bool& KeepLocalCopy = false,
		const Image::FormatConfiguration& imageformatConfiguration = Image::FormatConfiguration() );

	/** Remove and Unload the Texture Id
	 * @param TexId
	 * @return True if was removed
	 */
	bool remove( Uint32 TexId );

	/** Reload all loaded textures to recover the OpenGL context */
	void reloadAllTextures();

	/** Bind the the internal Texture Id indicated. This is useful if you are rendering a texture
	 * outside this class.
	 * @param TexId The internal Texture Id
	 * @param coordinateType Use normalized or pixel coordinates
	 * @param textureUnit The Texture Unit binded
	 * @param forceRebind Force the texture bind (even if is already binded ).
	 */
	void bind( const Uint32& TexId,
			   Texture::CoordinateType coordinateType = Texture::CoordinateType::Normalized,
			   const Uint32& textureUnit = 0, const bool& forceRebind = false );

	/** Bind the the Texture indicated. This is useful if you are rendering a texture outside this
	 * class.
	 * @param Tex The Texture Pointer
	 * @param coordinateType Selects the coordinate type to use with the binded texture.
	 * @param TextureUnit The Texture Unit binded
	 * @param forceRebind Force the texture bind (even if is already binded ).
	 */
	void bind( const Texture* Tex,
			   Texture::CoordinateType coordinateType = Texture::CoordinateType::Normalized,
			   const Uint32& TextureUnit = 0, const bool& forceRebind = false );

	/**
	 * @param TexId The internal Texture Id
	 * @return The OpenGL Texture Id (texture handler)
	 */
	Uint32 getTextureId( const Uint32& TexId );

	/**
	 * @return The real current texture id (OpenGL Texture Id)
	 * @param TextureUnit The Texture Unit binded
	 */
	int getCurrentTexture( const Uint32& TextureUnit = 0 ) const;

	/** Set the current internal texture id. This will set the TexId as the current texture binded.
	 * @param TexId The real current texture id (OpenGL Texture Id)
	 * @param TextureUnit The Texture Unit binded
	 */
	void setCurrentTexture( const int& TexId, const Uint32& TextureUnit );

	/** Returns the number of textures loaded */
	Uint32 getTextureCount() const { return (Uint32)mTextures.size(); }

	/** @return All the active textures */
	std::vector<Texture*> getTextures();

	/** Active a texture unit */
	void setActiveTextureUnit( const Uint32& Unit );

	/**
	 * @param Size
	 * @return A valid texture size for the video card (checks if support non power of two textures)
	 */
	unsigned int getValidTextureSize( const unsigned int& Size );

	/** Determine if the TextureId passed exists */
	bool existsId( const Uint32& TexId );

	/** @return A pointer to the Texture */
	Texture* getTexture( const Uint32& TexId );

	/** Get a local copy for all the textures */
	void grabTextures();

	/** Reload all the grabed textures */
	void ungrabTextures();

	/** Allocate space for Textures (only works if EE_ALLOC_TEXTURES_ON_VECTOR is defined) */
	void allocate( const unsigned int& size );

	/** @return The memory used by the textures (in bytes) */
	unsigned int getTextureMemorySize() { return mMemSize; }

	/** It's possible to create textures outside the texture factory loader, but the library will
	 * need to know of this texture, so it's necessary to push the texture to the factory.
	 * @param Filepath The Texture path ( if exists )
	 * @param TexId The OpenGL Texture Id
	 * @param Width Texture Width
	 * @param Height Texture Height
	 * @param ImgWidth Image Width.
	 * @param ImgHeight Image Height
	 * @param Mipmap Tell if the texture has mipmaps
	 * @param Channels Texture number of Channels ( bytes per pixel )
	 * @param ClampMode The Texture Clamp Mode
	 * @param CompressTexture The texture is compressed?
	 * @param LocalCopy If keep a local copy in memory of the texture
	 * @param MemSize The size of the texture in memory ( just if you need to specify the real size
	 * in memory, just useful to calculate the total texture memory ).
	 */
	Uint32 pushTexture( const std::string& Filepath, const Uint32& TexId, const unsigned int& Width,
						const unsigned int& Height, const unsigned int& ImgWidth,
						const unsigned int& ImgHeight, const bool& Mipmap,
						const unsigned int& Channels, const Texture::ClampMode& ClampMode,
						const bool& CompressTexture, const bool& LocalCopy = false,
						const Uint32& MemSize = 0 );

	/** Return a texture by it file path name
	 * @param Name File path name
	 * @return The texture, NULL if not exists.
	 */
	Texture* getByName( const std::string& Name );

	/** Return a texture by it hash path name
	 * @param Hash The file path hash
	 * @return The texture, NULL if not exists
	 */
	Texture* getByHash( const Uint32& Hash );

	~TextureFactory();

	const Texture::CoordinateType& getLastCoordinateType() const;

  protected:
	friend class Texture;

	TextureFactory();

	std::vector<int> mCurrentTexture;

	std::vector<Texture*> mTextures;

	unsigned int mMemSize;

	std::list<Uint32> mVectorFreeSlots;

	Texture::CoordinateType mLastCoordinateType;

	void unloadTextures();

	Uint32 findFreeSlot();

	bool mErasing;

	const bool& isErasing() const;

	void removeReference( Texture* Tex );
};

}} // namespace EE::Graphics

#endif
