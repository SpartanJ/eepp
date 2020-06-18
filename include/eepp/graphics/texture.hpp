#ifndef EE_GRAPHICSCTEXTURE_H
#define EE_GRAPHICSCTEXTURE_H

#include <eepp/core.hpp>
#include <eepp/core/noncopyable.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/math/originpoint.hpp>
#include <eepp/math/polygon2.hpp>

namespace EE { namespace Graphics {

class EE_API Texture : public DrawableResource, public Image, private NonCopyable {
  public:
	/** @enum TextureFilter Defines the texture filter used. */
	enum class Filter : Uint32 {
		Linear, //!< Linear filtering (Smoothed Zoom)
		Nearest //!< No filtering (Pixeled Zoom)
	};

	/** @enum ClampMode Set the clamp mode of the texture. */
	enum class ClampMode : Uint32 { ClampToEdge, ClampRepeat };

	enum class CoordinateType : Uint32 {
		Normalized, ///< Texture coordinates in range [0 .. 1]
		Pixels		///< Texture coordinates in range [0 .. size]
	};

	static Uint32 getMaximumSize();

	/** Set the OpenGL Texture Id (texture handle) */
	void setHandle( const int& texture ) { mTexture = texture; }

	/** @return The OpenGL Texture Id (texture handle) */
	int getHandle() const { return mTexture; }

	/** @return The hash of the filename */
	const String::HashType& getHashName() const;

	/** Set the Texture File Path */
	void setFilepath( const std::string& filepath ) { mFilepath = filepath; }

	/** @return The Texture File Path */
	std::string getFilepath() const { return mFilepath; }

	/** @return The Image Width */
	unsigned int getImageWidth() const { return mImgWidth; }

	/** @return The Image Height */
	unsigned int getImageHeight() const { return mImgHeight; }

	/** Set if the Texture use Mipmaps */
	void setMipmap( const bool& UseMipmap );

	/** @return If the texture use Mipmaps */
	bool getMipmap() const;

	/** Set the Texture Clamp Mode */
	void setClampMode( const Texture::ClampMode& clampmode );

	/** @return The Texture Clamp Mode */
	ClampMode getClampMode() const { return mClampMode; }

	/** Lock the Texture for direct access.
	**	It is needed to have any read/write access to the texture. This feature is not supported in
	*OpenGL ES. */
	Uint8* lock( const bool& ForceRGBA = false );

	/** @brief Unlock the previously locked Texture.
	 *	Unlocking the texture will upload the local copy of the texture ( that could have been
	 *modified ) to the GPU.
	 *	@param KeepData If true keeps the local copy of the texture un memory, otherwise it will be
	 *released.
	 *	@param Modified The flag indicates of the texture was modified between the Lock and Unlock
	 *calls. This is to force reloading the texture from memory to VRAM. In the case that the
	 *texture in ram was modified using the Texture methods to do this, it will already know that
	 *this is true, so it will upload the changes to the GPU.
	 */
	bool unlock( const bool& KeepData = false, const bool& Modified = false );

	/** @return A pointer to the first pixel of the texture ( keeped with a local copy ). \n You
	 * must have a copy of the texture on local memory. For that you need to Lock the texture first.
	 */
	const Uint8* getPixelsPtr();

	/** Set the Texture Filter Mode */
	void setFilter( const Filter& filter );

	/** @return The texture filter used by the texture */
	const Filter& getFilter() const;

	/** Save the Texture to a new File */
	bool saveToFile( const std::string& filepath, const Image::SaveType& Format );

	/** Replace a color on the texture */
	void replaceColor( const Color& ColorKey, const Color& NewColor );

	/** Create an Alpha mask from a Color */
	void createMaskFromColor( const Color& ColorKey, Uint8 Alpha );

	/** Fill a texture with a color */
	void fillWithColor( const Color& Color );

	/** Resize the texture */
	void resize( const Uint32& newWidth, const Uint32& newHeight,
				 ResamplerFilter filter = ResamplerFilter::RESAMPLER_LANCZOS4 );

	/** Scale the texture */
	void scale( const Float& scale, ResamplerFilter filter = ResamplerFilter::RESAMPLER_LANCZOS4 );

	/** Copy an image inside the texture */
	void copyImage( Image* image, const Uint32& x, const Uint32& y );

	/** @brief Update a part of the texture from an array of pixels
	**	The size of the @a pixel array must match the @a width and
	**	@a height arguments.
	**	No additional check is performed on the size of the pixel
	**	array or the bounds of the area to update, passing invalid
	**	arguments will lead to an undefined behaviour.
	**	This function does nothing if @a pixels is null or if the
	**	texture was not previously created.
	**	@param pixels Array of pixels to copy to the texture
	**	@param width  Width of the pixel region contained in @a pixels
	**	@param height Height of the pixel region contained in @a pixels
	**	@param x X offset in the texture where to copy the source pixels
	**	@param y Y offset in the texture where to copy the source pixels
	**	@param pf The pixel format of the @a pixel */
	void update( const Uint8* pixels, Uint32 width, Uint32 height, Uint32 x = 0, Uint32 y = 0,
				 PixelFormat pf = PixelFormat::PIXEL_FORMAT_RGBA );

