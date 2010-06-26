#ifndef EE_GAMINGCISOMAP_H
#define EE_GAMINGCISOMAP_H

#include "base.hpp"
#include "clight.hpp"

namespace EE { namespace Gaming {

class EE_API cIsoTile {
	public:
		eeColor Color[4];	//! Color of every vertex stored
		eeQuad2f Q; 			//! Vertex Buffer Data
		std::vector<eeUint> Layers;
		std::wstring TilePosStr;
		eeAABB Box;
};

class EE_API cIsoMap{
	public:
		cIsoMap();
		~cIsoMap();

		void Create( const eeUint& MapTilesX, const eeUint& MapTilesY, const eeUint& NumLayers = 1, const eeUint TilesWidth = 64, const eeUint TilesHeight = 32, const eeColor& AmbientCol = eeColor(255,255,255) );
		cIsoTile& Tile( const eeUint& MapTileX, const eeUint& MapTileY );
		void Layer( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& LayerNum, const eeUint& LayerData );
		void Draw();
		void Move( const eeFloat& offsetx, const eeFloat& offsety );

		Uint32 Width() const { return MapWidth; }
		Uint32 Height() const { return MapHeight; }

		void AmbientColor( const eeColor& AC ) { MapAmbientColor = AC; }
		eeColor AmbientColor() const { return MapAmbientColor; }

		void SetJointHeight( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& JointNum, const eeUint& Level = 1, const bool& JointUp = true );
		void SetTileHeight( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& Level = 1, const bool& JointUp = true );

		eeVector2i GetMouseTilePos() const { return MouseTilePos; }
		eeVector2f GetMouseMapCoords() const { return MouseMapPos; }
		eeAABB GetScreenMapCoords() const { return ScreenAABB; }

		eeVector2f TileBaseCoords( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& JointNum);
		eeQuad2f TileQBaseCoords( const eeUint& MapTileX, const eeUint& MapTileY );

		cFont* myFont;
		bool DrawFont;
		cLight Light;

		void Reset();
	protected:
		std::vector<cIsoTile> Map;

		// Map Ambient Color
		eeColor MapAmbientColor;

		// Map Basic Data
		eeUint MapWidth, MapHeight, MapLayers, TileWidth, TileHeight, TilesRange;
		eeFloat TileHWidth, TileHHeight, TileAltitude;

		eeVector2i MouseTilePos;
		eeVector2f MouseMapPos;
		eeAABB ScreenAABB;

		eeInt Tx, Ty, Tx2, Ty2;

		// Camera on Map
		eeFloat OffsetX, OffsetY;

		// Fast access to classes
		cEngine* EE;
		cTextureFactory* TF;
		cPrimitives PR;

		void CreateBaseVertexBuffer();
		void VertexChangeHeight( const eeInt& MapTileX, const eeInt& MapTileY, const eeUint& JointNum, const eeFloat& Height, const eeFloat& NewJointHeight, const bool& JointUp );
};

}}

#endif
