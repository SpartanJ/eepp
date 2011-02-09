#ifndef mEE_GAMINGCISOMAP_H
#define mEE_GAMINGCISOMAP_H

#include "base.hpp"

#include "../window/cinput.hpp"
#include "../window/cwindow.hpp"
#include "../window/cengine.hpp"
using namespace EE::Window;

#include "../graphics/ctexture.hpp"
#include "../graphics/ctexturefactory.hpp"
#include "../graphics/cshape.hpp"
#include "../graphics/cfont.hpp"
#include "../graphics/cprimitives.hpp"
#include "../graphics/glhelper.hpp"
using namespace EE::Graphics;

#include "clight.hpp"

namespace EE { namespace Gaming {

class EE_API cIsoTile {
	public:
		eeColor Color[4];	//! Color of every vertex stored
		eeQuad2f Q; 			//! Vertex Buffer Data
		std::vector<cShape*> Layers;
		std::wstring TilePosStr;
		eeAABB Box;
};

class EE_API cIsoMap {
	public:
		cIsoMap( cWindow * window = NULL );

		~cIsoMap();

		void Create( const eeUint& MapTilesX, const eeUint& MapTilesY, const eeUint& NumLayers = 1, const eeUint TilesWidth = 64, const eeUint TilesHeight = 32, const eeColor& AmbientCol = eeColor(255,255,255) );

		cIsoTile& Tile( const eeUint& MapTileX, const eeUint& MapTileY );

		void Layer( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& LayerNum, cShape * LayerData );

		void Draw();

		void Move( const eeFloat& offsetx, const eeFloat& offsety );

		void SetJointHeight( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& JointNum, const eeUint& Level = 1, const bool& JointUp = true );

		void SetTileHeight( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& Level = 1, const bool& JointUp = true );

		eeVector2f TileBaseCoords( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& JointNum);

		eeQuad2f TileQBaseCoords( const eeUint& MapTileX, const eeUint& MapTileY );

		Uint32 Width() const;

		Uint32 Height() const;

		void AmbientColor( const eeColor& AC );

		eeColor AmbientColor() const;

		eeVector2i GetMouseTilePos() const;

		eeVector2f GetMouseMapCoords() const;

		eeAABB GetScreenMapCoords() const;

		void Reset();

		void Font( cFont * font );

		cFont * Font() const;

		cLight& BaseLight();

		void DrawFont( bool draw );

		bool DrawFont() const;
	protected:
		std::vector<cIsoTile> Map;

		cWindow *	mWindow;
		eeColor		mMapAmbientColor;
		eeUint		mMapWidth;
		eeUint		mMapHeight;
		eeUint		mMapLayers;
		eeUint		mTileWidth;
		eeUint		mTileHeight;
		eeUint		mTilesRange;
		eeFloat		mTileHWidth;
		eeFloat		mTileHHeight;
		eeFloat		mTileAltitude;
		eeVector2i	mMouseTilePos;
		eeVector2f	mMouseMapPos;
		eeAABB		mScreenAABB;
		eeInt		Tx;
		eeInt		Ty;
		eeInt		Tx2;
		eeInt		Ty2;
		eeFloat		mOffsetX;
		eeFloat		mOffsetY;

		// Fast access to classes
		cPrimitives			mPR;

		cFont * mFont;

		cLight mLight;

		bool mDrawFont;

		void CreateBaseVertexBuffer();

		void VertexChangeHeight( const eeInt& MapTileX, const eeInt& MapTileY, const eeUint& JointNum, const eeFloat& Height, const eeFloat& NewJointHeight, const bool& JointUp );
};

}}

#endif
