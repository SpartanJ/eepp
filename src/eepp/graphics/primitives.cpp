#include <eepp/graphics/primitives.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/graphics/batchrenderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace Graphics {

static GlobalBatchRenderer * sBR = NULL;

Primitives::Primitives() :
	mFillMode( DRAW_FILL ),
	mBlendMode( ALPHA_NORMAL ),
	mLineWidth( 1.f ),
	mForceDraw( true )
{
	if ( NULL == sBR ) {
		sBR = GlobalBatchRenderer::instance();
	}
}

Primitives::~Primitives() {
}

void Primitives::drawPoint( const Vector2f& p, const Float& pointSize ) {
	sBR->setPointSize( pointSize );

	sBR->setTexture( NULL );
	sBR->pointsBegin();
	sBR->pointSetColor( mColor );

	sBR->batchPoint( p.x, p.y );

	drawBatch();
}

void Primitives::drawLine( const Line2f& line ) {
	sBR->setLineWidth( mLineWidth );

	sBR->setTexture( NULL );
	sBR->linesBegin();
	sBR->linesSetColor( mColor );

	sBR->batchLine( line.V[0].x, line.V[0].y, line.V[1].x, line.V[1].y );

	drawBatch();
}

void Primitives::drawTriangle( const Triangle2f& t, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3 ) {
	sBR->setTexture( NULL );
	sBR->setBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->setLineWidth( mLineWidth );

			sBR->lineLoopBegin();

			sBR->lineLoopSetColorFree( Color1, Color2 );
			sBR->batchLineLoop( t.V[0].x, t.V[0].y, t.V[1].x, t.V[1].y );
			sBR->lineLoopSetColorFree( Color2, Color3 );
			sBR->batchLineLoop( t.V[1].x, t.V[1].y, t.V[2].x, t.V[2].y );
			break;
		}
		default:
		case DRAW_FILL:
		{
			sBR->trianglesBegin();

			sBR->trianglesSetColorFree( Color1, Color2, Color3 );
			sBR->batchTriangle( t.V[0].x, t.V[0].y, t.V[1].x, t.V[1].y, t.V[2].x, t.V[2].y );

			break;
		}
	}

	drawBatch();
}

void Primitives::drawTriangle( const Triangle2f& t ) {
	drawTriangle( t, mColor, mColor, mColor );
}

void Primitives::drawCircle( const Vector2f& p, const Float& radius, Uint32 segmentsCount ) {
	if ( 0 == segmentsCount ) {
		// Optimized circle rendering
		static const float circleVAR[] = {
			 0.0000f,  1.0000f,
			 0.2588f,  0.9659f,
			 0.5000f,  0.8660f,
			 0.7071f,  0.7071f,
			 0.8660f,  0.5000f,
			 0.9659f,  0.2588f,
			 1.0000f,  0.0000f,
			 0.9659f, -0.2588f,
			 0.8660f, -0.5000f,
			 0.7071f, -0.7071f,
			 0.5000f, -0.8660f,
			 0.2588f, -0.9659f,
			 0.0000f, -1.0000f,
			-0.2588f, -0.9659f,
			-0.5000f, -0.8660f,
			-0.7071f, -0.7071f,
			-0.8660f, -0.5000f,
			-0.9659f, -0.2588f,
			-1.0000f, -0.0000f,
			-0.9659f,  0.2588f,
			-0.8660f,  0.5000f,
			-0.7071f,  0.7071f,
			-0.5000f,  0.8660f,
			-0.2588f,  0.9659f,
			 0.0000f,  1.0000f,
			 0.0f, 0.0f, // For an extra line to see the rotation.
		};
		static const int circleVAR_count = sizeof(circleVAR)/sizeof(float)/2;

		GLi->disable( GL_TEXTURE_2D );

		GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );

		GLi->pushMatrix();

		GLi->translatef( p.x, p.y, 0.0f );

		GLi->scalef( radius, radius, 1.0f);

		GLi->vertexPointer( 2, GL_FLOAT, 0, circleVAR, circleVAR_count * sizeof(float) * 2 );

		std::vector<ColorA> colors( circleVAR_count - 1 ,mColor );

		GLi->colorPointer( 4, GL_UNSIGNED_BYTE, 0, &colors[0], circleVAR_count * 4 );

		switch( mFillMode ) {
			case DRAW_LINE:
			{
				GLi->drawArrays( GL_LINE_LOOP, 0, circleVAR_count - 1 );
				break;
			}
			case DRAW_FILL:
			{
				GLi->drawArrays( GL_TRIANGLE_FAN, 0, circleVAR_count - 1 );
				break;
			}
		}

		GLi->popMatrix();

		GLi->enable( GL_TEXTURE_2D );

		GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );

		return;
	}

	drawArc( p, radius, segmentsCount, 360 );
}

