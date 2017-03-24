#include <eepp/graphics/batchrenderer.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>

namespace EE { namespace Graphics {

BatchRenderer::BatchRenderer() :
	mVertex( NULL ),
	mVertexSize( 0 ),
	mTVertex( NULL ),
	mNumVertex(0),
	mTexture(NULL),
	mTF( TextureFactory::instance() ),
	mBlend(ALPHA_NORMAL),
	mCurrentMode(DM_QUADS),
	mRotation(0.0f),
	mScale(1.0f,1.0f),
	mPosition(0.0f, 0.0f),
	mCenter(0.0f, 0.0f),
	mForceRendering(false),
	mForceBlendMode(true)
{
	allocVertexs( 1024 );
	init();
}

BatchRenderer::BatchRenderer( const unsigned int& Prealloc ) :
	mVertex( NULL ),
	mVertexSize( 0 ),
	mTVertex( NULL ),
	mNumVertex(0),
	mTexture(NULL),
	mTF( TextureFactory::instance() ),
	mBlend(ALPHA_NORMAL),
	mCurrentMode(DM_QUADS),
	mRotation(0.0f),
	mScale(1.0f,1.0f),
	mPosition(0.0f, 0.0f),
	mCenter(0.0f, 0.0f),
	mForceRendering(false),
	mForceBlendMode(true)
{
	allocVertexs( Prealloc );
	init();
}

BatchRenderer::~BatchRenderer() {
	eeSAFE_DELETE_ARRAY( mVertex );
}

void BatchRenderer::init() {
	quadsBegin();
}

void BatchRenderer::allocVertexs( const unsigned int& size ) {
	eeSAFE_DELETE_ARRAY( mVertex );
	mVertex		= eeNewArray( eeVertex, size );
	mVertexSize = size;
	mNumVertex	= 0;
}

void BatchRenderer::drawOpt() {
	if ( mForceRendering )
		flush();
}

void BatchRenderer::draw() {
	flush();
}

void BatchRenderer::setTexture( const Texture * Tex ) {
	if ( mTexture != Tex )
		flush();

	mTexture = Tex;
}

void BatchRenderer::setBlendMode( const EE_BLEND_MODE& Blend ) {
	if ( Blend != mBlend )
		flush();

	mBlend = Blend;
}

void BatchRenderer::addVertexs( const unsigned int& num ) {
	mNumVertex += num;

	if ( ( mNumVertex + num ) >= mVertexSize )
		flush();
}

void BatchRenderer::setDrawMode( const EE_DRAW_MODE& Mode, const bool& Force ) {
	if ( Force && mCurrentMode != Mode ) {
		flush();
		mCurrentMode = Mode;
	}
}

void BatchRenderer::flush() {	
	if ( mNumVertex == 0 )
		return;

	if ( GlobalBatchRenderer::instance() != this )
		GlobalBatchRenderer::instance()->draw();

	Uint32 NumVertex = mNumVertex;
	mNumVertex = 0;

	bool CreateMatrix = ( mRotation || mScale != 1.0f || mPosition.x || mPosition.y );

	BlendMode::setMode( mBlend );

	if ( mCurrentMode == DM_POINTS && NULL != mTexture ) {
		GLi->enable( GL_POINT_SPRITE );
		GLi->pointSize( (float)mTexture->getWidth() );
	}

	if ( CreateMatrix ) {
		GLi->loadIdentity();
		GLi->pushMatrix();

		GLi->translatef( mPosition.x + mCenter.x, mPosition.y + mCenter.y, 0.0f);
		GLi->rotatef( mRotation, 0.0f, 0.0f, 1.0f );
		GLi->scalef( mScale.x, mScale.y, 1.0f );
		GLi->translatef( -mCenter.x, -mCenter.y, 0.0f);
	}

	Uint32 alloc	= sizeof(eeVertex) * NumVertex;

	if ( NULL != mTexture ) {
		mTF->bind( mTexture );
		GLi->texCoordPointer( 2, GL_FP			, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] ) + sizeof(Vector2f)						, alloc		);
	} else {
		GLi->disable( GL_TEXTURE_2D );
		GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	GLi->vertexPointer	( 2, GL_FP				, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] )												, alloc		);
	GLi->colorPointer	( 4, GL_UNSIGNED_BYTE	, sizeof(eeVertex), reinterpret_cast<char*> ( &mVertex[0] ) + sizeof(Vector2f) + sizeof(eeTexCoord)	, alloc		);

	if ( !GLi->quadsSupported() ) {
		if ( DM_QUADS == mCurrentMode ) {
			GLi->drawArrays( DM_TRIANGLES, 0, NumVertex );
		} else if ( DM_POLYGON == mCurrentMode ) {
			GLi->drawArrays( DM_TRIANGLE_FAN, 0, NumVertex );
		} else {
			GLi->drawArrays( mCurrentMode, 0, NumVertex );
		}
	} else {
		GLi->drawArrays( mCurrentMode, 0, NumVertex );
	}

	if ( CreateMatrix ) {
		GLi->popMatrix();
	}

	if ( mCurrentMode == DM_POINTS && NULL != mTexture ) {
		GLi->disable( GL_POINT_SPRITE );
	}

	if ( NULL == mTexture ) {
		GLi->enable( GL_TEXTURE_2D );
		GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
	}
}

