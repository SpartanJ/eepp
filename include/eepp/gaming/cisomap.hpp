#ifndef EE_GAMINGCISOMAP_H
#define EE_GAMINGCISOMAP_H

#include <eepp/gaming/base.hpp>

#include <eepp/window/cinput.hpp>
#include <eepp/window/cwindow.hpp>
#include <eepp/window/cengine.hpp>
using namespace EE::Window;

#include <eepp/graphics/ctexture.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cshape.hpp>
#include <eepp/graphics/cfont.hpp>
#include <eepp/graphics/cprimitives.hpp>
#include <eepp/graphics/glhelper.hpp>
using namespace EE::Graphics;

#include <eepp/gaming/clight.hpp>

namespace EE { namespace Gaming {

class EE_API cIsoTile {
	public:
		eeColor Color[4];	//! Color of every vertex stored
		eeQuad2f Q; 			//! Vertex Buffer Data
		std::vector<cShape*> Layers;
		std::string TilePosStr;
		eeAABB Box;
};

class EE_API cIsoMap {
	public:
		cIsoMap( Window::cWindow * window = NULL );

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

		Window::cWindow *	mWindow;
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