void Primitives::drawArc( const Vector2f& p, const Float& radius, Uint32 segmentsCount, const Float& arcAngle, const Float& arcStartAngle ) {
	if(segmentsCount < 6) segmentsCount = 6;
	segmentsCount = segmentsCount > 360 ? 360 : segmentsCount;

	Float angle_shift =  360 / static_cast<Float>(segmentsCount);
	Float arcAngleA = arcAngle > 360 ? arcAngle - 360 * std::floor( arcAngle / 360 ) : arcAngle;

	sBR->setTexture( NULL );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->setLineWidth( mLineWidth );

			segmentsCount = Uint32( (Float)segmentsCount * (Float)eeabs( arcAngleA ) / 360 );
			Float startAngle = Math::radians(arcStartAngle);
			Float theta = Math::radians(arcAngleA) / Float(segmentsCount - 1);
			Float tangetialFactor = eetan(theta);
			Float radialFactor = eecos(theta);
			Float x = radius * eecos(startAngle);
			Float y = radius * eesin(startAngle);

			sBR->lineStripBegin();
			sBR->lineStripSetColor( mColor );

			for( Uint32 ii = 0; ii < segmentsCount; ii++ ) {
				sBR->batchLineStrip(x + p.x, y + p.y);

				Float tx = -y;
				Float ty = x;

				x += tx * tangetialFactor;
				y += ty * tangetialFactor;

				x *= radialFactor;
				y *= radialFactor;
			}

			break;
		}
		case DRAW_FILL:
		{
			sBR->triangleFanBegin();
			sBR->triangleFanSetColor( mColor );

			for( Float i = 0; i < arcAngleA; i+= angle_shift ) {
				Float startAngle = arcStartAngle + i;

				sBR->batchTriangleFan( p.x , p.y,
									   p.x + radius * Math::sinAng( startAngle ), p.y + radius * Math::cosAng( startAngle ),
									   p.x + radius * Math::sinAng( startAngle + angle_shift ), p.y + radius * Math::cosAng( startAngle + angle_shift ) );
			}

			break;
		}
	}

	drawBatch();
}

void Primitives::drawRectangle( const Rectf& R, const ColorA& TopLeft, const ColorA& BottomLeft, const ColorA& BottomRight, const ColorA& TopRight, const Float& Angle, const Vector2f& Scale ) {
	sBR->setTexture( NULL );
	sBR->setBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_FILL:
		{
			sBR->quadsBegin();
			sBR->quadsSetColorFree( TopLeft, BottomLeft, BottomRight, TopRight );

			Sizef size = const_cast<Rectf*>(&R)->size();

			sBR->batchQuadEx( R.Left, R.Top, size.width(), size.height(), Angle, Scale );
			break;
		}
		case DRAW_LINE:
		{
			sBR->setLineWidth( mLineWidth );

			sBR->lineLoopBegin();
			sBR->lineLoopSetColorFree( TopLeft, BottomLeft );

			if ( Scale != 1.0f || Angle != 0.0f ) {
				Quad2f Q( R );
				Sizef size = const_cast<Rectf*>(&R)->size();

				Q.scale( Scale );
				Q.rotate( Angle, Vector2f( R.Left + size.width() * 0.5f, R.Top + size.height() * 0.5f ) );

				sBR->batchLineLoop( Q[0].x, Q[0].y, Q[1].x, Q[1].y );
				sBR->lineLoopSetColorFree( BottomRight, TopRight );
				sBR->batchLineLoop( Q[2].x, Q[2].y, Q[3].x, Q[3].y );
			} else {
				sBR->batchLineLoop( R.Left, R.Top, R.Left, R.Bottom );
				sBR->lineLoopSetColorFree( BottomRight, TopRight );
				sBR->batchLineLoop( R.Right, R.Bottom, R.Right, R.Top );
			}

			break;
		}
	}

	drawBatch();
}

void Primitives::drawRectangle( const Rectf& R, const Float& Angle, const Vector2f& Scale ) {
	drawRectangle( R, mColor, mColor, mColor, mColor, Angle, Scale );
}

