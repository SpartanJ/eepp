#ifndef EE_GRAPHICS_GLYPHDRAWABLE_HPP
#define EE_GRAPHICS_GLYPHDRAWABLE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace Graphics {

class EE_API GlyphDrawable : public DrawableResource {
  public:
	static GlyphDrawable* New( Texture* texture, const Rect& srcRect,
							   const std::string& resourceName = "" );

	GlyphDrawable( Texture* texture, const Rect& srcRect, const std::string& resourceName = "" );

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful();

	/** @return The texture instance used by the GlyphDrawable. */
	Texture* getTexture();

	/** @return The Texture sector that represents the GlyphDrawable */
	const Rect& getSrcRect() const;

	/** @return This is the same as Destination Size but with the values rounded as integers. */
	Sizef getSize();

	const Float& getPixelDensity() const;
	void setPixelDensity(const Float& pixelDensity);

	protected:
	Texture* mTexture;
	Rectf mSrcRect;
	Float mPixelDensity;
};

}} // namespace EE::Graphics

#endif // EE_GRAPHICS_GLYPHDRAWABLE_HPP
