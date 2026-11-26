#ifndef EE_GRAPHICS_TEXTUREDRAWABLE_HPP
#define EE_GRAPHICS_TEXTUREDRAWABLE_HPP

#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace Graphics {

class TextureDrawable;
using TextureDrawablePtr = ResourcePtr<TextureDrawable>;

/** Per-consumer drawable state backed by a shared texture resource. */
class EE_API TextureDrawable : public DrawableResource {
  public:
	static TextureDrawablePtr New( TexturePtr texture );

	explicit TextureDrawable( TexturePtr texture );

	Sizef getSize();
	Sizef getPixelsSize();
	Sizef getNaturalSize();
	Sizef getNaturalPixelsSize();
	bool isVectorRaster() const;
	virtual Float getRasterScale() const;
	void draw();
	void draw( const Vector2f& position );
	void draw( const Vector2f& position, const Sizef& size );
	bool isStateful();
	DrawablePtr clone() const;

	const TexturePtr& getTexture() const;

	/** Draws @p texture from now on and notifies resource-change listeners. */
	void setTexture( TexturePtr texture );

  protected:
	TexturePtr mTexture;
	DrawableResourceConnection mTextureChangeConnection;
};

}} // namespace EE::Graphics

#endif
