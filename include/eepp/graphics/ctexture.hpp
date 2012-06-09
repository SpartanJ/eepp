#ifndef EE_GRAPHICSCTEXTURE_H
#define EE_GRAPHICSCTEXTURE_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cimage.hpp>

namespace EE { namespace Graphics {

#define TEX_FLAG_MIPMAP 	( 1 << 0 )
#define TEX_FLAG_MODIFIED	( 1 << 1 )
#define TEX_FLAG_COMPRESSED ( 1 << 2 )
#define TEX_FLAG_LOCKED 	( 1 << 3 )
#define TEX_FLAG_GRABED		( 1 << 4 )

class EE_API cTexture : public cImage {
	public:
		cTexture();

		cTexture( const Uint32& texture, const eeUint& width, const eeUint& height, const eeUint& imgwidth, const eeUint& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint32& MemSize = 0, const Uint8* data = NULL );

		cTexture( const cTexture& Copy );

		virtual ~cTexture();

		/** Create the Texture
		* @param texture The OpenGL Texture Id (texture handler)
		* @param width The Texture Width
		* @param height The Texture Height
		* @param imgwidth The Image Width
		* @param imgheight The Image Height
		* @param UseMipmap Set if Texture has Mipmaps
		* @param Channels The Texture number of channels.
		* @param filepath The Texture File path
		* @param ColorKey The Texture Color Key
		* @param ClampMode The Texture Clamp Mode
		* @param CompressedTexture The Texture was compressed on loading
		* @param data The Texture (raw texture)
		*/
		void Create( const Uint32& texture, const eeUint& width, const eeUint& height, const eeUint& imgwidth, const eeUint& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint32& MemSize = 0, const Uint8* data = NULL );

		/** Set the OpenGL Texture Id (texture handle) */
		void Handle( const Uint32& texture ) { mTexture = texture; }

		/** @return The OpenGL Texture Id (texture handle) */
		Uint32 Handle() const { return mTexture; }

		/** @return The hash of the filename */
		const Uint32& HashName() const;

		/** Set the Texture File Path */
		void Filepath( const std::string& filepath ) { mFilepath = filepath; }

		/** @return The Texture File Path */
		std::string Filepath() const { return mFilepath; }

		/** @return The Image Width */
		eeUint ImgWidth() const { return mImgWidth; }

		/** @return The Image Height */
		eeUint ImgHeight() const { return mImgHeight; }

		/** Set if the Texture use Mipmaps */
		void Mipmap( const bool& UseMipmap );

		/** @return If the texture use Mipmaps */
		bool Mipmap() const;

		/** Set the Texture Clamp Mode */
		void ClampMode( const EE_CLAMP_MODE& clampmode );

		/** @return The Texture Clamp Mode */
		EE_CLAMP_MODE ClampMode() const { return mClampMode; }

		/** Lock the Texture for direct access */
		Uint8 * Lock( const bool& ForceRGBA = false );

		/** Unlock the previously locked Texture */
		bool Unlock(const bool& KeepData = false, const bool& Modified = false);

		/** @return A pointer to the first pixel of the texture ( keeped with a local copy ). \n You must have a copy of the texture on local memory. For that you need to Lock the texture first. */
		const Uint8* GetPixelsPtr();

		/** Set the Texture Filter Mode */
		void TextureFilter( const EE_TEX_FILTER& filter );

		const EE_TEX_FILTER& TextureFilter() const;

		/** Save the Texture to a new File */
		bool SaveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format );

		/** Replace a color on the texture */
		void ReplaceColor( const eeColorA& ColorKey, const eeColorA& NewColor);

		/** Create an Alpha mask from a Color */
		void CreateMaskFromColor( const eeColorA& ColorKey, Uint8 Alpha );

		/** Fill a texture with a color */
		void FillWithColor( const eeColorA& Color );

		/** Resize the texture */
		void Resize( const eeUint& new_width, const eeUint& new_height );

		/** Copy an image inside the texture */
		void CopyImage( cImage * Img, const eeUint& x, const eeUint& y );

		/** Flip the texture ( rotate the texture 90º ). Warning: This is flipped in memory, a real flipping. */
		void Flip();

		/** @return If the Texture has a copy on the local memory */
		bool LocalCopy();

		/** Unload the Texture from Memory */
		void DeleteTexture();

		/** Set if the Texture is Grabed */
		void Grabed( const bool& isGrabed );

		/** @return If the texture is Grabed */
		bool Grabed() const;

		/** @return The current texture filter */
		EE_TEX_FILTER Filter() const { return mFilter; }

		/** @return If the texture was compressed on load (DXT compression) */
		bool Compressed() const;

