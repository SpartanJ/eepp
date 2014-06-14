#include <eepp/graphics/cprimitives.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/graphics/cbatchrenderer.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/cgl.hpp>

namespace EE { namespace Graphics {

static cGlobalBatchRenderer * sBR = NULL;

cPrimitives::cPrimitives() :
	mFillMode( DRAW_FILL ),
	mBlendMode( ALPHA_NORMAL ),
	mLineWidth( 1.f ),
	mForceDraw( true )
{
	if ( NULL == sBR ) {
		sBR = cGlobalBatchRenderer::instance();
	}
}

cPrimitives::~cPrimitives() {
}

void cPrimitives::DrawPoint( const eeVector2f& p, const Float& pointSize ) {
	sBR->SetPointSize( pointSize );

	sBR->SetTexture( NULL );
	sBR->PointsBegin();
	sBR->PointSetColor( mColor );

	sBR->BatchPoint( p.x, p.y );

	DrawBatch();
}

void cPrimitives::DrawLine( const eeLine2f& line ) {
	sBR->SetLineWidth( mLineWidth );

	sBR->SetTexture( NULL );
	sBR->LinesBegin();
	sBR->LinesSetColor( mColor );

	sBR->BatchLine( line.V[0].x, line.V[0].y, line.V[1].x, line.V[1].y );

	DrawBatch();
}

void cPrimitives::DrawTriangle( const eeTriangle2f& t, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3 ) {
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

void cPrimitives::DrawTriangle( const eeTriangle2f& t ) {
	DrawTriangle( t, mColor, mColor, mColor );
}

void cPrimitives::DrawCircle( const eeVector2f& p, const Float& radius, Uint32 points ) {
	if ( 0 == points ) {
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

		std::vector<eeColorA> colors( circleVAR_count - 1 ,mColor );

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

	if(points < 6) points = 6;
	Float angle_shift =  360 / static_cast<Float>(points);

	sBR->SetTexture( NULL );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );
			sBR->LineLoopBegin();
			sBR->LineLoopSetColor( mColor );

			for( Float i = 0; i < 360; i+= ( angle_shift + angle_shift ) )
				sBR->BatchLineLoop( p.x + radius * Math::sinAng(i), p.y + radius * Math::cosAng(i), p.x + radius * Math::sinAng( i + angle_shift ), p.y + radius * Math::cosAng( i + angle_shift ) );

			break;
		}
		case DRAW_FILL:
		{
			sBR->TriangleFanBegin();
			sBR->TriangleFanSetColor( mColor );

			for( Float i = 0; i < 360; i+= ( angle_shift + angle_shift + angle_shift ) )
				sBR->BatchTriangleFan( p.x + radius * Math::sinAng(i), p.y + radius * Math::cosAng(i), p.x + radius * Math::sinAng( i + angle_shift ), p.y + radius * Math::cosAng( i + angle_shift ), p.x + radius * Math::sinAng( i + angle_shift + angle_shift ), p.y + radius * Math::cosAng( i + angle_shift + angle_shift ) );

			break;
		}
	}

	DrawBatch();
}

void cPrimitives::DrawRectangle( const eeRectf& R, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const Float& Angle, const eeVector2f& Scale ) {
	sBR->SetTexture( NULL );
	sBR->SetBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_FILL:
		{
			sBR->QuadsBegin();
			sBR->QuadsSetColorFree( TopLeft, BottomLeft, BottomRight, TopRight );

			eeSizef size = const_cast<eeRectf*>(&R)->Size();

			sBR->BatchQuadEx( R.Left, R.Top, size.Width(), size.Height(), Angle, Scale );
			break;
		}
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );

			sBR->LineLoopBegin();
			sBR->LineLoopSetColorFree( TopLeft, BottomLeft );