void BatchRenderer::batchQuad( const Float& x, const Float& y, const Float& width, const Float& height, const Float& angle ) {
	batchQuadEx( x, y, width, height, angle );
}

void BatchRenderer::batchQuadEx( Float x, Float y, Float width, Float height, Float angle, Vector2f scale, OriginPoint originPoint ) {
	if ( mNumVertex + ( GLi->quadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	if ( originPoint.OriginType == OriginPoint::OriginCenter ) {
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

	setDrawMode( DM_QUADS, mForceBlendMode );

	if ( GLi->quadsSupported() ) {
		mTVertex 		= &mVertex[ mNumVertex ];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y;
		mTVertex->tex 	= mTexCoord[0];
		mTVertex->color = mVerColor[0];
		rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 1 ];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y + height;
		mTVertex->tex 	= mTexCoord[1];
		mTVertex->color = mVerColor[1];
		rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 2 ];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y + height;
		mTVertex->tex 	= mTexCoord[2];
		mTVertex->color = mVerColor[2];
		rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 3 ];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y;
		mTVertex->tex 	= mTexCoord[3];
		mTVertex->color = mVerColor[3];
		rotate(originPoint, &mTVertex->pos, angle);

		addVertexs( 4 );
	} else {
		mTVertex 		= &mVertex[ mNumVertex ];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y + height;
		mTVertex->tex 	= mTexCoord[1];
		mTVertex->color = mVerColor[1];
		rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 1 ];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y;
		mTVertex->tex 	= mTexCoord[0];
		mTVertex->color = mVerColor[0];
		rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 2 ];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y;
		mTVertex->tex 	= mTexCoord[3];
		mTVertex->color = mVerColor[3];
		rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 3 ];
		mTVertex->pos	= mVertex[ mNumVertex ].pos;
		mTVertex->tex 	= mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex 		= &mVertex[ mNumVertex + 4 ];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y + height;
		mTVertex->tex 	= mTexCoord[2];
		mTVertex->color = mVerColor[2];
		rotate(originPoint, &mTVertex->pos, angle);

		mTVertex 		= &mVertex[ mNumVertex + 5 ];
		mTVertex->pos	= mVertex[ mNumVertex + 2 ].pos;
		mTVertex->tex 	= mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertexs( 6 );
	}
}

