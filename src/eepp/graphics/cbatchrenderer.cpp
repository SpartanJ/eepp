#include <eepp/graphics/cbatchrenderer.hpp>
#include <eepp/graphics/ctexture.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/cgl.hpp>

namespace EE { namespace Graphics {

cBatchRenderer::cBatchRenderer() :
	mVertex( NULL ),
	mVertexSize( 0 ),
	mTVertex( NULL ),
	mNumVertex(0),
	mTexture(NULL),
	mTF( cTextureFactory::instance() ),
	mBlend(ALPHA_NORMAL),
	mCurrentMode(DM_QUADS),
	mRotation(0.0f),
	mScale(1.0f,1.0f),
	mPosition(0.0f, 0.0f),
	mCenter(0.0f, 0.0f),
	mForceRendering(false),
	mForceBlendMode(true)
{
	AllocVertexs( 1024 );
	Init();
}

cBatchRenderer::cBatchRenderer( const unsigned int& Prealloc ) :
	mVertex( NULL ),
	mVertexSize( 0 ),
	mTVertex( NULL ),
	mNumVertex(0),
	mTexture(NULL),
	mTF( cTextureFactory::instance() ),
	mBlend(ALPHA_NORMAL),
	mCurrentMode(DM_QUADS),
	mRotation(0.0f),
	mScale(1.0f,1.0f),
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

void cBatchRenderer::AllocVertexs( const unsigned int& size ) {
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

void cBatchRenderer::SetBlendMode( const EE_BLEND_MODE& Blend ) {
	if ( Blend != mBlend )
		Flush();

	mBlend = Blend;
}

void cBatchRenderer::AddVertexs( const unsigned int& num ) {
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

	BlendMode::SetMode( mBlend );

	if ( mCurrentMode == DM_POINTS && NULL != mTexture ) {
		GLi->Enable( GL_POINT_SPRITE );
		GLi->PointSize( (float)mTexture->Width() );
	}

	if ( CreateMatrix ) {
		GLi->LoadIdentity();
		GLi->PushMatrix();

		GLi->Translatef( mPosition.x + mCenter.x, mPosition.y + mCenter.y, 0.0f);
		GLi->Rotatef( mRotation, 0.0f, 0.0f, 1.0f );
		GLi->Scalef( mScale.x, mScale.y, 1.0f );
		GLi->Translatef( -mCenter.x, -mCenter.y, 0.0f);
	}

	Uint32 alloc	= sizeof(eeVertex) * NumVertex;

	if ( NULL != mTexture ) {
		mTF->Bind( mTexture );
		GLi->TexCoordPointer( 2, GL_FP			, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] ) + sizeof(eeVector2f)						, alloc		);
	} else {
		GLi->Disable( GL_TEXTURE_2D );
		GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	GLi->VertexPointer	( 2, GL_FP				, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] )												, alloc		);
	GLi->ColorPointer	( 4, GL_UNSIGNED_BYTE	, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] ) + sizeof(eeVector2f) + sizeof(eeTexCoord)	, alloc		);

	if ( !GLi->QuadsSupported() ) {
		if ( DM_QUADS == mCurrentMode ) {
			GLi->DrawArrays( DM_TRIANGLES, 0, NumVertex );
		} else if ( DM_POLYGON == mCurrentMode ) {
			GLi->DrawArrays( DM_TRIANGLE_FAN, 0, NumVertex );
		} else {
			GLi->DrawArrays( mCurrentMode, 0, NumVertex );
		}
	} else {
		GLi->DrawArrays( mCurrentMode, 0, NumVertex );
	}

	if ( CreateMatrix ) {
		GLi->PopMatrix();
	}

	if ( mCurrentMode == DM_POINTS && NULL != mTexture ) {
		GLi->Disable( GL_POINT_SPRITE );
	}

	if ( NULL == mTexture ) {
		GLi->Enable( GL_TEXTURE_2D );
		GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
	}
}

void cBatchRenderer::BatchQuad( const Float& x, const Float& y, const Float& width, const Float& height, const Float& angle ) {
	BatchQuadEx( x, y, width, height, angle );
}

