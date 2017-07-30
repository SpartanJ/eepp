#ifndef CGAMEOBJECTSUBTEXTUREEX_HPP
#define CGAMEOBJECTSUBTEXTUREEX_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/gameobjectsubtexture.hpp>

namespace EE { namespace Maps {

class EE_API GameObjectSubTextureEx : public GameObjectSubTexture {
	public:
		GameObjectSubTextureEx( const Uint32& Flags, MapLayer * Layer, Graphics::SubTexture * SubTexture = NULL, const Vector2f& Pos = Vector2f(), EE_BLEND_MODE Blend = ALPHA_NORMAL, EE_RENDER_MODE Render = RN_NORMAL, Float Angle = 0.f, Vector2f Scale = Vector2f::One, Color color = Color() );

		virtual ~GameObjectSubTextureEx();

		virtual void draw();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type );

		virtual void setFlag( const Uint32& Flag );
	protected:
		EE_BLEND_MODE		mBlend;
		EE_RENDER_MODE		mRender;
		Float				mAngle;
		Vector2f			mScale;
		Color				mColor;
		Color *			mVertexColors;
};

}}

#endif
