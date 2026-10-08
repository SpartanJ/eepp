#ifndef EE_GRAPHICS_GLYPHDRAWABLE_HPP
#define EE_GRAPHICS_GLYPHDRAWABLE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace Graphics {

class VertexBuffer;

enum class GlyphRenderMode : Uint8 { Mask, Subpixel, Color };

class EE_API GlyphDrawable : public DrawableResource {
  public:
	static GlyphDrawable* New( TexturePtr texture, const Rect& srcRect, const Sizef& destSize = {},
							   const std::string& resourceName = "" );

	enum class DrawMode {
		Image,	   ///< It will be treated as a simple image, no special offset is applied.
		Text,	   ///< Will add the glyph offset corresponding to that character
		TextItalic ///< Will add the glyph offset corresponding to that character and simulate
				   ///< italic skew
	};

	GlyphDrawable( TexturePtr texture, const Rect& srcRect, const Sizef& destSize = {},
				   const std::string& resourceName = "" );

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual void drawIntoVertexBuffer( VertexBuffer* vbo, const Vector2u& gridPos,
									   const Vector2f& pos, const Uint32& textureLevel = 0 );

	virtual bool isStateful();

	DrawablePtr clone() const;

	/** @return The texture instance used by the GlyphDrawable. */
	const TexturePtr& getTexture() const;

	/** @return The Texture sector that represents the GlyphDrawable */
	inline const Rectf& getSrcRect() const { return mSrcRect; }

	inline const Sizef& getDestSize() const { return mDestSize; }

	/** @return This is the same as Destination Size but with the values rounded as integers. */
	Sizef getSize();

	Sizef getPixelsSize();

	inline const Float& getPixelDensity() const { return mPixelDensity; }

	void setPixelDensity( const Float& pixelDensity );

	inline const Vector2f& getGlyphOffset() const { return mGlyphOffset; }

	void setGlyphOffset( const Vector2f& glyphOffset );

	const DrawMode& getDrawMode() const;

	void setDrawMode( const DrawMode& drawMode );

	inline bool isItalic() const { return mIsItalic; }

	void setIsItalic( bool isItalic );

	inline const Float& getAdvance() const { return mAdvance; }

	void setAdvance( Float advance );

	GlyphRenderMode getGlyphRenderMode() const;

	void setGlyphRenderMode( GlyphRenderMode renderMode );

  protected:
	TexturePtr mTexture;
	Rectf mSrcRect;
	Sizef mDestSize;
	Float mPixelDensity;
	Vector2f mGlyphOffset;
	DrawMode mDrawMode{ DrawMode::Image };
	Float mAdvance{ 0 };
	GlyphRenderMode mGlyphRenderMode{ GlyphRenderMode::Mask };
	bool mIsItalic{ false };
};

}} // namespace EE::Graphics

#endif // EE_GRAPHICS_GLYPHDRAWABLE_HPP