	/** @brief Update the whole texture from an array of pixels
	**	The @a pixel array is assumed to have the same size as
	**	the @a area rectangle.
	**	No additional check is performed on the size of the pixel
	**	array, passing invalid arguments will lead to an undefined
	**	behaviour.
	**	This function does nothing if @a pixels is null or if the
	**	texture was not previously created.
	**	@param pixels Array of pixels to copy to the texture */
	void update( const Uint8* pixels );

	/** @brief Update a part of the texture from an image
	**	The pixel format is automatically detected
	**	No additional check is performed on the size of the image,
	**	passing an invalid combination of image size and offset
	**	will lead to an undefined behaviour.
	**	This function does nothing if the texture was not
	**	previously created.
	**	@param image Image to copy to the texture
	**	@param x X offset in the texture where to copy the source image
	**	@param y Y offset in the texture where to copy the source image */
	void update( Image* image, Uint32 x = 0, Uint32 y = 0 );

	/** Replaces the current texture with the image provided, reusing the current texture id. */
	void replace( Image* image );

	/** Flip the texture ( rotate the texture 90º ). Warning: This is flipped in memory, a real
	 * flipping. */
	void flip();

	/** @return If the Texture has a copy on the local memory */
	bool hasLocalCopy();

	/** Unload the Texture from Memory */
	void deleteTexture();

	/** Set if the Texture is Grabed */
	void setGrabed( const bool& isGrabed );

	/** @return If the texture is Grabed */
	bool isGrabed() const;

	/** @return If the texture was compressed on load (DXT compression) */
	bool isCompressed() const;

	/** Render the texture on screen ( with less internal mess, a little bit faster way )
	 * @param x The x position on screen
	 * @param y The y position on screen
	 * @param Angle The Angle of the texture rendered
	 * @param scale The Scale factor of the rendered texture
	 * @param color The texture color
	 * @param Blend Set the Blend Mode ( default BlendAlpha )
	 * @param width The width of the texture rendered
	 * @param height The height of the texture rendered
	 */
	void drawFast( const Float& x, const Float& y, const Float& Angle = 0.0f,
				   const Vector2f& scale = Vector2f::One, const Color& color = Color::White,
				   const BlendMode& Blend = BlendAlpha, const Float& width = 0,
				   const Float& height = 0 );

	/** Render the texture on screen
	 * @param x The x position on screen
	 * @param y The y position on screen
	 * @param Angle The Angle of the texture rendered
	 * @param scale The Scale factor of the rendered texture
	 * @param color The texture color
	 * @param Blend Set the Blend Mode ( default BlendAlpha )
	 * @param Effect Set the Render Effect ( default RN_NORMAL, no effect )
	 * @param Center The rotation and scaling center. The center point is relative to the top-left
	 * corner of the object.
	 * @param texSector The texture sector to render. You can render only a part of the texture. (
	 * default render all the texture )
	 */
	void draw( const Float& x, const Float& y, const Float& Angle = 0,
			   const Vector2f& scale = Vector2f::One, const Color& color = Color::White,
			   const BlendMode& Blend = BlendAlpha, const RenderMode& Effect = RENDER_NORMAL,
			   OriginPoint Center = OriginPoint( OriginPoint::OriginCenter ),
			   const Rect& texSector = Rect( 0, 0, 0, 0 ) );

	/** Render the texture on screen. Extended because can set the vertex colors individually
	 * @param x The x position on screen
	 * @param y The y position on screen
	 * @param width The width of the texture rendered ( when Scale = 1, otherwise this width will be
	 * scaled like width * Scale )
	 * @param height The height of the texture rendered ( when Scale = 1, otherwise this height will
	 * be scaled like height * Scale )
	 * @param Angle The Angle of the texture rendered
	 * @param scale The Scale factor of the rendered texture
	 * @param Color0 The Left - Top vertex color
	 * @param Color1 The Left - Bottom vertex color
	 * @param Color2 The Right - Bottom vertex color
	 * @param Color3 The Right - Top vertex color
	 * @param Blend Set the Blend Mode ( default BlendAlpha )
	 * @param Effect Set the Render Effect ( default RN_NORMAL, no effect )
	 * @param Center The rotation and scaling center. The center point is relative to the top-left
	 * corner of the object.
	 * @param texSector The texture sector to render. You can render only a part of the texture. (
	 * default render all the texture )
	 */
	void drawEx( Float x, Float y, Float width = 0.0f, Float height = 0.0f, const Float& Angle = 0,
				 const Vector2f& scale = Vector2f::One, const Color& Color0 = Color::White,
				 const Color& Color1 = Color( 255, 255, 255, 255 ),
				 const Color& Color2 = Color( 255, 255, 255, 255 ),
				 const Color& Color3 = Color( 255, 255, 255, 255 ),
				 const BlendMode& Blend = BlendAlpha, const RenderMode& Effect = RENDER_NORMAL,
				 OriginPoint Center = OriginPoint( OriginPoint::OriginCenter ),
				 const Rect& texSector = Rect( 0, 0, 0, 0 ) );