			if ( Scale != 1.0f || Angle != 0.0f ) {
				eeQuad2f Q( R );
				eeSizef size = const_cast<eeRectf*>(&R)->Size();

				Q.Scale( Scale );
				Q.Rotate( Angle, eeVector2f( R.Left + size.Width() * 0.5f, R.Top + size.Height() * 0.5f ) );

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

void cPrimitives::DrawRectangle( const eeRectf& R, const Float& Angle, const eeVector2f& Scale ) {
	DrawRectangle( R, mColor, mColor, mColor, mColor, Angle, Scale );
}

void cPrimitives::DrawRoundedRectangle( const eeRectf& R, const eeColorA& TopLeft, const eeColorA& BottomLeft, const eeColorA& BottomRight, const eeColorA& TopRight, const Float& Angle, const eeVector2f& Scale, const unsigned int& Corners ) {
	sBR->SetTexture( NULL );
	sBR->SetBlendMode( mBlendMode );

	unsigned int i;
	eeSizef size		= const_cast<eeRectf*>( &R )->Size();
	Float xscalediff	= size.Width()	* Scale.x - size.Width();
	Float yscalediff	= size.Height()	* Scale.y - size.Height();
	eeVector2f Center( R.Left + size.Width() * 0.5f + xscalediff, R.Top + size.Height() * 0.5f + yscalediff );
	eePolygon2f Poly	= eePolygon2f::CreateRoundedRectangle( R.Left - xscalediff, R.Top - yscalediff, size.Width() + xscalediff, size.Height() + yscalediff, Corners );
	eeVector2f poly;

	Poly.Rotate( Angle, Center );

	switch( mFillMode ) {
		case DRAW_FILL:
		{
			if ( TopLeft == BottomLeft && BottomLeft == BottomRight && BottomRight == TopRight ) {
				sBR->PolygonSetColor( TopLeft );

				sBR->BatchPolygon( Poly );
			} else {
				for ( i = 0; i < Poly.Size(); i++ ) {
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
				for ( i = 0; i < Poly.Size(); i+=2 ) {
					sBR->BatchLineLoop( Poly[i], Poly[i+1] );
				}
			} else {
				for ( unsigned int i = 0; i < Poly.Size(); i++ ) {
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

void cPrimitives::DrawRoundedRectangle( const eeRectf& R, const Float& Angle, const eeVector2f& Scale, const unsigned int& Corners ) {
	DrawRoundedRectangle( R, mColor, mColor, mColor, mColor, Angle, Scale, Corners );
}

void cPrimitives::DrawQuad( const eeQuad2f& q, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const eeColorA& Color4, const Float& OffsetX, const Float& OffsetY ) {
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

void cPrimitives::DrawQuad( const eeQuad2f& q, const Float& OffsetX, const Float& OffsetY ) {
	DrawQuad( q, mColor, mColor, mColor, mColor, OffsetX, OffsetY );
}

void cPrimitives::DrawPolygon( const eePolygon2f& p ) {
	sBR->SetTexture( NULL );
	sBR->SetBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->SetLineWidth( mLineWidth );

			sBR->LineLoopBegin();
			sBR->LineLoopSetColor( mColor );

			for ( Uint32 i = 0; i < p.Size(); i += 2 )
				sBR->BatchLineLoop( p.X() + p[i].x, p.Y() + p[i].y, p.X() + p[i+1].x, p.Y() + p[i+1].y );

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

void cPrimitives::DrawBatch() {
	if ( mForceDraw )
		sBR->Draw();
	else
		sBR->DrawOpt();
}

void cPrimitives::ForceDraw( const bool& force ) {
	mForceDraw = force;

	if ( force )
		DrawBatch();
}

const bool& cPrimitives::ForceDraw() const {
	return mForceDraw;
}

void cPrimitives::SetColor( const eeColorA& Color ) {
	mColor = Color;
}

void cPrimitives::FillMode( const EE_FILL_MODE& Mode ) {
	mFillMode = Mode;
}

const EE_FILL_MODE& cPrimitives::FillMode() const {
	return mFillMode;
}

void cPrimitives::BlendMode( const EE_BLEND_MODE& Mode ) {
	mBlendMode = Mode;
}

const EE_BLEND_MODE& cPrimitives::BlendMode() const {
	return mBlendMode;
}

void cPrimitives::LineWidth( const Float& width ) {
	mLineWidth = width;
}

const Float& cPrimitives::LineWidth() const {
	return mLineWidth;
}

}}
