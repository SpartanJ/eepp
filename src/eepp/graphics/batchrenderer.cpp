#include <eepp/graphics/batchrenderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace Graphics {

BatchRenderer* BatchRenderer::New() {
	return eeNew( BatchRenderer, () );
}

BatchRenderer* BatchRenderer::New( const unsigned int& Prealloc ) {
	return eeNew( BatchRenderer, ( Prealloc ) );
}

BatchRenderer::BatchRenderer() {
	allocVertices( 4096 );
	init();
}

BatchRenderer::BatchRenderer( const unsigned int& Prealloc ) {
	allocVertices( Prealloc );
	init();
}

BatchRenderer::~BatchRenderer() {
	eeSAFE_DELETE_ARRAY( mVertex );
}

void BatchRenderer::init() {
	quadsBegin();
}

void BatchRenderer::allocVertices( const unsigned int& size ) {
	eeSAFE_DELETE_ARRAY( mVertex );
	mVertex = eeNewArray( VertexData, size );
	mVertexSize = size;
	mNumVertex = 0;
}

void BatchRenderer::drawOpt() {
	if ( mForceRendering )
		flush();
}

void BatchRenderer::draw() {
	flush();
}

void BatchRenderer::setTexture( const Texture* texture, Texture::CoordinateType coordinateType ) {
	if ( mTexture != texture || mCoordinateType != coordinateType )
		flush();

	mTexture = texture;
	mCoordinateType = coordinateType;
}

void BatchRenderer::setBlendMode( const BlendMode& blend ) {
	if ( blend != mBlend )
		flush();

	if ( mBlend != blend )
		mBlend = blend;
}

void BatchRenderer::addVertices( const unsigned int& num ) {
	mNumVertex += num;

	if ( ( mNumVertex + num ) >= mVertexSize ) {
		VertexData* newVertex = eeNewArray( VertexData, mVertexSize * 2 );

		for ( Uint32 i = 0; i < mVertexSize; i++ )
			newVertex[i] = mVertex[i];

		eeSAFE_DELETE_ARRAY( mVertex );
		mVertex = newVertex;
		mVertexSize = mVertexSize * 2;
	}
}

void BatchRenderer::setDrawMode( const PrimitiveType& Mode, const bool& Force ) {
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

	bool createMatrix = ( mRotation || mScale != 1.0f || mPosition.x || mPosition.y );

	BlendMode::setMode( mBlend );

	if ( mCurrentMode == PRIMITIVE_POINTS && NULL != mTexture ) {
		GLi->enable( GL_POINT_SPRITE );
		GLi->pointSize( (float)mTexture->getWidth() );
	}

	if ( createMatrix ) {
		GLi->loadIdentity();
		GLi->pushMatrix();

		GLi->translatef( mPosition.x + mCenter.x, mPosition.y + mCenter.y, 0.0f );
		GLi->rotatef( mRotation, 0.0f, 0.0f, 1.0f );
		GLi->scalef( mScale.x, mScale.y, 1.0f );
		GLi->translatef( -mCenter.x, -mCenter.y, 0.0f );
	}

	Uint32 alloc = sizeof( VertexData ) * NumVertex;

	if ( NULL != mTexture ) {
		const_cast<Texture*>( mTexture )->bind( mCoordinateType );
		GLi->texCoordPointer( 2, GL_FP, sizeof( VertexData ),
							  reinterpret_cast<char*>( &mVertex[0] ) + sizeof( Vector2f ), alloc );
	} else {
		GLi->disable( GL_TEXTURE_2D );
		GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	GLi->vertexPointer( 2, GL_FP, sizeof( VertexData ), reinterpret_cast<char*>( &mVertex[0] ),
						alloc );
	GLi->colorPointer(
		4, GL_UNSIGNED_BYTE, sizeof( VertexData ),
		reinterpret_cast<char*>( &mVertex[0] ) + sizeof( Vector2f ) + sizeof( Vector2f ), alloc );

	if ( !GLi->quadsSupported() ) {
		if ( PRIMITIVE_QUADS == mCurrentMode ) {
			GLi->drawArrays( PRIMITIVE_TRIANGLES, 0, NumVertex );
		} else if ( PRIMITIVE_POLYGON == mCurrentMode ) {
			GLi->drawArrays( PRIMITIVE_TRIANGLE_FAN, 0, NumVertex );
		} else {
			GLi->drawArrays( mCurrentMode, 0, NumVertex );
		}
	} else {
		GLi->drawArrays( mCurrentMode, 0, NumVertex );
	}

	if ( createMatrix ) {
		GLi->popMatrix();
	}

	if ( mCurrentMode == PRIMITIVE_POINTS && NULL != mTexture ) {
		GLi->disable( GL_POINT_SPRITE );
	}

	if ( NULL == mTexture ) {
		GLi->enable( GL_TEXTURE_2D );
		GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
	}
}

void BatchRenderer::batchQuad( const Float& x, const Float& y, const Float& width,
							   const Float& height ) {
	if ( mNumVertex + ( GLi->quadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_QUADS, mForceBlendMode );

	if ( GLi->quadsSupported() ) {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y + height;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y + height;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 4 );
	} else {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y + height;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos = mVertex[mNumVertex].pos;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 4];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y + height;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];

		mTVertex = &mVertex[mNumVertex + 5];
		mTVertex->pos = mVertex[mNumVertex + 2].pos;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 6 );
	}
}