		/** Render the texture on screen ( with less internal mess, a little bit faster way )
		* @param x The x position on screen
		* @param y The y position on screen
		* @param Angle The Angle of the texture rendered
		* @param Scale The Scale factor of the rendered texture
		* @param Color The texture color
		* @param blend Set the Blend Mode ( default ALPHA_NORMAL )
		* @param width The width of the texture rendered
		* @param height The height of the texture rendered
		*/
		void DrawFast( const eeFloat& x, const eeFloat& y, const eeFloat& Angle = 0.0f, const eeFloat& Scale = 1.0f, const eeColorA& Color = eeColorA(), const EE_PRE_BLEND_FUNC &blend = ALPHA_NORMAL, const eeFloat &width = 0, const eeFloat &height = 0 );

		/** Render the texture on screen
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
		void Draw( const eeFloat &x, const eeFloat &y, const eeFloat &Angle = 0, const eeFloat &Scale = 1.0f, const eeColorA& Color = eeColorA(255,255,255,255), const EE_PRE_BLEND_FUNC &blend = ALPHA_NORMAL, const EE_RENDERTYPE &Effect = RN_NORMAL, const bool &ScaleCentered = true, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/** Render the texture on screen. Extended because can set the vertex colors individually
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
		void DrawEx( const eeFloat &x, const eeFloat &y, const eeFloat &width = 0.0f, const eeFloat &height = 0.0f, const eeFloat &Angle = 0, const eeFloat &Scale = 1.0f, const eeColorA& Color0 = eeColorA(255,255,255,255), const eeColorA& Color1 = eeColorA(255,255,255,255), const eeColorA& Color2 = eeColorA(255,255,255,255), const eeColorA& Color3 = eeColorA(255,255,255,255), const EE_PRE_BLEND_FUNC &blend = ALPHA_NORMAL, const EE_RENDERTYPE &Effect = RN_NORMAL, const bool &ScaleCentered = true, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/** Render the texture on screen
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
		void DrawEx2( const eeFloat &x, const eeFloat &y, const eeFloat &width = 0.0f, const eeFloat &height = 0.0f, const eeFloat &Angle = 0, const eeFloat &Scale = 1.0f, const eeColorA& Color = eeColorA(255,255,255,255), const EE_PRE_BLEND_FUNC &blend = ALPHA_NORMAL, const EE_RENDERTYPE &Effect = RN_NORMAL, const bool &ScaleCentered = true, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/** Render a GL_QUAD on Screen
		* @param Q The eeQuad2f
		* @param offsetx The Offset X applyed to all the coordinates on eeQuad2f
		* @param offsety The Offset Y applyed to all the coordinates on eeQuad2f
		* @param Angle The Angle of the eeQuad2f rendered
		* @param Scale The Scale of the eeQuad2f rendered
		* @param Color The eeQuad2f color
		* @param blend Set the Blend Mode ( default ALPHA_NORMAL )
		* @param texSector The texture sector to render. You can render only a part of the texture. ( default render all the texture )
		*/
		void DrawQuad( const eeQuad2f& Q, const eeFloat &offsetx = 0.0f, const eeFloat &offsety = 0.0f, const eeFloat &Angle = 0.0f, const eeFloat &Scale = 1.0f, const eeColorA& Color = eeColorA(255,255,255,255), const EE_PRE_BLEND_FUNC &blend = ALPHA_NORMAL, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/** Render a GL_QUAD on Screen
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
		void DrawQuadEx( const eeQuad2f& Q, const eeFloat &offsetx = 0.0f, const eeFloat &offsety = 0.0f, const eeFloat &Angle = 0.0f, const eeFloat &Scale = 1.0f, const eeColorA& Color0 = eeColorA(255,255,255,255), const eeColorA& Color1 = eeColorA(255,255,255,255), const eeColorA& Color2 = eeColorA(255,255,255,255), const eeColorA& Color3 = eeColorA(255,255,255,255), const EE_PRE_BLEND_FUNC &blend = ALPHA_NORMAL, const eeRecti& texSector = eeRecti(0,0,0,0) );

		/** Set the texture factory internal id of the texture */
		void Id( const Uint32& id );

		/** @return The texture factory internal id of the texture */
		const Uint32& Id() const;

		/** Reload the texture from the current local copy. */
		void Reload();

		/** Set a pixel to the locked texture. */
		void SetPixel( const eeUint& x, const eeUint& y, const eeColorA& Color );
	protected:
		std::string 	mFilepath;

		Uint32 			mId;
		Uint32 			mTexId;
		GLint 			mTexture;

		eeUint 			mImgWidth;
		eeUint 			mImgHeight;

		Uint32			mFlags;

		EE_CLAMP_MODE 	mClampMode;
		EE_TEX_FILTER 	mFilter;

		GLint			mInternalFormat;

		void 			ApplyClampMode();

		Uint8 * 		iLock( const bool& ForceRGBA, const bool& KeepFormat );

		void			iTextureFilter( const EE_TEX_FILTER& filter );
};

}}

#endif
