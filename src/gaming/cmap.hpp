#ifndef EE_GAMINGCTILEMAP_HPP
#define EE_GAMINGCTILEMAP_HPP

#include "base.hpp"

#include "cgameobject.hpp"
#include "clight.hpp"
#include "clayer.hpp"

#include "../window/cinput.hpp"
#include "../window/cengine.hpp"
#include "../window/cwindow.hpp"
using namespace EE::Window;

namespace EE { namespace Gaming {

class cMap {
	public:
		cMap();

		virtual ~cMap();

		virtual void Create( eeSize Size, Uint32 MaxLayers, eeSize TileSize, Uint32 Flags = 0, eeSize viewSize = eeSize( 800, 600 ), cWindow * Window = NULL );

		virtual cLayer * AddLayer( Uint32 Type, Uint32 flags, std::string name );

		virtual cLayer * GetLayer( Uint32 index );

		virtual cLayer * GetLayerByHash( Uint32 hash );

		virtual cLayer * GetLayer( const std::string& name );

		virtual void Load( const std::string& path );

		virtual void Save( const std::string& path );

		virtual void Draw();

		virtual void Update();

		const eeSize& TileSize() const;

		const eeSize& Size() const;

		const Uint32& LayerCount() const;

		const Uint32& MaxLayers() const;

		const eeSize& ViewSize() const;

		void ViewSize( const eeSize& viewSize );

		const eeVector2f& Offset() const;

		void Offset( const eeVector2f& offset );

		const eeVector2i& Position() const;

		void Position( const eeVector2i& position );

		void Move( const eeVector2f& offset );

		void Move( const eeFloat& offsetx, const eeFloat& offsety );

		const eeVector2i& StartTile() const;

		const eeVector2i& EndTile() const;

		const Uint32& Flags() const;

		Uint32 ClampBorders() const;

		Uint32 ClipedArea() const;

		void DrawGrid( const bool& draw );

		Uint32 DrawGrid() const;

		void Reset();

		const eeVector2u& GetMouseTilePos() const;
	protected:
		cWindow *		mWindow;
		cLayer**		mLayers;
		Uint32			mFlags;
		Uint32			mMaxLayers;
		Uint32			mLayerCount;
		eeSize			mSize;
		eeSize			mTileSize;
		eeSize			mViewSize;
		eeVector2f		mOffset;
		eeVector2i		mScreenPos;
		eeVector2i		mStartTile;
		eeVector2i		mEndTile;
		eeVector2i		mMouseOverTile;
		eeVector2u		mMouseOverTileFinal;

		cGameObject *	CreateGameObject( const Uint32& Type, const Uint32& Flags );

		eeVector2f		FixOffset();

		void			CalcTilesClip();

		void			Clamp();

		void			GetMouseOverTile();

		void			GridDraw();

		void			DeleteLayers();
};

}}

#endif