void BatchRenderer::batchQuad( const Rectf& rect ) {
	if ( mNumVertex + ( GLi->quadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_QUADS, mForceBlendMode );

	if ( GLi->quadsSupported() ) {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = rect.Left;
		mTVertex->pos.y = rect.Top;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = rect.Left;
		mTVertex->pos.y = rect.Bottom;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = rect.Right;
		mTVertex->pos.y = rect.Bottom;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos.x = rect.Right;
		mTVertex->pos.y = rect.Top;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 4 );
	} else {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = rect.Left;
		mTVertex->pos.y = rect.Bottom;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = rect.Left;
		mTVertex->pos.y = rect.Top;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = rect.Right;
		mTVertex->pos.y = rect.Top;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos = mVertex[mNumVertex].pos;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 4];
		mTVertex->pos.x = rect.Right;
		mTVertex->pos.y = rect.Bottom;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];

		mTVertex = &mVertex[mNumVertex + 5];
		mTVertex->pos = mVertex[mNumVertex + 2].pos;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 6 );
	}
}

void BatchRenderer::batchQuadEx( Float x, Float y, Float width, Float height, Float angle,
								 Vector2f scale, OriginPoint originPoint ) {
	if ( mNumVertex + ( GLi->quadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	if ( originPoint.OriginType == OriginPoint::OriginCenter ) {
		originPoint.x = width * 0.5f;
		originPoint.y = height * 0.5f;
	}

	if ( scale != 1.0f ) {
		x = x + originPoint.x - originPoint.x * scale.x;
		y = y + originPoint.y - originPoint.y * scale.y;
		width *= scale.x;
		height *= scale.y;
		originPoint *= scale;
	}

	originPoint.x += x;
	originPoint.y += y;

	setDrawMode( PRIMITIVE_QUADS, mForceBlendMode );

	if ( GLi->quadsSupported() ) {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];
		rotate( originPoint, &mTVertex->pos, angle );

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y + height;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];
		rotate( originPoint, &mTVertex->pos, angle );

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y + height;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];
		rotate( originPoint, &mTVertex->pos, angle );

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];
		rotate( originPoint, &mTVertex->pos, angle );

		addVertices( 4 );
	} else {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y + height;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];
		rotate( originPoint, &mTVertex->pos, angle );

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = x;
		mTVertex->pos.y = y;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];
		rotate( originPoint, &mTVertex->pos, angle );

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];
		rotate( originPoint, &mTVertex->pos, angle );

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos = mVertex[mNumVertex].pos;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 4];
		mTVertex->pos.x = x + width;
		mTVertex->pos.y = y + height;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];
		rotate( originPoint, &mTVertex->pos, angle );

		mTVertex = &mVertex[mNumVertex + 5];
		mTVertex->pos = mVertex[mNumVertex + 2].pos;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 6 );
	}
}

void BatchRenderer::batchQuadFree( const Float& x0, const Float& y0, const Float& x1,
								   const Float& y1, const Float& x2, const Float& y2,
								   const Float& x3, const Float& y3 ) {
	if ( mNumVertex + ( GLi->quadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_QUADS, mForceBlendMode );

	if ( GLi->quadsSupported() ) {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = x0;
		mTVertex->pos.y = y0;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = x1;
		mTVertex->pos.y = y1;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = x2;
		mTVertex->pos.y = y2;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos.x = x3;
		mTVertex->pos.y = y3;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 4 );
	} else {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = x1;
		mTVertex->pos.y = y1;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = x0;
		mTVertex->pos.y = y0;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = x3;
		mTVertex->pos.y = y3;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos.x = x1;
		mTVertex->pos.y = y1;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 4];
		mTVertex->pos.x = x2;
		mTVertex->pos.y = y2;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];

		mTVertex = &mVertex[mNumVertex + 5];
		mTVertex->pos.x = x3;
		mTVertex->pos.y = y3;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 6 );
	}
}

