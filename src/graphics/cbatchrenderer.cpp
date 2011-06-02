#include "cbatchrenderer.hpp"
#include "ctexturefactory.hpp"
#include "cglobalbatchrenderer.hpp"

namespace EE { namespace Graphics {

cBatchRenderer::cBatchRenderer() :
	mVertex( NULL ),
	mNumVertex(0),
	mTexture(NULL),
	mBlend(ALPHA_NORMAL),
	mCurrentMode(DM_QUADS),
	mRotation(0.0f),
	mScale(1.0f),
	mPosition(0.0f, 0.0f),
	mCenter(0.0f, 0.0f),
	mForceRendering(false),
	mForceBlendMode(true)
{
	AllocVertexs( 1024 );
	Init();
}

cBatchRenderer::cBatchRenderer( const eeUint& Prealloc ) :
	mNumVertex(0),
	mTexture(NULL),
	mBlend(ALPHA_NORMAL),
	mCurrentMode(DM_QUADS),
	mRotation(0.0f),
	mScale(1.0f),
	mPosition(0.0f, 0.0f),
	mCenter(0.0f, 0.0f),
	mForceRendering(false),
	mForceBlendMode(true)
{
	AllocVertexs( Prealloc );
	Init();
}

cBatchRenderer::~cBatchRenderer() {
	eeSAFE_DELETE_ARRAY( mVertex );
}

void cBatchRenderer::Init() {
	QuadsBegin();
}

void cBatchRenderer::AllocVertexs( const eeUint& size ) {
	eeSAFE_DELETE_ARRAY( mVertex );
	mVertex		= eeNewArray( eeVertex, size );
	mVertexSize = size;
	mNumVertex	= 0;
}

void cBatchRenderer::DrawOpt() {
	if ( mForceRendering )
		Flush();
}

void cBatchRenderer::Draw() {
	Flush();
}

void cBatchRenderer::SetTexture( const cTexture * Tex ) {
	if ( mTexture != Tex )
		Flush();

	mTexture = Tex;
}

void cBatchRenderer::SetPreBlendFunc( const EE_PRE_BLEND_FUNC& Blend ) {
	if ( Blend != mBlend )
		Flush();

	mBlend = Blend;
}

void cBatchRenderer::AddVertexs( const eeUint& num ) {
	mNumVertex += num;

	if ( ( mNumVertex + num ) >= mVertexSize )
		Flush();
}

void cBatchRenderer::SetBlendMode( EE_DRAW_MODE Mode, const bool& Force ) {
	if ( Force && mCurrentMode != Mode ) {
		Flush();
		mCurrentMode = Mode;
	}
}

void cBatchRenderer::Flush() {	
	if ( mNumVertex == 0 )
		return;

	if ( cGlobalBatchRenderer::instance() != this )
		cGlobalBatchRenderer::instance()->Draw();

	Uint32 NumVertex = mNumVertex;
	mNumVertex = 0;

	bool CreateMatrix = ( mRotation || mScale != 1.0f || mPosition.x || mPosition.y );

	cTextureFactory::instance()->SetPreBlendFunc( mBlend );

	if ( mCurrentMode == DM_POINTS && NULL != mTexture ) {
		GLi->Enable( GL_POINT_SPRITE );
		GLi->PointSize( (GLfloat)mTexture->Width() );
	}

	if ( CreateMatrix ) {
		GLi->LoadIdentity();
		GLi->PushMatrix();

		GLi->Translatef( mPosition.x + mCenter.x, mPosition.y + mCenter.y, 0.0f);
		GLi->Rotatef( mRotation, 0.0f, 0.0f, 1.0f );
		GLi->Scalef( mScale, mScale, 1.0f );
		GLi->Translatef( -mCenter.x, -mCenter.y, 0.0f);
	}

	Uint32 alloc = sizeof(eeVertex) * NumVertex;

	GLi->VertexPointer	( 2, GL_FLOAT			, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] )												, alloc	);
	GLi->ColorPointer	( 4, GL_UNSIGNED_BYTE	, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] ) + sizeof(eeVector2f) + sizeof(eeTexCoord)	, alloc	);

	if ( NULL != mTexture ) {
		cTextureFactory::instance()->Bind( mTexture );
		GLi->TexCoordPointer( 2, GL_FLOAT		, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] ) + sizeof(eeVector2f)						, alloc	);
	} else {
		GLi->Disable( GL_TEXTURE_2D );
		GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	#ifdef EE_GLES
	if ( DM_QUADS == mCurrentMode ) {
		GLi->DrawArrays( DM_TRIANGLES, 0, NumVertex );
	} else if ( DM_POLYGON == mCurrentMode ) {
		GLi->DrawArrays( DM_TRIANGLE_FAN, 0, NumVertex );
	} else
	#endif
	{
		GLi->DrawArrays( mCurrentMode, 0, NumVertex );
	}

	if ( CreateMatrix ) {
		GLi->PopMatrix();
	}

	if ( mCurrentMode == DM_POINTS && mTexture > 0 ) {
		GLi->Disable( GL_POINT_SPRITE );
	}

	if ( mTexture == 0 ) {
		GLi->Enable( GL_TEXTURE_2D );
		GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
	}
}