	/** Render a quad on Screen
	 * @param Q The Quad2f
	 * @param Offset The Offset applied to all the coordinates on Quad2f
	 * @param Angle The Angle of the Quad2f rendered
	 * @param scale The Scale of the Quad2f rendered
	 * @param color The Quad2f color
	 * @param Blend Set the Blend Mode ( default BlendAlpha )
	 * @param texSector The texture sector to render. You can render only a part of the texture. (
	 * default render all the texture )
	 */
	void drawQuad( const Quad2f& Q, const Vector2f& Offset = Vector2f(), const Float& Angle = 0.0f,
				   const Vector2f& scale = Vector2f::One, const Color& color = Color::White,
				   const BlendMode& Blend = BlendAlpha,
				   const Rect& texSector = Rect( 0, 0, 0, 0 ) );

	/** Render a quad on Screen
	 * @param Q The Quad2f
	 * @param Offset The Offset applied to all the coordinates on Quad2f
	 * @param Angle The Angle of the Quad2f rendered
	 * @param scale The Scale of the Quad2f rendered
	 * @param Color0 The Left - Top vertex color
	 * @param Color1 The Left - Bottom vertex color
	 * @param Color2 The Right - Bottom vertex color
	 * @param Color3 The Right - Top vertex color
	 * @param Blend Set the Blend Mode ( default BlendAlpha )
	 * @param texSector The texture sector to render. You can render only a part of the texture. (
	 * default render all the texture )
	 */
	void drawQuadEx( Quad2f Q, const Vector2f& Offset = Vector2f(), const Float& Angle = 0.0f,
					 const Vector2f& scale = Vector2f::One, const Color& Color0 = Color::White,
					 const Color& Color1 = Color( 255, 255, 255, 255 ),
					 const Color& Color2 = Color( 255, 255, 255, 255 ),
					 const Color& Color3 = Color( 255, 255, 255, 255 ),
					 const BlendMode& Blend = BlendAlpha, Rect texSector = Rect( 0, 0, 0, 0 ) );

	Sizef getSize();

	Sizei getPixelSize();

	void draw();

	void draw( const Vector2f& position );

	void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	/** Set the texture factory internal id of the texture */
	void setTextureId( const Uint32& id );

	/** @return The texture factory internal id of the texture */
	const Uint32& getTextureId() const;

	/** Reload the texture from the current local copy. */
	void reload();

	/** Set a pixel to the locked texture. */
	void setPixel( const unsigned int& x, const unsigned int& y, const Color& Color );

	/** Bind the texture. Activate the texture for rendering.
	 * @param coordinateType Type of texture coordinates to use
	 * @param textureUnit The Texture unit that want to be used to bind ( usually 0 )
	 */
	void bind( CoordinateType coordinateType, const Uint32& textureUnit = 0 );

	/** Bind the texture. Activate the texture for rendering.
	 * @param textureUnit The Texture unit that want to be used to bind ( usually 0 )
	 */
	void bind( const Uint32& textureUnit = 0 );

	virtual ~Texture();

	/** return The reference coordinate type. */
	const CoordinateType& getCoordinateType() const;

	/** Sets the default coordinate type. This value is not forced when binded, but used as a
	reference for binding in the case of textures with a reference coordinate type. */
	void setCoordinateType( const CoordinateType& coordinateType );

  protected:
	enum TEXTURE_FLAGS {
		TEX_FLAG_MIPMAP = ( 1 << 0 ),
		TEX_FLAG_MODIFIED = ( 1 << 1 ),
		TEX_FLAG_COMPRESSED = ( 1 << 2 ),
		TEX_FLAG_LOCKED = ( 1 << 3 ),
		TEX_FLAG_GRABED = ( 1 << 4 )
	};

	friend class TextureFactory;

	Texture();

	Texture( const Uint32& texture, const unsigned int& width, const unsigned int& height,
			 const unsigned int& imgwidth, const unsigned int& imgheight, const bool& UseMipmap,
			 const unsigned int& channels, const std::string& filepath,
			 const Texture::ClampMode& clampMode, const bool& CompressedTexture,
			 const Uint32& memSize = 0, const Uint8* data = NULL );

	Texture( const Texture& copy );

	void create( const Uint32& texture, const unsigned int& width, const unsigned int& height,
				 const unsigned int& imgwidth, const unsigned int& imgheight, const bool& UseMipmap,
				 const unsigned int& channels, const std::string& filepath,
				 const Texture::ClampMode& clampMode, const bool& CompressedTexture,
				 const Uint32& memSize = 0, const Uint8* data = NULL );

	std::string mFilepath;
	Uint32 mTexId;
	int mTexture;

	unsigned int mImgWidth;
	unsigned int mImgHeight;

	Uint32 mFlags;

	ClampMode mClampMode;
	Filter mFilter;
	CoordinateType mCoordinateType;

	int mInternalFormat;

	void applyClampMode();

	Uint8* iLock( const bool& ForceRGBA, const bool& KeepFormat );

	void iTextureFilter( const Filter& filter );
};

}} // namespace EE::Graphics

#endif