void BatchRenderer::batchQuadFreeEx( const Float& x0, const Float& y0, const Float& x1,
									 const Float& y1, const Float& x2, const Float& y2,
									 const Float& x3, const Float& y3, const Float& Angle,
									 const Float& Scale ) {
	if ( mNumVertex + ( GLi->quadsSupported() ? 3 : 5 ) >= mVertexSize )
		return;

	Quad2f mQ;
	Vector2f QCenter;
	mQ.V[0].x = x0;
	mQ.V[1].x = x1;
	mQ.V[2].x = x2;
	mQ.V[3].x = x3;
	mQ.V[0].y = y0;
	mQ.V[1].y = y1;
	mQ.V[2].y = y2;
	mQ.V[3].y = y3;

	if ( Angle != 0 || Scale != 1.0f ) {
		QCenter = mQ.getCenter();
		mQ.rotate( Angle, QCenter );
		mQ.scale( Scale, QCenter );
	}

	setDrawMode( PRIMITIVE_QUADS, mForceBlendMode );

	if ( GLi->quadsSupported() ) {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = mQ[0].x;
		mTVertex->pos.y = mQ[0].y;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = mQ[1].x;
		mTVertex->pos.y = mQ[1].y;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = mQ[2].x;
		mTVertex->pos.y = mQ[2].y;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos.x = mQ[3].x;
		mTVertex->pos.y = mQ[3].y;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 4 );
	} else {
		mTVertex = &mVertex[mNumVertex];
		mTVertex->pos.x = mQ[1].x;
		mTVertex->pos.y = mQ[1].y;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 1];
		mTVertex->pos.x = mQ[0].x;
		mTVertex->pos.y = mQ[0].y;
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		mTVertex = &mVertex[mNumVertex + 2];
		mTVertex->pos.x = mQ[3].x;
		mTVertex->pos.y = mQ[3].y;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		mTVertex = &mVertex[mNumVertex + 3];
		mTVertex->pos.x = mQ[1].x;
		mTVertex->pos.y = mQ[1].y;
		mTVertex->tex = mTexCoord[1];
		mTVertex->color = mVerColor[1];

		mTVertex = &mVertex[mNumVertex + 4];
		mTVertex->pos.x = mQ[2].x;
		mTVertex->pos.y = mQ[2].y;
		mTVertex->tex = mTexCoord[2];
		mTVertex->color = mVerColor[2];

		mTVertex = &mVertex[mNumVertex + 5];
		mTVertex->pos.x = mQ[3].x;
		mTVertex->pos.y = mQ[3].y;
		mTVertex->tex = mTexCoord[3];
		mTVertex->color = mVerColor[3];

		addVertices( 6 );
	}
}

void BatchRenderer::quadsBegin() {
	setDrawMode( PRIMITIVE_QUADS, true );
	quadsSetTexCoord( 0, 0, 1, 1 );
	quadsSetColor( Color::White );
}

void BatchRenderer::quadsSetColor( const Color& Color ) {
	mVerColor[0] = mVerColor[1] = mVerColor[2] = mVerColor[3] = Color;
}

void BatchRenderer::quadsSetColorFree( const Color& Color0, const Color& Color1,
									   const Color& Color2, const Color& Color3 ) {
	mVerColor[0] = Color0;
	mVerColor[1] = Color1;
	mVerColor[2] = Color2;
	mVerColor[3] = Color3;
}

void BatchRenderer::quadsSetTexCoord( const Float& tl_u, const Float& tl_v, const Float& br_u,
									  const Float& br_v ) {
	mTexCoord[0].x = tl_u;
	mTexCoord[0].y = tl_v;
	mTexCoord[1].x = tl_u;
	mTexCoord[1].y = br_v;

	mTexCoord[2].x = br_u;
	mTexCoord[2].y = br_v;
	mTexCoord[3].x = br_u;
	mTexCoord[3].y = tl_v;
}

void BatchRenderer::quadsSetTexCoord( const Rectf& region ) {
	quadsSetTexCoord( region.Left, region.Top, region.Right, region.Bottom );
}

void BatchRenderer::quadsSetTexCoordFree( const Float& x0, const Float& y0, const Float& x1,
										  const Float& y1, const Float& x2, const Float& y2,
										  const Float& x3, const Float& y3 ) {
	mTexCoord[0].x = x0;
	mTexCoord[0].y = y0;
	mTexCoord[1].x = x1;
	mTexCoord[1].y = y1;
	mTexCoord[2].x = x2;
	mTexCoord[2].y = y2;
	mTexCoord[3].x = x3;
	mTexCoord[3].y = y3;
}