void cBatchRenderer::BatchQuadEx( Float x, Float y, Float width, Float height, Float angle, eeVector2f scale, eeOriginPoint originPoint ) {
	if ( mNumVertex + ( GLi->QuadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	if ( originPoint.OriginType == eeOriginPoint::OriginCenter ) {
		originPoint.x = width  * 0.5f;
		originPoint.y = height * 0.5f;
	}

	if ( scale != 1.0f ) {
		x				= x + originPoint.x - originPoint.x * scale.x;
		y				= y + originPoint.y - originPoint.y * scale.y;
		width			*= scale.x;
		height			*= scale.y;
		originPoint		*= scale;
	}

	originPoint.x += x;
	originPoint.y += y;

	SetBlendMode( DM_QUADS, mForceBlendMode );

	if ( GLi->QuadsSupported() ) {
		mTVertex 		= &mVertex[ mNumVertex ];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y;
		mTVertex->tex 	= mTexCoord[0];
		mTVertex->color = mVerColor[0];
		Rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 1 ];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y + height;
		mTVertex->tex 	= mTexCoord[1];
		mTVertex->color = mVerColor[1];
		Rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 2 ];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y + height;
		mTVertex->tex 	= mTexCoord[2];
		mTVertex->color = mVerColor[2];
		Rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 3 ];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y;
		mTVertex->tex 	= mTexCoord[3];
		mTVertex->color = mVerColor[3];
		Rotate(originPoint, &mTVertex->pos, angle);

		AddVertexs( 4 );
	} else {
		mTVertex 		= &mVertex[ mNumVertex ];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y + height;
		mTVertex->tex 	= mTexCoord[1];
		mTVertex->color = mVerColor[1];
		Rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 1 ];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y;
		mTVertex->tex 	= mTexCoord[0];
		mTVertex->color = mVerColor[0];
		Rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 2 ];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y;
		mTVertex->tex 	= mTexCoord[3];
		mTVertex->color = mVerColor[3];
		Rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 3 ];
		mTVertex->pos	= mVertex[ mNumVertex ].pos;
		mTVertex->tex 	= mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex 		= &mVertex[ mNumVertex + 4 ];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y + height;
		mTVertex->tex 	= mTexCoord[2];
		mTVertex->color = mVerColor[2];
		Rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 5 ];
		mTVertex->pos	= mVertex[ mNumVertex + 2 ].pos;
		mTVertex->tex 	= mTexCoord[3];
		mTVertex->color = mVerColor[3];

		AddVertexs( 6 );
	}
}

