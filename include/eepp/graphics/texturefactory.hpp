#ifndef EECTEXTUREFACTORY_H
#define EECTEXTUREFACTORY_H

#include <cstddef>
#include <eepp/core/containers.hpp>
#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

struct TextureRegistryRecord {
	ResourceId id;
	std::string displayName;
	TextureWeakPtr texture;
	std::size_t memoryBytes{ 0 };
};

using TextureRegistrySnapshot = std::vector<TextureRegistryRecord>;

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
	 * @return The created texture
	 */
	TexturePtr createEmptyTexture(
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
	 * @return The texture loaded or null if error
	 */
	TexturePtr
	loadFromPixels( const unsigned char* Pixels, const unsigned int& Width,
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
	 * @return The texture loaded or null if error
	 */
	TexturePtr loadFromPack(
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
	 * @return The texture loaded or null if error
	 */
	TexturePtr loadFromMemory(
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
	 * @return The texture loaded or null if error
	 */
	TexturePtr loadFromStream(
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
	 * @return The texture loaded or null if error
	 */
	TexturePtr loadFromFile(
		const std::string& Filepath, const bool& Mipmap = false,
		const Texture::ClampMode& ClampMode = Texture::ClampMode::ClampToEdge,
		const bool& CompressTexture = false, const bool& KeepLocalCopy = false,
		const Image::FormatConfiguration& imageformatConfiguration = Image::FormatConfiguration() );

	/** Binds the texture identity indicated. This is useful if you are rendering a texture
	 * outside this class.
	 * @param textureId The process-wide texture identity.
	 * @param coordinateType Use normalized or pixel coordinates
	 * @param textureUnit The Texture Unit binded
	 * @param forceRebind Force the texture bind (even if is already binded ).
	 */
	void bind( ResourceId textureId,
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
	 * @return The currently bound OpenGL texture handle.
	 * @param TextureUnit The bound texture unit.
	 */
	int getCurrentTexture( const Uint32& TextureUnit = 0 ) const;

	/** Sets the currently bound OpenGL texture handle.
	 * @param textureHandle The OpenGL texture handle.
	 * @param TextureUnit The Texture Unit binded
	 */
	void setCurrentTexture( const int& textureHandle, const Uint32& TextureUnit );

	/** Returns the number of textures loaded */
	Uint32 getTextureCount();

	/** @return A non-owning diagnostic snapshot of every currently live texture. */
	TextureRegistrySnapshot snapshotTextures();

	/** Removes expired records from the diagnostic live-texture registry. */
	void purgeExpiredTextures();

	/** @return The generation of the live-texture registry. It changes when a texture is created or
	 * its last owning handle is released. */
	Uint64 getLiveTextureGeneration() const;

	/** Destroys textures whose final owning handle was released. Must run on the graphics thread
	 * after pending batches have been flushed and while a context is current. */
	void collectReleasedTextures();

	/** @return The number of textures waiting for graphics-thread destruction. */
	std::size_t getPendingReleaseCount();

	/** Active a texture unit */
	void setActiveTextureUnit( const Uint32& Unit );

	/**
	 * @param Size
	 * @return A valid texture size for the video card (checks if support non power of two textures)
	 */
	unsigned int getValidTextureSize( const unsigned int& Size );

	/** Determines whether the texture identity exists in the factory. */
	bool existsId( ResourceId textureId );

	/** @return The texture matching @p textureId, or null if it is not factory-retained. */
	TexturePtr getTexture( ResourceId textureId );

	/** @return The memory used by the textures (in bytes) */
	unsigned int getTextureMemorySize();

	/** It's possible to create textures outside the texture factory loader, but the library will
	 * need to know of this texture, so it's necessary to push the texture to the factory.
	 * @param Filepath The Texture path ( if exists )
	 * @param textureHandle The OpenGL texture handle.
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
	TexturePtr pushTexture( const std::string& Filepath, const Uint32& textureHandle,
							const unsigned int& Width, const unsigned int& Height,
							const unsigned int& ImgWidth, const unsigned int& ImgHeight,
							const bool& Mipmap, const unsigned int& Channels,
							const Texture::ClampMode& ClampMode, const bool& CompressTexture,
							const bool& LocalCopy = false, const Uint32& MemSize = 0 );

	/** Return a texture by it file path name
	 * @param Name File path name
	 * @return The texture, NULL if not exists.
	 */
	TexturePtr getByName( const std::string& Name );

	/** Return a texture by it hash path name
	 * @param Hash The file path hash
	 * @return The texture, NULL if not exists
	 */
	TexturePtr getByHash( const String::HashType& hash );

	~TextureFactory();

	const Texture::CoordinateType& getLastCoordinateType() const;

  protected:
	friend class Texture;
	friend class TextureLoader;

	TextureFactory();

	std::vector<int> mCurrentTexture;

	using TextureMap = UnorderedMap<Uint64, TexturePtr>;

	struct LiveTextureRecord {
		ResourceId id;
		TextureWeakPtr texture;
	};

	struct TextureDeleter {
		void operator()( Texture* texture ) const noexcept;
	};

	TextureMap mTextures;
	UnorderedMap<Uint64, LiveTextureRecord> mLiveTextures;
	std::vector<Texture*> mReleasedTextures;
	std::atomic<Uint64> mLiveTextureGeneration{ 0 };

	Texture::CoordinateType mLastCoordinateType;

	void unloadTextures();

	void resetTextureBinding( const Texture* texture );

	bool releaseRetainedTexture( ResourceId textureId );

	void queueReleasedTexture( Texture* texture );

	void diagnoseLiveTexturesAtShutdown();
};

}} // namespace EE::Graphics

#endif
