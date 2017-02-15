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

void Primitives::DrawPoint( const Vector2f& p, const Float& pointSize ) {
	sBR->SetPointSize( pointSize );

	sBR->SetTexture( NULL );
	sBR->PointsBegin();
	sBR->PointSetColor( mColor );

	sBR->BatchPoint( p.x, p.y );

	DrawBatch();
}

void Primitives::DrawLine( const Line2f& line ) {
	sBR->SetLineWidth( mLineWidth );

	sBR->SetTexture( NULL );
	sBR->LinesBegin();
	sBR->LinesSetColor( mColor );

	sBR->BatchLine( line.V[0].x, line.V[0].y, line.V[1].x, line.V[1].y );

	DrawBatch();
}

void Primitives::DrawTriangle( const Triangle2f& t, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3 ) {
	sBR->SetTexture( NULL );
	sBR->SetBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );

			sBR->LineLoopBegin();

			sBR->LineLoopSetColorFree( Color1, Color2 );
			sBR->BatchLineLoop( t.V[0].x, t.V[0].y, t.V[1].x, t.V[1].y );
			sBR->LineLoopSetColorFree( Color2, Color3 );
			sBR->BatchLineLoop( t.V[1].x, t.V[1].y, t.V[2].x, t.V[2].y );
			break;
		}
		default:
		case DRAW_FILL:
		{
			sBR->TrianglesBegin();

			sBR->TrianglesSetColorFree( Color1, Color2, Color3 );
			sBR->BatchTriangle( t.V[0].x, t.V[0].y, t.V[1].x, t.V[1].y, t.V[2].x, t.V[2].y );

			break;
		}
	}

	DrawBatch();
}

void Primitives::DrawTriangle( const Triangle2f& t ) {
	DrawTriangle( t, mColor, mColor, mColor );
}

void Primitives::DrawCircle( const Vector2f& p, const Float& radius, Uint32 segmentsCount ) {
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

		GLi->Disable( GL_TEXTURE_2D );

		GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );

		GLi->PushMatrix();

		GLi->Translatef( p.x, p.y, 0.0f );

		GLi->Scalef( radius, radius, 1.0f);

		GLi->VertexPointer( 2, GL_FLOAT, 0, circleVAR, circleVAR_count * sizeof(float) * 2 );

		std::vector<ColorA> colors( circleVAR_count - 1 ,mColor );

		GLi->ColorPointer( 4, GL_UNSIGNED_BYTE, 0, &colors[0], circleVAR_count * 4 );

		switch( mFillMode ) {
			case DRAW_LINE:
			{
				GLi->DrawArrays( GL_LINE_LOOP, 0, circleVAR_count - 1 );
				break;
			}
			case DRAW_FILL:
			{
				GLi->DrawArrays( GL_TRIANGLE_FAN, 0, circleVAR_count - 1 );
				break;
			}
		}

		GLi->PopMatrix();

		GLi->Enable( GL_TEXTURE_2D );

		GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );

		return;
	}

	DrawArc( p, radius, segmentsCount, 360 );
}

void Primitives::DrawArc( const Vector2f& p, const Float& radius, Uint32 segmentsCount, const Float& arcAngle, const Float& arcStartAngle ) {
	if(segmentsCount < 6) segmentsCount = 6;
	segmentsCount = segmentsCount > 360 ? 360 : segmentsCount;

	Float angle_shift =  360 / static_cast<Float>(segmentsCount);
	Float arcAngleA = arcAngle > 360 ? arcAngle - 360 * std::floor( arcAngle / 360 ) : arcAngle;

	sBR->SetTexture( NULL );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );

			segmentsCount = Uint32( (Float)segmentsCount * (Float)eeabs( arcAngleA ) / 360 );
			Float startAngle = Math::radians(arcStartAngle);
			Float theta = Math::radians(arcAngleA) / Float(segmentsCount - 1);
			Float tangetialFactor = eetan(theta);
			Float radialFactor = eecos(theta);
			Float x = radius * eecos(startAngle);
			Float y = radius * eesin(startAngle);

			sBR->LineStripBegin();
			sBR->LineStripSetColor( mColor );

			for( Uint32 ii = 0; ii < segmentsCount; ii++ ) {
				sBR->BatchLineStrip(x + p.x, y + p.y);

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
			sBR->TriangleFanBegin();
			sBR->TriangleFanSetColor( mColor );

			for( Float i = 0; i < arcAngleA; i+= angle_shift ) {
				Float startAngle = arcStartAngle + i;

				sBR->BatchTriangleFan( p.x , p.y,
									   p.x + radius * Math::sinAng( startAngle ), p.y + radius * Math::cosAng( startAngle ),
									   p.x + radius * Math::sinAng( startAngle + angle_shift ), p.y + radius * Math::cosAng( startAngle + angle_shift ) );
			}

			break;
		}
	}

	DrawBatch();
}