void cBatchRenderer::BatchQuadFree( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3 ) {
	if ( mNumVertex + ( GLi->QuadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	SetBlendMode( DM_QUADS, mForceBlendMode );

	if ( GLi->QuadsSupported() ) {
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

		AddVertexs( 4 );
	} else {
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

		AddVertexs( 6 );
	}
}

void cBatchRenderer::BatchQuadFreeEx( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3, const Float& Angle, const Float& Scale ) {
	if ( mNumVertex + ( GLi->QuadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	eeQuad2f mQ;
	eeVector2f QCenter;
	mQ.V[0].x = x0; mQ.V[1].x = x1; mQ.V[2].x = x2; mQ.V[3].x = x3;
	mQ.V[0].y = y0; mQ.V[1].y = y1; mQ.V[2].y = y2; mQ.V[3].y = y3;

	if ( Angle != 0 ||  Scale != 1.0f ) {
		QCenter = mQ.GetCenter();
		mQ.Rotate( Angle, QCenter );
		mQ.Scale( Scale, QCenter );
	}

	SetBlendMode( DM_QUADS, mForceBlendMode );

	if ( GLi->QuadsSupported() ) {
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

		AddVertexs( 4 );
	} else {
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

		AddVertexs( 6 );
	}
}

void cBatchRenderer::QuadsBegin() {
	SetBlendMode( DM_QUADS, true );
	QuadsSetSubset( 0, 0, 1, 1 );
	QuadsSetColor( ColorA() );
}

void cBatchRenderer::QuadsSetColor( const ColorA& Color ) {
	mVerColor[0] = mVerColor[1] = mVerColor[2] = mVerColor[3] = Color;
}

void cBatchRenderer::QuadsSetColorFree( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3 ) {
	mVerColor[0] = Color0;
	mVerColor[1] = Color1;
	mVerColor[2] = Color2;
	mVerColor[3] = Color3;
}

void cBatchRenderer::QuadsSetSubset( const Float& tl_u, const Float& tl_v, const Float& br_u, const Float& br_v ) {
	mTexCoord[0].u = tl_u;	mTexCoord[1].u = tl_u;
	mTexCoord[0].v = tl_v;	mTexCoord[1].v = br_v;

	mTexCoord[2].u = br_u;	mTexCoord[3].u = br_u;
	mTexCoord[2].v = br_v;	mTexCoord[3].v = tl_v;
}

void cBatchRenderer::QuadsSetSubsetFree( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
	mTexCoord[3].u = x3; mTexCoord[3].v = y3;
}

void cBatchRenderer::Rotate( const eeVector2f& center, eeVector2f* point, const Float& angle ) {
	if ( angle ) {
		Float x = point->x - center.x;
		Float y = point->y - center.y;
		point->x = x * Math::cosAng(angle) - y * Math::sinAng(angle) + center.x;
		point->y = x * Math::sinAng(angle) + y * Math::cosAng(angle) + center.y;
	}
}

void cBatchRenderer::PointsBegin() {
	SetBlendMode( DM_POINTS, true );
	QuadsSetSubset( 0, 0, 1, 1 );
	PointSetColor( ColorA() );
}

void cBatchRenderer::PointSetColor( const ColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::BatchPoint( const Float& x, const Float& y ) {
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
	PointSetColor( ColorA() );
}

void cBatchRenderer::LinesSetColor( const ColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::LinesSetColorFree( const ColorA& Color0, const ColorA& Color1 ) {
	QuadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void cBatchRenderer::BatchLine( const Float& x0, const Float& y0, const Float& x1, const Float& y1 ) {
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
	PointSetColor( ColorA() );
}

void cBatchRenderer::LineLoopSetColor( const ColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::LineLoopSetColorFree( const ColorA& Color0, const ColorA& Color1 ) {
	QuadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void cBatchRenderer::BatchLineLoop( const Float& x0, const Float& y0, const Float& x1, const Float& y1 ) {
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

void cBatchRenderer::BatchLineLoop( const Float& x0, const Float& y0 ) {
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
	TriangleFanSetColor( ColorA() );
}

void cBatchRenderer::TriangleFanSetColor( const ColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::TriangleFanSetColorFree( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2 ) {
	QuadsSetColorFree( Color0, Color1, Color2, Color0 );
}

void cBatchRenderer::TriangleFanSetSubset( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
}

void cBatchRenderer::BatchTriangleFan( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 ) {

	if ( mNumVertex + 3 >= mVertexSize )
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

void cBatchRenderer::BatchTriangleFan( const Float& x0, const Float& y0 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	SetBlendMode( DM_TRIANGLE_FAN, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	AddVertexs(1);
}

void cBatchRenderer::TrianglesBegin() {
	SetBlendMode( DM_TRIANGLES, true );
	TrianglesSetSubset( 0, 0, 0, 1, 1, 1 );
	TrianglesSetColor( ColorA() );
}

void cBatchRenderer::TrianglesSetColor( const ColorA& Color ) {
	QuadsSetColor( Color );
}

void cBatchRenderer::TrianglesSetColorFree( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2 ) {
	QuadsSetColorFree( Color0, Color1, Color2, Color0 );
}

void cBatchRenderer::TrianglesSetSubset( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
}

void cBatchRenderer::BatchTriangle( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 ) {

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

void cBatchRenderer::PolygonSetColor( const ColorA& Color ) {
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

void cBatchRenderer::BatchPolygonByPoint( const Float& x, const Float& y ) {
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

void cBatchRenderer::SetLineWidth( const Float& lineWidth ) {
	GLi->LineWidth( lineWidth );
}

Float cBatchRenderer::GetLineWidth() {
	float lw = 1;

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	glGetFloatv( GL_LINE_WIDTH, &lw );
#endif

	return lw;
}

void cBatchRenderer::SetPointSize( const Float& pointSize ) {
	GLi->PointSize( pointSize );
}

Float cBatchRenderer::GetPointSize() {
	return GLi->PointSize();
}

void cBatchRenderer::ForceBlendModeChange( const bool& Force ) {
	mForceBlendMode = Force;
}

const bool& cBatchRenderer::ForceBlendModeChange() const {
	return mForceBlendMode;
}

}}
