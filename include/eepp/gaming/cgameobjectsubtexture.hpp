#ifndef EE_GAMINGCGAMEOBJECTSUBTEXTURE_HPP
#define EE_GAMINGCGAMEOBJECTSUBTEXTURE_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/cgameobject.hpp>

#include <eepp/graphics/csubtexture.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class EE_API cGameObjectSubTexture : public cGameObject {
	public:
		cGameObjectSubTexture( const Uint32& Flags, cLayer * Layer, cSubTexture * SubTexture = NULL, const Vector2f& Pos = Vector2f() );

		virtual ~cGameObjectSubTexture();

		virtual void Draw();

		virtual Vector2f Pos() const;

		virtual void Pos( Vector2f pos );

		virtual Vector2i TilePos() const;

		virtual void TilePos( Vector2i pos );

		virtual Sizei Size();

		cSubTexture * SubTexture() const;

		void SubTexture( cSubTexture * subTexture );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );
	protected:
		cSubTexture *	mSubTexture;
		Vector2f	mPos;
		Vector2i	mTilePos;
};

}}

#endif
