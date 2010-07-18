#ifndef EECTEXTUREFACTORY_H
#define EECTEXTUREFACTORY_H

#include "base.hpp"
#include "ctexture.hpp"
#include "cglobalbatchrenderer.hpp"
#include "../system/cmutex.hpp"

namespace EE { namespace Graphics {

#define TEXTURE_NONE 0xFFFFFFFF

class cGlobalBatchRenderer;

/** @brief The Texture Manager Class. Here we do all the textures stuff. (Singleton Class) */
class EE_API cTextureFactory: public cSingleton<cTextureFactory>, protected cMutex {
	friend class cSingleton<cTextureFactory>;
	public:
		/** Create an empty texture
		* @param Width Texture Width
		* @param Height Texture Height
		* @param DefaultColor The background color for the texture
		* @param mipmap Create Mipmap?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( usefull if want to reload the texture )
		* @return Internal Texture Id
		*/
		Uint32 CreateEmptyTexture( const eeUint& Width, const eeUint& Height, const eeColorA& DefaultColor = eeColorA(0,0,0,255), const bool& Mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Loads a RAW Texture from Memory
		* @param Pixels The Texture array
		* @param Width Texture Width
		* @param Height Texture Height
		* @param Channels Texture Number of Channels (in bytes)
		* @param Mipmap Create Mipmap?
		* @param ColorKey Color key for the texture ( eeRGB(true) for none )
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( usefull if want to reload the texture )
		* @param FileName A filename to recognize the texture ( the path in case that was loaded from outside the texture factory ).
		* @return Internal Texture Id
		*/
		Uint32 LoadFromPixels( const unsigned char * Pixels, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& Mipmap = false, const eeRGB& ColorKey = eeRGB(true), const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false, const std::string& FileName = std::string("") );

		/** Load a texture from Pack
		* @param Pack Pointer to the pack instance
		* @param FilePackPath The path of the file inside the pack
		* @param Mipmap Create Mipmap?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( usefull if want to reload the texture )
		* @return Internal Texture Id
		*/
		Uint32 LoadFromPack( cPack* Pack, const std::string& FilePackPath, const bool& Mipmap = false, const eeRGB& ColorKey = eeRGB(true), const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Load a texture from memory (RGBA Format)
		* @param ImagePtr The image data in RAM just as if it were still in a file
		* @param Size The size of the texture ( Width * Height * BytesPerPixel )
		* @param Mipmap Use mipmaps?
		* @param ColorKey The ColorKey for the texture
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( usefull if want to reload the texture )
		* @return The internal Texture Id
		*/
		Uint32 LoadFromMemory( const unsigned char* ImagePtr, const eeUint& Size, const bool& Mipmap = false, const eeRGB& ColorKey = eeRGB(true), const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Load a Texture from a file path
		* @param filepath The path for the texture
		* @param mipmap Use mipmaps?
		* @param ColorKey The ColorKey for the texture
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( usefull if want to reload the texture )
		* @return The internal Texture Id
		*/
		Uint32 Load( const std::string& Filepath, const bool& mipmap = false, const eeRGB& ColorKey  = eeRGB(true), const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Remove and Unload the Texture Id
		* @param TexId
		* @return True if was removed
		*/
		bool Remove( const Uint32& TexId );

		/** Reload all loaded textures to recover the OpenGL context */
		void ReloadAllTextures();

		/** Bind the the internal Texture Id indicated. This is usefull if you are rendering a texture outside this class.
		* @param TexId The internal Texture Id
		*/
		void Bind( const Uint32& TexId );

		/** Bind the the Texture indicated. This is usefull if you are rendering a texture outside this class.
		* @param Tex The Texture Pointer
		*/
		void Bind( const cTexture* Tex );

		/**
		* @param TexId The internal Texture Id
		* @return The OpenGL Texture Id (texture handler)
		*/
		Uint32 GetTextureId( const Uint32& TexId );

		/**
		* @return The real current texture id (OpenGL Texture Id)
		*/
		GLint GetCurrentTexture() const;

		/** Set the current internal texture id. This will set the TexId as the current texture binded.
		* @param TexId The internal Texture Id
		*/
		void SetCurrentTexture( const GLint& TexId );

		/** Returns the number of textures loaded */
		Uint32 GetNumTextures() const { return mTextures.size(); }

		/** Set the Blend Function
		* @param blend The Blend Mode
		* @param force If force to apply the blend ( no matters if the last blend was the same blend )
		*/
		void SetBlendFunc( const EE_RENDERALPHAS& blend, const bool& force = false );

		/**
		* @param Size
		* @return A valid texture size for the video card (checks if support non power of two textures)
		*/
		eeUint GetValidTextureSize( const eeUint& Size );

		/**	Saves an image from an array of unsigned chars to disk
		* @return False if failed, otherwise returns True
		*/
		bool SaveImage( const std::string& filepath, const EE_SAVETYPE& Format, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const unsigned char* data );

		/** Determine if the TextureId passed exists */
		bool TextureIdExists( const Uint32& TexId );

		/** @return A pointer to the Texture */
		cTexture* GetTexture( const Uint32& TexId );

		/** Get a local copy for all the textures */
		void GrabTextures();

		/** Allocate space for Textures (only works if EE_ALLOC_TEXTURES_ON_VECTOR is defined) */
		void Allocate( const eeUint& size );

		/** @return The memory used by the textures (in bytes) */
		eeUint MemorySize() { return mMemSize; }

		/** @return The texture size in memory (in bytes) */
		eeUint GetTexMemSize( const eeUint& TexId );

		/** It's possible to create textures outside the texture factory loader, but the library will need to know of this texture, so it's necessary to push the texture to the factory.
		* @param Filepath The Texture path ( if exists )
		* @param TexId The OpenGL Texture Id
		* @param Width Texture Width
		* @param Height Texture Height
		* @param ImgWidth Image Width.
		* @param ImgHeight Image Height
		* @param Mipmap Tell if the texture has mipmaps
		* @param Channels Texture number of Channels ( bytes per pixel )
		* @param ColorKey The transparent color key for the texture
		* @param ClampMode The Texture Clamp Mode
		* @param CompressTexture The texture is compressed?
		* @param LocalCopy If keep a local copy in memory of the texture
		*/
		Uint32 PushTexture( const std::string& Filepath, const Uint32& TexId, const eeUint& Width, const eeUint& Height, const eeUint& ImgWidth, const eeUint& ImgHeight, const bool& Mipmap, const eeUint& Channels, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& LocalCopy = false );
	protected:
		cTextureFactory();
		~cTextureFactory();

		GLint mCurrentTexture;

		EE_RENDERALPHAS mLastBlend;

		bool mPowOfTwo, mIsCalcPowOfTwo;

		std::vector<cTexture*> mTextures;

		eeUint mMemSize;

		std::queue<Uint32> mVectorFreeSlots;

		void UnloadTextures();

		Uint32 FindFreeSlot();
};

}}

#endif