void BatchRenderer::rotate( const Vector2f& center, Vector2f* point, const Float& angle ) {
	if ( angle ) {
		Float x = point->x - center.x;
		Float y = point->y - center.y;
		point->x = x * Math::cosAng( angle ) - y * Math::sinAng( angle ) + center.x;
		point->y = x * Math::sinAng( angle ) + y * Math::cosAng( angle ) + center.y;
	}
}

void BatchRenderer::pointsBegin() {
	setDrawMode( PRIMITIVE_POINTS, true );
	quadsSetTexCoord( 0, 0, 1, 1 );
	pointSetColor( Color::White );
}

void BatchRenderer::pointSetColor( const Color& color ) {
	mVerColor[0] = color;
}

void BatchRenderer::pointSetTexCoord( const Float& x, const Float& y ) {
	mTexCoord[0].x = x;
	mTexCoord[0].y = y;
}

void BatchRenderer::batchPoint( const Float& x, const Float& y,
								const PrimitiveType& primitiveType ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( primitiveType, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertices( 1 );
}

void BatchRenderer::batchPointList( const std::vector<VertexData>& points,
									const PrimitiveType& primitiveType ) {
	setDrawMode( primitiveType, mForceBlendMode );

	unsigned int curNumVertex = mNumVertex;

	addVertices( points.size() );

	memcpy( (void*)&mVertex[curNumVertex], (void*)&points[0],
			sizeof( VertexData ) * points.size() );
}

void BatchRenderer::linesBegin() {
	setDrawMode( PRIMITIVE_LINES, true );
	quadsSetTexCoord( 0, 0, 1, 1 );
	pointSetColor( Color::White );
}

void BatchRenderer::linesSetColor( const Color& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::linesSetColorFree( const Color& Color0, const Color& Color1 ) {
	quadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void BatchRenderer::batchLine( const Float& x0, const Float& y0, const Float& x1,
							   const Float& y1 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_LINES, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex = &mVertex[mNumVertex + 1];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex = mTexCoord[1];
	mTVertex->color = mVerColor[1];

	addVertices( 2 );
}

void BatchRenderer::lineLoopBegin() {
	setDrawMode( PRIMITIVE_LINE_LOOP, true );
	quadsSetTexCoord( 0, 0, 1, 1 );
	pointSetColor( Color::White );
}

void BatchRenderer::lineLoopSetColor( const Color& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::lineLoopSetColorFree( const Color& Color0, const Color& Color1 ) {
	quadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void BatchRenderer::batchLineLoop( const Float& x0, const Float& y0, const Float& x1,
								   const Float& y1 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_LINE_LOOP, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex = &mVertex[mNumVertex + 1];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex = mTexCoord[1];
	mTVertex->color = mVerColor[1];

	addVertices( 2 );
}

void BatchRenderer::batchLineLoop( const Vector2f& vector1, const Vector2f& vector2 ) {
	batchLineLoop( vector1.x, vector1.y, vector2.x, vector2.y );
}

void BatchRenderer::batchLineLoop( const Float& x0, const Float& y0 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_LINE_LOOP, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertices( 1 );
}

void BatchRenderer::batchLineLoop( const Vector2f& vector1 ) {
	batchLineLoop( vector1.x, vector1.y );
}

void BatchRenderer::lineStripBegin() {
	setDrawMode( PRIMITIVE_LINE_STRIP, true );
	quadsSetTexCoord( 0, 0, 1, 1 );
	pointSetColor( Color::White );
}

void BatchRenderer::lineStripSetColor( const Color& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::lineStripSetColorFree( const Color& Color0, const Color& Color1 ) {
	quadsSetColorFree( Color0, Color1, Color0, Color0 );
}

void BatchRenderer::batchLineStrip( const Float& x0, const Float& y0, const Float& x1,
									const Float& y1 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_LINE_STRIP, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex = &mVertex[mNumVertex + 1];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex = mTexCoord[1];
	mTVertex->color = mVerColor[1];

	addVertices( 2 );
}

void BatchRenderer::batchLineStrip( const Vector2f& vector1, const Vector2f& vector2 ) {
	batchLineStrip( vector1.x, vector1.y, vector2.x, vector2.y );
}

void BatchRenderer::batchLineStrip( const Float& x0, const Float& y0 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_LINE_STRIP, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertices( 1 );
}

void BatchRenderer::batchLineStrip( const Vector2f& vector1 ) {
	batchLineStrip( vector1.x, vector1.y );
}

void BatchRenderer::triangleFanBegin() {
	setDrawMode( PRIMITIVE_TRIANGLE_FAN, true );
	triangleFanSetTexCoord( 0, 0, 0, 1, 1, 1 );
	triangleFanSetColor( Color::White );
}

void BatchRenderer::triangleFanSetColor( const Color& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::triangleFanSetColorFree( const Color& Color0, const Color& Color1,
											 const Color& Color2 ) {
	quadsSetColorFree( Color0, Color1, Color2, Color0 );
}

void BatchRenderer::triangleFanSetTexCoord( const Float& x0, const Float& y0, const Float& x1,
											const Float& y1, const Float& x2, const Float& y2 ) {
	mTexCoord[0].x = x0;
	mTexCoord[0].y = y0;
	mTexCoord[1].x = x1;
	mTexCoord[1].y = y1;
	mTexCoord[2].x = x2;
	mTexCoord[2].y = y2;
}

void BatchRenderer::batchTriangleFan( const Float& x0, const Float& y0, const Float& x1,
									  const Float& y1, const Float& x2, const Float& y2 ) {

	if ( mNumVertex + 3 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_TRIANGLE_FAN, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex = &mVertex[mNumVertex + 1];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex = mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex = &mVertex[mNumVertex + 2];
	mTVertex->pos.x = x2;
	mTVertex->pos.y = y2;
	mTVertex->tex = mTexCoord[2];
	mTVertex->color = mVerColor[2];

	addVertices( 3 );
}

void BatchRenderer::batchTriangleFan( const Float& x0, const Float& y0 ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_TRIANGLE_FAN, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertices( 1 );
}

void BatchRenderer::trianglesBegin() {
	setDrawMode( PRIMITIVE_TRIANGLES, true );
	trianglesSetTexCoord( 0, 0, 0, 1, 1, 1 );
	trianglesSetColor( Color::White );
}

void BatchRenderer::trianglesSetColor( const Color& Color ) {
	quadsSetColor( Color );
}

void BatchRenderer::trianglesSetColorFree( const Color& Color0, const Color& Color1,
										   const Color& Color2 ) {
	quadsSetColorFree( Color0, Color1, Color2, Color0 );
}

void BatchRenderer::trianglesSetTexCoord( const Float& x0, const Float& y0, const Float& x1,
										  const Float& y1, const Float& x2, const Float& y2 ) {
	mTexCoord[0].x = x0;
	mTexCoord[0].y = y0;
	mTexCoord[1].x = x1;
	mTexCoord[1].y = y1;
	mTexCoord[2].x = x2;
	mTexCoord[2].y = y2;
}

void BatchRenderer::batchTriangle( const Float& x0, const Float& y0, const Float& x1,
								   const Float& y1, const Float& x2, const Float& y2 ) {

	if ( mNumVertex + 2 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_TRIANGLES, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x0;
	mTVertex->pos.y = y0;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	mTVertex = &mVertex[mNumVertex + 1];
	mTVertex->pos.x = x1;
	mTVertex->pos.y = y1;
	mTVertex->tex = mTexCoord[1];
	mTVertex->color = mVerColor[1];

	mTVertex = &mVertex[mNumVertex + 2];
	mTVertex->pos.x = x2;
	mTVertex->pos.y = y2;
	mTVertex->tex = mTexCoord[2];
	mTVertex->color = mVerColor[2];

	addVertices( 3 );
}

void BatchRenderer::polygonSetColor( const Color& Color ) {
	pointSetColor( Color );
}

void BatchRenderer::batchPolygon( const Polygon2f& Polygon ) {
	if ( Polygon.getSize() > mVertexSize )
		return;

	setDrawMode( PRIMITIVE_POLYGON, mForceBlendMode );

	for ( Uint32 i = 0; i < Polygon.getSize(); i++ ) {
		mTVertex = &mVertex[mNumVertex];

		mTVertex->pos = Polygon.getPosition() + Polygon[i];
		mTVertex->tex = mTexCoord[0];
		mTVertex->color = mVerColor[0];

		addVertices( 1 );
	}
}

void BatchRenderer::batchPolygonByPoint( const Float& x, const Float& y ) {
	if ( mNumVertex + 1 >= mVertexSize )
		return;

	setDrawMode( PRIMITIVE_POLYGON, mForceBlendMode );

	mTVertex = &mVertex[mNumVertex];
	mTVertex->pos.x = x;
	mTVertex->pos.y = y;
	mTVertex->tex = mTexCoord[0];
	mTVertex->color = mVerColor[0];

	addVertices( 1 );
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

}} // namespace EE::Graphics
