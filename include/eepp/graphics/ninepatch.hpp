#ifndef EE_GRAPHICS_NINEPATCH_HPP
#define EE_GRAPHICS_NINEPATCH_HPP

#include <eepp/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/textureregion.hpp>

namespace EE { namespace Graphics {

class EE_API NinePatch : public DrawableResource {
  public:
	enum NinePatchSides {
		Left = 0,
		Right,
		Down,
		Up,
		UpLeft,
		UpRight,
		DownLeft,
		DownRight,
		Center,
		SideCount
	};

	static NinePatch* New( const Uint32& TexId, int left, int top, int right, int bottom,
						   const Float& pixelDensity = 1, const std::string& name = "" );

	static NinePatch* New( TextureRegion* textureRegion, int left, int top, int right, int bottom,
						   const std::string& name = "" );

	NinePatch( const Uint32& TexId, int left, int top, int right, int bottom,
			   const Float& pixelDensity = 1, const std::string& name = "" );

	NinePatch( TextureRegion* textureRegion, int left, int top, int right, int bottom,
			   const std::string& name = "" );

	~NinePatch();

	virtual Sizef getSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	TextureRegion* getTextureRegion( const int& side );

  protected:
	TextureRegion* mDrawable[SideCount];
	Rect mRect;
	Rectf mRectf;
	Sizei mSize;
	Sizef mDestSize;
	Float mPixelDensity;

	void createFromTexture( const Uint32& TexId, int left, int top, int right, int bottom );

	virtual void onAlphaChange();

	virtual void onColorFilterChange();

	void updatePosition();

	void updateSize();
};

}} // namespace EE::Graphics

#endif
