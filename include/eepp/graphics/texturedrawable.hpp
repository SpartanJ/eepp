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
	void draw();
	void draw( const Vector2f& position );
	void draw( const Vector2f& position, const Sizef& size );
	bool isStateful();
	DrawablePtr createInstance() const;

	const TexturePtr& getTexture() const;

  protected:
	TexturePtr mTexture;
	DrawableResourceConnection mTextureChangeConnection;
};

}} // namespace EE::Graphics

#endif