void cBatchRenderer::BatchQuad( const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeFloat& angle ) {
#ifndef EE_GLES
	if ( mNumVertex + 3 >= mVertexSize )
			return;
#else
	if ( mNumVertex + 5 >= mVertexSize )
			return;
#endif

	eeVector2f center;

	if ( angle ) {
		center.x = x + width  * 0.5f;
		center.y = y + height * 0.5f;
	}

	SetBlendMode( DM_QUADS, mForceBlendMode );

#ifndef EE_GLES
	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y + height;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = x + width;
	mTVertex->pos.y = y + height;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 3 ];
	mTVertex->pos.x = x + width;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
	Rotate(center, &mTVertex->pos, angle);
#else
	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y + height;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = x + width;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 3 ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y + height;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 4 ];
	mTVertex->pos.x = x + width;
	mTVertex->pos.y = y + height;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 5 ];
	mTVertex->pos.x = x + width;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
	Rotate(center, &mTVertex->pos, angle);
#endif

	AddVertexs( EE_QUAD_VERTEX );
}

void cBatchRenderer::BatchQuadEx( const eeFloat& x, const eeFloat& y, const eeFloat& width, const eeFloat& height, const eeFloat& angle, const eeFloat& scale, const bool& scalefromcenter ) {
#ifndef EE_GLES
	if ( mNumVertex + 3 >= mVertexSize )
		return;
#else
	if ( mNumVertex + 5 >= mVertexSize )
		return;
#endif

	eeVector2f center;
	eeFloat mx = x;
	eeFloat my = y;
	eeFloat mwidth = width;
	eeFloat mheight = height;

	center.x = width  * 0.5f;
	center.y = height * 0.5f;

	if ( scale != 1.0f ) {
		if ( scalefromcenter ) {
			mx = mx + center.x - center.x * scale;
			my = my + center.y - center.y * scale;
		}
		mwidth *= scale;
		mheight *= scale;
	}

	center.x += x;
	center.y += y;

	SetBlendMode( DM_QUADS, mForceBlendMode );

#ifndef EE_GLES
	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = mx;
	mTVertex->pos.y = my;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = mx;
	mTVertex->pos.y = my + mheight;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = mx + mwidth;
	mTVertex->pos.y = my + mheight;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 3 ];
	mTVertex->pos.x = mx + mwidth;
	mTVertex->pos.y = my;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
	Rotate(center, &mTVertex->pos, angle);
#else
	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = mx;
	mTVertex->pos.y = my + mheight;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = mx;
	mTVertex->pos.y = my;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = mx + mwidth;
	mTVertex->pos.y = my;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 3 ];
	mTVertex->pos.x = mx;
	mTVertex->pos.y = my + mheight;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 4 ];
	mTVertex->pos.x = mx + mwidth;
	mTVertex->pos.y = my + mheight;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];
	Rotate(center, &mTVertex->pos, angle);

	mTVertex 		= &mVertex[ mNumVertex + 5 ];
	mTVertex->pos.x = mx + mwidth;
	mTVertex->pos.y = my;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
	Rotate(center, &mTVertex->pos, angle);
