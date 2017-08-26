#ifndef EE_GRAPHICS_NINEPATCH_HPP 
#define EE_GRAPHICS_NINEPATCH_HPP

#include <eepp/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/subtexture.hpp>

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

		NinePatch( const Uint32& TexId, int left, int top, int right, int bottom, const Float& pixelDensity = 1, const std::string& name = "" );
		
		NinePatch( SubTexture * subTexture, int left, int top, int right, int bottom, const std::string& name = "" );

		~NinePatch();
		
		virtual Sizef getSize();

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		SubTexture *	getSubTexture( const int& side );
	protected:
		SubTexture * 	mDrawable[ SideCount ];
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

}}

#endif