void BatchRenderer::batchQuadFree( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3 ) {
	if ( mNumVertex + ( GLi->quadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	setDrawMode( DM_QUADS, mForceBlendMode );

	if ( GLi->quadsSupported() ) {
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

		addVertexs( 4 );
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

		addVertexs( 6 );
	}
}

void BatchRenderer::batchQuadFreeEx( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3, const Float& Angle, const Float& Scale ) {
	if ( mNumVertex + ( GLi->quadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	Quad2f mQ;
	Vector2f QCenter;
	mQ.V[0].x = x0; mQ.V[1].x = x1; mQ.V[2].x = x2; mQ.V[3].x = x3;
	mQ.V[0].y = y0; mQ.V[1].y = y1; mQ.V[2].y = y2; mQ.V[3].y = y3;

	if ( Angle != 0 ||  Scale != 1.0f ) {
		QCenter = mQ.getCenter();
		mQ.rotate( Angle, QCenter );
		mQ.scale( Scale, QCenter );
	}

	setDrawMode( DM_QUADS, mForceBlendMode );

	if ( GLi->quadsSupported() ) {
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

		addVertexs( 4 );
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

		addVertexs( 6 );
	}
}

void BatchRenderer::quadsBegin() {
	setDrawMode( DM_QUADS, true );
	quadsSetSubset( 0, 0, 1, 1 );
	quadsSetColor( ColorA() );
}

void BatchRenderer::quadsSetColor( const ColorA& Color ) {
	mVerColor[0] = mVerColor[1] = mVerColor[2] = mVerColor[3] = Color;
}

void BatchRenderer::quadsSetColorFree( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3 ) {
	mVerColor[0] = Color0;
	mVerColor[1] = Color1;
	mVerColor[2] = Color2;
	mVerColor[3] = Color3;
}

void BatchRenderer::quadsSetSubset( const Float& tl_u, const Float& tl_v, const Float& br_u, const Float& br_v ) {
	mTexCoord[0].u = tl_u;	mTexCoord[1].u = tl_u;
	mTexCoord[0].v = tl_v;	mTexCoord[1].v = br_v;

	mTexCoord[2].u = br_u;	mTexCoord[3].u = br_u;
	mTexCoord[2].v = br_v;	mTexCoord[3].v = tl_v;
}

void BatchRenderer::quadsSetSubsetFree( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2, const Float& x3, const Float& y3 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
	mTexCoord[3].u = x3; mTexCoord[3].v = y3;
}

void BatchRenderer::rotate( const Vector2f& center, Vector2f* point, const Float& angle ) {
	if ( angle ) {
		Float x = point->x - center.x;
		Float y = point->y - center.y;
		point->x = x * Math::cosAng(angle) - y * Math::sinAng(angle) + center.x;
		point->y = x * Math::sinAng(angle) + y * Math::cosAng(angle) + center.y;
	}
}

void BatchRenderer::pointsBegin() {
	setDrawMode( DM_POINTS, true );
	quadsSetSubset( 0, 0, 1, 1 );
	pointSetColor( ColorA() );
}

void BatchRenderer::pointSetColor( const ColorA& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::batchPoint( const Float& x, const Float& y ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( DM_POINTS, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertexs(1);
}

void BatchRenderer::linesBegin() {
	setDrawMode( DM_LINES, true );
	quadsSetSubset( 0, 0, 1, 1 );
	pointSetColor( ColorA() );
}

void BatchRenderer::linesSetColor( const ColorA& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::linesSetColorFree( const ColorA& Color0, const ColorA& Color1 ) {
	quadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void BatchRenderer::batchLine( const Float& x0, const Float& y0, const Float& x1, const Float& y1 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( DM_LINES, mForceBlendMode );

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

	addVertexs(2);
}

void BatchRenderer::lineLoopBegin() {
	setDrawMode( DM_LINE_LOOP, true );
	quadsSetSubset( 0, 0, 1, 1 );
	pointSetColor( ColorA() );
}

void BatchRenderer::lineLoopSetColor( const ColorA& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::lineLoopSetColorFree( const ColorA& Color0, const ColorA& Color1 ) {
	quadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void BatchRenderer::batchLineLoop( const Float& x0, const Float& y0, const Float& x1, const Float& y1 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( DM_LINE_LOOP, mForceBlendMode );

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

	addVertexs(2);
}

void BatchRenderer::batchLineLoop( const Vector2f& vector1, const Vector2f& vector2 ) {
	batchLineLoop( vector1.x, vector1.y, vector2.x, vector2.y );
}

void BatchRenderer::batchLineLoop( const Float& x0, const Float& y0 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( DM_LINE_LOOP, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertexs(1);
}

void BatchRenderer::batchLineLoop( const Vector2f& vector1 ) {
	batchLineLoop( vector1.x, vector1.y );
}

void BatchRenderer::lineStripBegin() {
	setDrawMode( DM_LINE_STRIP, true );
	quadsSetSubset( 0, 0, 1, 1 );
	pointSetColor( ColorA() );
}

void BatchRenderer::lineStripSetColor( const ColorA& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::lineStripSetColorFree( const ColorA& Color0, const ColorA& Color1 ) {
	quadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void BatchRenderer::batchLineStrip( const Float& x0, const Float& y0, const Float& x1, const Float& y1 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( DM_LINE_STRIP, mForceBlendMode );

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

	addVertexs(2);
}

void BatchRenderer::batchLineStrip( const Vector2f& vector1, const Vector2f& vector2 ) {
	batchLineStrip( vector1.x, vector1.y, vector2.x, vector2.y );
}

void BatchRenderer::batchLineStrip( const Float& x0, const Float& y0 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( DM_LINE_STRIP, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertexs(1);
}

void BatchRenderer::batchLineStrip( const Vector2f& vector1 ) {
	batchLineStrip( vector1.x, vector1.y );
}

void BatchRenderer::triangleFanBegin() {
	setDrawMode( DM_TRIANGLE_FAN, true );
	triangleFanSetSubset( 0, 0, 0, 1, 1, 1 );
	triangleFanSetColor( ColorA() );
}

void BatchRenderer::triangleFanSetColor( const ColorA& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::triangleFanSetColorFree( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2 ) {
	quadsSetColorFree( Color0, Color1, Color2, Color0 );
}

void BatchRenderer::triangleFanSetSubset( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
}

void BatchRenderer::batchTriangleFan( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 ) {

	if ( mNumVertex + 3 >= mVertexSize )
		return;

	setDrawMode( DM_TRIANGLE_FAN, mForceBlendMode );

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

	addVertexs(3);
}

void BatchRenderer::batchTriangleFan( const Float& x0, const Float& y0 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( DM_TRIANGLE_FAN, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertexs(1);
}

void BatchRenderer::trianglesBegin() {
	setDrawMode( DM_TRIANGLES, true );
	trianglesSetSubset( 0, 0, 0, 1, 1, 1 );
	trianglesSetColor( ColorA() );
}

void BatchRenderer::trianglesSetColor( const ColorA& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::trianglesSetColorFree( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2 ) {
	quadsSetColorFree( Color0, Color1, Color2, Color0 );
}

void BatchRenderer::trianglesSetSubset( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 ) {
	mTexCoord[0].u = x0; mTexCoord[0].v = y0;
	mTexCoord[1].u = x1; mTexCoord[1].v = y1;
	mTexCoord[2].u = x2; mTexCoord[2].v = y2;
}

void BatchRenderer::batchTriangle( const Float& x0, const Float& y0, const Float& x1, const Float& y1, const Float& x2, const Float& y2 ) {

	if ( mNumVertex + 2 >= mVertexSize )
		return;

	setDrawMode( DM_TRIANGLES, mForceBlendMode );

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

	addVertexs(3);
}

void BatchRenderer::polygonSetColor( const ColorA& Color ) {
	pointSetColor( Color );
}

void BatchRenderer::batchPolygon( const Polygon2f& Polygon ) {
	if ( Polygon.getSize() > mVertexSize )
		return;

	setDrawMode( DM_POLYGON, mForceBlendMode );

	for ( Uint32 i = 0; i < Polygon.getSize(); i++ ) {
		mTVertex = &mVertex[ mNumVertex ];

		mTVertex->pos.x = Polygon.getX() + Polygon[i].x;
		mTVertex->pos.y = Polygon.getY() + Polygon[i].y;
		mTVertex->tex 	= mTexCoord[0];
		mTVertex->color = mVerColor[0];

		addVertexs(1);
	}
}

void BatchRenderer::batchPolygonByPoint( const Float& x, const Float& y ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( DM_POLYGON, mForceBlendMode );

	mTVertex 		= &mVertex[ mNumVertex ];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y;
	mTVertex->tex 	= mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertexs(1);
}

void BatchRenderer::batchPolygonByPoint( const Vector2f& Vector ) {
	batchPolygonByPoint( Vector.x, Vector.y );
}

void BatchRenderer::setLineWidth( const Float& lineWidth ) {
	GLi->lineWidth( lineWidth );
}

Float BatchRenderer::getLineWidth() {
	float lw = 1;

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	glGetFloatv( GL_LINE_WIDTH, &lw );
#endif

	return lw;
}

void BatchRenderer::setPointSize( const Float& pointSize ) {
	GLi->pointSize( pointSize );
}

Float BatchRenderer::getPointSize() {
	return GLi->pointSize();
}

void BatchRenderer::setForceBlendModeChange( const bool& Force ) {
	mForceBlendMode = Force;
}

const bool& BatchRenderer::getForceBlendModeChange() const {
	return mForceBlendMode;
}

}}
