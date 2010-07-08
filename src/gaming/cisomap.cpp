#include "cisomap.hpp"

namespace EE { namespace Gaming {

cIsoMap::cIsoMap() : OffsetX(0), OffsetY(0) {
	EE = cEngine::instance();
	TF = cTextureFactory::instance();
}

cIsoMap::~cIsoMap() {
	for ( eeUint x = 0; x < MapWidth; x++ )
		for ( eeUint y = 0; y < MapHeight; y++ )
			Tile(x, y).Layers.clear();

	Map.clear();
}

void cIsoMap::Create( const eeUint& MapTilesX, const eeUint& MapTilesY, const eeUint& NumLayers, const eeUint TilesWidth, const eeUint TilesHeight, const eeColor& AmbientCol ) {
	MapWidth = MapTilesX;
	MapHeight = MapTilesY;
	MapLayers = NumLayers;

	TileWidth = TilesWidth;
	TileHeight = TilesHeight;

	TileHWidth = TileWidth * 0.5f;
	TileHHeight = TileHeight * 0.5f;
	TileAltitude = TileHeight * 0.25f;

	MapAmbientColor = AmbientCol;

	Map.resize( MapWidth * MapHeight );
	CreateBaseVertexBuffer();

	TilesRange = 10;

	Light.Create( 250.0f, 0, 0, eeColor(255,255,255), LIGHT_ISOMETRIC );

	DrawFont = false;
}

void cIsoMap::CreateBaseVertexBuffer() {
	eeFloat dX, dY, pX, pY;
	eeUint i;

	for ( eeUint x = 0; x < MapWidth; x++ ) {
		for ( eeUint y = 0; y < MapHeight; y++ ) {
			cIsoTile * T = &Tile(x, y);

			if ( T->Layers.size() < 1 )
				T->Layers.resize( MapLayers );

			if ( (x % 2) == 0) {
				dX = -TileHWidth;
				dY = -TileHHeight;
			} else {
				dX = -TileHWidth;
				dY = 0;
			}
			pX = x * TileHWidth + dX;
			pY = y * TileHWidth + dY;

			T->Q.V[0] = eeVector2f( pX + TileHWidth, pY );				// Left Top Vertex
			T->Q.V[1] = eeVector2f( pX, pY + TileHHeight );				// Left Bottom Vertex
			T->Q.V[2] = eeVector2f( pX + TileHWidth, pY + TileHeight );	// Top Left Vertex
			T->Q.V[3] = eeVector2f( pX + TileWidth, pY + TileHHeight );	// Top Bottom Vertex

			T->TilePosStr = stringTowstring( intToStr( x ) + "-" + intToStr( y ) );
			T->Box = Quad2toAABB( T->Q );

			for ( i = 0; i < 4; i++ )
				T->Color[i] = MapAmbientColor;

			for ( i = 1; i < MapLayers; i++ )
				Layer( x, y, i, 0 );
		}
	}
}

cIsoTile& cIsoMap::Tile( const eeUint& MapTileX, const eeUint& MapTileY ) {
	if ( MapTileX >= MapWidth || MapTileY >= MapHeight )
		return Map[0];

	return Map[ MapTileX + MapTileY * MapWidth ];
}

void cIsoMap::Layer( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& LayerNum, const eeUint& LayerData ) {
	if( LayerNum < MapLayers )
		Tile(MapTileX, MapTileY).Layers[LayerNum] = LayerData;
}

void cIsoMap::Draw() {
	eeAABB TileAABB;

	ScreenAABB = eeAABB( -OffsetX, -OffsetY, EE->GetWidth() - OffsetX, EE->GetHeight() - OffsetY ); // Screen AABB to MAP AABB

	MouseMapPos = eeVector2f( cInput::instance()->MouseX() - OffsetX, cInput::instance()->MouseY() - OffsetY );

	Light.UpdatePos( MouseMapPos );

	if (OffsetX > 0)
		OffsetX = 0;

	if (OffsetY > 0)
		OffsetY = 0;

	if ( -OffsetX > Tile( MapWidth-1, MapHeight-1 ).Q.V[1].x - EE->GetWidth() )
		OffsetX = -(Tile( MapWidth-1, MapHeight-1 ).Q.V[1].x - EE->GetWidth());

	if ( -OffsetY > Tile( MapWidth-1, MapHeight-1 ).Q.V[1].y - EE->GetHeight() )
		OffsetY = -(Tile( MapWidth-1, MapHeight-1 ).Q.V[1].y - EE->GetHeight());

	Tx = (Int32)( -OffsetX / (eeFloat)TileHeight ) - TilesRange;
	Ty = (Int32)( -OffsetY / (eeFloat)TileHeight ) - TilesRange;

	if (Tx < 0) Tx = 0;
	if (Tx >= (eeInt)MapWidth) Tx = MapWidth-1;

	if (Ty < 0) Ty = 0;
	if (Ty >= (eeInt)MapHeight) Ty = MapHeight-1;

	Tx2 = ( Tx + TilesRange + (eeInt)( EE->GetWidth() / (eeFloat)TileHeight ) + TilesRange );
	Ty2 = ( Ty + TilesRange + (eeInt)(EE->GetHeight() / (eeFloat)TileHeight) + TilesRange );

	eeFloat TexCoords[8] = { 0, 0, 0, 1, 1, 1, 1, 0 };
	eeColorA SC(50,50,50,100);

	glLoadIdentity();
	glPushMatrix();
	glTranslatef( OffsetX, OffsetY, 0.0f );

	for ( eeUint L = 0; L < MapLayers; L++ ) {
		for ( eeInt y = Ty; y < Ty2; y++ ) {
			for ( eeInt x = Tx; x < Tx2; x++ ) {
				cIsoTile* T = &Tile( (eeUint)x, (eeUint)y );

				if ( L == 0 ) {
					TileAABB = T->Box;

					if ( Intersect( ScreenAABB, TileAABB )  ) {
						T->Color[0] = T->Color[1] = T->Color[2] = T->Color[3] = MapAmbientColor;

						if ( Intersect( Light.GetAABB(), TileAABB ) ) {
							T->Color[0] = Light.ProcessVertex( T->Q.V[0].x, T->Q.V[0].y, T->Color[0], T->Color[0] ); 	// Left - Top Vertex
							T->Color[1] = Light.ProcessVertex( T->Q.V[1].x, T->Q.V[1].y, T->Color[1], T->Color[1] ); 	// Left - Bottom Vertex
							T->Color[2] = Light.ProcessVertex( T->Q.V[2].x, T->Q.V[2].y, T->Color[2], T->Color[2] );	// Right - Bottom Vertex
							T->Color[3] = Light.ProcessVertex( T->Q.V[3].x, T->Q.V[3].y, T->Color[3], T->Color[3] );	// Right - Top Vertex
						}

						TF->Bind( T->Layers[L] );
						TF->SetBlendFunc(ALPHA_NORMAL);

						glColorPointer( 3, GL_UNSIGNED_BYTE, 0, reinterpret_cast<char*>( T ) );
						glVertexPointer( 2, GL_FLOAT, 0, reinterpret_cast<char*>( T ) + sizeof(eeColor) * 4 );
						glTexCoordPointer( 2, GL_FLOAT, 0, reinterpret_cast<char*>( &TexCoords[0] ) );

						glDrawArrays( GL_QUADS, 0, 4 );

						if ( TileAABB.Contains( MouseMapPos ) ) {
							if ( IntersectQuad2( T->Q, eeQuad2f( MouseMapPos, MouseMapPos, MouseMapPos, MouseMapPos ) ) ) {
								MouseTilePos.x = x;
								MouseTilePos.y = y;
							}
						}
					}
				} else {
					if ( T->Layers[L] > 0 ) {
						cTexture * Tex = TF->GetTexture( T->Layers[L] );

						if ( T != NULL ) {
							eeVector2f TileCenter( T->Q.V[1].x + (T->Q.V[3].x - T->Q.V[1].x) * 0.5f, T->Q.V[0].y + (T->Q.V[2].y - T->Q.V[0].y) * 0.5f );
							eeVector2f ObjPos( TileCenter.x - Tex->ImgWidth() * 0.5f, TileCenter.y - Tex->ImgHeight() * 0.80f );

							eeAABB LayerAABB( TileCenter.x - Tex->Width() * 0.5f, TileCenter.y - Tex->Height(), TileCenter.x + Tex->Width() * 0.5f, TileCenter.y  );
							eeAABB ShadowAABB( ObjPos.x, TileCenter.y - Tex->ImgHeight(), TileCenter.x + Tex->ImgWidth(), TileCenter.y );

							if ( Intersect( ScreenAABB, ShadowAABB ) ) {
								//AABB ShadowAABB_RECT( OffsetX + ObjPos.x, OffsetY + TileCenter.y - Tex->Height(), OffsetX + TileCenter.x + Tex->Width(), OffsetY + TileCenter.y );
								//PR.DrawRectangle( ShadowAABB_RECT, 0, 1, DRAW_LINE );
								Tex->DrawEx( OffsetX + ObjPos.x, OffsetY + ObjPos.y, (eeFloat)Tex->ImgWidth(), (eeFloat)Tex->ImgHeight(), 0, 1, SC, SC, SC, SC, ALPHA_NORMAL, RN_ISOMETRIC );
							}

							if ( Intersect( ScreenAABB, LayerAABB )  ) {
								Tex->DrawEx( OffsetX + TileCenter.x - (eeFloat)Tex->ImgWidth() * 0.5f, OffsetY + TileCenter.y - (eeFloat)Tex->ImgHeight(), (eeFloat)Tex->ImgWidth(), (eeFloat)Tex->ImgHeight(), 0, 1, eeColorA(T->Color[0]), eeColorA(T->Color[1]), eeColorA(T->Color[2]), eeColorA(T->Color[3]) );
							}
						}
					}
				}
			}
		}

		if ( L == 0 )
			glPopMatrix();
	}

	if ( DrawFont ) {
		for ( eeInt y = Ty; y < Ty2; y++ ) {
			for ( eeInt x = Tx; x < Tx2; x++ ) {
				cIsoTile* T = &Tile( (eeUint)x, (eeUint)y );
				myFont->Draw( T->TilePosStr, OffsetX + T->Q.V[1].x + ( T->Q.V[3].x - T->Q.V[1].x ) * 0.5f - T->TilePosStr.size() * myFont->GetFontSize() * 0.5f, OffsetY + T->Q.V[0].y + ( T->Q.V[2].y - T->Q.V[0].y ) * 0.5f - myFont->GetFontHeight() * 0.5f );
			}
		}
	}

	PR.DrawQuad( Tile( MouseTilePos.x, MouseTilePos.y ).Q, DRAW_LINE, ALPHA_NORMAL, 1.0f, OffsetX, OffsetY );
}

void cIsoMap::Move( const eeFloat& offsetx, const eeFloat& offsety ) {
	OffsetX += offsetx;
	OffsetY += offsety;
}

void cIsoMap::SetTileHeight( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& Level, const bool& JointUp ) {
	SetJointHeight( MapTileX, MapTileY, 0, Level, JointUp );
	SetJointHeight( MapTileX, MapTileY, 1, Level, JointUp );
	SetJointHeight( MapTileX, MapTileY, 2, Level, JointUp );
	SetJointHeight( MapTileX, MapTileY, 3, Level, JointUp );
}

void cIsoMap::SetJointHeight( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& JointNum, const eeUint& Level, const bool& JointUp ) {
	eeFloat AltitudeModif = ( JointUp ) ? -1.f : 1.f;
	eeFloat VertexHeight = (TileAltitude * AltitudeModif) * (eeFloat)Level;
	eeFloat NJHeight;

	if ( MapTileX >= MapWidth || MapTileY >= MapHeight )
		return;

	cIsoTile * T = &Tile( MapTileX, MapTileY );

	T->Q.V[JointNum].y += VertexHeight;

	NJHeight = T->Q.V[JointNum].y;

	switch (JointNum) {
		case 0:
			if ( (MapTileX % 2) == 0) {
				VertexChangeHeight( MapTileX - 1, MapTileY - 1, 3, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX	, MapTileY - 1, 2, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX + 1, MapTileY - 1, 1, VertexHeight, NJHeight, JointUp );
			} else {
				VertexChangeHeight( MapTileX + 1, MapTileY	  , 1, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX	, MapTileY - 1, 2, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX - 1, MapTileY	  , 3, VertexHeight, NJHeight, JointUp );
			}
			break;
		case 1:
			if ( (MapTileX % 2) == 0) {
				VertexChangeHeight( MapTileX - 1, MapTileY - 1, 2, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX - 2, MapTileY	  , 3, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX - 1, MapTileY	  , 0, VertexHeight, NJHeight, JointUp );
			} else {
				VertexChangeHeight( MapTileX - 1, MapTileY + 1, 0, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX - 1, MapTileY	  , 2, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX - 2, MapTileY	  , 3, VertexHeight, NJHeight, JointUp );
			}
			break;
		case 2:
			if ( (MapTileX % 2) == 0) {
				VertexChangeHeight( MapTileX + 1, MapTileY	  , 1, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX - 1, MapTileY	  , 3, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX	, MapTileY + 1, 0, VertexHeight, NJHeight, JointUp );
			} else {
				VertexChangeHeight( MapTileX	, MapTileY + 1, 0, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX + 1, MapTileY + 1, 1, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX - 1, MapTileY + 1, 3, VertexHeight, NJHeight, JointUp );
			}
			break;
		case 3:
			if ( (MapTileX % 2) == 0) {
				VertexChangeHeight( MapTileX + 1, MapTileY - 1, 2, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX + 1, MapTileY	  , 0, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX + 2, MapTileY	  , 1, VertexHeight, NJHeight, JointUp );
			} else {
				VertexChangeHeight( MapTileX + 1, MapTileY + 1, 0, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX + 2, MapTileY	  , 1, VertexHeight, NJHeight, JointUp );
				VertexChangeHeight( MapTileX + 1, MapTileY	  , 2, VertexHeight, NJHeight, JointUp );
			}
			break;
	}
}

void cIsoMap::VertexChangeHeight( const eeInt& MapTileX, const eeInt& MapTileY, const eeUint& JointNum, const eeFloat& Height, const eeFloat& NewJointHeight, const bool& JointUp ) {
	if ( ( MapTileX >= 0 && MapTileX < (eeInt)MapWidth ) && ( MapTileY >= 0 && MapTileY < (eeInt)MapHeight ) ) {
		cIsoTile * T = &Tile( MapTileX, MapTileY );

		if ( ( JointUp && T->Q.V[JointNum].y >= NewJointHeight ) || ( !JointUp && T->Q.V[JointNum].y <= NewJointHeight ) ) {
			T->Q.V[JointNum].y += Height;
			T->Box = Quad2toAABB( T->Q );
		}
	}
}

eeVector2f cIsoMap::TileBaseCoords( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& JointNum) {
	eeFloat dX, dY, pX, pY;

	if ( (MapTileX % 2) == 0) {
		dX = -TileHWidth;
		dY = -TileHHeight;
	} else {
		dX = -TileHWidth;
		dY = 0;
	}
	pX = MapTileX * TileHWidth + dX;
	pY = MapTileY * TileHWidth + dY;

	switch( JointNum ) {
		case 0:
			return eeVector2f( pX + TileHWidth, pY );					// Left Top Vertex
			break;
		case 1:
			return eeVector2f( pX, pY + TileHHeight );				// Left Bottom Vertex
			break;
		case 2:
			return eeVector2f( pX + TileHWidth, pY + TileHeight );	// Top Left Vertex
			break;
		case 3:
			return eeVector2f( pX + TileWidth, pY + TileHHeight );	// Top Bottom Vertex
			break;
	}

	return eeVector2f(0,0);
}

eeQuad2f cIsoMap::TileQBaseCoords( const eeUint& MapTileX, const eeUint& MapTileY ) {
	return eeQuad2f( TileBaseCoords( MapTileX, MapTileY, 0 ), TileBaseCoords( MapTileX, MapTileY, 1 ), TileBaseCoords( MapTileX, MapTileY, 2 ), TileBaseCoords( MapTileX, MapTileY, 3 ) );
}

void cIsoMap::Reset() {
	CreateBaseVertexBuffer();
}

}}
