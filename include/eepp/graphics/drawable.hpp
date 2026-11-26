#ifndef EE_GRAPHICS_DRAWABLE_HPP
#define EE_GRAPHICS_DRAWABLE_HPP

#include <eepp/graphics/blendmode.hpp>
#include <eepp/graphics/rendermode.hpp>
#include <eepp/graphics/resource.hpp>
#include <eepp/math/size.hpp>
#include <eepp/system/color.hpp>
using namespace EE::Math;
using namespace EE::System;

namespace EE { namespace Graphics {

class Drawable;
class StatefulDrawable;

using DrawablePtr = ResourcePtr<Drawable>;
using DrawableWeakPtr = ResourceWeakPtr<Drawable>;

class EE_API Drawable {
  public:
	enum Type {
		TEXTURE,
		TEXTUREDRAWABLE,
		TEXTUREREGION,
		SPRITE,
		ARC,
		RECTANGLE,
		TRIANGLE,
		CONVEXSHAPE,
		GROUP,
		NINEPATCH,
		STATELIST,
		SKIN,
		GLYPH,
		UINODEDRAWABLE,
		UINODEDRAWABLE_LAYERDRAWABLE,
		UIBORDERDRAWABLE,
		UIBACKGROUNDDRAWABLE,
		RICHTEXT,
		LINEARGRADIENT,
		REPEATINGLINEARGRADIENT,
		RADIALGRADIENT,
		REPEATINGRADIALGRADIENT,
		CUSTOM
	};

	virtual ~Drawable();

	virtual Sizef getSize() = 0;

	virtual Sizef getPixelsSize() = 0;

	/** @return The size to lay the drawable out at, in dp. It differs from getSize() for an SVG
	 * raster, which keeps the size of its SVG at the current density whatever size it was
	 * rasterized at, so that rasterizing it again doesn't change layout. */
	virtual Sizef getNaturalSize() { return getSize(); }

	/** @return getNaturalSize() in pixels. */
	virtual Sizef getNaturalPixelsSize() { return getPixelsSize(); }

	/** @return Whether the drawable draws an SVG raster: its natural size is already at the
	 * current density, and it can be rasterized again at the size it is drawn at. */
	virtual bool isVectorRaster() const { return false; }

	/** Ratio of raster pixels to source pixels for images decoded at a requested scale. */
	virtual Float getRasterScale() const { return 1.f; }

	virtual Float getMinIntrinsicWidth() { return getPixelsSize().getWidth(); }

	virtual Float getMaxIntrinsicWidth() { return getPixelsSize().getWidth(); }

	virtual void draw() = 0;

	virtual void draw( const Vector2f& position ) = 0;

	virtual void draw( const Vector2f& position, const Sizef& size ) = 0;

	virtual bool isStateful() = 0;

	/** Creates an independently mutable instance backed by the same immutable resource data.
	 * This is an ownership/setup operation; rendering loops must retain and reuse the result. */
	virtual DrawablePtr clone() const;

	void setAlpha( Uint8 alpha );

	const Uint8& getAlpha();

	void setColor( const Color& color );

	const Color& getColor() const;

	void setColorFilter( const Color& color );

	RGB getColorFilter();

	void clearColor();

	void clearColorFilter();

	void resetAlpha();

	Type getDrawableType() const;

	const Vector2f& getPosition() const;

	void setPosition( const Vector2f& position );

	virtual bool isDrawableResource() const;

  protected:
	Type mDrawableType;
	Color mColor;
	Vector2f mPosition;

	Drawable( Type drawableType );

	virtual void onAlphaChange();

	virtual void onColorFilterChange();

	virtual void onPositionChange();
};

}} // namespace EE::Graphics

#endif