void Primitives::DrawRectangle( const Rectf& R, const ColorA& TopLeft, const ColorA& BottomLeft, const ColorA& BottomRight, const ColorA& TopRight, const Float& Angle, const Vector2f& Scale ) {
	sBR->SetTexture( NULL );
	sBR->SetBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_FILL:
		{
			sBR->QuadsBegin();
			sBR->QuadsSetColorFree( TopLeft, BottomLeft, BottomRight, TopRight );

			Sizef size = const_cast<Rectf*>(&R)->size();

			sBR->BatchQuadEx( R.Left, R.Top, size.width(), size.height(), Angle, Scale );
			break;
		}
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );

			sBR->LineLoopBegin();
			sBR->LineLoopSetColorFree( TopLeft, BottomLeft );

			if ( Scale != 1.0f || Angle != 0.0f ) {
				Quad2f Q( R );
				Sizef size = const_cast<Rectf*>(&R)->size();

				Q.scale( Scale );
				Q.rotate( Angle, Vector2f( R.Left + size.width() * 0.5f, R.Top + size.height() * 0.5f ) );

				sBR->BatchLineLoop( Q[0].x, Q[0].y, Q[1].x, Q[1].y );
				sBR->LineLoopSetColorFree( BottomRight, TopRight );
				sBR->BatchLineLoop( Q[2].x, Q[2].y, Q[3].x, Q[3].y );
			} else {
				sBR->BatchLineLoop( R.Left, R.Top, R.Left, R.Bottom );
				sBR->LineLoopSetColorFree( BottomRight, TopRight );
				sBR->BatchLineLoop( R.Right, R.Bottom, R.Right, R.Top );
			}

			break;
		}
	}

	DrawBatch();
}

void Primitives::DrawRectangle( const Rectf& R, const Float& Angle, const Vector2f& Scale ) {
	DrawRectangle( R, mColor, mColor, mColor, mColor, Angle, Scale );
}

void Primitives::DrawRoundedRectangle( const Rectf& R, const ColorA& TopLeft, const ColorA& BottomLeft, const ColorA& BottomRight, const ColorA& TopRight, const Float& Angle, const Vector2f& Scale, const unsigned int& Corners ) {
	sBR->SetTexture( NULL );
	sBR->SetBlendMode( mBlendMode );

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
				sBR->PolygonSetColor( TopLeft );

				sBR->BatchPolygon( Poly );
			} else {
				for ( i = 0; i < Poly.size(); i++ ) {
					poly = Poly[i];

					if ( poly.x <= Center.x && poly.y <= Center.y )
						sBR->PolygonSetColor( TopLeft );
					else if ( poly.x <= Center.x && poly.y >= Center.y )
						sBR->PolygonSetColor( BottomLeft );
					else if ( poly.x > Center.x && poly.y > Center.y )
						sBR->PolygonSetColor( BottomRight );
					else if ( poly.x > Center.x && poly.y < Center.y )
						sBR->PolygonSetColor( TopRight );
					else
						sBR->PolygonSetColor( TopLeft );

					sBR->BatchPolygonByPoint( Poly[i] );
				}
			}

			break;
		}
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );

			sBR->LineLoopBegin();
			sBR->LineLoopSetColor( TopLeft );

			if ( TopLeft == BottomLeft && BottomLeft == BottomRight && BottomRight == TopRight ) {
				for ( i = 0; i < Poly.size(); i+=2 ) {
					sBR->BatchLineLoop( Poly[i], Poly[i+1] );
				}
			} else {
				for ( unsigned int i = 0; i < Poly.size(); i++ ) {
					poly = Poly[i];

					if ( poly.x <= Center.x && poly.y <= Center.y )
						sBR->LineLoopSetColor( TopLeft );
					else if ( poly.x < Center.x && poly.y > Center.y )
						sBR->LineLoopSetColor( BottomLeft );
					else if ( poly.x > Center.x && poly.y > Center.y )
						sBR->LineLoopSetColor( BottomRight );
					else if ( poly.x > Center.x && poly.y < Center.y )
						sBR->LineLoopSetColor( TopRight );
					else
						sBR->LineLoopSetColor( TopLeft );

					sBR->BatchLineLoop( Poly[i] );
				}
			}

			break;
		}
	}

	DrawBatch();
}