#endif

	AddVertexs( EE_QUAD_VERTEX );
}

void cBatchRenderer::BatchQuadFree( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3 ) {
#ifndef EE_GLES
	if ( mNumVertex + 3 >= mVertexSize )
		return;
#else
	if ( mNumVertex + 5 >= mVertexSize )
		return;
#endif

	SetBlendMode( DM_QUADS, mForceBlendMode );

#ifndef EE_GLES
	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = x2;
	mTVertex->pos.y = y2;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];

	mTVertex 		= &mVertex[ mNumVertex + 3 ];
	mTVertex->pos.x = x3;
	mTVertex->pos.y = y3;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
#else
	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = x3;
	mTVertex->pos.y = y3;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];

	mTVertex 		= &mVertex[ mNumVertex + 3 ];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex 		= &mVertex[ mNumVertex + 4 ];
	mTVertex->pos.x = x2;
	mTVertex->pos.y = y2;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];

	mTVertex 		= &mVertex[ mNumVertex + 5 ];
	mTVertex->pos.x = x3;
	mTVertex->pos.y = y3;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
#endif

	AddVertexs( EE_QUAD_VERTEX );
}

void cBatchRenderer::BatchQuadFreeEx( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3, const eeFloat& Angle, const eeFloat& Scale ) {
	#ifndef EE_GLES
	if ( mNumVertex + 3 >= mVertexSize )
		return;
	#else
	if ( mNumVertex + 5 >= mVertexSize )
		return;
	#endif

	eeQuad2f mQ;
	mQ.V[0].x = x0; mQ.V[1].x = x1; mQ.V[2].x = x2; mQ.V[3].x = x3;
	mQ.V[0].y = y0; mQ.V[1].y = y1; mQ.V[2].y = y2; mQ.V[3].y = y3;
	eeFloat MinX = mQ.V[0].x, MaxX = mQ.V[0].x, MinY = mQ.V[0].y, MaxY = mQ.V[0].y;
	eeVector2f QCenter;

	if ( Angle != 0 ||  Scale != 1.0f ) {
		for (Uint8 i = 1; i < 4; i++ ) {
			if ( MinX > mQ.V[i].x ) MinX = mQ.V[i].x;
			if ( MaxX < mQ.V[i].x ) MaxX = mQ.V[i].x;
			if ( MinY > mQ.V[i].y ) MinY = mQ.V[i].y;
			if ( MaxY < mQ.V[i].y ) MaxY = mQ.V[i].y;
		}

		QCenter.x = MinX + ( MaxX - MinX ) * 0.5f;
		QCenter.y = MinY + ( MaxY - MinY ) * 0.5f;
	}

	if ( Scale != 1.0f ) {
		for (Uint8 i = 0; i < 4; i++ ) {
			if ( mQ.V[i].x < QCenter.x )
				mQ.V[i].x = QCenter.x - eeabs(QCenter.x - mQ.V[i].x) * Scale;
			else
				mQ.V[i].x = QCenter.x + eeabs(QCenter.x - mQ.V[i].x) * Scale;

			if ( mQ.V[i].y < QCenter.y )
				mQ.V[i].y = QCenter.y - eeabs(QCenter.y - mQ.V[i].y) * Scale;
			else
				mQ.V[i].y = QCenter.y + eeabs(QCenter.y - mQ.V[i].y) * Scale;
		}
	}

	if ( Angle != 0 )
		mQ = RotateQuadCentered( mQ, Angle, QCenter );

	SetBlendMode( DM_QUADS, mForceBlendMode );

#ifndef EE_GLES
	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = mQ[0].x;
	mTVertex->pos.y = mQ[0].y;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = mQ[1].x;
	mTVertex->pos.y = mQ[1].y;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = mQ[2].x;
	mTVertex->pos.y = mQ[2].y;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];

	mTVertex 		= &mVertex[ mNumVertex + 3 ];
	mTVertex->pos.x = mQ[3].x;
	mTVertex->pos.y = mQ[3].y;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