void Primitives::drawRoundedRectangle( const Rectf& R, const ColorA& TopLeft, const ColorA& BottomLeft, const ColorA& BottomRight, const ColorA& TopRight, const Float& Angle, const Vector2f& Scale, const unsigned int& Corners ) {
	sBR->setTexture( NULL );
	sBR->setBlendMode( mBlendMode );

	unsigned int i;
	Sizef size		= const_cast<Rectf*>( &R )->size();
	Float xscalediff	= size.width()	* Scale.x - size.width();
	Float yscalediff	= size.height()	* Scale.y - size.height();
	Vector2f Center( R.Left + size.width() * 0.5f + xscalediff, R.Top + size.height() * 0.5f + yscalediff );
	Polygon2f Poly	= Polygon2f::createRoundedRectangle( R.Left - xscalediff, R.Top - yscalediff, size.width() + xscalediff, size.height() + yscalediff, Corners );
	Vector2f poly;

	Poly.rotate( Angle, Center );

	switch( mFillMode ) {
		case DRAW_FILL:
		{
			if ( TopLeft == BottomLeft && BottomLeft == BottomRight && BottomRight == TopRight ) {
				sBR->polygonSetColor( TopLeft );

				sBR->batchPolygon( Poly );
			} else {
				for ( i = 0; i < Poly.size(); i++ ) {
					poly = Poly[i];

					if ( poly.x <= Center.x && poly.y <= Center.y )
						sBR->polygonSetColor( TopLeft );
					else if ( poly.x <= Center.x && poly.y >= Center.y )
						sBR->polygonSetColor( BottomLeft );
					else if ( poly.x > Center.x && poly.y > Center.y )
						sBR->polygonSetColor( BottomRight );
					else if ( poly.x > Center.x && poly.y < Center.y )
						sBR->polygonSetColor( TopRight );
					else
						sBR->polygonSetColor( TopLeft );

					sBR->batchPolygonByPoint( Poly[i] );
				}
			}

			break;
		}
		case DRAW_LINE:
		{
			sBR->setLineWidth( mLineWidth );

			sBR->lineLoopBegin();
			sBR->lineLoopSetColor( TopLeft );

			if ( TopLeft == BottomLeft && BottomLeft == BottomRight && BottomRight == TopRight ) {
				for ( i = 0; i < Poly.size(); i+=2 ) {
					sBR->batchLineLoop( Poly[i], Poly[i+1] );
				}
			} else {
				for ( unsigned int i = 0; i < Poly.size(); i++ ) {
					poly = Poly[i];

					if ( poly.x <= Center.x && poly.y <= Center.y )
						sBR->lineLoopSetColor( TopLeft );
					else if ( poly.x < Center.x && poly.y > Center.y )
						sBR->lineLoopSetColor( BottomLeft );
					else if ( poly.x > Center.x && poly.y > Center.y )
						sBR->lineLoopSetColor( BottomRight );
					else if ( poly.x > Center.x && poly.y < Center.y )
						sBR->lineLoopSetColor( TopRight );
					else
						sBR->lineLoopSetColor( TopLeft );

					sBR->batchLineLoop( Poly[i] );
				}
			}

			break;
		}
	}

	drawBatch();
}

void Primitives::drawRoundedRectangle( const Rectf& R, const Float& Angle, const Vector2f& Scale, const unsigned int& Corners ) {
	drawRoundedRectangle( R, mColor, mColor, mColor, mColor, Angle, Scale, Corners );
}

void Primitives::drawQuad( const Quad2f& q, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const ColorA& Color4, const Float& OffsetX, const Float& OffsetY ) {
	sBR->setTexture( NULL );
	sBR->setBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->setLineWidth( mLineWidth );

			sBR->lineLoopBegin();
			sBR->lineLoopSetColorFree( Color1, Color2 );
			sBR->batchLineLoop( OffsetX + q[0].x, OffsetY + q[0].y, OffsetX + q[1].x, OffsetY + q[1].y );
			sBR->lineLoopSetColorFree( Color2, Color3 );
			sBR->batchLineLoop( OffsetX + q[2].x, OffsetY + q[2].y, OffsetX + q[3].x, OffsetY + q[3].y );
			break;
		}
		case DRAW_FILL:
		{
			sBR->quadsBegin();
			sBR->quadsSetColorFree( Color1, Color2, Color3, Color4 );
			sBR->batchQuadFree( OffsetX + q[0].x, OffsetY + q[0].y, OffsetX + q[1].x, OffsetY + q[1].y, OffsetX + q[2].x, OffsetY + q[2].y, OffsetX + q[3].x, OffsetY + q[3].y );
			break;
		}
	}

	drawBatch();
}

void Primitives::drawQuad( const Quad2f& q, const Float& OffsetX, const Float& OffsetY ) {
	drawQuad( q, mColor, mColor, mColor, mColor, OffsetX, OffsetY );
}

void Primitives::drawPolygon( const Polygon2f& p ) {
	sBR->setTexture( NULL );
	sBR->setBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->setLineWidth( mLineWidth );

			sBR->lineLoopBegin();
			sBR->lineLoopSetColor( mColor );

			for ( Uint32 i = 0; i < p.size(); i += 2 )
				sBR->batchLineLoop( p.x() + p[i].x, p.y() + p[i].y, p.x() + p[i+1].x, p.y() + p[i+1].y );

			break;
		}
		case DRAW_FILL:
		{
			sBR->polygonSetColor( mColor );
			sBR->batchPolygon( p );
			break;
		}
	}

	drawBatch();
}

void Primitives::drawBatch() {
	if ( mForceDraw )
		sBR->draw();
	else
		sBR->drawOpt();
}

void Primitives::forceDraw( const bool& force ) {
	mForceDraw = force;

	if ( force )
		drawBatch();
}

const bool& Primitives::forceDraw() const {
	return mForceDraw;
}

void Primitives::setColor( const ColorA& Color ) {
	mColor = Color;
}

void Primitives::fillMode( const EE_FILL_MODE& Mode ) {
	mFillMode = Mode;
}

const EE_FILL_MODE& Primitives::fillMode() const {
	return mFillMode;
}

void Primitives::blendMode( const EE_BLEND_MODE& Mode ) {
	mBlendMode = Mode;
}

const EE_BLEND_MODE& Primitives::blendMode() const {
	return mBlendMode;
}

void Primitives::lineWidth( const Float& width ) {
	mLineWidth = width;
}

const Float& Primitives::lineWidth() const {
	return mLineWidth;
}

}}