void Primitives::DrawRoundedRectangle( const Rectf& R, const Float& Angle, const Vector2f& Scale, const unsigned int& Corners ) {
	DrawRoundedRectangle( R, mColor, mColor, mColor, mColor, Angle, Scale, Corners );
}

void Primitives::DrawQuad( const Quad2f& q, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const ColorA& Color4, const Float& OffsetX, const Float& OffsetY ) {
	sBR->SetTexture( NULL );
	sBR->SetBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );

			sBR->LineLoopBegin();
			sBR->LineLoopSetColorFree( Color1, Color2 );
			sBR->BatchLineLoop( OffsetX + q[0].x, OffsetY + q[0].y, OffsetX + q[1].x, OffsetY + q[1].y );
			sBR->LineLoopSetColorFree( Color2, Color3 );
			sBR->BatchLineLoop( OffsetX + q[2].x, OffsetY + q[2].y, OffsetX + q[3].x, OffsetY + q[3].y );
			break;
		}
		case DRAW_FILL:
		{
			sBR->QuadsBegin();
			sBR->QuadsSetColorFree( Color1, Color2, Color3, Color4 );
			sBR->BatchQuadFree( OffsetX + q[0].x, OffsetY + q[0].y, OffsetX + q[1].x, OffsetY + q[1].y, OffsetX + q[2].x, OffsetY + q[2].y, OffsetX + q[3].x, OffsetY + q[3].y );
			break;
		}
	}

	DrawBatch();
}

void Primitives::DrawQuad( const Quad2f& q, const Float& OffsetX, const Float& OffsetY ) {
	DrawQuad( q, mColor, mColor, mColor, mColor, OffsetX, OffsetY );
}

void Primitives::DrawPolygon( const Polygon2f& p ) {
	sBR->SetTexture( NULL );
	sBR->SetBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );

			sBR->LineLoopBegin();
			sBR->LineLoopSetColor( mColor );

			for ( Uint32 i = 0; i < p.size(); i += 2 )
				sBR->BatchLineLoop( p.x() + p[i].x, p.y() + p[i].y, p.x() + p[i+1].x, p.y() + p[i+1].y );

			break;
		}
		case DRAW_FILL:
		{
			sBR->PolygonSetColor( mColor );
			sBR->BatchPolygon( p );
			break;
		}
	}

	DrawBatch();
}

void Primitives::DrawBatch() {
	if ( mForceDraw )
		sBR->Draw();
	else
		sBR->DrawOpt();
}

void Primitives::ForceDraw( const bool& force ) {
	mForceDraw = force;

	if ( force )
		DrawBatch();
}

const bool& Primitives::ForceDraw() const {
	return mForceDraw;
}

void Primitives::SetColor( const ColorA& Color ) {
	mColor = Color;
}

void Primitives::FillMode( const EE_FILL_MODE& Mode ) {
	mFillMode = Mode;
}

const EE_FILL_MODE& Primitives::FillMode() const {
	return mFillMode;
}

void Primitives::BlendMode( const EE_BLEND_MODE& Mode ) {
	mBlendMode = Mode;
}

const EE_BLEND_MODE& Primitives::BlendMode() const {
	return mBlendMode;
}

void Primitives::LineWidth( const Float& width ) {
	mLineWidth = width;
}

const Float& Primitives::LineWidth() const {
	return mLineWidth;
}

}}
