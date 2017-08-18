#ifndef EE_GRAPHICS_NINEPATCH_HPP 
#define EE_GRAPHICS_NINEPATCH_HPP

#include <eepp/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace Graphics {

class EE_API NinePatch : public Drawable {
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

		NinePatch( const Uint32& TexId, int left, int top, int right, int bottom );
		
		NinePatch( SubTexture * subTexture, int left, int top, int right, int bottom );

		~NinePatch();
		
		virtual Sizef getSize();

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );
	protected:
		Drawable * 	mDrawable[ SideCount ];
		Rect mRect;
		Sizei mSize;

		void createFromTexture( const Uint32& TexId, int left, int top, int right, int bottom );
};

}}

#endif