#else
	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = mQ[1].x;
	mTVertex->pos.y = mQ[1].y;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = mQ[0].x;
	mTVertex->pos.y = mQ[0].y;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = mQ[3].x;
	mTVertex->pos.y = mQ[3].y;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];

	mTVertex 		= &mVertex[ mNumVertex + 3 ];
	mTVertex->pos.x = mQ[1].x;
	mTVertex->pos.y = mQ[1].y;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex 		= &mVertex[ mNumVertex + 4 ];
	mTVertex->pos.x = mQ[2].x;
	mTVertex->pos.y = mQ[2].y;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];

	mTVertex 		= &mVertex[ mNumVertex + 5 ];
	mTVertex->pos.x = mQ[3].x;
	mTVertex->pos.y = mQ[3].y;
	mTVertex->tex 	= mTexCoord[3];
	mTVertex->color = mVerColor[3];
#endif

	AddVertexs( EE_QUAD_VERTEX );
}

void cBatchRenderer::QuadsBegin() {
	SetBlendMode( DM_QUADS, true );
	QuadsSetSubset( 0, 0, 1, 1 );
	QuadsSetColor( eeColorA() );
}

void cBatchRenderer::QuadsSetColor( const eeColorA& Color ) {
	mVerColor[0] = mVerColor[1] = mVerColor[2] = mVerColor[3] = Color;
}

void cBatchRenderer::QuadsSetColorFree( const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3 ) {
	mVerColor[0] = Color0;
	mVerColor[1] = Color1;
	mVerColor[2] = Color2;
	mVerColor[3] = Color3;
}

void cBatchRenderer::QuadsSetSubset( const eeFloat& tl_u, const eeFloat& tl_v, const eeFloat& br_u, const eeFloat& br_v ) {
	mTexCoord[0].u = tl_u;	mTexCoord[1].u = tl_u;
	mTexCoord[0].v = tl_v;	mTexCoord[1].v = br_v;

	mTexCoord[2].u = br_u;	mTexCoord[3].u = br_u;
	mTexCoord[2].v = br_v;	mTexCoord[3].v = tl_v;
}

void cBatchRenderer::QuadsSetSubsetFree( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2, const eeFloat& x3, const eeFloat& y3 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
	mTexCoord[3].u = x3; mTexCoord[3].v = y3;
}

void cBatchRenderer::Rotate( const eeVector2f& center, eeVector2f* point, const eeFloat& angle ) {
	if ( angle ) {
		eeFloat x = point->x - center.x;
		eeFloat y = point->y - center.y;
		point->x = x * cosAng(angle) - y * sinAng(angle) + center.x;
		point->y = x * sinAng(angle) + y * cosAng(angle) + center.y;
	}
}

void cBatchRenderer::PointsBegin() {
	SetBlendMode( DM_POINTS, true );
	QuadsSetSubset( 0, 0, 1, 1 );
	PointSetColor( eeColorA() );
}

