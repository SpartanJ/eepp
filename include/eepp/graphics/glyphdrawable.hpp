#ifndef EE_GRAPHICS_GLYPHDRAWABLE_HPP
#define EE_GRAPHICS_GLYPHDRAWABLE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace Graphics {

class VertexBuffer;

class EE_API GlyphDrawable : public DrawableResource {
  public:
	static GlyphDrawable* New( Texture* texture, const Rect& srcRect, const Sizef& destSize = {},
							   const std::string& resourceName = "" );

	enum class DrawMode {
		Image,	   ///< It will be treated as a simple image, no special offset is applied.
		Text,	   ///< Will add the glyph offset corresponding to that character
		TextItalic ///< Will add the glyph offset corresponding to that character and simulate
				   ///< italic skew
	};

	GlyphDrawable( Texture* texture, const Rect& srcRect, const Sizef& destSize = {},
				   const std::string& resourceName = "" );

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual void drawIntoVertexBuffer( VertexBuffer* vbo, const Vector2u& gridPos,
									   const Vector2f& pos, const Uint32& textureLevel = 0 );

	virtual bool isStateful();

	/** @return The texture instance used by the GlyphDrawable. */
	Texture* getTexture();

	/** @return The Texture sector that represents the GlyphDrawable */
	const Rectf& getSrcRect() const;

	const Sizef& getDestSize() const;

	/** @return This is the same as Destination Size but with the values rounded as integers. */
	Sizef getSize();

	Sizef getPixelsSize();

	const Float& getPixelDensity() const;

	void setPixelDensity( const Float& pixelDensity );

	const Vector2f& getGlyphOffset() const;

	void setGlyphOffset( const Vector2f& glyphOffset );

	const DrawMode& getDrawMode() const;

	void setDrawMode( const DrawMode& drawMode );

	bool isItalic() const { return mIsItalic; }

	void setIsItalic( bool isItalic );

	const Float& getAdvance() const;

	void setAdvance( Float advance );

  protected:
	Texture* mTexture;
	Rectf mSrcRect;
	Sizef mDestSize;
	Float mPixelDensity;
	Vector2f mGlyphOffset;
	DrawMode mDrawMode{ DrawMode::Image };
	Float mAdvance{ 0 };
	bool mIsItalic{ false };
};

}} // namespace EE::Graphics

#endif // EE_GRAPHICS_GLYPHDRAWABLE_HPP
