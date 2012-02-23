#include "cisomap.hpp"

namespace EE { namespace Gaming {

cIsoMap::cIsoMap( Window::cWindow * window ) :
	mWindow( window ),
	mOffsetX(0),
	mOffsetY(0),
	mFont(NULL)
{
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}
}

cIsoMap::~cIsoMap() {
}

void cIsoMap::Create( const eeUint& MapTilesX, const eeUint& MapTilesY, const eeUint& NumLayers, const eeUint TilesWidth, const eeUint TilesHeight, const eeColor& AmbientCol ) {
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	mMapWidth = MapTilesX;
	mMapHeight = MapTilesY;
	mMapLayers = NumLayers;

	mTileWidth = TilesWidth;
	mTileHeight = TilesHeight;

	mTileHWidth = mTileWidth * 0.5f;
	mTileHHeight = mTileHeight * 0.5f;
	mTileAltitude = mTileHeight * 0.25f;

	mMapAmbientColor = AmbientCol;

	Map.resize( mMapWidth * mMapHeight );

	CreateBaseVertexBuffer();

	mTilesRange = 10;

	mLight.Create( 250.0f, 0, 0, eeColor(255,255,255), LIGHT_ISOMETRIC );

	mDrawFont = false;
}

void cIsoMap::CreateBaseVertexBuffer() {
	eeUint i;

	for ( eeUint x = 0; x < mMapWidth; x++ ) {
		for ( eeUint y = 0; y < mMapHeight; y++ ) {
			cIsoTile * T = &Tile(x, y);

			if ( T->Layers.size() < 1 )
				T->Layers.resize( mMapLayers );

			T->Q			= TileQBaseCoords( x, y );
			T->Box			= Quad2toAABB( T->Q );
			T->TilePosStr	= toStr( x) + " - " + toStr( y );

			for ( i = 0; i < 4; i++ )
				T->Color[i] = mMapAmbientColor;

			for ( i = 1; i < mMapLayers; i++ )
				Layer( x, y, i, NULL );
		}
	}
}

cIsoTile& cIsoMap::Tile( const eeUint& MapTileX, const eeUint& MapTileY ) {
	eeASSERT ( MapTileX < mMapWidth && MapTileY < mMapHeight );
	return Map[ MapTileX + MapTileY * mMapWidth ];
}

void cIsoMap::Layer( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& LayerNum, cShape * LayerData ) {
	if( LayerNum < mMapLayers )
		Tile(MapTileX, MapTileY).Layers[LayerNum] = LayerData;
}

void cIsoMap::Draw() {
	eeAABB TileAABB;
	cIsoTile * T;

	mScreenAABB = eeAABB( -mOffsetX, -mOffsetY, mWindow->GetWidth() - mOffsetX, mWindow->GetHeight() - mOffsetY ); // Screen AABB to MAP AABB

	mMouseMapPos = eeVector2f( mWindow->GetInput()->MouseX() - mOffsetX, mWindow->GetInput()->MouseY() - mOffsetY );

	mLight.UpdatePos( mMouseMapPos );

	if (mOffsetX > 0)
		mOffsetX = 0;

	if (mOffsetY > 0)
		mOffsetY = 0;

	if ( -mOffsetX > Tile( mMapWidth-1, mMapHeight-1 ).Q.V[1].x - mWindow->GetWidth() )
		mOffsetX = -(Tile( mMapWidth-1, mMapHeight-1 ).Q.V[1].x - mWindow->GetWidth());

	if ( -mOffsetY > Tile( mMapWidth-1, mMapHeight-1 ).Q.V[1].y - mWindow->GetHeight() )
		mOffsetY = -(Tile( mMapWidth-1, mMapHeight-1 ).Q.V[1].y - mWindow->GetHeight());

	Tx = (Int32)( -mOffsetX / (eeFloat)mTileHeight ) - mTilesRange;
	Ty = (Int32)( -mOffsetY / (eeFloat)mTileHeight ) - mTilesRange;

	if (Tx < 0) Tx = 0;
	if (Tx >= (eeInt)mMapWidth) Tx = mMapWidth-1;

	if (Ty < 0) Ty = 0;
	if (Ty >= (eeInt)mMapHeight) Ty = mMapHeight-1;

	Tx2 = ( Tx + mTilesRange + (eeInt)( mWindow->GetWidth() / (eeFloat)mTileHeight ) + mTilesRange );
	Ty2 = ( Ty + mTilesRange + (eeInt)( mWindow->GetHeight() / (eeFloat)mTileHeight) + mTilesRange );

	eeColorA SC(50,50,50,100);

	cGlobalBatchRenderer::instance()->Draw();

	GLi->LoadIdentity();
	GLi->PushMatrix();
	GLi->Translatef( mOffsetX, mOffsetY, 0.0f );

	for ( eeUint L = 0; L < mMapLayers; L++ ) {
		for ( eeInt y = Ty; y < Ty2; y++ ) {
			for ( eeInt x = Tx; x < Tx2; x++ ) {
				T = &Tile( (eeUint)x, (eeUint)y );

				if ( L == 0 ) {
					TileAABB = T->Box;

					if ( Intersect( mScreenAABB, TileAABB )  ) {
						T->Color[0] = T->Color[1] = T->Color[2] = T->Color[3] = mMapAmbientColor;

						if ( Intersect( mLight.GetAABB(), TileAABB ) ) {
							T->Color[0] = mLight.ProcessVertex( T->Q.V[0].x, T->Q.V[0].y, T->Color[0], T->Color[0] ); 	// Left - Top Vertex
							T->Color[1] = mLight.ProcessVertex( T->Q.V[1].x, T->Q.V[1].y, T->Color[1], T->Color[1] ); 	// Left - Bottom Vertex
							T->Color[2] = mLight.ProcessVertex( T->Q.V[2].x, T->Q.V[2].y, T->Color[2], T->Color[2] );	// Right - Bottom Vertex
							T->Color[3] = mLight.ProcessVertex( T->Q.V[3].x, T->Q.V[3].y, T->Color[3], T->Color[3] );	// Right - Top Vertex
						}

						T->Layers[L]->Draw( T->Q, 0.f, 0.f, 0.f, 1.f, T->Color[0], T->Color[1], T->Color[2], T->Color[3] );

						if ( TileAABB.Contains( mMouseMapPos ) ) {
							if ( IntersectQuad2( T->Q, eeQuad2f( mMouseMapPos, mMouseMapPos, mMouseMapPos, mMouseMapPos ) ) ) {
								mMouseTilePos.x = x;
								mMouseTilePos.y = y;
							}
						}
					}
				} else {
					if ( T->Layers[L] != NULL ) {
						cShape * Shape = T->Layers[L];

						if ( T != NULL ) {
							eeVector2f TileCenter( T->Q.V[1].x + (T->Q.V[3].x - T->Q.V[1].x) * 0.5f, T->Q.V[0].y + (T->Q.V[2].y - T->Q.V[0].y) * 0.5f );
							eeVector2f ObjPos( TileCenter.x - Shape->DestWidth() * 0.5f, TileCenter.y - Shape->DestHeight() * 0.80f );

							eeAABB LayerAABB( TileCenter.x - Shape->DestWidth() * 0.5f, TileCenter.y - Shape->DestHeight(), TileCenter.x + Shape->DestWidth() * 0.5f, TileCenter.y  );
							eeAABB ShadowAABB( ObjPos.x, TileCenter.y - Shape->DestHeight(), TileCenter.x + Shape->DestWidth(), TileCenter.y );

							if ( Intersect( mScreenAABB, ShadowAABB ) ) {
								Shape->Draw( mOffsetX + ObjPos.x, mOffsetY + ObjPos.y, 0, 1, SC, SC, SC, SC, ALPHA_NORMAL, RN_ISOMETRIC );
							}

							if ( Intersect( mScreenAABB, LayerAABB )  ) {
								Shape->Draw( mOffsetX + TileCenter.x - (eeFloat)Shape->DestWidth() * 0.5f, mOffsetY + TileCenter.y - (eeFloat)Shape->DestHeight(), 0, 1, eeColorA(T->Color[0]), eeColorA(T->Color[1]), eeColorA(T->Color[2]), eeColorA(T->Color[3]) );
							}
						}
					}
				}
			}
		}

		cGlobalBatchRenderer::instance()->Draw();

		if ( L == 0 ) {
			GLi->PopMatrix();
		}
	}

	if ( mDrawFont ) {
		for ( eeInt y = Ty; y < Ty2; y++ ) {
			for ( eeInt x = Tx; x < Tx2; x++ ) {
				T = &Tile( (eeUint)x, (eeUint)y );
				mFont->Draw( T->TilePosStr, mOffsetX + T->Q.V[1].x + ( T->Q.V[3].x - T->Q.V[1].x ) * 0.5f - T->TilePosStr.size() * mFont->GetFontHeight() * 0.5f, mOffsetY + T->Q.V[0].y + ( T->Q.V[2].y - T->Q.V[0].y ) * 0.5f - mFont->GetFontHeight() * 0.5f );
			}
		}
	}

	mPR.DrawQuad( Tile( mMouseTilePos.x, mMouseTilePos.y ).Q, EE_DRAW_LINE, ALPHA_NORMAL, 1.0f, mOffsetX, mOffsetY );
}

void cIsoMap::Move( const eeFloat& offsetx, const eeFloat& offsety ) {
	mOffsetX += offsetx;
	mOffsetY += offsety;
}

void cIsoMap::SetTileHeight( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& Level, const bool& JointUp ) {
	SetJointHeight( MapTileX, MapTileY, 0, Level, JointUp );
	SetJointHeight( MapTileX, MapTileY, 1, Level, JointUp );
	SetJointHeight( MapTileX, MapTileY, 2, Level, JointUp );
	SetJointHeight( MapTileX, MapTileY, 3, Level, JointUp );
}

void cIsoMap::SetJointHeight( const eeUint& MapTileX, const eeUint& MapTileY, const eeUint& JointNum, const eeUint& Level, const bool& JointUp ) {
	eeFloat AltitudeModif = ( JointUp ) ? -1.f : 1.f;
	eeFloat VertexHeight = (mTileAltitude * AltitudeModif) * (eeFloat)Level;
	eeFloat NJHeight;

	if ( MapTileX >= mMapWidth || MapTileY >= mMapHeight )
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
	if ( ( MapTileX >= 0 && MapTileX < (eeInt)mMapWidth ) && ( MapTileY >= 0 && MapTileY < (eeInt)mMapHeight ) ) {
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
		dX = -mTileHWidth;
		dY = -mTileHHeight;
	} else {
		dX = -mTileHWidth;
		dY = 0;
	}

	pX = MapTileX * mTileHWidth + dX;
	pY = MapTileY * mTileHWidth + dY;

	switch( JointNum ) {
		case 0:	return eeVector2f( pX + mTileHWidth, pY );				// Left Top Vertex
		case 1:	return eeVector2f( pX, pY + mTileHHeight );				// Left Bottom Vertex
		case 2:	return eeVector2f( pX + mTileHWidth, pY + mTileHeight );	// Top Left Vertex
		case 3:	return eeVector2f( pX + mTileWidth, pY + mTileHHeight );	// Top Bottom Vertex
	}

	return eeVector2f(0,0);
}

eeQuad2f cIsoMap::TileQBaseCoords( const eeUint& MapTileX, const eeUint& MapTileY ) {
	return eeQuad2f( TileBaseCoords( MapTileX, MapTileY, 0 ), TileBaseCoords( MapTileX, MapTileY, 1 ), TileBaseCoords( MapTileX, MapTileY, 2 ), TileBaseCoords( MapTileX, MapTileY, 3 ) );
}

void cIsoMap::Reset() {
	CreateBaseVertexBuffer();
}

void cIsoMap::Font( cFont * font ) {
	mFont = font;
}

cFont * cIsoMap::Font() const {
	return mFont;
}

cLight& cIsoMap::BaseLight() {
	return mLight;
}

void cIsoMap::DrawFont( bool draw ) {
	mDrawFont = draw;
}

bool cIsoMap::DrawFont() const {
	return mDrawFont;
}

Uint32 cIsoMap::Width() const {
	return mMapWidth;
}

Uint32 cIsoMap::Height() const {
	return mMapHeight;
}

void cIsoMap::AmbientColor( const eeColor& AC ) {
	mMapAmbientColor = AC;
}

eeColor cIsoMap::AmbientColor() const {
	return mMapAmbientColor;
}

eeVector2i cIsoMap::GetMouseTilePos() const {
	return mMouseTilePos;
}

eeVector2f cIsoMap::GetMouseMapCoords() const {
	return mMouseMapPos;
}

eeAABB cIsoMap::GetScreenMapCoords() const {
	return mScreenAABB;
}

}}