void cBatchRenderer::PointSetColor( const eeColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::BatchPoint( const eeFloat& x, const eeFloat& y ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	SetBlendMode( DM_POINTS, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	AddVertexs(1);
}

void cBatchRenderer::LinesBegin() {
	SetBlendMode( DM_LINES, true );
	QuadsSetSubset( 0, 0, 1, 1 );
	PointSetColor( eeColorA() );
}

void cBatchRenderer::LinesSetColor( const eeColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::LinesSetColorFree( const eeColorA& Color0, const eeColorA& Color1 ) {
	QuadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void cBatchRenderer::BatchLine( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	SetBlendMode( DM_LINES, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	AddVertexs(2);
}

void cBatchRenderer::LineLoopBegin() {
	SetBlendMode( DM_LINE_LOOP, true );
	QuadsSetSubset( 0, 0, 1, 1 );
	PointSetColor( eeColorA() );
}

void cBatchRenderer::LineLoopSetColor( const eeColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::LineLoopSetColorFree( const eeColorA& Color0, const eeColorA& Color1 ) {
	QuadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void cBatchRenderer::BatchLineLoop( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	SetBlendMode( DM_LINE_LOOP, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	AddVertexs(2);
}

void cBatchRenderer::BatchLineLoop( const eeVector2f& vector1, const eeVector2f& vector2 ) {
	BatchLineLoop( vector1.x, vector1.y, vector2.x, vector2.y );
}

void cBatchRenderer::BatchLineLoop( const eeFloat& x0, const eeFloat& y0 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	SetBlendMode( DM_LINE_LOOP, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	AddVertexs(1);
}

void cBatchRenderer::BatchLineLoop( const eeVector2f& vector1 ) {
	BatchLineLoop( vector1.x, vector1.y );
}

void cBatchRenderer::TriangleFanBegin() {
	SetBlendMode( DM_TRIANGLE_FAN, true );
	TriangleFanSetSubset( 0, 0, 0, 1, 1, 1 );
	TriangleFanSetColor( eeColorA() );
}

void cBatchRenderer::TriangleFanSetColor( const eeColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::TriangleFanSetColorFree( const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2 ) {
	QuadsSetColorFree( Color0, Color1, Color2, Color0 );
}

void cBatchRenderer::TriangleFanSetSubset( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
}

void cBatchRenderer::BatchTriangleFan( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2 ) {

	if ( mNumVertex + 2 >= mVertexSize )
		return;

	SetBlendMode( DM_TRIANGLE_FAN, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = x2;
	mTVertex->pos.y = y2;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];

	AddVertexs(3);
}

void cBatchRenderer::TrianglesBegin() {
	SetBlendMode( DM_TRIANGLES, true );
	TrianglesSetSubset( 0, 0, 0, 1, 1, 1 );
	TrianglesSetColor( eeColorA() );
}

void cBatchRenderer::TrianglesSetColor( const eeColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::TrianglesSetColorFree( const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2 ) {
	QuadsSetColorFree( Color0, Color1, Color2, Color0 );
}

void cBatchRenderer::TrianglesSetSubset( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
}

void cBatchRenderer::BatchTriangle( const eeFloat& x0, const eeFloat& y0, const eeFloat& x1, const eeFloat& y1, const eeFloat& x2, const eeFloat& y2 ) {

	if ( mNumVertex + 2 >= mVertexSize )
		return;

	SetBlendMode( DM_TRIANGLES, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex 		= &mVertex[ mNumVertex + 1 ];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex 	= mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex 		= &mVertex[ mNumVertex + 2 ];
	mTVertex->pos.x = x2;
	mTVertex->pos.y = y2;
	mTVertex->tex 	= mTexCoord[2];
	mTVertex->color = mVerColor[2];

	AddVertexs(3);
}

void cBatchRenderer::PolygonSetColor( const eeColorA& Color ) {
	PointSetColor( Color );
}

void cBatchRenderer::BatchPolygon( const eePolygon2f& Polygon ) {
	if ( Polygon.Size() > mVertexSize )
		return;

	SetBlendMode( DM_POLYGON, mForceBlendMode );

	for ( Uint32 i = 0; i < Polygon.Size(); i++ ) {
		mTVertex = &mVertex[ mNumVertex ];

		mTVertex->pos.x = Polygon.X() + Polygon[i].x;
		mTVertex->pos.y = Polygon.Y() + Polygon[i].y;
		mTVertex->tex 	= mTexCoord[0];
		mTVertex->color = mVerColor[0];

		AddVertexs(1);
	}
}

void cBatchRenderer::BatchPolygonByPoint( const eeFloat& x, const eeFloat& y ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	SetBlendMode( DM_POLYGON, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	AddVertexs(1);
}

void cBatchRenderer::BatchPolygonByPoint( const eeVector2f& Vector ) {
	BatchPolygonByPoint( Vector.x, Vector.y );
}

void cBatchRenderer::SetLineWidth( const eeFloat& lineWidth ) {
	glLineWidth( lineWidth );
}

eeFloat cBatchRenderer::GetLineWidth() {
	float lw = 1;

	glGetFloatv( GL_LINE_WIDTH, &lw );

	return lw;
}

void cBatchRenderer::SetPointSize( const eeFloat& pointSize ) {
	GLi->PointSize( pointSize );
}

eeFloat cBatchRenderer::GetPointSize() {
	return GLi->PointSize();
}

void cBatchRenderer::ForceBlendModeChange( const bool& Force ) {
	mForceBlendMode = Force;
}

const bool& cBatchRenderer::ForceBlendModeChange() const {
	return mForceBlendMode;
}

}}
