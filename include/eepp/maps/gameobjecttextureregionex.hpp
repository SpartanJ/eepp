#ifndef EE_GAMEOBJECTTEXTUREREGIONEX_HPP
#define EE_GAMEOBJECTTEXTUREREGIONEX_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/gameobjecttextureregion.hpp>

namespace EE { namespace Maps {

class EE_API GameObjectTextureRegionEx : public GameObjectTextureRegion {
	public:
		GameObjectTextureRegionEx( const Uint32& Flags, MapLayer * Layer, Graphics::TextureRegion * TextureRegion = NULL, const Vector2f& Pos = Vector2f(), BlendMode Blend = BlendAlpha, RenderMode Render = RENDER_NORMAL, Float Angle = 0.f, Vector2f Scale = Vector2f::One, Color color = Color::White );

		virtual ~GameObjectTextureRegionEx();

		virtual void draw();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type );

		virtual void setFlag( const Uint32& Flag );
	protected:
		BlendMode		mBlend;
		RenderMode		mRender;
		Float				mAngle;
		Vector2f			mScale;
		Color				mColor;
		Color *			mVertexColors;
};

}}

#endif
