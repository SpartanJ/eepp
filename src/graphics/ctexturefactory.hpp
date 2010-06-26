#ifndef EECTEXTUREFACTORY_H
#define EECTEXTUREFACTORY_H

#include "base.hpp"
#include "ctexture.hpp"
#include "cglobalbatchrenderer.hpp"

namespace EE { namespace Graphics {

class cGlobalBatchRenderer;

/** @brief The Texture Manager Class. Here we do all the textures stuff. (Singleton Class) */
class EE_API cTextureFactory: public cSingleton<cTextureFactory> {
	friend class cSingleton<cTextureFactory>;
	public:
		/** Create an empty texture
		* @param Width Texture Width
		* @param Height Texture Height
		* @param DefaultColor The background color for the texture
		* @param mipmap Create Mipmap?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @return Internal Texture Id
		*/
		eeUint CreateEmptyTexture( const eeUint& Width, const eeUint& Height, const eeColorA& DefaultColor = eeColorA(0,0,0,255), const bool& mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );
		
		/** Loads a RAW Texture from Memory
		* @param Surface The Texture array
		* @param Width Texture Width
		* @param Height Texture Height
		* @param Channels Texture Number of Channels (in bytes)
		* @param mipmap Create Mipmap?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( usefull if want to reload the texture )
		* @return Internal Texture Id
		*/
		eeUint LoadFromPixels( const unsigned char* Surface, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& mipmap = false, const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Load a texture from Pack
		* @param Pack Pointer to the pack instance
		* @param FilePackPath The path of the file inside the pack
		* @param mipmap Create Mipmap?
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @param KeepLocalCopy Keep the array data copy. ( usefull if want to reload the texture )
		* @return Internal Texture Id
		*/
		eeUint LoadFromPack( cPack* Pack, const std::string& FilePackPath, const bool& mipmap = false, const eeRGB& ColorKey = eeRGB(true), const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );
		
		/** Load a texture from memory (RGBA Format)
		* @param Surface The image data in RAM just as if it were still in a file
		* @param Size The size of the texture ( Width * Height * BytesPerPixel )
		* @param mipmap Use mipmaps?
		* @param ColorKey The ColorKey for the texture
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @return The internal Texture Id
		*/
		eeUint LoadFromMemory( const unsigned char* Surface, const eeUint& Size, const bool& mipmap = false, const eeRGB& ColorKey = eeRGB(true), const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false, const bool& KeepLocalCopy = false );

		/** Load a Texture from a file path
		* @param filepath The path for the texture
		* @param mipmap Use mipmaps?
		* @param ColorKey The ColorKey for the texture
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @return The internal Texture Id
		*/
		eeUint Load( const std::string& filepath, const bool& mipmap = false, const eeRGB& ColorKey  = eeRGB(true), const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false );

		/** Reload a Texture Id
		* @param TexId The internal Texture Id
		* @param filepath If filepath is empty reload the same texture, otherwise loads a new texture on the Texture Id slot
		* @param mipmap Use mipmaps?
		* @param ColorKey  The ColorKey for the texture
		* @param ClampMode Defines the CLAMP MODE
		* @param CompressTexture If use the DXT compression on the texture loading ( if the card can display them, will convert RGB to DXT1, RGBA to DXT5 )
		* @return Returns the Texture Id
		*/
		Uint32 Reload( const Uint32& TexId, const std::string& filepath = "", const bool& mipmap = false, const eeRGB& ColorKey = eeRGB(true), const EE_CLAMP_MODE& ClampMode = EE_CLAMP_TO_EDGE, const bool& CompressTexture = false );

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

		/** Render the texture on screen ( with less internal mess, a little bit faster way )
		* @param TexId The internal Texture Id
		* @param x The x position on screen
		* @param y The y position on screen
		* @param Angle The Angle of the texture rendered
		* @param Scale The Scale factor of the rendered texture
		* @param Color The texture color
		* @param blend Set the Blend Mode ( default ALPHA_NORMAL )
		* @param width The width of the texture rendered
		* @param height The height of the texture rendered
		*/
		void DrawFast( const Uint32& TexId, const eeFloat& x, const eeFloat& y, const eeFloat& Angle = 0.0f, const eeFloat& Scale = 1.0f, const eeColorA& Color = eeColorA(), const EE_RENDERALPHAS &blend = ALPHA_NORMAL, const eeFloat &width = 0, const eeFloat &height = 0 );
		
		/** Render the texture on screen
		* @param TexId The internal Texture Id
		* @param x The x position on screen
		* @param y The y position on screen
		* @param width The width of the texture rendered ( when Scale = 1, otherwise this width will be scaled like width * Scale )
		* @param height The height of the texture rendered ( when Scale = 1, otherwise this height will be scaled like height * Scale )
		* @param Angle The Angle of the texture rendered
		* @param Scale The Scale factor of the rendered texture
		* @param Color The texture color
		* @param blend Set the Blend Mode ( default ALPHA_NORMAL )
		* @param Effect Set the Render Effect ( default RN_NORMAL, no effect )
		* @param ScaleCentered If true the texture will be scaled centered, otherwise will be scale from the Top - Left Corner
		* @param texSector The texture sector to render. You can render only a part of the texture. ( default render all the texture )
		*/
		void Draw( const Uint32 &TexId, const eeFloat &x, const eeFloat &y, const eeFloat &Angle = 0, const eeFloat &Scale = 1.0f, const eeColorA& Color = eeColorA(255,255,255,255), const EE_RENDERALPHAS &blend = ALPHA_NORMAL, const EE_RENDERTYPE &Effect = RN_NORMAL, const bool &ScaleCentered = true, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/** Render the texture on screen. Extended because can set the vertex colors individually
		* @param TexId The internal Texture Id
		* @param x The x position on screen
		* @param y The y position on screen
		* @param width The width of the texture rendered ( when Scale = 1, otherwise this width will be scaled like width * Scale )
		* @param height The height of the texture rendered ( when Scale = 1, otherwise this height will be scaled like height * Scale )
		* @param Angle The Angle of the texture rendered
		* @param Scale The Scale factor of the rendered texture
		* @param Color0 The Left - Top vertex color
		* @param Color1 The Left - Bottom vertex color
		* @param Color2 The Right - Bottom vertex color
		* @param Color3 The Right - Top vertex color
		* @param blend Set the Blend Mode ( default ALPHA_NORMAL )
		* @param Effect Set the Render Effect ( default RN_NORMAL, no effect )
		* @param ScaleCentered If true the texture will be scaled centered, otherwise will be scale from the Top - Left Corner
		* @param texSector The texture sector to render. You can render only a part of the texture. ( default render all the texture )
		*/
		void DrawEx( const Uint32 &TexId, const eeFloat &x, const eeFloat &y, const eeFloat &width = 0.0f, const eeFloat &height = 0.0f, const eeFloat &Angle = 0, const eeFloat &Scale = 1.0f, const eeColorA& Color0 = eeColorA(255,255,255,255), const eeColorA& Color1 = eeColorA(255,255,255,255), const eeColorA& Color2 = eeColorA(255,255,255,255), const eeColorA& Color3 = eeColorA(255,255,255,255), const EE_RENDERALPHAS &blend = ALPHA_NORMAL, const EE_RENDERTYPE &Effect = RN_NORMAL, const bool &ScaleCentered = true, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/** Render a GL_QUAD on Screen
		* @param TexId The internal Texture Id
		* @param Q The eeQuad2f
		* @param offsetx The Offset X applyed to all the coordinates on eeQuad2f
		* @param offsety The Offset Y applyed to all the coordinates on eeQuad2f
		* @param Angle The Angle of the eeQuad2f rendered
		* @param Scale The Scale of the eeQuad2f rendered
		* @param Color The eeQuad2f color
		* @param blend Set the Blend Mode ( default ALPHA_NORMAL )
		* @param texSector The texture sector to render. You can render only a part of the texture. ( default render all the texture )
		*/
		void DrawQuad( const Uint32 &TexId, const eeQuad2f& Q, const eeFloat &offsetx = 0.0f, const eeFloat &offsety = 0.0f, const eeFloat &Angle = 0.0f, const eeFloat &Scale = 1.0f, const eeColorA& Color = eeColorA(255,255,255,255), const EE_RENDERALPHAS &blend = ALPHA_NORMAL, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/** Render a GL_QUAD on Screen
		* @param TexId The internal Texture Id
		* @param Q The eeQuad2f
		* @param offsetx The Offset X applyed to all the coordinates on eeQuad2f
		* @param offsety The Offset X applyed to all the coordinates on eeQuad2f
		* @param Angle The Angle of the eeQuad2f rendered
		* @param Scale The Scale of the eeQuad2f rendered
		* @param Color0 The Left - Top vertex color
		* @param Color1 The Left - Bottom vertex color
		* @param Color2 The Right - Bottom vertex color
		* @param Color3 The Right - Top vertex color
		* @param blend Set the Blend Mode ( default ALPHA_NORMAL )
		* @param texSector The texture sector to render. You can render only a part of the texture. ( default render all the texture )
		*/
		void DrawQuadEx( const Uint32 &TexId, const eeQuad2f& Q, const eeFloat &offsetx = 0.0f, const eeFloat &offsety = 0.0f, const eeFloat &Angle = 0.0f, const eeFloat &Scale = 1.0f, const eeColorA& Color0 = eeColorA(255,255,255,255), const eeColorA& Color1 = eeColorA(255,255,255,255), const eeColorA& Color2 = eeColorA(255,255,255,255), const eeColorA& Color3 = eeColorA(255,255,255,255), const EE_RENDERALPHAS &blend = ALPHA_NORMAL, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/**
		* @param TexId The internal Texture Id
		* @return The OpenGL Texture Id (texture handler)
		*/
		Uint32 GetTextureId( const Uint32& TexId );

		/**
		* @return The real current texture id  (OpenGL Texture Id)
		*/
		Uint32 GetCurrentTexture() const;

		/** Set the current internal texture id. This will set the TexId as the current texture binded.
		* @param TexId The internal Texture Id
		*/
		void SetCurrentTexture( const Uint32& TexId );

		/**
		* @param TexId The internal Texture Id
		* @return The Texture Width
		*/
		eeFloat GetTextureWidth( const Uint32 &TexId );

		/**
		* @param TexId The internal Texture Id
		* @return The Texture Height
		*/
		eeFloat GetTextureHeight( const Uint32& TexId );

		/**
		* @param TexId The internal Texture Id
		* @return The Texture Path
		*/
		std::string GetTexturePath( const Uint32& TexId );

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
	protected:
		cTextureFactory();
		~cTextureFactory();
		
		cLog* Log;
		
		GLint mCurrentTexture;
		EE_RENDERALPHAS mLastBlend;
		
		bool mPowOfTwo, mIsCalcPowOfTwo;
		
		std::vector<cTexture*> mTextures;
		
		Uint32 mNextKey;
		eeUint mMemSize;
		
		cGlobalBatchRenderer* BR;
		
		eeUint iLoadFromPixels( const unsigned char* Surface, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const Uint32& TexPos = 0 );
		eeUint iLoad( const std::string& filepath, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const Uint32& TexPos = 0 );
		
		void UnloadTextures();
		
		eeUint PushTexture( const std::string& filepath, const Uint32& TexId, const eeUint& Width, const eeUint& Height, const eeUint& ImgWidth, const eeUint& ImgHeight, const bool& Mipmap, const eeUint& Channels, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const Uint32& TexPos = 0, const bool& LocalCopy = false );
		void Bind( const cTexture* Tex );
		
		GLint GetPrevTex();
		void BindPrev( const GLint& Prev );
};

}}

#endif
